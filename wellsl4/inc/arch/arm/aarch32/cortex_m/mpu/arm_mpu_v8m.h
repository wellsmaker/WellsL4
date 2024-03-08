/*
 * Copyright (c) 2018 Linaro Limited.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Convenience macros to represent the ARMv8-M-specific
 * configuration for memory access permission and
 * cache-ability attribution.
 */

/* Privileged No Access, Unprivileged No Access */
/*#define NO_ACCESS       0x0 */
/*#define NO_ACCESS_Msk   ((NO_ACCESS << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk) */
/* Privileged No Access, Unprivileged No Access */
/*#define P_NA_U_NA       0x0 */
/*#define P_NA_U_NA_Msk   ((P_NA_U_NA << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk) */
/* Privileged Read Write, Unprivileged No Access */
#define P_RW_U_NA       0x0
#define P_RW_U_NA_Msk   ((P_RW_U_NA << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Write, Unprivileged Read Only */
/*#define P_RW_U_RO       0x2 */
/*#define P_RW_U_RO_Msk   ((P_RW_U_RO << MPU_RASR_AP_Pos) & MPU_RASR_AP_Msk)*/
/* Privileged Read Write, Unprivileged Read Write */
#define P_RW_U_RW       0x1
#define P_RW_U_RW_Msk   ((P_RW_U_RW << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Write, Unprivileged Read Write */
#define FULL_ACCESS     0x1
#define FULL_ACCESS_Msk ((FULL_ACCESS << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Only, Unprivileged No Access */
#define P_RO_U_NA       0x2
#define P_RO_U_NA_Msk   ((P_RO_U_NA << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Only, Unprivileged Read Only */
#define P_RO_U_RO       0x3
#define P_RO_U_RO_Msk   ((P_RO_U_RO << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)
/* Privileged Read Only, Unprivileged Read Only */
#define RO              0x3
#define RO_Msk          ((RO << MPU_RBAR_AP_Pos) & MPU_RBAR_AP_Msk)

/* Attribute flag for not-allowing execution (eXecute Never) */
#define NOT_EXEC MPU_RBAR_XN_Msk

/* Attribute flag for share-ability */
#define NON_SHAREABLE       0x0
#define NON_SHAREABLE_Msk ((NON_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)
#define OUTER_SHAREABLE 0x2
#define OUTER_SHAREABLE_Msk ((OUTER_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)
#define INNER_SHAREABLE 0x3
#define INNER_SHAREABLE_Msk ((INNER_SHAREABLE << MPU_RBAR_SH_Pos) & MPU_RBAR_SH_Msk)

/* Helper define to calculate the region limit address. */
#define REGION_LIMIT_ADDR(base, size) (((base & MPU_RBAR_BASE_Msk) + size - 1) & MPU_RLAR_LIMIT_Msk)


/* Attribute flag for cache-ability */

/* Read/Write Allocation Configurations for Cacheable Memory */
#define R_NON_W_NON     0x0 /* Do not allocate Read/Write */
#define R_NON_W_ALLOC   0x1 /* Do not allocate Read, Allocate Write */
#define R_ALLOC_W_NON   0x2 /* Allocate Read, Do not allocate Write */
#define R_ALLOC_W_ALLOC 0x3 /* Allocate Read/Write */

/* Memory Attributes for Normal Memory */
#define NORMAL_O_WT_NT  0x80 /* Normal, Outer Write-through non-transient */
#define NORMAL_O_WB_NT  0xC0 /* Normal, Outer Write-back non-transient */
#define NORMAL_O_NON_C  0x40 /* Normal, Outer Non-Cacheable  */

#define NORMAL_I_WT_NT  0x08 /* Normal, Inner Write-through non-transient */
#define NORMAL_I_WB_NT  0x0C /* Normal, Inner Write-back non-transient */
#define NORMAL_I_NON_C  0x04 /* Normal, Inner Non-Cacheable  */

#define NORMAL_OUTER_INNER_WRITE_THROUGH_READ_ALLOCATE_NON_TRANS \
	((NORMAL_O_WT_NT | (R_ALLOC_W_NON << 4)) \
	 | \
	 (NORMAL_I_WT_NT | R_ALLOC_W_NON)) \

