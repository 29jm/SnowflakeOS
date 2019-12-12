#include <kernel/ps2.h>
#include <kernel/com.h>
#include <kernel/mouse.h>
#include <kernel/kbd.h>

#include <stdio.h>

// Describes the available PS/2 controllers
bool controllers[] = { true, true };

/* Performs discovery and initialisation of PS/2 controllers and devices.
 * A lot of redundancy here because of poorly-indexed identifiers.
 */
void init_ps2() {
	printf("[PS2] Initializing PS/2 devices\n");

	bool dual_channel = true;

	CLI();

	// Disable both PS/2 device ports
	// Even if only one is present, disabling the second is harmless
	ps2_write(PS2_CMD, PS2_DISABLE_FIRST);
	ps2_write(PS2_CMD, PS2_DISABLE_SECOND);

	// Flush output bufffer: if the controller had anything to say, ignore it
	inportb(PS2_DATA);

	// Get the controller configuration byte
	ps2_write(PS2_CMD, PS2_READ_CONFIG);
	uint8_t config = ps2_read(PS2_DATA);

	// Check the basics
	config |= PS2_CFG_SYSTEM_FLAG;

	if (config & PS2_CFG_MUST_BE_ZERO) {
		printf("[PS2] Invalid bit set in configuration byte\n");
	}

	// Disable interrupts and scan code translation
	config &= ~(PS2_CFG_FIRST_PORT | PS2_CFG_SECOND_PORT | PS2_CFG_TRANSLATION);

	// Update configuration byte
	ps2_write(PS2_CMD, PS2_WRITE_CONFIG);
	ps2_write(PS2_DATA, config);

	// Controller self-test
	ps2_write(PS2_CMD, PS2_SELF_TEST);

	if (ps2_read(PS2_DATA) != PS2_SELF_TEST_OK) {
		printf("[PS2] Controller failed self-test\n");

		controllers[0] = false;
		controllers[1] = false;

		return;
	}

	// The last write may have reset our controller:
	// better reset our configuration byte just in case
	ps2_write(PS2_CMD, PS2_WRITE_CONFIG);
	ps2_write(PS2_DATA, config);

	// Check if we _really_ have two channels
	ps2_write(PS2_CMD, PS2_ENABLE_SECOND);
	ps2_write(PS2_CMD, PS2_READ_CONFIG);
	config = ps2_read(PS2_DATA);

	if (config & PS2_CFG_SECOND_CLOCK) {
		printf("[PS2] Only one PS/2 controller\n");
		dual_channel = false;
		controllers[1] = false;
	} else {
		ps2_write(PS2_CMD, PS2_DISABLE_SECOND); // Re-disable the second controller
	}

	// Test the controllers
	ps2_write(PS2_CMD, PS2_TEST_FIRST);

	if (ps2_read(PS2_DATA) != PS2_TEST_OK) {
		printf("[PS2] First PS/2 port failed testing\n");
		controllers[0] = false;
	}

	if (dual_channel) {
		ps2_write(PS2_CMD, PS2_TEST_SECOND);

		if (ps2_read(PS2_DATA) != PS2_TEST_OK) {
			printf("[PS2] Second PS/2 port failed testing\n");
			controllers[1] = false;
		}
	}

	// Enable available ports
	if (controllers[0]) {
		ps2_write(PS2_CMD, PS2_ENABLE_FIRST);
		config |= PS2_CFG_FIRST_PORT;
		config &= ~PS2_CFG_FIRST_CLOCK;
	}

	if (controllers[1]) {
		ps2_write(PS2_CMD, PS2_ENABLE_SECOND);
		config |= PS2_CFG_SECOND_PORT;
		config &= ~PS2_CFG_SECOND_CLOCK;
	}

	// Enable interrupts from detected controllers
	ps2_write(PS2_CMD, PS2_WRITE_CONFIG);
	ps2_write(PS2_DATA, config);

	// Reset devices
	for (uint32_t i = 0; i < 2; i++) {
		if (!controllers[i]) {
			continue;
		}

		ps2_write_device(i, PS2_DEV_RESET);
		uint8_t ret = ps2_read(PS2_DATA);

		// If it fails, disable the device's port
		if (ret != PS2_DEV_ACK || ps2_read(PS2_DATA) != PS2_DEV_RESET_ACK) {
			printf("[PS2] Failed to reset device %d\n", i);

			controllers[i] = false;
			config &= ~(i == 0 ? PS2_CFG_FIRST_PORT : PS2_CFG_SECOND_PORT);

			ps2_write(PS2_CMD, PS2_WRITE_CONFIG);
			ps2_write(PS2_DATA, config);
		}

		// For some reason, mice send an additional 0x00 byte
		if (ps2_can_read()) {
			ps2_read(0x60);
		}
	}

	for (uint32_t i = 0; i < 2; i++) {
		if (controllers[i]) {
			uint32_t type = ps2_identify_device(i);

			switch (type) {
				case PS2_KEYBOARD:
				case PS2_KEYBOARD_TRANSLATED:
					printf("[PS2] Keyboard\n");
					init_kbd(i);
					break;
				case PS2_MOUSE:
				case PS2_MOUSE_SCROLL_WHEEL:
				case PS2_MOUSE_FIVE_BUTTONS:
					printf("[PS2] Mouse\n");
					init_mouse(i);
			}
		}
	}

	STI();
}

