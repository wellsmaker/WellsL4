
#include <linker/linker_defs.h>
#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu.h>
#include <sys/assert.h>
#include <sys/errno.h>
#include <device.h>
/*
 * Global status variable holding the number of HW MPU region indices, which
 * have been reserved by the MPU driver to program the static (fixed) memory
 * regions.
 */
static byte_t static_regions_num;

/**
 *  Get the number of supported MPU regions.
 */
static FORCE_INLINE byte_t get_num_regions(void)
{
#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4)
	/* Cortex-M0+, Cortex-M3, and Cortex-M4 MCUs may
	 * have a fixed number of 8 MPU regions.
	 */
	return 8;
#elif defined(DT_NUM_MPU_REGIONS)
	/* Retrieve the number of regions from DTS configuration. */
	return DT_NUM_MPU_REGIONS;
#else

	u32_t type = MPU->TYPE;
	type = (type & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
	return (byte_t)type;
#endif /* CPU_CORTEX_M0PLUS | CPU_CORTEX_M3 | CPU_CORTEX_M4 */
}


#if defined(CONFIG_CPU_CORTEX_M0) || \
	defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M1) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4) || \
	defined(CONFIG_CPU_CORTEX_M7)
#include "arm_mpu_v7m.c"
#elif defined(CONFIG_CPU_CORTEX_M23) || \
	defined(CONFIG_CPU_CORTEX_M33) || \
	defined(CONFIG_CPU_CORTEX_M35P)
#include "arm_mpu_v8m.c"
#else
#error "Unsupported ARM CPU"
#endif


static s32_t region_allocate_and_init(const byte_t index,
	const struct arm_mpu_region *region_conf)
{
	/* Attempt to allocate new region index. */
	if (index > (get_num_regions() - 1)) 
	{
		/* No available MPU region index. */
		return -EINVAL;
	}

	/* Program region */
	region_init(index, region_conf);

	return index;
}

/* This internal function programs an MPU region
 * of a given configuration at a given MPU index.
 */
static s32_t arm_core_mpu_configure_region(const byte_t index,
	const partition_t *new_region)
{
	struct arm_mpu_region region_conf;

	/* Populate internal ARM MPU region configuration structure. */
	region_conf.base = new_region->start;
	get_region_attr_from_k_mem_partition_info(&region_conf.attr,
		&new_region->attr, new_region->start, new_region->size);

	/* Allocate and program region */
	return region_allocate_and_init(index,
		(const struct arm_mpu_region *)&region_conf);
}

#if !defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS) || \
	!defined(CONFIG_MPU_GAP_FILLING)
/* This internal function programs a set of given MPU regions
 * over a background memory area, optionally performing a
 * sanity check of the memory regions to be programmed.
 */
static s32_t arm_core_mpu_configure_regions(const partition_t*regions[], 
	byte_t regions_num, byte_t start_reg_index, bool_t do_sanity_check)
{
	s32_t i;
	s32_t reg_index = start_reg_index;

	for (i = 0; i < regions_num; i++) 
	{
		if (regions[i]->size == 0U) 
		{
			continue;
		}
		
		/* Non-empty region. */

		if (do_sanity_check && (!mpu_partition_is_valid(regions[i]))) 
		{
			user_error("Partition %u: sanity check failed.", i);
			return -EINVAL;
		}

		reg_index = arm_core_mpu_configure_region(reg_index, regions[i]);

		if (reg_index == -EINVAL) 
		{
			return reg_index;
		}

		/* Increment number of programmed MPU indices. */
		reg_index++;
	}

	return reg_index;
}
#endif

/* ARM Core MPU Driver API Implementation for ARM MPU */

/**
 * @brief enable the MPU
 */
static void arm_core_mpu_enable(void)
{
	/* Enable MPU and use the default memory map as a
	 * background region for privileged software access.
	 */
	MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;

	/* Make sure that all the registers are set before proceeding */
	__DSB();
	__ISB();
}

/**
 * @brief disable the MPU
 */
static void arm_core_mpu_disable(void)
{
	/* Force any outstanding transfers to complete before disabling MPU */
	__DMB();

	/* Disable MPU */
	MPU->CTRL = 0;
}

#if defined(CONFIG_USERSPACE)
/**
 * @brief update configuration of an active memory partition
 */
