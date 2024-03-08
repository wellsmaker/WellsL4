/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KERNEL_BOOT_H_
#define KERNEL_BOOT_H_

#include <kernel/thread.h>
#include <object/tcb.h>
#include <arch/cpu.h>
#include <sys/string.h>
#include <device.h>
#include <toolchain.h>
#include <types_def.h>
#include <state/statedata.h>
#include <kernel_object.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
void set_app_shmem_bss_zero(void);
void set_bss_zero(void);
void copy_data(void);
word_t get_random32_value(void);
void set_random32(void *dst, size_t outlen);
void set_random32_canary_word(byte_t *dst_word, size_t word_size);
FUNC_NORETURN void cstart(void);
#endif

/*
 * System initialization levels. The PRE_KERNEL_1 and PRE_KERNEL_2 levels are
 * executed in the kernel's initialization context, which uses the interrupt
 * stack. The remaining levels are executed in the kernel's main task.
 */
enum sys_init_level {
	sys_init_level_pre_kernel_1	= 0,
	sys_init_level_pre_kernel_2	= 1,
	sys_init_level_post_kernel	= 2,
	sys_init_level_application	= 3,
};

/**
 * @brief Test whether startup is in the before-main-task phase.
 *
 * This impacts which services are available for use, and the context
 * in which functions are run.
 *
 * @return true if and only if start up is still running pre-kernel
 * initialization.
 */
static FORCE_INLINE bool is_pre_kernel(void)
{
	return (sys_device_level < sys_init_level_post_kernel);
}

#ifdef __cplusplus
}
#endif

#endif
