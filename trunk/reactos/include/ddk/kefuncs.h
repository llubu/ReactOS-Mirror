#ifndef __INCLUDE_DDK_KEFUNCS_H
#define __INCLUDE_DDK_KEFUNCS_H


/* KERNEL FUNCTIONS ********************************************************/

/*
 * FUNCTION: Acquires a spinlock so the caller can synchronize access to 
 * data
 * ARGUMENTS:
 *         SpinLock = Initialized spinlock
 *         OldIrql (OUT) = Set the previous irql on return 
 */
VOID
STDCALL
KeAcquireSpinLock (
	IN	PKSPIN_LOCK	SpinLock,
	OUT	PKIRQL		OldIrql
	);

VOID
STDCALL
KeAcquireSpinLockAtDpcLevel (
	IN	PKSPIN_LOCK	SpinLock
	);

KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch (
	IN	PKSPIN_LOCK	SpinLock
	);

VOID
STDCALL
KeAttachProcess (
	IN	PEPROCESS	Process
	);

/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 * RETURNS: Doesn't
 */
VOID
STDCALL
KeBugCheck (
	IN	ULONG	BugCheckCode
	);

/*
 * FUNCTION: Brings the system down in a controlled manner when an 
 * inconsistency that might otherwise cause corruption has been detected
 * ARGUMENTS:
 *           BugCheckCode = Specifies the reason for the bug check
 *           BugCheckParameter[1-4] = Additional information about bug
 * RETURNS: Doesn't
 */
VOID
STDCALL
KeBugCheckEx (
	IN	ULONG	BugCheckCode,
	IN	ULONG	BugCheckParameter1,
	IN	ULONG	BugCheckParameter2,
	IN	ULONG	BugCheckParameter3,
	IN	ULONG	BugCheckParameter4
	);

BOOLEAN
STDCALL
KeCancelTimer (
	IN	PKTIMER	Timer
	);

VOID
STDCALL
KeClearEvent (
	IN	PKEVENT	Event
	);

NTSTATUS
STDCALL
KeDelayExecutionThread (
	KPROCESSOR_MODE	WaitMode,
	BOOLEAN		Alertable,
	PLARGE_INTEGER	Internal
	);

BOOLEAN
STDCALL
KeDeregisterBugCheckCallback (
	IN	PKBUGCHECK_CALLBACK_RECORD	CallbackRecord
	);

VOID
STDCALL
KeDetachProcess (
	VOID
	);

VOID
STDCALL
KeEnterCriticalRegion (
	VOID
	);

/*
 * FUNCTION: Enters the kernel debugger
 * ARGUMENTS:
 *	None
 */
VOID
STDCALL
KeEnterKernelDebugger (
	VOID
	);

/*
 * VOID
 * KeFlushIoBuffers (
 *      PMDL    Mdl,
 *      BOOLEAN ReadOperation,
 *      BOOLEAN DmaOperation
 *      );
 */
#define KeFlushIoBuffers(Mdl,ReadOperation,DmaOperation)

VOID
STDCALL
KeFlushWriteBuffer (
	VOID
	);

KIRQL
STDCALL
KeGetCurrentIrql (
	VOID
	);

/*
 * ULONG
 * KeGetCurrentProcessorNumber(VOID);
 */
/*
 * FIXME: This should be an inline function on x86 systems
 */
#define KeGetCurrentProcessorNumber() 0UL

PKTHREAD
STDCALL
KeGetCurrentThread (
	VOID
	);

/*
 * ULONG KeGetDcacheFillSize(VOID);
 *
 * FUNCTION:
 *      Returns the microprocessor's data cache-line boundary in bytes
 */
#define KeGetDcacheFillSize() 1L

ULONG
STDCALL
KeGetPreviousMode (
	VOID
	);

