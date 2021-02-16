#include <kernel/sys/elf/elf.h>
#include <kernel/sys/elf/elf_errno.h>

#include <errno.h>

/* Checks the header on a elf file
 */
bool elf_check_mag(elf_header_t* header) {
    if (!header)
        return false;

    /* TODO: improve this */
    if (header->elf_ident[EI_MAG0] != ELF_MAG0) {
        goto elf_hdr_ivmag;
    } else if (header->elf_ident[EI_MAG1] != ELF_MAG1) {
        goto elf_hdr_ivmag;
    } else if (header->elf_ident[EI_MAG2] != ELF_MAG2) {
        goto elf_hdr_ivmag;
    } else if (header->elf_ident[EI_MAG3] != ELF_MAG3) {
        goto elf_hdr_ivmag;
    }

    return true;

elf_hdr_ivmag:
    errno = ELF_IVMAGN;
    return false;
}

bool elf_check_support(elf_header_t* header) {
    if (!elf_check_mag(header)) {
        return false;
    }

    if (header->elf_ident[EI_CLASS] != ELFCLASS) {
        errno = ELF_UNSCLS;
        return false;
    } else if (header->elf_ident[EI_DATA] != ELFDATAD) {
        errno = ELF_UNSORD;
        return false;
    } else if (header->elf_machine != ELF_MACH) {
        errno = ELF_UNSTGT;
        return false;
    } else if (header->elf_ident[EI_VERSION] != ELF_CVER) {
        errno = ELF_UNSVER;
        return false;
    } else if (header->elf_type != ET_REL && header->elf_type != ET_EXE) {
        errno = ELF_UNSTYP;
        return false;
    }
}
