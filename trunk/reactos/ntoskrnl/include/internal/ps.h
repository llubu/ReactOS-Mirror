#ifndef __INCLUDE_INTERNAL_PS_H
#define __INCLUDE_INTERNAL_PS_H

#include <internal/hal.h>
#include <internal/mm.h>

extern HANDLE SystemProcessHandle;

typedef struct _KAPC_STATE
{
   LIST_ENTRY ApcListHead[2];
   struct _KPROCESS* Process;
   ULONG KernelApcInProgress;
   ULONG KernelApcPending;
   USHORT UserApcPending;
} KAPC_STATE, *PKAPC_STATE;

typedef struct _KTHREAD
{
   DISPATCHER_HEADER DispatcherHeader;    // For waiting for the thread
   LIST_ENTRY        MutantListHead;
   PVOID             InitialStack;
   ULONG             StackLimit;
   NT_TEB*           Teb;
   PVOID             TlsArray;
   PVOID             KernelStack;
   UCHAR             DebugActive;
   UCHAR             State;
   UCHAR             Alerted[2];
   UCHAR             Iopl;
   UCHAR             NpxState;
   UCHAR             Saturation;
   KPRIORITY         Priority;
   KAPC_STATE        ApcState;
   ULONG             ContextSwitches;
   ULONG             WaitStatus;
   KIRQL             WaitIrql;
   ULONG             WaitMode;
   UCHAR             WaitNext;
   UCHAR             WaitReason;
   PKWAIT_BLOCK      WaitBlockList;
   LIST_ENTRY        WaitListEntry;
   ULONG             WaitTime;
   KPRIORITY         BasePriority;
   UCHAR             DecrementCount;
   UCHAR             PriorityDecrement;
   UCHAR             Quantum;
   KWAIT_BLOCK       WaitBlock[4];
   PVOID             LegoData;         // ??
   LONG              KernelApcDisable;
   KAFFINITY         UserAffinity;
   UCHAR             SystemAffinityActive;
   UCHAR             Pad;
   PKQUEUE           Queue;    
   KSPIN_LOCK        ApcQueueLock;
   KTIMER            Timer;
   LIST_ENTRY        QueueListEntry;
   KAFFINITY         Affinity;
   UCHAR             Preempted;
   UCHAR             ProcessReadyQueue;
   UCHAR             KernelStackResident;
   UCHAR             NextProcessor;
   PVOID             CallbackStack;
   BOOL              Win32Thread;
   PVOID             TrapFrame;
   PVOID             ApcStatePointer;      // Is actually eight bytes
   UCHAR             EnableStackSwap;
   UCHAR             LargeStack;
   UCHAR             ResourceIndex;
   UCHAR             PreviousMode;
   TIME              KernelTime;
   TIME              UserTime;
   KAPC_STATE        SavedApcState;
   UCHAR             Alertable;
   UCHAR             ApcQueueable;
   ULONG             AutoAlignment;
   PVOID             StackBase;
   KAPC              SuspendApc;
   KSEMAPHORE        SuspendSemaphore;
   LIST_ENTRY        ThreadListEntry;
   CHAR             FreezeCount;
   ULONG             SuspendCount;
   UCHAR             IdealProcessor;
   UCHAR             DisableBoost;
   LIST_ENTRY        ProcessThreadListEntry;        // Added by Phillip Susi for list of threads in a process

   /* Provisionally added by David Welch */
   hal_thread_state                   Context;
   KDPC              TimerDpc;			// Added by Phillip Susi for internal KeAddThreadTimeout() impl.
} KTHREAD, *PKTHREAD;

// According to documentation the stack should have a commited [ 1 page ] and
// a reserved part [ 1 M ] but can be specified otherwise in the image file.







// TopLevelIrp can be one of the following values:
// FIXME I belong somewhere else

#define 	FSRTL_FSP_TOP_LEVEL_IRP			(0x01)
#define 	FSRTL_CACHE_TOP_LEVEL_IRP		(0x02)
#define 	FSRTL_MOD_WRITE_TOP_LEVEL_IRP		(0x03)
#define		FSRTL_FAST_IO_TOP_LEVEL_IRP		(0x04)
#define		FSRTL_MAX_TOP_LEVEL_IRP_FLAG		(0x04)

typedef struct _TOP_LEVEL_IRP
{
	PIRP TopLevelIrp;
	ULONG TopLevelIrpConst;
} TOP_LEVEL_IRP;

typedef struct
{
   PACCESS_TOKEN Token;                              // 0x0
   UCHAR Unknown1;                                   // 0x4
   UCHAR Unknown2;                                   // 0x5
   UCHAR Pad[2];                                     // 0x6
   SECURITY_IMPERSONATION_LEVEL Level;               // 0x8
} PS_IMPERSONATION_INFO, *PPS_IMPERSONATION_INFO;

