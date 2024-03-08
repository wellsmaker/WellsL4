/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Internal kernel APIs implemented at the architecture layer.
 *
 * Not all architecture-specific defines are here, APIs that are used
 * by public functions and macros are defined in include/sys/arch_interface.h.
 *
 * For all FORCE_INLINE functions prototyped here, the implementation is expected
 * to be provided by arch/ARCH/include/kernel_arch_func.h
 */
#ifndef ARCH_THREAD_H_
#define ARCH_THREAD_H_

#include <types_def.h>
#include <arch/cpu.h>
#include <arch/irq.h>
#include <kernel_object.h>
#include <state/statedata.h>

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* NOTE: We cannot pull in kernel.h here, need some forward declarations  */
struct ktcb;
struct thread_page;
struct thread_stack;

typedef void (*thread_entry_t)(void *p1, void *p2, void *p3);

/**
 * @defgroup arch-timing Architecture timing APIs
 * @ingroup arch-interface
 * @{
 */

/**
 * Obtain the _current_thread cycle count, in units that are hardware-specific
 *
 * @see get_cycle_32()
 */
static FORCE_INLINE u32_t arch_k_cycle_get_32(void);


/**
 * @brief Power save idle routine
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of sys_power_save_idle in the kernel when the
 * '_sys_power_save_flag' variable is non-zero.
 *
 * Architectures that do not implement power management instructions may
 * immediately return, otherwise a power-saving instruction should be
 * issued to wait for an interrupt.
 *
 * @see cpu_idle()
 */
void arch_cpu_idle(void);

/**
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * The requirements for arch_cpu_atomic_idle() are as follows:
 *
 * -# Enabling interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that
 *    occurs if this requirement is not met.
 *
 * -# After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'key' input parameter.
 *
 * @see cpu_atomic_idle()
 *
 * @param key Lockout key returned by previous invocation of arch_irq_lock()
 */
void arch_cpu_atomic_idle(u32_t key);


/**
 * @addtogroup arch-smp
 * @{
 */

/**
 * Per-cpu entry function
 *
 * @param context parameter, implementation specific
 */
typedef FUNC_NORETURN void (*arch_cpu_start_t)(void *data);

/**
 * @brief Start a numbered CPU on a MP-capable system
 *
 * This starts and initializes a specific CPU.  The main thread on startup is
 * running on CPU zero, other processors are numbered sequentially.  On return
 * from this function, the CPU is known to have begun operating and will enter
 * the provided function.  Its interrupts will be initialized but disabled such
 * that irq_unlock() with the provided key will work to enable them.
 *
 * Normally, in SMP mode this function will be called by the kernel
 * initialization and should not be used as a user API.  But it is defined here
 * for special-purpose apps which want WellL4 running on one core and to use
 * others for design-specific processing.
 *
 * @param cpu_num Integer number of the CPU
 * @param stack Stack memory for the CPU
 * @param sz Stack buffer size, in bytes
 * @param fn Function to begin running on the CPU.
 * @param arg Untyped argument to be passed to "fn"
 */
void arch_start_cpu(s32_t cpu_num, struct thread_stack *stack, s32_t sz,
	arch_cpu_start_t fn, void *arg);


/**
 * @addtogroup arch-irq
 * @{
 */

/**
 * Lock interrupts on the _current_thread CPU
 *
 * @see irq_lock()
 */
static FORCE_INLINE u32_t arch_irq_lock(void);

/**
 * Unlock interrupts on the _current_thread CPU
 *
 * @see irq_unlock()
 */
static FORCE_INLINE void arch_irq_unlock(u32_t key);

/**
 * Test if calling arch_irq_unlock() with this key would unlock irqs
 *
 * @param key value returned by arch_irq_lock()
 * @return true if interrupts were unlocked prior to the arch_irq_lock()
 * call that produced the key argument.
 */
static FORCE_INLINE bool arch_irq_unlocked(u32_t key);

/**
 * Disable the specified interrupt line
 *
 * @see irq_disable()
 */
void arch_irq_disable(u32_t irq);

/**
 * Enable the specified interrupt line
 *
 * @see irq_enable()
 */
void arch_irq_enable(u32_t irq);

/**
 * Test if an interrupt line is enabled
 *
 * @see irq_is_enabled()
 */
s32_t arch_irq_is_enabled(u32_t irq);

/**
 * Arch-specific hook to install a dynamic interrupt.
 *
 * @param irq IRQ line number
 * @param sched_prior Interrupt sched_prior
 * @param routine Interrupt service routine
 * @param parameter ISR parameter
 * @param flag Arch-specific IRQ configuration flag
 *
 * @return The vector assigned to this interrupt
 */
