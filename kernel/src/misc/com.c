#include <kernel/com.h>

uint16_t inports(uint16_t port) {
	uint16_t rv;
	asm volatile ("inw %1, %0" : "=a" (rv) : "dN" (port));
	return rv;
}

void outports(uint16_t port, uint16_t data) {
	asm volatile ("outw %1, %0" : : "dN" (port), "a" (data));
}

uint32_t inportl(uint16_t port) {
	unsigned int rv;
	asm volatile ("inl %%dx, %%eax" : "=a" (rv) : "dN" (port));
	return rv;
}

void outportl(uint16_t port, uint32_t data) {
	asm volatile ("outl %%eax, %%dx" : : "dN" (port), "a" (data));
}

uint8_t inportb(uint16_t port) {
	unsigned char rv;
	asm volatile ("inb %1, %0" : "=a" (rv) : "dN" (port));
	return rv;
}

void outportb(uint16_t port, uint8_t data) {
	asm volatile ("outb %1, %0" : : "dN" (port), "a" (data));
}

void outportsm(uint16_t port, unsigned char * data, uint32_t size) {
	asm volatile ("rep outsw" : "+S" (data), "+c" (size) : "d" (port));
}

void inportsm(uint16_t port, unsigned char * data, uint32_t size) {
	asm volatile ("rep insw" : "+D" (data), "+c" (size) : "d" (port) : "memory");
}
