/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ARM_AARCH32_CORTEX_M_MPU_ARM_MPU_H_
#define ARM_AARCH32_CORTEX_M_MPU_ARM_MPU_H_

#ifndef _ASMLANGUAGE
#include <types_def.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#if defined(CONFIG_CPU_CORTEX_M0) || \
	defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M1) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4) || \
	defined(CONFIG_CPU_CORTEX_M7)
#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu_v7m.h>
#elif defined(CONFIG_CPU_CORTEX_M23) || \
	defined(CONFIG_CPU_CORTEX_M33) || \
	defined(CONFIG_CPU_CORTEX_M35P)
#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu_v8m.h>
#else
#error "Unsupported ARM CPU"
#endif

/* Region definition data structure */
struct arm_mpu_region 
{
	/* Region Base Address */
	u32_t base;
	/* Region Name */
	const char *name;
	/* Region Attributes */
	struct arm_mpu_region_attr attr;
};

/* MPU configuration data structure */
struct arm_mpu_config 
{
	/* Number of regions */
	u32_t num_regions;
	/* Regions */
	const struct arm_mpu_region *mpu_regions;
};

/* memory partition */
struct partition {
	/** start address of memory partition */
	uintptr_t start;
	/** size of memory partition */
	size_t size;
	
#if defined(CONFIG_MEMORY_PROTECTION)
	/** attribute of memory partition */
	partition_attr_t attr;
#endif /* CONFIG_MEMORY_PROTECTION */
};

typedef struct partition partition_t;

#define MPU_REGION_ENTRY(_name, _base, _attr) { \
		.name = _name, \
		.base = _base, \
		.attr = _attr, \
	}

/* Reference to the MPU configuration.
 *
 * This struct is defined and populated for each SoC (in the SoC definition),
 * and holds the build-time configuration information for the fixed MPU
 * regions enabled during kernel initialization. Dynamic MPU regions (e.g.
 * for Thread Stack, Stack Guards, etc.) are programmed during runtime, thus,
 * not kept here.
 */
extern const struct arm_mpu_config mpu_config;

#endif
#endif
