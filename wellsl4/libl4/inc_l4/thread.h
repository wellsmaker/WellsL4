#ifndef L4_THREAD_H_
#define L4_THREAD_H_

#include <inc_l4/types.h>
#include <syscalls/tcb.h>

/* TCR error code */
enum L4_TCR_Error {
	L4_ErrorOk				= 0,
	L4_ErrorNoPrivilege		= 1,
	L4_ErrorInvaildThread	= 2,
	L4_ErrorInvaildSpace	= 3,
	L4_ErrorInvaildScheduler= 4,
	L4_ErrorInvaildParam	= 5,
	L4_ErrorUtcbArea		= 6,
	L4_ErrorKipArea			= 7,
	L4_ErrorNoMem			= 8,
};

static inline L4_ThreadId_t L4_GlobalId(L4_Word_t threadno, L4_Word_t version)
{
	L4_ThreadId_t t;
	
	t.global.X.thread_no = threadno;
	t.global.X.version = version;
	
	return t;
}

static inline L4_Word_t L4_Version(L4_ThreadId_t t)
{
  	return t.global.X.version;
}

static inline L4_Word_t L4_ThreadNo(L4_ThreadId_t t)
{
  	return t.global.X.thread_no;
}

static inline L4_Bool_t L4_IsThreadEqual(const L4_ThreadId_t l, const L4_ThreadId_t r)
{
  	return l.raw == r.raw;
}

static inline L4_Bool_t L4_IsThreadNotEqual(const L4_ThreadId_t l, const L4_ThreadId_t r)
{
  	return l.raw != r.raw;
}

static inline L4_Bool_t L4_IsNilThread(L4_ThreadId_t t)
{
  	return t.raw == 0;
}

static inline L4_Bool_t L4_IsLocalId(L4_ThreadId_t t)
{
  	return t.local.X.zeros == 0;
}

static inline L4_Bool_t L4_IsGlobalId(L4_ThreadId_t t)
{
  	return t.local.X.zeros != 0;
}


static inline L4_Bool_t L4_ThreadWasHalted(L4_ThreadState_t s)
{
  	return s.raw & (1 << 0);
}

static inline L4_Bool_t L4_ThreadWasReceiving(L4_ThreadState_t s)
{
  	return s.raw & (1 << 1);
}

static inline L4_Bool_t L4_ThreadWasSending(L4_ThreadState_t s)
{
  	return s.raw & (1 << 2);
}

static inline L4_Bool_t L4_ThreadWasIpcing(L4_ThreadState_t s)
{
  	return s.raw & (3 << 1);
}

#if(0)
/* TCR related functions */
static inline L4_ThreadId_t L4_MyGlobalId(void)
{
	L4_ThreadId_t id;
	id = L4_TCR_MyGlobalId ();
	return id;
}

static inline L4_ThreadId_t L4_MyLocalId(void)
{
	L4_ThreadId_t id;
	id.raw = L4_TCR_MyLocalId ();
	return id;
}

static inline L4_ThreadId_t L4_Myself(void)
{
  	return L4_MyGlobalId ();
}

static inline L4_Word_t L4_ProcessorNo(void)
{
   return L4_TCR_ProcessorNo();
}

static inline L4_Word_t L4_UserDefinedHandle(void)
{
  	return L4_TCR_UserDefinedHandle();
}

static inline void L4_Set_UserDefinedHandle(L4_Word_t NewValue)
{
  	L4_TCR_Set_UserDefinedHandle(NewValue);
}

static inline L4_ThreadId_t L4_Pager(void)
{
	L4_ThreadId_t id;
	id.raw = L4_TCR_Pager();
	return id;
}

static inline void L4_Set_Pager(L4_ThreadId_t NewPager)
{
  	L4_TCR_Set_Pager(NewPager.raw);
}

static inline L4_ThreadId_t L4_ExceptionHandler(void)
{
	L4_ThreadId_t id;
	id.raw = L4_TCR_ExceptionHandler();
	return id;
}

static inline void L4_Set_ExceptionHandler(L4_ThreadId_t NewHandler)
{
  	L4_TCR_Set_ExceptionHandler(NewHandler.raw);
}

static inline L4_Word_t L4_ErrorCode(void)
{
  	return L4_TCR_ErrorCode();
}

