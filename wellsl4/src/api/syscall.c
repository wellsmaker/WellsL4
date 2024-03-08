#include <types_def.h>
#include <api/syscall.h>
#include <object/ipc.h>
#include <object/anode.h>
#include <object/tcb.h>
#include <object/interrupt.h>
#include <kernel/cspace.h>
#include <object/kip.h>
#include <kernel/time.h>
#include <kernel/thread.h>
#include <state/statedata.h>
#include <sys/assert.h>
#include <arch/registers.h>
#include <object/untyped.h>
#include <syscall_list.h>

#include <syscalls/device_binding_mrsh.c>
#include <syscalls/dobject_alloc_mrsh.c>
#include <syscalls/dobject_free_mrsh.c>
#include <syscalls/exchange_ipc_mrsh.c>
#include <syscalls/exchange_registers_mrsh.c>
#include <syscalls/kernel_interface_mrsh.c>
#include <syscalls/kobject_access_grant_mrsh.c>
#include <syscalls/kobject_access_revoke_mrsh.c>
#include <syscalls/processor_control_mrsh.c>
#include <syscalls/retype_untyped_mrsh.c>
#include <syscalls/schedule_control_mrsh.c>
#include <syscalls/space_control_mrsh.c>
#include <syscalls/switch_thread_mrsh.c>
#include <syscalls/system_clock_mrsh.c>
#include <syscalls/thread_control_mrsh.c>
#include <syscalls/unmap_page_mrsh.c>
#include <syscalls/uprintk_string_out_mrsh.c>

extern void arch_syscall_oops(void *ssf);
uintptr_t handle_invaild_handler(uintptr_t arg1, uintptr_t arg2,
				     uintptr_t arg3, uintptr_t arg4,
				     uintptr_t arg5, uintptr_t arg6,
				     void *ssf)
{
	user_error("Invalid system call id %" PRIuPTR " invoked", arg1);
	arch_syscall_oops(_current_cpu->syscall_frame_point);

	CODE_UNREACHABLE;
}

uintptr_t handle_reserved_handler(uintptr_t arg1, uintptr_t arg2,
				    uintptr_t arg3, uintptr_t arg4,
				    uintptr_t arg5, uintptr_t arg6, 
				    void *ssf)
{
	user_error("Unimplemented system call");
	arch_syscall_oops(_current_cpu->syscall_frame_point);
	
	CODE_UNREACHABLE;
}

#include <syscall_dispatch.c>