s32_t arch_irq_connect_dynamic(u32_t irq, u32_t sched_prior,
	void (*routine)(void *parameter),
	void *parameter, u32_t flag);


#ifdef CONFIG_IRQ_OFFLOAD
/**
 * Run a function in interrupt context.
 *
 * Implementations should invoke an exception such that the kernel goes through
 * its interrupt handling dispatch path, to include switching to the interrupt
 * stack, and runs the provided routine and parameter.
 *
 * The only intended use-case for this function is for test code to simulate
 * the correctness of kernel APIs in interrupt handling context. This API
 * is not intended for real applications.
 *
 * @see irq_offload()
 *
 * @param routine Function to run in interrupt context
 * @param parameter Value to pass to the function when invoked
 */
void arch_irq_offload(irq_offload_routine_t routine, void *parameter);
#endif

/**
 * @defgroup arch-smp Architecture-specific SMP APIs
 * @ingroup arch-interface
 * @{
 */
#ifdef CONFIG_SMP
/** Return the CPU struct for the currently executing CPU */
static FORCE_INLINE struct cpu *arch_curr_cpu(void);

/**
 * Broadcast an interrupt to all CPUs
 *
 * This will invoke sched_ipi() on other CPUs in the system.
 */
void arch_sched_ipi(void);
#endif


/**
 * @defgroup arch-userspace Architecture-specific userspace APIs
 * @ingroup arch-interface
 * @{
 */

#ifdef CONFIG_USERSPACE
/**
 * Invoke a system call with 0 arguments.
 *
 * No general-purpose register state other than return value may be preserved
 * when transitioning from supervisor mode back down to user mode for
 * security reasons.
 *
 * It is required that all arguments be stored in registers when elevating
 * privileges from user to supervisor mode.
 *
 * Processing of the syscall takes place on a separate kernel stack. Interrupts
 * should be enabled when invoking the system call marshallers from the
 * dispatch table. Thread preemption may occur when handling system calls.
 *
 * Call ids are untrusted and must be bounds-checked, as the value is used to
 * index the system call dispatch table, containing function pointers to the
 * specific system call code.
 *
 * @param call_id System call ID
 * @return Return value of the system call. Void system calls return 0 here.
 */
static FORCE_INLINE uintptr_t arch_syscall_invoke0(uintptr_t call_id);

/**
 * Invoke a system call with 1 argument.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static FORCE_INLINE uintptr_t arch_syscall_invoke1(uintptr_t arg1, uintptr_t call_id);

/**
 * Invoke a system call with 2 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static FORCE_INLINE uintptr_t arch_syscall_invoke2(uintptr_t arg1, uintptr_t arg2,
	uintptr_t call_id);

/**
 * Invoke a system call with 3 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static FORCE_INLINE uintptr_t arch_syscall_invoke3(uintptr_t arg1, uintptr_t arg2,
	uintptr_t arg3, uintptr_t call_id);

/**
 * Invoke a system call with 4 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param arg4 Fourth argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static FORCE_INLINE uintptr_t arch_syscall_invoke4(uintptr_t arg1, uintptr_t arg2,
	uintptr_t arg3, uintptr_t arg4, uintptr_t call_id);

/**
 * Invoke a system call with 5 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param arg4 Fourth argument to the system call.
 * @param arg5 Fifth argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static FORCE_INLINE uintptr_t arch_syscall_invoke5(uintptr_t arg1, uintptr_t arg2,
	uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t call_id);

/**
 * Invoke a system call with 6 arguments.
 *
 * @see arch_syscall_invoke0()
 *
 * @param arg1 First argument to the system call.
 * @param arg2 Second argument to the system call.
 * @param arg3 Third argument to the system call.
 * @param arg4 Fourth argument to the system call.
 * @param arg5 Fifth argument to the system call.
 * @param arg6 Sixth argument to the system call.
 * @param call_id System call ID, will be bounds-checked and used to reference
 *	          kernel-side dispatch table
 * @return Return value of the system call. Void system calls return 0 here.
 */
static FORCE_INLINE uintptr_t arch_syscall_invoke6(uintptr_t arg1, uintptr_t arg2,
	uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t arg6,
 	uintptr_t call_id);

/**
 * Indicate whether we are currently running in user mode
 *
 * @return true if the CPU is currently running with user permissions
 */
/* static FORCE_INLINE bool arch_is_user_context(void); */

/**
 * @brief Get the maximum number of partitions for a memory domain
 *
 * @return Max number of partitions, or -1 if there is no limit
 */
s32_t arm_core_page_max_partitions_get(void);

