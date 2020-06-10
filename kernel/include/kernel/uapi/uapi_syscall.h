#pragma once

#include <stdint.h>

#define SYS_YIELD 0
#define SYS_EXIT 1
#define SYS_SLEEP 2
#define SYS_PUTCHAR 3
#define SYS_SBRK 4
#define SYS_WM 5
#define SYS_INFO 6
#define SYS_EXEC 7
#define SYS_MAX 8 // First invalid syscall number

#define SYS_INFO_UPTIME 1
#define SYS_INFO_MEMORY 2
#define SYS_INFO_LOG    4

typedef struct {
	uint32_t kernel_heap_usage;
	double uptime;
	char* kernel_log; // Must be at least 2048 bytes long
} sys_info_t;