static inline const char *L4_ErrorCode_String(L4_Word_t error)
{
	switch (error) 
	{
		case L4_ErrorOk:
			return "l4_error_ok";
		case L4_ErrorNoPrivilege:
			return "l4_error_no_privilege";
		case L4_ErrorInvaildThread:
			return "l4_error_invalid_thread";
		case L4_ErrorInvaildSpace:
			return "l4_error_invalid_space";
		case L4_ErrorInvaildScheduler:
			return "l4_error_invalid_scheduler";
		case L4_ErrorInvaildParam:
			return "l4_error_invalid_param";
		case L4_ErrorUtcbArea:
			return "l4_error_utcb_area";
		case L4_ErrorKipArea:
			return "l4_error_kip_area";
		case L4_ErrorNoMem:
			return "l4_error_no_mem";
		default:
			return "invalid error code";
	};
};

static inline L4_Word_t L4_XferTimeouts(void)
{
  	return L4_TCR_XferTimeout();
}

static inline void L4_Set_XferTimeouts(L4_Word_t NewValue)
{
  	L4_TCR_Set_XferTimeout(NewValue);
}

static inline L4_ThreadId_t L4_IntendedReceiver(void)
{
 	L4_ThreadId_t id;
 	id.raw = L4_TCR_IntendedReceiver ();
 	return id;
}

static inline L4_ThreadId_t L4_ActualSender(void)
{
 	L4_ThreadId_t id;
 	id.raw = L4_TCR_ActualSender();
 	return id;
}

static inline void L4_Set_VirtualSender(L4_ThreadId_t t)
{
   	L4_TCR_Set_VirtualSender(t.raw);
}

static inline L4_Word_t L4_WordSizeMask(void)
{
#if defined(L4_TCR_WORD_SIZE_MASK)
	return L4_TCR_WordSizeMask();
#else
	return (~((L4_Word_t) 0));
#endif
}

static inline void L4_Reset_WordSizeMask(void)
{
#if defined(L4_TCR_WORD_SIZE_MASK)
	L4_TCR_Set_WordSizeMask(~((L4_Word_t) 0));
#endif
}
#endif

/* L4_ThreadControl : control thread exec syscall function
	A privileged thread, e.g., the root server, can delete and create threads through this function. It can also modify the global
    thread ID (version field only) of an existing thread.
    Threads can be created as active or inactive threads. Inactive threads do not execute but can be activated by active
    threads that execute in the same address space.
    
    An actively created thread starts immediately by executing a short receive operation from its pager. (An active thread
	must have a pager.) The activeted thread expects a start message (MsgTag and two untyped words) from its pager.
	Once it receives the start message, it takes the value of MR1 as its new IP, the value of MR2 as its new SP, and then
	starts execution at user level with the received IP and SP. The new thread will execute on the same processor where the
	activating ThreadControl was invoked.
	Interrupt threads are treated as normal threads. They are active at system startup and can not be deleted or migrated
	into a different address space (i.e., SpaceSpecifier must be equal to the interrupt thread ID). When an interrupt occurs the
	interrupt thread sends an IPC to its pager and waits for an empty end-of-interrupt acknowledgment message (MR 0=0).
	Interrupt threads never raise pagefaults. To deactivate interrupt message delivery the pager is set to the interrupt thread’s
	own ID.
	
	[Privileged Systemcall]
	output:
	1. result(return)

	input:
	1. dest:Addressed thread. Must be a global thread ID. Only the thread number is effectively used
            to address the thread. If a thread with the specified thread number exists, its version bits are
            overwritten by the version bits of dest id and any ongoing IPC operations are aborted. Otherwise,
            the specified version bits are used for thread creations, i.e., a thread creation generates a thread
            with ID dest.
	2. SpaceSpecifier: != nilthread, dest not existing, create;!= nilthread, dest existing, modifiaction
						= nilthread, dest existing, deletion
	3. Scheduler: != nilthread, Defines the scheduler thread that is permitted to schedule the addressed thread. Note that the
				  scheduler thread must exist when the addressed thread starts executing;
				  = nilthread, The current scheduler association is not modified
	4. Pager:!= nilthread, The pager of dest is set to the specified thread. If dest was inactive before, it is activated
			 = nilthread, The current pager association is not modified.
	5. UtcbLocation: != -1, The start address of the UTCB of the thread is set to UtcbLocation;
	                  = -1, The UTCB location is not modified.

	pagefaults:
	none

	ErrorCode:
	1 2 3 4 6 8
*/


