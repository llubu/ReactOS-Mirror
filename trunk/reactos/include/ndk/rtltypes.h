/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    rtltypes.h

Abstract:

    Type definitions for the Run-Time Library

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _RTLTYPES_H
#define _RTLTYPES_H

//
// Dependencies
//
#include <umtypes.h>
#include <pstypes.h>

//
// Maximum Atom Length
//
#define RTL_MAXIMUM_ATOM_LENGTH                              255

//
// Process Parameters Flags (FIXME: Rename)
//
#define PPF_NORMALIZED                                       0x01
#define PPF_PROFILE_USER                                     0x02
#define PPF_PROFILE_SERVER                                   0x04
#define PPF_PROFILE_KERNEL                                   0x08
#define PPF_UNKNOWN                                          0x10
#define PPF_RESERVE_1MB                                      0x20
#define PPF_DISABLE_HEAP_CHECKS                              0x100
#define PPF_PROCESS_OR_1                                     0x200
#define PPF_PROCESS_OR_2                                     0x400

//
// Exception Flags
//
#define EXCEPTION_CHAIN_END                                  ((PEXCEPTION_REGISTRATION_RECORD)-1)
#define EXCEPTION_UNWINDING                                  0x02
#define EXCEPTION_EXIT_UNWIND                                0x04
#define EXCEPTION_STACK_INVALID                              0x08
#define EXCEPTION_NESTED_CALL                                0x10
#define EXCEPTION_TARGET_UNWIND                              0x20
#define EXCEPTION_COLLIDED_UNWIND                            0x20

//
// Range and Range List Flags
//
#define RTL_RANGE_LIST_ADD_IF_CONFLICT                       0x00000001
#define RTL_RANGE_LIST_ADD_SHARED                            0x00000002

#define RTL_RANGE_SHARED                                     0x01
#define RTL_RANGE_CONFLICT                                   0x02

//
// Registry Keys
//
#define RTL_REGISTRY_ABSOLUTE                                0
#define RTL_REGISTRY_SERVICES                                1
#define RTL_REGISTRY_CONTROL                                 2
#define RTL_REGISTRY_WINDOWS_NT                              3
#define RTL_REGISTRY_DEVICEMAP                               4
#define RTL_REGISTRY_USER                                    5
#define RTL_REGISTRY_MAXIMUM                                 6
#define RTL_REGISTRY_HANDLE                                  0x40000000
#define RTL_REGISTRY_OPTIONAL                                0x80000000
#define RTL_QUERY_REGISTRY_SUBKEY                            0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY                            0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED                          0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE                           0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND                          0x00000010
#define RTL_QUERY_REGISTRY_DIRECT                            0x00000020
#define RTL_QUERY_REGISTRY_DELETE                            0x00000040

//
// Versioning
//
#define VER_MINORVERSION                                     0x0000001
#define VER_MAJORVERSION                                     0x0000002
#define VER_BUILDNUMBER                                      0x0000004
#define VER_PLATFORMID                                       0x0000008
#define VER_SERVICEPACKMINOR                                 0x0000010
#define VER_SERVICEPACKMAJOR                                 0x0000020
#define VER_SUITENAME                                        0x0000040
#define VER_PRODUCT_TYPE                                     0x0000080
#define VER_PLATFORM_WIN32s                                  0
#define VER_PLATFORM_WIN32_WINDOWS                           1
#define VER_PLATFORM_WIN32_NT                                2
#define VER_EQUAL                                            1
#define VER_GREATER                                          2
#define VER_GREATER_EQUAL                                    3
#define VER_LESS                                             4
#define VER_LESS_EQUAL                                       5
#define VER_AND                                              6
#define VER_OR                                               7
#define VER_CONDITION_MASK                                   7
#define VER_NUM_BITS_PER_CONDITION_MASK                      3

//
// Timezone IDs
//
#define TIME_ZONE_ID_UNKNOWN                                 0
#define TIME_ZONE_ID_STANDARD                                1
#define TIME_ZONE_ID_DAYLIGHT                                2

