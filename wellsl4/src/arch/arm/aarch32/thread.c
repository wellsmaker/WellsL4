/**
 * @file
 * @brief New thread creation for ARM Cortex-M and Cortex-R
 *
 * Core thread related primitives for the ARM Cortex-M and Cortex-R
 * processor architecture.
 */

#include <types_def.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <kernel/thread.h>
#include <kernel/stack.h>
#include <object/tcb.h>
#include <arch/thread.h>
#include <arch/cpu.h>
#include <model/entryhandler.h>
#include <state/statedata.h>
#include <sys/util.h>
#include <sys/errno.h>


/* stacks */
#define STACK_ROUND_UP(x) ROUND_UP(x, THREAD_STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, THREAD_STACK_ALIGN_SIZE)

#ifdef CONFIG_USERSPACE
extern u8_t *priv_stack_find(void *obj);
#endif

/* An initial context, to be "restored" by arm_pendsv(), is put at the other
 * end of the stack, and thus reusable by the stack when not needed anymore.
 *
 * The initial context is an exception stack page_f (ESF) since exiting the
 * PendSV exception will want to pop an ESF. Interestingly, even if the lsb of
 * an instruction address to jump to must always be set since the CPU always
 * runs in thumb mode, the ESF expects the real address of the instruction,
 * with the lsb *not* set (instructions are always aligned on 16 bit
 * halfwords).  Since the compiler automatically sets the lsb of function
 * addresses, we have to unset it manually before storing it in the 'pc' field
 * of the ESF.
 */
void arch_new_thread(struct ktcb *thread, struct thread_stack *stack, 
	size_t stack_size, thread_entry_t entry_ptr, void *para1, void *para2, 
	void *para3, word_t options)
{
	char *stack_start = THREAD_STACK_BUFFER(stack);
	char *stack_end;
	/* Offset between the top of stack and the high end of stack area. */
	word_t top_of_stack_offset = 0U;

#if defined(CONFIG_USERSPACE)
	/* Truncate the stack size to align with the MPU region granularity.
	 * This is done proactively to account for the case when the thread
	 * switches to user mode (thus, its stack area will need to be MPU-
	 * programmed to be assigned unprivileged RW access permission).
	 */
	stack_size &= ~(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE - 1);
#endif

#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT) && defined(CONFIG_USERSPACE)
	/* This is required to work-around the case where the thread
	 * is created without using THREAD_STACK_SIZEOF() macro in
	 * ktcb_create(). If THREAD_STACK_SIZEOF() is used, the
	 * Guard size has already been take out of stack_size.
	 */
	stack_size -= MPU_GUARD_ALIGN_AND_SIZE;
#endif

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING) && defined(CONFIG_MPU_STACK_GUARD)
	/* For a thread which intends to use the FP services, it is required to
	 * allocate a wider MPU right region, to always successfully detect an
	 * overflow of the stack.
	 *
	 * Note that the wider MPU regions requires re-adjusting the stack_info
	 * .start and .size.
	 *
	 */
	if ((options & option_fp_option) != 0) 
	{
		stack_start += MPU_GUARD_ALIGN_AND_SIZE_FLOAT - MPU_GUARD_ALIGN_AND_SIZE;
		stack_size -= MPU_GUARD_ALIGN_AND_SIZE_FLOAT - MPU_GUARD_ALIGN_AND_SIZE;
	}
#endif

	stack_end = stack_start + stack_size;

	struct esf *init_ctx;

	thread_init(thread, stack_start, stack_size, options);

	/* Carve the thread entry struct from the "base" of the stack
	 *
	 * The initial carved stack page_f only needs to contain the basic
	 * stack page_f (state context), because no FP operations have been
	 * performed yet for this thread.
	 */
	init_ctx = (struct esf *)(STACK_ROUND_DOWN(stack_end -
		(char *)top_of_stack_offset - sizeof(struct basic_esf)));

#if defined(CONFIG_USERSPACE)
	if ((options & option_user_option) != 0) 
	{
		init_ctx->basic.pc = (word_t)arch_user_mode_enter;
	} 
	else
	{
		init_ctx->basic.pc = (word_t)thread_entry_point;
	}
#else
	init_ctx->basic.pc = (word_t)thread_entry_point;
#endif

#if defined(CONFIG_CPU_CORTEX_M)
	/* force ARM mode by clearing LSB of address */
	init_ctx->basic.pc &= 0xfffffffe;
