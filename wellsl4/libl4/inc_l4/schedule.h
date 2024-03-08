#ifndef L4_SCHEDULE_H_
#define L4_SCHEDULE_H_

#include <inc_l4/types.h>
#include <syscalls/time.h>
#include <syscalls/registers.h>
#include <syscalls/tcb.h>

/* L4_ProcessorControl : processer configuration syscall function
   Control the internal frequency, external frequency, or voltage for a system processor

  [Privileged Systemcall]
  output:
   1. result:The result is 1 if the operation succeeded, otherwise the result is 0 and the ErrorCode TCR
  		   indicates the failure reason.

  input:
   1. ProcessorNo
   2. InternalFrequency
   3. ExternalFrequency
   4. voltage
   
  pagefaults:
   none
  
  ErrorCode:
   1

*/

L4_Word_t L4_ProcessorControl(L4_Word_t processor_no,
    L4_Word_t internal_frequency,
    L4_Word_t external_frequency,
    L4_Word_t voltage)
{
	L4_Word_t result;
	L4_Word_t except;

	except = processor_control(processor_no, internal_frequency, 
		external_frequency, voltage);
	
	if (!except)
		result = L4_True;
	else
		result = L4_False;

	return result;
}

/* L4_SpaceControl : config as syscall function
  A privileged thread, e.g., the root server, can configure address spaces through this function

  [Privileged Systemcall]
  output:
	1. result:The result is 1 if the operation succeeded, otherwise the result is 0 and the ErrorCode TCR
			  indicates the failure reason.
	2. control
	
  input:
	1. SpaceSpecifier
	2. control
	3. KernelInterfacePageArea
	4. UtcbArea
	5. Redirector
	
  pagefaults:
	none

  ErrorCode:
    1 3 6 7
	
*/
/* The specified fpages (located in MR0...) are unmapped. 
   Fpages are mapped as part of the IPC operation (see thread_page 64).*/

/* L4_Unmap Control : 0(25/57) f k(6)*/
/* 
  f = 0 : _current_thread address space not changed
  f = 1 : _current_thread address space changed

  k 	: Specifies the highest MRk that holds an fpage to be unmapped.
		  The number of fpages is thus k + 1
  */

L4_Word_t L4_SpaceControl(L4_ThreadId_t space_specifier,
    L4_Word_t control,
    L4_Fpage_t kernel_interface_page_area,
    L4_Fpage_t utcb_area,
    L4_ThreadId_t redirector,
    L4_Word_t *old_control)
{
	L4_Word_t result;
	L4_Word_t except;

	except = space_control(space_specifier.raw, control, 
		kernel_interface_page_area.raw, utcb_area.raw, redirector.raw);

	if (!except)
		result = L4_True;
	else
		result = L4_False;

	return result;
}

/* L4_SystemClock : get system clock syscall function
   Delivers the current system clock. Typically, the operation does not enter kernel mode.
   
   [Systemcall]
   output:
     1. clock(return)
   input:
     none
   pagefaults:
     none
*/
L4_Clock_t L4_SystemClock(void)
{
	L4_Clock_t temp = {
		.raw = 0,
	};

	system_clock(&(temp.raw));
	return temp;
}

/* L4_ThreadSwitch : switch other thread(not pree) syscall function
   The invoking thread releases the processor (non-preemptively) so that another ready thread can be processed. 

   [Systemcall]
   output:
	 none
   input:
     1. dest: = nilthread, Processing switches to an undefined ready thread which is selected by the scheduler. (It might
				be the invoking thread.) Since this is “ordinary” scheduling, the thread gets a new timeslice.
			  != nilthread, If dest is ready, processing switches to this thread. In this “extraordinary” scheduling, the invoking
              thread donates its remaining timeslice to the destination thread. (This one gets the donation
              in addition to its ordinarily scheduled timeslices, if any.)
              If the destination thread is not ready or resides on a different processor, the system call operates
              as described for dest = nilthread.
   pagefaults:
     none
*/

L4_Word_t L4_ThreadSwitch(L4_ThreadId_t dest)
{
	L4_Word_t result;
	L4_Word_t except;

	except = switch_thread(dest.raw);
	if (!except)
		result = L4_True;
	else
		result = L4_False;

	return result;
}

