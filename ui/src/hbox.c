#include <ui.h>
#include <snow.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void hbox_on_click(hbox_t* hbox, point_t pos) {
	for (uint32_t i = 0; i < hbox->children->count; i++) {
		widget_t* child = list_get_at(hbox->children, i);
		point_t local = ui_to_child_local(child, pos);

		if (point_in_rect(pos, child->bounds)) {
			if (child->on_click) {
				child->on_click(child, local);
			}
		}
	}
}

void hbox_on_draw(hbox_t* hbox, fb_t fb) {
	for (uint32_t i = 0; i < hbox->children->count; i++) {
		widget_t* child = list_get_at(hbox->children, i);

		if (child->on_draw) {
			child->on_draw(child, fb);
		}
	}

	// TODO: this is for debugging
	// rect_t r = ui_get_absolute_bounds((widget_t*) hbox);
	// snow_draw_border(fb, r.x, r.y, r.w, r.h, 0x00FF00);
}

void hbox_on_resize(hbox_t* hbox) {
	/* Size elements */

	for (uint32_t i = 0; i < hbox->children->count; i++) {
		widget_t* child = list_get_at(hbox->children, i);

		if (child->flags & UI_EXPAND_VERTICAL) {
			child->bounds.h = hbox->widget.bounds.h;
		} else if (child->bounds.h > hbox->widget.bounds.h) {
			printf("[ui] warning: container too narrow for element\n");
			child->bounds.h = hbox->widget.bounds.h;
		}
	}

	// Compute the size of EXPAND elements in the container's direction
	int32_t expand_space = hbox->widget.bounds.w;
	uint32_t n_expand = hbox->children->count;

	for (uint32_t i = 0; i < hbox->children->count; i++) {
		widget_t* child = list_get_at(hbox->children, i);

		// TODO: use a min size instead for every child
		if (!(child->flags & UI_EXPAND_HORIZONTAL)) {
			expand_space -= child->bounds.w;
			n_expand--;
		}
	}

	if (expand_space <= 0) {
		printf("[ui] error: container overflow\n");
		return;
	}

	uint32_t size = n_expand ? expand_space / n_expand : 0;

	for (uint32_t i = 0; n_expand && i < hbox->children->count; i++) {
		widget_t* child = list_get_at(hbox->children, i);

		if (child->flags & UI_EXPAND_HORIZONTAL) {
			child->bounds.w = size;
		}
	}

	/* Position elements */

	uint32_t position = 0;

	for (uint32_t i = 0; i < hbox->children->count; i++) {
		widget_t* child = list_get_at(hbox->children, i);

		child->bounds.x = position;

		position += child->bounds.w;
	}

	/* Ask elements to resize their content */

	for (uint32_t i = 0; i < hbox->children->count; i++) {
		widget_t* child = list_get_at(hbox->children, i);

		if (child->on_resize) {
			child->on_resize(child);
		}
	}
}

void hbox_on_mouse_move(hbox_t* hbox, point_t pos) {
	for (uint32_t i = 0; i < hbox->children->count; i++) {
		widget_t* child = list_get_at(hbox->children, i);
		point_t local = ui_to_child_local(child, pos);

		if (point_in_rect(pos, child->bounds)) {
			if (child->on_mouse_move) {
				child->on_mouse_move(child, local);
			}
		}
	}
}

hbox_t* hbox_new() {
	hbox_t* hbox = calloc(sizeof(hbox_t));

	hbox->children = list_new();
	hbox->widget.flags = UI_EXPAND;
	hbox->widget.on_click = (widget_clicked_t) hbox_on_click;
	hbox->widget.on_draw = (widget_draw_t) hbox_on_draw;
	hbox->widget.on_resize = (widget_resize_t) hbox_on_resize;
	hbox->widget.on_mouse_move = (widget_mouse_moved_t) hbox_on_mouse_move;

	return hbox;
}

void hbox_add(hbox_t* hbox, widget_t* widget) {
	widget->parent = (widget_t*) hbox;

	list_add(hbox->children, widget);

	hbox_on_resize(hbox);
}

void hbox_free(hbox_t* hbox) {
	for (uint32_t i = 0; i < hbox->children->count; i++) {
		// TODO: call a callback of children, they might've allocated.
		free(list_get_at(hbox->children, i));
	}

	free(hbox);
}