#include <snow.h>
#include <ui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

uint32_t w = 302;
uint32_t h = 350;
char buf[256];
button_t* text_field;

void num_clicked(button_t* btn) {
    // uint32_t n = strlen(buf);

    switch (btn->text[0]) {
    case '=': // TODO: do stuff
    case 'C':
        buf[0] = 0;
        break;
    default:
        // strcpy(buf+n, btn->text);
        break;
    }

    // button_set_text(text_field, buf);
}

int main() {
    window_t* win = snow_open_window("calc", w, h, WM_NORMAL);
    ui_app_t app = ui_app_new(win, NULL);

    /* Main vbox */

    vbox_t* main_vb = vbox_new();
    ui_set_root(app, (widget_t*) main_vb);

    button_t* text_field = button_new("TODO");
    text_field->widget.flags |= UI_EXPAND_HORIZONTAL;
    vbox_add(main_vb, (widget_t*) text_field);

    /* Controls hbox */

    hbox_t* controls_hb = hbox_new();
    vbox_add(main_vb, (widget_t*) controls_hb);

    /* Numeral buttons */

    button_t* num_btn[10];

    for (uint32_t i = 0; i < 10; i++) {
        char str[2] = "\0\0";
        num_btn[i] = button_new(itoa(i, str, 10));
        num_btn[i]->widget.flags |= UI_EXPAND;
        num_btn[i]->on_click = num_clicked;
    }

    vbox_t* nums_vb = vbox_new();
    hbox_add(controls_hb, (widget_t*) nums_vb);

    hbox_t* nums1_hb = hbox_new();
    vbox_add(nums_vb, (widget_t*) nums1_hb);

    hbox_add(nums1_hb, (widget_t*) num_btn[7]);
    hbox_add(nums1_hb, (widget_t*) num_btn[8]);
    hbox_add(nums1_hb, (widget_t*) num_btn[9]);

    hbox_t* nums2_hb = hbox_new();
    vbox_add(nums_vb, (widget_t*) nums2_hb);
    hbox_add(nums2_hb, (widget_t*) num_btn[4]);
    hbox_add(nums2_hb, (widget_t*) num_btn[5]);
    hbox_add(nums2_hb, (widget_t*) num_btn[6]);

    hbox_t* nums3_hb = hbox_new();
    vbox_add(nums_vb, (widget_t*) nums3_hb);
    hbox_add(nums3_hb, (widget_t*) num_btn[1]);
    hbox_add(nums3_hb, (widget_t*) num_btn[2]);
    hbox_add(nums3_hb, (widget_t*) num_btn[3]);

    vbox_add(nums_vb, (widget_t*) num_btn[0]);

    /* Operations vbox */

    vbox_t* ops_vb = vbox_new();
    ops_vb->widget.flags &= ~UI_EXPAND_HORIZONTAL;
    ops_vb->widget.bounds.w = 40;
    hbox_add(controls_hb, (widget_t*) ops_vb);

    char ops[] = "/*+-";

    for (uint32_t i = 0; i < strlen(ops); i++) {
        char op[2] = "\0\0";
        op[0] = ops[i];
        button_t* op_btn = button_new(op);
        op_btn->widget.flags |= UI_EXPAND;
        op_btn->on_click = num_clicked;
        vbox_add(ops_vb, (widget_t*) op_btn);
    }

    /* Actions vbox */

    vbox_t* actions_vb = vbox_new();
    actions_vb->widget.flags &= ~UI_EXPAND_HORIZONTAL;
    actions_vb->widget.bounds.w = 40;
    hbox_add(controls_hb, (widget_t*) actions_vb);

    char actions[] = "C=";

    for (uint32_t i = 0; i < strlen(actions); i++) {
        char action[2] = "\0\0";
        action[0] = actions[i];
        button_t* action_btn = button_new(action);
        action_btn->widget.flags |= UI_EXPAND;
        action_btn->on_click = num_clicked;
        vbox_add(actions_vb, (widget_t*) action_btn);
    }

    while (true) {
        wm_event_t event = snow_get_event(win);

        ui_handle_input(app, event);

        if (event.type) {
            ui_draw(app);
            snow_render_window(win);
        }

        snow_sleep(30);
    }

    return 0;
}