/* L4_Schedule : set scheduler syscall function
   The system call can be used by schedulers to define the priority, timeslice length, and other scheduling parameters of
   threads. Furthermore, it delivers thread states.
   The system call is only effective if the calling thread resides in the same address space as the destination thread’s
   scheduler (see thread control, page 23).
   
   [Systemcall]
   output:
     1. result(return)
     	0 - Error. The operation failed completely. The ErrorCode TCR indicates the reason for the failure.
     	1 - Dead. The thread is unable to execute or does not exist.
     	2 - Inactive. The thread is inactive/stopped.
     	3 - Running. The thread is ready to execute at user-level.
     	4 - Pending send. A user-invoked IPC send operation currently waits for the destination (recipient)
			to become ready to receive.
     	5 - Sending. A user-invoked IPC send operation currently transfers an outgoing message.
     	6 - Waiting to receive. A user-invoked IPC receive operation currently waits for an incoming message.
     	7 - Receiving. A user-invoked IPC receive operation currently receives an incoming message
     2. old_TimeControl
   input:
     1. dest
     2. TimeControl
     3. ProcessorControl
     4. PrioControl
     5. PreemptionControl
     
   pagefaults:
     none

   ErrorCode:
     1 2 5
*/


L4_Word_t L4_Schedule(L4_ThreadId_t dest,
   L4_Word_t time_control,
   L4_Word_t processor_control,
   L4_Word_t prior_control,
   L4_Word_t preemption_control,
   L4_Word_t *old_time_control)
{
	L4_Word_t result;
	L4_Word_t except;

	except = schedule_control(dest.raw, time_control, processor_control, 
		prior_control, preemption_control);
	
	old_time_control = L4_NULL;

	if (!except)
		result = L4_True;
	else
		result = L4_False;

	return result;
}

/*On both 32-bit and 64-bit processors, the system clock is represented as a 64-bit unsigned counter.
  The clock measures time in 1 us units, independent of the processor frequency. Although the clock 
  base is undefined, it is guaranteed that the counter will not overflow for at least 1,000 years.*/
static inline L4_Clock_t L4_ClockAddUsec(const L4_Clock_t l, 
	const L4_U64_t r)
{
  	return (L4_Clock_t) { raw : l.raw + r };
}

static inline L4_Clock_t L4_ClockSubUsec(const L4_Clock_t l, 
	const L4_U64_t r)
{
  	return (L4_Clock_t) { raw : l.raw - r };
}

static inline L4_Bool_t L4_IsClockEarlier(const L4_Clock_t l,
	const L4_Clock_t r)
{
  	return l.raw < r.raw;
}

static inline L4_Bool_t L4_IsClockLater(const L4_Clock_t l,
	const L4_Clock_t r)
{
  	return l.raw > r.raw;
}

static inline L4_Bool_t L4_IsClockEqual(const L4_Clock_t l,
	const L4_Clock_t r)
{
  	return l.raw == r.raw;
}

static inline L4_Bool_t L4_IsClockNotEqual(const L4_Clock_t l,
	const L4_Clock_t r)
{
  	return l.raw != r.raw;
}

static inline L4_Time_t L4_TimePeriod(L4_U64_t microseconds)
{
#define L4_SET_TIMEPERIOD(exp, man) \
	do { time.period.m = man; \
		 time.period.e = exp; \
	} while (0)
#define L4_TRY_EXPONENT(N) \
	else if (microseconds < (1UL << N)) \
	L4_SET_TIMEPERIOD (N - 10, microseconds >> (N - 10))

	L4_Time_t time;
	time.raw = 0;

	if (__builtin_constant_p(microseconds)) 
	{
		if (0) 
		{

		}
		L4_TRY_EXPONENT (10); L4_TRY_EXPONENT(11);
		L4_TRY_EXPONENT (12); L4_TRY_EXPONENT(13);
		L4_TRY_EXPONENT (14); L4_TRY_EXPONENT(15);
		L4_TRY_EXPONENT (16); L4_TRY_EXPONENT(17);
		L4_TRY_EXPONENT (18); L4_TRY_EXPONENT(19);
		L4_TRY_EXPONENT (20); L4_TRY_EXPONENT(21);
		L4_TRY_EXPONENT (22); L4_TRY_EXPONENT(23);
		L4_TRY_EXPONENT (24); L4_TRY_EXPONENT(25);
		L4_TRY_EXPONENT (26); L4_TRY_EXPONENT(27);
		L4_TRY_EXPONENT (28); L4_TRY_EXPONENT(29);
		L4_TRY_EXPONENT (30); L4_TRY_EXPONENT(31);
		else
		{
			return L4_NEVER;
		}
	}
	else 
	{
		L4_Word_t l4_exp = 0;
		L4_Word_t man    = microseconds;
		while (man >= (1 << 10)) 
		{
		  	man >>= 1;
		  	l4_exp++;
		}
		if (l4_exp <= 31)
		{
		  	L4_SET_TIMEPERIOD(l4_exp, man);
		}
		else
		{
		  	return L4_NEVER;
		}
	}
	return time;
#undef L4_TRY_EXPONENT
#undef L4_SET_TIMEPERIOD
}

