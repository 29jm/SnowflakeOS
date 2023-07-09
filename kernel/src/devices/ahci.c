#include <kernel/ahci.h>
#include <kernel/irq.h>
#include <kernel/paging.h>
#include <kernel/pci.h>
#include <kernel/sata.h>
#include <kernel/sys.h>

#include <list.h>
#include <stdlib.h>
#include <string.h>

static list_t ahci_controllers;
static uint32_t next_dev_id = 0;
static bool list_initialized = false;

static void ahci_get_abar(ahci_controller_t* c);
static void ahci_map_abar(ahci_controller_t* c);
static void ahci_reset_controller(ahci_controller_t* c);
static void ahci_controller_enable_int(ahci_controller_t* c, bool enable);
static void ahci_controller_handle_int(ahci_controller_t* c, uint32_t is);
static ahci_port_t* ahci_alloc_port_mem(ahci_controller_t* c, int port_num);
static void ahci_interrupt_handler(registers_t* regs);

#define SPIN_WAIT 1000000

void init_ahci() {
    ahci_controller_t* c;

    if (!list_initialized || list_empty(&ahci_controllers)) {
        printk("no AHCI controllers detected");
        return;
    }

    printk("initalizing AHCI controllers");

    list_for_each_entry(c, &ahci_controllers) {
        init_controller(c);
    }

    // Only enable interrupts after all controllers have been initalized
    list_for_each_entry(c, &ahci_controllers) {
        ahci_controller_enable_int(c, true);
    }
}

/* Places each AHCI controller identified on the PCI bus into a list of AHCI
 * controllers.
 */
bool ahci_add_controller(pci_device_t* pci_dev) {
    if (!list_initialized) {
        ahci_controllers = LIST_HEAD_INIT(ahci_controllers);
        list_initialized = true;
    }

    if (next_dev_id >= AHCI_MAX_CONTROLLERS_NUM) {
        printke("trying to add an AHCI controller with no spots left");
        return false;
    }

    if (pci_dev->hdr_type != PCI_DEV_GENERAL) {
        printke("unsupported PCI header type for an AHCI controller");
        return false;
    }

    ahci_controller_t* controller = kmalloc(sizeof(ahci_controller_t));

    if (!controller) {
        printke("unable to allocate memory for ahci controller with PCI id: %d", pci_dev->id);
        return false;
    }

    controller->id = next_dev_id++;
    controller->pci_dev = pci_dev;

    if (!list_add(&ahci_controllers, (void*) controller)) {
        printke("unable to add AHCI controller to list");
        kfree(controller);
    }

    return true;
}

/* Gets characteristics of the AHCI Base Address Register (ABAR), its address
 * and size.
 * Fills the 'abar_phys' and 'address_size' fields of the given controller
 * with the address in its BAR5 and the size of the address space covered by
 * BAR5 respectively.
 */
static void ahci_get_abar(ahci_controller_t* c) {
    uint32_t bar5, size;

    // Base address
    bar5 = c->pci_dev->header0.bar[5] & ~MM_BAR_INFO_MASK;

    // Write all 1s to BAR5, see what sticks, that'll be the max address
    pci_write_config_long(c->pci_dev->bus, c->pci_dev->dev, c->pci_dev->func,
        HDR0_BAR5, 0xFFFFFFFF);

    // Read BAR5 back and compute its size
    // See: https://wiki.osdev.org/PCI#Address_and_size_of_the_BAR
    size = pci_read_config_long(c->pci_dev->bus, c->pci_dev->dev, c->pci_dev->func, HDR0_BAR5);
    size &= ~MM_BAR_INFO_MASK; // Mask information bits
    size = ~size;              // Invert
    size += 1;                 // Add one

    // Restore BAR5 to its initial value
    pci_write_config_long(c->pci_dev->bus, c->pci_dev->dev, c->pci_dev->func, HDR0_BAR5, bar5);

    // Store BAR5's address and size in the passed structure
    bar5 &= ~MM_BAR_INFO_MASK;
    c->abar_phys = (void*) bar5;
    c->address_size = size;
}

/* Enables interrupts from the given AHCI controller and makes it respond to
 * memory/IO space accesses.
 * Does so by writing to the command register of its PCI device.
 * See: https://wiki.osdev.org/PCI#Command_Register
 */
void ahci_enable_pci_interrupts(ahci_controller_t* controller) {
    pci_device_t* p = controller->pci_dev;
    uint16_t cmd = pci_read_config_word(p->bus, p->dev, p->func, 4);
    cmd |= 0b00000111;     // IO Space, Memory Space, Bus Master enable
    cmd &= ~0b10000000000; // Toggle off interrupt disable

    pci_write_config_word(p->bus, p->dev, p->func, 4, cmd);
}

/* Maps the ABAR into virtual memory, fills 'abar_virt' of the given
 * AHCI controller.
 */
