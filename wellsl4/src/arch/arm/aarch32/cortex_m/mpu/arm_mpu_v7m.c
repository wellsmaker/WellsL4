#include <sys/math_extras.h>
#include <arch/irq.h>

/* Global MPU configuration at system initialization. */
static void mpu_init(void)
{
	/* No specific configuration at init for ARMv7-M MPU. */
}

/* This internal function performs MPU region initialization.
 *
 * Note:
 *	 The caller must provide a valid region index.
 */
static void region_init(const u32_t index, const struct arm_mpu_region *region_conf)
{
	/* Select the region you want to access */
	MPU->RNR = index;
	/* Configure the region */
	MPU->RBAR = (region_conf->base & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk | index;
	MPU->RASR = region_conf->attr.rasr | MPU_RASR_ENABLE_Msk;
}

/* @brief Partition sanity check
 *
 * This internal function performs run-time sanity check for
 * MPU region start address and size.
 *
 * @param part Pointer to the data structure holding the partition
 *			   information (must be valid).
 */
static s32_t mpu_partition_is_valid(const partition_t *part)
{
	/* Partition size must be power-of-two,
	 * and greater or equal to the minimum
	 * MPU region size. Start address of the
	 * partition must align with size.
	 */
	s32_t partition_is_valid =
		((part->size & (part->size - 1)) == 0U) &&
		(part->size >= CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE) &&
		((part->start & (part->size - 1)) == 0U);

	return partition_is_valid;
}

/**
 * This internal function converts the region size to
 * the SIZE field value of MPU_RASR.
 *
 * Note: If size is not a power-of-two, it is rounded-up to the next
 * power-of-two value, and the returned SIZE field value corresponds
 * to that power-of-two value.
 */
static u32_t size_to_mpu_rasr_size(u32_t size)
{
	/* The minimal supported region size is 32 bytes */
	if (size <= 32U) 
	{
		return REGION_32B;
	}

	/*
	 * A size value greater than 2^31 could not be handled by
	 * round_up_to_next_power_of_two() properly. We handle
	 * it separately here.
	 */
	if (size > (1UL << 31)) 
	{
		return REGION_4G;
	}

	return ((32 - __builtin_clz(size - 1) - 2 + 1) << MPU_RASR_SIZE_Pos) & MPU_RASR_SIZE_Msk;
}

/**
 * This internal function is utilized by the MPU driver to combine a given
 * region attribute configuration and size and fill-in a driver-specific
 * structure with the correct MPU region configuration.
 */
static void get_region_attr_from_k_mem_partition_info(
	arm_mpu_region_attr_t *p_attr, const partition_attr_t *attr, 
	u32_t base, u32_t size)
{
	/* in ARMv7-M MPU the base address is not required
	 * to determine region attributes
	 */
	(void) base;

	p_attr->rasr = attr->rasr_attr | size_to_mpu_rasr_size(size);
}

#if defined(CONFIG_USERSPACE)
/**
 * This internal function returns the minimum HW MPU region index
 * that may hold the configuration of a dynamic memory region.
 *
 * Trivial for ARMv7-M MPU, where dynamic memory areas are programmed
 * in MPU regions indices right after the static regions.
 */
static s32_t get_dyn_region_min_index(void)
{
	return static_regions_num;
}

/**
 * This internal function converts the SIZE field value of MPU_RASR
 * to the region size (in bytes).
 */
static u32_t mpu_rasr_size_to_size(u32_t rasr_size)
{
	return 1 << (rasr_size + 1);
}

static u32_t mpu_region_get_base(u32_t index)
{
	MPU->RNR = index;
	return MPU->RBAR & MPU_RBAR_ADDR_Msk;
}

static u32_t mpu_region_get_size(u32_t index)
{
	MPU->RNR = index;
	u32_t rasr_size = (MPU->RASR & MPU_RASR_SIZE_Msk) >> MPU_RASR_SIZE_Pos;

	return mpu_rasr_size_to_size(rasr_size);
}

/**
 * This internal function checks if region is enabled or not.
 *
 * Note:
 *	 The caller must provide a valid region number.
 */
static s32_t is_enabled_region(u32_t index)
{
	/* Lock IRQs to ensure RNR value is correct when reading RASR. */
	word_t key;
	u32_t rasr;

	key = irq_lock();
	MPU->RNR = index;
	rasr = MPU->RASR;
	irq_unlock(key);

	return (rasr & MPU_RASR_ENABLE_Msk) ? 1 : 0;
}

/* Only a single bit is set for all user accessible permissions.
 * In ARMv7-M MPU this is bit AP[1].
 */
#define MPU_USER_READ_ACCESSIBLE_Msk (P_RW_U_RO & P_RW_U_RW & P_RO_U_RO & RO)

/**
 * This internal function returns the access permissions of an MPU region
 * specified by its region index.
 *
 * Note:
 *	 The caller must provide a valid region number.
 */
static u32_t get_region_ap(u32_t r_index)
{
	/* Lock IRQs to ensure RNR value is correct when reading RASR. */
	word_t key;
	u32_t rasr;

	key = irq_lock();
	MPU->RNR = r_index;
	rasr = MPU->RASR;
	irq_unlock(key);

	return (rasr & MPU_RASR_AP_Msk) >> MPU_RASR_AP_Pos;
}

/**
 * This internal function checks if the given buffer is in the region.
 *
 * Note:
 *	 The caller must provide a valid region number.
 */
static s32_t is_in_region(u32_t r_index, u32_t start, u32_t size)
{
	u32_t r_addr_start;
	u32_t r_size_lshift;
	u32_t r_addr_end;
	u32_t end;

	/* Lock IRQs to ensure RNR value is correct when reading RBAR, RASR. */
	word_t key;
	u32_t rbar, rasr;

	key = irq_lock();
	MPU->RNR = r_index;
	rbar = MPU->RBAR;
	rasr = MPU->RASR;
	irq_unlock(key);

	r_addr_start = rbar & MPU_RBAR_ADDR_Msk;
	r_size_lshift = ((rasr & MPU_RASR_SIZE_Msk) >> MPU_RASR_SIZE_Pos) + 1;
	r_addr_end = r_addr_start + (1UL << r_size_lshift) - 1;

	size = size == 0 ? 0 : size - 1;
	if (u32_add_overflow(start, size, &end)) 
	{
		return 0;
	}

	if ((start >= r_addr_start) && (end <= r_addr_end))
	{
		return 1;
	}

	return 0;
}

/**
 * This internal function checks if the region is user accessible or not.
 *
 * Note:
 *	 The caller must provide a valid region number.
 */
static s32_t is_user_accessible_region(u32_t r_index, s32_t write)
{
	u32_t r_ap = get_region_ap(r_index);


	if (write) 
	{
		return r_ap == P_RW_U_RW;
	}

	return r_ap & MPU_USER_READ_ACCESSIBLE_Msk;
}

/**
 * This internal function validates whether a given memory buffer
 * is user accessible or not.
 */
static s32_t mpu_buffer_validate(void *addr, size_t size, s32_t write)
{
	s32_t r_index;

	/* Iterate all mpu regions in reversed order */
	for (r_index = get_num_regions() - 1; r_index >= 0;  r_index--)
	{
		if (!is_enabled_region(r_index) || !is_in_region(r_index, (u32_t)addr, size)) 
		{
			continue;
		}

		/* For ARM MPU, higher region number takes sched_prior.
		 * Since we iterate all mpu regions in reversed order, so
		 * we can stop the iteration immediately once we find the
		 * matched region that grants permission or denies access.
		 */
		if (is_user_accessible_region(r_index, write)) 
		{
			return 0;
		}
		else
		{
			return -EPERM;
		}
	}

	return -EPERM;

}
#endif /* CONFIG_USERSPACE */

static s32_t arm_core_mpu_configure_region(const byte_t index,
	const partition_t *new_region);
static s32_t arm_core_mpu_configure_regions(const partition_t*regions[], 
	byte_t regions_num, byte_t start_reg_index, bool_t do_sanity_check);

/* This internal function programs the static MPU regions.
 *
 * It returns the number of MPU region indices configured.
 *
 * Note:
 * If the static MPU regions configuration has not been successfully
 * performed, the error signal is propagated to the caller of the function.
 */
static s32_t mpu_configure_static_mpu_regions(const partition_t
	*static_regions[], const byte_t regions_num,
	const u32_t background_area_base,
	const u32_t background_area_end)
{
	s32_t mpu_reg_index = static_regions_num;

	/* In ARMv7-M architecture the static regions are
	 * programmed on top of SRAM region configuration.
	 */
	ARG_UNUSED(background_area_base);
	ARG_UNUSED(background_area_end);
	
	mpu_reg_index = arm_core_mpu_configure_regions(static_regions, regions_num, mpu_reg_index, true);
	static_regions_num = mpu_reg_index;

	return mpu_reg_index;
}

/* This internal function programs the dynamic MPU regions.
 *
 * It returns the number of MPU region indices configured.
 *
 * Note:
 * If the dynamic MPU regions configuration has not been successfully
 * performed, the error signal is propagated to the caller of the function.
 */
static s32_t mpu_configure_dynamic_mpu_regions(const partition_t
	*dynamic_regions[], byte_t regions_num)
{
	s32_t mpu_reg_index = static_regions_num;

	/* In ARMv7-M architecture the dynamic regions are
	 * programmed on top of existing SRAM region configuration.
	 */

	mpu_reg_index = arm_core_mpu_configure_regions(dynamic_regions, regions_num, mpu_reg_index, false);

	if (mpu_reg_index != -EINVAL) 
	{
		/* Disable the non-programmed MPU regions. */
		for (s32_t i = mpu_reg_index; i < get_num_regions(); i++) 
		{
			ARM_MPU_ClrRegion(i);
		}
	}

	return mpu_reg_index;
}