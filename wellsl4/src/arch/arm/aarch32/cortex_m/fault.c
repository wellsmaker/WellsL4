
/**
 * @file
 * @brief Common fault handler for ARM Cortex-M
 *
 * Common fault handler for ARM Cortex-M processors.
 */

#include <arch/cpu.h>
#include <sys/inttypes.h>
#include <sys/assert.h>
#include <sys/util.h>
#include <types_def.h>
#include <api/fatal.h>
#include <sys/errno.h>
#include <sys/string.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <arch/syscall.h>
/*
 * This is used by some architectures to define code ranges which may
 * perform operations that could generate a CPU exception that should not
 * be fatal. Instead, the exception should return but set the program
 * counter to a 'fixup' memory address which will gracefully error out.
 *
 * For example, in the case where user mode passes in a C string via
 * system call, the length of that string needs to be measured. A specially
 * written assembly language version of strlen (arch_user_string_len)
 * defines start and end symbols where the memory in the string is examined;
 * if this generates a fault, jumping to the fixup symbol within the same
 * function will return an error result to the caller.
 *
 * To ensure precise control of the state of registers and the stack pointer,
 * these functions need to be written in assembly.
 *
 * The arch-specific fault handling code will define an array of these
 * handle_pange structures and return from the exception with the PC updated
 * to the fixup address if a match is found.
 */

struct handle_pange {
	void *start;
	void *end;
	void *fixup;
};

#define HANDLE_RANGE_INIT(name) \
	{ name ## _fault_start, name ## _fault_end, name ## _fixup }

#define HANDLE_RANGE_DECLARE(name)          \
	void name ## _fault_start(void); \
	void name ## _fault_end(void);   \
	void name ## _fixup(void)


#if defined(CONFIG_ASSERT)
#define PR_EXC(...) user_error(__VA_ARGS__)
#define STORE_xFAR(reg_var, reg) u32_t reg_var = (u32_t)reg
#else
#define PR_EXC(...)
#define STORE_xFAR(reg_var, reg)
#endif /* CONFIG_PRINTK || CONFIG_LOG */

#define PR_FAULT_INFO(...) PR_EXC(__VA_ARGS__)

#if defined(CONFIG_ARM_MPU) && defined(CONFIG_CPU_HAS_NXP_MPU)
#define EMN(edr)   (((edr) & SYSMPU_EDR_EMN_MASK) >> SYSMPU_EDR_EMN_SHIFT)
#define EACD(edr)  (((edr) & SYSMPU_EDR_EACD_MASK) >> SYSMPU_EDR_EACD_SHIFT)
#endif

/* Exception Return (EXC_RETURN) is provided in LR upon exception entry.
 * It is used to perform an exception return and to detect possible state
 * transition upon exception.
 */

/* Prefix. Indicates that this is an EXC_RETURN value.
 * This field reads as 0b11111111.
 */
#define EXC_RETURN_INDICATOR_PREFIX     (0xFF << 24)
/* bit[0]: Exception Secure. The security domain the exception was taken to. */
#define EXC_RETURN_EXCEPTION_SECURE_Pos 0
#define EXC_RETURN_EXCEPTION_SECURE_Msk \
		BIT(EXC_RETURN_EXCEPTION_SECURE_Pos)
#define EXC_RETURN_EXCEPTION_SECURE_Non_Secure 0
#define EXC_RETURN_EXCEPTION_SECURE_Secure EXC_RETURN_EXCEPTION_SECURE_Msk
/* bit[2]: Stack Pointer selection. */
#define EXC_RETURN_SPSEL_Pos 2
#define EXC_RETURN_SPSEL_Msk BIT(EXC_RETURN_SPSEL_Pos)
#define EXC_RETURN_SPSEL_MAIN 0
#define EXC_RETURN_SPSEL_PROCESS EXC_RETURN_SPSEL_Msk
/* bit[3]: Mode. Indicates the Mode that was stacked from. */
#define EXC_RETURN_MODE_Pos 3
#define EXC_RETURN_MODE_Msk BIT(EXC_RETURN_MODE_Pos)
#define EXC_RETURN_MODE_HANDLER 0
#define EXC_RETURN_MODE_THREAD EXC_RETURN_MODE_Msk
/* bit[4]: Stack page_f type. Indicates whether the stack page_f is a standard
 * integer only stack page_f or an extended floating-point stack page_f.
 */
#define EXC_RETURN_STACFRAME_TYPE_Pos 4
#define EXC_RETURN_STACFRAME_TYPE_Msk BIT(EXC_RETURN_STACFRAME_TYPE_Pos)
#define EXC_RETURN_STACFRAME_TYPE_EXTENDED 0
#define EXC_RETURN_STACFRAME_TYPE_STANDARD EXC_RETURN_STACFRAME_TYPE_Msk
/* bit[5]: Default callee register stacking. Indicates whether the default
 * stacking rules apply, or whether the callee registers are already on the
 * stack.
 */
#define EXC_RETURN_CALLEE_STACPos 5
#define EXC_RETURN_CALLEE_STACMsk BIT(EXC_RETURN_CALLEE_STACPos)
#define EXC_RETURN_CALLEE_STACSKIPPED 0
#define EXC_RETURN_CALLEE_STACDEFAULT EXC_RETURN_CALLEE_STACMsk
/* bit[6]: Secure or Non-secure stack. Indicates whether a Secure or
 * Non-secure stack is used to restore stack page_f on exception return.
 */
#define EXC_RETURN_RETURN_STACPos 6
#define EXC_RETURN_RETURN_STACMsk BIT(EXC_RETURN_RETURN_STACPos)
#define EXC_RETURN_RETURN_STACNon_Secure 0
#define EXC_RETURN_RETURN_STACSecure EXC_RETURN_RETURN_STACMsk

/* Integrity signature for an ARMv8-M implementation */
#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
#define INTEGRITY_SIGNATURE_STD 0xFEFA125BUL
#define INTEGRITY_SIGNATURE_EXT 0xFEFA125AUL
#else
#define INTEGRITY_SIGNATURE 0xFEFA125BUL
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */
/* Size (in words) of the additional state context that is pushed
 * to the Secure stack during a Non-Secure exception entry.
 */
#define ADDITIONAL_STATE_CONTEXT_WORDS 10


/**
 *
 * Dump information regarding fault (FAULT_DUMP == 1)
 *
 * Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 1
 * (short form).
 *
 * eg. (precise bus error escalated to hard fault):
 *
 * Fault! EXC #3
 * HARD FAULT: Escalation (see below)!
 * MMFSR: 0x00000000, BFSR: 0x00000082, UFSR: 0x00000000
 * BFAR: 0xff001234
 *
 *
 *
 * Dump information regarding fault (FAULT_DUMP == 2)
 *
 * Dump information regarding the fault when CONFIG_FAULT_DUMP is set to 2
 * (long form), and return the error code for the kernel to identify the fatal
 * error reason.
 *
 * eg. (precise bus error escalated to hard fault):
 *
 * ***** HARD FAULT *****
 *    Fault escalation (see below)
 * ***** BUS FAULT *****
 *   Precise data bus error
 *   Address: 0xff001234
 *
 */

#if (CONFIG_FAULT_DUMP == 1)
static void fault_show(const arch_esf_t *esf, sword_t fault)
{
	PR_EXC("Fault! EXC #%d", fault);

#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	PR_EXC("MMFSR: 0x%x, BFSR: 0x%x, UFSR: 0x%x",
	       SCB_MMFSR, SCB_BFSR, SCB_UFSR);
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	PR_EXC("SFSR: 0x%x", SAU->SFSR);
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
}
#else
/* For Dump level 2, detailed information is generated by the
 * fault handling functions for individual fault conditions, so this
 * function is left empty.
 *
 * For Dump level 0, no information needs to be generated.
 */
