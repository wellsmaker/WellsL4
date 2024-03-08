

#include <sys/assert.h>
#include <linker/linker_defs.h>
#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu_anode.h>
#include <object/anode.h>

/*
 * Maximum number of dynamic memory partitions that may be supplied to the MPU
 * driver for programming during run-time. Note that the actual number of the
 * available MPU regions for dynamic programming depends on the number of the
 * static MPU regions currently being programmed, and the total number of HW-
 * available MPU regions. This macro is only used internally in function
 * arm_configure_dynamic_mpu_regions(), to reserve sufficient area for the
 * array of dynamic regions passed to the underlying driver.
 */
#if defined(CONFIG_USERSPACE)
#define MAX_DYNAMIC_MPU_REGIONS_NUM \
	CONFIG_MAX_DOMAIN_PARTITIONS + /* User thread stack */ 1 + \
	(IS_ENABLED(CONFIG_MPU_STACK_GUARD) ? 1 : 0)
#else
#define MAX_DYNAMIC_MPU_REGIONS_NUM \
	(IS_ENABLED(CONFIG_MPU_STACK_GUARD) ? 1 : 0)
#endif /* CONFIG_USERSPACE */

/* Convenience macros to denote the start address and the size of the system
 * memory area, where dynamic memory regions may be programmed at run-time.
 */
#if defined(CONFIG_USERSPACE)
#define _MPU_DYNAMIC_REGIONS_AREA_START ((u32_t)&_app_smem_start)
#else
#define _MPU_DYNAMIC_REGIONS_AREA_START ((u32_t)&__kernel_ram_start)
#endif /* CONFIG_USERSPACE */

#define _MPU_DYNAMIC_REGIONS_AREA_SIZE ((u32_t)&__kernel_ram_end - \
		_MPU_DYNAMIC_REGIONS_AREA_START)

/**
 * @brief Use the HW-specific MPU driver to program
 *        the static MPU regions.
 *
 * Program the static MPU regions using the HW-specific MPU driver. The
 * function is meant to be invoked only once upon system initialization.
 *
 * If the function attempts to configure a number of regions beyond the
 * MPU HW limitations, the system behavior will be undefined.
 *
 * For some MPU architectures, such as the unmodified ARMv8-M MPU,
 * the function must execute with MPU enabled.
 */
void arm_configure_static_mpu_regions(void)
{
#if defined(CONFIG_COVERAGE_GCOV) && defined(CONFIG_USERSPACE)
	const struct partition gcov_region =
	{
		.start = (u32_t)&__gcov_bss_start,
		.size = (u32_t)&__gcov_bss_size,
		.attr = MEM_PARTITION_P_RW_U_RW,
	};
#endif /* CONFIG_COVERAGE_GCOV && CONFIG_USERSPACE */
#if defined(CONFIG_NOCACHE_MEMORY)
	const struct partition nocache_region =
	{
		.start = (u32_t)&_nocache_ram_start,
		.size = (u32_t)&_nocache_ram_size,
		.attr = MEM_PARTITION_P_RW_U_NA_NOCACHE,
	};
#endif /* CONFIG_NOCACHE_MEMORY */
#if defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
	const struct partition ramfunc_region =
	{
		.start = (u32_t)&_ramfunc_ram_start,
		.size = (u32_t)&_ramfunc_ram_size,
		.attr = MEM_PARTITION_P_RX_U_RX,
	};
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */

	/* Define a constant array of partition k_objects
	 * to hold the configuration of the respective static
	 * MPU regions.
	 */
	const struct partition *static_regions[] = 
	{
#if defined(CONFIG_COVERAGE_GCOV) && defined(CONFIG_USERSPACE)
		&gcov_region,
#endif /* CONFIG_COVERAGE_GCOV && CONFIG_USERSPACE */
#if defined(CONFIG_NOCACHE_MEMORY)
		&nocache_region,
#endif /* CONFIG_NOCACHE_MEMORY */
#if defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
		&ramfunc_region
#endif /* CONFIG_ARCH_HAS_RAMFUNC_SUPPORT */
	};

	/* Configure the static MPU regions within firmware SRAM boundaries.
	 * Start address of the image is given by _image_ram_start. The end
	 * of the firmware SRAM area is marked by __kernel_ram_end, taking
	 * into account the unused SRAM area, as well.
	 */
	arm_core_mpu_configure_static_mpu_regions(static_regions,
		ARRAY_SIZE(static_regions),
		(u32_t)&_image_ram_start,
		(u32_t)&__kernel_ram_end);

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
	/* Define a constant array of partition k_objects that holds the
	 * boundaries of the areas, inside which dynamic region programming
	 * is allowed. The information is passed to the underlying driver at
	 * initialization.
	 */
	const struct partition dyn_region_areas[] = 
	{
		{
			.start = _MPU_DYNAMIC_REGIONS_AREA_START,
			.size =  _MPU_DYNAMIC_REGIONS_AREA_SIZE,
		}
	};

	arm_core_mpu_mark_areas_for_dynamic_regions(dyn_region_areas,
		ARRAY_SIZE(dyn_region_areas));
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */
}

