/**
 * @file
 * @brief Kernel fatal error handler for ARM Cortex-M and Cortex-R
 *
 * This module provides the arm_fatal_error() routine for ARM Cortex-M
 * and Cortex-R CPUs.
 */

#include <arch/cpu.h>
#include <sys/assert.h>
#include <api/fatal.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <toolchain.h>


static void esf_dump(const arch_esf_t *esf)
{
	user_error("r0/a1:  0x%08x  r1/a2:  0x%08x  r2/a3:  0x%08x",
		esf->basic.a1, esf->basic.a2, esf->basic.a3);
	user_error("r3/a4:  0x%08x r12/ip:  0x%08x r14/lr:  0x%08x",
		esf->basic.a4, esf->basic.ip, esf->basic.lr);
	user_error("xpsr:  0x%08x", esf->basic.xpsr);
	
#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	for (sword_t i = 0; i < 16; i += 4) 
	{
		user_error("s[%2d]:  0x%08x  s[%2d]:  0x%08x"
			"  s[%2d]:  0x%08x  s[%2d]:  0x%08x",
			i, (word_t)esf->s[i],
			i + 1, (word_t)esf->s[i + 1],
			i + 2, (word_t)esf->s[i + 2],
			i + 3, (word_t)esf->s[i + 3]);
	}
	user_error("fpscr:  0x%08x", esf->fpscr);
#endif
	user_error("Faulting instruction address (r15/pc): 0x%08x",
		esf->basic.pc);
}

void arm_fatal_error(word_t reason, const arch_esf_t *esf)
{
	if (esf != NULL)
	{
		esf_dump(esf);
	}
	
	fatal_error(reason, esf);
}

/**
 * @brief Handle a software-generated fatal exception
 * (e.g. kernel oops, panic, etc.).
 *
 * Notes:
 * - the function is invoked in SVC Handler
 * - if triggered from nPRIV mode, only oops and stack fail error reasons
 *   may be propagated to the fault handling process.
 * - We expect the supplied exception stack page_f to always be a valid
 *   page_f. That is because, if the ESF cannot be stacked during an SVC,
 *   a processor fault (e.g. stacking error) will be generated, and the
 *   fault handler will executed insted of the SVC.
 *
 * @param esf exception page_f
 */
void do_kernel_oops(const arch_esf_t *esf)
{
	/* Stacked R0 holds the exception reason. */
	word_t reason = esf->basic.r0;

#if defined(CONFIG_USERSPACE)
	if ((__get_CONTROL() & CONTROL_nPRIV_Msk) == CONTROL_nPRIV_Msk) 
	{
		/*
		 * Exception triggered from nPRIV mode.
		 *
		 * User mode is only allowed to induce oopses and stack check
		 * failures via software-triggered system fatal exceptions.
		 */
		if (!((esf->basic.r0 == fatal_oops_fatal) ||
			(esf->basic.r0 == fatal_stack_check_fatal)))
		{

			reason = fatal_oops_fatal;
		}
	}

#endif /* CONFIG_USERSPACE */
	arm_fatal_error(reason, esf);
}

FUNC_NORETURN void arch_syscall_oops(void *ssf_ptr)
{
	word_t *ssf_contents = ssf_ptr;
	arch_esf_t oops_esf = { 0 };

	/* TODO: Copy the rest of the register set out of ssf_ptr */
	oops_esf.basic.pc = ssf_contents[3];

	arm_fatal_error(fatal_oops_fatal, &oops_esf);
	CODE_UNREACHABLE;
}
