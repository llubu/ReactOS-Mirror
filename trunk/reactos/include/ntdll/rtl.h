/* $Id$
 *
 */

#ifndef __INCLUDE_NTDLL_RTL_H
#define __INCLUDE_NTDLL_RTL_H

#include <ntos/types.h>
#include <napi/teb.h>
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifndef __USE_W32API

typedef struct _DEBUG_BUFFER
{
  HANDLE SectionHandle;
  PVOID SectionBase;
  PVOID RemoteSectionBase;
  ULONG SectionBaseDelta;
  HANDLE EventPairHandle;
  ULONG Unknown[2];
  HANDLE RemoteThreadHandle;
  ULONG InfoClassMask;
  ULONG SizeOfInfo;
  ULONG AllocatedSize;
  ULONG SectionSize;
  PVOID ModuleInformation;
  PVOID BackTraceInformation;
  PVOID HeapInformation;
  PVOID LockInformation;
  PVOID Reserved[8];
} DEBUG_BUFFER, *PDEBUG_BUFFER;

/* DEBUG_MODULE_INFORMATION.Flags constants */
#define LDRP_STATIC_LINK                  0x00000002
#define LDRP_IMAGE_DLL                    0x00000004
#define LDRP_LOAD_IN_PROGRESS             0x00001000
#define LDRP_UNLOAD_IN_PROGRESS           0x00002000
#define LDRP_ENTRY_PROCESSED              0x00004000
#define LDRP_ENTRY_INSERTED               0x00008000
#define LDRP_CURRENT_LOAD                 0x00010000
#define LDRP_FAILED_BUILTIN_LOAD          0x00020000
#define LDRP_DONT_CALL_FOR_THREADS        0x00040000
#define LDRP_PROCESS_ATTACH_CALLED        0x00080000
#define LDRP_DEBUG_SYMBOLS_LOADED         0x00100000
#define LDRP_IMAGE_NOT_AT_BASE            0x00200000
#define LDRP_WX86_IGNORE_MACHINETYPE      0x00400000

typedef struct _DEBUG_MODULE_INFORMATION {
	ULONG  Reserved[2];
	PVOID  Base;
	ULONG  Size;
	ULONG  Flags;
	USHORT  Index;
	USHORT  Unknown;
	USHORT  LoadCount;
	USHORT  ModuleNameOffset;
	CHAR  ImageName[256];
} DEBUG_MODULE_INFORMATION, *PDEBUG_MODULE_INFORMATION;

typedef struct _DEBUG_HEAP_INFORMATION {
	PVOID  Base;
	ULONG  Flags;
	USHORT  Granularity;
	USHORT  Unknown;
	ULONG  Allocated;
	ULONG  Committed;
	ULONG  TagCount;
	ULONG  BlockCount;
	ULONG  Reserved[7];
	PVOID  Tags;
	PVOID  Blocks;
} DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;

typedef struct _DEBUG_LOCK_INFORMATION {
	PVOID  Address;
	USHORT  Type;
	USHORT  CreatorBackTraceIndex;
	ULONG  OwnerThreadId;
	ULONG  ActiveCount;
	ULONG  ContentionCount;
	ULONG  EntryCount;
	ULONG  RecursionCount;
	ULONG  NumberOfSharedWaiters;
	ULONG  NumberOfExclusiveWaiters;
} DEBUG_LOCK_INFORMATION, *PDEBUG_LOCK_INFORMATION;

typedef struct _CRITICAL_SECTION_DEBUG
{
  USHORT Type;
  USHORT CreatorBackTraceIndex;
  struct _CRITICAL_SECTION *CriticalSection;
  LIST_ENTRY ProcessLocksList;
  ULONG EntryCount;
  ULONG ContentionCount;
  PVOID OwnerBackTrace[2];
  PVOID Spare[2];
} CRITICAL_SECTION_DEBUG, *PCRITICAL_SECTION_DEBUG;