#endif

	init_ctx->basic.a1 = (word_t)entry_ptr;
	init_ctx->basic.a2 = (word_t)para1;
	init_ctx->basic.a3 = (word_t)para2;
	init_ctx->basic.a4 = (word_t)para3;
	 /* clear all, thumb bit is 1, even if RO */
	init_ctx->basic.xpsr = 0x01000000UL;
	thread->callee_saved.psp = (word_t)init_ctx;
	
#if defined(CONFIG_CPU_CORTEX_R)
	init_ctx->basic.lr = (word_t)init_ctx->basic.pc;
	thread->callee_saved.spsr = A_BIT | T_BIT | MODE_SYS;
	thread->callee_saved.lr = (word_t)init_ctx->basic.pc;
#endif

	thread->arch.basepri = 0;

#if defined(CONFIG_USERSPACE) || defined(CONFIG_FP_SHARING)
	thread->arch.mode = 0;
#if defined(CONFIG_USERSPACE)
	thread->arch.priv_stack_start = 0;
#endif
#endif

	/* swap_return_value can contain garbage */
	/*
	 * initial values in all other registers/thread entries are
	 * irrelevant.
	 */
}

#ifdef CONFIG_USERSPACE
extern FUNC_NORETURN void arm_userspace_enter(thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       u32_t stack_end,
					       u32_t stack_start);