static void ahci_map_abar(ahci_controller_t* c) {
    uint32_t num_pages = divide_up(c->address_size, PAGE_SIZE);

    // Get a virtual address where to map the ABAR
    void* virt = kamalloc(num_pages * PAGE_SIZE, PAGE_SIZE);

    // Disable page caching, as reads and writes talk to the hardware
    paging_unmap_pages((uintptr_t) virt, num_pages);
    paging_map_pages((uintptr_t) virt, (uintptr_t) c->abar_phys, num_pages,
        PAGE_CACHE_DISABLE | PAGE_RW);

    c->abar_virt = virt;
}

void init_controller(ahci_controller_t* c) {
    HBA_ghc_t* ghc;
    int int_no = c->pci_dev->header0.int_line;

    ahci_get_abar(c);
    irq_register_handler(IRQ_TO_INT(int_no), ahci_interrupt_handler);
    ahci_enable_pci_interrupts(c);
    ahci_map_abar(c);

    ghc = (HBA_ghc_t*) c->abar_virt;

    ahci_print_controller(c);
    ahci_reset_controller(c);

    // Check devices on available ports
    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        if (!((ghc->ports_implemented >> i) & 0x01)) {
            continue;
        }

        // Check port device status and type
        HBA_port_t* p = AHCI_GET_PORT(c, i);

        if ((p->ssts & 0xF) == 0x03) { // Device present and connection established
            ahci_port_t* port = ahci_alloc_port_mem(c, i);
            ahci_reset_port(port->port);

            // Enable interrupts for the port
            port->port->ie |= 0x1;

            uint32_t device_type = p->sig;
            switch (device_type) {
            case SATA_SIG_ATA:
                sata_add_device(port, SATA_DEV_SATA); // Regular sata device
                break;
            case SATA_SIG_ATAPI:
                sata_add_device(port, SATA_DEV_SATAPI); // Sata packet interface device
                break;
            case SATA_SIG_SEMB:
                sata_add_device(port, SATA_DEV_SEMB); // Some sort of bridge
                break;
            case SATA_SIG_PM:
                sata_add_device(port, SATA_DEV_PM); // Sata port multiplier
                break;
            default:
                printke("couldn't determine sata device type, provided type was: 0x%x", device_type);
                break;
            }
        }
    }
}

/* Allocates memory for the data structures of a port:
 * - Command List  - Contains one command header, pointing to the Command Table
 * - FIS Received  - Sent by the device when it has responded
 * - Command Table - Contains eight PRDTs, pointing nowhere. Setting these up
 *                   is the job of the SATA driver.
 *
 * Note: Can only process one command at a time with the current design.
 */
static ahci_port_t* ahci_alloc_port_mem(ahci_controller_t* c, int n) {
    HBA_port_t* p = AHCI_GET_PORT(c, n);
    uint32_t size, cmdlist_phys_base;
    void* cmdlist_base;
    HBA_cmd_hdr_t* hdr;
    ahci_port_t* ahci_port;
    const uint32_t cmd_table_size = sizeof(HBA_cmd_table_t) + (AHCI_PRDT_COUNT-1)*sizeof(HBA_prdt_entry_t);

    if (!ahci_stop_port(p)) {
        printk("failed to stop command processing for port %d", n);
        return NULL;
    }

    // Allocate an area for all port data structures, with the limitation of only one command table
    // with one PRDT of 512 bytes.
    // The memory layout is as follows:
    //    Command List    (addr: base, page aligned)
    //    FIS Received    (addr: Command List+CMDLIST_SIZE, 256B aligned)
    //    Command Table   (addr: FIS Received+FIS_REC_SIZE, 128B aligned)

    size = HBA_CMDLIST_SIZE + FIS_REC_SIZE + cmd_table_size;

    // If its greater than a page, we have an issue
    if (size > PAGE_SIZE) {
        printke("Issue allocating space for ahci dev: %d port %d, memory space larger than a page", c->id, n);
        return NULL;
    }

    cmdlist_base = kamalloc(size, PAGE_SIZE); // The physical pages will be aligned too
    if (!cmdlist_base) {
        printke("error allocating virt mem for ahci dev: %d port %d, cmdlist base", c->id, n);
        return NULL;
    }

    // Mark page as cache disable
    if (!paging_disable_page_cache(cmdlist_base)) {
        printke("error disabling page cache for ahci dev: %d port %d, cmdlist base", c->id, n);
        return NULL;
    }

    // Set the port's Command List address
    cmdlist_phys_base = (uint32_t) paging_virt_to_phys((uintptr_t) cmdlist_base);
    p->clb = cmdlist_phys_base;
    p->clbu = 0;

    // Set the port's FIS Received address
    p->fb = cmdlist_phys_base + HBA_CMDLIST_SIZE;
    p->fbu = 0;

    // Zero out every port data structure we just allocated
    memset(cmdlist_base, 0, size);

    // Configure one command header in the Command List
    hdr = (HBA_cmd_hdr_t*) cmdlist_base;
    hdr->ctba = cmdlist_phys_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    hdr->ctbau = 0;

    // Enable port operations - none are defined at this point
    ahci_start_port(p);

    // Create an AHCI port structure and return it
    ahci_port = kmalloc(sizeof(ahci_port_t));
    ahci_port->port = p;
    ahci_port->ahci_controller = c;
    ahci_port->port_num = n;
    ahci_port->command_list = cmdlist_base;
    ahci_port->fis_received = cmdlist_base + (HBA_CMDLIST_SIZE);
    ahci_port->command_table = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);

    return ahci_port;
}

