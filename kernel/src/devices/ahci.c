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
static bool list_inited = false;

static void ahci_get_bar_size(ahci_controller_t* c);
static void ahci_map_uncacheable(ahci_controller_t* c);
static void ahci_reset_controller(ahci_controller_t* c);
static void ahci_controller_enable_int(ahci_controller_t* c, bool enable);
static void ahci_controller_handle_int(ahci_controller_t* c, uint32_t is);
static ahci_port_t* ahci_alloc_port_mem(ahci_controller_t* c, int port_num);
static void int_handler(registers_t* regs);

#define SPIN_WAIT 1000000

void init_ahci() {
    ahci_controller_t* c;

    printk("initalizing AHCI");

    if (list_empty(&ahci_controllers)) {
        printk("no AHCI controllers detected");
        return;
    }

    list_for_each_entry(c, &ahci_controllers) {
        init_controller(c);
    }

    // Only enable interrupts after all controllers have been initalized
    list_for_each_entry(c, &ahci_controllers) {
        ahci_controller_enable_int(c, true);
    }
}

bool ahci_add_controller(pci_device_t* pci_dev) {
    if (!list_inited) {
        ahci_controllers = LIST_HEAD_INIT(ahci_controllers);
        list_inited = true;
    }

    if (next_dev_id >= AHCI_MAX_CONTROLLERS_NUM) {
        printke("trying to add an AHCI controller with no spots left");
        return false;
    }

    ahci_controller_t* controller = kmalloc(sizeof(ahci_controller_t));

    if (!controller) {
        printke("unable to allocate memory for ahci controller with pci id: %d",
            pci_dev->id);
        return false;
    }

    controller->id = next_dev_id;
    next_dev_id++;
    controller->pci_dev = pci_dev;

    if (!list_add(&ahci_controllers, (void*) controller)) {
        printke("unable to add AHCI controller to list");
        kfree(controller);
    }

    return true;
}

static void ahci_get_bar_size(ahci_controller_t* c) {
    // Get BAR size
    uint32_t bar5, size;
    bar5 = c->pci_dev->header0.bar[5] & ~MM_BAR_INFO_MASK;

    // Write all 1s to indicate get address size
    pci_write_config_long(c->pci_dev->bus, c->pci_dev->dev, c->pci_dev->func,
        HDR0_BAR5, 0xFFFFFFFF);

    // Read back the value
    size = pci_read_config_long(c->pci_dev->bus, c->pci_dev->dev, c->pci_dev->func,
        HDR0_BAR5);

    // Calculation to get size
    size &= ~MM_BAR_INFO_MASK; // Mask information bits
    size = ~size;              // Invert
    size += 1;

    // Reset bar
    pci_write_config_long(c->pci_dev->bus, c->pci_dev->dev, c->pci_dev->func,
        HDR0_BAR5, bar5);

    bar5 &= ~MM_BAR_INFO_MASK; // Mask off the information bits
    c->base_address = (void*) bar5;
    c->address_size = size;
}

void ahci_enable_pci_int_mem_access(ahci_controller_t* controller) {
    pci_device_t* p = controller->pci_dev;
    uint16_t cmd = pci_read_config_word(p->bus, p->dev, p->func, 4);
    cmd |= 0b00000111;     // IO Space, Memory Space, Bus Master enable
    cmd &= ~0b10000000000; // Toggle off interrupt disable

    pci_write_config_word(p->bus, p->dev, p->func, 4, cmd);
}

/* Map base address into virtual memory, and mark as do not cache as the physical
 * address is in hardware.
 */
static void ahci_map_uncacheable(ahci_controller_t* c) {
    uint32_t num_pages = c->address_size / PAGE_SIZE;

    if (c->address_size % PAGE_SIZE != 0) {
        // Grab the remaining bottom page if the required memory isn't paged aligned
        num_pages++;
    }

    // NOTE: not sure how best to implement getting free virtual
    // memory in kernel space that can be mapped to hardware
    void* virt = kamalloc(num_pages * PAGE_SIZE, PAGE_SIZE);
    paging_unmap_pages((uintptr_t) virt, num_pages);

    // Disable caching this page because it is mmio
    paging_map_pages((uintptr_t) virt, (uintptr_t) c->base_address, num_pages,
        PAGE_CACHE_DISABLE | PAGE_RW);

    c->base_address_virt = virt;
}

