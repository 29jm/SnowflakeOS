#include <ui.h>
#include <snow.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void vbox_on_click(vbox_t* vbox, point_t pos) {
	for (uint32_t i = 0; i < vbox->children->count; i++) {
		widget_t* child = list_get_at(vbox->children, i);
		point_t local = ui_to_child_local(child, pos);

		if (point_in_rect(pos, child->bounds)) {
			if (child->on_click) {
				child->on_click(child, local);
			}
		}
	}
}

void vbox_on_draw(vbox_t* vbox, fb_t fb) {
	for (uint32_t i = 0; i < vbox->children->count; i++) {
		widget_t* child = list_get_at(vbox->children, i);

		if (child->on_draw) {
			child->on_draw(child, fb);
		}
	}

	// TODO: this is for debugging
	// rect_t r = ui_get_absolute_bounds((widget_t*) vbox);
	// snow_draw_border(fb, r.x, r.y, r.w, r.h, 0xFF0000);
}

void vbox_on_resize(vbox_t* vbox) {
	/* Size elements */

	for (uint32_t i = 0; i < vbox->children->count; i++) {
		widget_t* child = list_get_at(vbox->children, i);

		if (child->flags & UI_EXPAND_HORIZONTAL) {
			child->bounds.w = vbox->widget.bounds.w;
		} else if (child->bounds.w > vbox->widget.bounds.w) {
			printf("[ui] warning: container too narrow for element\n");
			child->bounds.w = vbox->widget.bounds.w;
		}
	}

	// Compute the size of EXPAND elements in the container's direction
	int32_t expand_space = vbox->widget.bounds.h;
	uint32_t n_expand = vbox->children->count;

	for (uint32_t i = 0; i < vbox->children->count; i++) {
		widget_t* child = list_get_at(vbox->children, i);

		// TODO: use a min size instead for every child
		if (!(child->flags & UI_EXPAND_VERTICAL)) {
			expand_space -= child->bounds.h;
			n_expand--;
		}
	}

	if (expand_space <= 0) {
		printf("[ui] error: container overflow\n");
		return;
	}

	uint32_t height = n_expand ? expand_space / n_expand : 0;

	for (uint32_t i = 0; n_expand && i < vbox->children->count; i++) {
		widget_t* child = list_get_at(vbox->children, i);

		if (child->flags & UI_EXPAND_VERTICAL) {
			child->bounds.h = height;
		}
	}

	/* Position elements */

	uint32_t y = 0;

	for (uint32_t i = 0; i < vbox->children->count; i++) {
		widget_t* child = list_get_at(vbox->children, i);

		child->bounds.y = y;

		y += child->bounds.h;
	}

	/* Ask elements to resize their content */

	for (uint32_t i = 0; i < vbox->children->count; i++) {
		widget_t* child = list_get_at(vbox->children, i);

		if (child->on_resize) {
			child->on_resize(child);
		}
	}
}

void vbox_on_mouse_move(vbox_t* vbox, point_t p) {
	for (uint32_t i = 0; i < vbox->children->count; i++) {
		widget_t* child = list_get_at(vbox->children, i);
		point_t local = ui_to_child_local(child, p);

		if (point_in_rect(p, child->bounds)) {
			if (child->on_mouse_move) {
				child->on_mouse_move(child, local);
			}
		}
	}
}

vbox_t* vbox_new() {
	vbox_t* vbox = calloc(sizeof(vbox_t));

	vbox->children = list_new();
	vbox->widget.flags = UI_EXPAND;
	vbox->widget.on_click = (widget_clicked_t) vbox_on_click;
	vbox->widget.on_draw = (widget_draw_t) vbox_on_draw;
	vbox->widget.on_resize = (widget_resize_t) vbox_on_resize;
	vbox->widget.on_mouse_move = (widget_mouse_moved_t) vbox_on_mouse_move;

	return vbox;
}

void vbox_add(vbox_t* vbox, widget_t* widget) {
	widget->parent = (widget_t*) vbox;

	list_add(vbox->children, widget);

	vbox_on_resize(vbox);
}

void vbox_free(vbox_t* vbox) {
	for (uint32_t i = 0; i < vbox->children->count; i++) {
		// TODO: call a callback of children, they might've allocated.
		free(list_get_at(vbox->children, i));
	}

	free(vbox);
}