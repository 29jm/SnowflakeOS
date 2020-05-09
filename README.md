# SnowflakeOS

![A picture is worth, like, a few words](https://29jm.github.io/assets/snowy_bg.png)

A hobby OS to help me learn about kernel stuff, to eventually get into linux kernel developpement. Currently it supports:
+ booting in higher half
+ paging
+ memory management
+ handling IRQs
+ 80x25 text mode
+ serial output
+ PS/2 keyboard
+ PS/2 mouse
+ PIC timer
+ loading GRUB modules as usermode processes
+ preemptive multitasking
+ VESA graphics
+ window management

I aim to make the code readable and well-organized. A blog follows the development of this project, here https://29jm.github.io/.

## Installing dependencies

### An i686 cross-compiler

This is the only dependency you will need to build yourself, others can be installed through your package manager.

Follow this guide to build a working cross-compiler:  
> https://wiki.osdev.org/GCC_Cross-Compiler

It should be in your `$PATH`, so that `i686-elf-*` executables are available.

### xorriso

+ `xorriso` (Debian/Ubuntu)
+ `libisoburn` (Archlinux)

### GRUB

+ `grub`
+ `mtools`

### An emulator

Install at least one of the following packages:
+ `qemu` (recommended)
+ `bochs`

## Running SnowflakeOS

Run either

    make qemu

or

    make bochs

to test SnowflakeOS.

## Installing SnowflakeOS

Testing this project on real hardware is possible. You can copy `SnowflakeOS.iso` to an usb drive using `dd`, like you would when making a live usb of another OS, and boot it directly.

Note that this is rarely ever tested, who knows what it'll do :) I'd love to hear about it if you try this, on which hardware, etc...