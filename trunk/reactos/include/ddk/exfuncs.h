#ifndef _NTOS_EXFUNCS_H
#define _NTOS_EXFUNCS_H

/* EXECUTIVE ROUTINES ******************************************************/

#ifdef __NTOSKRNL__
extern POBJECT_TYPE EXPORTED ExDesktopObjectType;
extern POBJECT_TYPE EXPORTED ExEventObjectType;
extern POBJECT_TYPE EXPORTED ExWindowStationObjectType;
extern POBJECT_TYPE EXPORTED ExIoCompletionType;
#else
extern POBJECT_TYPE IMPORTED ExDesktopObjectType;
extern POBJECT_TYPE IMPORTED ExEventObjectType;
extern POBJECT_TYPE IMPORTED ExWindowStationObjectType;
extern POBJECT_TYPE IMPORTED ExIoCompletionType;
#endif

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))

VOID
FASTCALL
ExAcquireFastMutex (
	PFAST_MUTEX	FastMutex
	);
VOID
FASTCALL
ExAcquireFastMutexUnsafe (
	PFAST_MUTEX	FastMutex
	);
BOOLEAN
STDCALL
ExAcquireResourceExclusive (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
BOOLEAN
STDCALL
ExAcquireResourceExclusiveLite (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
BOOLEAN
STDCALL
ExAcquireResourceSharedLite (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
BOOLEAN
FASTCALL
ExAcquireRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    );
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx (
    IN PEX_RUNDOWN_REF RunRef,
    IN ULONG Count
    );
PVOID
FASTCALL
ExfAcquirePushLockExclusive (
	PVOID		Lock
	);
PVOID
FASTCALL
ExfAcquirePushLockShared (
	PVOID		Lock
	);
PVOID
FASTCALL
ExfReleasePushLock (
	PVOID		Lock
	);
BOOLEAN
STDCALL
ExAcquireSharedStarveExclusive (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);
BOOLEAN
STDCALL
ExAcquireSharedWaitForExclusive (
	PERESOURCE	Resource,
	BOOLEAN		Wait
	);

PVOID
STDCALL
ExAllocateFromZone (
	PZONE_HEADER	Zone
	);

/*
 * PVOID
 * ExAllocateFromZone (
 *	PZONE_HEADER	Zone
 *	);
 *
 * FUNCTION:
 *	Allocate a block from a zone
 *
 * ARGUMENTS:
 *	Zone = Zone to allocate from
 *
 * RETURNS:
 *	The base address of the block allocated
 */
#define ExAllocateFromZone(Zone) \
	(PVOID)((Zone)->FreeList.Next); \
	if ((Zone)->FreeList.Next) \
		(Zone)->FreeList.Next = (Zone)->FreeList.Next->Next

/*
 * FUNCTION: Allocates memory from the nonpaged pool
 * ARGUMENTS:
 *      NumberOfBytes = minimum size of the block to be allocated
 *      PoolType = the type of memory to use for the block (ignored)
 * RETURNS:
 *      the address of the block if it succeeds
 */
PVOID
STDCALL
ExAllocatePool (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	);

PVOID
STDCALL
ExAllocatePoolWithQuota (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	);
PVOID
STDCALL
ExAllocatePoolWithQuotaTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	);
PVOID
STDCALL
ExAllocatePoolWithTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	);

PVOID
NTAPI
ExAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    );

VOID
STDCALL
ExConvertExclusiveToSharedLite (
	PERESOURCE	Resource
	);

NTSTATUS
STDCALL
ExCreateCallback (
	OUT	PCALLBACK_OBJECT	* CallbackObject,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	BOOLEAN			Create,
	IN	BOOLEAN			AllowMultipleCallbacks
	);

NTSTATUS
STDCALL
ExDeleteResource (
	PERESOURCE	Resource
	);
NTSTATUS
STDCALL
ExDeleteResourceLite (
	PERESOURCE	Resource
	);

VOID
STDCALL
ExDisableResourceBoostLite (
	PERESOURCE	Resource
	);

VOID
STDCALL
ExEnumHandleTable (
	PULONG	HandleTable,
	PVOID	Callback,
	PVOID	Param,
	PHANDLE	Handle OPTIONAL
	);