//
// RTL Lock Type (Critical Section or Resource)
//
#define RTL_CRITSECT_TYPE                                   0
#define RTL_RESOURCE_TYPE                                   1

#ifdef NTOS_MODE_USER

//
// String Hash Algorithms
//
#define HASH_STRING_ALGORITHM_DEFAULT                       0
#define HASH_STRING_ALGORITHM_X65599                        1
#define HASH_STRING_ALGORITHM_INVALID                       0xffffffff

//
// RtlDuplicateString Flags
//
#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE         1
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING   2

//
// Codepages
//
#define NLS_MB_CODE_PAGE_TAG                                NlsMbCodePageTag
#define NLS_MB_OEM_CODE_PAGE_TAG                            NlsMbOemCodePageTag
#define NLS_OEM_LEAD_BYTE_INFO                              NlsOemLeadByteInfo
#endif
#define MAXIMUM_LEADBYTES                                   12

//
// RTL Debug Queries
//
#define RTL_DEBUG_QUERY_MODULES                             0x01
#define RTL_DEBUG_QUERY_BACKTRACES                          0x02
#define RTL_DEBUG_QUERY_HEAPS                               0x04
#define RTL_DEBUG_QUERY_HEAP_TAGS                           0x08
#define RTL_DEBUG_QUERY_HEAP_BLOCKS                         0x10
#define RTL_DEBUG_QUERY_LOCKS                               0x20

//
// RTL Handle Flags
//
#define RTL_HANDLE_VALID                                    0x1

//
// RTL Atom Flags
//
#define RTL_ATOM_IS_PINNED                                  0x1

//
// Codepage Tags
//
#ifdef NTOS_MODE_USER
extern BOOLEAN NTSYSAPI NLS_MB_CODE_PAGE_TAG;
extern BOOLEAN NTSYSAPI NLS_MB_OEM_CODE_PAGE_TAG;

//
// Constant String Macro
//
#define RTL_CONSTANT_STRING(__SOURCE_STRING__)                  \
{                                                               \
    sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
    sizeof(__SOURCE_STRING__),                                  \
    (__SOURCE_STRING__)                                         \
}

#endif

#ifdef NTOS_MODE_USER

