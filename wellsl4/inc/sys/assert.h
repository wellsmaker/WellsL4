/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYS_ASSERT_H_
#define SYS_ASSERT_H_

#include <sys/stdbool.h>
#include <types_def.h>
#include <sys/printk.h>
#include <state/statedata.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef CONFIG_COLOUR_PRINTING
#define ANSI_RESET "\033[0m"
#define ANSI_GREEN ANSI_RESET "\033[32m"
#define ANSI_DARK  ANSI_RESET "\033[30;1m"
#else
#define ANSI_RESET ""
#define ANSI_GREEN ANSI_RESET ""
#define ANSI_DARK  ANSI_RESET ""
#endif

#ifdef CONFIG_ASSERT
#ifndef __ASSERT_ON
#define __ASSERT_ON CONFIG_ASSERT_LEVEL
#endif
#endif

#ifdef CONFIG_FORCE_NO_ASSERT
#undef __ASSERT_ON
#define __ASSERT_ON 0
#endif

#if defined(CONFIG_ASSERT_VERBOSE)
#define __ASSERT_PRINT(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define __ASSERT_PRINT(fmt, ...)
#endif

#ifdef CONFIG_ASSERT_NO_MSG_INFO
#define __ASSERT_MSG_INFO(fmt, ...)
#else
#define __ASSERT_MSG_INFO(fmt, ...) __ASSERT_PRINT("\t" fmt "\r\n", ##__VA_ARGS__)
#endif

#if !defined(CONFIG_ASSERT_NO_COND_INFO) && !defined(CONFIG_ASSERT_NO_FILE_INFO)
#define __ASSERT_LOC(test)                              \
	__ASSERT_PRINT("\rASSERTION FAIL [%s] @ %s:%d\r\n", \
	STRINGIFY(test),  __FILE__, __LINE__)
#endif

#if defined(CONFIG_ASSERT_NO_COND_INFO) && !defined(CONFIG_ASSERT_NO_FILE_INFO)
#define __ASSERT_LOC(test)                         \
	__ASSERT_PRINT("\rASSERTION FAIL @ %s:%d\r\n", \
	__FILE__, __LINE__)
#endif

#if !defined(CONFIG_ASSERT_NO_COND_INFO) && defined(CONFIG_ASSERT_NO_FILE_INFO)
#define __ASSERT_LOC(test)                      \
	__ASSERT_PRINT("\rASSERTION FAIL [%s]\r\n", \
	STRINGIFY(test))
#endif

#if defined(CONFIG_ASSERT_NO_COND_INFO) && defined(CONFIG_ASSERT_NO_FILE_INFO)
#define __ASSERT_LOC(test)                 \
	__ASSERT_PRINT("\rASSERTION FAIL\r\n")
#endif

#ifdef __ASSERT_ON

#if (__ASSERT_ON < 0) || (__ASSERT_ON > 2)
#error "Invalid assert level: must be between 0 and 2"
#endif

void assert_post_action(void);
#define __ASSERT_POST_ACTION() assert_post_action()

void _fail(
    const char  *str,
    const char  *file,
    word_t line,
    const char  *function
) __NORETURN;

#define fail(s) _fail(s, __FILE__, __LINE__, __func__)

void _assert_fail(
    const char  *assertion,
    const char  *file,
    word_t line,
    const char  *function
) __NORETURN;

#ifdef CONFIG_PRINTF_RESERVED
#define assert(test) 										  \
    if(!(test)) _assert_fail(#test, __FILE__, __LINE__, __FUNCTION__);
#else
#define assert(test)                                          \
	do {                                                      \
		if (!(test)) {                                        \
			__ASSERT_LOC(test);                               \
			__ASSERT_POST_ACTION();                           \
		}                                                     \
	} while (false)
#endif

#define assert_info(test, fmt, ...)                              \
	do {                                                      \
		if (!(test)) {                                        \
			__ASSERT_LOC(test);                               \
			__ASSERT_MSG_INFO(fmt, ##__VA_ARGS__);            \
			__ASSERT_POST_ACTION();                           \
		}                                                     \
	} while (false)

#define assert_eval(expr1, expr2, test, fmt, ...)           \
	do {                                                      \
		expr2;                                            	  \
		assert_info(test, fmt, ##__VA_ARGS__);              	  \
	} while (false)


#define user_error(...) \
	do {																	 \
		printk(ANSI_DARK "<<" ANSI_GREEN "WellsL4(CPU %lu)" ANSI_DARK			 \
				" [%s/%d T%p \"%s\" @%lx]: ",								 \
				_current_cpu_index, 					 					 \
				__func__, __LINE__, _current_cpu,				 			 \
				_current_thread,											 \
				_current_thread); 											 \
		printk(__VA_ARGS__);												 \
		printk(">>" ANSI_RESET "\r\n");										 \
	} while (0)

#else
#define assert_info(test, fmt, ...) { }
#define assert_eval(expr1, expr2, test, fmt, ...) expr1
#define assert(test) { }
#define user_error(...) { }
#endif


#ifdef __cplusplus
}
#endif

#endif