NTSTATUS
STDCALL
ExExtendZone (
	PZONE_HEADER	Zone,
	PVOID		Segment,
	ULONG		SegmentSize
	);

/*
 * FUNCTION: Releases previously allocated memory
 * ARGUMENTS:
 *        block = block to free
 */
VOID
STDCALL
ExFreePool (
	PVOID	block
	);

/*
 * PVOID
 * ExFreeToZone (
 *	PZONE_HEADER	Zone,
 *	PVOID		Block
 *	);
 *
 * FUNCTION:
 *	Frees a block from a zone
 *
 * ARGUMENTS:
 *	Zone = Zone the block was allocated from
 *	Block = Block to free
 */
#define ExFreeToZone(Zone,Block) \
	(((PSINGLE_LIST_ENTRY)(Block))->Next = (Zone)->FreeList.Next, \
	 (Zone)->FreeList.Next = ((PSINGLE_LIST_ENTRY)(Block)), \
	 ((PSINGLE_LIST_ENTRY)(Block))->Next)

/*
 * ERESOURCE_THREAD
 * ExGetCurrentResourceThread (
 *	VOID
 *	);
 */
#define ExGetCurrentResourceThread() \
	((ERESOURCE_THREAD)KeGetCurrentThread())

LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG ExChange,
    IN PLONGLONG Comperand
    );

VOID
STDCALL
ExGetCurrentProcessorCounts (
	PULONG	ThreadKernelTime,
	PULONG	TotalCpuTime,
	PULONG	ProcessorNumber
);

VOID
STDCALL
ExGetCurrentProcessorCpuUsage (
	PULONG	CpuUsage
	);

ULONG
STDCALL
ExGetExclusiveWaiterCount (
	PERESOURCE	Resource
	);

KPROCESSOR_MODE
STDCALL
ExGetPreviousMode (
	VOID
	);

ULONG
STDCALL
ExGetSharedWaiterCount (
	PERESOURCE	Resource
	);

/*
 * VOID
 * ExInitializeFastMutex (
 *	PFAST_MUTEX	FastMutex
 *	);
 */
#define ExInitializeFastMutex(_FastMutex) \
	((PFAST_MUTEX)_FastMutex)->Count = 1; \
	((PFAST_MUTEX)_FastMutex)->Owner = NULL; \
	((PFAST_MUTEX)_FastMutex)->Contention = 0; \
	KeInitializeEvent(&((PFAST_MUTEX)_FastMutex)->Event, \
	                  SynchronizationEvent, \
	                  FALSE);

NTSTATUS
STDCALL
ExInitializeResource (
	PERESOURCE	Resource
	);
NTSTATUS
STDCALL
ExInitializeResourceLite (
	PERESOURCE	Resource
	);

VOID
FASTCALL
ExInitializeRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    );

/*
 * VOID
 * ExInitializeSListHead (
 *	PSLIST_HEADER	SListHead
 *	);
 */
#define ExInitializeSListHead(ListHead) \
	(ListHead)->Alignment = 0

/*
 * VOID
 * ExInitializeWorkItem (
 *	PWORK_QUEUE_ITEM	Item,
 *	PWORKER_THREAD_ROUTINE	Routine,
 *	PVOID			Context
 *	);
 *
 * FUNCTION:
 *	Initializes a work item to be processed by one of the system
 *	worker threads
 *
 * ARGUMENTS:
 *	Item = Pointer to the item to be initialized
 *	Routine = Routine to be called by the worker thread
 *	Context = Parameter to be passed to the callback
 */
#define ExInitializeWorkItem(Item, Routine, RoutineContext) \
	ASSERT_IRQL(DISPATCH_LEVEL); \
	(Item)->WorkerRoutine = (Routine); \
	(Item)->Parameter = (RoutineContext); \
	(Item)->List.Flink = NULL;

NTSTATUS
STDCALL
ExInitializeZone (
	PZONE_HEADER	Zone,
	ULONG		BlockSize,
	PVOID		InitialSegment,
	ULONG		InitialSegmentSize
	);

LARGE_INTEGER
STDCALL
ExInterlockedAddLargeInteger (
	PLARGE_INTEGER	Addend,
	LARGE_INTEGER	Increment,
	PKSPIN_LOCK	Lock
	);