L4_Word_t L4_ThreadControl (L4_ThreadId_t   dest,
   L4_ThreadId_t space_specifier,
   L4_ThreadId_t scheduler,
   L4_ThreadId_t pager,
   void *utcb_location)
{
	L4_Word_t result;
	L4_Word_t except;

	except = thread_control(dest.raw, space_specifier.raw, scheduler.raw, 
		pager.raw);

	if (!except)
		result = L4_True;
	else
		result = L4_False;

	return result;
}

/* L4_ExchangeRegisters : read/exchange kernel registers syscall function
  Exchanges or reads a thread’s FLAGS, SP, and IP hardware registers as well as pager and UserDefinedHandle TCRs.
  Furthermore, thread execution can be suspended or resumed. The destination thread must be an active thread (see page 23)
  residing in the invoker’s address space.

  [Systemcall]
  output:
  	1. result(return)
    2. old control:
    3. old sp:current user-level stack pointer
    4. old ip:current user-level instruction pointer
    5. old flags:user-level processor flags of the thread
    6. old userdefinedhandle:thread’s UserDefinedHandle
    7. old pager:thread’s pager

  input:
    1. dest:Thread ID of the addressed thread. This may be a local or a global ID. However, the addressed
            thread must reside in the current address space.
    2. control:
    3. sp:current user-level stack pointer
    4. ip:current user-level instruction pointer
    5. flags:user-level processor flags of the thread
    6. userdefinedhandle:thread’s UserDefinedHandle
    7. pager:thread’s pager
    
  pagefaults:
    none

  ErrorCode:
  	2
  */


L4_ThreadId_t L4_ExchangeRegisters (L4_ThreadId_t dest,
   L4_Word_t control,
   L4_Word_t sp,
   L4_Word_t ip,
   L4_Word_t flags,
   L4_Word_t user_defined_handle,
   L4_ThreadId_t pager,
   L4_Word_t *old_control,
   L4_Word_t *old_sp,
   L4_Word_t *old_ip,
   L4_Word_t *old_flags,
   L4_Word_t *old_user_defined_handle,
   L4_ThreadId_t *old_pager)
{
	L4_ThreadId_t result;
	L4_Word_t except;

	except = exchange_registers(dest.raw, control, &sp, &ip, &flags);

	old_sp = &sp;
	old_ip = &ip;
	old_flags = &flags;

	if (!except)
		result = dest;
	else
		result.raw = 0;

	return result;
}

/* L4_ExchangeRegisters derived functions */
/*
 * These are the functions derived from the exchange register syscall.
 * For normal C, function overloading is not supported.  Functions
 * with unique names (i.e., with the suffix inside <> appended) have
 * therefore been provided.
 *
 *   L4_GlobalId							(t)
 *   L4_LocalId								(t)
 *   L4_SameThreads							(l, r)
 *   L4_UserDefinedHandle<Of>				(t)
 *   L4_Set_UserDefinedHandle<Of>			(t, handle)
 *   L4_Pager<Of>							(t)
 *   L4_Set_Pager<Of>						(t, pager)
 *   L4_Start								(t)
 *   L4_Start<_SpIp>						(t, sp, ip)
 *   L4_Start<_SpIpFlags>					(t, sp, ip, flag)
 *   L4_Stop								(t)
 *   L4_Stop<_SpIpFlags>					(t, &sp, &ip, &flag)
 *   L4_AbortReceive_and_stop				(t)
 *   L4_AbortReceive_and_stop<_SpIpFlags>	(t, &sp, &ip, &flag)
 *   L4_AbortSend_and_stop					(t)
 *   L4_AbortSend_and_stop<_SpIpFlags>		(t, &sp, &ip, &flag)
 *   L4_AbortIpc_and_stop					(t)
 *   L4_AbortIpc_and_stop<_SpIpFlags>		(t, &sp, &ip, &flag)
 *
 */

