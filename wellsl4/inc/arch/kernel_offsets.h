#ifndef ARCH_KERNEL_OFFSETS_H_
#define ARCH_KERNEL_OFFSETS_H_

#if defined(CONFIG_X86) || defined(CONFIG_X86_64)
#include <arch/x86/kernel_offsets.h>
#elif defined(CONFIG_ARC)
#include <arch/arc/kernel_offsets.h>
#elif defined(CONFIG_XTENSA)
#include <arch/xtensa/kernel_offsets.h>
#elif defined(CONFIG_ARM64)
#include <arch/arm/aarch64/kernel_offsets.h>
#elif defined(CONFIG_ARM)
#include <arch/arm/aarch32/kernel_offsets.h>
#endif


#endif