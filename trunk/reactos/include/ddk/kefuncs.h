
#ifndef __INCLUDE_DDK_KEFUNCS_H
#define __INCLUDE_DDK_KEFUNCS_H

#define KEBUGCHECK(a) DbgPrint("KeBugCheck at %s:%i\n",__FILE__,__LINE__), KeBugCheck(a)

/* KERNEL FUNCTIONS ********************************************************/

NTSTATUS
STDCALL
KeRestoreFloatingPointState(
  IN PKFLOATING_SAVE  FloatSave);

NTSTATUS
STDCALL
KeSaveFloatingPointState(
  OUT PKFLOATING_SAVE  FloatSave);

#ifndef KeFlushIoBuffers
#define KeFlushIoBuffers(Mdl, ReadOperation, DmaOperation)
#endif

VOID STDCALL KeAttachProcess(struct _KPROCESS *Process);

VOID FASTCALL KiAcquireSpinLock(PKSPIN_LOCK SpinLock);

VOID FASTCALL KiReleaseSpinLock(PKSPIN_LOCK SpinLock);

VOID KeDrainApcQueue(VOID);

struct _KPROCESS* KeGetCurrentProcess(VOID);

/*
 * FUNCTION: Acquires a spinlock so the caller can synchronize access to 
 * data
 * ARGUMENTS:
 *         SpinLock = Initialized spinlock
 *         OldIrql (OUT) = Set the previous irql on return 
 */
VOID STDCALL KeAcquireSpinLock (PKSPIN_LOCK	SpinLock,
				PKIRQL		OldIrql);

#ifndef __USE_W32API

static __inline
VOID
KeMemoryBarrier(
  VOID)
{
  volatile LONG Barrier;
  __asm__ __volatile__ ("xchg %%eax, %0" : : "m" (Barrier) : "%eax");
}

VOID STDCALL KeAcquireSpinLockAtDpcLevel (IN PKSPIN_LOCK	SpinLock);

#define KefAcquireSpinLockAtDpcLevel KeAcquireSpinLockAtDpcLevel

VOID
STDCALL
KeReleaseSpinLockFromDpcLevel(
  IN PKSPIN_LOCK  SpinLock);

#endif
  
/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 * RETURNS: Doesn't
 *
 * NOTES - please use the macro KEBUGCHECK with the same argument so the end-user
 * knows what file/line number where the bug check occured
 */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode);

/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 *           BugCheckParameter[1-4] = Additional information about bug
 * RETURNS: Doesn't
 *
 * NOTES - please use the macro KEBUGCHECKEX with the same arguments so the end-user
 * knows what file/line number where the bug check occured
 */
VOID STDCALL KeBugCheckEx (ULONG	BugCheckCode,
			   ULONG	BugCheckParameter1,
			   ULONG	BugCheckParameter2,
			   ULONG	BugCheckParameter3,
			   ULONG	BugCheckParameter4);

BOOLEAN STDCALL KeCancelTimer (PKTIMER	Timer);

VOID STDCALL KeClearEvent (PKEVENT	Event);

BOOLEAN STDCALL KeConnectInterrupt(PKINTERRUPT InterruptObject);

NTSTATUS STDCALL KeDelayExecutionThread (KPROCESSOR_MODE	WaitMode,
					 BOOLEAN		Alertable,
					 PLARGE_INTEGER	Internal);

BOOLEAN STDCALL KeDeregisterBugCheckCallback (
		       PKBUGCHECK_CALLBACK_RECORD	CallbackRecord);

VOID STDCALL KeDetachProcess (VOID);

VOID STDCALL KeDisconnectInterrupt(PKINTERRUPT InterruptObject);

VOID STDCALL KeEnterCriticalRegion (VOID);

/*
 * FUNCTION: Enters the kernel debugger
 * ARGUMENTS:
 *	None
 */
VOID STDCALL KeEnterKernelDebugger (VOID);

KIRQL STDCALL KeGetCurrentIrql (VOID);

#ifndef __USE_W32API
#define KeGetCurrentProcessorNumber() (KeGetCurrentKPCR()->ProcessorNumber)
ULONG KeGetDcacheFillSize(VOID);
KPROCESSOR_MODE STDCALL KeGetPreviousMode (VOID);
#endif

struct _KTHREAD* STDCALL KeGetCurrentThread (VOID);

