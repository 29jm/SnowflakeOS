#include <kernel/timer.h>
#include <kernel/com.h>

#include <stdlib.h>
#include <stdio.h>
#include <list.h>

static uint32_t current_tick;
static list_t callbacks;

void init_timer() {
    callbacks = LIST_HEAD_INIT(callbacks);

    irq_register_handler(IRQ0, &timer_callback);

    uint32_t divisor = TIMER_QUOTIENT / TIMER_FREQ;

    outportb(PIT_CMD, PIT_SET);
    outportb(PIT_0, divisor & 0xFF);
    outportb(PIT_0, (divisor >> 8) & 0xFF);
}

void timer_callback(registers_t* regs) {
    current_tick++;

    handler_t* callback;
    list_for_each_entry(callback, &callbacks) {
        (*callback)(regs);
    }
}

uint32_t timer_get_tick() {
    return current_tick;
}

/* Returns the time since boot in seconds
 */
float timer_get_time() {
    return current_tick * (1.0f / TIMER_FREQ);
}

/* Registers a callback to be called on each timer tick.
 */
void timer_register_callback(handler_t handler) {
    handler_t* callback = (handler_t*) kmalloc(sizeof(handler_t));
    *callback = handler;

    list_add(&callbacks, callback);
}

void timer_remove_callback(handler_t handler) {
    list_t* iter;
    handler_t* callback;
    list_for_each(iter, callback, &callbacks) {
        if (*callback == handler) {
            list_del(iter);
            kfree(callback);
            return;
        }
    }
}