//
// Table and Compare result types
//
typedef enum _TABLE_SEARCH_RESULT
{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

typedef enum _RTL_GENERIC_COMPARE_RESULTS
{
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

#else

//
// ACL Query Information Classes
//
typedef enum _ACL_INFORMATION_CLASS
{
    AclRevisionInformation = 1,
    AclSizeInformation
} ACL_INFORMATION_CLASS;

#endif

//
// RTL Path Types
//
typedef enum _RTL_PATH_TYPE
{
    INVALID_PATH = 0,
    UNC_PATH,              // "//foo"
    ABSOLUTE_DRIVE_PATH,   // "c:/foo"
    RELATIVE_DRIVE_PATH,   // "c:foo"
    ABSOLUTE_PATH,         // "/foo"
    RELATIVE_PATH,         // "foo"
    DEVICE_PATH,           // "//./foo"
    UNC_DOT_PATH           // "//."
} RTL_PATH_TYPE;

#ifndef NTOS_MODE_USER

//
// Callback function for RTL Timers or Registered Waits
//
typedef VOID
(NTAPI *WAITORTIMERCALLBACKFUNC)(
    PVOID pvContext,
    BOOLEAN fTimerOrWaitFired
);

//
// Handler during Vectored RTL Exceptions
//
typedef LONG
(NTAPI *PVECTORED_EXCEPTION_HANDLER)(
    PEXCEPTION_POINTERS ExceptionPointers
);

#else

//
// Handler during regular RTL Exceptions
//
typedef EXCEPTION_DISPOSITION
(NTAPI *PEXCEPTION_ROUTINE)(
    IN struct _EXCEPTION_RECORD *ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT struct _CONTEXT *ContextRecord,
    IN OUT PVOID DispatcherContext
);

#endif

//
// Callback for RTL Heap Enumeration
//
typedef NTSTATUS
(*PHEAP_ENUMERATION_ROUTINE)(
    IN PVOID HeapHandle,
    IN PVOID UserParam
);

//
// Thread and Process Start Routines for RtlCreateUserThread/Process
//
typedef ULONG (NTAPI *PTHREAD_START_ROUTINE)(
    PVOID Parameter
);

typedef VOID
(NTAPI *PRTL_BASE_PROCESS_START_ROUTINE)(
    PTHREAD_START_ROUTINE StartAddress,
    PVOID Parameter
);

//
// Declare empty structure definitions so that they may be referenced by
// routines before they are defined
//
struct _RTL_AVL_TABLE;
struct _RTL_GENERIC_TABLE;
struct _RTL_RANGE;
typedef struct _COMPRESSED_DATA_INFO COMPRESSED_DATA_INFO, *PCOMPRESSED_DATA_INFO;

//
// Routines and callbacks for the RTL AVL/Generic Table package
//
typedef NTSTATUS
(NTAPI *PRTL_AVL_MATCH_FUNCTION)(
    struct _RTL_AVL_TABLE *Table,
    PVOID UserData,
    PVOID MatchData
);

typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_AVL_COMPARE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
);

typedef RTL_GENERIC_COMPARE_RESULTS
(NTAPI *PRTL_GENERIC_COMPARE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    PVOID FirstStruct,
    PVOID SecondStruct
);

typedef PVOID
(NTAPI *PRTL_GENERIC_ALLOCATE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    CLONG ByteSize
);

typedef VOID
(NTAPI *PRTL_GENERIC_FREE_ROUTINE) (
    struct _RTL_GENERIC_TABLE *Table,
    PVOID Buffer
);

typedef PVOID
(NTAPI *PRTL_AVL_ALLOCATE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    CLONG ByteSize
);

typedef VOID
(NTAPI *PRTL_AVL_FREE_ROUTINE) (
    struct _RTL_AVL_TABLE *Table,
    PVOID Buffer
);

//
// RTL Query Registry callback
//
typedef NTSTATUS
(NTAPI *PRTL_QUERY_REGISTRY_ROUTINE)(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
);

//
// RTL Range List callbacks
//
#ifdef NTOS_MODE_USER
typedef BOOLEAN
(NTAPI *PRTL_CONFLICT_RANGE_CALLBACK)(
    PVOID Context,
    struct _RTL_RANGE *Range
);

//
// Custom Heap Commit Routine for RtlCreateHeap
//
typedef NTSTATUS
(NTAPI * PRTL_HEAP_COMMIT_ROUTINE)(
    IN PVOID Base,
    IN OUT PVOID *CommitAddress,
    IN OUT PSIZE_T CommitSize
);

//
// Version Info redefinitions
//
typedef OSVERSIONINFOW RTL_OSVERSIONINFOW;
typedef LPOSVERSIONINFOW PRTL_OSVERSIONINFOW;
typedef OSVERSIONINFOEXW RTL_OSVERSIONINFOEXW;
typedef LPOSVERSIONINFOEXW PRTL_OSVERSIONINFOEXW;

//
// Simple pointer definitions
//
typedef ACL_REVISION_INFORMATION *PACL_REVISION_INFORMATION;
typedef ACL_SIZE_INFORMATION *PACL_SIZE_INFORMATION;

//
// Parameters for RtlCreateHeap
//
typedef struct _RTL_HEAP_PARAMETERS
{
    ULONG Length;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T MaximumAllocationSize;
    SIZE_T VirtualMemoryThreshold;
    SIZE_T InitialCommit;
    SIZE_T InitialReserve;
    PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    SIZE_T Reserved[2];
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;

//
// RTL Bitmap structures
//
typedef struct _RTL_BITMAP
{
    ULONG SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

typedef struct _RTL_BITMAP_RUN
{
    ULONG StartingIndex;
    ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

//
// RtlGenerateXxxName context
//
typedef struct _GENERATE_NAME_CONTEXT
{
    USHORT Checksum;
    BOOLEAN CheckSumInserted;
    UCHAR NameLength;
    WCHAR NameBuffer[8];
    ULONG ExtensionLength;
    WCHAR ExtensionBuffer[4];
    ULONG LastIndexValue;
} GENERATE_NAME_CONTEXT, *PGENERATE_NAME_CONTEXT;

//
// RTL Splay and Balanced Links structures
//
typedef struct _RTL_SPLAY_LINKS
{
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

typedef struct _RTL_BALANCED_LINKS
{
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS, *PRTL_BALANCED_LINKS;

//
// RTL Avl/Generic Tables
//
typedef struct _RTL_GENERIC_TABLE
{
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    PLIST_ENTRY OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_GENERIC_TABLE, *PRTL_GENERIC_TABLE;

typedef struct _RTL_AVL_TABLE
{
    RTL_BALANCED_LINKS BalancedRoot;
    PVOID OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_AVL_TABLE, *PRTL_AVL_TABLE;

//
// RtlQueryRegistry Data
//
typedef struct _RTL_QUERY_REGISTRY_TABLE
{
    PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
    ULONG Flags;
    PWSTR Name;
    PVOID EntryContext;
    ULONG DefaultType;
    PVOID DefaultData;
    ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

//
// RTL Unicode Table Structures
//
typedef struct _UNICODE_PREFIX_TABLE_ENTRY
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    struct _UNICODE_PREFIX_TABLE_ENTRY *NextPrefixTree;
    struct _UNICODE_PREFIX_TABLE_ENTRY *CaseMatch;
    RTL_SPLAY_LINKS Links;
    PUNICODE_STRING Prefix;
} UNICODE_PREFIX_TABLE_ENTRY, *PUNICODE_PREFIX_TABLE_ENTRY;

typedef struct _UNICODE_PREFIX_TABLE
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    PUNICODE_PREFIX_TABLE_ENTRY NextPrefixTree;
    PUNICODE_PREFIX_TABLE_ENTRY LastNextEntry;
} UNICODE_PREFIX_TABLE, *PUNICODE_PREFIX_TABLE;

//
// Time Structure for RTL Time calls
//
typedef struct _TIME_FIELDS
{
    CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;
#endif

//
// ACE Definition
//
typedef struct _ACE
{
    ACE_HEADER Header;
    ACCESS_MASK AccessMask;
} ACE, *PACE;

//
// Information Structures for RTL Debug Functions
//
typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    ULONG Reserved[2];
    PVOID Base;
    ULONG Size;
    ULONG Flags;
    USHORT Index;
    USHORT Unknown;
    USHORT LoadCount;
    USHORT ModuleNameOffset;
    CHAR ImageName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG ModuleCount;
    RTL_PROCESS_MODULE_INFORMATION ModuleEntry[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

typedef struct _RTL_PROCESS_HEAP_INFORMATION
{
    PVOID Base;
    ULONG Flags;
    USHORT Granularity;
    USHORT Unknown;
    ULONG Allocated;
    ULONG Committed;
    ULONG TagCount;
    ULONG BlockCount;
    ULONG Reserved[7];
    PVOID Tags;
    PVOID Blocks;
} RTL_PROCESS_HEAP_INFORMATION, *PRTL_PROCESS_HEAP_INFORMATION;

typedef struct _RTL_PROCESS_HEAPS
{
    ULONG HeapCount;
    RTL_PROCESS_HEAP_INFORMATION HeapEntry[1];
} RTL_PROCESS_HEAPS, *PRTL_PROCESS_HEAPS;

typedef struct _RTL_PROCESS_LOCK_INFORMATION
{
    PVOID Address;
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    ULONG OwnerThreadId;
    ULONG ActiveCount;
    ULONG ContentionCount;
    ULONG EntryCount;
    ULONG RecursionCount;
    ULONG NumberOfSharedWaiters;
    ULONG NumberOfExclusiveWaiters;
} RTL_PROCESS_LOCK_INFORMATION, *PRTL_PROCESS_LOCK_INFORMATION;

typedef struct _RTL_PROCESS_LOCKS
{
    ULONG LockCount;
    RTL_PROCESS_LOCK_INFORMATION LockEntry[1];
} RTL_PROCESS_LOCKS, *PRTL_PROCESS_LOCKS;

typedef struct _RTL_PROCESS_BACKTRACE_INFORMATION
{
    /* FIXME */
    ULONG Unknown;
} RTL_PROCESS_BACKTRACE_INFORMATION, *PRTL_PROCESS_BACKTRACE_INFORMATION;

typedef struct _RTL_PROCESS_BACKTRACES
{
    ULONG BackTraceCount;
    RTL_PROCESS_BACKTRACE_INFORMATION BackTraceEntry[1];
} RTL_PROCESS_BACKTRACES, *PRTL_PROCESS_BACKTRACES;

typedef struct _RTL_DEBUG_BUFFER
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
    PRTL_PROCESS_MODULES ModuleInformation;
    PRTL_PROCESS_BACKTRACES BackTraceInformation;
    PRTL_PROCESS_HEAPS HeapInformation;
    PRTL_PROCESS_LOCKS LockInformation;
    PVOID Reserved[8];
} RTL_DEBUG_BUFFER, *PRTL_DEBUG_BUFFER;

//
// RTL Handle Structures
//
typedef struct _RTL_HANDLE_TABLE_ENTRY
{
    ULONG Flags;
    struct _RTL_HANDLE_TABLE_ENTRY *NextFree;
} RTL_HANDLE_TABLE_ENTRY, *PRTL_HANDLE_TABLE_ENTRY;

typedef struct _RTL_HANDLE_TABLE
{
    ULONG MaximumNumberOfHandles;
    ULONG SizeOfHandleTableEntry;
    ULONG Reserved[2];
    PRTL_HANDLE_TABLE_ENTRY FreeHandles;
    PRTL_HANDLE_TABLE_ENTRY CommittedHandles;
    PRTL_HANDLE_TABLE_ENTRY UnCommittedHandles;
    PRTL_HANDLE_TABLE_ENTRY MaxReservedHandles;
} RTL_HANDLE_TABLE, *PRTL_HANDLE_TABLE;

//
// Exception Record
//
typedef struct _EXCEPTION_REGISTRATION_RECORD
{
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

//
// Current Directory Structures
//
typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

#ifndef NTOS_MODE_USER

//
// RTL Critical Section Structures
//
typedef struct _RTL_CRITICAL_SECTION_DEBUG
{
    USHORT Type;
    USHORT CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION *CriticalSection;
    LIST_ENTRY ProcessLocksList;
    ULONG EntryCount;
    ULONG ContentionCount;
    ULONG Spare[2];
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG, RTL_RESOURCE_DEBUG, *PRTL_RESOURCE_DEBUG;

typedef struct _RTL_CRITICAL_SECTION
{
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;

#else

//
// RTL Range List Structures
//
typedef struct _RTL_RANGE_LIST
{
    LIST_ENTRY ListHead;
    ULONG Flags;
    ULONG Count;
    ULONG Stamp;
} RTL_RANGE_LIST, *PRTL_RANGE_LIST;

typedef struct _RTL_RANGE
{
    ULONGLONG Start;
    ULONGLONG End;
    PVOID UserData;
    PVOID Owner;
    UCHAR Attributes;
    UCHAR Flags;
} RTL_RANGE, *PRTL_RANGE;

typedef struct _RANGE_LIST_ITERATOR
{
    PLIST_ENTRY RangeListHead;
    PLIST_ENTRY MergedHead;
    PVOID Current;
    ULONG Stamp;
} RTL_RANGE_LIST_ITERATOR, *PRTL_RANGE_LIST_ITERATOR;

#endif

//
// RTL Resource
//
typedef struct _RTL_RESOURCE
{
    RTL_CRITICAL_SECTION Lock;
    HANDLE SharedSemaphore;
    ULONG SharedWaiters;
    HANDLE ExclusiveSemaphore;
    ULONG ExclusiveWaiters;
    LONG NumberActive;
    HANDLE OwningThread;
    ULONG TimeoutBoost;
    PVOID DebugInfo;
} RTL_RESOURCE, *PRTL_RESOURCE;

//
// RTL Message Structures for PE Resources
//
typedef struct _RTL_MESSAGE_RESOURCE_ENTRY
{
    USHORT Length;
    USHORT Flags;
    CHAR Text[1];
} RTL_MESSAGE_RESOURCE_ENTRY, *PRTL_MESSAGE_RESOURCE_ENTRY;

typedef struct _RTL_MESSAGE_RESOURCE_BLOCK
{
    ULONG LowId;
    ULONG HighId;
    ULONG OffsetToEntries;
} RTL_MESSAGE_RESOURCE_BLOCK, *PRTL_MESSAGE_RESOURCE_BLOCK;

typedef struct _RTL_MESSAGE_RESOURCE_DATA
{
    ULONG NumberOfBlocks;
    RTL_MESSAGE_RESOURCE_BLOCK Blocks[1];
} RTL_MESSAGE_RESOURCE_DATA, *PRTL_MESSAGE_RESOURCE_DATA;

//
// Structures for RtlCreateUserProcess
//
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;
    ULONG Flags;
    ULONG DebugFlags;
    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;
    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PWSTR Environment;
    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;
    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[32];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _RTL_USER_PROCESS_INFORMATION
{
    ULONG Size;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
    SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

//
// RTL Atom Table Structures
//
typedef struct _RTL_ATOM_TABLE_ENTRY
{
    struct _RTL_ATOM_TABLE_ENTRY *HashLink;
    USHORT HandleIndex;
    USHORT Atom;
    USHORT ReferenceCount;
    UCHAR Flags;
    UCHAR NameLength;
    WCHAR Name[1];
} RTL_ATOM_TABLE_ENTRY, *PRTL_ATOM_TABLE_ENTRY;

typedef struct _RTL_ATOM_TABLE
{
    ULONG Signature;
    union
    {
#ifdef NTOS_MODE_USER
        RTL_CRITICAL_SECTION CriticalSection;
#else
        FAST_MUTEX FastMutex;
#endif
    };
    union
    {
#ifdef NTOS_MODE_USER
        RTL_HANDLE_TABLE RtlHandleTable;
#else
        PHANDLE_TABLE ExHandleTable;
#endif
    };
    ULONG NumberOfBuckets;
    PRTL_ATOM_TABLE_ENTRY Buckets[1];
} RTL_ATOM_TABLE, *PRTL_ATOM_TABLE;

#ifndef NTOS_MODE_USER

//
// System Time and Timezone Structures
//
typedef struct _SYSTEMTIME
{
    USHORT wYear;
    USHORT wMonth;
    USHORT wDayOfWeek;
    USHORT wDay;
    USHORT wHour;
    USHORT wMinute;
    USHORT wSecond;
    USHORT wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

typedef struct _TIME_ZONE_INFORMATION
{
    LONG Bias;
    WCHAR StandardName[32];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[32];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

#endif

//
// Header for NLS Files
//
typedef struct _NLS_FILE_HEADER
{
    USHORT HeaderSize;
    USHORT CodePage;
    USHORT MaximumCharacterSize;
    USHORT DefaultChar;
    USHORT UniDefaultChar;
    USHORT TransDefaultChar;
    USHORT TransUniDefaultChar;
    USHORT DBCSCodePage;
    UCHAR LeadByte[MAXIMUM_LEADBYTES];
} NLS_FILE_HEADER, *PNLS_FILE_HEADER;

#endif
