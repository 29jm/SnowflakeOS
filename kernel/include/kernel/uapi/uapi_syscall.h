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
#define SYS_WRITE 13
#define SYS_MKDIR 14
#define SYS_FSEEK 15
#define SYS_FTELL 16
#define SYS_CHDIR 17
#define SYS_GETCWD 18
#define SYS_UNLINK 19
#define SYS_RENAME 20
#define SYS_MAKETTY 21
#define SYS_STAT 22
#define SYS_MAX 23 // First invalid syscall number

#define SYS_INFO_UPTIME 1
#define SYS_INFO_MEMORY 2
#define SYS_INFO_LOG    4

typedef struct {
    uint32_t kernel_heap_usage;
    uint32_t ram_usage;
    uint32_t ram_total;
    float uptime;
    char* kernel_log; // Must be at least 2048 bytes long
} sys_info_t;

typedef struct {
    uint8_t* buf;
    uint32_t size;
} sys_buf_t;