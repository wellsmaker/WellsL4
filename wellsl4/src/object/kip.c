#include <object/kip.h>
#include <default/default.h>
#include <sys/assert.h>
#include <api/syscall.h>
#include <kernel/thread.h>

struct kip_info kip = 
{
	.kid      = 0x00000000,
	.apiv.raw = 0x84 << 24 | 7 << 16,  /* L4 X.2, rev 7 */
	.apif.raw = 0x00000000            /* Little endian 32-bit */
};

word_t syscall_kernel_interface(
	word_t *version, 
	word_t *flag, 
	word_t *id)
{

	bool_t is_sufficient = false;
	
	update_timestamp(false);
	is_sufficient = check_budget_restart();

	if (is_sufficient)
	{
		/*
#if defined(CONFIG_USERSPACE)
			user_error("KIP Object: Illegal operation attempted.");
			current_syscall_error_code = TCR_INVAIL_KIP;
			return (word_t)EXCEPTION_SYSCALL_ERROR;
#endif
		*/
		*version = kip.apiv.raw;
		*flag = kip.apif.raw;
		*id = kip.kid;
	
		schedule();
		/* reschedule_unlocked(); */

	}

	return (word_t)&kip;
}
