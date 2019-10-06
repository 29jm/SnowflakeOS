# SnowflakeOS
A hobby OS to help me learn about kernel stuff, to eventually get into linux kernel developpement. Currently it can: boot in higher half, setup paging, handle hardware exceptions and IRQs, print things on the screen, get keyboard input (but it's useless right now), count seconds with the PIC, start usermode processes and switch between them... I also aim to make the code readable and well-organized. 

# Build & Run in a QEMU
Run `make` to get the binary and disk image, and `make qemu/bochs` to run it.

# Installation on real hardware
`sudo dd if=SnowflakeOS.iso of=/dev/sdX # I'm not responsible if you break anything`  
Where /dev/sdX is the device file representing an USB key. You can then boot from it.

# Requirements
* An i686 cross-compiler in your path (see osdev.org's article)
* Xorriso, found in the package `libisoburn` on Archlinux
* `grub` and its optional dependency `mtools`
* `qemu`, unless you run it on real hardware
