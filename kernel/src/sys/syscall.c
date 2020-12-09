#include <kernel/syscall.h>
#include <kernel/pmm.h>
#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/fb.h>
#include <kernel/wm.h>
#include <kernel/serial.h>
#include <kernel/pipe.h>
#include <kernel/sys.h> // for UNUSED macro

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/uapi/uapi_syscall.h>

static void syscall_handler(registers_t* regs);

static void syscall_yield(registers_t* regs);
static void syscall_exit(registers_t* regs);
static void syscall_sleep(registers_t* regs);
static void syscall_putchar(registers_t* regs);
static void syscall_sbrk(registers_t* regs);
static void syscall_wm(registers_t* regs);
static void syscall_info(registers_t* regs);
static void syscall_exec(registers_t* regs);
static void syscall_open(registers_t* regs);
static void syscall_close(registers_t* regs);
static void syscall_read(registers_t* regs);
static void syscall_write(registers_t* regs);
static void syscall_readdir(registers_t* regs);
static void syscall_mkdir(registers_t* regs);
static void syscall_fseek(registers_t* regs);
static void syscall_ftell(registers_t* regs);
static void syscall_chdir(registers_t* regs);
static void syscall_getcwd(registers_t* regs);
static void syscall_unlink(registers_t* regs);
static void syscall_rename(registers_t* regs);
static void syscall_maketty(registers_t* regs);
static void syscall_stat(registers_t* regs);

handler_t syscall_handlers[SYSCALL_NUM] = { 0 };

void init_syscall() {
    isr_register_handler(48, syscall_handler);

    syscall_handlers[SYS_YIELD] = syscall_yield;
    syscall_handlers[SYS_EXIT] = syscall_exit;
    syscall_handlers[SYS_SLEEP] = syscall_sleep;
    syscall_handlers[SYS_PUTCHAR] = syscall_putchar;
    syscall_handlers[SYS_SBRK] = syscall_sbrk;
    syscall_handlers[SYS_WM] = syscall_wm;
    syscall_handlers[SYS_INFO] = syscall_info;
    syscall_handlers[SYS_EXEC] = syscall_exec;
    syscall_handlers[SYS_OPEN] = syscall_open;
    syscall_handlers[SYS_CLOSE] = syscall_close;
    syscall_handlers[SYS_READ] = syscall_read;
    syscall_handlers[SYS_WRITE] = syscall_write;
    syscall_handlers[SYS_READDIR] = syscall_readdir;
    syscall_handlers[SYS_MKDIR] = syscall_mkdir;
    syscall_handlers[SYS_FSEEK] = syscall_fseek;
    syscall_handlers[SYS_FTELL] = syscall_ftell;
    syscall_handlers[SYS_CHDIR] = syscall_chdir;
    syscall_handlers[SYS_GETCWD] = syscall_getcwd;
    syscall_handlers[SYS_UNLINK] = syscall_unlink;
    syscall_handlers[SYS_RENAME] = syscall_rename;
    syscall_handlers[SYS_MAKETTY] = syscall_maketty;
    syscall_handlers[SYS_STAT] = syscall_stat;
}

static void syscall_handler(registers_t* regs) {
    if (regs->eax < SYS_MAX && syscall_handlers[regs->eax]) {
        handler_t handler = syscall_handlers[regs->eax];
        regs->eax = 0;
        handler(regs);
    } else {
        printke("unknown syscall %d", regs->eax);
    }
}

/* Convention:
 * - Syscall number in eax,
 * - Arguments shall be passed in this order: ebx, ecx, edx, esi,
 * - If more are needed, pack them into a struct pointer,
 * - Values shall be returned first in eax, then in user-provided pointers.
 */

static void syscall_yield(registers_t* regs) {
    UNUSED(regs);

    proc_schedule();
}

static void syscall_exit(registers_t* regs) {
    UNUSED(regs);

    // TODO: exit status?
    proc_exit_current_process();
}

static void syscall_sleep(registers_t* regs) {
    uint32_t ms = regs->ebx;
    proc_sleep(ms);
}

static void syscall_putchar(registers_t* regs) {
    putchar((char) regs->ebx);
}

static void syscall_sbrk(registers_t* regs) {
    uint32_t size = regs->ebx;
    regs->eax = (uint32_t) proc_sbrk(size);
}

/* Sends a command to the window manager:
 *     int32_t syscall_wm(command, param);
 */
