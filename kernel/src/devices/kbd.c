#include <kernel/kbd.h>
#include <kernel/ps2.h>
#include <kernel/irq.h>
#include <kernel/sys.h>

#include <stdio.h>
#include <ctype.h>

// Contains keycode mappings for one-byte scancodes
// A zero means the index isn't a valid scancode
uint32_t simple_sc_to_kc[] = {
	0, KBD_F9, 0, KBD_F5,
	KBD_F4, KBD_F1, KBD_F2, KBD_F12,
	0, KBD_F10, KBD_F8, KBD_F6,
	KBD_F4, KBD_TAB, KBD_BACKTICK, 0,
	0, KBD_LEFT_ALT, KBD_LEFT_SHIFT, 0,
	KBD_LEFT_CONTROL, KBD_Q, KBD_1, 0,
	0, 0, KBD_Z, KBD_S,
	KBD_A, KBD_W, KBD_2, 0,
	0, KBD_C, KBD_X, KBD_D,
	KBD_E, KBD_4, KBD_3, 0,
	0, KBD_SPACE, KBD_V, KBD_F,
	KBD_T, KBD_R, KBD_5, 0,
	0, KBD_N, KBD_B, KBD_H,
	KBD_G, KBD_Y, KBD_7, 0,
	0, 0, KBD_M, KBD_J,
	KBD_U, KBD_7, KBD_8, 0,
	0, KBD_COMMA, KBD_K, KBD_I,
	KBD_O, KBD_0, KBD_9, 0,
	0, KBD_PERIOD, KBD_SLASH, KBD_L,
	KBD_SEMICOLON, KBD_P, KBD_MINUS, 0,
	0, 0, KBD_TICK, 0,
	KBD_LEFT_SQUARE, KBD_EQUAL, 0, 0,
	KBD_CAPSLOCK, KBD_RIGHT_SHIFT, KBD_ENTER, KBD_RIGHT_SQUARE,
	0, KBD_BACKSLASH, 0, 0,
	0, 0, 0, 0,
	0, 0, KBD_BACKSPACE, 0,
	0, KBD_KP_1, 0, KBD_KP_4,
	KBD_KP_7, 0, 0, 0,
	KBD_KP_0, KBD_KP_PERIOD, KBD_KP_2, KBD_KP_5,
	KBD_KP_6, KBD_KP_8, KBD_ESCAPE, KBD_NUMLOCK,
	KBD_F11, KBD_KP_PLUS, KBD_KP_3, KBD_KP_MINUS,
	KBD_KP_MUL, KBD_KP_9, KBD_SCROLLLOCK, 0,
	0, 0, 0, KBD_F7
};

// Maps relevant scancodes to their printable ASCII counterparts
char kc_to_char[] = {
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'\t', ' ', ',', '.', '\x08', '\'', '`', '/', '\\', ';',
	'+', '-', '=', '[', ']', '\x0A', '\x1B',
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'.', '+', '-', '*',
	'/', '\x0A'
};

uint32_t device;
kbd_context_t context;
bool key_states[256] = { false };
kbd_event_t next_event;

