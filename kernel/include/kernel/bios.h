#ifndef BIOS_H
#define BIOS_H

/* For documentation, see
 * http://wiki.osdev.org/Memory_Map_%28x86%29#BIOS_Data_Area_.28BDA.29
 */
#define PORT_COM1 0x0400
#define KEYBOARD_STATE_FLAGS 0x0417
#define DISPLAY_MODE 0x0449
#define IRQ0_TICKS 0x046C
#define DRIVE_NUMBER 0x0475

#endif