FUNC_NORETURN void arch_user_mode_enter(thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{

	/* Set up privileged stack before entering user mode */
	_current_thread->arch.priv_stack_start =
		(word_t)priv_stack_find(_current_thread->userspace_stack_point);
#if defined(CONFIG_MPU_STACK_GUARD)
	/* Stack right area reserved at the bottom of the thread's
	 * privileged stack. Adjust the available (writable) stack
	 * buffer area accordingly.
	 */
#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	_current_thread->arch.priv_stack_start +=
		(_current_thread->base.option & option_fp_option) ?
		MPU_GUARD_ALIGN_AND_SIZE_FLOAT : MPU_GUARD_ALIGN_AND_SIZE;
#else
	_current_thread->arch.priv_stack_start += MPU_GUARD_ALIGN_AND_SIZE;
#endif
#endif
	arm_userspace_enter(user_entry, p1, p2, p3,
		(word_t)_current_thread->stack_info.start,
		_current_thread->stack_info.size);
	CODE_UNREACHABLE;
}

bool arm_thread_is_in_user_mode(void)
{
	uint32_t value;

#if defined(CONFIG_CPU_CORTEX_M)
	/* return mode information */
	value = __get_CONTROL();
	return (value & CONTROL_nPRIV_Msk) != 0;
#else
	/*
	 * For Cortex-R, the mode (lower 5) bits will be 0x10 for user mode.
	 */
	value = __get_CPSR();
	return ((value & CPSR_M_Msk) == CPSR_M_USR);
#endif
}

#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
/*
 * @brief Configure ARM built-in stack right
 *
 * This function configures per thread stack guards by reprogramming
 * the built-in Process Stack Pointer Limit Register (PSPLIM).
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
 
/*
 * This option signifies the CPU has the MSPLIM, PSPLIM registers.

 * The stack pointer limit registers, MSPLIM, PSPLIM, limit the
 * extend to which the Main and Process Stack Pointers, respectively,
 * can descend. MSPLIM, PSPLIM are always present in ARMv8-M
 * MCUs that implement the ARMv8-M Main Extension (Mainline).

 * In an ARMv8-M Mainline implementation with the Security Extension
 * the MSPLIM, PSPLIM registers have additional Secure instances.
 * In an ARMv8-M Baseline implementation with the Security Extension
 * the MSPLIM, PSPLIM registers have only Secure instances.
*/
void configure_builtin_stack_guard(struct ktcb *thread)
{
#if defined(CONFIG_USERSPACE)
	if ((thread->arch.mode & CONTROL_nPRIV_Msk) != 0)
	{
		/* Only configure stack limit for record_threads in privileged mode
		 * (i.e supervisor record_threads or user record_threads doing system call).
		 * User record_threads executing in user mode do not require a stack
		 * limit protection.
		 */
		__set_PSPLIM(0);
		return;
	}
	/* Only configure PSPLIM to right the privileged stack area, if
	 * the thread is currently using it, otherwise right the default
	 * thread stack. Note that the conditional check relies on the
	 * thread privileged stack being allocated in higher memory area
	 * than the default thread stack (ensured by design).
	 */
	word_t guard_start =
		((thread->arch.priv_stack_start) && (__get_PSP() >= thread->arch.priv_stack_start)) ?
		(word_t)thread->arch.priv_stack_start :
		(word_t)thread->userspace_stack_point;

	assert_info(thread->stack_info.start == ((word_t)thread->userspace_stack_point),
		"stack_info.start does not point to the start of the"
		"thread allocated area.");
#else
	word_t guard_start = thread->stack_info.start;
#endif
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM(guard_start);
#else
#error "Built-in PSP limit checks not supported by HW"
#endif
}
#endif

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
#define IS_MPU_GUARD_VIOLATION(guard_start, guard_len, fault_addr, stack_start) \
	((fault_addr == -EINVAL) ? \
	((fault_addr >= guard_start) && \
	(fault_addr < (guard_start + guard_len)) && \
	(stack_start < (guard_start + guard_len))) \
	: \
	(stack_start < (guard_start + guard_len)))

/**
 * @brief Assess occurrence of _current_thread thread's stack corruption
 *
 * This function performs an assessment whether a memory fault (on a
 * given memory address) is the result of stack memory corruption of
 * the _current_thread thread.
 *
 * Thread stack corruption for supervisor record_threads or user record_threads in
 * privilege mode (when User Space is supported) is reported upon an
 * attempt to access the stack right area (if MPU Stack Guard feature
 * is supported). Additionally the _current_thread PSP (process stack pointer)
 * must be pointing inside or below the right area.
 *
 * Thread stack corruption for user record_threads in user mode is reported,
 * if the _current_thread PSP is pointing below the start of the _current_thread
 * thread's stack.
 *
 * Notes:
 * - we assume a fully descending stack,
 * - we assume a stacking error has occurred,
 * - the function shall be called when handling MemManage and Bus fault,
 *   and only if a Stacking error has been reported.
 *
 * If stack corruption is detected, the function returns the lowest
 * allowed address where the Stack Pointer can safely point to, to
 * prevent from errors when un-stacking the corrupted stack page_f
 * upon exception return.
 *
 * @param fault_addr memory address on which memory access violation
 *                   has been reported. It can be invalid (-EINVAL),
 *                   if only Stacking error has been reported.
 * @param psp        _current_thread address the PSP points to
 *
 * @return The lowest allowed stack page_f pointer, if error is a
 *         thread stack corruption, otherwise return 0.
 */
word_t checktcb_stack_fail(const word_t fault_addr, const word_t psp)
{
	const struct ktcb *thread = _current_thread;

	if (!thread)
	{
		return 0;
	}

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	word_t guard_len = (thread->base.option & option_fp_option) ?
		MPU_GUARD_ALIGN_AND_SIZE_FLOAT : MPU_GUARD_ALIGN_AND_SIZE;
#else
	word_t guard_len = MPU_GUARD_ALIGN_AND_SIZE;
#endif

#if defined(CONFIG_USERSPACE)
	if (thread->arch.priv_stack_start) 
	{
		/* User thread */
		if ((__get_CONTROL() & CONTROL_nPRIV_Msk) == 0) 
		{
			/* User thread in privilege mode */
			if (IS_MPU_GUARD_VIOLATION( thread->arch.priv_stack_start - guard_len,
				guard_len, fault_addr, psp)) 
			{
				/* Thread's privilege stack corruption */
				return thread->arch.priv_stack_start;
			}
		}
		else
		{
			if (psp < (word_t)thread->userspace_stack_point) 
			{
				/* Thread's user stack corruption */
				return (word_t)thread->userspace_stack_point;
			}
		}
	}
	else 
	{
		/* Supervisor thread */
		if (IS_MPU_GUARD_VIOLATION(thread->stack_info.start - guard_len,
				guard_len, fault_addr, psp)) 
		{
			/* Supervisor thread stack corruption */
			return thread->stack_info.start;
		}
	}
#else /* CONFIG_USERSPACE */
	if (IS_MPU_GUARD_VIOLATION(thread->stack_info.start - guard_len,
		guard_len, fault_addr, psp))
	{
		/* Thread stack corruption */
		return thread->stack_info.start;
	}
#endif

	return 0;
}
#endif

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
sword_t arch_float_disable(struct ktcb *thread)
{
	if (thread != _current_thread) 
	{
		return -EINVAL;
	}

	if (arch_is_in_isr())
	{
		return -EINVAL;
	}

	/* Disable all floating point capabilities for the thread */

	/* FP_REG flag is used in SWAP and stack check fail. Locking
	 * interrupts here prevents a possible context-switch or MPU
	 * fault to take an outdated thread option flag into
	 * account.
	 */
	sword_t key = arch_irq_lock();

	thread->base.option &= ~option_fp_option;

	__set_CONTROL(__get_CONTROL() & (~CONTROL_FPCA_Msk));

	/* No need to add an ISB barrier after setting the CONTROL
	 * register; arch_irq_unlock() already adds one.
	 */

	arch_irq_unlock(key);

	return 0;
}
#endif /* CONFIG_FLOAT && CONFIG_FP_SHARING */

void arch_switch_to_main_thread(struct ktcb *_main_thread,
				struct thread_stack *_main_stack,
				size_t _main_stack_size,
				thread_entry_t _main)
{
#if defined(CONFIG_FLOAT)
	/* Initialize the Floating Point Status and Control Register when in
	 * Unshared FP Registers mode (In Shared FP Registers mode, FPSCR is
	 * initialized at thread creation for record_threads that make use of the FP).
	 */
	__set_FPSCR(0);
#if defined(CONFIG_FP_SHARING)
	/* In Sharing mode clearing FPSCR may set the CONTROL.FPCA flag. */
	__set_CONTROL(__get_CONTROL() & (~(CONTROL_FPCA_Msk)));
	__ISB();
#endif /* CONFIG_FP_SHARING */
#endif /* CONFIG_FLOAT */

#ifdef CONFIG_ARM_MPU
	extern void arm_configure_static_mpu_regions(void);
	/* Configure static memory map. This will program MPU regions,
	 * to set up access permissions for fixed memory sections, such
	 * as Application Memory or No-Cacheable SRAM area.
	 *
	 * This function is invoked once, upon system initialization.
	 */
	arm_configure_static_mpu_regions();
#endif

	/* get high address of the stack, i.e. its start (stack grows down) */
	char *start_of__main_stack;

	start_of__main_stack = THREAD_STACK_BUFFER(_main_stack) + _main_stack_size;
	start_of__main_stack = (char *)STACK_ROUND_DOWN(start_of__main_stack);

	_current_thread = _main_thread;
#ifdef CONFIG_TRACING
	extern void sys_trace_thread_switched_in(void);
	sys_trace_thread_switched_in();
#endif

	/* the ready queue cache already contains the main thread */

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
	/*
	 * If stack protection is enabled, make sure to set it
	 * before jumping to thread entry function
	 */
	extern void arm_configure_dynamic_mpu_regions(struct ktcb *thread);
	arm_configure_dynamic_mpu_regions(_main_thread);
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* Set PSPLIM register for built-in stack guarding of main thread. */
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM((word_t)_main_stack);
#else
#error "Built-in PSP limit checks not supported by HW"
#endif
#endif /* CONFIG_BUILTIN_STACK_GUARD */

	/*
	 * Set PSP to the highest address of the main stack
	 * before enabling interrupts and jumping to main.
	 */
	__asm__ volatile (
	"mov   r0,  %0\n\t"	/* Store _main in R0 */
#if defined(CONFIG_CPU_CORTEX_M)
	"msr   PSP, %1\n\t"	/* __set_PSP(start_of__main_stack) */
#endif

	"movs r1, #0\n\t"
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE) \
			|| defined(CONFIG_ARMV7_R)
	"cpsie i\n\t"		/* __enable_irq() */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	"cpsie if\n\t"		/* __enable_irq(); __enable_fault_irq() */
	"msr   BASEPRI, r1\n\t"	/* __set_BASEPRI(0) */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	"isb\n\t"
	"movs r2, #0\n\t"
	"movs r3, #0\n\t"
	"bl thread_entry_point\n\t"	/* thread_entry_point(_main, 0, 0, 0); */
	:
	: "r" (_main), "r" (start_of__main_stack)
	: "r0" /* not to be overwritten by msr PSP, %1 */
	);

	CODE_UNREACHABLE;
}
