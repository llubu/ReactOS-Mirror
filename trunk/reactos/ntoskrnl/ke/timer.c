/*
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ke/timer.c
 * PURPOSE:        Handle timers
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 28/05/98: Created
 */

/* NOTES ******************************************************************/
/*
 * System time units are 100-nanosecond intervals
 */

/* INCLUDES ***************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *****************************************************************/

#define TIMER_IRQ 0


/* GLOBALS ****************************************************************/

/*
 * Current time
 */
static unsigned long long system_time = 0;

/*
 * Number of timer interrupts since initialisation
 */
static volatile unsigned long long ticks=0;

/*
 * The increment in the system clock every timer tick (in system time units)
 * 
 * = (1/18.2)*10^9 
 * 
 */
#define CLOCK_INCREMENT (54945055)

/*
 * PURPOSE: List of timers
 */
static LIST_ENTRY timer_list_head = {NULL,NULL};
static KSPIN_LOCK timer_list_lock = {0,};



#define MICROSECONDS_PER_TICK (54945)
#define TICKS_TO_CALIBRATE (1)
#define CALIBRATE_PERIOD (MICROSECONDS_PER_TICK * TICKS_TO_CALIBRATE)

static unsigned int loops_per_microsecond = 100;

/* FUNCTIONS **************************************************************/

void KeCalibrateTimerLoop()
{
   unsigned int start_tick;
   unsigned int end_tick;
   unsigned int nr_ticks;
   unsigned int i;
   unsigned int microseconds;
   
   for (i=0;i<20;i++)
     {
   
	start_tick = ticks;
        microseconds = 0;
        while (start_tick == ticks);
        while (ticks == (start_tick+TICKS_TO_CALIBRATE))
        {
                KeStallExecutionProcessor(1);
                microseconds++;
        };

//        DbgPrint("microseconds %d\n",microseconds);

        if (microseconds > (CALIBRATE_PERIOD+1000))
        {
           loops_per_microsecond = loops_per_microsecond + 1;
        }
        if (microseconds < (CALIBRATE_PERIOD-1000))
        {
           loops_per_microsecond = loops_per_microsecond - 1;
        }
//        DbgPrint("loops_per_microsecond %d\n",loops_per_microsecond);
     }
//     for(;;);
}


NTSTATUS STDCALL NtQueryTimerResolution (OUT PULONG MinimumResolution,
					 OUT PULONG MaximumResolution, 
					 OUT PULONG ActualResolution)
{
   return(ZwQueryTimerResolution(MinimumResolution,MaximumResolution,
				 ActualResolution));
}

NTSTATUS STDCALL ZwQueryTimerResolution (OUT PULONG MinimumResolution,
					 OUT PULONG MaximumResolution, 
					 OUT PULONG ActualResolution)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSetTimerResolution(IN ULONG RequestedResolution,
				      IN BOOL SetOrUnset,
				      OUT PULONG ActualResolution)
{
   return(ZwSetTimerResolution(RequestedResolution,
			       SetOrUnset,
			       ActualResolution));
}

NTSTATUS STDCALL ZwSetTimerResolution(IN ULONG RequestedResolution,
				      IN BOOL SetOrUnset,
				      OUT PULONG ActualResolution)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryPerformanceCounter(IN PLARGE_INTEGER Counter,
					   IN PLARGE_INTEGER Frequency)
{
   return(ZwQueryPerformanceCounter(Counter,
				    Frequency));
}

NTSTATUS STDCALL ZwQueryPerformanceCounter(IN PLARGE_INTEGER Counter,
					   IN PLARGE_INTEGER Frequency)
{
   UNIMPLEMENTED;
}


NTSTATUS KeAddThreadTimeout(PKTHREAD Thread, PLARGE_INTEGER Interval)
{
   KeInitializeTimer(&(Thread->TimerBlock));
   KeSetTimer(&(Thread->TimerBlock),*Interval,NULL);
}


NTSTATUS STDCALL NtDelayExecution(IN BOOLEAN Alertable,
				  IN TIME *Interval)
{
   return(ZwDelayExecution(Alertable,Interval));
}

NTSTATUS STDCALL ZwDelayExecution(IN BOOLEAN Alertable,
				  IN TIME *Interval)
{
   UNIMPLEMENTED;
}

NTSTATUS KeDelayExecutionThread(KPROCESSOR_MODE WaitMode,
				BOOLEAN Alertable,
				PLARGE_INTEGER Interval)
