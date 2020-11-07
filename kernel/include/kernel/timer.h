#pragma once

#include <kernel/irq.h>

void init_timer();
void timer_callback();
uint32_t timer_get_tick();
double timer_get_time();
void timer_register_callback(handler_t handler);
void timer_remove_callback(handler_t handler);

#define TIMER_FREQ 50 // in Hz
#define TIMER_QUOTIENT 1193180

#define PIT_0 0x40
#define PIT_1 0x41
#define PIT_2 0x42
#define PIT_CMD 0x43
#define PIT_SET 0x36