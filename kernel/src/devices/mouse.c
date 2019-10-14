#include <kernel/mouse.h>
#include <kernel/com.h>
#include <kernel/irq.h>
#include <kernel/sys.h>

#include <stdio.h>

uint32_t current_byte = 0;
uint32_t bytes_per_packet = 3;
uint8_t packet[4] = { 0 };

uint32_t device;
int32_t x, y;
bool left_pressed;
bool right_pressed;
bool middle_pressed;

/* Initializes a mouse plugged in controller `dev`.
 * Tries to enable as many of its features as possible.
 */
void init_mouse(uint32_t dev) {
    device = dev;
    x = 0;
    y = 0;
    left_pressed = false;
    right_pressed = false;

    irq_register_handler(IRQ12, mouse_handle_interrupt);

    // Enable features
    mouse_enable_scroll_wheel();
    mouse_enable_five_buttons();

    // Set mouse parameters
    mouse_set_sample_rate(80);
    mouse_set_resolution(0x00); // One unit per mm
    mouse_set_scaling(false);   // Disable acceleration

    // Start receiving IRQ12s
    ps2_write_device(device, PS2_DEV_ENABLE_SCAN);
    ps2_expect_ack();
}

/* Receives one byte from the mouse and packages it in `packet`. Once that
 * packet is full, calls `mouse_handle_packet`.
 */
void mouse_handle_interrupt(registers_t* regs) {
    UNUSED(regs);

    uint8_t byte = ps2_read(PS2_DATA);

    // Try to stay synchronized by discarding obviously out of place bytes
    if (current_byte == 0 && !(byte & MOUSE_ALWAYS_SET)) {
        return;
    }

    packet[current_byte] = byte;
    current_byte = (current_byte + 1) % bytes_per_packet;

    // We've received a full packet
    if (current_byte == 0) {
        mouse_handle_packet();
    }
}

void mouse_handle_packet() {
    uint8_t flags = packet[0];
    int32_t delta_x = (int32_t) packet[1];
    int32_t delta_y = (int32_t) packet[2];
    uint8_t extra = 0;

    // Superior mice send four bytes
    if (bytes_per_packet == 4) {
        extra = packet[3];

        if (extra & MOUSE_UNUSED_A || extra & MOUSE_UNUSED_B) {
            return; // Unused bits are set: beware
        }
    }

    // Packets with X or Y overflow are probably garbage
    if (flags & MOUSE_X_OVERFLOW || flags & MOUSE_Y_OVERFLOW) {
        return;
    }

    // Two's complement by hand
    if (flags & MOUSE_X_NEG) {
        delta_x |= 0xFFFFFF00;
    }

    if (flags & MOUSE_Y_NEG) {
        delta_y |= 0xFFFFFF00;
    }

    left_pressed = flags & MOUSE_LEFT;
    right_pressed = flags & MOUSE_RIGHT;
    middle_pressed = flags & MOUSE_MIDDLE;

    x += delta_x;
    y -= delta_y; // Point the y-axis downward

	printf("\x1B[s\x1B[23;65H");
	printf("\x1B[K");
	printf("%d;%d", x, y);
	printf("\x1B[u");
}

void mouse_set_sample_rate(uint8_t rate) {
    ps2_write_device(device, MOUSE_SET_SAMPLE);
    ps2_expect_ack();
    ps2_write_device(device, rate);
    ps2_expect_ack();
}

void mouse_set_resolution(uint8_t level) {
    ps2_write_device(device, MOUSE_SET_RESOLUTION);
    ps2_expect_ack();
    ps2_write_device(device, level);
    ps2_expect_ack();
}

void mouse_set_scaling(bool enabled) {
    uint8_t cmd = enabled ? MOUSE_ENABLE_SCALING : MOUSE_DISABLE_SCALING;

    ps2_write_device(device, cmd);
    ps2_expect_ack();
}

/* Uses a magic sequence to enable scroll wheel support.
 */
void mouse_enable_scroll_wheel() {
    mouse_set_sample_rate(200);
    mouse_set_sample_rate(100);
    mouse_set_sample_rate(80);

    uint32_t type = ps2_identify_device(device);

    if (type == PS2_MOUSE_SCROLL_WHEEL) {
        bytes_per_packet = 4;
        printf("[MOUSE] Enabled scroll wheel\n");
    } else {
        printf("[MOUSE] Unable to enable scroll wheel\n");
    }
}

/* Uses a magic sequence to enable five buttons support.
 */
void mouse_enable_five_buttons() {
    if (bytes_per_packet != 4) {
        return;
    }

    mouse_set_sample_rate(200);
    mouse_set_sample_rate(200);
    mouse_set_sample_rate(80);

    uint32_t type = ps2_identify_device(device);

    if (type != PS2_MOUSE_FIVE_BUTTONS) {
        printf("[MOUSE] Mouse has fewer than five buttons\n");
    } else {
        printf("[MOUSE] Five buttons enabled\n");
    }
}