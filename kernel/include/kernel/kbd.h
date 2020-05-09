#pragma once

#include <kernel/isr.h>

#include <stdint.h>
#include <stdbool.h>

#include <kernel/uapi/uapi_kbd.h>

// "SSC" means "Scan Code Set" here, i.e. the set of bytes corresponding to
// physical keyboard keys.
#define KBD_SSC_CMD 0xF0
#define KBD_SSC_GET 0x00
#define KBD_SSC_2 0x02
#define KBD_SSC_3 0x03

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

typedef void (*kbd_callback_t)(kbd_event_t);

void init_kbd(uint32_t dev);
void kbd_handler(registers_t* regs);
bool kbd_process_byte(kbd_context_t* ctx, uint8_t sc, kbd_event_t* event);
bool kbd_is_valid_scancode(uint8_t* bytes, uint32_t len, uint32_t* key_code);
bool kbd_is_key_pressed(uint32_t key_code);
char kbd_make_shift(char c);
void kbd_set_callback(kbd_callback_t handler);