static void fault_show(const arch_esf_t *esf, sword_t fault)
{
	(void)esf;
	(void)fault;
}
#endif /* FAULT_DUMP == 1 */

#ifdef CONFIG_USERSPACE
HANDLE_RANGE_DECLARE(arm_user_string_nlen);

static const struct handle_pange exceptions[] = {
	HANDLE_RANGE_INIT(arm_user_string_nlen)
};
#endif

/* Perform an assessment whether an MPU fault shall be
 * treated as recoverable.
 *
 * @return true if error is recoverable, otherwise return false.
 */
static bool memory_fault_recoverable(arch_esf_t *esf)
{
#ifdef CONFIG_USERSPACE
	for (sword_t i = 0; i < ARRAY_SIZE(exceptions); i++)
	{
		/* Mask out instruction mode */
		u32_t start = (u32_t)exceptions[i].start & ~0x1;
		u32_t end = (u32_t)exceptions[i].end & ~0x1;

		if (esf->basic.pc >= start && esf->basic.pc < end)
		{
			esf->basic.pc = (u32_t)(exceptions[i].fixup);
			return true;
		}
	}
#endif

	return false;
}

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
/* HardFault is used for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
u32_t checktcb_stack_fail(const u32_t fault_addr,
	const u32_t psp);
#endif /* CONFIG_MPU_STACK_GUARD || defined(CONFIG_USERSPACE) */

/**
 *
 * @brief Dump MemManage fault information
 *
 * See arm_fault_dump() for example.
 *
 * @return error code to identify the fatal error reason
 */