/*
 * FUNCTION: Puts the current thread into an alertable or nonalertable 
 * wait state for a given internal
 * ARGUMENTS:
 *          WaitMode = Processor mode in which the caller is waiting
 *          Altertable = Specifies if the wait is alertable
 *          Interval = Specifies the interval to wait
 * RETURNS: Status
 */
{
   PKTHREAD CurrentThread = KeGetCurrentThread();
   KeAddThreadTimeout(CurrentThread,Interval);
   return(KeWaitForSingleObject(&(CurrentThread->TimerBlock),Executive,
				KernelMode,Alertable,NULL));
}

VOID KeStallExecutionProcessor(ULONG MicroSeconds)
{
   unsigned int i;
   for (i=0; i<(loops_per_microsecond*MicroSeconds) ;i++)
     {
	__asm__("nop\n\t");
     }
}

static inline void ULLToLargeInteger(unsigned long long src,
				     PLARGE_INTEGER dest)
{
   dest->LowPart = src & 0xffffffff;
   dest->HighPart = (src>>32);
}

static inline void SLLToLargeInteger(signed long long src,
				     PLARGE_INTEGER dest)
{
   if (src > 0)
     {
	dest->LowPart = src & 0xffffffff;
	dest->HighPart = (src>>32);
     }
   else
     {
	src = -src;
	dest->LowPart = src & 0xffffffff;
	dest->HighPart = -(src>>32);
     }
}

static inline signed long long LargeIntegerToSLL(PLARGE_INTEGER src)
{
   signed long long r;
   
   r = src->LowPart;
   if (src->HighPart >= 0)
     {
	r = r | (((unsigned long long)src->HighPart)<<32);
     }
   else
     {
	r = r | (((unsigned long long)(-(src->HighPart)))<<32);
	r = -r;
     }
   return(r);
}


LARGE_INTEGER KeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFreq)
/*
 * FUNCTION: Queries the finest grained running count avaiable in the system
 * ARGUMENTS:
 *         PerformanceFreq (OUT) = The routine stores the number of 
 *                                 performance counters tick per second here
 * RETURNS: The performance counter value in HERTZ
 * NOTE: Returns the system tick count or the time-stamp on the pentium
 */
{
   PerformanceFreq->HighPart=0;
   PerformanceFreq->LowPart=0;
}

ULONG KeQueryTimeIncrement(VOID)
/*
 * FUNCTION: Gets the increment (in 100-nanosecond units) that is added to 
 * the system clock every time the clock interrupts
 * RETURNS: The increment
 */
{
   return(CLOCK_INCREMENT);
}

VOID KeQuerySystemTime(PLARGE_INTEGER CurrentTime)
/*
 * FUNCTION: Gets the current system time
 * ARGUMENTS:
 *          CurrentTime (OUT) = The routine stores the current time here
 * NOTE: The time is the number of 100-nanosecond intervals since the
 * 1st of January, 1601.
 */
{
   ULLToLargeInteger(system_time,CurrentTime);
}

NTSTATUS STDCALL NtGetTickCount(PULONG UpTime)
{
   return(ZwGetTickCount(UpTime));
}

NTSTATUS STDCALL ZwGetTickCount(PULONG UpTime)
{
   UNIMPLEMENTED;
}

BOOLEAN KeSetTimer(PKTIMER Timer, LARGE_INTEGER DueTime, PKDPC Dpc)
/*
 * FUNCTION: Sets the absolute or relative interval at which a timer object
 * is to be set to the signaled state and optionally supplies a 
 * CustomTimerDpc to be executed when the timer expires.
 * ARGUMENTS:
 *          Timer = Points to a previously initialized timer object
 *          DueTimer = If positive then absolute time to expire at
 *                     If negative then the relative time to expire at
 *          Dpc = If non-NULL then a dpc to be called when the timer expires
 * RETURNS: True if the timer was already in the system timer queue
 *          False otherwise
 */
{
   return(KeSetTimerEx(Timer,DueTime,0,Dpc));
}

BOOLEAN KeSetTimerEx(PKTIMER Timer, LARGE_INTEGER DueTime, LONG Period,
		     PKDPC Dpc)
/*
 * FUNCTION: Sets the absolute or relative interval at which a timer object
 * is to be set to the signaled state and optionally supplies a 
 * CustomTimerDpc to be executed when the timer expires.
 * ARGUMENTS:
 *          Timer = Points to a previously initialized timer object
 *          DueTimer = If positive then absolute time to expire at
 *                     If negative then the relative time to expire at
 *          Dpc = If non-NULL then a dpc to be called when the timer expires
 * RETURNS: True if the timer was already in the system timer queue
 *          False otherwise
 */
{
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&timer_list_lock,&oldlvl);
   
   Timer->dpc=Dpc;
   Timer->period=Period;
   Timer->expire_time = LargeIntegerToSLL(&DueTime);
   if (Timer->expire_time < 0)
     {
	Timer->expire_time = system_time - Timer->expire_time;
     }
   Timer->signaled = FALSE;
   if (Timer->running)
     {
	KeReleaseSpinLock(&timer_list_lock,oldlvl);
	return(TRUE);	
     }
   InsertTailList(&timer_list_head,&Timer->entry);
   KeReleaseSpinLock(&timer_list_lock,oldlvl);
   return(FALSE);
}

