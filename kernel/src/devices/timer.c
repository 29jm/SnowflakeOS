#include <stdio.h>

#include <kernel/timer.h>
#include <kernel/list.h>
#include <kernel/com.h>

#include <stdlib.h>

static uint32_t current_tick;
static list_t* callbacks;

void init_timer() {
    callbacks = list_new();

    irq_register_handler(IRQ0, &timer_callback);

    uint32_t divisor = TIMER_QUOTIENT / TIMER_FREQ;

    outportb(PIT_CMD, PIT_SET);
    outportb(PIT_0, divisor & 0xFF);
    outportb(PIT_0, (divisor >> 8) & 0xFF);
}

void timer_callback(registers_t* regs) {
    current_tick++;

    for (uint32_t i = 0; i < callbacks->count; i++) {
        handler_t callback = *(handler_t*) list_get_at(callbacks, i);
        callback(regs);
    }
}

uint32_t timer_get_tick() {
    return current_tick;
}

/* Returns the time since boot in seconds
 */
double timer_get_time() {
    return current_tick*(1.0/TIMER_FREQ);
}

/* Registers a callback to be called on each timer tick.
 */
void timer_register_callback(handler_t handler) {
    handler_t* callback = (handler_t*) kmalloc(sizeof(handler_t));
    *callback = handler;

    list_add(callbacks, callback);
}

void timer_remove_callback(handler_t handler) {
    for (uint32_t i = 0; i < callbacks->count; i++) {
        handler_t* callback = list_get_at(callbacks, i);

        if (*callback == handler) {
            list_remove_at(callbacks, i);
            break;
        }
    }
}