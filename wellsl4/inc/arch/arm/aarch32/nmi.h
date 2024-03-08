/**
 * @file
 *
 * @brief NMI routines for ARM Cortex M series
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARCH_ARM_AARCH32_NMI_H_
#define ARCH_ARM_AARCH32_NMI_H_

#ifndef _ASMLANGUAGE
#ifdef CONFIG_RUNTIME_NMI
extern void arm_nmi_init(void);
#define NMI_INIT() arm_nmi_init()
#else
#define NMI_INIT()
#endif
#endif

#endif
