#include <types_def.h>
#include <sys/string.h>
#include <sys/math_extras.h>
#include <sys/stdbool.h>
#include <object/tcb.h>
#include <object/anode.h>
#include <sys/assert.h>
#include <sys/dlist.h>
#include <model/spinlock.h>
#include <mpu/arm_mpu.h>
#include <arch/thread.h>
#include <api/errno.h>
#include <api/syscall.h>
#include <kernel/thread.h>
#include <object/objecttype.h>

static spinlock_t space_lock;
#define LOCKED(lck) \
		for (spinlock_key_t __i = {},	\
		     __key = lock_spin_lock(lck);	\
		     !__i.key;					\
             unlock_spin_unlock(lck, __key),\
             __i.key = 1)

static u8_t max_partitions;

#if (defined(CONFIG_EXECUTE_XOR_WRITE) || \
	defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)) \
	&& __ASSERT_ON
static bool_t contrast_partition(const struct partition *part_target, 
	const struct partition *part_array, word_t part_n)
{
	bool_t is_executable = PARTITION_IS_EXECUTABLE(part_target->attr);
	bool_t is_writable  = PARTITION_IS_WRITABLE(part_target->attr);
	word_t last = part_target->start + part_target->size - 1;

	if (is_executable && is_writable)
	{
		user_error("partition is writable and executable <start %lx>",
			part_target->start);
		return false;
	}

	for (word_t i = 0U; i < part_n; i++) 
	{
		bool is_cur_writable, is_cur_executable;
		word_t cur_last;

		cur_last = part_array[i].start + part_array[i].size - 1;

		if (last < part_array[i].start || cur_last < part_target->start) 
		{
			continue;
		}
		
#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
		/* Partitions overlap */
		user_error(false, "overlapping partitions <%lx...%x>, <%lx...%x>",
			part_target->start, last, part_array[i].start, cur_last);
		return false;
#endif

		is_cur_writable = PARTITION_IS_WRITABLE(part_array[i].attr);
		is_cur_executable = PARTITION_IS_EXECUTABLE(part_array[i].attr);

		if ((is_cur_writable && is_executable) || (is_cur_executable && is_writable)) 
		{
			user_error("overlapping partitions are writable and executable "
				 "<%lx...%x>, <%lx...%x>", part_target->start, last,
				 part_array[i].start, cur_last);
			return false;
		}
	}

	return true;
}

static FORCE_INLINE bool_t contrast_partition_domain(const struct thread_page *page_item,
					 const struct partition *part_target)
{
	return contrast_partition(part_target, page_item->partitions, page_item->partitions_number);
}
#else
#define contrast_partition(...) (true)
#define contrast_partition_domain(...) (true)
#endif


void reset_page(struct thread_page *page_item, u8_t part_n, struct partition *part_array[])
{
	assert_info(page_item != NULL, "");
	assert_info(part_n == 0U || part_array != NULL, "");
	assert_info(part_n <= max_partitions, "");

	LOCKED(&space_lock)
	{
		page_item->partitions_number = 0U;
		(void)memset(page_item->partitions, 0, sizeof(page_item->partitions));

		if (part_n != 0U) 
		{
			for (word_t i = 0U; i < part_n; i++) 
			{
				assert_info(part_array[i] != NULL, "");
				assert_info((part_array[i]->start + part_array[i]->size) >
					 part_array[i]->start,
					 "invalid partition %p size %zu",
					 part_array[i], part_array[i]->size);
	
#if defined(CONFIG_EXECUTE_XOR_WRITE) || defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
				assert_info(contrast_partition_domain(page_item, part_array[i]), "");
#endif
				page_item->partitions[i] = *part_array[i];
				page_item->partitions_number++;
			}
		}
		sys_dlist_init(&page_item->pagetable_list);
	}
}