VOID
STDCALL
KeInitializeApc (
	PKAPC			Apc,
	PKTHREAD		Thread,
	UCHAR			StateIndex,
	PKKERNEL_ROUTINE	KernelRoutine,
	PKRUNDOWN_ROUTINE	RundownRoutine,
	PKNORMAL_ROUTINE	NormalRoutine,
	UCHAR			Mode,
	PVOID			Context
	);

/*
 * VOID
 * KeInitializeCallbackRecord (
 *      PKBUGCHECK_CALLBACK_RECORD CallbackRecord
 *      );
 */
#define KeInitializeCallbackRecord(CallbackRecord) \
	(CallbackRecord)->State = BufferEmpty

VOID
STDCALL
KeInitializeDeviceQueue (
	PKDEVICE_QUEUE	DeviceQueue
	);

VOID
STDCALL
KeInitializeDpc (
	PKDPC			Dpc,
	PKDEFERRED_ROUTINE	DeferredRoutine,
	PVOID			DeferredContext
	);

VOID
STDCALL
KeInitializeEvent (
	PKEVENT		Event,
	EVENT_TYPE	Type,
	BOOLEAN		State
	);

VOID
STDCALL
KeInitializeMutex (
	PKMUTEX	Mutex,
	ULONG	Level
	);

VOID
STDCALL
KeInitializeSemaphore (
	PKSEMAPHORE	Semaphore,
	LONG		Count,
	LONG		Limit
	);

/*
 * FUNCTION: Initializes a spinlock
 * ARGUMENTS:
 *        SpinLock = Spinlock to initialize
 */
VOID
STDCALL
KeInitializeSpinLock (
	PKSPIN_LOCK	SpinLock
	);

VOID
STDCALL
KeInitializeTimer (
	PKTIMER	Timer
	);

VOID
STDCALL
KeInitializeTimerEx (
	PKTIMER		Timer,
	TIMER_TYPE	Type
	);

BOOLEAN
STDCALL
KeInsertByKeyDeviceQueue (
	PKDEVICE_QUEUE		DeviceQueue,
	PKDEVICE_QUEUE_ENTRY	DeviceQueueEntry,
	ULONG			SortKey
	);

BOOLEAN
STDCALL
KeInsertDeviceQueue (
	PKDEVICE_QUEUE		DeviceQueue,
	PKDEVICE_QUEUE_ENTRY	DeviceQueueEntry
	);

VOID
STDCALL
KeInsertQueueApc (
	PKAPC	Apc,
	PVOID	SystemArgument1,
	PVOID	SystemArgument2,
	UCHAR	Mode
	);

BOOLEAN
STDCALL
KeInsertQueueDpc (
	PKDPC	Dpc,
	PVOID	SystemArgument1,
	PVOID	SystemArgument2
	);

VOID
STDCALL
KeLeaveCriticalRegion (
	VOID
	);

VOID
STDCALL
KeLowerIrql (
	KIRQL	NewIrql
	);

NTSTATUS
STDCALL
KePulseEvent (
	PKEVENT		Event,
	KPRIORITY	Increment,
	BOOLEAN		Wait
	);

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

VOID
STDCALL
KeRaiseIrql (
	KIRQL	NewIrql,
	PKIRQL	OldIrql
	);

/*
 * FUNCTION: Raises a user mode exception
 * ARGUMENTS:
 *	ExceptionCode = Status code of the exception
 */
VOID
STDCALL
KeRaiseUserException (
	IN	NTSTATUS	ExceptionCode
	);

LONG
STDCALL
KeReadStateEvent (
	PKEVENT	Event
	);

LONG
STDCALL
KeReadStateMutex (
	PKMUTEX	Mutex
	);

LONG
STDCALL
KeReadStateSemaphore (
	PKSEMAPHORE	Semaphore
	);

BOOLEAN
STDCALL
KeReadStateTimer (
	PKTIMER	Timer
	);

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

VOID
STDCALL
KeReleaseSpinLockFromDpcLevel (
	PKSPIN_LOCK	Spinlock
	);

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