/*L4_ExchangeRegisters Input Control Registers : WRCdhpufisSRH*/
/* W  R  C  d h p u f i s S R H */
/* 12 11 10 9 8 7 6 5 4 3 2 1 0 */
/*L4_ExchangeRegisters Output Control Registers : SRH*/
/*
  W/R/C:extended control tranfer protocol(items);
  All control transfer items are passed in the message register of the invoking thread.
  	CtrlXferConfItem -> idmask -> If bit n in the mask is set to 1, the kernel will append
  	CtrlXferItem number n to the message.
  	
	C : 1 : the last CtrlXferConfItem
	R : 1 : (C=1) from the last CtrlXferConfItem read;(C=0) from MR0 read
	W : 1 : (R=1) from the last read item write;(C=1)from the last CtrlXferConfItem write;(C=0)from MR0 write

  hpufis:
  	s->SP
  	i->IP
  	f->flag
  	u->userdefinehandle
  	p->pager
  	h->H-flag
  	1 : the register/state is overwritten by the corresponding input parameter
  	0 : the corresponding input parameter is ignored and the register/state is not modified

  S/R:
  	Controls whether the addressed thread’s ongoing IPC opereration should be canceled/aborted
    through the system call or not.

    S : 0 : waiting to send or is sending a message of addressed thread,IPC continue and SP, 
    IP or FLAGS modifications are delayed until the IPC operation terminates
    S : 1 : waiting to send->canceled;is sending->aborted

    R : 0 : receive
    R : 1 : receive

  H: 
  	Halts/resumes the thread
    H : 0 : resume or no effect
    H : 1 : User-level thread execution is halted. Note that ongoing IPCs and other kernel operations are
    not affected by H

  d:
    d : 0 : return value undefined
    d : 1 : result parameters (IP, SP, FLAGS, UserDefinedHandle, pager, control) are delivered
*/

static inline L4_ThreadId_t L4_GlobaldIdOf(L4_ThreadId_t t)
{
	L4_Word_t     dummy;
	L4_ThreadId_t dummy_id;

	if (L4_IsLocalId (t))
		return L4_ExchangeRegisters(t, 0, 0, 0, 0, 0, L4_NILTHREAD, 
				&dummy, &dummy, &dummy, &dummy, &dummy, &dummy_id);
	else
		return t;
}

static inline L4_Bool_t L4_SameThreads(L4_ThreadId_t l, L4_ThreadId_t r)
{
  	return L4_GlobaldIdOf(l).raw == L4_GlobaldIdOf(r).raw;
}

static inline L4_ThreadId_t L4_LocalIdOf(L4_ThreadId_t t)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;

	if (L4_IsGlobalId (t))
		return L4_ExchangeRegisters(t, 0, 0, 0, 0, 0, L4_NILTHREAD,
				&dummy, &dummy, &dummy, &dummy, &dummy, &dummy_id);
	else
		return t;
}

static inline L4_Word_t L4_UserDefinedHandleOf(L4_ThreadId_t t)
{
	L4_Word_t dummy;
	L4_Word_t handle;
	L4_ThreadId_t dummy_id;

	/* d = 1 */
	(void)L4_ExchangeRegisters(t, (1 << 9), 0, 0, 0, 0, L4_NILTHREAD,
		&dummy, &dummy, &dummy, &dummy, &handle, &dummy_id);

	return handle;
}

static inline void L4_Set_UserDefinedHandleOf(
	L4_ThreadId_t t, 
	L4_Word_t handle)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;

	/* u = 1 */
	(void)L4_ExchangeRegisters(t, (1 << 6), 0, 0, 0, handle, L4_NILTHREAD,
		&dummy, &dummy, &dummy, &dummy, &dummy, &dummy_id);
}

static inline L4_ThreadId_t L4_PagerOf(L4_ThreadId_t t)
{
	L4_Word_t     dummy;
	L4_ThreadId_t pager;

	/* d = 1 */
	(void)L4_ExchangeRegisters(t, (1 << 9), 0, 0, 0, 0, L4_NILTHREAD,
		&dummy, &dummy, &dummy, &dummy, &dummy, &pager);

	return pager;
}

