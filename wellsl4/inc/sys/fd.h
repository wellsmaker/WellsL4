/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SYS_FDTABLE_H_
#define SYS_FDTABLE_H_

#include <types_def.h>
#include <sys/printk.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __off_t_defined
#ifndef __USE_FILE_OFFSET64
typedef long int off_t;
#else
typedef long long int off_t;
#endif
#define __off_t_defined
#endif

/**
 * File descriptor virtual method table.
 * All operations beyond read/write go thru ioctl method.
 */
struct fd_op_vtable {
	ssize_t (*read)(void *obj, void *buf, size_t sz);
	ssize_t (*write)(void *obj, const void *buf, size_t sz);
	sword_t (*ioctl)(void *obj, word_t request, va_list args);
};

/**
 * @brief Reserve file descriptor.
 *
 * This function allows to reserve a space for file descriptor entry in
 * the underlying table, and thus allows caller to fail fast if no free
 * descriptor is available. If this function succeeds, finalize_fd()
 * or free_fd() must be called mandatorily.
 *
 * @return Allocated file descriptor, or -1 in case of error (errno is set)
 */
sword_t reserve_fd(void);

/**
 * @brief Finalize creation of file descriptor.
 *
 * This function should be called exactly once after reserve_fd(), and
 * should not be called in any other case.
 *
 * @param fd File descriptor previously returned by reserve_fd()
 * @param obj pointer to I/O object structure
 * @param vtable pointer to I/O operation implementations for the object
 */
void finalize_fd(sword_t fd, void *obj, const struct fd_op_vtable *vtable);

/**
 * @brief Allocate file descriptor for underlying I/O object.
 *
 * This function combines operations of reserve_fd() and finalize_fd()
 * in one step, and provided for convenience.
 *
 * @param obj pointer to I/O object structure
 * @param vtable pointer to I/O operation implementations for the object
 *
 * @return Allocated file descriptor, or -1 in case of error (errno is set)
 */
sword_t alloc_fd(void *obj, const struct fd_op_vtable *vtable);

/**
 * @brief Release reserved file descriptor.
 *
 * This function may be called once after reserve_fd(), and should
 * not be called in any other case.
 *
 * @param fd File descriptor previously returned by reserve_fd()
 */
void free_fd(sword_t fd);

/**
 * @brief Get underlying object pointer from file descriptor.
 *
 * This function is useful for functions other than read/write/ioctl
 * to look up underlying I/O object by fd, optionally checking its
 * type (using vtable reference). If fd refers to invalid entry,
 * NULL will be returned with errno set to EBADF. If fd is valid,
 * but vtable param is not NULL and doesn't match object's vtable,
 * NULL is returned and errno set to err param.
 *
 * @param fd File descriptor previously returned by reserve_fd()
 * @param vtable Expected object vtable or NULL
 * @param err errno value to set if object vtable doesn't match
 *
 * @return Object pointer or NULL, with errno set
 */
void *get_fd_obj(sword_t fd, const struct fd_op_vtable *vtable, sword_t err);

/**
 * @brief Get underlying object pointer and vtable pointer from file descriptor.
 *
 * @param fd File descriptor previously returned by reserve_fd()
 * @param vtable A pointer to a pointer variable to store the vtable
 *
 * @return Object pointer or NULL, with errno set
 */
void *get_fd_obj_and_vtable(sword_t fd, const struct fd_op_vtable **vtable);

/**
 * @brief Call ioctl vmethod on an object using varargs.
 *
 * We need this helper function because ioctl vmethod is declared to
 * take va_list and the only portable way to construct va_list is from
 * function's ... parameters.
 *
 * @param vtable vtable containing ioctl function pointer
 * @param obj Object to call ioctl on
 * @param request ioctl request number
 * @param ... Variadic arguments to ioctl
 */
static FORCE_INLINE sword_t fdtable_call_ioctl(const struct fd_op_vtable *vtable, void *obj,
				       unsigned long request, ...)
{
	va_list args;
	sword_t res;

	va_start(args, request);
	res = vtable->ioctl(obj, request, args);
	va_end(args);

	return res;
}

/**
 * Request codes for fd_op_vtable.ioctl().
 *
 * Note that these codes are internal WellL4 numbers, for internal
 * WellL4 operations (and subject to change without notice, not part
 * of "stable ABI"). These are however expected to co-exist with
 * "well-known" POSIX/Linux ioctl numbers, and not clash with them.
 */
enum {
	/* Codes below 0x100 are reserved for fcntl() codes. */
	IOCTL_CLOSE = 0x100,
	IOCTL_FSYNC,
	IOCTL_LSEEK,
	IOCTL_POLL_PREPARE,
	IOCTL_POLL_UPDATE,
	IOCTL_POLL_OFFLOAD,
	IOCTL_GETSOCKNAME,
};

#ifdef __cplusplus
}
#endif

#endif /* SYS_FDTABLE_H_ */
