#include <ui.h>
#include <snow.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* `lbox` stands for "linear box".
 */

void lbox_on_click(lbox_t* lbox, point_t pos) {
	for (uint32_t i = 0; i < lbox->children->count; i++) {
		widget_t* child = list_get_at(lbox->children, i);
		point_t local = ui_to_child_local(child, pos);

		if (point_in_rect(pos, child->bounds)) {
			if (child->on_click) {
				child->on_click(child, local);
			}
		}
	}
}

void lbox_on_draw(lbox_t* lbox, fb_t fb) {
	// rect_t b = ui_get_absolute_bounds((widget_t*) lbox);

	// snow_draw_rect(fb, b.x, b.y, b.w, b.h, 0x000000);

	for (uint32_t i = 0; i < lbox->children->count; i++) {
		widget_t* child = list_get_at(lbox->children, i);

		if (child->on_draw) {
			child->on_draw(child, fb);
		}
	}
}

void lbox_on_resize(lbox_t* lbox) {
	uint32_t other_direction = lbox->direction == UI_HBOX ?
		UI_VBOX : UI_HBOX;

	/* Size elements */

	// In the opposite direction of the container
	for (uint32_t i = 0; i < lbox->children->count; i++) {
		widget_t* child = list_get_at(lbox->children, i);

		if (child->flags & other_direction) {
			if (lbox->direction == UI_HBOX) {
				child->bounds.h = lbox->widget.bounds.h;
			} else {
				child->bounds.w = lbox->widget.bounds.w;
			}
		} else if (lbox->direction == UI_HBOX &&
			       child->bounds.h > lbox->widget.bounds.h) {
			printf("[hbox] warning: container too narrow for element\n");
			child->bounds.h = lbox->widget.bounds.h;
		} else if (lbox->direction == UI_VBOX &&
			       child->bounds.w > lbox->widget.bounds.w) {
			printf("[vbox] warning: container too narrow for element\n");
			child->bounds.w = lbox->widget.bounds.w;
		}
	}

	// Compute the size of EXPAND elements in the container's direction
	int32_t expand_space = lbox->direction == UI_HBOX ?
	    lbox->widget.bounds.w : lbox->widget.bounds.h;
	uint32_t n_expand = lbox->children->count;

	for (uint32_t i = 0; i < lbox->children->count; i++) {
		widget_t* child = list_get_at(lbox->children, i);

		// TODO: use a min size instead for every child
		if (!(child->flags & lbox->direction)) {
			expand_space -= lbox->direction == UI_HBOX ?
			    child->bounds.w : child->bounds.h;
			n_expand--;
		}
	}

	if (expand_space <= 0) {
		printf("[ui] error: container overflow\n");
		return;
	}

	uint32_t size = n_expand ? expand_space / n_expand : 0;

	for (uint32_t i = 0; n_expand && i < lbox->children->count; i++) {
		widget_t* child = list_get_at(lbox->children, i);

		if (child->flags & lbox->direction) {
			if (lbox->direction == UI_HBOX) {
				child->bounds.w = size;
			} else {
				child->bounds.h = size;
			}
		}
	}

	/* Position elements */

	uint32_t position = 0;

	for (uint32_t i = 0; i < lbox->children->count; i++) {
		widget_t* child = list_get_at(lbox->children, i);

		if (lbox->direction == UI_HBOX) {
			child->bounds.x = position;
		} else {
			child->bounds.y = position;
		}

		position += lbox->direction == UI_HBOX ?
			child->bounds.w : child->bounds.h;
	}

	/* Ask elements to resize their content */

	for (uint32_t i = 0; i < lbox->children->count; i++) {
		widget_t* child = list_get_at(lbox->children, i);

		if (child->on_resize) {
			child->on_resize(child);
		}
	}
}

void lbox_on_mouse_move(lbox_t* lbox, point_t pos) {
	for (uint32_t i = 0; i < lbox->children->count; i++) {
		widget_t* child = list_get_at(lbox->children, i);
		point_t local = ui_to_child_local(child, pos);

		if (point_in_rect(pos, child->bounds)) {
			if (child->on_mouse_move) {
				child->on_mouse_move(child, local);
			}
		}
	}
}

lbox_t* lbox_new(uint32_t direction) {
	lbox_t* lbox = calloc(sizeof(lbox_t));

	lbox->children = list_new();
	lbox->widget.flags = UI_EXPAND;
	lbox->widget.on_click = (widget_clicked_t) lbox_on_click;
	lbox->widget.on_draw = (widget_draw_t) lbox_on_draw;
	lbox->widget.on_resize = (widget_resize_t) lbox_on_resize;
	lbox->widget.on_mouse_move = (widget_mouse_moved_t) lbox_on_mouse_move;
	lbox->direction = direction;

	return lbox;
}

hbox_t* hbox_new() {
	return lbox_new(UI_HBOX);
}

vbox_t* vbox_new() {
	return lbox_new(UI_VBOX);
}

void lbox_add(lbox_t* lbox, widget_t* widget) {
	widget->parent = (widget_t*) lbox;

	list_add(lbox->children, widget);
	lbox_on_resize(lbox);
}

void lbox_clear(lbox_t* lbox) {
	while (lbox->children->count) {
		widget_t* child = list_get_at(lbox->children, 0);

		if (child->on_free) {
			child->on_free(child);
		}

		list_remove_at(lbox->children, 0);
		free(child);
	}
}

void vbox_add(vbox_t* vbox, widget_t* widget) {
	lbox_add(vbox, widget);
}

void hbox_add(hbox_t* hbox, widget_t* widget) {
	lbox_add(hbox, widget);
}

void vbox_clear(vbox_t* vbox) {
	lbox_clear(vbox);
}

void lbox_free(lbox_t* lbox) {
	lbox_clear(lbox);
	free(lbox->children);
	free(lbox);
}