typedef struct _CRITICAL_SECTION
{
  PCRITICAL_SECTION_DEBUG DebugInfo;
  LONG LockCount;
  LONG RecursionCount;
  HANDLE OwningThread;
  HANDLE LockSemaphore;
  ULONG_PTR SpinCount;
} CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;

#define RTL_CRITSECT_TYPE 0

typedef CRITICAL_SECTION RTL_CRITICAL_SECTION;
typedef PCRITICAL_SECTION PRTL_CRITICAL_SECTION;
typedef LPCRITICAL_SECTION LPRTL_CRITICAL_SECTION;
typedef CRITICAL_SECTION_DEBUG RTL_CRITICAL_SECTION_DEBUG;
typedef PCRITICAL_SECTION_DEBUG PRTL_CRITICAL_SECTION_DEBUG;

#endif /* !__USE_W32API */

typedef struct _RTL_PROCESS_INFO
{
   ULONG Size;
   HANDLE ProcessHandle;
   HANDLE ThreadHandle;
   CLIENT_ID ClientId;
   SECTION_IMAGE_INFORMATION ImageInfo;
} RTL_PROCESS_INFO, *PRTL_PROCESS_INFO;

typedef struct _RTL_RESOURCE
{
   CRITICAL_SECTION Lock;
   HANDLE SharedSemaphore;
   ULONG SharedWaiters;
   HANDLE ExclusiveSemaphore;
   ULONG ExclusiveWaiters;
   LONG NumberActive;
   HANDLE OwningThread;
   ULONG TimeoutBoost; /* ?? */
   PVOID DebugInfo; /* ?? */
} RTL_RESOURCE, *PRTL_RESOURCE;

typedef struct _RTL_HANDLE
{
   struct _RTL_HANDLE *Next;	/* pointer to next free handle */
} RTL_HANDLE, *PRTL_HANDLE;

typedef struct _RTL_HANDLE_TABLE
{
   ULONG TableSize;		/* maximum number of handles */
   ULONG HandleSize;		/* size of handle in bytes */
   PRTL_HANDLE Handles;		/* pointer to handle array */
   PRTL_HANDLE Limit;		/* limit of pointers */
   PRTL_HANDLE FirstFree;	/* pointer to first free handle */
   PRTL_HANDLE LastUsed;	/* pointer to last allocated handle */
} RTL_HANDLE_TABLE, *PRTL_HANDLE_TABLE;


/* RtlQueryProcessDebugInformation */
#define PDI_MODULES     0x01	/* The loaded modules of the process */
#define PDI_BACKTRACE   0x02	/* The heap stack back traces */
#define PDI_HEAPS       0x04	/* The heaps of the process */
#define PDI_HEAP_TAGS   0x08	/* The heap tags */
#define PDI_HEAP_BLOCKS 0x10	/* The heap blocks */
#define PDI_LOCKS       0x20	/* The locks created by the process */

NTSTATUS
STDCALL
RtlpWaitForCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlpUnWaitCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlpCreateCriticalSectionSem(
    PRTL_CRITICAL_SECTION CriticalSection
);

VOID
STDCALL
RtlpInitDeferedCriticalSection(
    VOID
);

NTSTATUS STDCALL
RtlAddAccessAllowedAceEx (IN OUT PACL Acl,
			  IN ULONG Revision,
			  IN ULONG Flags,
			  IN ACCESS_MASK AccessMask,
			  IN PSID Sid);

NTSTATUS STDCALL
RtlAddAccessDeniedAceEx (IN OUT PACL Acl,
			 IN ULONG Revision,
			 IN ULONG Flags,
			 IN ACCESS_MASK AccessMask,
			 IN PSID Sid);

NTSTATUS STDCALL
RtlAddAuditAccessAceEx(IN OUT PACL Acl,
		       IN ULONG Revision,
                       IN ULONG Flags,
                       IN ACCESS_MASK AccessMask,
                       IN PSID Sid,
                       IN BOOLEAN Success,
                       IN BOOLEAN Failure);

