/* printk.h - low-level debug output */

/*
 * Copyright (c) 2010-2012, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SYS_PRINTH_
#define SYS_PRINTH_


#include <toolchain.h>
#include <types_def.h>
#include <sys/inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#if(__GNUC__)
typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v)   __builtin_va_end(v)
#define va_arg(v,l)   __builtin_va_arg(v,l)
#define va_copy(d,s)  __builtin_va_copy(d,s)
#else
#error "GNUC IS NOT SUPPORT!"
#endif

/**
 *
 * @brief Print kernel debugging message.
 *
 * This routine prints a kernel debugging message to the system console.
 * Output is send immediately, without any mutual exclusion or buffering.
 *
 * A basic set of conversion specifier characters are supported:
 *   - signed decimal: \%d, \%i
 *   - unsigned decimal: \%u
 *   - unsigned hexadecimal: \%x (\%X is treated as \%x)
 *   - pointer: \%p
 *   - string: \%s
 *   - character: \%c
 *   - percent: \%\%
 *
 * Field width (with or without leading zeroes) is supported.
 * Length attributes h, hh, l, ll and z are supported. However, integral
 * values with %lld and %lli are only printed if they fit in a long
 * otherwise 'ERR' is printed. Full 64-bit values may be printed with %llx.
 * Flags and precision attributes are not supported.
 *
 * @param fmt Format string.
 * @param ... Optional list of format arguments.
 *
 * @return N/A
 */

__printf(1, 2) void printk(const char *fmt, ...);
__printf(1, 0) void vprintk(const char *fmt, va_list ap);
__printf(3, 4) sword_t snprintk(char *str, size_t size, const char *fmt, ...);
__printf(3, 0) sword_t vsnprintk(char *str,size_t size, const char *fmt, va_list ap);
__printf(3, 0) void vprintk_core(sword_t (*out)(sword_t f, void *c), void *ctx, 
	const char *fmt, va_list ap);

/* __syscall void uprintk_string_out(char *c, size_t n); */

#endif

#ifdef __cplusplus
}
#endif

#endif