/**
 * @brief Add a thread to a memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when the provided thread has been added to a memory domain.
 *
 * The thread's memory domain pointer will be set to the domain to be added
 * to.
 *
 * @param thread Thread which needs to be configured.
 */
void arm_core_page_table_add(struct ktcb *thread);

/**
 * @brief Remove a thread from a memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when the provided thread has been removed from a memory domain.
 *
 * The thread's memory domain pointer will be the domain that the thread
 * is being removed from.
 *
 * @param thread Thread being removed from its memory domain
 */
void arm_core_page_table_remove(struct ktcb *thread);

/**
 * @brief Remove a partition from the memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has had a partition removed.
 *
 * The partition index data, and the number of partitions configured, are not
 * respectively cleared and decremented in the domain until after this function
 * runs.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition index that needs to be deleted
 */
void arm_core_page_partition_remove(struct thread_page *domain, u32_t partition_id);

/**
 * @brief Add a partition to the memory domain
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has a partition added.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition that needs to be added
 */
void arm_core_page_partition_add(struct thread_page *domain, u32_t partition_id);

/**
 * @brief Remove the memory domain
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has been destroyed.
 *
 * Thread assignments to the memory domain are only cleared after this function
 * runs.
 *
 * @param domain The memory domain structure which needs to be deleted.
 */
void arm_core_page_destroy(struct thread_page *domain);

/**
 * @brief Check memory region permissions
 *
 * Given a memory region, return whether the _current_thread memory management hardware
 * configuration would allow a user thread to read/write that region. Used by
 * system calls to validate buffers coming in from userspace.
 *
 * Notes:
 * The function is guaranteed to never return validation success, if the entire
 * buffer area is not user accessible.
 *
 * The function is guaranteed to correctly validate the permissions of the
 * supplied buffer, if the user access permissions of the entire buffer are
 * enforced by a single, enabled memory management region.
 *
 * In some architectures the validation will always return failure
 * if the supplied memory buffer spans multiple enabled memory management
 * regions (even if all such regions permit user access).
 *
 * @warning 0 size buffer has undefined behavior.
 *
 * @param addr start address of the buffer
 * @param size the size of the buffer
 * @param write If nonzero, additionally check if the area is writable.
 *	  Otherwise, just check if the memory can be read.
 *
 * @return nonzero if the permissions don't match.
 */
s32_t arm_core_buffer_validate(void *addr, size_t size, s32_t write);

/**
 * Perform a one-way transition from supervisor to kernel mode.
 *
 * Implementations of this function must do the following:
 *
 * - Reset the thread's stack pointer to a suitable initial value. We do not
 *   need any prior context since this is a one-way operation.
 * - Set up any kernel stack region for the CPU to use during privilege
 *   elevation
 * - Put the CPU in whatever its equivalent of user mode is
 * - Transfer execution to arch_new_thread() passing along all the supplied
 *   arguments, in user mode.
 *
 * @param user_entry Entry point to start executing as a user thread
 * @param p1 1st parameter to user thread
 * @param p2 2nd parameter to user thread
 * @param p3 3rd parameter to user thread
 */
FUNC_NORETURN void arch_user_mode_enter(thread_entry_t user_entry,
	void *p1, void *p2, void *p3);

/**
 * @brief Induce a kernel oops that appears to come from a specific location
 *
 * Normally, k_oops() generates an exception that appears to come from the
 * call site of the k_oops() itself.
 *
 * However, when validating arguments to a system call, if there are problems
 * we want the oops to appear to come from where the system call was invoked
 * and not inside the validation function.
 *
 * @param ssf System call stack page_f pointer. This gets passed as an argument
 *            to syscall_handler_t functions and its contents are completely
 *            architecture specific.
 */
FUNC_NORETURN void arch_syscall_oops(void *ssf);

/**
 * @brief Safely take the length of a potentially bad string
 *
 * This must not fault, instead the err parameter must have -1 written to it.
 * This function otherwise should work exactly like libc strnlen(). On success
 * *err should be set to 0.
 *
 * @param s String to measure
 * @param maxsize Max length of the string
 * @param err Error value to write
 * @return Length of the string, not counting NULL byte, up to maxsize
 */
size_t arch_user_string_nlen(const char *s, size_t maxsize, s32_t *err);
#endif


/**
 * @defgroup arch-timing Architecture timing APIs
 * @{
 */
#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
/**
 * Architecture-specific implementation of busy-waiting
 *
 * @param usec_to_wait Wait period, in microseconds
 */
void arch_busy_wait(u32_t usec_to_wait);
#endif


/**
 * @defgroup arch-record_threads Architecture thread APIs
 * @ingroup arch-interface
 * @{
 */