VOID
FASTCALL
ExInterlockedAddLargeStatistic (
	IN	PLARGE_INTEGER	Addend,
	IN	ULONG		Increment
	);

ULONG
STDCALL
ExInterlockedAddUlong (
	PULONG		Addend,
	ULONG		Increment,
	PKSPIN_LOCK	Lock
	);

/*
 * PVOID
 * STDCALL
 * ExInterlockedAllocateFromZone (
 *	PZONE_HEADER	Zone,
 *	PKSPIN_LOCK	Lock
 *	);
 */
#define ExInterlockedAllocateFromZone(Zone,Lock) \
	(PVOID)ExInterlockedPopEntryList(&(Zone)->FreeList,Lock)

LONGLONG
FASTCALL
ExInterlockedCompareExchange64 (
	IN OUT	PLONGLONG	Destination,
	IN	PLONGLONG	Exchange,
	IN	PLONGLONG	Comparand,
	IN	PKSPIN_LOCK	Lock
	);

INTERLOCKED_RESULT
STDCALL
ExInterlockedDecrementLong (
	PLONG		Addend,
	PKSPIN_LOCK	Lock
	);

ULONG
STDCALL
ExInterlockedExchangeUlong (
	PULONG		Target,
	ULONG		Value,
	PKSPIN_LOCK	Lock
	);

NTSTATUS
STDCALL
ExInterlockedExtendZone (
	PZONE_HEADER	Zone,
	PVOID		Segment,
	ULONG		SegmentSize,
	PKSPIN_LOCK	Lock
	);

PSLIST_ENTRY
FASTCALL
ExInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

/*
 * PVOID
 * ExInterlockedFreeToZone (
 *	PZONE_HEADER	Zone,
 *	PVOID		Block,
 *	PKSPIN_LOCK	Lock
 *	);
 */
#define ExInterlockedFreeToZone(Zone,Block,Lock) \
	ExInterlockedPushEntryList(&(Zone)->FreeList,((PSINGLE_LIST_ENTRY)(Block)),(Lock))

INTERLOCKED_RESULT
STDCALL
ExInterlockedIncrementLong (
	PLONG		Addend,
	PKSPIN_LOCK	Lock
	);
PLIST_ENTRY
STDCALL
ExInterlockedInsertHeadList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	ListEntry,
	PKSPIN_LOCK	Lock
	);
PLIST_ENTRY
STDCALL
ExInterlockedInsertTailList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	ListEntry,
	PKSPIN_LOCK	Lock
	);
PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPopEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PKSPIN_LOCK		Lock
	);
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntrySList (
	PSLIST_HEADER	ListHead,
	PKSPIN_LOCK	Lock
	);
PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPushEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PSINGLE_LIST_ENTRY	ListEntry,
	PKSPIN_LOCK		Lock
	);
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntrySList (
	PSLIST_HEADER		ListHead,
	PSINGLE_LIST_ENTRY	ListEntry,
	PKSPIN_LOCK		Lock
	);
PLIST_ENTRY
STDCALL
ExInterlockedRemoveHeadList (
	PLIST_ENTRY	Head,
	PKSPIN_LOCK	Lock
	);

/*
 * BOOLEAN
 * ExIsFullZone (
 *	PZONE_HEADER	Zone
 *	);
 */
#define ExIsFullZone(Zone) \
	((Zone)->FreeList.Next==(PSINGLE_LIST_ENTRY)NULL)

/*
 * BOOLEAN
 * ExIsObjectInFirstZoneSegment (
 *	PZONE_HEADER	Zone,
 *	PVOID		Object
 *	);
 */
#define ExIsObjectInFirstZoneSegment(Zone,Object) \
	(((PUCHAR)(Object)>=(PUCHAR)(Zone)->SegmentList.Next) && \
	 ((PUCHAR)(Object)<(PUCHAR)(Zone)->SegmentList.Next+(Zone)->TotalSegmentSize))

BOOLEAN
STDCALL
ExIsProcessorFeaturePresent (
	IN	ULONG	ProcessorFeature
	);

BOOLEAN
STDCALL
ExIsResourceAcquiredExclusiveLite (
	PERESOURCE	Resource
	);