VOID STDCALL KeInitializeApc (IN PKAPC  Apc,
	IN PKTHREAD  Thread,
  IN KAPC_ENVIRONMENT TargetEnvironment,
	IN PKKERNEL_ROUTINE  KernelRoutine,
	IN PKRUNDOWN_ROUTINE  RundownRoutine,
	IN PKNORMAL_ROUTINE  NormalRoutine,
  IN KPROCESSOR_MODE  Mode,
	IN PVOID  Context);


/*
 * VOID
 * KeInitializeCallbackRecord (
 *      PKBUGCHECK_CALLBACK_RECORD CallbackRecord
 *      );
 */
#ifndef KeInitializeCallbackRecord
#define KeInitializeCallbackRecord(CallbackRecord) \
	(CallbackRecord)->State = BufferEmpty
#endif

VOID STDCALL KeInitializeDeviceQueue (PKDEVICE_QUEUE	DeviceQueue);

VOID STDCALL KeInitializeDpc (PKDPC			Dpc,
			      PKDEFERRED_ROUTINE	DeferredRoutine,
			      PVOID			DeferredContext);

VOID STDCALL KeInitializeEvent (PKEVENT		Event,
				EVENT_TYPE	Type,
				BOOLEAN		State);

VOID STDCALL KeInitializeInterrupt(PKINTERRUPT InterruptObject,
				   PKSERVICE_ROUTINE ServiceRoutine,
				   PVOID ServiceContext,
				   PKSPIN_LOCK SpinLock,
				   ULONG Vector,
				   KIRQL Irql,
				   KIRQL SynchronizeIrql,
				   KINTERRUPT_MODE InterruptMode,
				   BOOLEAN ShareVector,
				   CHAR ProcessorNumber,
				   BOOLEAN FloatingSave);

VOID STDCALL KeInitializeMutant(IN PKMUTANT Mutant,
				IN BOOLEAN InitialOwner);

VOID STDCALL KeInitializeMutex (PKMUTEX	Mutex,
				ULONG	Level);

VOID STDCALL
KeInitializeQueue(IN PKQUEUE Queue,
		  IN ULONG Count);

PLIST_ENTRY STDCALL
KeRundownQueue(IN PKQUEUE Queue);

VOID STDCALL KeInitializeSemaphore (PKSEMAPHORE	Semaphore,
				    LONG		Count,
				    LONG		Limit);

/*
 * FUNCTION: Initializes a spinlock
 * ARGUMENTS:
 *        SpinLock = Spinlock to initialize
 */
VOID STDCALL KeInitializeSpinLock (PKSPIN_LOCK	SpinLock);

VOID STDCALL KeInitializeTimer (PKTIMER	Timer);

VOID STDCALL KeInitializeTimerEx (PKTIMER		Timer,
				  TIMER_TYPE	Type);

BOOLEAN STDCALL KeInsertByKeyDeviceQueue (PKDEVICE_QUEUE DeviceQueue,
					  PKDEVICE_QUEUE_ENTRY	QueueEntry,
					  ULONG			SortKey);

BOOLEAN STDCALL KeInsertDeviceQueue (PKDEVICE_QUEUE		DeviceQueue,
				     PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);

LONG STDCALL
KeInsertHeadQueue(IN PKQUEUE Queue,
		  IN PLIST_ENTRY Entry);

LONG STDCALL
KeInsertQueue(IN PKQUEUE Queue,
	      IN PLIST_ENTRY Entry);

BOOLEAN STDCALL KeInsertQueueApc (PKAPC	Apc,
			       PVOID	SystemArgument1,
			       PVOID	SystemArgument2,
                KPRIORITY PriorityBoost);

BOOLEAN STDCALL KeInsertQueueDpc (PKDPC	Dpc,
				  PVOID	SystemArgument1,
				  PVOID	SystemArgument2);

VOID STDCALL KeLeaveCriticalRegion (VOID);

VOID STDCALL KeLowerIrql (KIRQL	NewIrql);

LONG STDCALL KePulseEvent (PKEVENT		Event,
			   KPRIORITY	Increment,
			   BOOLEAN		Wait);

LARGE_INTEGER
STDCALL
KeQueryPerformanceCounter (
	PLARGE_INTEGER	PerformanceFrequency
	);

VOID
STDCALL
KeQuerySystemTime (
	PLARGE_INTEGER	CurrentTime
	);

VOID
STDCALL
KeQueryTickCount (
	PLARGE_INTEGER	TickCount
	);

ULONG
STDCALL
KeQueryTimeIncrement (
	VOID
	);

