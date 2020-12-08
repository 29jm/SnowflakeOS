#include <snow.h>
#include <ui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXDIGIT 10
#define NUL_ACTION 0x0
#define ADD_ACTION 0x1
#define SUB_ACTION 0x2
#define MUL_ACTION 0x4
#define DIV_ACTION 0x8
#define EQU_ACTION 0x10

uint32_t w = 302;
uint32_t h = 350;
long int display = 0;
long int accumulate = 0;
uint32_t action = NUL_ACTION;
uint32_t numdigits = 0;
char buf[MAXDIGIT];
char dispbuf[MAXDIGIT];
button_t* text_field;

void accumulate_selection(void) {
    accumulate = 0;

    for (int counter = 0; (counter < MAXDIGIT && isdigit(buf[counter])); counter++)
        accumulate = accumulate * 10 + (buf[counter] - '0');
}

void clear_buffer(void) {
    for (int counter = 0; counter < MAXDIGIT; counter++)
        buf[counter] = 0;
    buf[0] = '0';
    numdigits = 0;
    accumulate = 0;
}

void update_buffer(void) {
    for (int counter = 0; counter < MAXDIGIT; counter++)
        dispbuf[counter] = 0;

    if (display)
        itoa(display, dispbuf, 10);
    else
        dispbuf[0] = '0';
}

void clear_vars(void) {
    clear_buffer();
    display = 0;
    action = NUL_ACTION;
}

void take_action(void) {
    switch(action) {
    case ADD_ACTION:
        display = display + accumulate;
        break;
    case SUB_ACTION:
        display = display - accumulate;
        break;
    case MUL_ACTION:
        display = display * accumulate;
        break;
    case DIV_ACTION:
        if (accumulate == 0) {
            printf("Division by zero!\n");
            clear_vars();
        }
        else if (display == 0)
            display = 0;
        else
            display = display / accumulate;
        break;
    case EQU_ACTION:
        break;
    default:
        display = accumulate;
        break;
    }
    update_buffer();
    clear_buffer();
}

void num_clicked(button_t* btn) {
    switch (btn->text[0]) {
    case '=':
        take_action();
        action = EQU_ACTION;
        break;
    case 'C':
        clear_vars();
        update_buffer();
        break;
    case '+':
        take_action();
        action = ADD_ACTION;
        break;
    case '-':
        take_action();
        action = SUB_ACTION;
        break;
    case '*':
        take_action();
        action = MUL_ACTION;
        break;
    case '/':
        take_action();
        action = DIV_ACTION;
        break;
    default:
        if (numdigits < (MAXDIGIT - 2)) {
            buf[numdigits++] = btn->text[0];
            accumulate_selection();
            strcpy(dispbuf, buf);
        }
        else
            printf("Numeric Overflow!\n");
        break;
    }
}

int main() {
    window_t* win = snow_open_window("calc", w, h, WM_NORMAL);
    ui_app_t app = ui_app_new(win, NULL);

    /* Main vbox */

    vbox_t* main_vb = vbox_new();
    ui_set_root(app, (widget_t*) main_vb);

    button_t* text_field = button_new("TODO");
    free(text_field->text);
    text_field->text = dispbuf;
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

    clear_vars();
    update_buffer();

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
