#ifndef UNTYPED_H_
#define UNTYPED_H

#include <types_def.h>
#include <api/fatal.h>
#include <kernel/cspace.h>
#include <arch/thread.h>
#include <kernel_object.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Obtain the size of a C string passed from user mode
 *
 * Given a C string pointer and a maximum size, obtain the true
 * size of the string (not including the trailing NULL byte) just as
 * if calling strnlen() on it, with the same semantics of strnlen() with
 * respect to the return value and the maxlen parameter.
 *
 * Any memory protection faults triggered by the examination of the string
 * will be safely handled and an error code returned.
 *
 * NOTE: Doesn't guarantee that user mode has actual access to this
 * string, you will need to still do a SYSCALL_MEMORY_READ()
 * with the obtained size value to guarantee this.
 *
 * @param src String to measure size of
 * @param maxlen Maximum number of characters to examine
 * @param err Pointer to sword_t, filled in with -1 on memory error, 0 on
 *	success
 * @return undefined on error, or strlen(src) if that is less than maxlen, or
 *	maxlen if there were no NULL terminating characters within the
 *	first maxlen bytes.
 */
static FORCE_INLINE size_t user_string_nlen(const char *src, size_t maxlen, sword_t *err)
{
	return arch_user_string_nlen(src, maxlen, err);
}

sword_t do_user_string_copy(char *dst, const char *src, size_t maxlen);
sword_t do_user_from_copy(void *dst, const void *src, size_t size);
sword_t do_user_to_copy(void *dst, const void *src, size_t size);


exception_t retype_untyped_object(struct d_object *dest, enum obj_tag user_type,
	bool_t reset, size_t user_size, struct d_object *src);

/*
__syscall exception_t retype_untyped(void *kobject, 
	size_t user_size, enum obj_tag user_type)
{
	return EXCEPTION_NONE;

}
*/


#ifdef __cplusplus
}
#endif

#endif
