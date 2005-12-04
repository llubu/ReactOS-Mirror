/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    pstypes.h

Abstract:

    Type definitions for the Process Manager

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _PSTYPES_H
#define _PSTYPES_H

//
// Dependencies
//
#include <umtypes.h>
#include <ldrtypes.h>
#include <mmtypes.h>
#include <obtypes.h>
#ifndef NTOS_MODE_USER
#include <extypes.h>
#include <setypes.h>
#endif

//
// KUSER_SHARED_DATA location in User Mode
//
#define USER_SHARED_DATA                        (0x7FFE0000)

//
// Kernel Exports
//
#ifndef NTOS_MODE_USER

extern NTSYSAPI struct _EPROCESS* PsInitialSystemProcess;
extern NTSYSAPI POBJECT_TYPE PsProcessType;

#endif

//
// Global Flags
//
#define FLG_STOP_ON_EXCEPTION                   0x00000001
#define FLG_SHOW_LDR_SNAPS                      0x00000002
#define FLG_DEBUG_INITIAL_COMMAND               0x00000004
#define FLG_STOP_ON_HUNG_GUI                    0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK              0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK              0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS            0x00000040
#define FLG_HEAP_VALIDATE_ALL                   0x00000080
#define FLG_POOL_ENABLE_TAIL_CHECK              0x00000100
#define FLG_POOL_ENABLE_FREE_CHECK              0x00000200
#define FLG_POOL_ENABLE_TAGGING                 0x00000400
#define FLG_HEAP_ENABLE_TAGGING                 0x00000800
#define FLG_USER_STACK_TRACE_DB                 0x00001000
#define FLG_KERNEL_STACK_TRACE_DB               0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST            0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL              0x00008000
#define FLG_IGNORE_DEBUG_PRIV                   0x00010000
#define FLG_ENABLE_CSRDEBUG                     0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD           0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS          0x00080000
#define FLG_HEAP_ENABLE_CALL_TRACING            0x00100000
#define FLG_HEAP_DISABLE_COALESCING             0x00200000
#define FLG_ENABLE_CLOSE_EXCEPTIONS             0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING            0x00800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING          0x01000000
#define FLG_HEAP_PAGE_ALLOCS                    0x02000000
#define FLG_DEBUG_INITIAL_COMMAND_EX            0x04000000

//
// Process priority classes
//
#define PROCESS_PRIORITY_CLASS_INVALID          0
#define PROCESS_PRIORITY_CLASS_IDLE             1
#define PROCESS_PRIORITY_CLASS_NORMAL           2
#define PROCESS_PRIORITY_CLASS_HIGH             3
#define PROCESS_PRIORITY_CLASS_REALTIME         4
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL     5
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL     6

//
// Process base priorities
//
#define PROCESS_PRIORITY_IDLE                   3
#define PROCESS_PRIORITY_NORMAL                 8
#define PROCESS_PRIORITY_NORMAL_FOREGROUND      9

#if 0
//
// Job Access Types
//
#define JOB_OBJECT_ASSIGN_PROCESS               0x1
#define JOB_OBJECT_SET_ATTRIBUTES               0x2
#define JOB_OBJECT_QUERY                        0x4
#define JOB_OBJECT_TERMINATE                    0x8
#define JOB_OBJECT_SET_SECURITY_ATTRIBUTES      0x10
#define JOB_OBJECT_ALL_ACCESS                   (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 31)
#endif

#ifdef NTOS_MODE_USER
//
// Current Process/Thread built-in 'special' handles
//
#define NtCurrentProcess()                      ((HANDLE)(LONG_PTR)-1)
#define ZwCurrentProcess()                      NtCurrentProcess()
#define NtCurrentThread()                       ((HANDLE)(LONG_PTR)-2)
#define ZwCurrentThread()                       NtCurrentThread()

