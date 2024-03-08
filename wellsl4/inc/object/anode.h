#ifndef ASPACE_H_
#define ASPACE_H_


#include <types_def.h>
#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu_anode.h>
#include <sys/dlist.h>
#include <kernel_object.h>
#include <api/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory Domain
 *
 */
struct thread_page {
#if defined(CONFIG_USERSPACE)
	/** partitions in the domain */
	struct partition partitions[CONFIG_MAX_DOMAIN_PARTITIONS];
#endif	/* CONFIG_USERSPACE */
	/** domain q */
	sys_dlist_t pagetable_list;
	/** number of partitions in the domain */
	byte_t partitions_number;
};

typedef struct thread_page thread_page_t;

/**
 * @def PARTITION_DEFINE
 * @brief Used to declare a memory partition
 * @req K-MP-001
 */
#if defined(ARCH_PARTITION_ALIGN_CHECK) 
#define PARTITION_DEFINE(name, start, size, attr) \
	ARCH_PARTITION_ALIGN_CHECK(start, size); \
	struct partition name = { (uintptr_t)start, size, attr}
#else
#define PARTITION_DEFINE(name, start, size, attr) \
	struct partition name = { (uintptr_t)start, size, attr}
#endif /* ARCH_PARTITION_ALIGN_CHECK */

/**
 * @brief Initialize a memory domain.
 *
 * Initialize a memory domain with given name and memory partitions.
 *
 * See documentation for add_to_page() for details about
 * partition constraints.
 *
 * @param domain The memory domain to be initialized.
 * @param num_parts The number of array items of "parts" parameter.
 * @param parts An array of pointers to the memory partitions. Can be NULL
 *              if num_parts is zero.
 * @req K-MD-001
 */
void reset_page(struct thread_page *domain, byte_t num_parts, struct partition *parts[]);
/**
 * @brief Destroy a memory domain.
 *
 * Destroy a memory domain.
 *
 * @param domain The memory domain to be destroyed.
 * @req K-MD-001
 */
void remove_from_page_table_of(struct thread_page *domain);

/**
 * @brief Add a memory partition into a memory domain.
 *
 * Add a memory partition into a memory domain. Partitions must conform to
 * the following constraints:
 *
 * - Partition bounds must be within system RAM boundaries on MMU-based
 *   systems.
 * - Partitions in the same memory domain may not overlap each other.
 * - Partitions must not be defined which expose private kernel
 *   data structures or kernel k_objects.
 * - The starting address alignment, and the partition size must conform to
 *   the constraints of the underlying memory management hardware, which
 *   varies per architecture.
 * - Memory domain partitions are only intended to control access to memory
 *   from user mode record_threads.
 *
 * Violating these constraints may lead to CPU exceptions or undefined
 * behavior.
 *
 * @param domain The memory domain to be added a memory partition.
 * @param part The memory partition to be added
 * @req K-MD-001
 */
void add_to_page(struct thread_page *domain, struct partition *part);

/**
 * @brief Remove a memory partition from a memory domain.
 *
 * Remove a memory partition from a memory domain.
 *
 * @param domain The memory domain to be removed a memory partition.
 * @param part The memory partition to be removed
 * @req K-MD-001
 */
void remove_from_page(struct thread_page *domain, struct partition *part);

/**
 * @brief Add a thread into a memory domain.
 *
 * Add a thread into a memory domain.
 *
 * @param domain The memory domain that the thread is going to be added into.
 * @param thread ID of thread going to be added into the memory domain.
 *
 * @req K-MD-001
 */
void add_to_page_table(struct thread_page *domain, struct ktcb *thread);

/**
 * @brief Remove a thread from its memory domain.
 *
 * Remove a thread from its memory domain.
 *
 * @param thread ID of thread going to be removed from its memory domain.
 * @req K-MD-001
 */
void remove_from_page_table(struct ktcb *thread);


exception_t do_guard_page(struct ktcb *s_thread, 
	struct ktcb *d_thread, 
	word_t addr,
	word_t len,
	word_t right);

exception_t do_map_page(struct ktcb *thread,
	word_t addr,
	word_t len,
	word_t right);

exception_t do_map_string(struct ktcb *thread,
	word_t len,
	word_t ptr);

/*
__syscall exception_t unmap_page(word_t control)
{
	return EXCEPTION_NONE;
}
*/


#ifdef __cplusplus
}
#endif


#endif
