#include <kernel/serial.h>
#include <kernel/com.h>

#define BUF_SIZE 2048

int serial_received();
int serial_is_transmit_empty();

static char kernel_log[BUF_SIZE];
static uint32_t log_index;

void init_serial() {
	outportb(SERIAL_PORT + SERIAL_OE, 0x00);
	outportb(SERIAL_PORT + SERIAL_FE, 0x80);
	outportb(SERIAL_PORT + SERIAL_DR, 0x03); // 38400 baud
	outportb(SERIAL_PORT + SERIAL_OE, 0x00);
	outportb(SERIAL_PORT + SERIAL_FE, 0x03);
	outportb(SERIAL_PORT + SERIAL_PE, 0xC7);
	outportb(SERIAL_PORT + SERIAL_BI, 0x0B);
}

int serial_received() {
	return inportb(SERIAL_PORT + SERIAL_THRE) & 1;
}

char serial_read() {
	while (serial_received() == 0);

	return inportb(SERIAL_PORT);
}

int serial_is_transmit_empty() {
	return inportb(SERIAL_PORT + SERIAL_THRE) & 0x20;
}

/* Writes a byte to the serial port, and to an internal buffer for debugging
 * purposes.
 */
void serial_write(char c) {
	while (serial_is_transmit_empty() == 0);

	outportb(SERIAL_PORT, c);

	// In-kernel logging, for dmesg-like support
	// Trailing '\0' are handled implicitly
	if (log_index < BUF_SIZE-1) {
		kernel_log[log_index] = c;
	} else {
		for (uint32_t i = BUF_SIZE/2; i < BUF_SIZE; i++) {
			kernel_log[i - BUF_SIZE/2] = kernel_log[i];
		}

		log_index = BUF_SIZE/2;
		kernel_log[log_index] = c;
	}

	log_index++;
}

char* serial_get_log() {
	return kernel_log;
}