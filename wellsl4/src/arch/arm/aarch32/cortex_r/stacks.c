
#include <aarch32/cortex_r/stack.h>
#include <sys/string.h>

THREAD_STACK_DEFINE(arm_fiq_stack,   CONFIG_ARMV7_FIQ_STACSIZE);
THREAD_STACK_DEFINE(arm_abort_stack, CONFIG_ARMV7_EXCEPTION_STACSIZE);
THREAD_STACK_DEFINE(arm_undef_stack, CONFIG_ARMV7_EXCEPTION_STACSIZE);
THREAD_STACK_DEFINE(arm_svc_stack,   CONFIG_ARMV7_SVC_STACSIZE);
THREAD_STACK_DEFINE(arm_sys_stack,   CONFIG_ARMV7_SYS_STACSIZE);

#if defined(CONFIG_INIT_STACKS)
void arm_init_stacks(void)
{
	memset(arm_fiq_stack, 0xAA, CONFIG_ARMV7_FIQ_STACSIZE);
	memset(arm_svc_stack, 0xAA, CONFIG_ARMV7_SVC_STACSIZE);
	memset(arm_abort_stack, 0xAA, CONFIG_ARMV7_EXCEPTION_STACSIZE);
	memset(arm_undef_stack, 0xAA, CONFIG_ARMV7_EXCEPTION_STACSIZE);
	memset(&_interrupt_stack, 0xAA, CONFIG_ISR_STACK_SIZE);
}
#endif
