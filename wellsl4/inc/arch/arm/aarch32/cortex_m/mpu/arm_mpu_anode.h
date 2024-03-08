/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ARCH_ARM_AARCH32_CORTEX_M_MPU_ARM_CORE_MPU_DEV_H_
#define ARCH_ARM_AARCH32_CORTEX_M_MPU_ARM_CORE_MPU_DEV_H_

#include <types_def.h>
#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @defgroup mem_domain_apis Memory domain APIs
 * @ingroup kernel_apis
 * @{
 */

#if defined(CONFIG_ARM_MPU)
#if defined(CONFIG_USERSPACE)

/**
 * @brief Maximum number of memory domain partitions
 *
 * This internal macro returns the maximum number of memory partitions, which
 * may be defined in a memory domain, given the amount of available HW MPU
 * regions.
 *
 * @param mpu_regions_num the number of available HW MPU regions.
 */
#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) && \
	defined(CONFIG_MPU_GAP_FILLING)
/*
 * For ARM MPU architectures, where the domain partitions cannot be defined
 * on top of the statically configured memory regions, the maximum number of
 * memory domain partitions is set to half of the number of available MPU
 * regions. This ensures that in the worst-case where there are gaps between
 * the memory partitions of the domain, the desired memory map can still be
 * programmed using the available number of HW MPU regions.
 */
#define ARM_CORE_MPU_MAX_DOMAIN_PARTITIONS_GET(mpu_regions_num) \
	(mpu_regions_num / 2)
#else
/*
 * For ARM MPU architectures, where the domain partitions can be defined
 * on top of the statically configured memory regions, the maximum number
 * of memory domain partitions is equal to the number of available MPU regions.
 */
#define ARM_CORE_MPU_MAX_DOMAIN_PARTITIONS_GET(mpu_regions_num) \
	(mpu_regions_num)
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief Maximum number of MPU regions required to configure a
 *        memory region for (user) Thread Stack.
 */
#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) && \
	defined(CONFIG_MPU_GAP_FILLING)
/* When dynamic regions may not be defined on top of statically
 * allocated memory regions, defining a region for a thread stack
 * requires two additional MPU regions to be configured; one for
 * defining the thread stack and an additional one for partitioning
 * the underlying memory area.
 */
#define ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_THREAD_STACK 2
#else
/* When dynamic regions may be defined on top of statically allocated
 * memory regions, a thread stack area may be configured using a
 * single MPU region.
 */
#define ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_THREAD_STACK 1
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief Maximum number of MPU regions required to configure a
 *        memory region for a (supervisor) Thread Stack Guard.
 */
#if (defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) && \
		defined(CONFIG_MPU_GAP_FILLING)) \
	|| defined(CONFIG_CPU_HAS_NXP_MPU)
/*
 * When dynamic regions may not be defined on top of statically
 * allocated memory regions, defining a region for a supervisor
 * thread stack right requires two additional MPU regions to be
 * configured; one for defining the stack right and an additional
 * one for partitioning the underlying memory area.
 *
 * The same is required for the NXP MPU due to its OR-based decision
 * policy; the MPU stack right applies more restrictive permissions on
 * the underlying (SRAM) regions, and, therefore, we need to partition
 * the underlying SRAM region.
 */
#define ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_MPU_STACGUARD 2
#elif defined(CONFIG_CPU_HAS_ARM_MPU)
/* When dynamic regions may be defined on top of statically allocated
 * memory regions, a supervisor thread stack right area may be configured
 * using a single MPU region.
 */
#define ARM_CORE_MPU_NUM_MPU_REGIONS_FOR_MPU_STACGUARD 1
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS || CPU_HAS_NXP_MPU */
#endif /* CONFIG_USERSPACE */

/* ARM Core MPU Driver API */

/*
 * This API has to be implemented by all the MPU drivers that have
 * ARM_MPU support.
 */