NTSTATUS STDCALL
RtlDeleteCriticalSection (PRTL_CRITICAL_SECTION CriticalSection);

WCHAR STDCALL
RtlDowncaseUnicodeChar(IN WCHAR Source);

NTSTATUS STDCALL
RtlEnterCriticalSection (PRTL_CRITICAL_SECTION CriticalSection);

NTSTATUS STDCALL
RtlInitializeCriticalSection (PRTL_CRITICAL_SECTION CriticalSection);

NTSTATUS STDCALL
RtlInitializeCriticalSectionAndSpinCount (PRTL_CRITICAL_SECTION CriticalSection,
					  ULONG SpinCount);

NTSTATUS STDCALL
RtlInt64ToUnicodeString (IN ULONGLONG Value,
			 IN ULONG Base,
			 PUNICODE_STRING String);

NTSTATUS STDCALL
RtlLeaveCriticalSection (PRTL_CRITICAL_SECTION CriticalSection);

BOOLEAN STDCALL
RtlTryEnterCriticalSection (PRTL_CRITICAL_SECTION CriticalSection);

DWORD STDCALL
RtlCompactHeap (
	HANDLE	heap,
	DWORD	flags
	);

ULONG STDCALL
RtlComputeCrc32 (IN ULONG Initial,
		 IN PUCHAR Data,
		 IN ULONG Length);

PDEBUG_BUFFER STDCALL
RtlCreateQueryDebugBuffer(IN ULONG Size,
			  IN BOOLEAN EventPair);

NTSTATUS STDCALL
RtlDestroyQueryDebugBuffer(IN PDEBUG_BUFFER DebugBuffer);

BOOLEAN STDCALL
RtlEqualComputerName (
	IN	PUNICODE_STRING	ComputerName1,
	IN	PUNICODE_STRING	ComputerName2
	);

BOOLEAN STDCALL
RtlEqualDomainName (
	IN	PUNICODE_STRING	DomainName1,
	IN	PUNICODE_STRING	DomainName2
	);

VOID STDCALL
RtlEraseUnicodeString (
	IN	PUNICODE_STRING	String
	);

NTSTATUS STDCALL
RtlLargeIntegerToChar (
	IN	PLARGE_INTEGER	Value,
	IN	ULONG		Base,
	IN	ULONG		Length,
	IN OUT	PCHAR		String
	);


/* Path functions */

typedef enum
{
    INVALID_PATH = 0,
    UNC_PATH,              /* "//foo" */
    ABSOLUTE_DRIVE_PATH,   /* "c:/foo" */
    RELATIVE_DRIVE_PATH,   /* "c:foo" */
    ABSOLUTE_PATH,         /* "/foo" */
    RELATIVE_PATH,         /* "foo" */
    DEVICE_PATH,           /* "//./foo" */
    UNC_DOT_PATH           /* "//." */
} DOS_PATHNAME_TYPE;

ULONG
STDCALL
RtlDetermineDosPathNameType_U (
   PCWSTR Path
	);

BOOLEAN
STDCALL
RtlDoesFileExists_U (
	PWSTR FileName
	);

BOOLEAN
STDCALL
RtlDosPathNameToNtPathName_U (
	PWSTR		dosname,
	PUNICODE_STRING	ntname,
	PWSTR		*shortname,
	PCURDIR		nah
	);

ULONG
STDCALL
RtlDosSearchPath_U (
	WCHAR *sp,
	WCHAR *name,
	WCHAR *ext,
	ULONG buf_sz,
	WCHAR *buffer,
	WCHAR **shortname
	);

ULONG
STDCALL
RtlGetCurrentDirectory_U (
	ULONG MaximumLength,
	PWSTR Buffer
	);

ULONG
STDCALL
RtlGetFullPathName_U (
	WCHAR *dosname,
	ULONG size,
	WCHAR *buf,
	WCHAR **shortname
	);