//
// Process/Thread/Job Information Classes for NtQueryInformationProcess/Thread/Job
//
typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    ProcessImageFileName,
    ProcessLUIDDeviceMapsEnabled,
    ProcessBreakOnTermination,
    ProcessDebugObjectHandle,
    ProcessDebugFlags,
    ProcessHandleTracing,
    ProcessIoPriority,
    ProcessExecuteFlags,
    ProcessTlsInformation,
    ProcessCookie,
    ProcessImageInformation,
    ProcessCycleTime,
    ProcessPagePriority,
    ProcessInstrumentationCallback,
    MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS
{
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    ThreadBreakOnTermination,
    ThreadSwitchLegacyState,
    ThreadIsTerminated,
    ThreadLastSystemCall,
    ThreadIoPriority,
    ThreadCycleTime,
    ThreadPagePriority,
    ThreadActualBasePriority,
    MaxThreadInfoClass
} THREADINFOCLASS;

#else

typedef enum _JOBOBJECTINFOCLASS
{
    JobObjectBasicAccountingInformation = 1,
    JobObjectBasicLimitInformation,
    JobObjectBasicProcessIdList,
    JobObjectBasicUIRestrictions,
    JobObjectSecurityLimitInformation,
    JobObjectEndOfJobTimeInformation,
    JobObjectAssociateCompletionPortInformation,
    JobObjectBasicAndIoAccountingInformation,
    JobObjectExtendedLimitInformation,
    JobObjectJobSetInformation,
    MaxJobObjectInfoClass
} JOBOBJECTINFOCLASS;

//
// Declare empty structure definitions so that they may be referenced by
// routines before they are defined
//
struct _W32THREAD;
struct _W32PROCESS;
struct _ETHREAD;

//
// Win32K Process and Thread Callbacks
//
typedef NTSTATUS
(NTAPI *PW32_PROCESS_CALLBACK)(
    struct _EPROCESS *Process,
    BOOLEAN Create
);

typedef NTSTATUS
(NTAPI *PW32_THREAD_CALLBACK)(
    struct _ETHREAD *Thread,
    BOOLEAN Create
);

#endif

#ifdef NTOS_MODE_USER

//
// ClientID Structure
//
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

#endif

