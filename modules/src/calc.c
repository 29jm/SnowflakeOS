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

// void print_bounds(rect_t r) {
// 	printf("{ x:%d, y:%d, w:%d, h:%d }\n", r.x, r.y, r.w, r.h);
// }

// uint32_t char2int(char c) {
// 	return c - '0';
// }

// uint32_t powi(uint32_t a, uint32_t b) {
// 	uint32_t n = 1;

// 	for (uint32_t i = 0; i < b; i++) {
// 		n *= a;
// 	}

// 	return n;
// }

// uint32_t str2int(char* s) {
// 	uint32_t len = strlen(s);
// 	uint32_t n = 0;

// 	for (uint32_t i = 0; i < len; i++) {
// 		n += powi(10, len - i - 1) * char2int(s[i]);
// 	}

// 	return n;
// }

// int32_t parse_int(const char* s) {
// 	uint32_t n = 0;

// 	while (isdigit(s[n])) {
// 		n++;
// 	}

// 	char* s2 = strdup(s);
// 	s2[n] = 0;
// 	uint32_t res = str2int(s2);
// 	free(s2);

// 	return res;
// }

// enum tokens {
// 	NUM, PLUS, MINUS, MUL, DIV, LP, RP
// };

// void tokenize(list_t* tokens, list_t* values) {
// 	char* s = buf;
// 	char tmp[200] = "";
// 	bool was_num = false;

// 	while (s < &buf[strlen(buf)]) {
// 		char c = *s;


// 		if (c == '+') {
// 			list_add(tokens, PLUS);
// 		} else if (c == '-') {
// 			list_add(tokens, MINUS);
// 		} else if (c == '*') {
// 			list_add(tokens, MUL);
// 		} else if (c == '/') {
// 			list_add(tokens, DIV);
// 		} else {
// 			if (!was_num) {
// 				list_add(tokens, NUM);
// 				was_num = true;
// 			}

// 			uint32_t n = strlen(tmp);
// 			tmp[n] = *s;
// 			tmp[n + 1] = 0;

// 			if (s + 1 == &buf[strlen(buf)] || !isdigit(s[1])) {
// 				list_add(values, str2int(tmp));
// 				was_num = false;
// 			}
// 		}
// 	}
// }

// void shunting_yard() {
// 	list_t* nums = list_new();
// 	list_t* tokens = list_new();
// 	list_t* output = list_new();
// 	list_t* ops = list_new();

// 	tokenize(tokens, nums);

// }

// void num_clicked(button_t* btn) {
// 	uint32_t n = strlen(buf);

// 	switch (btn->text[0]) {
// 	case '=': // TODO: do stuff
// 	case 'C':
// 		buf[0] = 0;
// 		break;
// 	default:
// 		strcpy(buf+n, btn->text);
// 		break;
// 	}

// 	button_set_text(text_field, buf);
// }

int main() {
	// window_t* win = snow_open_window("calc", w, h, WM_NORMAL);
	// ui_app_t app = ui_app_new(win);

	// /* Main vbox */

	// vbox_t* main_vb = vbox_new();
	// ui_set_root(app, (widget_t*) main_vb);

	// text_field = button_new("");
	// text_field->widget.flags |= UI_EXPAND_HORIZONTAL;
	// vbox_add(main_vb, (widget_t*) text_field);

	// /* Controls hbox */

	// hbox_t* controls_hb = hbox_new();
	// vbox_add(main_vb, (widget_t*) controls_hb);

	// /* Numeral buttons */

	// button_t* num_btn[10];

	// for (uint32_t i = 0; i < 10; i++) {
	// 	char str[2] = "\0\0";
	// 	num_btn[i] = button_new(itoa(i, str, 10));
	// 	num_btn[i]->widget.flags |= UI_EXPAND;
	// 	num_btn[i]->on_click = num_clicked;
	// }

	// vbox_t* nums_vb = vbox_new();
	// hbox_add(controls_hb, (widget_t*) nums_vb);

	// hbox_t* nums1_hb = hbox_new();
	// vbox_add(nums_vb, (widget_t*) nums1_hb);

	// hbox_add(nums1_hb, (widget_t*) num_btn[7]);
	// hbox_add(nums1_hb, (widget_t*) num_btn[8]);
	// hbox_add(nums1_hb, (widget_t*) num_btn[9]);

	// hbox_t* nums2_hb = hbox_new();
	// vbox_add(nums_vb, (widget_t*) nums2_hb);
	// hbox_add(nums2_hb, (widget_t*) num_btn[4]);
	// hbox_add(nums2_hb, (widget_t*) num_btn[5]);
	// hbox_add(nums2_hb, (widget_t*) num_btn[6]);

	// hbox_t* nums3_hb = hbox_new();
	// vbox_add(nums_vb, (widget_t*) nums3_hb);
	// hbox_add(nums3_hb, (widget_t*) num_btn[1]);
	// hbox_add(nums3_hb, (widget_t*) num_btn[2]);
	// hbox_add(nums3_hb, (widget_t*) num_btn[3]);

	// vbox_add(nums_vb, (widget_t*) num_btn[0]);

	// /* Operations vbox */

	// vbox_t* ops_vb = vbox_new();
	// ops_vb->widget.flags &= ~UI_EXPAND_HORIZONTAL;
	// ops_vb->widget.bounds.w = 40;
	// hbox_add(controls_hb, (widget_t*) ops_vb);

	// char ops[] = "/*+-";

	// for (uint32_t i = 0; i < strlen(ops); i++) {
	// 	char op[2] = "\0\0";
	// 	op[0] = ops[i];
	// 	button_t* op_btn = button_new(op);
	// 	op_btn->widget.flags |= UI_EXPAND;
	// 	op_btn->on_click = num_clicked;
	// 	vbox_add(ops_vb, (widget_t*) op_btn);
	// }

	// /* Actions vbox */

	// vbox_t* actions_vb = vbox_new();
	// actions_vb->widget.flags &= ~UI_EXPAND_HORIZONTAL;
	// actions_vb->widget.bounds.w = 40;
	// hbox_add(controls_hb, (widget_t*) actions_vb);

	// char actions[] = "C=";

	// for (uint32_t i = 0; i < strlen(actions); i++) {
	// 	char action[2] = "\0\0";
	// 	action[0] = actions[i];
	// 	button_t* action_btn = button_new(action);
	// 	action_btn->widget.flags |= UI_EXPAND;
	// 	action_btn->on_click = num_clicked;
	// 	vbox_add(actions_vb, (widget_t*) action_btn);
	// }

	// while (true) {
	// 	wm_event_t event = snow_get_event(win);

	// 	ui_handle_input(app, event);

	// 	if (event.type) {
	// 		ui_draw(app);
	// 		snow_render_window(win);
	// 	}

	// 	snow_sleep(30);
	// }

	// return 0;
}