/**
 * @brief Use the HW-specific MPU driver to program
 *        the dynamic MPU regions.
 *
 * Program the dynamic MPU regions using the HW-specific MPU
 * driver. This function is meant to be invoked every time the
 * memory map is to be re-programmed, e.g during thread context
 * switch, entering user mode, reconfiguring memory domain, etc.
 *
 * For some MPU architectures, such as the unmodified ARMv8-M MPU,
 * the function must execute with MPU enabled.
 */
void arm_configure_dynamic_mpu_regions(struct ktcb *thread)
{
	/* Define an array of partition k_objects to hold the configuration
	 * of the respective dynamic MPU regions to be programmed for
	 * the given thread. The array of partitions (along with its
	 * actual size) will be supplied to the underlying MPU driver.
	 */
	struct partition *dynamic_regions[MAX_DYNAMIC_MPU_REGIONS_NUM];
	byte_t region_num = 0U;

#if defined(CONFIG_USERSPACE)
	struct partition thread_stack;

	/* Memory domain */
	struct thread_page *pagetable_item = thread->userspace_fpage_table.pagetable_item;

	if (pagetable_item) 
	{
		u32_t partitions_number = pagetable_item->partitions_number;
		struct partition partition;
		s32_t i;

		for (i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) 
		{
			partition = pagetable_item->partitions[i];
			if (partition.size == 0) 
			{
				/* Zero size indicates a non-existing
				 * memory partition.
				 */
				continue;
			}

			assert_info(region_num < MAX_DYNAMIC_MPU_REGIONS_NUM,
				"Out-of-bounds error for dynamic region map.");
			
			dynamic_regions[region_num] = &pagetable_item->partitions[i];
			region_num++;
			partitions_number--;
			
			if (partitions_number == 0U) 
			{
				break;
			}
		}
	}
	/* Thread user stack */
	if (thread->arch.priv_stack_start)
	{
		u32_t base = (u32_t)thread->userspace_stack_point;
		u32_t size = thread->stack_info.size + (thread->stack_info.start - base);

		assert_info(region_num < MAX_DYNAMIC_MPU_REGIONS_NUM,
			"Out-of-bounds error for dynamic region map.");
		
		thread_stack = (const struct partition){base, size, MEM_PARTITION_P_RW_U_RW};
		dynamic_regions[region_num] = &thread_stack;
		region_num++;
	}
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_MPU_STACK_GUARD)
	struct partition right;

	/* Privileged stack right */
	u32_t guard_start;
	u32_t guard_size = MPU_GUARD_ALIGN_AND_SIZE;

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	if ((thread->base.option & option_fp_option) != 0) 
	{
		guard_size = MPU_GUARD_ALIGN_AND_SIZE_FLOAT;
	}
#endif

