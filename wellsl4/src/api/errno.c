#include <sys/errno.h>
#include <state/statedata.h>

/*
 * Define _k_neg_eagain for use in assembly files as errno.h is
 * not assembly language safe.
 * FIXME: wastes 4 bytes
 */
const int _k_neg_eagain = -EAGAIN;

#if defined(CONFIG_ERRNO) 
int *get_errno(void)
{
	return &_current_thread->errno_var;
}
#endif
