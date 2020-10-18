# SnowflakeOS

![A picture is worth, like, a few words](https://29jm.github.io/assets/sos-paint.png)

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
+ VBE graphics
+ window management
+ basic ext2 support

I aim to make the code readable and well-organized. A blog follows the development of this project, here https://29jm.github.io/.

## Dependencies

### Packages

Install the following packages:

+ `xorriso` for Debian/Ubuntu; `libisoburn` on Archlinux
+ `grub`
+ `mtools`
+ `qemu` (recommended)
+ `bochs` (optional)
+ `clang` + development packages, e.g. `base-devel` on Archlinux (optional)

### Cross-compiler

#### Building your own

Run

    make toolchain

to build the cross-compiler needed to compile SnowflakeOS. This command will download and run build scripts for `gcc` and `binutils` from GNU FTP servers, and install the cross-compiler in `toolchain/compiler`.

#### Using clang directly

Instead of building your own toolchain, you can compile SnowflakeOS with your system's `clang`: simply uncomment the following lines in the main `Makefile`:
```shell
# CC=clang
# CFLAGS+=-target i386-pc-none-eabi -m32
# CFLAGS+=-mno-mmx -mno-sse -mno-sse2
```

#### Using a preinstalled cross-compiler

If your distro provides you with a cross compiler, you may want to save time and use it. To do so, you must edit the following variables in the main `Makefile` so that they match the executables of your cross compiler:

    AR=$(HOST)-ar
    AS=$(HOST)-as
    LD=$(HOST)-ld
    CC=$(HOST)-gcc --sysroot=$(SYSROOT) -isystem=/$(INCLUDEDIR)

You may edit `HOST`, or hardcode the executables names directly.

## Running SnowflakeOS

Run either

    make qemu # or
    make bochs

to test SnowflakeOS.

## Installing SnowflakeOS

Testing this project on real hardware is possible. You can copy `SnowflakeOS.iso` to an usb drive using `dd`, like you would when making a live usb of another OS, and boot it directly.

Note that this is rarely ever tested, who knows what it'll do :) I'd love to hear about it if you try this, on which hardware, etc...