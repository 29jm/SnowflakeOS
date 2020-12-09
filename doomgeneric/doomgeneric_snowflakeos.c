#include "doomgeneric.h"
#include "doomkeys.h"

#include <snow.h>
#include <stdio.h>
#include <string.h>

static window_t* win;

void DG_Init() {
    win = snow_open_window("doom", DOOMGENERIC_RESX, DOOMGENERIC_RESY, WM_NORMAL);
    printf("[doom] opened window\n");
}

void DG_DrawFrame() {
    printf("[doom] drawing frame\n");
    memcpy((void*) win->fb.address, DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
    printf("[doom] copied frame\n");
    snow_render_window(win);
    printf("[doom] rendered\n");
}

void DG_SleepMs(uint32_t ms) {
    snow_sleep(ms);
}

uint32_t DG_GetTicksMs() {
    sys_info_t si;

    syscall2(SYS_INFO, SYS_INFO_UPTIME, (uintptr_t) &si);

    uint32_t t = si.uptime * 1000;

    return t;
}

int DG_GetKey(int* pressed, unsigned char* doomkey) {
    return 0;
}

void DG_SetWindowTitle(const char* title) {
}