ULONG
STDCALL
RtlGetLongestNtPathLength (
	VOID
	);

ULONG STDCALL
RtlGetNtGlobalFlags (VOID);

BOOLEAN STDCALL
RtlGetNtProductType (PNT_PRODUCT_TYPE ProductType);

ULONG
STDCALL
RtlGetProcessHeaps (
	ULONG	HeapCount,
	HANDLE	*HeapArray
	);

ULONG
STDCALL
RtlIsDosDeviceName_U (
	PWSTR DeviceName
	);

NTSTATUS
STDCALL
RtlSetCurrentDirectory_U (
	PUNICODE_STRING name
	);

/* Environment functions */
VOID
STDCALL
RtlAcquirePebLock (
	VOID
	);

VOID
STDCALL
RtlReleasePebLock (
	VOID
	);

NTSTATUS
STDCALL
RtlCreateEnvironment (
	BOOLEAN	Inherit,
	PWSTR	*Environment
	);

VOID
STDCALL
RtlDestroyEnvironment (
	PWSTR	Environment
	);

NTSTATUS
STDCALL
RtlExpandEnvironmentStrings_U (
	PWSTR		Environment,
	PUNICODE_STRING	Source,
	PUNICODE_STRING	Destination,
	PULONG		Length
	);

NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PWSTR		Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

NTSTATUS STDCALL
RtlQueryProcessDebugInformation(IN ULONG ProcessId,
				IN ULONG DebugInfoClassMask,
				IN OUT PDEBUG_BUFFER DebugBuffer);

VOID
STDCALL
RtlSetCurrentEnvironment (
	PWSTR	NewEnvironment,
	PWSTR	*OldEnvironment
	);

NTSTATUS
STDCALL
RtlSetEnvironmentVariable (
	PWSTR		*Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	);

NTSTATUS
STDCALL
RtlCreateUserThread (
	IN	HANDLE			ProcessHandle,
	IN	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	IN	BOOLEAN			CreateSuspended,
	IN	LONG			StackZeroBits,
	IN OUT	PULONG			StackReserve,
	IN OUT	PULONG			StackCommit,
	IN	PTHREAD_START_ROUTINE	StartAddress,
	IN	PVOID			Parameter,
	IN OUT	PHANDLE			ThreadHandle,
	IN OUT	PCLIENT_ID		ClientId
	);

NTSTATUS STDCALL
RtlExitUserThread (NTSTATUS Status);

NTSTATUS
STDCALL
RtlFreeUserThreadStack (
	IN	HANDLE	ProcessHandle,
	IN	HANDLE	ThreadHandle
	);

NTSTATUS
STDCALL
RtlCreateUserProcess (
	IN	PUNICODE_STRING			ImageFileName,
	IN	ULONG				Attributes,
	IN	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
	IN	PSECURITY_DESCRIPTOR		ProcessSecutityDescriptor OPTIONAL,
	IN	PSECURITY_DESCRIPTOR		ThreadSecurityDescriptor OPTIONAL,
	IN	HANDLE				ParentProcess OPTIONAL,
	IN	BOOLEAN				CurrentDirectory,
	IN	HANDLE				DebugPort OPTIONAL,
	IN	HANDLE				ExceptionPort OPTIONAL,
	OUT	PRTL_PROCESS_INFO		ProcessInfo
	);