void arm_core_mpu_mem_partition_config_update(
	partition_t *partition,
	partition_attr_t *new_attr)
{
	/* Find the partition. ASSERT if not found. */
	byte_t i;
	byte_t reg_index = get_num_regions();

	for (i = get_dyn_region_min_index(); i < get_num_regions(); i++)
	{
		if (!is_enabled_region(i)) 
		{
			continue;
		}

		u32_t base = mpu_region_get_base(i);

		if (base != partition->start) 
		{
			continue;
		}

		u32_t size = mpu_region_get_size(i);

		if (size != partition->size) 
		{
			continue;
		}

		/* Region found */
		reg_index = i;
		break;
	}
	
	assert_info(reg_index != get_num_regions(),
		 "Memory domain partition not found\n");

	/* Modify the permissions */
	partition->attr = *new_attr;
	arm_core_mpu_configure_region(reg_index, partition);
}

/**
 * @brief get the maximum number of available (free) MPU region indices
 *        for configuring dynamic MPU partitions
 */
s32_t arm_core_mpu_get_max_available_dyn_regions(void)
{
	return get_num_regions() - static_regions_num;
}

/**
 * @brief validate the given buffer is user accessible or not
 *
 * Presumes the background mapping is NOT user accessible.
 */
s32_t arm_core_mpu_buffer_validate(void *addr, size_t size, s32_t write)
{
	return mpu_buffer_validate(addr, size, write);
}

#endif /* CONFIG_USERSPACE */

/**
 * @brief configure fixed (static) MPU regions.
 */
void arm_core_mpu_configure_static_mpu_regions(const partition_t
	*static_regions[], const byte_t regions_num,
	const u32_t background_area_start, const u32_t background_area_end)
{
	if (mpu_configure_static_mpu_regions(static_regions, regions_num,
		background_area_start, background_area_end) == -EINVAL) 
	{
		user_error("Configuring %u static MPU regions failed\n", regions_num);
	}
}

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
/**
 * @brief mark memory areas for dynamic region configuration
 */
void arm_core_mpu_mark_areas_for_dynamic_regions(
	const partition_t dyn_region_areas[],
	const byte_t dyn_region_areas_num)
{
	if (mpu_mark_areas_for_dynamic_regions(dyn_region_areas,
		dyn_region_areas_num) == -EINVAL)
	{
		user_error("Marking %u areas for dynamic regions failed\n",
			dyn_region_areas_num);
	}
}
#endif /* CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS */

/**
 * @brief configure dynamic MPU regions.
 */
void arm_core_mpu_configure_dynamic_mpu_regions(const partition_t
	*dynamic_regions[], byte_t regions_num)
{
	if (mpu_configure_dynamic_mpu_regions(dynamic_regions, regions_num)
		== -EINVAL) 
	{
		user_error("Configuring %u dynamic MPU regions failed\n",
			regions_num);
	}
}

/* ARM MPU Driver Initial Setup */

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
static s32_t init_arm_mpu_module(struct device *arg)
{
	u32_t r_index;

	if (mpu_config.num_regions > get_num_regions()) 
	{
		/* Attempt to configure more MPU regions than
		 * what is supported by hardware. As this operation
		 * is executed during system (pre-kernel) initialization,
		 * we want to ensure we can detect an attempt to
		 * perform invalid configuration.
		 */
		user_error("Request to configure: %u regions (supported: %u)\n",
			mpu_config.num_regions,
			get_num_regions()
		);
		return -1;
	}

	arm_core_mpu_disable();

	/* Architecture-specific configuration */
	mpu_init();

	/* Program fixed regions configured at SOC definition. */
	for (r_index = 0U; r_index < mpu_config.num_regions; r_index++) 
	{
		region_init(r_index, &mpu_config.mpu_regions[r_index]);
	}

	/* Update the number of programmed MPU regions. */
	static_regions_num = mpu_config.num_regions;

	arm_core_mpu_enable();

	/* Sanity check for number of regions in Cortex-M0+, M3, and M4. */
#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4)
	assert_info(
		(MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos == 8,
		"Invalid number of MPU regions\n");
#elif defined(DT_NUM_MPU_REGIONS)
	assert_info(
		(MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos ==
		DT_NUM_MPU_REGIONS,
		"Invalid number of MPU regions\n");
#endif /* CORTEX_M0PLUS || CPU_CORTEX_M3 || CPU_CORTEX_M4 */
	return 0;
}

SYS_INIT(init_arm_mpu_module, pre_kernel_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