ULONGLONG 
STDCALL
KeQueryInterruptTime(
    VOID
    );
            
VOID
STDCALL
KeRaiseIrql (
	KIRQL	NewIrql,
	PKIRQL	OldIrql
	);

KIRQL
STDCALL
KeRaiseIrqlToDpcLevel (
	VOID
	);

LONG
STDCALL
KeReadStateEvent (
	PKEVENT	Event
	);

LONG STDCALL
KeReadStateMutant(IN PKMUTANT Mutant);

LONG STDCALL
KeReadStateMutex(IN PKMUTEX Mutex);

LONG STDCALL
KeReadStateQueue(IN PKQUEUE Queue);

LONG STDCALL
KeReadStateSemaphore(IN PKSEMAPHORE Semaphore);

BOOLEAN STDCALL
KeReadStateTimer(IN PKTIMER Timer);

BOOLEAN
STDCALL
KeRegisterBugCheckCallback (
	PKBUGCHECK_CALLBACK_RECORD	CallbackRecord,
	PKBUGCHECK_CALLBACK_ROUTINE	CallbackRoutine,
	PVOID				Buffer,
	ULONG				Length,
	PUCHAR				Component
	);

LONG
STDCALL
KeReleaseMutant(
	IN PKMUTANT Mutant,
	IN KPRIORITY Increment,
	IN BOOLEAN Abandon,
	IN BOOLEAN Wait
	);

LONG
STDCALL
KeReleaseMutex (
	PKMUTEX	Mutex,
	BOOLEAN	Wait
	);

LONG
STDCALL
KeReleaseSemaphore (
	PKSEMAPHORE	Semaphore,
	KPRIORITY	Increment,
	LONG		Adjustment,
	BOOLEAN		Wait
	);

VOID
STDCALL
KeReleaseSpinLock (
	PKSPIN_LOCK	Spinlock,
	KIRQL		NewIrql
	);

#ifndef __USE_W32API
VOID
STDCALL
KeReleaseSpinLockFromDpcLevel (
	PKSPIN_LOCK	Spinlock
	);
#endif

PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveByKeyDeviceQueue (
	PKDEVICE_QUEUE	DeviceQueue,
	ULONG		SortKey
	);

PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveDeviceQueue (
	PKDEVICE_QUEUE	DeviceQueue
	);

BOOLEAN STDCALL
KeRemoveEntryDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
			 PKDEVICE_QUEUE_ENTRY DeviceQueueEntry);

PLIST_ENTRY STDCALL
KeRemoveQueue(IN PKQUEUE Queue,
	      IN KPROCESSOR_MODE WaitMode,
	      IN PLARGE_INTEGER Timeout OPTIONAL);

BOOLEAN STDCALL
KeRemoveQueueDpc(IN PKDPC Dpc);

LONG STDCALL
KeResetEvent(IN PKEVENT Event);

VOID STDCALL
KeRosDumpStackFrames ( PULONG Frame, ULONG FrameCount );

ULONG STDCALL
KeRosGetStackFrames ( PULONG Frames, ULONG FrameCount );

BOOLEAN STDCALL
KeRosPrintAddress(PVOID address);

NTSTATUS STDCALL
KeSetAffinityThread(PKTHREAD	Thread,
		    KAFFINITY	Affinity);

LONG STDCALL
KeSetBasePriorityThread(struct _KTHREAD* Thread,
			LONG Increment);

LONG
STDCALL
KeSetEvent (
	PKEVENT		Event,
	KPRIORITY	Increment,
	BOOLEAN		Wait
	);

KPRIORITY STDCALL KeSetPriorityThread (struct _KTHREAD*	Thread,
				       KPRIORITY	Priority);

BOOLEAN STDCALL KeSetTimer (PKTIMER		Timer,
			    LARGE_INTEGER	DueTime,
			    PKDPC		Dpc);

BOOLEAN STDCALL KeSetTimerEx (PKTIMER		Timer,
			      LARGE_INTEGER	DueTime,
			      LONG		Period,
			      PKDPC		Dpc);

VOID STDCALL KeStallExecutionProcessor (ULONG	MicroSeconds);

BOOLEAN STDCALL KeSynchronizeExecution (PKINTERRUPT		Interrupt,
					PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
					PVOID SynchronizeContext);