static inline L4_Time_t L4_TimePoint(L4_Clock_t at)
{
#define L4_SET_TIMEPOINT(exp, man, carry) \
	do { time.point.m = man; \
	       time.point.e = exp; \
	       time.point.c = carry; \
	       time.point.a = 1; \
	     } while (0)
#define L4_TRY_EXPONENT(N) \
	else if (microseconds < (1 << (10+N))) \
	  L4_SET_TIMEPOINT (N, now.raw >> N, (now.raw >> (N+10)) & 0x1)

	L4_Clock_t now = L4_SystemClock();
	L4_Word_t  microseconds = now.raw - at.raw;
	L4_Time_t  time;

	if (__builtin_constant_p (microseconds)) 
	{
		if (0) 
		{

		}
		L4_TRY_EXPONENT (0);  L4_TRY_EXPONENT (1);
		L4_TRY_EXPONENT (2);  L4_TRY_EXPONENT (3);
		L4_TRY_EXPONENT (4);  L4_TRY_EXPONENT (5);
		L4_TRY_EXPONENT (6);  L4_TRY_EXPONENT (7);
		L4_TRY_EXPONENT (8);  L4_TRY_EXPONENT (9);
		L4_TRY_EXPONENT (10); L4_TRY_EXPONENT (11);
		L4_TRY_EXPONENT (12); L4_TRY_EXPONENT (13);
		L4_TRY_EXPONENT (14); L4_TRY_EXPONENT (15);
		else
		{
	 		return L4_NEVER;
		}
	} 
	else 
	{
		L4_Word_t exp = 0;
		L4_Word_t man = microseconds;
		
		while (man >= (1 << 10)) 
		{
		  	man >>= 1;
		  	exp++;
		}
		
		if (exp <= 15)
		{
		  L4_SET_TIMEPOINT (exp, man, (now.raw >> (exp + 10)) & 0x1);
		}
		else
		{
		  return L4_NEVER;
		}
	}
	return time;
#undef L4_TRY_EXPONENT
#undef L4_SET_TIMEPOINT
}

static inline void L4_Yield(void)
{
  	L4_ThreadSwitch(L4_NILTHREAD);
}

static inline L4_Word_t L4_Set_Priority(L4_ThreadId_t tid, 
	L4_Word_t prio)
{
	L4_Word_t dummy;

	prio &= 0xff;

	return L4_Schedule(tid, ~0UL, ~0UL, prio, ~0UL, &dummy);
}

static inline L4_Word_t L4_Set_Stride(L4_ThreadId_t tid,
	L4_Word_t stride)
{
	L4_Word_t dummy;

	return L4_Schedule(tid, ~0UL, ~0UL, (stride << 16) | 0xffff, ~0UL, &dummy);
}

static inline L4_Word_t L4_Set_ProcessorNo(L4_ThreadId_t tid, 
	L4_Word_t cpu_no)
{
	L4_Word_t dummy;

	cpu_no &= 0xffff;

	return L4_Schedule(tid, ~0UL, cpu_no, ~0UL, ~0UL, &dummy);
}

static inline L4_Word_t L4_Set_Timeslice(L4_ThreadId_t tid, 
	L4_Time_t timeslice, 
	L4_Time_t totalquantum)
{
	L4_Word_t timectrl = (timeslice.raw << 16) | totalquantum.raw;

	return L4_Schedule(tid, timectrl, ~0UL, ~0UL, ~0UL, &timectrl);
}

static inline L4_Word_t L4_Timeslice(L4_ThreadId_t tid, 
	L4_Time_t *timeslice, 
	L4_Time_t *totalquantum)
{
	L4_Word_t res, timectrl;

	res 				= L4_Schedule(tid, ~0UL, ~0UL, ~0UL, ~0UL, &timectrl);
	timeslice->raw 		= timectrl >> 16;
	totalquantum->raw 	= timectrl;

	return res;
}

static inline L4_Word_t L4_Set_PreemptionDelay(L4_ThreadId_t tid, 
	L4_Word_t sensitivePrio, 
	L4_Word_t maxDelay)
{
	L4_Word_t dummy;

	L4_Word_t pctrl = ((sensitivePrio & 0xff) << 16) | (maxDelay & 0xffff);

	return L4_Schedule(tid, ~0UL, ~0UL, ~0UL, pctrl, &dummy);
}


#endif