#define NORMAL_OUTER_INNER_WRITE_BACWRITE_READ_ALLOCATE_NON_TRANS \
	((NORMAL_O_WB_NT | (R_ALLOC_W_ALLOC << 4)) \
	 | \
	 (NORMAL_I_WB_NT | R_ALLOC_W_ALLOC))

#define NORMAL_OUTER_INNER_NON_CACHEABLE \
	((NORMAL_O_NON_C | (R_NON_W_NON << 4)) \
	 | \
	 (NORMAL_I_NON_C | R_NON_W_NON))

/* Common cache-ability configuration for Flash, SRAM regions */
#define MPU_CACHE_ATTRIBUTES_FLASH \
	NORMAL_OUTER_INNER_WRITE_THROUGH_READ_ALLOCATE_NON_TRANS
#define MPU_CACHE_ATTRIBUTES_SRAM \
	NORMAL_OUTER_INNER_WRITE_BACWRITE_READ_ALLOCATE_NON_TRANS
#define MPU_CACHE_ATTRIBUTES_SRAM_NOCACHE \
	NORMAL_OUTER_INNER_NON_CACHEABLE

/* Global MAIR configurations */
#define MPU_MAIR_ATTR_FLASH         MPU_CACHE_ATTRIBUTES_FLASH
#define MPU_MAIR_INDEX_FLASH        0
#define MPU_MAIR_ATTR_SRAM          MPU_CACHE_ATTRIBUTES_SRAM
#define MPU_MAIR_INDEX_SRAM         1
#define MPU_MAIR_ATTR_SRAM_NOCACHE  MPU_CACHE_ATTRIBUTES_SRAM_NOCACHE
#define MPU_MAIR_INDEX_SRAM_NOCACHE 2

/* Some helper defines for common regions.
 *
 * Note that the ARMv8-M MPU architecture requires that the
 * enabled MPU regions are non-overlapping. Therefore, it is
 * recommended to use these helper defines only for configuring
 * fixed MPU regions at build-time (i.e. regions that are not
 * expected to be re-programmed or re-adjusted at run-time so
 * that they do not overlap with other MPU regions).
 */
#define REGION_RAM_ATTR(base, size) {\
		.rbar = NOT_EXEC | \
			P_RW_U_NA_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */ \
		.mair_idx = MPU_MAIR_INDEX_SRAM, \
		.r_limit = REGION_LIMIT_ADDR(base, size),  /* Region Limit */ \
	}

#if defined(CONFIG_MPU_ALLOW_FLASH_WRITE)
/* Note that the access permissions allow for un-privileged writes, contrary
 * to ARMv7-M where un-privileged code has Read-Only permissions.
 */
#define REGION_FLASH_ATTR(base, size) {\
		.rbar = P_RW_U_RW_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */ \
		.mair_idx = MPU_MAIR_INDEX_FLASH, \
		.r_limit = REGION_LIMIT_ADDR(base, size),  /* Region Limit */ \
	}
#else /* CONFIG_MPU_ALLOW_FLASH_WRITE */
#define REGION_FLASH_ATTR(base, size) {\
		.rbar = RO_Msk | NON_SHAREABLE_Msk, /* AP, XN, SH */ \
		/* Cache-ability */ \
		.mair_idx = MPU_MAIR_INDEX_FLASH, \
		.r_limit = REGION_LIMIT_ADDR(base, size),  /* Region Limit */ \
	}
#endif /* CONFIG_MPU_ALLOW_FLASH_WRITE */


struct arm_mpu_region_attr {
	/* Attributes belonging to RBAR */
	byte_t rbar: 5;
	/* MAIR index for attribute indirection */
	byte_t mair_idx: 3;
	/* Region Limit Address value to be written to the RLAR register. */
	u32_t r_limit;
};

typedef struct arm_mpu_region_attr arm_mpu_region_attr_t;

/* Typedef for the partition attribute */
struct partition_attr{
	u16_t rbar;
	u16_t mair_idx;
};

typedef struct partition_attr partition_attr_t;

/* Kernel macros for memory attribution
 * (access permissions and cache-ability).
 *
 * The macros are to be stored in partition_attr_t
 * k_objects. The format of a partition_attr_t object
 * is as follows: field <rbar> contains a direct mapping
 * of the <XN> and <AP> bit-fields of the RBAR register;
 * field <mair_idx> contains a direct mapping of AttrIdx
 * bit-field, stored in RLAR register.
 */