NTSTATUS STDCALL KeWaitForMultipleObjects (ULONG		Count,
					   PVOID		Object[],
					   WAIT_TYPE	WaitType,
					   KWAIT_REASON	WaitReason,
					   KPROCESSOR_MODE	WaitMode,
					   BOOLEAN		Alertable,
					   PLARGE_INTEGER	Timeout,
					   PKWAIT_BLOCK	WaitBlockArray);

NTSTATUS
STDCALL
KeWaitForMutexObject (
	PKMUTEX		Mutex,
	KWAIT_REASON	WaitReason,
	KPROCESSOR_MODE	WaitMode,
	BOOLEAN		Alertable,
	PLARGE_INTEGER	Timeout
	);

NTSTATUS
STDCALL
KeWaitForSingleObject (
	PVOID		Object,
	KWAIT_REASON	WaitReason,
	KPROCESSOR_MODE	WaitMode,
	BOOLEAN		Alertable,
	PLARGE_INTEGER	Timeout
	);



/* io permission map has a 8k size
 * Each bit in the IOPM corresponds to an io port byte address. The bitmap
 * is initialized to allow IO at any port. [ all bits set ]. 
 */
typedef struct _IOPM
{
	UCHAR Bitmap[8192];
} IOPM, *PIOPM;

/*
 * FUNCTION: Provides the kernel with a new access map for a driver
 * ARGUMENTS:
 * 	NewMap: =  If FALSE the kernel's map is set to all disabled. If TRUE
 *			the kernel disables access to a particular port.
 *	IoPortMap = Caller supplies storage for the io permission map.
 * REMARKS
 *	Each bit in the IOPM corresponds to an io port byte address. The bitmap
 *	is initialized to allow IO at any port. [ all bits set ]. The IOPL determines
 *	the minium privilege level required to perform IO prior to checking the permission map.
 */
BOOL STDCALL
Ke386SetIoAccessMap(ULONG NewMap, PULONG IoPermissionMap);

/*
 * FUNCTION: Queries the io permission  map.
 * ARGUMENTS:
 * 	NewMap: =  If FALSE the kernel's map is set to all disabled. If TRUE
 *			the kernel disables access to a particular port.
 *	IoPortMap = Caller supplies storage for the io permission map.
 * REMARKS
 *	Each bit in the IOPM corresponds to an io port byte address. The bitmap
 *	is initialized to allow IO at any port. [ all bits set ]. The IOPL determines
 *	the minium privilege level required to perform IO prior to checking the permission map.
 */
BOOL STDCALL
Ke386QueryIoAccessMap(ULONG NewMap, PULONG IoPermissionMap);

/*
 * FUNCTION: Set the process IOPL
 * ARGUMENTS:
 *	Eprocess = Pointer to a executive process object
 *	EnableIo = Specify TRUE to enable IO and FALSE to disable 
 */
BOOL STDCALL
Ke386IoSetAccessProcess(struct _EPROCESS* Eprocess, BOOL EnableIo);

/*
 * FUNCTION: Sets the contents of a gdt descriptor.
 * ARGUMENTS:
 *     Entry = The selector to set.
 *     Value1 = The value of the low dword of the descriptor.
 *     Value2 = The value of the high dword of the descriptor.
 */
VOID
KeSetGdtSelector(
	ULONG Entry,
	ULONG Value1,
	ULONG Value2
);

/*
 * FUNCTION: Releases a set of Global Descriptor Table Selectors
 * ARGUMENTS:
 *	SelArray = 
 *	NumOfSelectors = 
 */
NTSTATUS 
KeI386ReleaseGdtSelectors(
	OUT PULONG SelArray,
	IN ULONG NumOfSelectors
);

/*
 * FUNCTION: Allocates a set of Global Descriptor Table Selectors
 * ARGUMENTS:
 *	SelArray = 
 *	NumOfSelectors = 
 */
NTSTATUS
KeI386AllocateGdtSelectors(
	OUT PULONG SelArray,
    IN ULONG NumOfSelectors
);


KIRQL
FASTCALL
KfAcquireSpinLock (
	IN	PKSPIN_LOCK	SpinLock
	);

VOID
FASTCALL
KfLowerIrql (
	IN	KIRQL	NewIrql
	);


KIRQL
FASTCALL
KfRaiseIrql (
	IN	KIRQL	NewIrql
	);

VOID
FASTCALL
KfReleaseSpinLock (
	IN	PKSPIN_LOCK	SpinLock,
	IN	KIRQL		NewIrql
	);


VOID STDCALL KiDispatchInterrupt(VOID);

