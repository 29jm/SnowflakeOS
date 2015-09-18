#include <stdio.h>

#include <kernel/keyboard.h>
#include <kernel/irq.h>
#include <kernel/com.h>
#include <kernel/sys.h>

uint8_t keyboard_buffer[512];
uint32_t buf_length;

char sc_to_char[] = {
	0, 0,
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
	'[', ']', '\r', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
	';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm',
	',', '.', '/', 0, '*', 0, ' ',
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	'7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',
	0, 0, 0, 0, 0
};

key_state_t keys_state[256];

void init_keyboard() {
	irq_register_handler(IRQ1, &keyboard_handler);
}

void keyboard_handler(registers_t* regs) {
	UNUSED(regs);

	key_event_t key_event;
	uint8_t scancode = inportb(KBD_DATA);
	uint8_t sc = scancode & 0x7F;

	if (scancode & KEY_RELEASED) {
		keys_state[sc].pressed = 0;
		key_event.pressed = 0;
	}
	else {
		keys_state[sc].pressed = 1;
		key_event.pressed = 1;
	}

	key_event.key = sc;
	key_event.alt = keys_state[KEY_L_ALT].pressed;
	key_event.alt_gr = 0;
	key_event.shift = keys_state[KEY_L_SHIFT].pressed ||
					  keys_state[KEY_R_SHIFT].pressed;
	key_event.super = keys_state[KEY_SUPER].pressed;
	key_event.control = keys_state[KEY_L_CTRL].pressed;

	if (sc_to_char[scancode]) {
		printf("%c %s %s %s %s\n", sc_to_char[sc], key_event.alt ? "Alt" : "",
												key_event.shift ? "Shift" : "",
												key_event.control ? "Control" : "",
												key_event.super ? "Super" : "");
	}
}

void keyboard_wait() {
	while (inportb(KBD_STATUS) & (1 << 1));
}
