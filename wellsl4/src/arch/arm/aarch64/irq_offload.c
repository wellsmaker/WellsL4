/**
 * @file
 * @brief Software interrupts utility code - ARM64 implementation
 */

#include <kernel/thread.h>
#include <arch/irq.h>
#include <aarch64/exc.h>


volatile irq_offload_routine_t offload_routine;
static void *offload_param;

void irq_do_offload(void)
{
	offload_routine(offload_param);
}

void arch_irq_offload(irq_offload_routine_t routine, void *parameter)
{
	sched_lock();
	offload_routine = routine;
	offload_param = parameter;

	arm64_offload();

	offload_routine = NULL;
	sched_unlock();
}