/** Handle arch-specific logic for setting up new record_threads
 *
 * The stack and arch-specific thread state variables must be set up
 * such that a later attempt to switch to this thread will succeed
 * and we will enter thread_entry_point with the requested thread and
 * arguments as its parameters.
 *
 * At some point in this function's implementation, set_new_thread() must
 * be called with the true bounds of the available stack buffer within the
 * thread's stack object.
 *
 * @param thread Pointer to uninitialized struct ktcb
 * @param pStack Pointer to the stack space.
 * @param stackSize Stack size in bytes.
 * @param entry Thread entry function.
 * @param p1 1st entry point parameter.
 * @param p2 2nd entry point parameter.
 * @param p3 3rd entry point parameter.
 * @param prio Thread sched_prior.
 * @param options Thread options.
 */
void arch_new_thread(struct ktcb *thread, struct thread_stack *pStack,
	size_t stackSize, thread_entry_t entry, void *p1, void *p2, void *p3,
	u32_t options);

#ifdef CONFIG_USE_SWITCH
/**
 * Cooperatively context switch
 *
 * Architectures have considerable leeway on what the specific semantics of
 * the switch handles are, but optimal implementations should do the following
 * if possible:
 *
 * 1) Push all thread state relevant to the context switch to the _current_thread stack
 * 2) Update the switched_from parameter to contain the _current_thread stack pointer,
 *    after all context has been saved. switched_from is used as an output-
 *    only parameter and its _current_thread value is ignored (and can be NULL, see
 *    below).
 * 3) Set the stack pointer to the value provided in switch_to
 * 4) Pop off all thread state from the stack we switched to and return.
 *
 * Some arches may implement thread->switch handle as a pointer to the thread
 * itself, and save context somewhere in thread->arch. In this case, on initial
 * context switch from the dummy thread, thread->switch handle for the outgoing
 * thread is NULL. Instead of dereferencing switched_from all the way to get
 * the thread pointer, subtract ___thread_t_switch_handle_OFFSET to obtain the
 * thread pointer instead.
 *
 * @param switch_to Incoming thread's switch handle
 * @param switched_from Pointer to outgoing thread's switch handle storage
 *        location, which may be updated.
 */
static FORCE_INLINE void arch_switch(void *switch_to, void **switched_from);
#else
/**
 * Cooperatively context switch
 *
 * Must be called with interrupts locked with the provided key.
 * This is the older-style context switching method, which is incompatible
 * with SMP. New arch ports, either SMP or UP, are encouraged to implement
 * arch_switch() instead.
 *
 * @param key Interrupt locking key
 * @return If woken from blocking on some kernel object, the result of that
 *         blocking operation.
 */
s32_t arch_swap(u32_t key);

/**
 * Set the return value for the specified thread.
 *
 * It is assumed that the specified @a thread is pending.
 *
 * @param thread Pointer to thread object
 * @param value value to set as return value
 */
//static FORCE_INLINE void arch_thread_swap_retval_set(struct ktcb *thread, u32_t value);
#endif

#ifdef CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN
/**
 * Custom logic for entering main thread context at early boot
 *
 * Used by architectures where the typical trick of setting up a dummy thread
 * in early boot context to "switch out" of isn't workable.
 *
 * @param _main_thread main thread object
 * @param _main_stack main thread's stack object
 * @param _main_stack_size Size of the stack object's buffer
 * @param _main Entry point for application main function.
 */
void arch_switch_to_main_thread(struct ktcb *_main_thread,
	struct thread_stack *_main_stack,
	size_t _main_stack_size,
	thread_entry_t _main);
#endif

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
/**
 * @brief Disable floating point context preservation
 *
 * The function is used to disable the preservation of floating
 * point context information for a particular thread.
 *
 * @note For ARM architecture, disabling floating point preservation may only
 * be requested for the _current_thread thread and cannot be requested in ISRs.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the floating point disabling could not be performed.
 */
s32_t arch_float_disable(struct ktcb *thread);
#endif

/**
 * @defgroup arch-pm Architecture-specific power management APIs
 * @ingroup arch-interface
 * @{
 */
/** Halt the system, optionally propagating a reason code */
FUNC_NORETURN void arch_system_halt(u32_t reason);

static FORCE_INLINE struct cpu *arch_curr_cpu(void)
{
	u32_t index = 0;
#if defined(CONFIG_SMP) 
	/* get register value */
	/* get core number */
	index = arch_get_curr_cpu_index();
	return &_kernel.cpus[index];
#else
	return &_kernel.cpus[index];
#endif
}

#ifdef __cplusplus
}
#endif

#endif
#endif