BOOLEAN KeCancelTimer(PKTIMER Timer)
/*
 * FUNCTION: Removes a timer from the system timer list
 * ARGUMENTS:
 *       Timer = timer to cancel
 * RETURNS: True if the timer was running
 *          False otherwise
 */
{
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&timer_list_lock,&oldlvl);
		     
   if (!Timer->running)
     {
	return(FALSE);
     }
   RemoveEntryList(&Timer->entry);
   KeReleaseSpinLock(&timer_list_lock,oldlvl);
   return(TRUE);
}

BOOLEAN KeReadStateTimer(PKTIMER Timer)
{
   return(Timer->signaled);
}

VOID KeInitializeTimer(PKTIMER Timer)
/*
 * FUNCTION: Initalizes a kernel timer object
 * ARGUMENTS:
 *          Timer = caller supplied storage for the timer
 * NOTE: This function initializes a notification timer
 */
{
   KeInitializeTimerEx(Timer,NotificationTimer);
}

VOID KeInitializeTimerEx(PKTIMER Timer, TIMER_TYPE Type)
/*
 * FUNCTION: Initializes a kernel timer object
 * ARGUMENTS:
 *          Timer = caller supplied storage for the timer
 *          Type = the type of timer (notification or synchronization)
 * NOTE: When a notification type expires all waiting threads are released
 * and the timer remains signalled until it is explicitly reset. When a 
 * syncrhonization timer expires its state is set to signalled until a
 * single waiting thread is released and then the timer is reset.
 */
{
   Timer->running=FALSE;
   Timer->type=Type;
   Timer->signaled=FALSE;
}

VOID KeQueryTickCount(PLARGE_INTEGER TickCount)
/*
 * FUNCTION: Returns the number of ticks since the system was booted
 * ARGUMENTS:
 *         TickCount (OUT) = Points to storage for the number of ticks
 */
{
   ULLToLargeInteger(ticks,TickCount);
}

static void HandleExpiredTimer(PKTIMER current)
{
   if (current->dpc!=NULL)
     {
	current->dpc->DeferredRoutine(current->dpc,
				      current->dpc->DeferredContext,
				      current->dpc->SystemArgument1,
				      current->dpc->SystemArgument2);
     }
   current->signaled=TRUE;
   if (current->period !=0)
     {
	current->expire_time = current->expire_time + current->period;
     }
   else
     {
	RemoveEntryList(&current->entry);
	current->running=FALSE;
     }
}

void KeExpireTimers(void)
{
   PLIST_ENTRY current_entry = timer_list_head.Flink;
   PKTIMER current = CONTAINING_RECORD(current_entry,KTIMER,entry);
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&timer_list_lock,&oldlvl);
   
   while (current_entry!=(&timer_list_head))
     {
	if (system_time == current->expire_time)
	  {
	     HandleExpiredTimer(current);
	  }
	
	current_entry = current_entry->Flink;     
	current = CONTAINING_RECORD(current_entry,KTIMER,entry);
     }
   KeReleaseSpinLock(&timer_list_lock,oldlvl);
}

VOID KiTimerInterrupt(VOID)
/*
 * FUNCTION: Handles a timer interrupt
 */
{
   char str[16];
   char* vidmem=(char *)physical_to_linear(0xb8000 + 160 - 16);
   int i;
   
   /*
    * Increment the number of timers ticks 
    */
   ticks++;
   system_time = system_time + CLOCK_INCREMENT;
   
   /*
    * Display the tick count in the top left of the screen as a debugging
    * aid
    */
   sprintf(str,"%.8u",ticks);
   for (i=0;i<8;i++)
     {
	*vidmem=str[i];
	vidmem++;
	*vidmem=0x7;
	vidmem++;
     }
   
   return(TRUE);
}


void InitializeTimer(void)
/*
 * FUNCTION: Initializes timer irq handling
 * NOTE: This is only called once from main()
 */
{
   InitializeListHead(&timer_list_head);
   KeInitializeSpinLock(&timer_list_lock);
   
   /*
    * Calculate the starting time for the system clock
    */
}