static inline void L4_Set_PagerOf(L4_ThreadId_t t, 
	L4_ThreadId_t p)
{
	L4_Word_t     dummy;
	L4_ThreadId_t dummy_id;

	/* p = 1 */
	(void)L4_ExchangeRegisters(t, (1 << 7), 0, 0, 0, 0, p,
		&dummy, &dummy, &dummy, &dummy, &dummy, &dummy_id);
}

static inline void L4_Start(L4_ThreadId_t t)
{
	L4_Word_t     dummy;
	L4_ThreadId_t dummy_id;

	/* h = 1; S/R = 1 */
	(void) L4_ExchangeRegisters(t, (1 << 8) + 6, 0, 0, 0, 0, L4_NILTHREAD,
		&dummy, &dummy, &dummy, &dummy, &dummy,&dummy_id);
}

static inline void L4_Start_SpIp(L4_ThreadId_t t, 
	L4_Word_t sp, 
	L4_Word_t ip)
{
	L4_Word_t     dummy;
	L4_ThreadId_t dummy_id;

	/* h = 1; S/R = 1; i = 1; s = 1 */
	(void)L4_ExchangeRegisters(t, (3 << 3) + (1 << 8) + 6, sp, ip, 0, 0, L4_NILTHREAD,
		&dummy, &dummy, &dummy, &dummy, &dummy, &dummy_id);
}


static inline void L4_Start_SpIpFlags(L4_ThreadId_t t, 
	L4_Word_t sp, 
	L4_Word_t ip, 
	L4_Word_t flag)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;

	/* h = 1; S/R = 1; i = 1; s = 1; f = 1 */
	(void)L4_ExchangeRegisters(t, (7 << 3) + (1 << 8) + 6, sp, ip, flag, 0, L4_NILTHREAD,
		&dummy, &dummy, &dummy, &dummy, &dummy, &dummy_id);
}

static inline L4_ThreadState_t L4_Stop(L4_ThreadId_t t)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;
	L4_ThreadState_t state;

	/* H,d,h = 1 */
	(void)L4_ExchangeRegisters(t, 1 + (1 << 8) + (1 << 9), 0, 0, 0, 0,L4_NILTHREAD, 
		&state.raw, &dummy, &dummy, &dummy, &dummy, &dummy_id);

	return state;
}

static inline L4_ThreadState_t L4_Stop_SpIpFlags(L4_ThreadId_t t, 
	L4_Word_t *sp, 
	L4_Word_t *ip, 
	L4_Word_t *flag)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;
	L4_ThreadState_t state;

	/* H,d,h = 1 */
	(void)L4_ExchangeRegisters(t, 1 + (1 << 8) + (1 << 9), 0, 0, 0, 0,L4_NILTHREAD, 
		&state.raw, sp, ip, flag,&dummy, &dummy_id);

	return state;
}

static inline L4_ThreadState_t L4_AbortReceive_and_stop(L4_ThreadId_t t)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;
	L4_ThreadState_t state;

	/* H,R,d,h = 1 */
	(void)L4_ExchangeRegisters(t, 3 + (1 << 8) + (1 << 9), 0, 0, 0, 0,L4_NILTHREAD,
		&state.raw, &dummy, &dummy,&dummy, &dummy, &dummy_id);

	return state;
}

static inline L4_ThreadState_t L4_AbortReceive_and_stop_SpIpFlags(
	L4_ThreadId_t t,
	L4_Word_t *sp,
	L4_Word_t *ip,
	L4_Word_t *flag)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;
	L4_ThreadState_t state;

	/* H,R,d,h = 1 */
	(void)L4_ExchangeRegisters(t, 3 + (1 << 8) + (1 << 9), 0, 0, 0, 0, L4_NILTHREAD, 
		&state.raw, sp, ip, flag, &dummy, &dummy_id);

	return state;
}

static inline L4_ThreadState_t L4_AbortSend_and_stop(L4_ThreadId_t t)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;
	L4_ThreadState_t state;

	/* H,S,d,h = 1 */
	(void)L4_ExchangeRegisters(t, 5 + (1 << 8) + (1 << 9), 0, 0, 0, 0, L4_NILTHREAD, 
		&state.raw, &dummy, &dummy, &dummy, &dummy, &dummy_id);

	return state;
}

