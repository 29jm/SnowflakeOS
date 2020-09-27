#pragma once

#include <kernel/uapi/uapi_fs.h>

#include <stdint.h>

#define SYS_YIELD 0
#define SYS_EXIT 1
#define SYS_SLEEP 2
#define SYS_PUTCHAR 3
#define SYS_SBRK 4
#define SYS_WM 5
#define SYS_INFO 6
#define SYS_EXEC 7
#define SYS_OPEN 9
#define SYS_CLOSE 10
#define SYS_READ 11
#define SYS_READDIR 12
#define SYS_MAX 13 // First invalid syscall number

#define SYS_INFO_UPTIME 1
#define SYS_INFO_MEMORY 2
#define SYS_INFO_LOG    4

typedef struct {
	uint32_t kernel_heap_usage;
	double uptime;
	char* kernel_log; // Must be at least 2048 bytes long
} sys_info_t;

typedef struct {
	uint8_t* buf;
	uint32_t size;
} sys_buf_t;