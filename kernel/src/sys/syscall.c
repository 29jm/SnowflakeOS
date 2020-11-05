#include <kernel/syscall.h>
#include <kernel/pmm.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/fb.h>
#include <kernel/wm.h>
#include <kernel/serial.h>
#include <kernel/fs.h>
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

handler_t syscall_handlers[SYSCALL_NUM] = { 0 };

void init_syscall() {
    isr_register_handler(48, &syscall_handler);

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
}

static void syscall_handler(registers_t* regs) {
    if (regs->eax < SYS_MAX && syscall_handlers[regs->eax]) {
        handler_t handler = syscall_handlers[regs->eax];
        regs->eax = 0;
        handler(regs);
    } else {
        printf("[sys] Unknown syscall %d\n", regs->eax);
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
            printf("[wm] Wrong command: %d\n", cmd);
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

    regs->eax = proc_exec(name);
}

static void syscall_open(registers_t* regs) {
    const char* path = (const char*) regs->ebx;
    uint32_t mode = regs->ecx;

    regs->eax = proc_open(path, mode);
}

static void syscall_close(registers_t* regs) {
    uint32_t fd = regs->ebx;

    proc_close(fd);
}

static void syscall_read(registers_t* regs) {
    uint32_t fd = regs->ebx;
    uint8_t* buf = (uint8_t*) regs->ecx;
    uint32_t size = regs->edx;

    if (!proc_has_fd(fd)) {
        regs->eax = 0;
        return;
    }

    regs->eax = fs_read(fd, buf, size);
}

static void syscall_readdir(registers_t* regs) {
    uint32_t fd = regs->ebx;
    sos_directory_entry_t* d_ent = (sos_directory_entry_t*) regs->ecx;

    if (!proc_has_fd(fd)) {
        regs->eax = -1;
        return;
    }

    uint32_t read = fs_readdir(fd, d_ent, d_ent->entry_size);
    regs->eax = read;
}

static void syscall_write(registers_t* regs) {
    uint32_t fd = regs->ebx;
    uint8_t* buf = (uint8_t*) regs->ecx;
    uint32_t size = regs->edx;

    if (!proc_has_fd(fd)) {
        regs->eax = 0;
        return;
    }

    regs->eax = fs_write(fd, buf, size);
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

    if (!proc_has_fd(fd)) {
        regs->eax = (uint32_t) -1;
        return;
    }

    regs->eax = fs_fseek(fd, offset, whence);
}

static void syscall_ftell(registers_t* regs) {
    uint32_t fd = regs->ebx;

    if (!proc_has_fd(fd)) {
        regs->eax = (uint32_t) -1;
        return;
    }

    regs->eax = fs_ftell(fd);
}