/* Read-Write access permission attributes */
#define MEM_PARTITION_P_RW_U_RW ((partition_attr_t) \
	{(P_RW_U_RW_Msk | NOT_EXEC), MPU_MAIR_INDEX_SRAM})
#define MEM_PARTITION_P_RW_U_NA ((partition_attr_t) \
	{(P_RW_U_NA_Msk | NOT_EXEC), MPU_MAIR_INDEX_SRAM})
#define MEM_PARTITION_P_RO_U_RO ((partition_attr_t) \
	{(P_RO_U_RO_Msk | NOT_EXEC), MPU_MAIR_INDEX_SRAM})
#define MEM_PARTITION_P_RO_U_NA ((partition_attr_t) \
	{(P_RO_U_NA_Msk | NOT_EXEC), MPU_MAIR_INDEX_SRAM})

/* Execution-allowed attributes */
#define MEM_PARTITION_P_RWX_U_RWX ((partition_attr_t) \
	{(P_RW_U_RW_Msk), MPU_MAIR_INDEX_SRAM})
#define MEM_PARTITION_P_RX_U_RX ((partition_attr_t) \
	{(P_RO_U_RO_Msk), MPU_MAIR_INDEX_SRAM})

/*
 * @brief Evaluate Write-ability
 *
 * Evaluate whether the access permissions include write-ability.
 *
 * @param attr The partition_attr_t object holding the
 *             MPU attributes to be checked against write-ability.
 */
#define PARTITION_IS_WRITABLE(attr) ({ \
		sword_t is_writable; \
			switch (attr.rbar & MPU_RBAR_AP_Msk) { \
			case P_RW_U_RW_Msk: \
			case P_RW_U_NA_Msk: \
				__is_writable__ = true; \
				break; \
			default: \
				__is_writable__ = false; \
		} \
		is_writable; \
	})

/*
 * @brief Evaluate Execution allowance
 *
 * Evaluate whether the access permissions include execution.
 *
 * @param attr The partition_attr_t object holding the
 *             MPU attributes to be checked against execution
 *             allowance.
 */
#define PARTITION_IS_EXECUTABLE(attr) (!((attr.rbar) & (NOT_EXEC)))

/* Attributes for no-cache enabling (share-ability is selected by default) */

/* Read-Write access permission attributes */
#define MEM_PARTITION_P_RW_U_RW_NOCACHE ((partition_attr_t) \
	{(P_RW_U_RW_Msk | NOT_EXEC | OUTER_SHAREABLE_Msk), \
		MPU_MAIR_INDEX_SRAM_NOCACHE})
#define MEM_PARTITION_P_RW_U_NA_NOCACHE ((partition_attr_t) \
	{(P_RW_U_NA_Msk | NOT_EXEC | OUTER_SHAREABLE_Msk), \
		MPU_MAIR_INDEX_SRAM_NOCACHE})
#define MEM_PARTITION_P_RO_U_RO_NOCACHE ((partition_attr_t) \
	{(P_RO_U_RO_Msk | NOT_EXEC | OUTER_SHAREABLE_Msk), \
		MPU_MAIR_INDEX_SRAM_NOCACHE})
#define MEM_PARTITION_P_RO_U_NA_NOCACHE ((partition_attr_t) \
	{(P_RO_U_NA_Msk | NOT_EXEC | OUTER_SHAREABLE_Msk), \
		MPU_MAIR_INDEX_SRAM_NOCACHE})

/* Execution-allowed attributes */
#define MEM_PARTITION_P_RWX_U_RWX_NOCACHE ((partition_attr_t) \
	{(P_RW_U_RW_Msk | OUTER_SHAREABLE_Msk), MPU_MAIR_INDEX_SRAM_NOCACHE})
#define MEM_PARTITION_P_RX_U_RX_NOCACHE ((partition_attr_t) \
	{(P_RO_U_RO_Msk | OUTER_SHAREABLE_Msk), MPU_MAIR_INDEX_SRAM_NOCACHE})

#endif /* _ASMLANGUAGE */

#define ARCH_PARTITION_ALIGN_CHECK(start, size) \
	BUILD_ASSERT_MSG((size > 0) && ((u32_t)start % \
			CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE == 0U) && \
		((size) % CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE == 0), \
		" the start and size of the partition must align " \
		"with the minimum MPU region size.")