NTSTATUS
STDCALL
RtlCreateProcessParameters (
	OUT	PRTL_USER_PROCESS_PARAMETERS	*ProcessParameters,
	IN	PUNICODE_STRING	ImagePathName OPTIONAL,
	IN	PUNICODE_STRING	DllPath OPTIONAL,
	IN	PUNICODE_STRING	CurrentDirectory OPTIONAL,
	IN	PUNICODE_STRING	CommandLine OPTIONAL,
	IN	PWSTR		Environment OPTIONAL,
	IN	PUNICODE_STRING	WindowTitle OPTIONAL,
	IN	PUNICODE_STRING	DesktopInfo OPTIONAL,
	IN	PUNICODE_STRING	ShellInfo OPTIONAL,
	IN	PUNICODE_STRING	RuntimeInfo OPTIONAL
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlDeNormalizeProcessParams (
	IN	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

NTSTATUS
STDCALL
RtlDestroyProcessParameters (
	IN	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

PRTL_USER_PROCESS_PARAMETERS
STDCALL
RtlNormalizeProcessParams (
	IN	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters
	);

NTSTATUS
STDCALL
RtlLocalTimeToSystemTime (
	PLARGE_INTEGER	LocalTime,
	PLARGE_INTEGER	SystemTime
	);

NTSTATUS
STDCALL
RtlSystemTimeToLocalTime (
	PLARGE_INTEGER	SystemTime,
	PLARGE_INTEGER	LocalTime
	);

VOID STDCALL
RtlTimeToElapsedTimeFields(IN PLARGE_INTEGER Time,
			   OUT PTIME_FIELDS TimeFields);

VOID
STDCALL
RtlRaiseStatus (
	IN	NTSTATUS	Status
	);


/* resource functions */

BOOLEAN
STDCALL
RtlAcquireResourceExclusive (
	IN	PRTL_RESOURCE	Resource,
	IN	BOOLEAN		Wait
	);

BOOLEAN
STDCALL
RtlAcquireResourceShared (
	IN	PRTL_RESOURCE	Resource,
	IN	BOOLEAN		Wait
	);

VOID
STDCALL
RtlConvertExclusiveToShared (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlConvertSharedToExclusive (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlDeleteResource (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlDumpResource (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlInitializeResource (
	IN	PRTL_RESOURCE	Resource
	);

VOID
STDCALL
RtlReleaseResource (
	IN	PRTL_RESOURCE	Resource
	);

/* handle table functions */

PRTL_HANDLE
STDCALL
RtlAllocateHandle (
	IN	PRTL_HANDLE_TABLE	HandleTable,
	IN OUT	PULONG			Index
	);

VOID
STDCALL
RtlDestroyHandleTable (
	IN	PRTL_HANDLE_TABLE	HandleTable
	);

BOOLEAN
STDCALL
RtlFreeHandle (
	IN	PRTL_HANDLE_TABLE	HandleTable,
	IN	PRTL_HANDLE		Handle
	);

VOID
STDCALL
RtlInitializeHandleTable (
	IN	ULONG			TableSize,
	IN	ULONG			HandleSize,
	IN	PRTL_HANDLE_TABLE	HandleTable
	);

BOOLEAN
STDCALL
RtlIsValidHandle (
	IN	PRTL_HANDLE_TABLE	HandleTable,
	IN	PRTL_HANDLE		Handle
	);

BOOLEAN
STDCALL
RtlIsValidIndexHandle (
	IN	PRTL_HANDLE_TABLE	HandleTable,
	IN OUT	PRTL_HANDLE		*Handle,
	IN	ULONG			Index
	);

NTSTATUS STDCALL
RtlAdjustPrivilege(IN ULONG Privilege,
		   IN BOOLEAN Enable,
		   IN BOOLEAN CurrentThread,
		   OUT PBOOLEAN Enabled);

NTSTATUS
STDCALL
RtlImpersonateSelf (
	IN	SECURITY_IMPERSONATION_LEVEL	ImpersonationLevel
	);

NTSTATUS
STDCALL
RtlpNtCreateKey (
	OUT	HANDLE			KeyHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	ULONG			Unused1,
	OUT	PULONG			Disposition,
	IN	ULONG			Unused2
	);

NTSTATUS
STDCALL
RtlpNtEnumerateSubKey (
	IN	HANDLE		KeyHandle,
	OUT	PUNICODE_STRING	SubKeyName,
	IN	ULONG		Index,
	IN	ULONG		Unused
	);

NTSTATUS
STDCALL
RtlpNtMakeTemporaryKey (
	IN	HANDLE	KeyHandle
	);

NTSTATUS
STDCALL
RtlpNtOpenKey (
	OUT	HANDLE			KeyHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	ULONG			Unused
	);

NTSTATUS
STDCALL
RtlpNtQueryValueKey (
	IN	HANDLE	KeyHandle,
	OUT	PULONG	Type		OPTIONAL,
	OUT	PVOID	Data		OPTIONAL,
	IN OUT	PULONG	DataLength	OPTIONAL,
	IN	ULONG	Unused
	);

NTSTATUS
STDCALL
RtlpNtSetValueKey (
	IN	HANDLE	KeyHandle,
	IN	ULONG	Type,
	IN	PVOID	Data,
	IN	ULONG	DataLength
	);


VOID STDCALL
RtlRunDecodeUnicodeString (IN UCHAR Hash,
			   IN OUT PUNICODE_STRING String);

VOID STDCALL
RtlRunEncodeUnicodeString (IN OUT PUCHAR Hash,
			   IN OUT PUNICODE_STRING String);

/* Timer Queue functions */

#ifdef __USE_W32API
#include <winnt.h>
#else /* __USE_W32API */
typedef VOID (CALLBACK *WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );
#endif /* __USE_W32API */

NTSTATUS
STDCALL
RtlCreateTimer(HANDLE TimerQueue,PHANDLE phNewTimer, WAITORTIMERCALLBACKFUNC Callback,PVOID Parameter,DWORD DueTime,DWORD Period,ULONG Flags);

NTSTATUS
STDCALL
RtlCreateTimerQueue(PHANDLE TimerQueue);

NTSTATUS
STDCALL
RtlDeleteTimer(HANDLE TimerQueue,HANDLE Timer,HANDLE CompletionEvent);

NTSTATUS
STDCALL
RtlUpdateTimer(HANDLE TimerQueue,HANDLE Timer,ULONG DueTime,ULONG Period);

NTSTATUS
STDCALL
RtlDeleteTimerQueueEx(HANDLE TimerQueue,HANDLE CompletionEvent);

NTSTATUS
STDCALL
RtlDeleteTimerQueue(HANDLE TimerQueue);


#ifndef __NTDRIVER__

#ifndef __INTERLOCKED_DECLARED
#define __INTERLOCKED_DECLARED

LONG
STDCALL
InterlockedIncrement (
	PLONG Addend
	);

LONG
STDCALL
InterlockedDecrement (
	PLONG lpAddend
	);

LONG
STDCALL
InterlockedExchange (
	PLONG Target,
	LONG Value
	);

LONG
STDCALL
InterlockedCompareExchange (
	PLONG Destination,
	LONG Exchange,
	LONG Comperand
	);

LONG
STDCALL
InterlockedExchangeAdd (
	PLONG Addend,
	LONG Increment
	);

#ifndef InterlockedExchangePointer
   #ifdef _WIN64
      #define InterlockedExchangePointer(Target, Value) \
             (PVOID)InterlockedExchange64((PLONGLONG)(Target), (LONGLONG)(Value))
   #else
      #define InterlockedExchangePointer(Target, Value) \
             (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))
   #endif
#endif

#ifndef InterlockedCompareExchangePointer
   #ifdef _WIN64
      #define InterlockedCompareExchangePointer(Target, Exchange, Comperand) \
             (PVOID)InterlockedCompareExchange64((PLONGLONG)(Target), (LONGLONG)(Exchange), (LONGLONG)(Comperand))
   #else
      #define InterlockedCompareExchangePointer(Target, Exchange, Comperand) \
             (PVOID)InterlockedCompareExchange((PLONG)Target, (LONG)Exchange, (LONG)Comperand)
   #endif
#endif

#endif /* __INTERLOCKED_DECLARED */

#endif /* __NTDRIVER__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __INCLUDE_NTDLL_RTL_H */

/* EOF */
