#include <ui.h>
#include <snow.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 30

void set_str(const char* base, const char* end, uint32_t n, char* s) {
    char num[BUF_SIZE];

    strcpy(s, base);
    itoa(n, num, 10);
    strcat(s, num);
    strcat(s, end);
}

int main() {
    char heap_usage[BUF_SIZE];
    char mem_usage[BUF_SIZE];
    char mem_total[BUF_SIZE];

    ui_app_t app = ui_app_new("System information", 275, 100, NULL);

    vbox_t* vbox = vbox_new();
    ui_set_root(app, W(vbox));

    button_t* heap_btn = button_new("Kernel heap used:");
    button_t* ram_btn = button_new("Ram used:");
    button_t* ram_tot_btn = button_new("Total ram used:");

    vbox_add(vbox, W(heap_btn));
    vbox_add(vbox, W(ram_btn));
    vbox_add(vbox, W(ram_tot_btn));

    while (true) {
        wm_event_t evt = snow_get_event(app.win);

        if (evt.type == WM_EVENT_KBD && evt.kbd.keycode == KBD_ESCAPE) {
            break;
        }

        sys_info_t info;
        syscall2(SYS_INFO, SYS_INFO_MEMORY, (uintptr_t) &info);

        set_str("Kernel heap used: ", "KiB", info.kernel_heap_usage >> 10, heap_usage);
        set_str("Ram used: ", "KiB", info.ram_usage >> 10, mem_usage);
        set_str("Ram total: ", "MiB", info.ram_total >> 20, mem_total);

        button_set_text(heap_btn, heap_usage);
        button_set_text(ram_btn, mem_usage);
        button_set_text(ram_tot_btn, mem_total);

        ui_draw(app);
        snow_sleep(300);
    }

    ui_app_destroy(app);

    return 0;
}