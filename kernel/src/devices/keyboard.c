#include <kernel/keyboard.h>
#include <kernel/irq.h>
#include <kernel/sys.h>

#include <stdio.h>

uint32_t device;
kbd_state_t state;
bool key_states[256] = { false };

void init_keyboard(uint32_t dev) {
	device = dev;
	state = (kbd_state_t) {
		.state = KBD_NORMAL,
		.alt = false,
		.alt_gr = false,
		.shift = false,
		.super = false,
		.control = false
	};

	irq_register_handler(IRQ1, keyboard_handler);

	// Get the current scancode set
	ps2_write_device(device, KBD_SSC_CMD);
	ps2_expect_ack();
	ps2_write_device(device, KBD_SSC_GET);
	ps2_expect_ack();
	uint8_t scancode_set = ps2_read(PS2_DATA);

	if (scancode_set != KBD_SSC_2) {
		printf("[KBD] Wrong scancode set (%d), TODO\n", scancode_set);
	}

	// Start receiving IRQ1s
	ps2_write_device(device, PS2_DEV_ENABLE_SCAN);
	ps2_expect_ack();
}

void keyboard_handler(registers_t* regs) {
	UNUSED(regs);

	uint8_t sc = ps2_read(PS2_DATA);

	// Replacing this with a switch causes garbage scan codes if I don't add
	// a `printf("%d", sc)` of sorts before. I must have run into UB.
	if (state.state == KBD_NORMAL) {
		if (sc == KBD_RELEASE_SC) {
			state.state = KBD_RELEASING;
			return;
		}

		// if (sc == KBD_CONTINUE_SC) {
		// 	state.state = KBD_CONTINUE;
		// }

		key_states[sc] = true;
		printf("%x down\n", sc);
	} else if (state.state == KBD_RELEASING) {
		state.state = KBD_NORMAL;
		key_states[sc] = false;
		printf("%x up\n", sc);
	} else if (state.state == KBD_CONTINUE) {

	}
}