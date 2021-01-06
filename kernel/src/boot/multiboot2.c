#include <kernel/multiboot2.h>
#include <kernel/sys.h>

static const char* tag_table[] = {
    "TAG_END",
    "TAG_CMDLINE",
    "<unknown>",
    "TAG_MODULE",
    "TAG_MEM",
    "TAG_BOOTDEV",
    "TAG_MEMMAP",
    "TAG_VBE",
    "TAG_FB",
    "<unknown>",
    "TAG_APM",
    "<unknown>",
    "<unknown>",
    "<unknown>",
    "TAG_RSDP1",
    "TAG_RSDP2",
};

/* Prints the multiboot2 tags given by the bootloader.
 */
void mb2_print_tags(mb2_t* boot) {
    if (boot->total_size <= sizeof(mb2_t)) {
        printke("no tags given");
        return;
    }

    mb2_tag_t* tag = boot->tags;
    mb2_tag_t* prev_tag = tag;

    do {
        const char* tag_name;

        if (tag->type < sizeof(tag_table) / sizeof(tag_table[0])) {
            tag_name = tag_table[tag->type];
        } else {
            tag_name = "<unknown>";
        }

        printk("%12s (%2d): %d bytes", tag_name, tag->type, tag->size);

        prev_tag = tag;
        tag = (mb2_tag_t*) ((uintptr_t) tag + align_to(tag->size, 8));
    } while (prev_tag->type != MB2_TAG_END);
}

/* Returns the first multiboot2 tag of the requested type.
 */
mb2_tag_t* mb2_find_tag(mb2_t* boot, uint32_t tag_type) {
    mb2_tag_t* tag = boot->tags;
    mb2_tag_t* prev_tag = tag;

    do {
        if (tag->type == tag_type) {
            return tag;
        }

        prev_tag = tag;
        tag = (mb2_tag_t*) ((uintptr_t) tag + align_to(tag->size, 8));
    } while (prev_tag->type != MB2_TAG_END);

    return NULL;
}