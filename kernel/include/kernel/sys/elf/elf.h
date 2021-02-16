/* Kernel level elf loading and manipulation functions in this header you will
 * find the functions needed to check an elf file for validity and load it into
 * memory.
 */
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define ELF_IDENTN 16

#define ELF_MAG0 0x7F
#define ELF_MAG1 'E'
#define ELF_MAG2 'L'
#define ELF_MAG3 'F'

#define ELFDATAD (1)
#define ELFCLASS (1)
#define ELF_MACH (3)
#define ELF_CVER (1)

#define SECHDR_UNDEF (0x00)

/* Named indexes inside the ident structure
 */
enum elf_ident {
    EI_MAG0 = 0,
    EI_MAG1,
    EI_MAG2,
    EI_MAG3,
    EI_CLASS,
    EI_DATA,
    EI_VERSION,
    EI_OSABI,
    EI_ABIVERSION,
    EI_PAD
};

enum elf_type {
    ET_NONE = 0,
    ET_REL,
    ET_EXE
};

enum sechdr_types {
    SECTHDR_NULL    = 0,
    SECTHDR_PROGBITS,
    SECTHDR_SYMTAB,
    SECTHDR_STRTAB,
    SECTHDR_RELA,
    SECTHDR_NOBITS,
    SECTHDR_REL,
};

enum sechdr_attr {
    SECTHDR_WRITE   = 1,
    SECTHDR_ALLOC,
};

/* These typedefs are present here to help making the code more readable
 */
typedef uint16_t elf_hword_t;
typedef uint32_t elf_word_t;
typedef int32_t elf_sword_t;
typedef uint32_t elf_offset_t;
typedef uint32_t elf_addr_t;

typedef struct __elf_header
{
    uint8_t         elf_ident[ELF_IDENTN];
    elf_hword_t     elf_type;
    elf_hword_t     elf_machine;
    elf_word_t      elf_version;
    elf_addr_t      elf_entry;
    elf_offset_t    elf_proghdroff;
    elf_offset_t    elf_secthdroff;
    elf_word_t      elf_flags;
    elf_hword_t     elf_hdrsz;
    elf_hword_t     elf_proghdr_entsz;
    elf_hword_t     elf_proghdr_entct;
    elf_hword_t     elf_secthdr_entsz;
    elf_hword_t     elf_secthdr_entct;
    elf_hword_t     elf_secthdr_secnam_ndx;
} elf_header_t;

typedef struct __elf_secthdr {
    elf_word_t     sh_name;
    elf_word_t     sh_type;
    elf_word_t     sh_flags;
    elf_addr_t     sh_addr;
    elf_offset_t   sh_offset;
    elf_word_t     sh_size;
    elf_word_t     sh_link;
    elf_word_t     sh_info;
    elf_word_t     sh_addr_align;
    elf_word_t     sh_entsz;
} elf_secthdr_t;

bool elf_check_mag(elf_header_t* header);
bool elf_check_support(elf_header_t* header);
