#ifndef COM_H
#define COM_H

#include <stdint.h>

unsigned char inportb(uint16_t port);
void outportb(uint16_t port, uint8_t data);

uint16_t inports(uint16_t port);
void outports(uint16_t port, uint16_t data);

uint32_t inportl(uint16_t port);
void outportl(uint16_t port, uint32_t data);

void inportsm(uint16_t port, unsigned char * data, uint32_t size);
void outportsm(uint16_t port, unsigned char * data, uint32_t size);

#endif