/* was ULONG */
USHORT
STDCALL
ExIsResourceAcquiredSharedLite (
	PERESOURCE	Resource
	);

VOID
STDCALL
ExLocalTimeToSystemTime (
	PLARGE_INTEGER	LocalTime,
	PLARGE_INTEGER	SystemTime
	);

VOID
STDCALL
ExNotifyCallback (
	IN	PCALLBACK_OBJECT	CallbackObject,
	IN	PVOID	Argument1,
	IN	PVOID	Argument2
	);

VOID
STDCALL
ExPostSystemEvent (
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3
	);

/*
 * USHORT
 * ExQueryDepthSList (
 *	PSLIST_HEADER	SListHead
 *	);
 */
#define ExQueryDepthSList(ListHead) \
	(USHORT)(ListHead)->Depth

VOID
STDCALL
ExQueueWorkItem (
	PWORK_QUEUE_ITEM	WorkItem,
	WORK_QUEUE_TYPE		QueueType
	);

SIZE_T
STDCALL
ExQueryPoolBlockSize (                          
    IN PVOID PoolBlock,                         
    OUT PBOOLEAN QuotaCharged                   
    );  

VOID
STDCALL
ExRaiseAccessViolation (
	VOID
	);
VOID
STDCALL
ExRaiseDatatypeMisalignment (
	VOID
	);
VOID
STDCALL
ExRaiseStatus (
	NTSTATUS	Status
	);
VOID
STDCALL
ExRaiseException (
	PEXCEPTION_RECORD pExcptRec
	);
VOID
STDCALL
ExRaiseHardError (
	IN NTSTATUS ErrorStatus,
	IN ULONG NumberOfParameters, 
	IN PUNICODE_STRING UnicodeStringParameterMask OPTIONAL,
	IN PVOID *Parameters, 
	IN HARDERROR_RESPONSE_OPTION ResponseOption, 
	OUT PHARDERROR_RESPONSE Response 
	);

PVOID
STDCALL
ExRegisterCallback (
	IN	PCALLBACK_OBJECT	CallbackObject,
	IN	PCALLBACK_FUNCTION	CallbackFunction,
	IN	PVOID			CallbackContext
	);

VOID
STDCALL
ExReinitializeResourceLite (
	PERESOURCE	Resource
	);
VOID
FASTCALL
ExReInitializeRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    );

/* ReactOS Specific: begin */
VOID
FASTCALL
ExReleaseFastMutex (
	PFAST_MUTEX	Mutex
	);
/* ReactOS Specific: end */
VOID
FASTCALL
ExReleaseFastMutexUnsafe (
	PFAST_MUTEX	Mutex
	);
/*
VOID
STDCALL
ExReleaseResource (
	PERESOURCE	Resource
	);
*/
#define ExReleaseResource(Resource) \
	(ExReleaseResourceLite (Resource))

VOID
FASTCALL
ExReleaseResourceLite (
	PERESOURCE	Resource
	);
VOID
STDCALL
ExReleaseResourceForThread (
	PERESOURCE		Resource,
	ERESOURCE_THREAD	ResourceThreadId
	);
VOID
STDCALL
ExReleaseResourceForThreadLite (
	PERESOURCE		Resource,
	ERESOURCE_THREAD	ResourceThreadId
	);
VOID
FASTCALL
ExReleaseRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    );
VOID
FASTCALL
ExReleaseRundownProtectionEx (
    IN PEX_RUNDOWN_REF RunRef,
    IN ULONG Count
    );
/* ReactOS Specific: begin */
VOID STDCALL
ExRosDumpPagedPoolByTag (
    IN ULONG Tag
    );
ULONG STDCALL
ExRosQueryPoolTag (
    IN PVOID Block
    );
/* ReactOS Specific: end */
VOID
FASTCALL
ExRundownCompleted (
    IN PEX_RUNDOWN_REF RunRef
    );
VOID
STDCALL
ExSetResourceOwnerPointer (
	IN	PERESOURCE	Resource,
	IN	PVOID		OwnerPointer
	);

ULONG
STDCALL
ExSetTimerResolution (
    IN ULONG DesiredTime,
    IN BOOLEAN SetResolution
    );

