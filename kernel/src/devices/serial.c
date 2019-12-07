#include <kernel/serial.h>
#include <kernel/com.h>

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

void serial_write(char c) {
    while (serial_is_transmit_empty() == 0);

    outportb(SERIAL_PORT, c);
}