#ifndef KERNEL_STACH_
#define KERNEL_STACH_

#include <types_def.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <kernel_object.h>
#include <linker/section_tags.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Obtain an extern reference to a stack
 *
 * This macro properly brings the symbol of a thread stack declared
 * elsewhere into scope.
 *
 * @param sym Thread stack symbol name
 * @req K-MISC-005
 */
#define THREAD_STACK_EXTERN(sym) extern struct thread_stack sym[]

#ifdef ARCH_THREAD_STACK_DEFINE
#define THREAD_STACK_DEFINE(sym, size) ARCH_THREAD_STACK_DEFINE(sym, size)
#define THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
		ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size)
#define THREAD_STACK_LEN(size) ARCH_THREAD_STACK_LEN(size)
#define THREAD_STACK_MEMBER(sym, size) ARCH_THREAD_STACK_MEMBER(sym, size)
#define THREAD_STACK_SIZEOF(sym) ARCH_THREAD_STACK_SIZEOF(sym)
#define THREAD_STACK_RESERVED ((size_t)ARCH_THREAD_STACK_RESERVED)
static FORCE_INLINE char *THREAD_STACK_BUFFER(struct thread_stack *sym)
{
	return ARCH_THREAD_STACK_BUFFER(sym);
}
#else
/**
 * @brief Declare a toplevel thread stack memory region
 *
 * This declares a region of memory suitable for use as a thread's stack.
 *
 * This is the generic, historical definition. Align to STACK_ALIGN and put in
 * 'noinit' section so that it isn't zeroed at boot
 *
 * The declared symbol will always be a struct thread_stack which can be passed to
 * ktcb_create(), but should otherwise not be manipulated. If the buffer
 * inside needs to be examined, examine thread->stack_info for the associated
 * thread object to obtain the boundaries.
 *
 * It is legal to precede this definition with the 'static' keyword.
 *
 * It is NOT legal to take the sizeof(sym) and pass that to the stackSize
 * parameter of ktcb_create(), it may not be the same as the
 * 'size' parameter. Use THREAD_STACK_SIZEOF() instead.
 *
 * Some arches may round the size of the usable stack region up to satisfy
 * alignment constraints. THREAD_STACK_SIZEOF() will return the aligned
 * size.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 * @req K-TSTACK-001
 */
#define THREAD_STACK_DEFINE(sym, size) \
	struct thread_stack __noinit __aligned(STACK_ALIGN) sym[size]

/**
 * @brief Calculate size of stacks to be allocated in a stack array
 *
 * This macro calculates the size to be allocated for the stacks
 * inside a stack array. It accepts the indicated "size" as a parameter
 * and if required, pads some extra bytes (e.g. for MPU scenarios). Refer
 * THREAD_STACK_ARRAY_DEFINE definition to see how this is used.
 *
 * @param size Size of the stack memory region
 * @req K-TSTACK-001
 */
#define THREAD_STACK_LEN(size) (size)

/**
 * @brief Declare a toplevel array of thread stack memory regions
 *
 * Create an array of equally sized stacks. See THREAD_STACK_DEFINE
 * definition for additional details and constraints.
 *
 * This is the generic, historical definition. Align to STACK_ALIGN and put in
 * 'noinit' section so that it isn't zeroed at boot
 *
 * @param sym Thread stack symbol name
 * @param nmemb Number of stacks to declare
 * @param size Size of the stack memory region
 * @req K-TSTACK-001
 */
#define THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct thread_stack __noinit \
		__aligned(STACK_ALIGN) sym[nmemb][THREAD_STACK_LEN(size)]

/**
 * @brief Declare an embedded stack memory region
 *
 * Used for stacks embedded within other data structures. Use is highly
 * discouraged but in some cases necessary. For memory protection scenarios,
 * it is very important that any RAM preceding this member not be writable
 * by record_threads else a stack overflow will lead to silent corruption. In other
 * words, the containing data structure should live in RAM owned by the kernel.
 *
 * @param sym Thread stack symbol name
 * @param size Size of the stack memory region
 * @req K-TSTACK-001
 */
#define THREAD_STACK_MEMBER(sym, size) \
	struct thread_stack __aligned(STACK_ALIGN) sym[size]

/**
 * @brief Return the size in bytes of a stack memory region
 *
 * Convenience macro for passing the desired stack size to ktcb_create()
 * since the underlying implementation may actually create something larger
 * (for instance a right area).
 *
 * The value returned here is not guaranteed to match the 'size' parameter
 * passed to THREAD_STACK_DEFINE and may be larger.
 *
 * @param sym Stack memory symbol
 * @return Size of the stack
 * @req K-TSTACK-001
 */
#define THREAD_STACK_SIZEOF(sym) sizeof(sym)


/**
 * @brief Indicate how much additional memory is reserved for stack k_objects
 *
 * Any given stack declaration may have additional memory in it for right
 * areas or supervisor mode stacks. This macro indicates how much space
 * is reserved for this. The memory reserved may not be contiguous within
 * the stack object, and does not account for additional space used due to
 * enforce alignment.
 */
#define THREAD_STACK_RESERVED		((size_t)0U)

/**
 * @brief Get a pointer to the physical stack buffer
 *
 * This macro is deprecated. If a stack buffer needs to be examined, the
 * bounds should be obtained from the associated thread's stack_info struct.
 *
 * @param sym Declared stack symbol name
 * @return The buffer itself, a char *
 * @req K-TSTACK-001
 */
static FORCE_INLINE char *THREAD_STACK_BUFFER(struct thread_stack *sym)
{
	return (char *)sym;
}

#endif

#ifdef __cplusplus
}
#endif

#endif