void add_to_page(struct thread_page *page_item, struct partition *part)
{
	assert_info(page_item != NULL, "");
	assert_info(part != NULL, "");
	assert_info((part->start + part->size) > part->start,
		 "invalid partition %p size %zu", part, part->size);

#if defined(CONFIG_EXECUTE_XOR_WRITE) || defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
	assert_info(contrast_partition_domain(page_item, part), "");
#endif

	LOCKED(&space_lock)
	{
		word_t index;
		for (index = 0; index < max_partitions; index++)
		{
			/* A zero-sized partition denotes it's a free partition */
			if (page_item->partitions[index].size == 0U) 
			{
				break;
			}
		}

		/* Assert if there is no free partition */
		assert_info(index < max_partitions, "no free partition found");

		page_item->partitions[index].start = part->start;
		page_item->partitions[index].size = part->size;
		page_item->partitions[index].attr = part->attr;
		page_item->partitions_number++;
		arm_core_page_partition_add(page_item, index);
	}
}

void remove_from_page(struct thread_page *page_item, struct partition *part)
{
	assert_info(page_item != NULL, "");
	assert_info(part != NULL, "");

	LOCKED(&space_lock)
	{
		word_t index;
		/* find a partition that matches the given start and size */
		for (index = 0; index < max_partitions; index++) 
		{
			if (page_item->partitions[index].start == part->start &&
				page_item->partitions[index].size == part->size)
			{
				break;
			}
		}
		
		/* Assert if not found */
		assert_info(index < max_partitions, "no matching partition found");
		
		arm_core_page_partition_remove(page_item, index);
		/* A zero-sized partition denotes it's a free partition */
		page_item->partitions[index].size = 0U;
		page_item->partitions_number--;
	}
}

void add_to_page_table(struct thread_page *page_item, struct ktcb *thread)
{
	assert_info(page_item != NULL, "");
	assert_info(thread != NULL, "");
	/* assert_info(thread->userspace_fpage_table.pagetable_item == NULL, "mem page_item unset"); */

	LOCKED(&space_lock)
	{
		sys_dlist_append(&page_item->pagetable_list, &thread->userspace_fpage_table.pagetable_node);
		thread->userspace_fpage_table.pagetable_item = page_item;
		arm_core_page_table_add(thread);
	}
}

void remove_from_page_table(struct ktcb *thread)
{
	assert_info(thread != NULL, "");
	assert_info(thread->userspace_fpage_table.pagetable_item != NULL, "mem page_item set");

	LOCKED(&space_lock)
	{
		arm_core_page_table_remove(thread);
		sys_dlist_remove(&thread->userspace_fpage_table.pagetable_node);
		thread->userspace_fpage_table.pagetable_item = NULL;
	}
}

void remove_from_page_table_of(struct thread_page *page_item)
{		  
	sys_dnode_t *pagetable_node, *next_table_node;

	assert_info(page_item != NULL, "");

	LOCKED(&space_lock)
	{
		arm_core_page_destroy(page_item);
		SYS_DLIST_FOR_EACH_NODE_SAFE(&page_item->pagetable_list, pagetable_node, next_table_node) 
		{
			struct ktcb *thread = CONTAINER_OF(pagetable_node, struct ktcb, userspace_fpage_table);
			sys_dlist_remove(&thread->userspace_fpage_table.pagetable_node);
			thread->userspace_fpage_table.pagetable_item = NULL;
		}
	}
}

exception_t do_guard_page(struct ktcb *s_thread, 
	struct ktcb *d_thread, 
	word_t addr,
	word_t len,
	word_t right)
{
	partition_attr_t right_attr = MEM_PARTITION_P_RW_U_NA;

	/* user right */
	switch (right)
	{
		/* na */
		case 0x0:
			right_attr = MEM_PARTITION_P_RW_U_NA;
			break;
		/* ro */
		case 0x4:
			/* v8 not support */
			right_attr = MEM_PARTITION_P_RW_U_RO;
			break;	
		/* rw */
		case 0x6:
			right_attr = MEM_PARTITION_P_RW_U_RW;
			break;
		/* rwx */
		case 0x7:
			right_attr = MEM_PARTITION_P_RWX_U_RWX;
			break;
		default:
			assert_info(0, "IPC Dest Thread Page is not conist.");
			break;
	}

	struct partition context = {
		.start = addr,
		.size = len,
		.attr = right_attr /* 0rwx */
	};

	struct thread_page *s_page = 
		s_thread->userspace_fpage_table.pagetable_item;
	struct thread_page *d_page = 
		d_thread->userspace_fpage_table.pagetable_item;

	if (s_page)
	{
		remove_from_page(s_page, &context);
	}

	if (!d_page)
	{
		d_page = d_object_alloc(obj_frame_obj, 0);

		assert_info(d_page, "IPC Dest Thread Page is not conist.");

		reset_page(d_page, 0, NULL);

	}

	add_to_page(d_page, &context);
	add_to_page_table(d_page, d_thread);

	return EXCEPTION_NONE;
}