static inline L4_ThreadState_t L4_AbortSend_and_stop_SpIpFlags(
	L4_ThreadId_t t, 
	L4_Word_t *sp, 
	L4_Word_t *ip, 
	L4_Word_t *flag)
{
	L4_Word_t 		dummy;
	L4_ThreadId_t 	dummy_id;
	L4_ThreadState_t 	state;

	/* H,S,d,h = 1 */
	(void)L4_ExchangeRegisters(t, 5 + (1 << 8) + (1 << 9), 0, 0, 0, 0, L4_NILTHREAD, 
		&state.raw, sp, ip, flag, &dummy, &dummy_id);

	return state;
}

static inline L4_ThreadState_t L4_AbortIpc_and_stop(L4_ThreadId_t t)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;
	L4_ThreadState_t state;

	/* H,S,R,d,h = 1 */
	(void)L4_ExchangeRegisters(t, 7 + (1 << 8) + (1 << 9), 0, 0, 0, 0, L4_NILTHREAD,
		&state.raw, &dummy, &dummy, &dummy, &dummy, &dummy_id);

	return state;
}

static inline L4_ThreadState_t L4_AbortIpc_and_stop_SpIpFlags(
	L4_ThreadId_t t, 
	L4_Word_t *sp, 
	L4_Word_t *ip, 
	L4_Word_t *flag)
{
	L4_Word_t dummy;
	L4_ThreadId_t dummy_id;
	L4_ThreadState_t state;

	/* H,S,R,d,h = 1 */
	(void)L4_ExchangeRegisters(t, 7 + (1 << 8) + (1 << 9), 0, 0, 0, 0, L4_NILTHREAD, 
		&state.raw, sp, ip, flag, &dummy, &dummy_id);

	return state;
}

#define L4_EXREGS_CTRLXFER_CONF_FLAG   (1UL << 10)
#define L4_EXREGS_CTRLXFER_READ_FLAG   (1UL << 11)
#define L4_EXREGS_CTRLXFER_WRITE_FLAG  (1UL << 12)

static inline L4_Word_t L4_ConfCtrlXferItems(L4_ThreadId_t dest)
{
	L4_Word_t		dummy;
	L4_Word_t		old_control;
	L4_ThreadId_t	dummy_tid;	

	L4_ExchangeRegisters(dest, L4_EXREGS_CTRLXFER_CONF_FLAG, 0, 0, 0, 0, L4_NILTHREAD,
						&old_control, &dummy, &dummy, &dummy, &dummy, &dummy_tid);

	return old_control;
}

static inline L4_Word_t L4_ReadCtrlXferItems(L4_ThreadId_t dest)
{
	L4_Word_t	    dummy;
	L4_Word_t	    old_control;
	L4_ThreadId_t dummy_tid;

	L4_ExchangeRegisters(dest, L4_EXREGS_CTRLXFER_READ_FLAG, 0, 0, 0, 0, L4_NILTHREAD,
						&old_control, &dummy, &dummy, &dummy, &dummy, &dummy_tid);

	return old_control;
}

static inline L4_Word_t L4_WriteCtrlXferItems(L4_ThreadId_t dest)
{
	L4_Word_t	    dummy;
	L4_Word_t	    old_control;
	L4_ThreadId_t dummy_tid;

	L4_ExchangeRegisters(dest, L4_EXREGS_CTRLXFER_WRITE_FLAG, 0, 0, 0, 0, L4_NILTHREAD,
						&old_control, &dummy, &dummy, &dummy, &dummy, &dummy_tid);

	return old_control;
}

static inline L4_Word_t L4_AssociateInterrupt(L4_ThreadId_t InterruptThread, 
	L4_ThreadId_t HandlerThread)
{
	/*For interrupt thread, SpaceSpecifier = interrupt thread id(SpaceSpecifier = dest)*/
	return L4_ThreadControl(InterruptThread, InterruptThread, L4_NILTHREAD, 
							HandlerThread, (void *) -1);
}

static inline L4_Word_t L4_DeassociateInterrupt(L4_ThreadId_t InterruptThread)
{
	/*For interrupt thread, SpaceSpecifier = interrupt thread id(SpaceSpecifier = dest)*/
	return L4_ThreadControl(InterruptThread, InterruptThread, L4_NILTHREAD, 
							InterruptThread, (void *) -1);
}

#endif