/* Stubs Start here */

VOID
STDCALL
KeReleaseInterruptSpinLock(
	IN PKINTERRUPT Interrupt,
	IN KIRQL OldIrql
	);

BOOLEAN
STDCALL
KeAreApcsDisabled(
	VOID
	);

VOID
STDCALL
KeFlushQueuedDpcs(
	VOID
	);

ULONG
STDCALL
KeGetRecommendedSharedDataAlignment(
	VOID
	);

ULONG
STDCALL
KeQueryRuntimeThread(
	IN PKTHREAD Thread,
	OUT PULONG UserTime
	);    

BOOLEAN
STDCALL
KeSetKernelStackSwapEnable(
	IN BOOLEAN Enable
	);

BOOLEAN
STDCALL
KeDeregisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord
    );

BOOLEAN
STDCALL
KeRegisterBugCheckReasonCallback(
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PUCHAR Component
    );

VOID 
STDCALL
KeTerminateThread(
	IN KPRIORITY   	 Increment  	 
);

BOOLEAN
STDCALL
KeIsExecutingDpc(
	VOID
);

VOID
STDCALL
KeSetEventBoostPriority(
	IN PKEVENT Event,
	IN PKTHREAD *Thread OPTIONAL
);

PVOID
STDCALL
KeFindConfigurationEntry(
    IN PVOID Unknown,
    IN ULONG Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG RegKey
);

PVOID
STDCALL
KeFindConfigurationNextEntry(
    IN PVOID Unknown,
    IN ULONG Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG RegKey,
    IN PVOID *NextLink
);

VOID
STDCALL
KeFlushEntireTb(
    IN BOOLEAN Unknown,
    IN BOOLEAN CurrentCpuOnly
);

VOID
STDCALL
KeRevertToUserAffinityThread(
    VOID
);

VOID
STDCALL
KiCoprocessorError(
    VOID
);

VOID
STDCALL
KiUnexpectedInterrupt(
    VOID
);

VOID
STDCALL
KeSetDmaIoCoherency(
    IN ULONG Coherency
);

VOID
STDCALL
KeSetProfileIrql(
    IN KIRQL ProfileIrql
);

VOID
STDCALL
KeSetSystemAffinityThread(
    IN KAFFINITY Affinity
);

NTSTATUS
STDCALL
KeUserModeCallback(
    IN ULONG	FunctionID,
    IN PVOID	InputBuffer,
    IN ULONG	InputLength,
    OUT PVOID	*OutputBuffer,
    OUT PULONG	OutputLength
);

VOID
STDCALL
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement
);

VOID
STDCALL
KeCapturePersistentThreadState(
	IN PVOID	CurrentThread,
	IN ULONG	Setting1,
	IN ULONG	Setting2,
	IN ULONG	Setting3,
	IN ULONG	Setting4,
	IN ULONG	Setting5,
	IN PVOID	ThreadState
);

BOOLEAN
STDCALL
KeRemoveSystemServiceTable(
    IN ULONG TableIndex
);

NTSTATUS
KeI386FlatToGdtSelector(
	IN ULONG	Base,
	IN USHORT	Length,
	IN USHORT	Selector
);

CCHAR
STDCALL
KeSetIdealProcessorThread (
	IN PKTHREAD Thread,
	IN CCHAR Processor
	);

typedef
VOID
(FASTCALL *PTIME_UPDATE_NOTIFY_ROUTINE)(
    IN HANDLE ThreadId,
    IN KPROCESSOR_MODE Mode
    );

VOID
FASTCALL
KeSetTimeUpdateNotifyRoutine(
    IN PTIME_UPDATE_NOTIFY_ROUTINE NotifyRoutine
    );

PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveByKeyDeviceQueueIfBusy (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
    );

KAFFINITY
STDCALL
KeQueryActiveProcessors (
    VOID
    );

VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    );

VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel(
    IN PKLOCK_QUEUE_HANDLE LockHandle
    );

KPRIORITY
STDCALL
KeQueryPriorityThread (
    IN PKTHREAD Thread
    );  

KIRQL
STDCALL
KeAcquireInterruptSpinLock(
    IN PKINTERRUPT Interrupt
    );

VOID
__cdecl
KeSaveStateForHibernate(
    IN PVOID State
);

NTSTATUS
STDCALL
KeRaiseUserException(
	IN NTSTATUS	ExceptionCode
);

#endif /* __INCLUDE_DDK_KEFUNCS_H */
