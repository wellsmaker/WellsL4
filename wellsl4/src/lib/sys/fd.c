/**
 * @file
 * @brief File descriptor table
 *
 * This file provides generic file descriptor table implementation, suitable
 * for any I/O object implementing POSIX I/O semantics (i.e. read/write +
 * aux operations).
 */

#include <sys/errno.h>
#include <sys/fd.h>
#include <sys/util.h>

/** part function */

struct fd_entry {
	void *obj;
	const struct fd_op_vtable *vtable;
};

/* A few magic values for fd_entry::obj used in the code. */
#define FD_OBJ_RESERVED   (void *)0x01
#define FD_OBJ_STDIN      (void *)0x10
#define FD_OBJ_STDOUT     (void *)0x11
#define FD_OBJ_STDERR     (void *)0x12

#ifdef CONFIG_POSIX_API
static const struct fd_op_vtable stdinout_fd_op_vtable;
#endif

#ifdef CONFIG_POSIX_API
static struct fd_entry fdtable[CONFIG_POSIX_MAX_FDS] = {
	/*
	 * Predefine entries for stdin/stdout/stderr. Object pointer
	 * is unused and just should be !0 (random different values
	 * are used to posisbly help with debugging).
	 */
	{FD_OBJ_STDIN,  &stdinout_fd_op_vtable},
	{FD_OBJ_STDOUT, &stdinout_fd_op_vtable},
	{FD_OBJ_STDERR, &stdinout_fd_op_vtable},
};
#else
static struct fd_entry fdtable[0];
#endif

static sword_t find_fd_entry(void)
{
	sword_t fd;

	for (fd = 0; fd < ARRAY_SIZE(fdtable); fd++)
	{
		if (fdtable[fd].obj == NULL) 
		{
			return fd;
		}
	}

	errno = ENFILE;
	return -1;
}

static sword_t check_fd(sword_t fd)
{
	if (fd < 0 || fd >= ARRAY_SIZE(fdtable)) 
	{
		errno = EBADF;
		return -1;
	}

	fd = array_index_sanitize(fd, ARRAY_SIZE(fdtable));

	if (fdtable[fd].obj == NULL)
	{
		errno = EBADF;
		return -1;
	}

	return 0;
}

void *get_fd_obj(sword_t fd, const struct fd_op_vtable *vtable, sword_t err)
{
	struct fd_entry *fd_entry;

	if (check_fd(fd) < 0)
	{
		return NULL;
	}

	fd_entry = &fdtable[fd];

	if (vtable != NULL && fd_entry->vtable != vtable)
	{
		errno = err;
		return NULL;
	}

	return fd_entry->obj;
}

void *get_fd_obj_and_vtable(sword_t fd, const struct fd_op_vtable **vtable)
{
	struct fd_entry *fd_entry;

	if (check_fd(fd) < 0) 
	{
		return NULL;
	}

	fd_entry = &fdtable[fd];
	*vtable = fd_entry->vtable;

	return fd_entry->obj;
}

sword_t reserve_fd(void)
{
	sword_t fd;

	fd = find_fd_entry();
	if (fd >= 0) 
	{
		/* Mark entry as used, finalize_fd() will fill it in. */
		fdtable[fd].obj = FD_OBJ_RESERVED;
	}
	return fd;
}

void finalize_fd(sword_t fd, void *obj, const struct fd_op_vtable *vtable)
{
	/* Assumes fd was already bounds-checked. */
	fdtable[fd].obj = obj;
	fdtable[fd].vtable = vtable;
}

void free_fd(sword_t fd)
{
	/* Assumes fd was already bounds-checked. */
	fdtable[fd].obj = NULL;
}

sword_t alloc_fd(void *obj, const struct fd_op_vtable *vtable)
{
	sword_t fd;

	fd = reserve_fd();
	if (fd >= 0) 
	{
		finalize_fd(fd, obj, vtable);
	}

	return fd;
}

#ifdef CONFIG_POSIX_API
ssize_t read(sword_t fd, void *buf, size_t sz)
{
	if (check_fd(fd) < 0)
	{
		return -1;
	}

	return fdtable[fd].vtable->read(fdtable[fd].obj, buf, sz);
}
FUNC_ALIAS(read, _read, ssize_t);

ssize_t write(sword_t fd, const void *buf, size_t sz)
{
	if (check_fd(fd) < 0)
	{
		return -1;
	}

	return fdtable[fd].vtable->write(fdtable[fd].obj, buf, sz);
}
FUNC_ALIAS(write, _write, ssize_t);

sword_t close(sword_t fd)
{
	sword_t res;

	if (check_fd(fd) < 0)
	{
		return -1;
	}

	res = fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, IOCTL_CLOSE);
	free_fd(fd);

	return res;
}
FUNC_ALIAS(close, _close, sword_t);

sword_t fsync(sword_t fd)
{
	if (check_fd(fd) < 0)
	{
		return -1;
	}

	return fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, IOCTL_FSYNC);
}

off_t lseek(sword_t fd, off_t offset, sword_t whence)
{
	if (check_fd(fd) < 0)
	{
		return -1;
	}

	return fdtable_call_ioctl(fdtable[fd].vtable, fdtable[fd].obj, IOCTL_LSEEK, offset, whence);
}
FUNC_ALIAS(lseek, _lseek, off_t);

sword_t ioctl(sword_t fd, unsigned long request, ...)
{
	va_list args;
	sword_t res;

	if (check_fd(fd) < 0)
	{
		return -1;
	}

	va_start(args, request);
	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, request, args);
	va_end(args);

	return res;
}

/*
 * In the SimpleLink case, we have yet to add support for the fdtable
 * feature. The socket offload subsys has already defined fcntl, hence we
 * avoid redefining fcntl here.
 */
#ifndef CONFIG_SOC_FAMILY_TISIMPLELINK
sword_t fcntl(sword_t fd, sword_t cmd, ...)
{
	va_list args;
	sword_t res;

	if (check_fd(fd) < 0)
	{
		return -1;
	}

	/* Handle fdtable commands. */
	switch (cmd)
	{
	case F_DUPFD:
		/* Not implemented so far. */
		errno = EINVAL;
		return -1;
	}

	/* The rest of commands are per-fd, handled by ioctl vmethod. */
	va_start(args, cmd);
	res = fdtable[fd].vtable->ioctl(fdtable[fd].obj, cmd, args);
	va_end(args);

	return res;
}
#endif

/*
 * fd operations for stdio/stdout/stderr
 */
static ssize_t stdinout_read_vmeth(void *obj, void *buffer, size_t count)
{
	return 0;
}

static ssize_t stdinout_write_vmeth(void *obj, const void *buffer, size_t count)
{
	return 0;
}

static sword_t stdinout_ioctl_vmeth(void *obj, word_t request, va_list args)
{
	return 0;
}

static const struct fd_op_vtable stdinout_fd_op_vtable = {
	.read = stdinout_read_vmeth,
	.write = stdinout_write_vmeth,
	.ioctl = stdinout_ioctl_vmeth,
};

#endif