void init_kbd(uint32_t dev) {
	device = dev;
	context = (kbd_context_t) {
		.state = KBD_NORMAL,
		.alt = false,
		.alt_gr = false,
		.shift = false,
		.super = false,
		.control = false
	};

	irq_register_handler(IRQ1, kbd_handler);

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

void kbd_handler(registers_t* regs) {
	UNUSED(regs);

	uint8_t sc = ps2_read(PS2_DATA);

	if (!kbd_process_byte(&context, sc, &next_event)) {
		return; // We're in the middle of a scancode
	}

	// We've received a full scancode
	key_states[next_event.key_code] = next_event.pressed;

	switch (next_event.key_code) {
		case KBD_LEFT_ALT:
			context.alt = next_event.pressed;
			break;
		case KBD_RIGHT_ALT:
			context.alt_gr = next_event.pressed;
			break;
		case KBD_LEFT_CONTROL:
		case KBD_RIGHT_CONTROL:
			context.control = next_event.pressed;
			break;
		case KBD_LEFT_SHIFT:
		case KBD_RIGHT_SHIFT:
			context.shift = next_event.pressed;
			break;
		case KBD_SUPER:
			context.super = next_event.pressed;
			break;
	}

	if (next_event.key_code <= KBD_KP_ENTER && next_event.pressed) {
		char c = kc_to_char[next_event.key_code];

		if (context.shift) {
			c = kbd_make_shift(c);
		}

		printf("%c", c);
	}
}

/* Interprets the received byte according to the current keyboard state and
 * previously received bytes.
 * Returns whether a full scancode was just received, in which case `event` is
 * filled with the corresponding key event.
 * Note: expects `event` not to be modified between calls.
 */
bool kbd_process_byte(kbd_context_t* ctx, uint8_t sc, kbd_event_t* event) {
	ctx->scancode[ctx->current++] = sc;

	switch (ctx->state) {
		case KBD_NORMAL:
			event->pressed = true;

			if (sc == 0xF0) {
				ctx->state = KBD_RELEASE_SHORT;
			} else if (sc == 0xE0 || sc == 0xE1) {
				ctx->state = KBD_CONTINUE;
			} else {
				ctx->current = 0;
				event->key_code = simple_sc_to_kc[sc];
			}

			break;
		case KBD_RELEASE_SHORT:
			ctx->state = KBD_NORMAL;
			ctx->current = 0;
			event->key_code = simple_sc_to_kc[sc];
			event->pressed = false;

			break;
		case KBD_CONTINUE:
			if (sc == 0xF0 && (ctx->current-1) == 1) {
				event->pressed = false;
				break;
			}

			if (kbd_is_valid_scancode(ctx->scancode, ctx->current,
					&event->key_code)) {
				ctx->state = KBD_NORMAL;
				ctx->current = 0;
			}

			break;
	}

	return ctx->state == KBD_NORMAL;
}

/* Returns whether `bytes` is a valid, complete multibyte scancode.
 * If it is, the corresponding key code is placed in `key_code`.
 * Note: hopefully this type of scancode is a "prefix code", otherwise this
 * is pure hell.
 */
bool kbd_is_valid_scancode(uint8_t* bytes, uint32_t len, uint32_t* key_code) {
	if (len < 2) {
		return false;
	}

	// Discard the 0xE0 prefix
	bytes = &bytes[1];
	len -= 1;

	// The 0xF0 prefix indicates a release event, discard it
	if (bytes[0] == 0xF0) {
		bytes = &bytes[1];
		len -= 1;
	}

	if (len == 1) {
		switch (bytes[0]) {
			case 0x75: *key_code = KBD_UP; return true;
			case 0x72: *key_code = KBD_DOWN; return true;
			case 0x6B: *key_code = KBD_LEFT; return true;
			case 0x74: *key_code = KBD_RIGHT; return true;
			case 0x4A: *key_code = KBD_KP_SLASH; return true;
			case 0x5A: *key_code = KBD_KP_ENTER; return true;
			case 0x69: *key_code = KBD_END; return true;
			case 0x6C: *key_code = KBD_HOME; return true;
			case 0x70: *key_code = KBD_INSERT; return true;
			case 0x71: *key_code = KBD_DELETE; return true;
			case 0x7D: *key_code = KBD_PAGE_UP; return true;
			case 0x7A: *key_code = KBD_PAGE_DOWN; return true;
			case 0x11: *key_code = KBD_RIGHT_ALT; return true;
			case 0x2F: *key_code = KBD_MENU; return true;
			case 0x1F: *key_code = KBD_SUPER; return true;
		}
	}

	if (len == 3) {
		if (bytes[0] == 0x12 && bytes[1] == 0xE0 && bytes[2] == 0x7C) {
			*key_code = KBD_PRINT_SCREEN;
			return true;
		}
	}

	if (len == 4) {
		if (bytes[0] == 0x7C && bytes[1] == 0xE0
				&& bytes[2] == 0xF0 && bytes[3] == 0x12) {
			*key_code = KBD_PRINT_SCREEN;
			return true;
		}
	}

	// Just assume the bytes are good, there's only one option here
	// It also means we'll never overflow our `bytes` buffer in the calling
	// function
	if (len == 7) {
		*key_code = KBD_PAUSE;
		return true;
	}

	return false;
}

bool kbd_is_key_pressed(uint32_t key_code) {
	return key_states[key_code];
}

/* Returns the shifted version of printable character `c` if applicable, and
 * `c` otherwise.
 */
char kbd_make_shift(char c) {
	if (c >= 'a' && c <= 'z') {
		return toupper(c);
	}

	switch (c) {
		case '`': return '~';
		case '1': return '!';
		case '2': return '@';
		case '3': return '#';
		case '4': return '$';
		case '5': return '%';
		case '6': return '^';
		case '7': return '&';
		case '8': return '*';
		case '9': return '(';
		case '0': return ')';
		case '-': return '_';
		case '=': return '+';
		case '[': return '{';
		case ']': return '}';
		case ';': return ':';
		case '\'': return '"';
		case ',': return '<';
		case '.': return '>';
		case '/': return '?';
		case '\\': return '|';
	}

	return c;
}