struct _WIN32THREADDATA;

typedef struct _ETHREAD 
{
   KTHREAD Tcb;
   TIME	CreateTime;
   TIME	ExitTime;
   NTSTATUS ExitStatus;
   LIST_ENTRY PostBlockList;
   LIST_ENTRY TerminationPortList;  
   KSPIN_LOCK ActiveTimerListLock;
   PVOID ActiveTimerListHead;
   CLIENT_ID Cid;
   PLARGE_INTEGER LpcReplySemaphore;
   PVOID LpcReplyMessage;
   PLARGE_INTEGER LpcReplyMessageId;
   PPS_IMPERSONATION_INFO ImpersonationInfo;
   LIST_ENTRY IrpList;
   TOP_LEVEL_IRP TopLevelIrp;
   ULONG ReadClusterSize;
   UCHAR ForwardClusterOnly;
   UCHAR DisablePageFaultClustering;
   UCHAR DeadThread;
   UCHAR HasTerminated;
   ACCESS_MASK GrantedAccess;
   struct _EPROCESS* ThreadsProcess;
   PKSTART_ROUTINE StartAddress;
   LPTHREAD_START_ROUTINE Win32StartAddress; 
   UCHAR LpcExitThreadCalled;
   UCHAR HardErrorsAreDisabled;
   UCHAR LpcReceivedMsgIdValid;
   UCHAR ActiveImpersonationInfo;
   ULONG PerformanceCountHigh;

   /*
    * Added by David Welch (welch@cwcom.net)
    */
   struct _EPROCESS* OldProcess;
   struct _WIN32THREADDATA *Win32ThreadData; // Pointer to win32 private thread data

} ETHREAD, *PETHREAD;


typedef struct _KPROCESS 
{
   DISPATCHER_HEADER 	DispatcherHeader;
   PVOID		PageTableDirectory; // FIXME: I should point to a PTD
   TIME			ElapsedTime;
   TIME			KernelTime;
   TIME			UserTime;
   LIST_ENTRY		InMemoryList;  
   LIST_ENTRY		SwappedOutList;   	
   KSPIN_LOCK		SpinLock;
   KAFFINITY		Affinity;
   ULONG		StackCount;
   KPRIORITY		BasePriority;
   ULONG		DefaultThreadQuantum;
   UCHAR		ProcessState;
   ULONG		ThreadSeed;
   UCHAR		DisableBoost;
} KPROCESS, *PKPROCESS;

struct _WIN32PROCESSDATA;

typedef struct _EPROCESS
{
   KPROCESS Pcb;
   NTSTATUS ExitStatus;
   KEVENT LockEvent;
   ULONG LockCount;
   TIME CreateTime;
   TIME ExitTime;
   PVOID LockOwner;
   ULONG UniqueProcessId;
   LIST_ENTRY ActiveProcessLinks;
   ULONG QuotaPeakPoolUsage[2];
   ULONG QuotaPoolUsage[2];
   ULONG PagefileUsage;
   ULONG CommitCharge;
   ULONG PeakPagefileUsage;
   ULONG PeakVirtualUsage;
   LARGE_INTEGER VirtualSize;
   PVOID Vm;                // Actually 48 bytes
   PVOID LastProtoPteFault;
   struct _EPORT* DebugPort;
   struct _EPORT* ExceptionPort;
   PVOID ObjectTable;
   PVOID Token;
   KMUTEX WorkingSetLock;
   PVOID WorkingSetPage;
   UCHAR ProcessOutswapEnabled;
   UCHAR ProcessOutswapped;
   UCHAR AddressSpaceInitialized;
   UCHAR AddressSpaceDeleted;
   KMUTEX AddressCreationLock;
   PVOID ForkInProgress;
   PVOID VmOperation;
   PKEVENT VmOperationEvent;
   PVOID PageDirectoryPte;
   LARGE_INTEGER LastFaultCount;
   PVOID VadRoot;
   PVOID VadHint;
   PVOID CloneRoot;
   ULONG NumberOfPrivatePages;
   ULONG NumberOfLockedPages;
   UCHAR ForkWasSuccessFul;
   UCHAR ExitProcessCalled;
   UCHAR CreateProcessReported;
   HANDLE SectionHandle;
   PPEB Peb;
   PVOID SectionBaseAddress;
   PVOID QuotaBlock;
   NTSTATUS LastThreadExitStatus;
   LARGE_INTEGER WorkingSetWatch;         //
   ULONG InheritedFromUniqueProcessId;
   ACCESS_MASK GrantedAccess;
   ULONG DefaultHardErrorProcessing;
   PVOID LdtInformation;
   ULONG VadFreeHint;
   PVOID VdmObjects;
   KMUTANT ProcessMutant;
   CHAR ImageFileName[16];
   LARGE_INTEGER VmTrimFaultValue;
   struct _WIN32PROCESSDATA *Win32Process;
   
   /*
    * Added by David Welch (welch@mcmail.com)
    */
   MADDRESS_SPACE       AddressSpace;
   HANDLE_TABLE         HandleTable;
   LIST_ENTRY           ProcessListEntry;
   
   /*
    * Added by Philip Susi for list of threads in process
    */
   LIST_ENTRY           ThreadListHead;        
} EPROCESS, *PEPROCESS;

