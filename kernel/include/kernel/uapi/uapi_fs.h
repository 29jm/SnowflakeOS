#pragma once

#include <stdint.h>

#define O_READ 1

typedef struct {
	uint32_t inode;
	uint16_t entry_size;
	uint8_t name_len_low;
	uint8_t type;
	char name[];
} sos_directory_entry_t;