/**
 * @brief configure a set of fixed (static) MPU regions
 *
 * Internal API function to configure a set of static MPU memory regions,
 * within a (background) memory area determined by start and end address.
 * The total number of HW MPU regions to be programmed depends on the MPU
 * architecture.
 *
 * The function shall be invoked once, upon system initialization.
 *
 * @param static_regions[] an array of pointers to memory partitions
 *                         to be programmed
 * @param regions_num the number of regions to be programmed
 * @param background_area_start the start address of the background memory area
 * @param background_area_end the end address of the background memory area
 *
 * The function shall assert if the operation cannot be not performed
 * successfully. Therefore:
 * - the number of HW MPU regions to be programmed shall not exceed the number
 *   of available MPU indices,
 * - the size and alignment of the static regions shall comply with the
 *   requirements of the MPU hardware.
 */
extern void arm_core_mpu_configure_static_mpu_regions(
	const struct partition *static_regions[], const byte_t regions_num,
	const u32_t background_area_start, const u32_t background_area_end);

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)

/* Number of memory areas, inside which dynamic regions
 * may be programmed in run-time.
 */
#define MPU_DYNAMIC_REGION_AREAS_NUM 1

/**
 * @brief mark a set of memory regions as eligible for dynamic configuration
 *
 * Internal API function to configure a set of memory regions, determined
 * by their start address and size, as memory areas eligible for dynamically
 * programming MPU regions (such as a supervisor stack overflow right) at
 * run-time (for example, thread upon context-switch).
 *
 * The function shall be invoked once, upon system initialization.
 *
 * @param dyn_region_areas an array of partition k_objects declaring the
 *                             eligible memory areas for dynamic programming
 * @param dyn_region_areas_num the number of eligible areas for dynamic
 *                             programming.
 *
 * The function shall assert if the operation cannot be not performed
 * successfully. Therefore, the requested areas shall correspond to
 * static memory regions, configured earlier by
 * arm_core_mpu_configure_static_mpu_regions().
 */
extern void arm_core_mpu_mark_areas_for_dynamic_regions(
	const struct partition dyn_region_areas[],
	const byte_t dyn_region_areas_num);

#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief configure a set of dynamic MPU regions
 *
 * Internal API function to configure a set of dynamic MPU memory regions
 * within a (background) memory area. The total number of HW MPU regions
 * to be programmed depends on the MPU architecture.
 *
 * @param dynamic_regions[] an array of pointers to memory partitions
 *                          to be programmed
 * @param regions_num the number of regions to be programmed
 *
 * The function shall assert if the operation cannot be not performed
 * successfully. Therefore, the number of HW MPU regions to be programmed shall
 * not exceed the number of (currently) available MPU indices.
 */
extern void arm_core_mpu_configure_dynamic_mpu_regions(
	const struct partition *dynamic_regions[], byte_t regions_num);

#if defined(CONFIG_USERSPACE)
/**
 * @brief update configuration of an active memory partition
 *
 * Internal API function to re-configure the access permissions of an
 * active memory partition, i.e. a partition that has earlier been
 * configured in the (_current_thread) thread context.
 *
 * @param partition Pointer to a structure holding the partition information
 *                  (must be valid).
 * @param new_attr  New access permissions attribute for the partition.
 *
 * The function shall assert if the operation cannot be not performed
 * successfully (e.g. the given partition can not be found).
 */
extern void arm_core_mpu_mem_partition_config_update(
	struct partition *partition,
	partition_attr_t *new_attr);

#endif /* CONFIG_USERSPACE */

/**
 * @brief Get the maximum number of available (free) MPU region indices
 *        for configuring dynamic MPU regions.
 */
extern s32_t arm_core_mpu_get_max_available_dyn_regions(void);

/**
 * @brief validate the given buffer is user accessible or not
 *
 * Note: Validation will always return failure, if the supplied buffer
 *       spans multiple enabled MPU regions (even if these regions all
 *       permit user access).
 */
extern s32_t arm_core_mpu_buffer_validate(void *addr, size_t size, s32_t write);

#if defined(CONFIG_USERSPACE)
s32_t arm_core_page_max_partitions_get(void);
void arm_core_page_partition_add(struct thread_page *page_f, u32_t partition_id);
void arm_core_page_partition_remove(struct thread_page *page_f, u32_t partition_id);
void arm_core_page_destroy(struct thread_page *page_f);
void arm_core_page_table_add(struct ktcb *thread);
void arm_core_page_table_remove(struct ktcb *thread);
s32_t arm_core_buffer_validate(void *addr, size_t size, s32_t write);
#endif



#endif /* CONFIG_ARM_MPU */

#ifdef __cplusplus
}
#endif

#endif