//
// Descriptor Table Entry Definition
//
#define _DESCRIPTOR_TABLE_ENTRY_DEFINED
typedef struct _DESCRIPTOR_TABLE_ENTRY
{
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;

//
// PEB Lock Routine
//
typedef VOID
(NTAPI *PPEBLOCKROUTINE)(
    PVOID PebLock
);

//
// PEB Free Block Descriptor
//
typedef struct _PEB_FREE_BLOCK
{
    struct _PEB_FREE_BLOCK* Next;
    ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

//
// Process Environment Block (PEB)
//
typedef struct _PEB
{
    UCHAR InheritedAddressSpace;                     /* 00h */
    UCHAR ReadImageFileExecOptions;                  /* 01h */
    UCHAR BeingDebugged;                             /* 02h */
    BOOLEAN SpareBool;                               /* 03h */
    HANDLE Mutant;                                   /* 04h */
    PVOID ImageBaseAddress;                          /* 08h */
    PPEB_LDR_DATA Ldr;                               /* 0Ch */
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;  /* 10h */
    PVOID SubSystemData;                             /* 14h */
    PVOID ProcessHeap;                               /* 18h */
    PVOID FastPebLock;                               /* 1Ch */
    PPEBLOCKROUTINE FastPebLockRoutine;              /* 20h */
    PPEBLOCKROUTINE FastPebUnlockRoutine;            /* 24h */
    ULONG EnvironmentUpdateCount;                    /* 28h */
    PVOID* KernelCallbackTable;                      /* 2Ch */
    PVOID EventLogSection;                           /* 30h */
    PVOID EventLog;                                  /* 34h */
    PPEB_FREE_BLOCK FreeList;                        /* 38h */
    ULONG TlsExpansionCounter;                       /* 3Ch */
    PVOID TlsBitmap;                                 /* 40h */
    ULONG TlsBitmapBits[0x2];                        /* 44h */
    PVOID ReadOnlySharedMemoryBase;                  /* 4Ch */
    PVOID ReadOnlySharedMemoryHeap;                  /* 50h */
    PVOID* ReadOnlyStaticServerData;                 /* 54h */
    PVOID AnsiCodePageData;                          /* 58h */
    PVOID OemCodePageData;                           /* 5Ch */
    PVOID UnicodeCaseTableData;                      /* 60h */
    ULONG NumberOfProcessors;                        /* 64h */
    ULONG NtGlobalFlag;                              /* 68h */
    LARGE_INTEGER CriticalSectionTimeout;            /* 70h */
    ULONG HeapSegmentReserve;                        /* 78h */
    ULONG HeapSegmentCommit;                         /* 7Ch */
    ULONG HeapDeCommitTotalFreeThreshold;            /* 80h */
    ULONG HeapDeCommitFreeBlockThreshold;            /* 84h */
    ULONG NumberOfHeaps;                             /* 88h */
    ULONG MaximumNumberOfHeaps;                      /* 8Ch */
    PVOID* ProcessHeaps;                             /* 90h */
    PVOID GdiSharedHandleTable;                      /* 94h */
    PVOID ProcessStarterHelper;                      /* 98h */
    PVOID GdiDCAttributeList;                        /* 9Ch */
    PVOID LoaderLock;                                /* A0h */
    ULONG OSMajorVersion;                            /* A4h */
    ULONG OSMinorVersion;                            /* A8h */
    USHORT OSBuildNumber;                            /* ACh */
    USHORT OSCSDVersion;                             /* AEh */
    ULONG OSPlatformId;                              /* B0h */
    ULONG ImageSubSystem;                            /* B4h */
    ULONG ImageSubSystemMajorVersion;                /* B8h */
    ULONG ImageSubSystemMinorVersion;                /* BCh */
    ULONG ImageProcessAffinityMask;                  /* C0h */
    ULONG GdiHandleBuffer[0x22];                     /* C4h */
    PVOID PostProcessInitRoutine;                    /* 14Ch */
    struct _RTL_BITMAP *TlsExpansionBitmap;          /* 150h */
    ULONG TlsExpansionBitmapBits[0x20];              /* 154h */
    ULONG SessionId;                                 /* 1D4h */
    PVOID AppCompatInfo;                             /* 1D8h */
    UNICODE_STRING CSDVersion;                       /* 1DCh */
} PEB, *PPEB;

//
// GDI Batch Descriptor
//
typedef struct _GDI_TEB_BATCH
{
    ULONG Offset;
    ULONG HDC;
    ULONG Buffer[0x136];
} GDI_TEB_BATCH, *PGDI_TEB_BATCH;

//
// Initial TEB
//
typedef struct _INITIAL_TEB
{
    PVOID PreviousStackBase;
    PVOID PreviousStackLimit;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID AllocatedStackBase;
} INITIAL_TEB, *PINITIAL_TEB;

//
// TEB Active Frame Structures
//
typedef struct _TEB_ACTIVE_FRAME_CONTEXT 
{
    ULONG Flags;
    LPSTR FrameName;
} TEB_ACTIVE_FRAME_CONTEXT, *PTEB_ACTIVE_FRAME_CONTEXT;

typedef struct _TEB_ACTIVE_FRAME
{
    ULONG Flags;
    struct _TEB_ACTIVE_FRAME *Previous;
    PTEB_ACTIVE_FRAME_CONTEXT Context;
} TEB_ACTIVE_FRAME, *PTEB_ACTIVE_FRAME;

//
// Thread Environment Block (TEB)
//
typedef struct _TEB
{
    NT_TIB Tib;                             /* 00h */
    PVOID EnvironmentPointer;               /* 1Ch */
    CLIENT_ID Cid;                          /* 20h */
    PVOID ActiveRpcHandle;                  /* 28h */
    PVOID ThreadLocalStoragePointer;        /* 2Ch */
    struct _PEB *ProcessEnvironmentBlock;   /* 30h */
    ULONG LastErrorValue;                   /* 34h */
    ULONG CountOfOwnedCriticalSections;     /* 38h */
    PVOID CsrClientThread;                  /* 3Ch */
    struct _W32THREAD* Win32ThreadInfo;     /* 40h */
    ULONG User32Reserved[0x1A];             /* 44h */
    ULONG UserReserved[5];                  /* ACh */
    PVOID WOW32Reserved;                    /* C0h */
    LCID CurrentLocale;                     /* C4h */
    ULONG FpSoftwareStatusRegister;         /* C8h */
    PVOID SystemReserved1[0x36];            /* CCh */
    LONG ExceptionCode;                     /* 1A4h */
    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer; /* 1A8h */
    UCHAR SpareBytes1[0x28];                /* 1ACh */
    GDI_TEB_BATCH GdiTebBatch;              /* 1D4h */
    CLIENT_ID RealClientId;                 /* 6B4h */
    PVOID GdiCachedProcessHandle;           /* 6BCh */
    ULONG GdiClientPID;                     /* 6C0h */
    ULONG GdiClientTID;                     /* 6C4h */
    PVOID GdiThreadLocalInfo;               /* 6C8h */
    ULONG Win32ClientInfo[62];              /* 6CCh */
    PVOID glDispatchTable[0xE9];            /* 7C4h */
    ULONG glReserved1[0x1D];                /* B68h */
    PVOID glReserved2;                      /* BDCh */
    PVOID glSectionInfo;                    /* BE0h */
    PVOID glSection;                        /* BE4h */
    PVOID glTable;                          /* BE8h */
    PVOID glCurrentRC;                      /* BECh */
    PVOID glContext;                        /* BF0h */
    NTSTATUS LastStatusValue;               /* BF4h */
    UNICODE_STRING StaticUnicodeString;     /* BF8h */
    WCHAR StaticUnicodeBuffer[0x105];       /* C00h */
    PVOID DeallocationStack;                /* E0Ch */
    PVOID TlsSlots[0x40];                   /* E10h */
    LIST_ENTRY TlsLinks;                    /* F10h */
    PVOID Vdm;                              /* F18h */
    PVOID ReservedForNtRpc;                 /* F1Ch */
    PVOID DbgSsReserved[0x2];               /* F20h */
    ULONG HardErrorDisabled;                /* F28h */
    PVOID Instrumentation[14];              /* F2Ch */
    PVOID SubProcessTag;                    /* F64h */
    PVOID EtwTraceData;                     /* F68h */
    PVOID WinSockData;                      /* F6Ch */
    ULONG GdiBatchCount;                    /* F70h */
    BOOLEAN InDbgPrint;                     /* F74h */
    BOOLEAN FreeStackOnTermination;         /* F75h */
    BOOLEAN HasFiberData;                   /* F76h */
    UCHAR IdealProcessor;                   /* F77h */
    ULONG GuaranteedStackBytes;             /* F78h */
    PVOID ReservedForPerf;                  /* F7Ch */
    PVOID ReservedForOle;                   /* F80h */
    ULONG WaitingOnLoaderLock;              /* F84h */
    ULONG SparePointer1;                    /* F88h */
    ULONG SoftPatchPtr1;                    /* F8Ch */
    ULONG SoftPatchPtr2;                    /* F90h */
    PVOID *TlsExpansionSlots;               /* F94h */
    ULONG ImpersionationLocale;             /* F98h */
    ULONG IsImpersonating;                  /* F9Ch */
    PVOID NlsCache;                         /* FA0h */
    PVOID pShimData;                        /* FA4h */
    ULONG HeapVirualAffinity;               /* FA8h */
    PVOID CurrentTransactionHandle;         /* FACh */
    PTEB_ACTIVE_FRAME ActiveFrame;          /* FB0h */
    PVOID FlsData;                          /* FB4h */
    UCHAR SafeThunkCall;                    /* FB8h */
    UCHAR BooleanSpare[3];                  /* FB9h */
} TEB, *PTEB;

#ifdef NTOS_MODE_USER

//
// Process Information Structures for NtQueryProcessInformation
//
typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION,*PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_ACCESS_TOKEN
{
    HANDLE Token;
    HANDLE Thread;
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    union
    {
        struct
        {
            HANDLE DirectoryHandle;
        } Set;
        struct
        {
            ULONG DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

typedef struct _KERNEL_USER_TIMES
{
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

typedef struct _PROCESS_SESSION_INFORMATION
{
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

#endif

typedef struct _PROCESS_PRIORITY_CLASS
{
    BOOLEAN Foreground;
    UCHAR   PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

//
// Thread Information Structures for NtQueryProcessInformation
//
typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

#ifndef NTOS_MODE_USER

//
// EPROCESS Quota Structures
//
typedef struct _EPROCESS_QUOTA_ENTRY
{
    SIZE_T Usage;
    SIZE_T Limit;
    SIZE_T Peak;
    SIZE_T Return;
} EPROCESS_QUOTA_ENTRY, *PEPROCESS_QUOTA_ENTRY;

typedef struct _EPROCESS_QUOTA_BLOCK
{
    EPROCESS_QUOTA_ENTRY QuotaEntry[3];
    LIST_ENTRY QuotaList;
    ULONG ReferenceCount;
    ULONG ProcessCount;
} EPROCESS_QUOTA_BLOCK, *PEPROCESS_QUOTA_BLOCK;

//
// FIXME: This really belongs in mmtypes.h
//
typedef struct _PAGEFAULT_HISTORY
{
    ULONG CurrentIndex;
    ULONG MapIndex;
    KSPIN_LOCK SpinLock;
    PVOID Reserved;
    PROCESS_WS_WATCH_INFORMATION WatchInfo[1];
} PAGEFAULT_HISTORY, *PPAGEFAULT_HISTORY;

//
// Process Impersonation Information
//
typedef struct _PS_IMPERSONATION_INFORMATION
{
    PACCESS_TOKEN Token;
    BOOLEAN CopyOnOpen;
    BOOLEAN EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
} PS_IMPERSONATION_INFORMATION, *PPS_IMPERSONATION_INFORMATION;

//
// Process Termination Port
//
typedef struct _TERMINATION_PORT
{
    struct _TERMINATION_PORT *Next;
    PVOID Port;
} TERMINATION_PORT, *PTERMINATION_PORT;

//
// Executive Thread (ETHREAD)
//
#include <pshpack4.h>
typedef struct _ETHREAD
{
    KTHREAD                        Tcb;                         /* 1C0 */
    LARGE_INTEGER                  CreateTime;                  /* 1C0 */
    LARGE_INTEGER                  ExitTime;                    /* 1C0 */
    union
    {
        LIST_ENTRY                 LpcReplyChain;               /* 1C0 */
        LIST_ENTRY                 KeyedWaitChain;              /* 1C0 */
    };
    union
    {
        NTSTATUS                   ExitStatus;                  /* 1C8 */
        PVOID                      OfsChain;                    /* 1C8 */
    };
    LIST_ENTRY                     PostBlockList;               /* 1CC */
    union
    {
        struct _TERMINATION_PORT   *TerminationPort;            /* 1D4 */
        struct _ETHREAD            *ReaperLink;                 /* 1D4 */
        PVOID                      KeyedWaitValue;              /* 1D4 */
    };
    KSPIN_LOCK                     ActiveTimerListLock;         /* 1D8 */
    LIST_ENTRY                     ActiveTimerListHead;         /* 1D8 */
    CLIENT_ID                      Cid;                         /* 1E0 */
    union
    {
        KSEMAPHORE                 LpcReplySemaphore;           /* 1E4 */
        KSEMAPHORE                 KeyedReplySemaphore;         /* 1E4 */
    };
    union
    {
        PVOID                      LpcReplyMessage;             /* 200 */
        PVOID                      LpcWaitingOnPort;            /* 200 */
    };
    PPS_IMPERSONATION_INFORMATION  ImpersonationInfo;           /* 204 */
    LIST_ENTRY                     IrpList;                     /* 208 */
    ULONG                          TopLevelIrp;                 /* 210 */
    PDEVICE_OBJECT                 DeviceToVerify;              /* 214 */
    struct _EPROCESS               *ThreadsProcess;             /* 218 */
    PKSTART_ROUTINE                StartAddress;                /* 21C */
    union
    {
        PVOID                      Win32StartAddress;           /* 220 */
        ULONG                      LpcReceivedMessageId;        /* 220 */
    };
    LIST_ENTRY                     ThreadListEntry;             /* 224 */
    EX_RUNDOWN_REF                 RundownProtect;              /* 22C */
    EX_PUSH_LOCK                   ThreadLock;                  /* 230 */
    ULONG                          LpcReplyMessageId;           /* 234 */
    ULONG                          ReadClusterSize;             /* 238 */
    ACCESS_MASK                    GrantedAccess;               /* 23C */
    union
    {
        struct
        {
           ULONG                   Terminated:1;
           ULONG                   DeadThread:1;
           ULONG                   HideFromDebugger:1;
           ULONG                   ActiveImpersonationInfo:1;
           ULONG                   SystemThread:1;
           ULONG                   HardErrorsAreDisabled:1;
           ULONG                   BreakOnTermination:1;
           ULONG                   SkipCreationMsg:1;
           ULONG                   SkipTerminationMsg:1;
        };
        ULONG                      CrossThreadFlags;            /* 240 */
    };
    union
    {
        struct
        {
           ULONG                   ActiveExWorker:1;
           ULONG                   ExWorkerCanWaitUser:1;
           ULONG                   MemoryMaker:1;
           ULONG                   KeyedEventInUse:1;
        };
        ULONG                      SameThreadPassiveFlags;      /* 244 */
    };
    union
    {
        struct
        {
           ULONG                   LpcReceivedMsgIdValid:1;
           ULONG                   LpcExitThreadCalled:1;
           ULONG                   AddressSpaceOwner:1;
           ULONG                   OwnsProcessWorkingSetExclusive:1;
           ULONG                   OwnsProcessWorkingSetShared:1;
           ULONG                   OwnsSystemWorkingSetExclusive:1;
           ULONG                   OwnsSystemWorkingSetShared:1;
           ULONG                   OwnsSessionWorkingSetExclusive:1;
           ULONG                   OwnsSessionWorkingSetShared:1;
           ULONG                   ApcNeeded:1;
        };
        ULONG                      SameThreadApcFlags;          /* 248 */
    };
    UCHAR                          ForwardClusterOnly;          /* 24C */
    UCHAR                          DisablePageFaultClustering;  /* 24D */
    UCHAR                          ActiveFaultCount;            /* 24E */
} ETHREAD;

#if defined(_NTOSKRNL_)
    #include <internal/mm.h>
#endif

//
// Executive Process (EPROCESS)
//
typedef struct _EPROCESS
{
    KPROCESS              Pcb;                          /* 000 */
    EX_PUSH_LOCK          ProcessLock;                  /* 078 */
    LARGE_INTEGER         CreateTime;                   /* 080 */
    LARGE_INTEGER         ExitTime;                     /* 088 */
    EX_RUNDOWN_REF        RundownProtect;               /* 090 */
    HANDLE                UniqueProcessId;              /* 094 */
    LIST_ENTRY            ActiveProcessLinks;           /* 098 */
    ULONG                 QuotaUsage[3];                /* 0A0 */
    ULONG                 QuotaPeak[3];                 /* 0AC */
    ULONG                 CommitCharge;                 /* 0B8 */
    ULONG                 PeakVirtualSize;              /* 0BC */
    ULONG                 VirtualSize;                  /* 0C0 */
    LIST_ENTRY            SessionProcessLinks;          /* 0C4 */
    PVOID                 DebugPort;                    /* 0CC */
    PVOID                 ExceptionPort;                /* 0D0 */
    PHANDLE_TABLE         ObjectTable;                  /* 0D4 */
    EX_FAST_REF           Token;                        /* 0D8 */
    ULONG                 WorkingSetPage;               /* 0DC */
    KGUARDED_MUTEX        AddressCreationLock;          /* 0E0 */
    KSPIN_LOCK            HyperSpaceLock;               /* 100 */
    PETHREAD              ForkInProgress;               /* 104 */
    ULONG                 HardwareTrigger;              /* 108 */
    MM_AVL_TABLE          PhysicalVadroot;              /* 10C */
    PVOID                 CloneRoot;                    /* 110 */
    ULONG                 NumberOfPrivatePages;         /* 114 */
    ULONG                 NumberOfLockedPages;          /* 118 */
    PVOID                 *Win32Process;                /* 11C */
    struct _EJOB          *Job;                         /* 120 */
    PVOID                 SectionObject;                /* 124 */
    PVOID                 SectionBaseAddress;           /* 128 */
    PEPROCESS_QUOTA_BLOCK QuotaBlock;                   /* 12C */
    PPAGEFAULT_HISTORY    WorkingSetWatch;              /* 130 */
    PVOID                 Win32WindowStation;           /* 134 */
    HANDLE                InheritedFromUniqueProcessId; /* 138 */
    PVOID                 LdtInformation;               /* 13C */
    PVOID                 VadFreeHint;                  /* 140 */
    PVOID                 VdmObjects;                   /* 144 */
    PVOID                 DeviceMap;                    /* 148 */
    PVOID                 Spare0[3];                    /* 14C */
    union
    {
        HARDWARE_PTE_X86  PagedirectoryPte;             /* 158 */
        ULONGLONG         Filler;                       /* 158 */
    };
    ULONG                 Session;                      /* 160 */
    CHAR                  ImageFileName[16];            /* 164 */
    LIST_ENTRY            JobLinks;                     /* 174 */
    PVOID                 LockedPagesList;              /* 17C */
    LIST_ENTRY            ThreadListHead;               /* 184 */
    PVOID                 SecurityPort;                 /* 188 */
    PVOID                 PaeTop;                       /* 18C */
    ULONG                 ActiveThreds;                 /* 190 */
    ACCESS_MASK           GrantedAccess;                /* 194 */
    ULONG                 DefaultHardErrorProcessing;   /* 198 */
    NTSTATUS              LastThreadExitStatus;         /* 19C */
    struct _PEB*          Peb;                          /* 1A0 */
    EX_FAST_REF           PrefetchTrace;                /* 1A4 */
    LARGE_INTEGER         ReadOperationCount;           /* 1A8 */
    LARGE_INTEGER         WriteOperationCount;          /* 1B0 */
    LARGE_INTEGER         OtherOperationCount;          /* 1B8 */
    LARGE_INTEGER         ReadTransferCount;            /* 1C0 */
    LARGE_INTEGER         WriteTransferCount;           /* 1C8 */
    LARGE_INTEGER         OtherTransferCount;           /* 1D0 */
    ULONG                 CommitChargeLimit;            /* 1D8 */
    ULONG                 CommitChargePeak;             /* 1DC */
    PVOID                 AweInfo;                      /* 1E0 */
    SE_AUDIT_PROCESS_CREATION_INFO SeAuditProcessCreationInfo; /* 1E4 */
    MMSUPPORT             Vm;                           /* 1E8 */
    LIST_ENTRY            MmProcessLinks;               /* 230 */
    ULONG                 ModifiedPageCount;            /* 238 */
    ULONG                 JobStatus;                    /* 23C */
    union
    {
        struct
        {
            ULONG         CreateReported:1;
            ULONG         NoDebugInherit:1;
            ULONG         ProcessExiting:1;
            ULONG         ProcessDelete:1;
            ULONG         Wow64SplitPages:1;
            ULONG         VmDeleted:1;
            ULONG         OutswapEnabled:1;
            ULONG         Outswapped:1;
            ULONG         ForkFailed:1;
            ULONG         Wow64VaSpace4Gb:1;
            ULONG         AddressSpaceInitialized:2;
            ULONG         SetTimerResolution:1;
            ULONG         BreakOnTermination:1;
            ULONG         SessionCreationUnderway:1;
            ULONG         WriteWatch:1;
            ULONG         ProcessInSession:1;
            ULONG         OverrideAddressSpace:1;
            ULONG         HasAddressSpace:1;
            ULONG         LaunchPrefetched:1;
            ULONG         InjectInpageErrors:1;
            ULONG         VmTopDown:1;
            ULONG         ImageNotifyDone:1;
            ULONG         PdeUpdateNeeded:1;
            ULONG         VdmAllowed:1;
            ULONG         SmapAllowed:1;
            ULONG         CreateFailed:1;
            ULONG         DefaultIoPriority:3;
            ULONG         Spare1:1;
            ULONG         Spare2:1;
        };
        ULONG             Flags;                        /* 240 */
    };

    NTSTATUS              ExitStatus;                   /* 244 */
    USHORT                NextPageColor;                /* 248 */
    union
    {
        struct
        {
            UCHAR         SubSystemMinorVersion;        /* 24A */
            UCHAR         SubSystemMajorVersion;        /* 24B */
        };
        USHORT            SubSystemVersion;             /* 24A */
    };
    UCHAR                 PriorityClass;                /* 24C */
    MM_AVL_TABLE          VadRoot;                      /* 250 */
    ULONG                 Cookie;                       /* 270 */

#ifdef _REACTOS_
    /* FIXME: WILL BE DEPRECATED WITH PUSHLOCK SUPPORT IN 0.3.0*/
    KEVENT                LockEvent;                    /* 274 */
    ULONG                 LockCount;                    /* 284 */
    struct _KTHREAD       *LockOwner;                   /* 288 */

    /* FIXME: MOVE TO AVL TREES                                */
    MADDRESS_SPACE        AddressSpace;                 /* 28C */
#endif
} EPROCESS;
#include <poppack.h>

//
// Job Token Filter Data
//
#include <pshpack1.h>
typedef struct _PS_JOB_TOKEN_FILTER
{
    ULONG CapturedSidCount;
    PSID_AND_ATTRIBUTES CapturedSids;
    ULONG CapturedSidsLength;
    ULONG CapturedGroupCount;
    PSID_AND_ATTRIBUTES CapturedGroups;
    ULONG CapturedGroupsLength;
    ULONG CapturedPrivilegeCount;
    PLUID_AND_ATTRIBUTES CapturedPrivileges;
    ULONG CapturedPrivilegesLength;
} PS_JOB_TOKEN_FILTER, *PPS_JOB_TOKEN_FILTER;

//
// Executive Job (EJOB)
//
typedef struct _EJOB
{
    KEVENT Event;
    LIST_ENTRY JobLinks;
    LIST_ENTRY ProcessListHead;
    ERESOURCE JobLock;
    LARGE_INTEGER TotalUserTime;
    LARGE_INTEGER TotalKernelTime;
    LARGE_INTEGER ThisPeriodTotalUserTime;
    LARGE_INTEGER ThisPeriodTotalKernelTime;
    ULONG TotalPageFaultCount;
    ULONG TotalProcesses;
    ULONG ActiveProcesses;
    ULONG TotalTerminatedProcesses;
    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    ULONG LimitFlags;
    ULONG MinimumWorkingSetSize;
    ULONG MaximumWorkingSetSize;
    ULONG ActiveProcessLimit;
    ULONG Affinity;
    UCHAR PriorityClass;
    ULONG UIRestrictionsClass;
    ULONG SecurityLimitFlags;
    PVOID Token;
    PPS_JOB_TOKEN_FILTER Filter;
    ULONG EndOfJobTimeAction;
    PVOID CompletionPort;
    PVOID CompletionKey;
    ULONG SessionId;
    ULONG SchedulingClass;
    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
    IO_COUNTERS IoInfo;
    ULONG ProcessMemoryLimit;
    ULONG JobMemoryLimit;
    ULONG PeakProcessMemoryUsed;
    ULONG PeakJobMemoryUsed;
    ULONG CurrentJobMemoryUsed;
    KGUARDED_MUTEX MemoryLimitsLock;
    ULONG MemberLevel;
    ULONG JobFlags;
} EJOB, *PEJOB;
#include <poppack.h>

//
// Win32K Callback Registration Data
//
typedef struct _W32_CALLOUT_DATA
{
    PW32_PROCESS_CALLBACK W32ProcessCallout;
    PW32_THREAD_CALLBACK W32ThreadCallout;
    PVOID UserGlobalAtomTableCallout;
    PVOID UserPowerEventCallout;
    PVOID UserPowerStateCallout;
    PVOID UserJobCallout;
    PVOID NtGdiUserFlushUserBatch;
    OB_OPEN_METHOD DesktopOpen;
    PVOID DesktopUnmap;
    OB_DELETE_METHOD DesktopDelete;
    OB_OKAYTOCLOSE_METHOD WinstaOkayToClose;
    OB_DELETE_METHOD WinStaDelete;
    OB_PARSE_METHOD WinStaParse;
    OB_OPEN_METHOD WinStaOpen;
#ifdef _REACTOS_
    /* FIXME: REACTOS ONLY */
    OB_FIND_METHOD WinStaFind;
    OB_OPEN_METHOD WinStaCreate;
    OB_CREATE_METHOD DesktopCreate;
#endif
} W32_CALLOUT_DATA, *PW32_CALLOUT_DATA;

#endif // !NTOS_MODE_USER

#endif // _PSTYPES_H
