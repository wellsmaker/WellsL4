

/**
 * @file
 * @brief New thread creation for ARM64 Cortex-A
 *
 * Core thread related primitives for the ARM64 Cortex-A
 */

#include <object/tcb.h>
#include <kernel/thread.h>
#include <kernel/stack.h>
#include <arch/cpu.h>
#include <arch/arm/kernel_data.h>
/**
 *
 * @brief Initialize a new thread from its stack space
 *
 * The control structure (thread) is put at the lower address of the stack. An
 * initial context, to be "restored" by arm64_pendsv(), is put at the other
 * end of the stack, and thus reusable by the stack when not needed anymore.
 *
 * <options> is currently unused.
 *
 * @param stack      pointer to the aligned stack memory
 * @param stack_size  size of the available stack memory in bytes
 * @param entry_ptr the entry point
 * @param para1 entry point to the first param
 * @param para2 entry point to the second param
 * @param para3 entry point to the third param
 * @param sched_prior   thread sched_prior
 * @param options    thread options: option_essential_option, option_fp_option
 *
 * @return N/A
 */

void thread_entry_wrapper(thread_entry_t k, void *p1, void *p2, void *p3);

void arch_new_thread(struct ktcb *thread, struct thread_stack *stack,
	size_t stack_size, thread_entry_t entry_ptr, void *para1, void *para2, 
	void *para3, word_t options)
{
	char *stack_start = THREAD_STACK_BUFFER(stack);
	char *stackEnd;
	struct esf *init_ctx;

	stackEnd = stack_start + stack_size;
	thread_init(thread, stack_start, stack_size, options);
	init_ctx = (struct esf *)(STACK_ROUND_DOWN(stackEnd - sizeof(struct basic_esf)));
	init_ctx->basic.regs[0] = (u64_t)entry_ptr;
	init_ctx->basic.regs[1] = (u64_t)para1;
	init_ctx->basic.regs[2] = (u64_t)para2;
	init_ctx->basic.regs[3] = (u64_t)para3;

	/*
	 * We are saving:
	 *
	 * - SP: to pop out entry_ptr and parameters when going through
	 *   thread_entry_wrapper().
	 * - x30: to be used by ret in arm64_pendsv() when the new task is
	 *   first scheduled.
	 * - ELR_EL1: to be used by eret in thread_entry_wrapper() to return
	 *   to thread_entry_point() with entry_ptr in x0 and the parameters already
	 *   in place in x1, x2, x3.
	 * - SPSR_EL1: to enable IRQs (we are masking debug exceptions, SError
	 *   interrupts and FIQs).
	 */

	thread->callee_saved.sp = (u64_t)init_ctx;
	thread->callee_saved.x30 = (u64_t)thread_entry_wrapper;
	thread->callee_saved.elr = (u64_t)thread_entry_point;
	thread->callee_saved.spsr = SPSR_MODE_EL1H | DAIF_FIQ;
}
