/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TOOLCHAIN_XCC_H_
#define TOOLCHAIN_XCC_H_

/* toolchain/gcc.h errors out if __BYTE_ORDER__ cannot be determined
 * there. However, __BYTE_ORDER__ is actually being defined later in
 * this file. So define __BYTE_ORDER__ to skip the check in gcc.h
 * and undefine after including gcc.h.
 *
 * Clang has it defined so there is no need to work around.
 */
#ifndef __clang__
#define __BYTE_ORDER__
#endif

#include <toolchain/gcc.h>

#ifndef __clang__
#undef __BYTE_ORDER__
#endif

#include <sys/stdbool.h>

#ifndef UINT32_C
#define UINT32_C(x)	x ## U
#endif

#ifndef __COUNTER__
/* XCC (GCC-based compiler) doesn't support __COUNTER__
 * but this should be good enough
 */
#define __COUNTER__ __LINE__
#endif

#undef __in_section_unique
#define __in_section_unique(seg) \
	__attribute__((section("." STRINGIFY(seg) "." STRINGIFY(__COUNTER__))))

#ifndef __GCC_LINKER_CMD__
#include <xtensa/config/core.h>

/*
 * XCC does not define the following macros with the expected names, but the
 * HAL defines similar ones. Thus we include it and define the missing macros
 * ourselves.
 */
#if XCHAL_MEMORY_ORDER == XTHAL_BIGENDIAN
#define __BYTE_ORDER__		__ORDER_BIG_ENDIAN__
#elif XCHAL_MEMORY_ORDER == XTHAL_LITTLEENDIAN
#define __BYTE_ORDER__		__ORDER_LITTLE_ENDIAN__
#else
#error "Cannot determine __BYTE_ORDER__"
#endif

#endif /* __GCC_LINKER_CMD__ */

#define __builtin_unreachable() do { assert_info(false, "Unreachable code"); } \
	while (true)

#endif