BOOLEAN
STDCALL
ExVerifySuite(
    SUITE_TYPE SuiteType
    );

BOOLEAN
STDCALL
ExSystemExceptionFilter();

VOID
STDCALL
ExSystemTimeToLocalTime (
	PLARGE_INTEGER	SystemTime,
	PLARGE_INTEGER	LocalTime
	);

BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
	PFAST_MUTEX	FastMutex
	);

BOOLEAN
STDCALL
ExTryToAcquireResourceExclusiveLite (
	PERESOURCE	Resource
	);

VOID
STDCALL
ExUnregisterCallback (
	IN	PVOID	CallbackRegistration
	);

typedef GUID UUID;

NTSTATUS
STDCALL
ExUuidCreate(
    OUT UUID *Uuid
    );

VOID
FASTCALL
ExWaitForRundownProtectionRelease (
    IN PEX_RUNDOWN_REF RunRef
    );

PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList (
  IN PSLIST_HEADER ListHead
  );

PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList(
  IN PSLIST_HEADER ListHead,
  IN PSLIST_ENTRY ListEntry
  );

/*
 * PVOID
 * ExAllocateFromNPagedLookasideList (
 *	PNPAGED_LOOKASIDE_LIST	LookSide
 *	);
 *
 * FUNCTION:
 *	Removes (pops) the first entry from the specified nonpaged
 *	lookaside list.
 *
 * ARGUMENTS:
 *	Lookaside = Pointer to a nonpaged lookaside list
 *
 * RETURNS:
 *	Address of the allocated list entry
 */
static
inline
PVOID
ExAllocateFromNPagedLookasideList (
	IN	PNPAGED_LOOKASIDE_LIST	Lookaside
	)
{
	PVOID Entry;

	Lookaside->TotalAllocates++;
	Entry = ExInterlockedPopEntrySList (&Lookaside->ListHead,
	                                    &Lookaside->Obsoleted);
	if (Entry == NULL)
	{
		Lookaside->AllocateMisses++;
		Entry = (Lookaside->Allocate)(Lookaside->Type,
		                              Lookaside->Size,
		                              Lookaside->Tag);
	}

  return Entry;
}

static inline PVOID
ExAllocateFromPagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside)
{
  PVOID Entry;

  Lookaside->TotalAllocates++;
  Entry = InterlockedPopEntrySList(&Lookaside->ListHead);
  if (Entry == NULL) {
    Lookaside->AllocateMisses++;
    Entry = (Lookaside->Allocate)(Lookaside->Type,
      Lookaside->Size, Lookaside->Tag);
  }
  return Entry;
}

VOID
STDCALL
ExDeleteNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside
	);

VOID
STDCALL
ExDeletePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside
	);


/*
 * VOID
 * ExFreeToNPagedLookasideList (
 *	PNPAGED_LOOKASIDE_LIST	Lookaside,
 *	PVOID			Entry
 *	);
 *
 * FUNCTION:
 *	Inserts (pushes) the specified entry into the specified
 *	nonpaged lookaside list.
 *
 * ARGUMENTS:
 *	Lookaside = Pointer to the nonpaged lookaside list
 *	Entry = Pointer to the entry that is inserted in the lookaside list
 */
static
inline
VOID
ExFreeToNPagedLookasideList (
	IN	PNPAGED_LOOKASIDE_LIST	Lookaside,
	IN	PVOID			Entry
	)
{
	Lookaside->TotalFrees++;
	if (ExQueryDepthSList (&Lookaside->ListHead) >= Lookaside->Depth)
	{
		Lookaside->FreeMisses++;
		(Lookaside->Free)(Entry);
	}
	else
	{
		ExInterlockedPushEntrySList (&Lookaside->ListHead,
		                             (PSINGLE_LIST_ENTRY)Entry,
		                             &Lookaside->Obsoleted);
	}
}

static inline VOID
ExFreeToPagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside,
  IN PVOID  Entry)
{
  Lookaside->TotalFrees++;
  if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->Depth) {
    Lookaside->FreeMisses++;
    (Lookaside->Free)(Entry);
  } else {
    InterlockedPushEntrySList(&Lookaside->ListHead, (PSLIST_ENTRY)Entry);
  }
}

