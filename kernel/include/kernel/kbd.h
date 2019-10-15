#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <kernel/ps2.h>
#include <kernel/irq.h>

#include <stdbool.h>

// "SSC" means "Scan Code Set" here, i.e. the set of bytes corresponding to
// physical keyboard keys.
#define KBD_SSC_CMD 0xF0
#define KBD_SSC_GET 0x00
#define KBD_SSC_2 0x02
#define KBD_SSC_3 0x03

// Key codes
enum {
	KBD_A, KBD_B, KBD_C, KBD_D, KBD_E, KBD_F, KBD_G, KBD_H, KBD_I, KBD_J, KBD_K,
	KBD_L, KBD_M, KBD_N, KBD_O, KBD_P, KBD_Q, KBD_R, KBD_S, KBD_T, KBD_U, KBD_V,
	KBD_W, KBD_X, KBD_Y, KBD_Z,
	KBD_1, KBD_2, KBD_3, KBD_4, KBD_5,
	KBD_6, KBD_7, KBD_8, KBD_9, KBD_0,
	KBD_TAB, KBD_SPACE, KBD_COMMA, KBD_PERIOD, KBD_BACKSPACE,
	KBD_TICK, KBD_BACKTICK,
	KBD_SLASH, KBD_BACKSLASH, KBD_SEMICOLON,
	KBD_PLUS, KBD_MINUS, KBD_EQUAL,
	KBD_LEFT_SQUARE, KBD_RIGHT_SQUARE,
	KBD_ENTER, KBD_ESCAPE,
	KBD_KP_1, KBD_KP_2, KBD_KP_3, KBD_KP_4, KBD_KP_5,
	KBD_KP_6, KBD_KP_7, KBD_KP_8, KBD_KP_9, KBD_KP_0,
	KBD_KP_PERIOD, KBD_KP_PLUS, KBD_KP_MINUS, KBD_KP_MUL,
	KBD_KP_SLASH, KBD_KP_ENTER,
	KBD_LEFT_ALT, KBD_RIGHT_ALT, KBD_LEFT_SHIFT,
	KBD_LEFT, KBD_RIGHT, KBD_UP, KBD_DOWN,
	KBD_F1, KBD_F2, KBD_F3, KBD_F4, KBD_F5, KBD_F6,
	KBD_F7, KBD_F8, KBD_F9, KBD_F10, KBD_F11, KBD_F12,
	KBD_RIGHT_SHIFT, KBD_LEFT_CONTROL, KBD_RIGHT_CONTROL,
	KBD_CAPSLOCK, KBD_NUMLOCK, KBD_SCROLLLOCK,
	KBD_PRINT_SCREEN, KBD_PAUSE, KBD_SUPER, KBD_MENU,
	KBD_HOME, KBD_INSERT, KBD_DELETE, KBD_END,
	KBD_PAGE_DOWN, KBD_PAGE_UP
};

// Possible keyboard transmission states
enum {
	// Next byte could be a one byte scancode, a release indicator preceding
	// a one-byte scancode, or a multibyte scancode indicator
	KBD_NORMAL,
	// Next byte is the one-byte scancode released
	KBD_RELEASE_SHORT,
	// Next bytes should be part of a multibyte scancode
	KBD_CONTINUE
};

typedef struct {
	uint32_t key_code;
	bool pressed;
} kbd_event_t;

typedef struct {
	uint32_t state;
	uint8_t scancode[8];
	uint32_t current;
	bool alt;
	bool alt_gr;
	bool shift;
	bool super;
	bool control;
} kbd_context_t;

void init_kbd(uint32_t dev);
void kbd_handler(registers_t* regs);
bool kbd_process_byte(kbd_context_t* ctx, uint8_t sc, kbd_event_t* event);
bool kbd_is_valid_scancode(uint8_t* bytes, uint32_t len, uint32_t* key_code);
bool kbd_is_key_pressed(uint32_t key_code);
char kbd_make_shift(char c);

#endif