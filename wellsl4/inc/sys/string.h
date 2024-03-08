/* string.h */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_MINIMAL_INCLUDE_STRING_H_
#define LIB_LIBC_MINIMAL_INCLUDE_STRING_H_

#include <types_def.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
	#define _MLIBC_RESTRICT __restrict__
#else
	#define _MLIBC_RESTRICT restrict
#endif

#define MEM_WORD_T_WIDTH __INTPTR_WIDTH__

/*
 * The mem_word_t should match the optimal memory access word width
 * on the target platform. Here we defaults it to uintptr_t.
 */

typedef uintptr_t mem_word_t;

int atoi(const char *s);
void *bsearch(const void *key, const void *array, size_t count, size_t size,
	      int (*cmp)(const void *key, const void *element));
unsigned long strtoul(const char *nptr, char **endptr, register int base);
int strncasecmp(const char *s1, const char *s2, size_t n);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strstr(const char *s, const char *find);
long strtol(const char *nptr, char **endptr, register int base);

char  *strcpy(char *_MLIBC_RESTRICT d, const char *_MLIBC_RESTRICT s);
char  *strncpy(char *_MLIBC_RESTRICT d, const char *_MLIBC_RESTRICT s,
		      size_t n);
char  *strchr(const char *s, sword_t c);
char  *strrchr(const char *s, sword_t c);
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
sword_t    strcmp(const char *s1, const char *s2);
sword_t    strncmp(const char *s1, const char *s2, size_t n);
char *strcat(char *_MLIBC_RESTRICT dest,
		    const char *_MLIBC_RESTRICT src);
char  *strncat(char *_MLIBC_RESTRICT d, const char *_MLIBC_RESTRICT s,
		      size_t n);
char *strstr(const char *s, const char *find);

size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);

sword_t    memcmp(const void *m1, const void *m2, size_t n);
void  *memmove(void *d, const void *s, size_t n);
void  *memcpy(void *_MLIBC_RESTRICT d, const void *_MLIBC_RESTRICT s,
		     size_t n);
void  *memset(void *buf, sword_t c, size_t n);
void  *memchr(const void *s, sword_t c, size_t n);

#ifdef __cplusplus
}
#endif

#endif
