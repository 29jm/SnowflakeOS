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

#define KBD_RELEASE_SC 0xF0
#define KBD_CONTINUE_SC 0xE0

enum {
	// Next byte could be a new key, or the "release-key" byte
	KBD_NORMAL,
	// Next byte will be the scancode of a key that's been released
	KBD_RELEASING,
	// Next byte should be part of a multibyte scancode
	KBD_CONTINUE
};

typedef struct {
	uint32_t state;
	bool alt;
	bool alt_gr;
	bool shift;
	bool super;
	bool control;
} kbd_state_t;

void init_keyboard(uint32_t dev);
void keyboard_handler(registers_t* regs);

#endif