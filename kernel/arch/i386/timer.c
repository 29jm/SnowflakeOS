#include <kernel/timer.h>
#include <kernel/irq.h>
#include <kernel/com.h>
#include <stdio.h>

volatile uint32_t current_tick = 0;

void init_timer() {
	irq_register_handler(IRQ0, timer_callback);

	uint32_t divisor = TIMER_QUOTIENT / TIMER_FREQ;

	outportb(PIT_CMD, PIT_SET); // TODO: use defines
	outportb(PIT_0, divisor & 0xFF);
	outportb(PIT_0, (divisor >> 8) & 0xFF);
}

void timer_callback(registers_t* regs) {
	current_tick++;
}

uint32_t timer_get_tick() {
	return current_tick;
}
