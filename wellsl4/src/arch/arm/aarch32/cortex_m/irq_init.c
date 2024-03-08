
/**
 * @file
 * @brief ARM Cortex-M interrupt initialization
 *
 */

#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

/**
 *
 * @brief Initialize interrupts
 *
 * Ensures all interrupts have their sched_prior set to _EXC_IRQ_DEFAULT_PRIO and
 * not 0, which they have it set to when coming out of reset. This ensures that
 * interrupt locking via BASEPRI works as expected.
 *
 * @return N/A
 */

void arm_int_lib_init(void)
{
	sword_t irq = 0;

	for (; irq < CONFIG_NUM_IRQS; irq++) {
		NVIC_SetPriority((IRQn_Type)irq, _IRQ_PRIO_OFFSET);
	}
}