#define PROCESS_STATE_TERMINATED (1)
#define PROCESS_STATE_ACTIVE     (2)

VOID PiInitProcessManager(VOID);
VOID PiShutdownProcessManager(VOID);
VOID PsInitThreadManagment(VOID);
VOID PsInitProcessManagment(VOID);
VOID PsInitIdleThread(VOID);
VOID PsDispatchThread(ULONG NewThreadStatus);
VOID PsDispatchThreadNoLock(ULONG NewThreadStatus);
VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus);
VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus);
VOID PsReleaseThread(PETHREAD Thread);
VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext);
VOID PsBeginThreadWithContextInternal(VOID);
VOID PiKillMostProcesses(VOID);
NTSTATUS STDCALL PiTerminateProcess(PEPROCESS Process, NTSTATUS ExitStatus);
ULONG PsUnfreezeThread(PETHREAD Thread, PNTSTATUS WaitStatus);
ULONG PsFreezeThread(PETHREAD Thread, PNTSTATUS WaitStatus,
		     UCHAR Alertable, ULONG WaitMode);
VOID PiInitApcManagement(VOID);
VOID PiDeleteThread(PVOID ObjectBody);
VOID PiCloseThread(PVOID ObjectBody, ULONG HandleCount);
VOID PsReapThreads(VOID);
NTSTATUS PsInitializeThread(HANDLE ProcessHandle,
			    PETHREAD* ThreadPtr,
			    PHANDLE ThreadHandle,
			    ACCESS_MASK DesiredAccess,
			    POBJECT_ATTRIBUTES ObjectAttributes);

PACCESS_TOKEN PsReferenceEffectiveToken(PETHREAD Thread,
					PTOKEN_TYPE TokenType,
					PUCHAR b,
					PSECURITY_IMPERSONATION_LEVEL Level);

NTSTATUS PsOpenTokenOfProcess(HANDLE ProcessHandle,
			      PACCESS_TOKEN* Token);

ULONG PsSuspendThread(PETHREAD Thread,
		      PNTSTATUS WaitStatus,
		      UCHAR Alertable,
		      ULONG WaitMode);
ULONG PsResumeThread(PETHREAD Thread,
		     PNTSTATUS WaitStatus);


#define THREAD_STATE_INVALID      (0)
#define THREAD_STATE_RUNNABLE     (1)
#define THREAD_STATE_RUNNING      (2)
#define THREAD_STATE_SUSPENDED    (3)
#define THREAD_STATE_FROZEN       (4)
#define THREAD_STATE_TERMINATED_1 (5)
#define THREAD_STATE_TERMINATED_2 (6)
#define THREAD_STATE_MAX          (7)


// Internal thread priorities, added by Phillip Susi
// TODO: rebalence these to make use of all priorities... the ones above 16 can not all be used right now

#define PROCESS_PRIO_IDLE			3
#define PROCESS_PRIO_NORMAL			8
#define PROCESS_PRIO_HIGH			13
#define PROCESS_PRIO_RT				18

/*
 * Functions the HAL must provide
 */

void HalInitFirstTask(PETHREAD thread);
NTSTATUS HalInitTask(PETHREAD thread, PKSTART_ROUTINE fn, PVOID StartContext);
void HalTaskSwitch(PKTHREAD thread);
NTSTATUS HalInitTaskWithContext(PETHREAD Thread, PCONTEXT Context);
NTSTATUS HalReleaseTask(PETHREAD Thread);
VOID PiDeleteProcess(PVOID ObjectBody);
VOID PsReapThreads(VOID);
VOID PsUnfreezeOtherThread(PETHREAD Thread);
VOID PsFreezeOtherThread(PETHREAD Thread);
VOID PsFreezeProcessThreads(PEPROCESS Process);
VOID PsUnfreezeProcessThreads(PEPROCESS Process);
PEPROCESS PsGetNextProcess(PEPROCESS OldProcess);

#endif /* __INCLUDE_INTERNAL_PS_H */
