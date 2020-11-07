#pragma once

#define SERIAL_PORT 0x3F8

#define SERIAL_DR 0
#define SERIAL_OE 1
#define SERIAL_PE 2
#define SERIAL_FE 3
#define SERIAL_BI 4
#define SERIAL_THRE 5
#define SERIAL_TEMT 6
#define SERIAL_IE 7

void init_serial();
char serial_read();
void serial_write(char c);
char* serial_get_log();