static void syscall_wm(registers_t* regs) {
    uint32_t cmd = regs->ebx;

    switch (cmd) {
        case WM_CMD_OPEN: {
                wm_param_open_t* param = (wm_param_open_t*) regs->ecx;
                regs->eax = wm_open_window(param->fb, param->flags);
            } break;
        case WM_CMD_CLOSE:
            wm_close_window(regs->ecx);
            break;
        case WM_CMD_RENDER: {
                wm_param_render_t* param = (wm_param_render_t*) regs->ecx;
                wm_render_window(param->win_id, param->clip);
            } break;
        case WM_CMD_INFO: {
                fb_t* fb = (fb_t*) regs->ecx;
                *fb = fb_get_info();
            } break;
        case WM_CMD_EVENT: {
                wm_param_event_t* param = (wm_param_event_t*) regs->ecx;
                wm_get_event(param->win_id, param->event);
        } break;
        default:
            printke("wrong command: %d", cmd);
            regs->eax = -1;
            break;
    }
}

/* Returns known system information.
 */
static void syscall_info(registers_t* regs) {
    uint32_t request = regs->ebx;
    sys_info_t* info = (sys_info_t*) regs->ecx;

    if (request & SYS_INFO_MEMORY) {
        info->kernel_heap_usage = memory_usage();
        info->ram_usage = pmm_used_memory();
        info->ram_total = pmm_total_memory();
    }

    if (request & SYS_INFO_UPTIME) {
        info->uptime = timer_get_time();
    }

    if (request & SYS_INFO_LOG && info->kernel_log) {
        strcpy(info->kernel_log, serial_get_log());
    }
}

static void syscall_exec(registers_t* regs) {
    char* name = (char*) regs->ebx;
    char** args = (char**) regs->ecx;

    regs->eax = proc_exec(name, args);
}

static void syscall_open(registers_t* regs) {
    const char* path = (const char*) regs->ebx;
    uint32_t flags = regs->ecx;

    regs->eax = proc_open(path, flags);
}

static void syscall_close(registers_t* regs) {
    uint32_t fd = regs->ebx;

    proc_close(fd);
}

static void syscall_read(registers_t* regs) {
    uint32_t fd = regs->ebx;
    uint8_t* buf = (uint8_t*) regs->ecx;
    uint32_t size = regs->edx;

    regs->eax = proc_read(fd, buf, size);
}

static void syscall_readdir(registers_t* regs) {
    uint32_t fd = regs->ebx;
    sos_directory_entry_t* d_ent = (sos_directory_entry_t*) regs->ecx;

    regs->eax = proc_readdir(fd, d_ent);
}

static void syscall_write(registers_t* regs) {
    uint32_t fd = regs->ebx;
    uint8_t* buf = (uint8_t*) regs->ecx;
    uint32_t size = regs->edx;

    regs->eax = proc_write(fd, buf, size);
}

static void syscall_mkdir(registers_t* regs) {
    const char* path = (const char*) regs->ebx;
    uint32_t mode = regs->ecx;

    fs_mkdir(path, mode);
}

static void syscall_fseek(registers_t* regs) {
    uint32_t fd = regs->ebx;
    uint32_t offset = regs->ecx;
    int32_t whence = regs->edx;

    regs->eax = proc_fseek(fd, offset, whence);
}

static void syscall_ftell(registers_t* regs) {
    uint32_t fd = regs->ebx;

    regs->eax = proc_ftell(fd);
}

static void syscall_chdir(registers_t* regs) {
    const char* path = (const char*) regs->ebx;

    regs->eax = proc_chdir(path);
}

static void syscall_getcwd(registers_t* regs) {
    char* buf = (char*) regs->ebx;
    uint32_t size = regs->ecx;

    char* cwd = proc_get_cwd();

    if (strlen(cwd) + 1 > size) {
        regs->eax = (uintptr_t) NULL;
    } else {
        regs->eax = (uintptr_t) strcpy(buf, cwd);
    }

    kfree(cwd);
}

static void syscall_unlink(registers_t* regs) {
    char* path = (char*) regs->ebx;

    regs->eax = fs_unlink(path);
}

static void syscall_rename(registers_t* regs) {
    char* old_path = (char*) regs->ebx;
    char* new_path = (char*) regs->ecx;

    regs->eax = fs_rename(old_path, new_path);
}

static void syscall_maketty(registers_t* regs) {
    ft_entry_t* entry = zalloc(sizeof(ft_entry_t));

    entry->fd = FS_STDOUT_FILENO;
    entry->inode = pipe_new();

    proc_add_fd(entry);

    regs->eax = 0;
}

static void syscall_stat(registers_t* regs) {
    const char* path = (const char*) regs->ebx;
    stat_t* buf = (stat_t*) regs->ecx;

    regs->eax = fs_stat(path, buf);
}