VOID
STDCALL
ExInitializeNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth
	);

VOID
STDCALL
ExInitializePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth
	);

ULONG FASTCALL
ExfInterlockedAddUlong(IN PULONG Addend,
		       IN ULONG Increment,
		       IN PKSPIN_LOCK Lock);

PLIST_ENTRY FASTCALL
ExfInterlockedInsertHeadList(IN PLIST_ENTRY ListHead,
			     IN PLIST_ENTRY ListEntry,
			     IN PKSPIN_LOCK Lock);

PLIST_ENTRY FASTCALL
ExfInterlockedInsertTailList(IN PLIST_ENTRY ListHead,
			     IN PLIST_ENTRY ListEntry,
			     IN PKSPIN_LOCK Lock);

PSINGLE_LIST_ENTRY FASTCALL
ExfInterlockedPopEntryList(IN PSINGLE_LIST_ENTRY ListHead,
			   IN PKSPIN_LOCK Lock);

PSINGLE_LIST_ENTRY FASTCALL
ExfInterlockedPushEntryList(IN PSINGLE_LIST_ENTRY ListHead,
			    IN PSINGLE_LIST_ENTRY ListEntry,
			    IN PKSPIN_LOCK Lock);

PLIST_ENTRY FASTCALL
ExfInterlockedRemoveHeadList(IN PLIST_ENTRY Head,
			     IN PKSPIN_LOCK Lock);

INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedIncrementLong(IN PLONG Addend);

INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedDecrementLong(IN PLONG Addend);

ULONG FASTCALL
Exfi386InterlockedExchangeUlong(IN PULONG Target,
				IN ULONG Value);

INTERLOCKED_RESULT STDCALL
Exi386InterlockedIncrementLong(IN PLONG Addend);

INTERLOCKED_RESULT STDCALL
Exi386InterlockedDecrementLong(IN PLONG Addend);

ULONG STDCALL
Exi386InterlockedExchangeUlong(IN PULONG Target,
			       IN ULONG Value);


LONG
FASTCALL
InterlockedCompareExchange (
	PLONG	Destination,
	LONG	Exchange,
	LONG	Comperand
	);

#ifdef _GNU_H_WINDOWS_H
#ifdef InterlockedDecrement
#undef InterlockedDecrement
#undef InterlockedExchange
#undef InterlockedExchangeAdd
#undef InterlockedIncrement
#endif /* def InterlockedDecrement */
#endif /* def _GNU_H_WINDOWS_H */
LONG
FASTCALL
InterlockedDecrement (
	PLONG	Addend
	);
LONG
FASTCALL
InterlockedExchange (
	PLONG	Target,
	LONG	Value
	);
LONG
FASTCALL
InterlockedExchangeAdd (
	PLONG	Addend,
	LONG	Value
	);
LONG
FASTCALL
InterlockedIncrement (
	PLONG	Addend
	);

#ifndef InterlockedExchangePointer
# ifdef _WIN64
#  define InterlockedExchangePointer(__T__, __V__) \
             (PVOID)InterlockedExchange64((PLONGLONG)(__T__), (LONGLONG)(__V__))
# else
#  define InterlockedExchangePointer(__T__, __V__) \
             (PVOID)InterlockedExchange((PLONG)(__T__), (LONG)(__V__))
# endif
#endif

#ifndef InterlockedCompareExchangePointer
# ifdef _WIN64
#  define InterlockedCompareExchangePointer(__T__, __V__, __C__) \
             (PVOID)InterlockedCompareExchange64((PLONGLONG)(__T__), (LONGLONG)(__V__), (LONGLONG)(__C__))
# else
#  define InterlockedCompareExchangePointer(__T__, __V__, __C__) \
             (PVOID)InterlockedCompareExchange((PLONG)(__T__), (LONG)(__V__), (LONG)(__C__))
# endif
#endif

/*---*/

typedef
unsigned int
(*exception_hook) (
	CONTEXT		* c,
	unsigned int	exp
	);
unsigned int
ExHookException (
	exception_hook	fn,
	unsigned int	exp
	);

#endif /* ndef _NTOS_EXFUNCS_H */