#if defined(CONFIG_USERSPACE)
	if (thread->arch.priv_stack_start) 
	{
		guard_start = thread->arch.priv_stack_start - guard_size;

		assert_info((u32_t)&priv_stacks_ram_start <= guard_start,
		"Guard start: (0x%x) below privilege stacks boundary: (0x%x)",
		guard_start, (u32_t)&priv_stacks_ram_start);
	} 
	else 
	{
		guard_start = thread->stack_info.start - guard_size;

		assert_info((u32_t)thread->userspace_stack_point == guard_start,
		"Guard start (0x%x) not beginning at stack object (0x%x)\n",
		guard_start, (u32_t)thread->userspace_stack_point);
	}
#else
	guard_start = thread->stack_info.start - guard_size;
#endif /* CONFIG_USERSPACE */

	assert_info(region_num < MAX_DYNAMIC_MPU_REGIONS_NUM,
		"Out-of-bounds error for dynamic region map.");

	right = (const struct partition)
	{
		guard_start,
		guard_size,
		MEM_PARTITION_P_RO_U_NA
	};
		
	dynamic_regions[region_num] = &right;
	region_num++;
#endif /* CONFIG_MPU_STACK_GUARD */

	/* Configure the dynamic MPU regions */
	arm_core_mpu_configure_dynamic_mpu_regions(
		(const struct partition **)dynamic_regions,
		region_num);
}

#if defined(CONFIG_USERSPACE)
s32_t arm_core_page_max_partitions_get(void)
{
	s32_t available_regions = arm_core_mpu_get_max_available_dyn_regions();

	available_regions -= ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_THREAD_STACK;

	if (IS_ENABLED(CONFIG_MPU_STACK_GUARD)) 
	{
		available_regions -= ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_MPU_STACGUARD;
	}

	return ARM_CORE_MPU_MAX_DOMAIN_PARTITIONS_GET(available_regions);
}

void arm_core_page_partition_add(struct thread_page *page_f, u32_t partition_id)
{
  /* No-op on this architecture */
}

void arm_core_page_partition_remove(struct thread_page *page_f, u32_t partition_id)
{
	/* Request to remove a partition from a memory domain.
	 * This resets the access permissions of the partition
	 * to default (Privileged RW, Unprivileged NA).
	 */
	partition_attr_t reset_attr = MEM_PARTITION_P_RW_U_NA;

	if (_current_thread->userspace_fpage_table.pagetable_item != page_f)
	{
		return;
	}

	arm_core_mpu_mem_partition_config_update(
		&page_f->partitions[partition_id], &reset_attr);
}

void arm_core_page_destroy(struct thread_page *page_f)
{
	/* This function will reset the access permission configuration
	 * of the active partitions of the memory domain.
	 */
	s32_t i;
	struct partition partition;

	if (_current_thread->userspace_fpage_table.pagetable_item != page_f) 
	{
		return;
	}

	/* Partitions belonging to the memory page_f will be reset
	 * to default (Privileged RW, Unprivileged NA) permissions.
	 */
	partition_attr_t reset_attr = MEM_PARTITION_P_RW_U_NA;

	for (i = 0; i < CONFIG_MAX_DOMAIN_PARTITIONS; i++) 
	{
		partition = page_f->partitions[i];
		if (partition.size == 0U) 
		{
			/* Zero size indicates a non-existing
			 * memory partition.
			 */
			continue;
		}
		
		arm_core_mpu_mem_partition_config_update(&partition, &reset_attr);
	}
}

void arm_core_page_table_add(struct ktcb *thread)
{
	if (_current_thread != thread) 
	{
	 return;
	}

	/* Request to configure memory domain for a thread.
	* This triggers re-programming of the entire dynamic
	* memory map.
	*/
	arm_configure_dynamic_mpu_regions(thread);
}

void arm_core_page_table_remove(struct ktcb *thread)
{
	if (_current_thread != thread) 
	{
		return;
	}

	arm_core_page_destroy(thread->userspace_fpage_table.pagetable_item);
}

s32_t arm_core_buffer_validate(void *addr, size_t size, s32_t write)
{
	return arm_core_mpu_buffer_validate(addr, size, write);
}

#endif
