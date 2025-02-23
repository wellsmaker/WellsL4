/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CMSIS interface file
 *
 * This header contains the interface to the ARM CMSIS Core headers.
 */

#ifndef ARCH_ARM_AARCH32_CORTEX_M_CMSIS_H_
#define ARCH_ARM_AARCH32_CORTEX_M_CMSIS_H_

#include <generated_dts_board.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CP10 Access Bits */
#define CPACR_CP10_Pos          20U
#define CPACR_CP10_Msk          (3UL << CPACR_CP10_Pos)
#define CPACR_CP10_NO_ACCESS    (0UL << CPACR_CP10_Pos)
#define CPACR_CP10_PRIV_ACCESS  (1UL << CPACR_CP10_Pos)
#define CPACR_CP10_RESERVED     (2UL << CPACR_CP10_Pos)
#define CPACR_CP10_FULL_ACCESS  (3UL << CPACR_CP10_Pos)

/* CP11 Access Bits */
#define CPACR_CP11_Pos          22U
#define CPACR_CP11_Msk          (3UL << CPACR_CP11_Pos)
#define CPACR_CP11_NO_ACCESS    (0UL << CPACR_CP11_Pos)
#define CPACR_CP11_PRIV_ACCESS  (1UL << CPACR_CP11_Pos)
#define CPACR_CP11_RESERVED     (2UL << CPACR_CP11_Pos)
#define CPACR_CP11_FULL_ACCESS  (3UL << CPACR_CP11_Pos)

#define SCB_UFSR  (*((__IOM u16_t *) &SCB->CFSR + 1))
#define SCB_BFSR  (*((__IOM byte_t *) &SCB->CFSR + 1))
#define SCB_MMFSR (*((__IOM byte_t *) &SCB->CFSR))

/* Fill in CMSIS required values for non-CMSIS compliant SoCs.
 * Use __NVIC_PRIO_BITS as it is required and simple to check, but
 * ultimately all SoCs will define their own CMSIS types and constants.
 */
#ifndef __NVIC_PRIO_BITS
typedef enum {
	Reset_IRQn                    = -15,
	NonMaskableInt_IRQn           = -14,
	HardFault_IRQn                = -13,
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	MemoryManagement_IRQn         = -12,
	BusFault_IRQn                 = -11,
	UsageFault_IRQn               = -10,
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	SecureFault_IRQn              = -9,
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
	SVCall_IRQn                   =  -5,
	DebugMonitor_IRQn             =  -4,
	PendSV_IRQn                   =  -2,
	SysTick_IRQn                  =  -1,
} IRQn_Type;

#if defined(CONFIG_CPU_CORTEX_M0)
#define __CM0_REV        0
#elif defined(CONFIG_CPU_CORTEX_M0PLUS)
#define __CM0PLUS_REV    0
#elif defined(CONFIG_CPU_CORTEX_M1)
#define __CM1_REV        0
#elif defined(CONFIG_CPU_CORTEX_M3)
#define __CM3_REV        0
#elif defined(CONFIG_CPU_CORTEX_M4)
#define __CM4_REV        0
#elif defined(CONFIG_CPU_CORTEX_M7)
#define __CM7_REV        0
#elif defined(CONFIG_CPU_CORTEX_M23)
#define __CM23_REV       0
#elif defined(CONFIG_CPU_CORTEX_M33)
#define __CM33_REV       0
#elif defined(CONFIG_CPU_CORTEX_M35P)
#define __CM35P_REV       0
#else
#error "Unknown Cortex-M device"
#endif

#ifndef __MPU_PRESENT
#define __MPU_PRESENT             1U
#endif
#define __NVIC_PRIO_BITS               DT_NUM_IRQ_PRIO_BITS
#define __Vendor_SysTickConfig         0 /* Default to standard SysTick */
#endif /* __NVIC_PRIO_BITS */

#if __NVIC_PRIO_BITS != DT_NUM_IRQ_PRIO_BITS
#error "DT_NUM_IRQ_PRIO_BITS and __NVIC_PRIO_BITS are not set to the same value"
#endif

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_CPU_CORTEX_M0)
#include <arch/arm/aarch32/cortex_m/core_cm0.h>
#elif defined(CONFIG_CPU_CORTEX_M0PLUS)
#include <arch/arm/aarch32/cortex_m/core_cm0plus.h>
#elif defined(CONFIG_CPU_CORTEX_M1)
#include <arch/arm/aarch32/cortex_m/core_cm1.h>
#elif defined(CONFIG_CPU_CORTEX_M3)
#include <arch/arm/aarch32/cortex_m/core_cm3.h>
#elif defined(CONFIG_CPU_CORTEX_M4)
#include <arch/arm/aarch32/cortex_m/core_cm4.h>
#elif defined(CONFIG_CPU_CORTEX_M7)
#include <arch/arm/aarch32/cortex_m/core_cm7.h>
#elif defined(CONFIG_CPU_CORTEX_M23)
#include <arch/arm/aarch32/cortex_m/core_cm23.h>
#elif defined(CONFIG_CPU_CORTEX_M33)
#include <arch/arm/aarch32/cortex_m/core_cm33.h>
#elif defined(CONFIG_CPU_CORTEX_M35P)
#include <arch/arm/aarch32/cortex_m/core_cm35p.h>
#elif defined(CONFIG_CPU_CORTEX_SC000)
#include <arch/arm/aarch32/cortex_m/core_sc000.h>
#elif defined(CONFIG_CPU_CORTEX_SC300)
#include <arch/arm/aarch32/cortex_m/core_sc300.h>
#elif defined(CONFIG_CPU_ARMV8MBL)
#include <arch/arm/aarch32/cortex_m/core_armv8mbl.h>
#elif defined(CONFIG_CPU_ARMV8MML)
#include <arch/arm/aarch32/cortex_m/core_armv8mml.h>
#elif defined(CONFIG_CPU_ARMV81MML)
#include <arch/arm/aarch32/cortex_m/core_armv81mml.h>
#else
#error "Unknown Cortex-M device"
#endif

#endif
