#ifndef SYSCALL_H_
#define SYSCALL_H_

#ifndef _ASMLANGUAGE
#include <types_def.h>
#include <arch/syscall.h>
#include <sys/stdbool.h>
#include <object/cnode.h>
#include <state/statedata.h>
#include <sys/math_extras.h>
#include <sys/assert.h>
#include <kernel_object.h>
#include <kernel/thread.h>
#ifdef __cplusplus
extern "C" {
#endif

#if(0)
enum invocation_label {
	invaild_invocation,
	memory_control,
	space_unmap,
	space_control,
	processor_control,
	get_kernel_interface,
	exchange_registers,
	thread_control,
	thread_switch,
	schedule_control,
	get_system_clock,
	exchange_ipc,
	n_invocations
};

enum invocation_label {
    InvalidInvocation,
    UntypedRetype,
    TCBReadRegisters,
    TCBWriteRegisters,
    TCBCopyRegisters,
#if !defined(CONFIG_ENABLE_KERNEL_MCS)
    TCBConfigure,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    TCBConfigure,
#endif
    TCBSetPriority,
    TCBSetMCPriority,
#if !defined(CONFIG_ENABLE_KERNEL_MCS)
    TCBSetSchedParams,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    TCBSetSchedParams,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    TCBSetTimeoutEndpoint,
#endif
    TCBSetIPCBuffer,
#if !defined(CONFIG_ENABLE_KERNEL_MCS)
    TCBSetSpace,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    TCBSetSpace,
#endif
    TCBSuspend,
    TCBResume,
    TCBBindNotification,
    TCBUnbindNotification,
#if (!defined CONFIG_ENABLE_KERNEL_MCS) && CONFIG_MAX_NUM_NODES > 1
    TCBSetAffinity,
#endif
#if defined(CONFIG_HARDWARE_DEBUG_API)
    TCBSetBreakpoint,
#endif
#if defined(CONFIG_HARDWARE_DEBUG_API)
    TCBGetBreakpoint,
#endif
#if defined(CONFIG_HARDWARE_DEBUG_API)
    TCBUnsetBreakpoint,
#endif
#if defined(CONFIG_HARDWARE_DEBUG_API)
    TCBConfigureSingleStepping,
#endif
    TCBSetTLSBase,
    CNodeRevoke,
    CNodeDelete,
    CNodeCancelBadgedSends,
    CNodeCopy,
    CNodeMint,
    CNodeMove,
    CNodeMutate,
    CNodeRotate,
#if !defined(CONFIG_ENABLE_KERNEL_MCS)
    CNodeSaveCaller,
#endif
    IRQIssueIRQHandler,
    IRQAckIRQ,
    IRQSetIRQHandler,
    IRQClearIRQHandler,
    DomainSetSet,
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    SchedControlConfigure,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    SchedContextBind,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    SchedContextUnbind,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    SchedContextUnbindObject,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    SchedContextConsumed,
#endif
#if defined(CONFIG_ENABLE_KERNEL_MCS)
    SchedContextYieldTo,
#endif
    nInvocationLabels
};
#endif

extern s32_t arm_core_buffer_validate(void *addr, size_t size, s32_t write);

/**
 * @typedef syscall_handler_t
 * @brief System call handler function type
 *
 * These are kernel-side skeleton functions for system calls. They are
 * necessary to sanitize the arguments passed into the system call:
 *
 * - Any kernel object or device pointers are validated with _SYSCALL_IS_OBJ()
 * - Any memory buffers passed in are checked to ensure that the calling thread
 *   actually has access to them
 * - Many kernel calls do no sanity checking of parameters other than
 *   assertions. The handler must check all of these conditions using
 *   _SYSCALL_ASSERT()
 * - If the system call has more than 6 arguments, then arg6 will be a pointer
 *   to some struct containing arguments 6+. The struct itself needs to be
 *   validated like any other buffer passed in from userspace, and its members
 *   individually validated (if necessary) and then passed to the real
 *   implementation like normal arguments
 *
 * Even if the system call implementation has no return value, these always
 * return something, even 0, to prevent register leakage to userspace.
 *
 * Once everything has been validated, the real implementation will be executed.
 *
 * @param arg1 system call argument 1
 * @param arg2 system call argument 2
 * @param arg3 system call argument 3
 * @param arg4 system call argument 4
 * @param arg5 system call argument 5
 * @param arg6 system call argument 6
 * @param ssf System call stack page_f pointer. Used to generate kernel oops
 *            via _arch_syscall_oops_at(). Contents are arch-specific.
 * @return system call return value, or 0 if the system call implementation
 *         return void
 *
 */
typedef uintptr_t (*syscall_handler_t)(uintptr_t arg1, uintptr_t arg2,
					  uintptr_t arg3, uintptr_t arg4,
					  uintptr_t arg5, uintptr_t arg6,
					  void *ssf);
/* extern const syscall_handler_t k_syscall_table[SYSCALL_LIMIT]; *、

/* True if a syscall function must trap to the kernel, usually a
 * compile-time decision.
 */
static FORCE_INLINE bool_t syscall_trap(void)
{
	bool_t is_user = false;

#ifdef CONFIG_USERSPACE
#if defined(__SUPERVISOR__)
	is_user = false;
#elif defined(__USER__)
	is_user = true;
#else
	is_user = arch_is_user_context();
#endif

#endif
	return is_user;
}

/**
 * Indicate whether the CPU is currently in user mode
 *
 * @return true if the CPU is currently running with user permissions
 */
static FORCE_INLINE bool_t is_user_context(void)
{
#ifdef CONFIG_USERSPACE
	return arch_is_user_context();
#else
	return false;
#endif
}

/*
 * System Call Declaration macros
 *
 * These macros are used in public header files to declare system calls.
 * They generate FORCE_INLINE functions which have different implementations
 * depending on the _current_thread compilation context:
 *
 * - Kernel-only code, or CONFIG_USERSPACE disabled, these inlines will
 *   directly call the implementation
 * - User-only code, these inlines will marshal parameters and elevate
 *   privileges
 * - Mixed or indeterminate code, these inlines will do a runtime check
 *   to determine what course of action is needed.
 *
 * All system calls require a verifier function and an implementation
 * function.  These must follow a naming convention. For a system call
 * named k_foo():
 *
 * - The handler function will be named vrfy_k_foo(). Handler
 *   functions have the same type signature as the wrapped call,
 *   verify arguments passed up from userspace, and call the
 *   implementation function. See documentation for that typedef for
 *   more information.  - The implementation function will be named
 *   impl_k_foo(). This is the actual implementation of the system
 *   call.
 */

#define SYSCALL_OOPS(expr) \
	do { \
		if (expr) { \
			arch_syscall_oops(_current_cpu->syscall_frame_point); \
		} \
	} while (false)

/**
 * @brief Runtime expression check for system call arguments
 *
 * Used in handler functions to perform various runtime checks on arguments,
 * and generate a kernel oops if anything is not expected, printing a custom
 * message.
 *
 * @param expr Boolean expression to verify, a false result will trigger an
 *             oops
 * @param fmt Printf-style format string (followed by appropriate variadic
 *            arguments) to print on verification failure
 * @return False on success, True on failure
 */
#define SYSCALL_VERIFY_MSG(expr, fmt, ...) \
	({ \
		bool expr_copy = !(expr); \
		if (expr_copy) { \
			user_error("syscall %s failed check: " fmt, __func__, ##__VA_ARGS__); \
		} \
		expr_copy; \
	})

/**
 * @brief Runtime expression check for system call arguments
 *
 * Used in handler functions to perform various runtime checks on arguments,
 * and generate a kernel oops if anything is not expected.
 *
 * @param expr Boolean expression to verify, a false result will trigger an
 *             oops. A stringified version of this expression will be printed.
 * @return 0 on success, nonzero on failure
 */
#define SYSCALL_VERIFY(expr) SYSCALL_VERIFY_MSG(expr, #expr)

/**
 * @brief Runtime check that a user thread has read and/or write permission to
 *        a memory area
 *
 * Checks that the particular memory area is readable and/or writeable by the
 * currently running thread if the CPU was in user mode, and generates a kernel
 * oops if it wasn't. Prevents userspace from getting the kernel to read and/or
 * modify memory the thread does not have access to, or passing in garbage
 * pointers that would crash/pagefault the kernel if dereferenced.
 *
 * @param ptr Memory area to examine
 * @param size Size of the memory area
 * @param write If the thread should be able to write to this memory, not just
 *		read it
 * @return 0 on success, nonzero on failure
 */
#define SYSCALL_MEMORY(ptr, size, write) \
	SYSCALL_VERIFY_MSG( \
		arm_core_buffer_validate((void *)ptr, size, write) == 0, \
		"Memory region %p (size %zu) %s access denied", \
		(void *)(ptr), (size_t)(size), \
		write ? "write" : "read")

/**
 * @brief Runtime check that a user thread has read permission to a memory area
 *
 * Checks that the particular memory area is readable by the currently running
 * thread if the CPU was in user mode, and generates a kernel oops if it
 * wasn't. Prevents userspace from getting the kernel to read memory the thread
 * does not have access to, or passing in garbage pointers that would
 * crash/pagefault the kernel if dereferenced.
 *
 * @param ptr Memory area to examine
 * @param size Size of the memory area
 * @return 0 on success, nonzero on failure
 */
#define SYSCALL_MEMORY_READ(ptr, size) \
	SYSCALL_MEMORY(ptr, size, 0)

/**
 * @brief Runtime check that a user thread has write permission to a memory area
 *
 * Checks that the particular memory area is readable and writable by the
 * currently running thread if the CPU was in user mode, and generates a kernel
 * oops if it wasn't. Prevents userspace from getting the kernel to read or
 * modify memory the thread does not have access to, or passing in garbage
 * pointers that would crash/pagefault the kernel if dereferenced.
 *
 * @param ptr Memory area to examine
 * @param size Size of the memory area
 * @param 0 on success, nonzero on failure
 */
#define SYSCALL_MEMORY_WRITE(ptr, size) \
	SYSCALL_MEMORY(ptr, size, 1)

#define SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, write) \
	({ \
		size_t product; \
		SYSCALL_VERIFY_MSG( \
			!size_mul_overflow((size_t)(nmemb), \
				(size_t)(size), \
				&product), \
			"%zux%zu array is too large", \
			(size_t)(nmemb), \
			(size_t)(size)) ||  \
		SYSCALL_MEMORY(ptr, product, write); \
	})

/**
 * @brief Validate user thread has read permission for sized array
 *
 * Used when the memory region is expressed in terms of number of elements and
 * each element size, handles any overflow issues with computing the total
 * array bounds. Otherwise see _SYSCALL_MEMORY_READ.
 *
 * @param ptr Memory area to examine
 * @param nmemb Number of elements in the array
 * @param size Size of each array element
 * @return 0 on success, nonzero on failure
 */
#define SYSCALL_MEMORY_ARRAY_READ(ptr, nmemb, size) \
	SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, 0)

/**
 * @brief Validate user thread has read/write permission for sized array
 *
 * Used when the memory region is expressed in terms of number of elements and
 * each element size, handles any overflow issues with computing the total
 * array bounds. Otherwise see _SYSCALL_MEMORY_WRITE.
 *
 * @param ptr Memory area to examine
 * @param nmemb Number of elements in the array
 * @param size Size of each array element
 * @return 0 on success, nonzero on failure
 */
#define SYSCALL_MEMORY_ARRAY_WRITE(ptr, nmemb, size) \
	SYSCALL_MEMORY_ARRAY(ptr, nmemb, size, 1)


#define SYSCALL_OBJ(ptr, type) \
	SYSCALL_VERIFY_MSG( \
		obj_validation_check( \
			k_object_find((void *)ptr), \
			(void *)ptr, \
			type) == 0, \
		"access denied")

/**
 * @brief Runtime check driver object pointer for presence of operation
 *
 * Validates if the driver object is capable of performing a certain operation.
 *
 * @param ptr Untrusted device instance object pointer
 * @param api_struct Name of the driver API struct (e.g. gpio_driver_api)
 * @param op Driver operation (e.g. manage_callback)
 * @return 0 on success, nonzero on failure
 */
#define SYSCALL_DRIVER_API_OP(dev_obj_ptr, dev_api, dev_api_op) \
	({ \
		struct dev_api *dev = (struct dev_api *) \
			((struct device *)dev_obj_ptr)->driver_api; \
		SYSCALL_VERIFY_MSG( \
			dev->dev_api_op != NULL, \
			"Operation %s not defined for driver " \
			"instance %p", \
			#dev_api_op, \
			dev); \
	})

/**
 * @brief Runtime check that device object is of a specific driver type
 *
 * Checks that the driver object passed in is initialized, the caller has
 * correct permissions, and that it belongs to the specified driver
 * subsystems. Additionally, all devices store a function pointer to the
 * driver's init function. If this doesn't match the value provided, the
 * check will fail.
 *
 * This provides an easy way to determine if a device object not only
 * belongs to a particular subsystem, but is of a specific device driver
 * implementation. Useful for defining out-of-subsystem system calls
 * which are implemented for only one driver.
 *
 * @param _device Untrusted device pointer
 * @param _dtype Expected kernel object type for the provided device pointer
 * @param _init_fn Expected init function memory address
 * @return 0 on success, nonzero on failure
 */
#define SYSCALL_DRIVER_INIT(dev_obj_ptr, dev_obj_type, dev_init_func) \
	({ \
		struct device *dev = (struct device *)dev_obj_ptr; \
		SYSCALL_OBJ(dev, dev_obj_type) || \
		SYSCALL_VERIFY_MSG( \
			dev->config->init == dev_init_func, \
			"init function mismatch"); \
	})

#ifdef __cplusplus
}
#endif


#endif
#endif