exception_t do_map_page(struct ktcb *thread,
	word_t addr,
	word_t len,
	word_t right)
{
	partition_attr_t right_attr = MEM_PARTITION_P_RW_U_NA;

	/* user right */
	switch (right)
	{
		/* na */
		case 0x0:
			right_attr = MEM_PARTITION_P_RW_U_NA;
			break;
		/* ro */
		case 0x4:
			/* v8 not support */
			right_attr = MEM_PARTITION_P_RW_U_RO;
			break;	
		/* rw */
		case 0x6:
			right_attr = MEM_PARTITION_P_RW_U_RW;
			break;
		/* rwx */
		case 0x7:
			right_attr = MEM_PARTITION_P_RWX_U_RWX;
			break;
		default:
			assert_info(0, "IPC Dest Thread Page is not conist.");
			break;
	}

	struct partition context = {
		.start = addr,
		.size = len,
		.attr = right_attr
	};
		
	struct thread_page *page = 
		thread->userspace_fpage_table.pagetable_item;

	if (!page)
	{
		page = d_object_alloc(obj_frame_obj, 0);

		assert_info(page, "IPC Dest Thread Page is not conist.");
		reset_page(page, 0, NULL);
	}

	
	add_to_page(page, &context);
	add_to_page_table(page, thread);

	return EXCEPTION_NONE;

}

exception_t do_unmap_page(struct ktcb *thread)
{
	d_object_free(thread->userspace_fpage_table.pagetable_item);
	remove_from_page_table(thread);

	return EXCEPTION_NONE;

}

exception_t do_map_string(struct ktcb *thread,
	word_t len,
	word_t ptr)
{
	struct partition context = {
		.start = ptr,
		.size = len
	};
	struct thread_page *page = 
		thread->userspace_fpage_table.pagetable_item;
	
	if (!page)
	{
		page = d_object_alloc(obj_frame_obj, 0);

		assert_info(page, "IPC Dest Thread Page is not conist.");
		reset_page(page, 0, NULL);
	}

	
	add_to_page(page, &context);
	add_to_page_table(page, thread);

	return EXCEPTION_NONE;

}

exception_t syscall_unmap_page(word_t control)
{
	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
#if (0)
		if (inv_level != space_unmap)
		{
			user_error("Page Object: Illegal operation Unmap attempted.");
			current_syscall_error_code = TCR_INVAL_PARA;
			return EXCEPTION_SYSCALL_ERROR;
		}
#endif
	
#if defined(CONFIG_USERSPACE) 
#endif
	
		do_unmap_page(_current_thread);
	
		schedule();
		reschedule_unlocked();
	
		return EXCEPTION_NONE;

	}
	return EXCEPTION_FAULT;
}

static sword_t init_page_module(struct device *arg)
{
	ARG_UNUSED(arg);

	max_partitions = arm_core_page_max_partitions_get();
	
	/*
	 * max_partitions must be less than or equal to
	 * CONFIG_MAX_DOMAIN_PARTITIONS, or would encounter array index
	 * out of bounds error.
	 */
	assert_info(max_partitions <= CONFIG_MAX_DOMAIN_PARTITIONS, "");

	return 0;
}

SYS_INIT(init_page_module, pre_kernel_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