static u32_t mem_manage_fault(arch_esf_t *esf, sword_t from_hard_fault,
			      bool *recoverable)
{
	u32_t reason = fatal_cpu_exception_fatal;
	u32_t mmfar = -EINVAL;

	PR_FAULT_INFO("***** MPU FAULT *****");

	if ((SCB->CFSR & SCB_CFSR_MSTKERR_Msk) != 0)
	{
		PR_FAULT_INFO("  Stacking error (context area might be"
			" not valid)");
	}
	if ((SCB->CFSR & SCB_CFSR_MUNSTKERR_Msk) != 0) 
	{
		PR_FAULT_INFO("  Unstacking error");
	}
	if ((SCB->CFSR & SCB_CFSR_DACCVIOL_Msk) != 0) 
	{
		PR_FAULT_INFO("  Data Access Violation");
		/* In a fault handler, to determine the true faulting address:
		 * 1. Read and save the MMFAR value.
		 * 2. Read the MMARVALID bit in the MMFSR.
		 * The MMFAR address is valid only if this bit is 1.
		 *
		 * Software must follow this sequence because another higher
		 * sched_prior exception might change the MMFAR value.
		 */
		mmfar = SCB->MMFAR;

		if ((SCB->CFSR & SCB_CFSR_MMARVALID_Msk) != 0) 
		{
			PR_EXC("  MMFAR Address: 0x%x", mmfar);
			if (from_hard_fault) 
			{
				/* clear SCB_MMAR[VALID] to reset */
				SCB->CFSR &= ~SCB_CFSR_MMARVALID_Msk;
			}
		}
	}
	if ((SCB->CFSR & SCB_CFSR_IACCVIOL_Msk) != 0)
	{
		PR_FAULT_INFO("  Instruction Access Violation");
	}
#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	if ((SCB->CFSR & SCB_CFSR_MLSPERR_Msk) != 0) 
	{
		PR_FAULT_INFO(
			"  Floating-point lazy state preservation error");
	}
#endif /* !defined(CONFIG_ARMV7_M_ARMV8_M_FP) */

	/* When stack protection is enabled, we need to assess
	 * if the memory violation error is a stack corruption.
	 *
	 * By design, being a Stacking MemManage fault is a necessary
	 * and sufficient condition for a thread stack corruption.
	 */
	if (SCB->CFSR & SCB_CFSR_MSTKERR_Msk) 
	{
#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
		/* MemManage Faults are always banked between security
		 * states. Therefore, we can safely assume the fault
		 * originated from the same security state.
		 *
		 * As we only assess thread stack corruption, we only
		 * process the error further if the stack page_f is on
		 * PSP. For always-banked MemManage Fault, this is
		 * equivalent to inspecting the RETTOBASE flag.
		 */
		if (SCB->ICSR & SCB_ICSR_RETTOBASE_Msk) 
		{
			u32_t min_stack_ptr = checktcb_stack_fail(mmfar,
				((u32_t) &esf[0]));

			if (min_stack_ptr) 
			{
				/* When MemManage Stacking Error has occurred,
				 * the stack context page_f might be corrupted
				 * but the stack pointer may have actually
				 * descent below the allowed (thread) stack
				 * area. We may face a problem with un-stacking
				 * the page_f, upon the exception return, if we
				 * do not have sufficient access permissions to
				 * read the corrupted stack page_f. Therefore,
				 * we manually force the stack pointer to the
				 * lowest allowed position, inside the thread's
				 * stack.
				 *
				 * Note:
				 * The PSP will normally be adjusted in a tail-
				 * chained exception performing context switch,
				 * after aborting the corrupted thread. The
				 * adjustment, here, is required as tail-chain
				 * cannot always be guaranteed.
				 *
				 * The manual adjustment of PSP is safe, as we
				 * will not be re-scheduling this thread again
				 * for execution; thread stack corruption is a
				 * fatal error and a thread that corrupted its
				 * stack needs to be aborted.
				 */
				__set_PSP(min_stack_ptr);

				reason = fatal_stack_check_fatal;
			} 
			else 
			{
				user_error("Stacking error not a stack fail\n");
			}
		}
#else
	(void)mmfar;
	user_error("Stacking error without stack right / User-mode support\n");
#endif /* CONFIG_MPU_STACK_GUARD || CONFIG_USERSPACE */
	}

	/* clear MMFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_MEMFAULTSR_Msk;
	/* Assess whether system shall ignore/recover from this MPU fault. */
	*recoverable = memory_fault_recoverable(esf);

	return reason;
}

/**
 *
 * @brief Dump BusFault information
 *
 * See arm_fault_dump() for example.
 *
 * @return N/A
 */
