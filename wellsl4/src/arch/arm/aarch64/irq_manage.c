/**
 * @file
 * @brief ARM64 Cortex-A interrupt management
 */

#include <arch/cpu.h>
#include <device.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/irq.h>
#include <api/fatal.h>

void arm64_fatal_error(word_t reason, const arch_esf_t *esf);

word_t arch_is_irq_pending(void)
{

}

void arch_irq_enable(word_t irq)
{
	struct device *dev = sw_isr_table[0].arg;

	irq_enable_next_level(dev, (irq >> 8) - 1);
}

void arch_irq_disable(word_t irq)
{
	struct device *dev = sw_isr_table[0].arg;

	irq_disable_next_level(dev, (irq >> 8) - 1);
}

sword_t arch_irq_is_enabled(word_t irq)
{
	struct device *dev = sw_isr_table[0].arg;

	return irq_is_enabled_next_level(dev);
}

void arm64_irq_sched_prior_set(word_t irq, word_t prio, u32_t flag)
{
	struct device *dev = sw_isr_table[0].arg;

	if (irq == 0)
		return;

	irq_set_priority_next_level(dev, (irq >> 8) - 1, prio, flag);
}

void irq_spurious(void *unused)
{
	ARG_UNUSED(unused);

	arm64_fatal_error(fatal_spurious_irq_fatal, NULL);
}