void init_controller(ahci_controller_t* c) {
    HBA_ghc_t* ghc;
    int int_no = c->pci_dev->header0.int_line;

    ahci_get_bar_size(c);
    ahci_enable_pci_int_mem_access(c);
    ahci_map_uncacheable(c);

    ghc = (HBA_ghc_t*) c->base_address_virt;

    ahci_print_controller(c);

    ahci_reset_controller(c);
    irq_register_handler(IRQ_TO_INT(int_no), int_handler);

    // Check devices on available ports
    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        if (!((ghc->ports_implemented >> i) & 0x01))
            continue;

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

// At this point in time we will only allow one command at a time, no adding multiple
// commands to the command list, and only r/w one sector at a time.
static ahci_port_t* ahci_alloc_port_mem(ahci_controller_t* c, int n) {
    HBA_port_t* p = AHCI_GET_PORT(c, n);
    uint32_t size, cmdlist_phys_base;
    void* cmdlist_base;
    HBA_cmd_hdr_t* hdr;
    HBA_cmd_table_t* tbl;
    ahci_port_t* ahci_port;

    if (!ahci_stop_port(p))
        return NULL;

    //// Map out area for command list and FIS receive and data base address
    // Align on page so that there is no discontinuity in physical memory - size can't exceed one
    // page, cmdlist must be 1k aligned, and fis must be 256b aligned, cmdtbl must be 126b aligned
    //
    // In memory it looks like:
    //    COMMAND_LIST      (addr: BASE)
    //    FIS_RECEIVE       (addr: BASE+CMDLIST_SIZE)
    //    one COMMAND_TABLE (addr: FIS_REC+FIS_REC_SIZE)
    //    DATA_BASE         (addr: COMMAND_TABLE+CMD_TABLE_SIZE)
    //
    //    DATA_BASE size is one sata block
    //
    size = HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t) + SATA_BLOCK_SIZE;

    // If its greater than a page, we have an issue
    if (size > PAGE_SIZE) {
        printke("Issue allocating space for ahci dev: %d port %d, memory space larger than a page", c->id, n);
        return NULL;
    }

    cmdlist_base = kamalloc(size, PAGE_SIZE);
    if (!cmdlist_base) {
        printke("error allocating virt mem for ahci dev: %d port %d, cmdlist base", c->id, n);
        return NULL;
    }
    // Mark page as cache disable
    if (!paging_disable_page_cache(cmdlist_base)) {
        printke("error disabling page cache for ahci dev: %d port %d, cmdlist base", c->id, n);
        return NULL;
    }

    // Set command list base
    cmdlist_phys_base = (uint32_t) paging_virt_to_phys((uintptr_t) cmdlist_base);
    if (!cmdlist_phys_base) {
        printke("error getting physical memory location for ahci dev: %d port %d, cmdlist base", c->id, n);
        return NULL;
    }
    p->clb = cmdlist_phys_base;
    p->clbu = 0;

    // Set fis received base
    p->fb = cmdlist_phys_base + HBA_CMDLIST_SIZE;
    p->fbu = 0;

    // Check alignments
    if (cmdlist_phys_base & 0b1111111111) { // 1k align
        printke("Issue setting up ahci dev: %d port %d, cmdlist not aligned", c->id, n);
        return NULL;
    }
    if ((cmdlist_phys_base + HBA_CMDLIST_SIZE) & 0b11111111) { // 256 align
        printke("Issue setting up ahci dev: %d port %d, fis receive not aligned", c->id, n);
        return NULL;
    }
    if ((cmdlist_phys_base + HBA_CMDLIST_SIZE + FIS_REC_SIZE) & 0b1111111) { // 128 align
        printke("Issue setting up ahci dev: %d port %d, cmdtbl not aligned", c->id, n);
        return NULL;
    }

    //// Zero out memory space and then appropriately fill the data structures
    memset(cmdlist_base, 0, size);

    // Cmd hdr
    hdr = (HBA_cmd_hdr_t*) cmdlist_base;
    hdr->prdtl = 1;
    hdr->ctba = cmdlist_phys_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    hdr->ctbau = 0;

    // Prd
    tbl = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    tbl->prdt_entry[0].dba = cmdlist_phys_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t));
    tbl->prdt_entry[0].dbau = 0;
    tbl->prdt_entry[0].dbc = SATA_BLOCK_SIZE;
    tbl->prdt_entry[0].i = 1;

    ahci_start_port(p);

    // Create port structure and return it
    ahci_port = kmalloc(sizeof(ahci_port_t));
    ahci_port->port = p;
    ahci_port->c = c;
    ahci_port->port_num = n;
    ahci_port->cmdlist_base_virt = cmdlist_base;
    ahci_port->fis_rec_virt = cmdlist_base + (HBA_CMDLIST_SIZE);
    ahci_port->cmd_table_virt = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE);
    ahci_port->data_base_virt = cmdlist_base + (HBA_CMDLIST_SIZE + FIS_REC_SIZE + sizeof(HBA_cmd_table_t));

    return ahci_port;
}

static void ahci_reset_controller(ahci_controller_t* c) {
    HBA_ghc_t* ghc = (HBA_ghc_t*) c->base_address_virt;
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
    HBA_ghc_t* ghc = (HBA_ghc_t*) c->base_address_virt;

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

static void int_handler(registers_t* regs) {
    ahci_controller_t* c;

    UNUSED(regs);

    list_for_each_entry(c, &ahci_controllers) {
        HBA_ghc_t* ghc = (HBA_ghc_t*) c->base_address_virt;
        uint32_t is = ghc->int_status;
        ghc->int_status |= is;

        if (!is) {
            return;
        }

        printk("interrupt fired for AHCI controller: %d, ports: 0x%x", c->id, is);
        ahci_controller_handle_int(c, is);
    }
}

static void ahci_controller_handle_int(ahci_controller_t* c, uint32_t is) {
    for (int i = 0; i < AHCI_MAX_PORTS; i++) {
        if (((is >> i) & 0x01)) {
            HBA_port_t* p = AHCI_GET_PORT(c, i);
            p->is |= p->is;
        }
    }
}

void ahci_print_controller(ahci_controller_t* c) {
    HBA_ghc_t* ghc = (HBA_ghc_t*) c->base_address_virt;

    printk("AHCI controller with id: %d, pci id: %d", c->id, c->pci_dev->id);
    printk("- BAR: 0x%x", c->base_address);
    printk("- BAR virt: 0x%x", c->base_address_virt);
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