static sword_t bus_fault(arch_esf_t *esf, sword_t from_hard_fault, bool *recoverable)
{
	u32_t reason = fatal_cpu_exception_fatal;

	PR_FAULT_INFO("***** BUS FAULT *****");

	if (SCB->CFSR & SCB_CFSR_STKERR_Msk) 
	{
		PR_FAULT_INFO("  Stacking error");
	}
	if (SCB->CFSR & SCB_CFSR_UNSTKERR_Msk) 
	{
		PR_FAULT_INFO("  Unstacking error");
	}
	if (SCB->CFSR & SCB_CFSR_PRECISERR_Msk)
	{
		PR_FAULT_INFO("  Precise data bus error");
		/* In a fault handler, to determine the true faulting address:
		 * 1. Read and save the BFAR value.
		 * 2. Read the BFARVALID bit in the BFSR.
		 * The BFAR address is valid only if this bit is 1.
		 *
		 * Software must follow this sequence because another
		 * higher sched_prior exception might change the BFAR value.
		 */
		STORE_xFAR(bfar, SCB->BFAR);

		if ((SCB->CFSR & SCB_CFSR_BFARVALID_Msk) != 0) {
			PR_EXC("  BFAR Address: 0x%x", bfar);
			if (from_hard_fault)
			{
				/* clear SCB_CFSR_BFAR[VALID] to reset */
				SCB->CFSR &= ~SCB_CFSR_BFARVALID_Msk;
			}
		}
	}
	if (SCB->CFSR & SCB_CFSR_IMPRECISERR_Msk) 
	{
		PR_FAULT_INFO("  Imprecise data bus error");
	}
	if ((SCB->CFSR & SCB_CFSR_IBUSERR_Msk) != 0) 
	{
		PR_FAULT_INFO("  Instruction bus error");
#if !defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	}
#else
	} else if (SCB->CFSR & SCB_CFSR_LSPERR_Msk) 
	{
		PR_FAULT_INFO("  Floating-point lazy state preservation error");
	}
#endif /* !defined(CONFIG_ARMV7_M_ARMV8_M_FP) */

#if defined(CONFIG_ARM_MPU) && defined(CONFIG_CPU_HAS_NXP_MPU)
	u32_t sperr = SYSMPU->CESR & SYSMPU_CESR_SPERR_MASK;
	u32_t mask = BIT(31);
	sword_t i;
	u32_t ear = -EINVAL;

	if (sperr) {
		for (i = 0; i < SYSMPU_EAR_COUNT; i++, mask >>= 1)
		{
			if ((sperr & mask) == 0U) 
			{
				continue;
			}
			STORE_xFAR(edr, SYSMPU->SP[i].EDR);
			ear = SYSMPU->SP[i].EAR;

			PR_FAULT_INFO("  NXP MPU error, port %d", i);
			PR_FAULT_INFO("    Mode: %s, %s Address: 0x%x",
			       edr & BIT(2) ? "Supervisor" : "User",
			       edr & BIT(1) ? "Data" : "Instruction",
			       ear);
			PR_FAULT_INFO(
					"    Type: %s, Master: %d, Regions: 0x%x",
			       edr & BIT(0) ? "Write" : "Read",
			       EMN(edr), EACD(edr));

			/* When stack protection is enabled, we need to assess
			 * if the memory violation error is a stack corruption.
			 *
			 * By design, being a Stacking Bus fault is a necessary
			 * and sufficient condition for a stack corruption.
			 */
			if (SCB->CFSR & SCB_CFSR_STKERR_Msk) 
			{
#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
				/* Note: we can assume the fault originated
				 * from the same security state for ARM
				 * platforms implementing the NXP MPU
				 * (CONFIG_CPU_HAS_NXP_MPU=y).
				 *
				 * As we only assess thread stack corruption,
				 * we only process the error further, if the
				 * stack page_f is on PSP. For NXP MPU-related
				 * Bus Faults (banked), this is equivalent to
				 * inspecting the RETTOBASE flag.
				 */
				if (SCB->ICSR & SCB_ICSR_RETTOBASE_Msk) 
				{
					u32_t min_stack_ptr =
						checktcb_stack_fail(ear, 
						((u32_t) &esf[0]));
					if (min_stack_ptr) 
					{
						/* When BusFault Stacking Error
						 * has occurred, the stack
						 * context page_f might be
						 * corrupted but the stack
						 * pointer may have actually
						 * moved. We may face problems
						 * with un-stacking the page_f,
						 * upon exception return, if we
						 * do not have sufficient
						 * permissions to read the
						 * corrupted stack page_f.
						 * Therefore, we manually force
						 * the stack pointer to the
						 * lowest allowed position.
						 *
						 * Note:
						 * The PSP will normally be
						 * adjusted in a tail-chained
						 * exception performing context
						 * switch, after aborting the
						 * corrupted thread. Here, the
						 * adjustment is required as
						 * tail-chain cannot always be
						 * guaranteed.
						 */
						__set_PSP(min_stack_ptr);

						reason = fatal_stack_check_fatal;
						break;
					}
				}
#else
				(void)ear;
				user_error("Stacking error without stack right"
					"or User-mode support");
#endif /* CONFIG_MPU_STACK_GUARD || CONFIG_USERSPACE */
			}
		}
		SYSMPU->CESR &= ~sperr;
	}
#endif /* defined(CONFIG_ARM_MPU) && defined(CONFIG_CPU_HAS_NXP_MPU) */

	/* clear BFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_BUSFAULTSR_Msk;
	*recoverable = memory_fault_recoverable(esf);

	return reason;
}

/**
 *
 * @brief Dump UsageFault information
 *
 * See arm_fault_dump() for example.
 *
 * @return error code to identify the fatal error reason
 */
static u32_t usage_fault(const arch_esf_t *esf)
{
	u32_t reason = fatal_cpu_exception_fatal;

	PR_FAULT_INFO("***** USAGE FAULT *****");

	/* bits are sticky: they stack and must be reset */
	if ((SCB->CFSR & SCB_CFSR_DIVBYZERO_Msk) != 0)
	{
		PR_FAULT_INFO("  Division by zero");
	}
	if ((SCB->CFSR & SCB_CFSR_UNALIGNED_Msk) != 0) 
	{
		PR_FAULT_INFO("  Unaligned memory access");
	}
#if defined(CONFIG_ARMV8_M_MAINLINE)
	if ((SCB->CFSR & SCB_CFSR_STKOF_Msk) != 0) 
	{
		PR_FAULT_INFO("  Stack overflow (context area not valid)");
#if defined(CONFIG_BUILTIN_STACK_GUARD)
		/* Stack Overflows are always reported as stack corruption
		 * errors. Note that the built-in stack overflow mechanism
		 * prevents the context area to be loaded on the stack upon
		 * UsageFault exception entry. As a result, we cannot rely
		 * on the reported faulty instruction address, to determine
		 * the instruction that triggered the stack overflow.
		 */
		reason = fatal_stack_check_fatal;
#endif /* CONFIG_BUILTIN_STACK_GUARD */
	}
#endif /* CONFIG_ARMV8_M_MAINLINE */
	if ((SCB->CFSR & SCB_CFSR_NOCP_Msk) != 0) 
	{
		PR_FAULT_INFO("  No coprocessor instructions");
	}
	if ((SCB->CFSR & SCB_CFSR_INVPC_Msk) != 0) 
	{
		PR_FAULT_INFO("  Illegal load of EXC_RETURN into PC");
	}
	if ((SCB->CFSR & SCB_CFSR_INVSTATE_Msk) != 0)
	{
		PR_FAULT_INFO("  Illegal use of the EPSR");
	}
	if ((SCB->CFSR & SCB_CFSR_UNDEFINSTR_Msk) != 0)
	{
		PR_FAULT_INFO("  Attempt to execute undefined instruction");
	}

	/* clear UFSR sticky bits */
	SCB->CFSR |= SCB_CFSR_USGFAULTSR_Msk;

	return reason;
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
/**
 *
 * @brief Dump SecureFault information
 *
 * See arm_fault_dump() for example.
 *
 * @return N/A
 */
static void secure_fault(const arch_esf_t *esf)
{
	PR_FAULT_INFO("***** SECURE FAULT *****");

	STORE_xFAR(sfar, SAU->SFAR);
	if ((SAU->SFSR & SAU_SFSR_SFARVALID_Msk) != 0) 
	{
		PR_EXC("  Address: 0x%x", sfar);
	}

	/* bits are sticky: they stack and must be reset */
	if ((SAU->SFSR & SAU_SFSR_INVEP_Msk) != 0)
	{
		PR_FAULT_INFO("  Invalid entry point");
	} 
	else if ((SAU->SFSR & SAU_SFSR_INVIS_Msk) != 0)
	{
		PR_FAULT_INFO("  Invalid integrity signature");
	} 
	else if ((SAU->SFSR & SAU_SFSR_INVER_Msk) != 0)
	{
		PR_FAULT_INFO("  Invalid exception return");
	} 
	else if ((SAU->SFSR & SAU_SFSR_AUVIOL_Msk) != 0) 
	{
		PR_FAULT_INFO("  Attribution unit violation");
	}
	else if ((SAU->SFSR & SAU_SFSR_INVTRAN_Msk) != 0) 
	{
		PR_FAULT_INFO("  Invalid transition");
	} 
	else if ((SAU->SFSR & SAU_SFSR_LSPERR_Msk) != 0)
	{
		PR_FAULT_INFO("  Lazy state preservation");
	} 
	else if ((SAU->SFSR & SAU_SFSR_LSERR_Msk) != 0)
	{
		PR_FAULT_INFO("  Lazy state error");
	}

	/* clear SFSR sticky bits */
	SAU->SFSR |= 0xFF;
}
#endif /* defined(CONFIG_ARM_SECURE_FIRMWARE) */

/**
 *
 * @brief Dump debug monitor exception information
 *
 * See arm_fault_dump() for example.
 *
 * @return N/A
 */
static void debug_monitor(const arch_esf_t *esf)
{
	ARG_UNUSED(esf);

	PR_FAULT_INFO(
		"***** Debug monitor exception (not implemented) *****");
}

#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

/**
 *
 * @brief Dump hard fault information
 *
 * See arm_fault_dump() for example.
 *
 * @return error code to identify the fatal error reason
 */
static u32_t hard_fault(arch_esf_t *esf, bool *recoverable)
{
	u32_t reason = fatal_cpu_exception_fatal;

	PR_FAULT_INFO("***** HARD FAULT *****");

#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* Workaround for #18712:
	 * HardFault may be due to escalation, as a result of
	 * an SVC instruction that could not be executed; this
	 * can occur if ARCH_CATCH_EXCEPTION() is called by an ISR,
	 * which executes at sched_prior equal to the SVC handler
	 * sched_prior. We handle the case of Kernel OOPS and Stack
	 * Fail here.
	 */
	u16_t *ret_addr = (u16_t *)esf->basic.pc;
	/* SVC is a 16-bit instruction. On a synchronous SVC
	 * escalated to Hard Fault, the return address is the
	 * next instruction, i.e. after the SVC.
	 */
#define _SVC_OPCODE 0xDF00

	u16_t fault_insn = *(ret_addr - 1);
	if (((fault_insn & 0xff00) == _SVC_OPCODE) &&
		((fault_insn & 0x00ff) == svc_runtime_exception_svc)) 
	{
		PR_EXC("ARCH_CATCH_EXCEPTION with reason %x\n", esf->basic.r0);
		reason = esf->basic.r0;
	}
#undef _SVC_OPCODE

	*recoverable = memory_fault_recoverable(esf);
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	*recoverable = false;

	if ((SCB->HFSR & SCB_HFSR_VECTTBL_Msk) != 0)
	{
		PR_EXC("  Bus fault on vector table read");
	} 
	else if ((SCB->HFSR & SCB_HFSR_FORCED_Msk) != 0)
	{
		PR_EXC("  Fault escalation (see below)");
		if (SCB_MMFSR != 0) 
		{
			reason = mem_manage_fault(esf, 1, recoverable);
		} 
		else if (SCB_BFSR != 0) 
		{
			reason = bus_fault(esf, 1, recoverable);
		} 
		else if (SCB_UFSR != 0) 
		{
			reason = usage_fault(esf);
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
		} 
		else if (SAU->SFSR != 0)
		{
			secure_fault(esf);
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
		}
	}
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

	return reason;
}

/**
 *
 * @brief Dump reserved exception information
 *
 * See arm_fault_dump() for example.
 *
 * @return N/A
 */
static void reserved_exception(const arch_esf_t *esf, sword_t fault)
{
	ARG_UNUSED(esf);

	PR_FAULT_INFO("***** %s %d) *****",
	       fault < 16 ? "Reserved Exception (" : "Spurious interrupt (IRQ ",
	       fault - 16);
}

/* Handler function for ARM fault conditions. */
static u32_t fault_handle(arch_esf_t *esf, sword_t fault, bool *recoverable)
{
	u32_t reason = fatal_cpu_exception_fatal;

	*recoverable = false;

	switch (fault) 
	{
		case 3:
			reason = hard_fault(esf, recoverable);
			break;
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
		/* HardFault is raised for all fault conditions on ARMv6-M. */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
		case 4:
			reason = mem_manage_fault(esf, 0, recoverable);
			break;
		case 5:
			reason = bus_fault(esf, 0, recoverable);
			break;
		case 6:
			reason = usage_fault(esf);
			break;
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
		case 7:
			secure_fault(esf);
			break;
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
		case 12:
			debug_monitor(esf);
			break;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
		default:
			reserved_exception(esf, fault);
			break;
	}

	if ((*recoverable) == false) 
	{
		/* Dump generic information about the fault. */
		fault_show(esf, fault);
	}

	return reason;
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
#if (CONFIG_FAULT_DUMP == 2)
/**
 * @brief Dump the Secure Stack information for an exception that
 * has occurred in Non-Secure state.
 *
 * @param secure_esf Pointer to the secure stack page_f.
 */
static void secure_stack_dump(const arch_esf_t *secure_esf)
{
	/*
	 * In case a Non-Secure exception interrupted the Secure
	 * execution, the Secure state has stacked the additional
	 * state context and the top of the stack contains the
	 * integrity signature.
	 *
	 * In case of a Non-Secure function call the top of the
	 * stack contains the return address to Secure state.
	 */
	u32_t *top_of_sec_stack = (u32_t *)secure_esf;
	u32_t sec_ret_addr;
#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
	if ((*top_of_sec_stack == INTEGRITY_SIGNATURE_STD) ||
		(*top_of_sec_stack == INTEGRITY_SIGNATURE_EXT))
	{
#else
	if (*top_of_sec_stack == INTEGRITY_SIGNATURE) 
	{
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */
		/* Secure state interrupted by a Non-Secure exception.
		 * The return address after the additional state
		 * context, stacked by the Secure code upon
		 * Non-Secure exception entry.
		 */
		top_of_sec_stack += ADDITIONAL_STATE_CONTEXT_WORDS;
		secure_esf = (const arch_esf_t *)top_of_sec_stack;
		sec_ret_addr = secure_esf->basic.pc;
	} 
	else
	{
		/* Exception during Non-Secure function call.
		 * The return address is located on top of stack.
		 */
		sec_ret_addr = *top_of_sec_stack;
	}
	
	PR_FAULT_INFO("  S instruction address:  0x%x", sec_ret_addr);

}
#define SECURE_STACDUMP(esf) secure_stack_dump(esf)
#else
/* We do not dump the Secure stack information for lower dump levels. */
#define SECURE_STACDUMP(esf)
#endif /* CONFIG_FAULT_DUMP== 2 */
#endif /* CONFIG_ARM_SECURE_FIRMWARE */

/*
 * This internal function does the following:
 *
 * - Retrieves the exception stack page_f
 * - Evaluates whether to report being in a int_nest_count exception
 *
 * If the ESF is not successfully retrieved, the function signals
 * an error by returning NULL.
 *
 * @return ESF pointer on success, otherwise return NULL
 */
static FORCE_INLINE arch_esf_t *get_esf(u32_t msp, u32_t psp, u32_t exc_return,
	bool *irq_nested_exc)
{
	bool alternative_state_exc = false;
	arch_esf_t *ptr_esf;

	*irq_nested_exc = false;

	if ((exc_return & EXC_RETURN_INDICATOR_PREFIX) !=
			EXC_RETURN_INDICATOR_PREFIX) 
	{
		/* Invalid EXC_RETURN value. This is a fatal error. */
		return NULL;
	}

#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	if ((exc_return & EXC_RETURN_EXCEPTION_SECURE_Secure) == 0U)
	{
		/* Secure Firmware shall only handle Secure Exceptions.
		 * This is a fatal error.
		 */
		return NULL;
	}

	if (exc_return & EXC_RETURN_RETURN_STACSecure) 
	{
		/* Exception entry occurred in Secure stack. */
	} 
	else 
	{
		/* Exception entry occurred in Non-Secure stack. Therefore,
		 * msp/psp point to the Secure stack, however, the actual
		 * exception stack page_f is located in the Non-Secure stack.
		 */
		alternative_state_exc = true;

		/* Dump the Secure stack before handling the actual fault. */
		arch_esf_t *secure_esf;

		if (exc_return & EXC_RETURN_SPSEL_PROCESS) 
		{
			/* Secure stack pointed by PSP */
			secure_esf = (arch_esf_t *)psp;
		} 
		else 
		{
			/* Secure stack pointed by MSP */
			secure_esf = (arch_esf_t *)msp;
			*irq_nested_exc = true;
		}

		SECURE_STACDUMP(secure_esf);

		/* Handle the actual fault.
		 * Extract the correct stack page_f from the Non-Secure state
		 * and supply it to the fault handing function.
		 */
		if (exc_return & EXC_RETURN_MODE_THREAD)
		{
			ptr_esf = (arch_esf_t *)__Tget_PSP_NS();
		} 
		else 
		{
			ptr_esf = (arch_esf_t *)__Tget_MSP_NS();
		}
	}
#elif defined(CONFIG_ARM_NONSECURE_FIRMWARE)
	if (exc_return & EXC_RETURN_EXCEPTION_SECURE_Secure) 
	{
		/* Non-Secure Firmware shall only handle Non-Secure Exceptions.
		 * This is a fatal error.
		 */
		return NULL;
	}

	if (exc_return & EXC_RETURN_RETURN_STACSecure)
	{
		/* Exception entry occurred in Secure stack.
		 *
		 * Note that Non-Secure firmware cannot inspect the Secure
		 * stack to determine the root cause of the fault. Fault
		 * inspection will indicate the Non-Secure instruction
		 * that performed the branch to the Secure domain.
		 */
		alternative_state_exc = true;

		PR_FAULT_INFO("Exception occurred in Secure State");

		if (exc_return & EXC_RETURN_SPSEL_PROCESS)
		{
			/* Non-Secure stack page_f on PSP */
			ptr_esf = (arch_esf_t *)psp;
		} 
		else
		{
			/* Non-Secure stack page_f on MSP */
			ptr_esf = (arch_esf_t *)msp;
		}
	} 
	else 
	{
		/* Exception entry occurred in Non-Secure stack. */
	}
#else
	/* The processor has a single execution state.
	 * We verify that the Thread mode is using PSP.
	 */
	if ((exc_return & EXC_RETURN_MODE_THREAD) &&
		(!(exc_return & EXC_RETURN_SPSEL_PROCESS)))
	{
		PR_EXC("SPSEL in thread mode does not indicate PSP");
		return NULL;
	}
#endif /* CONFIG_ARM_SECURE_FIRMWARE */

	if (!alternative_state_exc) 
	{
		if (exc_return & EXC_RETURN_MODE_THREAD) 
		{
			/* Returning to thread mode */
			ptr_esf =  (arch_esf_t *)psp;
		} 
		else
		{
			/* Returning to handler mode */
			ptr_esf = (arch_esf_t *)msp;
			*irq_nested_exc = true;
		}
	}

	return ptr_esf;
}

/**
 *
 * @brief ARM Fault handler
 *
 * This routine is called when fatal error conditions are detected by hardware
 * and is responsible for:
 * - resetting the processor fault status registers (for the case when the
 *   error handling policy allows the system to recover from the error),
 * - reporting the error information,
 * - determining the error reason to be provided as input to the user-
 *   provided routine, system_halt_handler().
 * The system_halt_handler() is invoked once the above operations are
 * completed, and is responsible for implementing the error handling policy.
 *
 * The function needs, first, to determine the exception stack page_f.
 * Note that the _current_thread security state might not be the actual
 * state in which the processor was executing, when the exception occurred.
 * The actual state may need to be determined by inspecting the EXC_RETURN
 * value, which is provided as argument to the Fault handler.
 *
 * If the exception occurred in the same security state, the stack page_f
 * will be pointed to by either MSP or PSP depending on the processor
 * execution state when the exception occurred. MSP and PSP values are
 * provided as arguments to the Fault handler.
 *
 * @param msp MSP value immediately after the exception occurred
 * @param psp PSP value immediately after the exception occurred
 * @param exc_return EXC_RETURN value present in LR after exception entry.
 *
 */
extern void arm_fatal_error(word_t reason, const arch_esf_t *esf);
	
void arm_fault(u32_t msp, u32_t psp, u32_t exc_return)
{
	u32_t reason = fatal_cpu_exception_fatal;
	sword_t fault = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
	bool recoverable, irq_nested_exc;
	arch_esf_t *esf;

	/* Create a stack-ed copy of the ESF to be used during
	 * the fault handling process.
	 */
	arch_esf_t esf_copy;

	/* Force unlock interrupts */
	arch_irq_unlock(0);

	/* Retrieve the Exception Stack Frame (ESF) to be supplied
	 * as argument to the remainder of the fault handling process.
	 */
	 esf = get_esf(msp, psp, exc_return, &irq_nested_exc);
	assert_info(esf != NULL,
		"ESF could not be retrieved successfully. Shall never occur.");

	reason = fault_handle(esf, fault, &recoverable);
	if (recoverable) 
	{
		return;
	}

	/* Copy ESF */
	memcpy(&esf_copy, esf, sizeof(arch_esf_t));

	/* Overwrite stacked IPSR to mark a int_nest_count exception,
	 * or a return to Thread mode. Note that this may be
	 * required, if the retrieved ESF contents are invalid
	 * due to, for instance, a stacking error.
	 */
	if (irq_nested_exc) 
	{
		if ((esf_copy.basic.xpsr & IPSR_ISR_Msk) == 0)
		{
			esf_copy.basic.xpsr |= IPSR_ISR_Msk;
		}
	}
	else 
	{
		esf_copy.basic.xpsr &= ~(IPSR_ISR_Msk);
	}

	arm_fatal_error(reason, &esf_copy);
}

/**
 *
 * @brief Initialization of fault handling
 *
 * Turns on the desired hardware faults.
 *
 * @return N/A
 */
void arm_fault_init(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* If Stack guarding via SP limit checking is enabled, disable
	 * SP limit checking inside HardFault and NMI. This is done
	 * in order to allow for the desired fault logging to execute
	 * properly in all cases.
	 *
	 * Note that this could allow a Secure Firmware Main Stack
	 * to descend into non-secure region during HardFault and
	 * NMI exception entry. To prevent from this, non-secure
	 * memory regions must be located higher than secure memory
	 * regions.
	 *
	 * For Non-Secure Firmware this could allow the Non-Secure Main
	 * Stack to attempt to descend into secure region, in which case a
	 * Secure Hard Fault will occur and we can track the fault from there.
	 */
	SCB->CCR |= SCB_CCR_STKOFHFNMIGN_Msk;
#endif /* CONFIG_BUILTIN_STACK_GUARD */
}
