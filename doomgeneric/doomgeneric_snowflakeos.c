#include "doomgeneric.h"
#include "doomkeys.h"

#include <snow.h>
#include <stdio.h>
#include <string.h>

static window_t* win;
static int key = 0;

int convertToDoomKey(int kc, char repr) {
    switch (kc) {
    case KBD_ENTER:
        return KEY_ENTER;
    case KBD_ESCAPE:
        return KEY_ESCAPE;
    case KBD_LEFT:
        return KEY_LEFTARROW;
    case KBD_RIGHT:
        return KEY_RIGHTARROW;
    case KBD_UP:
        return KEY_UPARROW;
    case KBD_DOWN:
        return KEY_DOWNARROW;
    case KBD_LEFT_CONTROL:
    case KBD_RIGHT_CONTROL:
        return KEY_FIRE;
    case KBD_SPACE:
        return KEY_USE;
    case KBD_LEFT_SHIFT:
    case KBD_RIGHT_SHIFT:
        return KEY_RSHIFT;
    default:
        return repr;
    }
}

void DG_Init() {
    win = snow_open_window("doom", DOOMGENERIC_RESX, DOOMGENERIC_RESY, WM_NORMAL);
}

void DG_DrawFrame() {
    wm_event_t evt = snow_get_event(win);

    if (evt.type & WM_EVENT_KBD) {
        key = convertToDoomKey(evt.kbd.keycode, evt.kbd.repr);
        key |= evt.kbd.pressed << 16;
    }

    memcpy((void*) win->fb.address, DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
    snow_render_window(win);
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
    if (key != 0) {
        *pressed = key >> 16;
        *doomkey = key & 0xffff;
        key = 0;

        return 1;
    }

    return 0;
}

void DG_SetWindowTitle(const char* title) {

}