static void ahci_reset_controller(ahci_controller_t* c) {
    HBA_ghc_t* ghc = (HBA_ghc_t*) c->abar_virt;
    uint32_t spin = 0;

    ghc->ghc |= AHCI_ENABLE;

    // reset controller
    ghc->ghc |= (1 << 0);

    // wait for reset to finish
    while (spin < SPIN_WAIT && (ghc->ghc & 0x1)) {
        spin++;
    }

    if (spin > SPIN_WAIT) {
        printke("Controller stuck during reset");
    }

    ghc->ghc |= AHCI_ENABLE;
}

static void ahci_controller_enable_int(ahci_controller_t* c, bool enable) {
    HBA_ghc_t* ghc = (HBA_ghc_t*) c->abar_virt;

    if (enable) {
        ghc->ghc |= (1 << 1);
    } else {
        ghc->ghc &= ~(1 << 1);
    }
}

bool ahci_reset_port(HBA_port_t* p) {
    uint32_t spin = 0;

    if (!ahci_stop_port(p)) {
        return false;
    }

    p->sctl |= 0x1;

    // wait enough time for COMRESET to send
    while (spin < SPIN_WAIT) {
        spin++;
    }

    p->sctl &= ~0x1;

    spin = 0;
    while ((p->ssts & 0xF) != 0x03) {
        spin++;

        if (spin > SPIN_WAIT) {
            printke("Port stuck during reset");
            return false;
        }
    }

    ahci_port_clear_err(p);

    ahci_start_port(p);

    return true;
}

void ahci_start_port(HBA_port_t* p) {
    // enable processing command list
    p->cmd |= HBA_PORT_CMD_ST;

    // enable recieving FIS
    p->cmd |= HBA_PORT_CMD_FRE;
}

bool ahci_stop_port(HBA_port_t* p) {
    int spin = 0;

    // Disable processing command list
    p->cmd &= ~HBA_PORT_CMD_ST;

    // Disable recieving FIS
    p->cmd &= ~HBA_PORT_CMD_FRE;

    // Wait for the FIS and command list engine to complete
    while (true) {
        spin++;

        if (spin > SPIN_WAIT) {
            printke("port is stuck");
            return false;
        }

        if (p->cmd & HBA_PORT_CMD_FR)
            continue;
        if (p->cmd & HBA_PORT_CMD_CR)
            continue;

        break;
    }

    return true;
}

void ahci_port_clear_err(HBA_port_t* p) {
    p->serr = 0x07ff0fff; // all 1s but ignoring reserved bits
}

static void ahci_interrupt_handler(registers_t* regs) {
    ahci_controller_t* c;

    UNUSED(regs);

    list_for_each_entry(c, &ahci_controllers) {
        HBA_ghc_t* ghc = (HBA_ghc_t*) c->abar_virt;
        uint32_t is = ghc->int_status;

        ghc->int_status |= is; // Write it back to ACK

        if (!is) {
            return;
        }

        // printk("interrupt fired for AHCI controller: %d, ports: 0x%x", c->id, is);
        ahci_controller_handle_int(c, is);
    }
}

static void ahci_controller_handle_int(ahci_controller_t* c, uint32_t is) {
    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        if ((is >> i) & 0x01) {
            HBA_port_t* p = AHCI_GET_PORT(c, i);
            p->is |= p->is;
        }
    }
}

void ahci_print_controller(ahci_controller_t* c) {
    HBA_ghc_t* ghc = (HBA_ghc_t*) c->abar_virt;

    printk("AHCI controller with id: %d, pci id: %d", c->id, c->pci_dev->id);
    printk("- BAR: 0x%x", c->abar_phys);
    printk("- BAR virt: 0x%x", c->abar_virt);
    printk("- BAR size: 0x%x", c->address_size);
    printk("- version: 0x%x", ghc->version);
    printk("- capabilities: 0x%x, %d control slots per port",
        ghc->cap1, ((ghc->cap1 >> 8) & 0b1111) + 1);
}

void ahci_print_all_controllers() {
    ahci_controller_t* c;
    list_for_each_entry(c, &ahci_controllers) {
        ahci_print_controller(c);
    }
}
