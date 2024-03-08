#ifndef L4_IPC_H_
#define L4_IPC_H_

#include <inc_l4/types.h>
#include <syscalls/ipc.h>

enum L4_IpcFlag {
	L4_IpcSuccess = 0x8,
	L4_IpcPropagte = 0x1,
	L4_IpcRedirecte = 0x2,
	L4_IpcXcpus = 0x4,
};

/* L4_Ipc : inter-process communication and synchronization syscall function
   IPC is the fundamental operation for inter-process communication and synchronization. It can be used for intra- and
   inter-address-space communication. All communication is synchronous and unbuffered: a message is transferred from
   the sender to the recipient if and only if the recipient has invoked a corresponding IPC operation. The sender blocks until
   this happens or until a period specified by the sender has elapsed without the destination becoming ready to receive.
   IPC can be used to copy data as well as to map or grant fpages from the sender to the recipient. For the description of
   messages see page 50. A single IPC call combines an optional send phase followed by an optional receive phase. Which
   phases are included is determined by the parameters to and FromSpecifier. Transitions between send phase and receive
   phase are atomic.
   Ipc operations are also controlled by MRs, BRs and some TCRs. RcvTimeout and SndTimeout are directly specified
   as system-call parameters. Each timeout can be 0,1(i.e., never expire), relative or absolute. For details on timeouts see
   page 30.

   IPC Default IPC function. Must always be used except if all criteria for using LIPC are fulfilled.
   LIPC IPC function that may be optimized for sending messages to local threads. Should be used
   whenever it is absolutely clear that in the overwhelming majority of all invocations.
   1. a send phase is included; and
   2. the destination thread is specified as a local thread ID; and
   3. a receive phase is included; and
   4. the destination thread runs on the same processor; and
   5. the RcvTimeout is infinite, and
   6. the IPC includes no map/grant operations.   
   
  [Systemcall]
  output: 
	1. from:Thread ID of the sender from which the IPC was received. Thread IDs are delivered as local
            thread IDs iff they identify a thread executing in the same address space as the current thread. It
            does not matter whether the sender specified the destination as local or global id.
            Only defined for IPC operations that include a receive phase.
  input:
	1. to:= nilthread IPC includes no send phase
	      != nilthread Destination thread; IPC includes a send phase
	2. FromSpecifier: = nilthread IPC includes no receive phase
					  = anythread IPC includes a receive phase. Incoming messages are accepted from any thread (including
						hardware interrupts)
					  = anylocalthread IPC includes a receive phase. Incoming messages are accepted from any thread that resides in
						the current address space
					  != above Ipc includes a receive phase. Incoming messages are accepted only from the specified thread.
						(Note that hardware interrupts can be specified.)
	3. Timeouts: 
		RcvTimeout(16):
   		  The receive phase waits until either a message transfer starts or the RcvTimeout expires. Ignored
            for send-only IPC operations.
            For relative receive timeout values, the receive timeout starts to run after the send phase has
            successfully completed. If the receive timeout expires before the message transfer has been
            started IPC fails with “receive timeout”. A pending incoming message is received if the timeout
            period is 0.
          
		SndTimeout(16):
			If the send timeout expires before the message transfer could start the IPC operation fails with
			“send timeout”. A send timeout of 0 ensures that IPC happens only if the addressed receiver is
			ready to receive when the send IPC operation is invoked. Otherwise, IPC fails immediately, i.e.,
			without blocking.
			
  pagefaults:
	1. Pre-send pagefaults
	2. Post-receive pagefaults
	3. Xfer pagefaults
	
  ErrorCode:
    x(28/56) e(3) p
    p = 0 : send
    p = 1 : receive

    error1 - Timeout - x
    error2 - Non-existing partner - x
    error3 - Canceled by another thread (system call exchange registers) - x
    error4 - Message Overflow - offset
    error5 - Xfer timeout during page fault in the invoker’s address space - offset
    error6 - Xfer timeout during page fault in the partner’s address space - offset
    error7 - Aborted by another thread (system call exchange registers) - offset
  
  input registers:
  	MR(0-u+t) - Message
  	TCR - XferTimeouts
  	BR(0-...) - Acceptor
  output registers:
    MR(0-u+t) - Message
*/

static inline L4_MsgTag_t L4_Ipc(L4_ThreadId_t To,
   L4_ThreadId_t From,
   L4_Word_t     Timeout,
   L4_ThreadId_t *FromAny)
{
	L4_MsgTag_t Tag;
	L4_Word_t Except;

	Except = exchange_ipc(To.raw, From.raw, Timeout, (L4_Word_t *)FromAny);
	
	if (!Except)
		Tag.raw = L4_MR0;
	else
		Tag.raw = 0x0;
	
	return Tag;
}

static inline L4_Bool_t L4_IpcSucceeded(L4_MsgTag_t Tag)
{
  	return(Tag.X.flag & L4_IpcSuccess) == 0;
}

static inline L4_Bool_t L4_IpcFailed(L4_MsgTag_t Tag)
{
  	return(Tag.X.flag & L4_IpcSuccess) != 0;
}

static inline L4_Bool_t L4_IpcPropagated(L4_MsgTag_t Tag)
{
  	return(Tag.X.flag & L4_IpcPropagte) != 0;
}