/* Asks the device to identify itself, returns an enum value.
 * Useful as some devices's identities are several bytes long.
 */
uint32_t ps2_identify_device(uint32_t num) {
	ps2_write_device(num, PS2_DEV_DISABLE_SCAN); // Disables scanning
	ps2_expect_ack();
	ps2_write_device(num, PS2_DEV_IDENTIFY); // Identify
	ps2_expect_ack();

	uint32_t first_id_byte = ps2_read(PS2_DATA);
	uint32_t second_id_byte = ps2_read(PS2_DATA);

	return ps2_identity_bytes_to_type(first_id_byte, second_id_byte);
}

/* Converts the (one or two) bytes sent by a PS/2 device in response to an
 * `identity` request to a code representing the device type.
 */
uint32_t ps2_identity_bytes_to_type(uint8_t first, uint8_t second) {
	if (first == 0x00 || first == 0x03 || first == 0x04) {
		return first; // PS2_MOUSE* match the first byte
	} else if (first == 0xAB) {
		if (second == 0x41 || second == 0xC1) {
			return PS2_KEYBOARD_TRANSLATED;
		} else if (second == 0x83) {
			return PS2_KEYBOARD;
		}
	}

	return PS2_DEVICE_UNKNOWN;
}

/* Loops until the controller's input buffer is empty or our timer has expired.
 * Call this function before writing to the controller's command port, PS2_CMD.
 */
bool ps2_wait_write() {
	int timer = PS2_TIMEOUT;

	while ((inportb(0x64) & 2) && timer-- > 0) {
		asm ("pause");
	}

	return timer != 0;
}

/* Loops until data is available in the controller's output buffer or until our
 * timer has expired.
 * Call this function before reading from the controller's data port, PS2_DATA.
 */
bool ps2_wait_read() {
	int timer = PS2_TIMEOUT;

	while (!(inportb(0x64) & 1) && timer-- >= 0) {
		asm ("pause");
	}

	return timer != 0;
}

/* Returns the first byte of data available from `port`.
 * Returns `(uint8_t) -1` on read error, and also when this value was
 * legitimately read. TODO: room for improvement.
 */
uint8_t ps2_read(uint32_t port) {
	if (ps2_wait_read()) {
		return inportb(port);
	}

	printf("[PS2] Read failed\n");

	return -1;
}

/* Tries to write a byte on the specified port, and returns whether the
 * operation was successful.
 */
bool ps2_write(uint32_t port, uint8_t b) {
	if (ps2_wait_write()) {
		outportb(port, b);
		return true;
	}

	printf("[PS2] Write failed\n");

	return false;
}

/* Write a byte to the specified `device` input buffer.
 * This function is used to send command to devices.
 */
bool ps2_write_device(uint32_t device, uint8_t b) {
	if (device != 0) {
		if (!ps2_write(PS2_CMD, PS2_WRITE_SECOND)) {
			return false;
		}
	}

	return ps2_write(PS2_DATA, b);
}

/* Returns true if a device replied with `PS2_DEV_ACK`.
 * This is usually in reply to a command sent to that device.
 */
bool ps2_expect_ack() {
	uint8_t ret = ps2_read(PS2_DATA);

	if (ret != PS2_DEV_ACK) {
		printf("[PS2] Device failed to acknowledge command\n");
		return false;
	}

	return true;
}

bool ps2_can_read() {
	return inportb(PS2_CMD) & 1;
}