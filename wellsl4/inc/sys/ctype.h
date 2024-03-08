/* ctype.h */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_
#define LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_

#include <types_def.h>

#ifdef __cplusplus
extern "C" {
#endif

static FORCE_INLINE sword_t isupper(sword_t a)
{
	return (sword_t)(((unsigned)(a)-(unsigned)'A') < 26U);
}

static FORCE_INLINE sword_t isalpha(sword_t c)
{
	return (sword_t)((((unsigned)c|32u)-(unsigned)'a') < 26U);
}

static FORCE_INLINE sword_t isspace(sword_t c)
{
	return (sword_t)(c == (sword_t)' ' || ((unsigned)c-(unsigned)'\t') < 5U);
}

static FORCE_INLINE sword_t isgraph(sword_t c)
{
	return (sword_t)((((unsigned)c) > ' ') &&
			(((unsigned)c) <= (unsigned)'~'));
}

static FORCE_INLINE sword_t isprint(sword_t c)
{
	return (sword_t)((((unsigned)c) >= ' ') &&
			(((unsigned)c) <= (unsigned)'~'));
}

static FORCE_INLINE sword_t isdigit(sword_t a)
{
	return (sword_t)(((unsigned)(a)-(unsigned)'0') < 10U);
}

static FORCE_INLINE sword_t isxdigit(sword_t a)
{
	word_t ua = (word_t)a;

	return (sword_t)(((ua - (unsigned)'0') < 10U) ||
			((ua | 32U) - (unsigned)'a' < 6U));
}

static FORCE_INLINE sword_t tolower(sword_t chr)
{
	return (chr >= (sword_t)'A' && chr <= (sword_t)'Z') ? (chr + 32) : (chr);
}

static FORCE_INLINE sword_t toupper(sword_t chr)
{
	return (sword_t)((chr >= (sword_t)'a' && chr <=
				(sword_t)'z') ? (chr - 32) : (chr));
}

static FORCE_INLINE sword_t isalnum(sword_t chr)
{
	return (sword_t)(isalpha(chr) || isdigit(chr));
}

#ifdef __cplusplus
}
#endif

#endif  /* LIB_LIBC_MINIMAL_INCLUDE_CTYPE_H_ */
