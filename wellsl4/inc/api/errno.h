#ifndef FAULT_H_
#define FAULT_H_

#include <types_def.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

enum syscall_error {
	/** TCR errors see L4 X.2 R7 Reference, Page 24-25 */
	/* No privilege. Current thread does not have privilege to perform the operation */
	TCR_NO_PRIVILIGE = 1ul << 0,
	/* Unavailable thread. The dest parameter specified a kernel thread or an unavailable 
	   interrupt thread */
	TCR_INVAL_THREAD = 1ul << 1, 
	/* Invalid space. The SpaceSpecifier parameter specified an invalid thread ID, or 
	   activation of a thread in a not yet initialized space */
	TCR_INVAL_SPACE	= 1ul << 2,
	/* Invalid scheduler. The scheduler paramter specified an invalid thread ID, or 
	   was set to nilthrad for a creating THREADCONTROL operation.*/ 
	TCR_INVAL_SCHED	= 1ul << 3,
	/* Invaild parameter */
	TCR_INVAL_PARA   = 1ul << 4,
	/* Invalid UTCB location. UtcbLocation lies outside of UTCB area, or attempt to 
	   change the UtcbLocation for an already active thread*/
	TCR_INVAL_UTCB = 1ul << 5, 
	/* Invaild Kip */
	TCR_INVAIL_KIP   = 1ul << 6,
	/* Out of memory. Kernel was not able to allocate the resources required to 
	   perform the operation */
	TCR_OUT_OF_MEM = 1ul << 7,

	/** Schedule errors see L4 X.2 R7 Reference, Page 24-25 */	
	/* TCR Error */
	SCHED_TCR_ERROR = 1ul << 8,
	/* Thread not exist */
	SCHED_THREAD_NOT_EXIST	= 1ul << 9,  /* DUMMY / DEAD / ABORTING */
	/* Thread State Invaild */
	SCHED_THREAD_INAVTIVE	= 1ul << 10, /* RESTART / SUSPEND */
	SCHED_THREAD_RUNNING	= 1ul << 11, /* QUEUED -> OTHER */
	SCHED_THREAD_SEND_BLOCKED	= 1ul << 12,
	SCHED_THREAD_SENDING	= 1ul << 13, /* SENDBLOCKED -> OTHER */
	SCHED_THREAD_RECV_BLOCKED	= 1ul << 14,
	SCHED_THREAD_RECEIVING	= 1ul << 15, /* RECVBLOCKED -> OTHER */
	SCHED_THREAD_NOTIFY_BLOCKED = 1ul << 16,
	
	/** Ipc errors see L4 X.2 R7 Reference, Page 67-68 
		NOTE: Message overflow may also occur if MR's are not enough */
	/* The error occurred during the send phase. */
	IPC_SEND_PHASE = 1ul << 17,
	/* The error occurred during the recv phase. */
	IPC_RECV_PHASE	 = 1ul << 18,
	/* Error happened before a partner thread was involved in the message transfer. 
	   Therefore, the error is signaled only to the thread that invoked the failing 
	   IPC operation. */
	/* From is undefined in this case */
	IPC_TIMEOUT	= 1ul << 19,
	/* Non-existing partner. If the error occurred in the send phase, 
	   to does not exist. (Anythread as a destination is illegal and will 
	   also raise this error.) If the error occurred in the receive phase,
	   FromSpecifier does not exist. (FromSpecifier = anythread is legal, 
	   and thus will never raise this error.) */
	IPC_NOT_EXIST = 1ul << 20,	
	/* Canceled by another thread (system call exchange registers). */
	IPC_CANCELED = 1ul << 21,

	/* A partner thread is already involved in the IPC operation, and the error is 
	   therefore signaled to both threads. */
	/* A message overflow can occur 
	   (1) if a receiving buffer string is too short, 
	   (2) if not enough buffer string items are present, 
	   and (4) if a map/grant of an fpage fails because 
	   the system has not enough page-table space available*/
	IPC_MSG_OVERFLOW = 1ul << 22,
	/* Xfer timeout during thread_page fault in the invoker’s address space */
	IPC_XFER_TIMEOUT_INVOKER    = 1ul << 23,	
	/* Xfer timeout during thread_page fault in the partner’s address space */
	IPC_XFER_TIMEOUT_PARTNER    = 1ul << 24,	
	/* Aborted by another thread (system call exchange registers) */
	IPC_ABORTED	= 1ul << 25	
};

typedef enum syscall_error syscall_error_t; /* errno member */

enum exception {
    EXCEPTION_NONE,
    EXCEPTION_FAULT,
    EXCEPTION_LOOKUP_FAULT,
    EXCEPTION_SYSCALL_ERROR,
    EXCEPTION_PREEMPTED
};
typedef word_t exception_t;

sword_t *get_errno(void);

#endif
#ifdef __cplusplus
}
#endif

#endif