BOOLEAN
STDCALL
KeRemoveEntryDeviceQueue (
	PKDEVICE_QUEUE		DeviceQueue,
	PKDEVICE_QUEUE_ENTRY	DeviceQueueEntry
	);

BOOLEAN
STDCALL
KeRemoveQueueDpc (
	PKDPC	Dpc
	);

LONG
STDCALL
KeResetEvent (
	PKEVENT	Event
	);

LONG
STDCALL
KeSetBasePriorityThread (
	PKTHREAD	Thread,
	LONG		Increment
	);

LONG
STDCALL
KeSetEvent (
	PKEVENT		Event,
	KPRIORITY	Increment,
	BOOLEAN		Wait
	);

VOID
STDCALL
KeSetImportanceDpc (
	IN	PKDPC		Dpc,
	IN	KDPC_IMPORTANCE	Importance
	);

KPRIORITY
STDCALL
KeSetPriorityThread (
	PKTHREAD	Thread,
	KPRIORITY	Priority
	);

VOID
STDCALL
KeSetTargetProcessorDpc (
	IN	PKDPC	Dpc,
	IN	CCHAR	Number
	);

BOOLEAN
STDCALL
KeSetTimer (
	PKTIMER		Timer,
	LARGE_INTEGER	DueTime,
	PKDPC		Dpc
	);

BOOLEAN
STDCALL
KeSetTimerEx (
	PKTIMER		Timer,
	LARGE_INTEGER	DueTime,
	LONG		Period,
	PKDPC		Dpc
	);

VOID
STDCALL
KeStallExecutionProcessor (
	ULONG	MicroSeconds
	);

BOOLEAN
STDCALL
KeSynchronizeExecution (
	PKINTERRUPT		Interrupt,
	PKSYNCHRONIZE_ROUTINE	SynchronizeRoutine,
	PVOID			SynchronizeContext
	);

VOID
STDCALL
KeUpdateSystemTime (
	VOID
	);

NTSTATUS
STDCALL
KeWaitForMultipleObjects (
	ULONG		Count,
	PVOID		Object[],
	WAIT_TYPE	WaitType,
	KWAIT_REASON	WaitReason,
	KPROCESSOR_MODE	WaitMode,
	BOOLEAN		Alertable,
	PLARGE_INTEGER	Timeout,
	PKWAIT_BLOCK	WaitBlockArray
	);

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



// io permission map has a 8k size
// Each bit in the IOPM corresponds to an io port byte address. The bitmap
// is initialized to allow IO at any port. [ all bits set ]. 

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
VOID Ke386SetIoAccessMap(ULONG NewMap, PIOPM *IoPermissionMap);

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
VOID Ke386QueryIoAccessMap(BOOLEAN NewMap, PIOPM *IoPermissionMap);

/*
 * FUNCTION: Set the process IOPL
 * ARGUMENTS:
 *	Eprocess = Pointer to a executive process object
 *	EnableIo = Specify TRUE to enable IO and FALSE to disable 
 */
NTSTATUS Ke386IoSetAccessProcess(PEPROCESS Eprocess, BOOLEAN EnableIo);

/*
 * FUNCTION: Releases a set of Global Descriptor Table Selectors
 * ARGUMENTS:
 *	SelArray = 
 *	NumOfSelectors = 
 */
NTSTATUS KeI386ReleaseGdtSelectors(OUT PULONG SelArray,
				   IN ULONG NumOfSelectors);

/*
 * FUNCTION: Allocates a set of Global Descriptor Table Selectors
 * ARGUMENTS:
 *	SelArray = 
 *	NumOfSelectors = 
 */
NTSTATUS KeI386AllocateGdtSelectors(OUT PULONG SelArray,
				    IN ULONG NumOfSelectors);


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

#endif /* __INCLUDE_DDK_KEFUNCS_H */
