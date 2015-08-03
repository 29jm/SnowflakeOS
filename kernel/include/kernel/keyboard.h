#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <kernel/irq.h>

typedef enum {
	KEY_ESC = 1,
	KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
	KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_TAB,
	KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
	KEY_L_BRACKET, KEY_R_BRACKET, KEY_ENTER, KEY_L_CTRL,
	KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L,
	KEY_SEMICOLON, KEY_SINGLE_QUOTE, KEY_BACKTICK, KEY_L_SHIFT, KEY_BACKSLASH,
	KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M,
	KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_R_SHIFT, KEY_KP_MULTIPLY,
	KEY_L_ALT, KEY_SPACE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
	KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_NUMLOCK, KEY_SCROLLLOCK,
	KEY_KP_7, KEY_KP_8, KEY_KP_9, KEY_KP_4, KEY_KP_5, KEY_KP_6, KEY_KP_1, KEY_KP_2,
	KEY_KP_3, KEY_KP_0, KEY_KP_PERIOD,
	KEY_F11 = 0x57, KEY_F12, KEY_SUPER = 0x5B
} key_t;

typedef struct {
	int pressed;
} key_state_t;

typedef struct {
	key_t key;
	int pressed;
	int alt;
	int alt_gr;
	int shift;
	int super;
	int control;
} key_event_t;

void init_keyboard();
void keyboard_handler(registers_t* regs);
void keyboard_wait();

#define KEY_RELEASED (1 << 7)

#define KBD_DATA 0x60
#define KBD_STATUS 0x64 // Read
#define KBD_CMD 0x64    // Write

#define KBD_ACK 0xFA
#define KBD_RESEND 0xFE

#endif