static inline L4_Bool_t L4_IpcRedirected(L4_MsgTag_t Tag)
{
  	return(Tag.X.flag & L4_IpcRedirecte) != 0;
}

static inline L4_Bool_t L4_IpcXcpu(L4_MsgTag_t Tag)
{
  	return(Tag.X.flag & L4_IpcXcpus) != 0;
}

static inline void L4_Set_Propagation(L4_MsgTag_t * Tag)
{
  	Tag->X.flag = 1;
}

static inline L4_Word_t L4_Timeouts(L4_Time_t SndTimeout, 
	L4_Time_t RcvTimeout)
{
  	return (SndTimeout.raw << 16) + (RcvTimeout.raw);
}

/*
 * These are the functions derived from the two IPC syscalls.  For
 * normal C, function overloading is not supported.  Functions with
 * unique names (i.e., with the suffix inside <> appended) have
 * therefore been provided.
 *
 *   L4_Call				(To)
 *   L4_Call<_Timeouts>		(To, SndTimeout, RcvTimeout)
 *   L4_Send				(To)
 *   L4_Send<_Timeout>		(To, SndTimeout)
 *   L4_Reply				(To)
 *   L4_Receive				(from)
 *   L4_Receive<_Timeout>	(from, RcvTimeout)
 *   L4_Wait				(&from)
 *   L4_Wait<_Timeout>		(RcvTimeout, &from)
 *   L4_ReplyWait			(To, &from)
 *   L4_ReplyWait<_Timeout>	(To, RcvTimeout, &from)
 *   L4_Sleep				(time)
 *   L4_Lcall				(To)
 *   L4_Lreply_Wait			(To, &from)
 *
 */

/* combination */
static inline L4_MsgTag_t L4_Call_Timeouts(L4_ThreadId_t To, 
	L4_Time_t SndTimeout, 
	L4_Time_t RcvTimeout, 
	L4_ThreadId_t *FromAny)
{
  	return L4_Ipc(To, To, 
		L4_Timeouts(SndTimeout, RcvTimeout), FromAny);
}

static inline L4_MsgTag_t L4_Call(L4_ThreadId_t To)
{
	L4_ThreadId_t FromAny;
  	return L4_Call_Timeouts(To, L4_NEVER, 
		L4_NEVER, &FromAny);
}

static inline L4_MsgTag_t L4_SendReceive(L4_ThreadId_t From, L4_ThreadId_t To)
{
	L4_ThreadId_t FromAny;
	  return L4_Ipc(To, From, 
		L4_Timeouts(L4_NEVER, L4_NEVER), &FromAny);
}

static inline L4_MsgTag_t L4_Send_Timeout(L4_ThreadId_t To, 
	L4_Time_t SndTimeout)
{
	L4_ThreadId_t FromAny;
  	return L4_Ipc(To, L4_NILTHREAD, 
		L4_Timeouts(SndTimeout, L4_ZEROTIME), &FromAny);
}

static inline L4_MsgTag_t L4_Send(L4_ThreadId_t To)
{
  	return L4_Send_Timeout(To, L4_NEVER);
}

static inline L4_MsgTag_t L4_Reply(L4_ThreadId_t To)
{
  	return L4_Send_Timeout(To, L4_ZEROTIME);
}

static inline L4_MsgTag_t L4_Receive_Timeout(L4_ThreadId_t From, 
	L4_Time_t RcvTimeout)
{
	L4_ThreadId_t FromAny;
  	return L4_Ipc(L4_NILTHREAD, From, 
		L4_Timeouts(L4_ZEROTIME, RcvTimeout), &FromAny);
}

static inline L4_MsgTag_t L4_Receive(L4_ThreadId_t From)
{
  	return L4_Receive_Timeout(From, L4_NEVER);
}

static inline L4_MsgTag_t L4_ReplyWait_Timeout(L4_ThreadId_t To, 
	L4_Time_t RcvTimeout, 
	L4_ThreadId_t *FromAny)
{
  	return L4_Ipc(To, L4_ANYTHREAD, 
		L4_Timeouts(L4_ZEROTIME, RcvTimeout), FromAny);
}

static inline L4_MsgTag_t L4_ReplyWait(L4_ThreadId_t To, 
	L4_ThreadId_t *FromAny)
{
  	return L4_ReplyWait_Timeout(To, L4_NEVER, FromAny);
}

/* standlone */
static inline L4_MsgTag_t L4_Wait_Timeout(L4_Time_t RcvTimeout,
	L4_ThreadId_t *FromAny)
{
	return L4_Ipc(L4_NILTHREAD, L4_ANYTHREAD, 
		L4_Timeouts(L4_ZEROTIME, RcvTimeout), FromAny);
}

static inline L4_MsgTag_t L4_Wait(L4_ThreadId_t *FromAny)
{
	return L4_Wait_Timeout(L4_NEVER, FromAny);
}

static inline void L4_Sleep(L4_Time_t Period)
{
	L4_ThreadId_t FromAny;
  	L4_Ipc(L4_NILTHREAD, L4_NILTHREAD, 
		(L4_Word_t)Period.raw, &FromAny);
}

#endif
