#ifndef __INCLUDE_DDK_FSFUNCS_H
#define __INCLUDE_DDK_FSFUNCS_H
/* $Id: fsfuncs.h,v 1.30 2004/11/21 17:47:22 navaraf Exp $ */
#define FlagOn(x,f) ((x) & (f))

#include <ntos/fstypes.h>

/* Some comments on the Prototypes that aren't in the GNU IFS:

The following come from alternate sources, or guessed from documentation:
FsRtlNotifyFullChangeDirectory GOOGLE GROUPS
FsRtlIsPagingFile OSR DOCUMENTATION
FsRtlAcquireFileExclusive (GUESS: The function takes a single parameter. The function name is "AcquireFile". Logical assumption says this is a File Object. NTFSD ML Post confirms.)
FsRtlReleaseFile (GUESS: The function takes a single parameter. The function name is "AcquireFile". Logical assumption says this is a File Object. NTFSD ML Post confirms.)
FsRtlMdlReadCompleteDev (FsRtlMdlReadComplete is on GNU IFS. The Dev Suffix simply means an extra PDEVICE_OBJECT param)
FsRtlMdlReadDev (FsRtlMdlReadDev is on GNU IFS. The Dev Suffix simply means an extra PDEVICE_OBJECT param)
FsRtlMdlWriteCompleteDev FsRtlMdlWriteCompleteDev is on GNU IFS. The Dev Suffix simply means an extra PDEVICE_OBJECT param)
FsRtlPrepareMdlWrite (Compared with CcMdlWrite, which is already documented)
FsRtlPrepareMdlWriteDev (Same as above, and add a pointer to device object (Dev suffix)
FsRtlGetNextMcbEntry(FsRtlGetNextLargeMcbEntry is documented and uses LONGLONGs. Logical assumption that this one only uses LONGS and non-large MCB (Documented))

Stream Context. Going along with public OSR documenttion:

FsRtlInsertPerStreamContext:
"This call is used by the file system filter driver to associate a given context block 
(allocated by the filter and initialized using FsRtlInitPerStreamContext) with the stream associated with the given file object."
Notice we are told "given context block...initialized using FsRtlInitPerStreamContext". This function description tells us:
" provide space for the FSRTL_PER_STREAM_CONTEXT block in the filter driver�s context structure"
Therefore, one of the parameters is PFSRTL_PER_STREAM_CONTEXT.
"with the stream associated with the given file object." The OSR Documentations then mentions:
"Tracking per-file (or �per stream�) context information in FSRTL_ADVANCED_FCB_HEADER"
So we are associating a FSRTL_PER_STREAM_CONTEXT block with the FSRTL_ADVANCED_FCB_HEADER associated with the file object.
FSRTL_ADVANCED_FCB_HEADER is documented by a search through Google.
FSRTL_PER_STREAM_CONTEXT is *NOT* documented anywhere else then in the IFS, so it has been removed.

FsRtlLookupPerStreamContextInternal
"FsRtlLookupPerStreamContext � this call is used by the file system filter driver to locate a given context
block that is associated with the file object.  Typically, a file system filter driver will identify its 
own context block using unique OwnerId and InstanceId parameters when creating the context block and subsequently
when locating the associated information."
OSR tells us here that the last two parameters are OwnerId and InstanceId. It also says it will find a given context block,
so there's our Return Value. (Although, not being documented, we must put PVOID). It looks into a file object's stream, so we
probably need that FCB header again.

FsRtlRemovePerStreamContext
OSR is vague, so all we know for sure is that we are sending an FCB Header. The return value isn't NTSTATUS, but seems to be a
pointer. We don't know what the two other parameters are, so they have been marked as unknown.

FsRtlTeardownPerStreamContexts
OSR doens't tell a lot, but we only have one parameter. It must be the FCB Header. Furthermore, the CVS of Captive implements
this function as a stub, and confirms the theory.

*/

#ifdef __NTOSKRNL__
extern PUCHAR EXPORTED FsRtlLegalAnsiCharacterArray;
#else
extern PUCHAR IMPORTED FsRtlLegalAnsiCharacterArray;
#endif

#define FSRTL_WILD_CHARACTER  0x08


VOID
STDCALL
FsRtlFreeFileLock(
	IN PFILE_LOCK FileLock
	);

VOID
STDCALL
FsRtlAcquireFileExclusive (
    IN PFILE_OBJECT FileObject
    );

PFILE_LOCK
STDCALL
FsRtlAllocateFileLock (
    IN PCOMPLETE_LOCK_IRP_ROUTINE   CompleteLockIrpRoutine OPTIONAL,
    IN PUNLOCK_ROUTINE              UnlockRoutine OPTIONAL
    );

#define FsRtlAreThereCurrentFileLocks(FL) (((FL)->FastIoIsQuestionable))

BOOLEAN STDCALL
FsRtlAddLargeMcbEntry(IN PLARGE_MCB Mcb,
		      IN LONGLONG Vbn,
		      IN LONGLONG Lbn,
		      IN LONGLONG SectorCount);

BOOLEAN STDCALL
FsRtlAddMcbEntry (IN PMCB     Mcb,
		  IN VBN      Vbn,
		  IN LBN      Lbn,
		  IN ULONG    SectorCount);

VOID STDCALL
FsRtlAddToTunnelCache (
    IN PTUNNEL          Cache,
    IN ULONGLONG        DirectoryKey,
    IN PUNICODE_STRING  ShortName,
    IN PUNICODE_STRING  LongName,
    IN BOOLEAN          KeyByShortName,
    IN ULONG            DataLength,
    IN PVOID            Data
    );

PVOID STDCALL
FsRtlAllocatePool (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	);

PVOID STDCALL
FsRtlAllocatePoolWithQuota (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes
	);

PVOID STDCALL
FsRtlAllocatePoolWithQuotaTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	);
PVOID
STDCALL
FsRtlAllocatePoolWithTag (
	IN	POOL_TYPE	PoolType,
	IN	ULONG		NumberOfBytes,
	IN	ULONG		Tag
	);
DWORD
STDCALL
FsRtlAllocateResource (
	VOID
	);
BOOLEAN
STDCALL
FsRtlAreNamesEqual (
	IN	PUNICODE_STRING	Name1,
	IN	PUNICODE_STRING	Name2,
	IN	BOOLEAN		IgnoreCase,
	IN	PWCHAR		UpcaseTable	OPTIONAL
	);
DWORD
STDCALL
FsRtlBalanceReads (
	DWORD	Unknown0
	);
BOOLEAN
STDCALL
FsRtlCheckLockForReadAccess (
	IN PFILE_LOCK   FileLock,
	IN PIRP         Irp
	);
BOOLEAN
STDCALL
FsRtlCheckLockForWriteAccess (
	IN PFILE_LOCK   FileLock,
	IN PIRP         Irp
	);

NTSTATUS STDCALL
FsRtlCheckOplock(IN POPLOCK Oplock,
		 IN PIRP Irp,
		 IN PVOID Context,
		 IN POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine OPTIONAL,
		 IN POPLOCK_FS_PREPOST_IRP PostIrpRoutine OPTIONAL);

BOOLEAN
STDCALL
FsRtlCopyRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	IN	ULONG			LockKey,
	OUT	PVOID			Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	);
BOOLEAN
STDCALL
FsRtlCopyWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Wait,
	IN	ULONG			LockKey,
	IN	PVOID			Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	);

BOOLEAN STDCALL
FsRtlCurrentBatchOplock(IN POPLOCK Oplock);

VOID STDCALL
FsRtlDeleteKeyFromTunnelCache(IN PTUNNEL Cache,
			      IN ULONGLONG DirectoryKey);

VOID STDCALL
FsRtlDeleteTunnelCache(IN PTUNNEL Cache);

VOID STDCALL
FsRtlDeregisterUncProvider(IN HANDLE Handle);

VOID STDCALL
FsRtlDissectDbcs(IN ANSI_STRING Name,
		 OUT PANSI_STRING FirstPart,
		 OUT PANSI_STRING RemainingPart);

VOID STDCALL
FsRtlDissectName(IN UNICODE_STRING Name,
		 OUT PUNICODE_STRING FirstPart,
		 OUT PUNICODE_STRING RemainingPart);

BOOLEAN STDCALL
FsRtlDoesDbcsContainWildCards(IN PANSI_STRING Name);

BOOLEAN STDCALL
FsRtlDoesNameContainWildCards(IN PUNICODE_STRING Name);

BOOLEAN
STDCALL
FsRtlFastCheckLockForRead (
	IN	PFILE_LOCK		FileLock,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PLARGE_INTEGER		Length,
	IN	ULONG			Key,
	IN	PFILE_OBJECT		FileObject,
	IN	PEPROCESS		Process
	);
BOOLEAN
STDCALL
FsRtlFastCheckLockForWrite (
	IN	PFILE_LOCK		FileLock,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PLARGE_INTEGER		Length,
	IN	ULONG			Key,
	IN	PFILE_OBJECT		FileObject,
	IN	PEPROCESS		Process
	);
NTSTATUS
STDCALL
FsRtlFastUnlockAll (
	IN	PFILE_LOCK           FileLock,
	IN	PFILE_OBJECT         FileObject,
	IN	PEPROCESS            Process,
	IN	PVOID                Context OPTIONAL
	);
NTSTATUS
STDCALL
FsRtlFastUnlockAllByKey (
	IN	PFILE_LOCK	FileLock,
	IN	PFILE_OBJECT	FileObject,
	IN	PEPROCESS	Process,
	IN	ULONG		Key,
	IN	PVOID		Context OPTIONAL
	);
NTSTATUS
STDCALL
FsRtlFastUnlockSingle (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN PVOID                Context OPTIONAL,
    IN BOOLEAN              AlreadySynchronized
    );
BOOLEAN
STDCALL
FsRtlFindInTunnelCache (
    IN PTUNNEL          Cache,
    IN ULONGLONG        DirectoryKey,
    IN PUNICODE_STRING  Name,
    OUT PUNICODE_STRING ShortName,
    OUT PUNICODE_STRING LongName,
    IN OUT PULONG       DataLength,
    OUT PVOID           Data
    );
NTSTATUS
STDCALL
FsRtlGetFileSize (
	IN	PFILE_OBJECT	FileObject,
	IN OUT	PLARGE_INTEGER	FileSize
	);
PFILE_LOCK_INFO
STDCALL
FsRtlGetNextFileLock (
	IN	PFILE_LOCK	FileLock,
	IN	BOOLEAN		Restart
	);

BOOLEAN STDCALL
FsRtlGetNextLargeMcbEntry(IN PLARGE_MCB Mcb,
			  IN ULONG RunIndex,
			  OUT PLONGLONG Vbn,
			  OUT PLONGLONG Lbn,
			  OUT PLONGLONG SectorCount);

BOOLEAN STDCALL
FsRtlGetNextMcbEntry (IN PMCB     Mcb,
		      IN ULONG    RunIndex,
		      OUT PVBN    Vbn,
		      OUT PLBN    Lbn,
		      OUT PULONG  SectorCount);
#define FsRtlEnterFileSystem    KeEnterCriticalRegion
#define FsRtlExitFileSystem     KeLeaveCriticalRegion

VOID
STDCALL
FsRtlIncrementCcFastReadNotPossible( VOID );

VOID
STDCALL
FsRtlIncrementCcFastReadWait( VOID );

VOID
STDCALL
FsRtlIncrementCcFastReadNoWait( VOID );

VOID
STDCALL
FsRtlIncrementCcFastReadResourceMiss( VOID );

VOID
STDCALL
FsRtlInitializeFileLock (
	IN PFILE_LOCK                   FileLock,
	IN PCOMPLETE_LOCK_IRP_ROUTINE   CompleteLockIrpRoutine OPTIONAL,
	IN PUNLOCK_ROUTINE              UnlockRoutine OPTIONAL
	);

VOID STDCALL
FsRtlInitializeLargeMcb(IN PLARGE_MCB Mcb,
			IN POOL_TYPE PoolType);

VOID STDCALL
FsRtlInitializeMcb (IN PMCB         Mcb,
		    IN POOL_TYPE    PoolType);

VOID STDCALL
FsRtlInitializeOplock(IN OUT POPLOCK Oplock);


VOID
STDCALL
FsRtlInitializeTunnelCache (
    IN PTUNNEL Cache
    );

NTSTATUS
STDCALL
FsRtlInsertPerFileObjectContext (
    IN PFSRTL_ADVANCED_FCB_HEADER PerFileObjectContext,
    IN PVOID /* PFSRTL_PER_FILE_OBJECT_CONTEXT*/ Ptr
    );

NTSTATUS
STDCALL
FsRtlInsertPerStreamContext (
    IN PFSRTL_ADVANCED_FCB_HEADER PerStreamContext,
    IN PFSRTL_PER_STREAM_CONTEXT Ptr
    );

BOOLEAN
STDCALL
FsRtlIsDbcsInExpression (
	DWORD	Unknown0,
	DWORD	Unknown1
	);
BOOLEAN
STDCALL
FsRtlIsFatDbcsLegal (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
BOOLEAN
STDCALL
FsRtlIsHpfsDbcsLegal (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	);
BOOLEAN
STDCALL
FsRtlIsNameInExpression (
	IN	PUNICODE_STRING	Expression,
	IN	PUNICODE_STRING	Name,
	IN	BOOLEAN		IgnoreCase,
	IN	PWCHAR		UpcaseTable	OPTIONAL
	);

BOOLEAN STDCALL
FsRtlIsNtstatusExpected(IN NTSTATUS NtStatus);

ULONG
FsRtlIsPagingFile (
    IN PFILE_OBJECT FileObject
    );

BOOLEAN STDCALL
FsRtlIsTotalDeviceFailure(IN NTSTATUS NtStatus);

#define FsRtlIsUnicodeCharacterWild(C) ( \
    (((C) >= 0x40) ? \
    FALSE : \
    FlagOn((*FsRtlLegalAnsiCharacterArray)[(C)], FSRTL_WILD_CHARACTER )) \
    )

BOOLEAN STDCALL
FsRtlLookupLargeMcbEntry(IN PLARGE_MCB Mcb,
			 IN LONGLONG Vbn,
			 OUT PLONGLONG Lbn OPTIONAL,
			 OUT PLONGLONG SectorCountFromLbn OPTIONAL,
			 OUT PLONGLONG StartingLbn OPTIONAL,
			 OUT PLONGLONG SectorCountFromStartingLbn OPTIONAL,
			 OUT PULONG Index OPTIONAL);

BOOLEAN STDCALL
FsRtlLookupLastLargeMcbEntry(IN PLARGE_MCB Mcb,
			     OUT PLONGLONG Vbn,
			     OUT PLONGLONG Lbn);
BOOLEAN
STDCALL
FsRtlLookupLastLargeMcbEntryAndIndex (
    IN PLARGE_MCB OpaqueMcb,
    OUT PLONGLONG LargeVbn,
    OUT PLONGLONG LargeLbn,
    OUT PULONG Index
    );

BOOLEAN STDCALL
FsRtlLookupLastMcbEntry (IN PMCB     Mcb,
			 OUT PVBN    Vbn,
			 OUT PLBN    Lbn);
BOOLEAN STDCALL
FsRtlLookupMcbEntry (IN PMCB     Mcb,
		     IN VBN      Vbn,
		     OUT PLBN    Lbn,
		     OUT PULONG  SectorCount OPTIONAL,
		     OUT PULONG  Index);

PFSRTL_PER_STREAM_CONTEXT
STDCALL
FsRtlLookupPerStreamContextInternal (
    IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
    IN PVOID OwnerId OPTIONAL,
    IN PVOID InstanceId OPTIONAL
    );

PVOID /* PFSRTL_PER_FILE_OBJECT_CONTEXT*/
STDCALL
FsRtlLookupPerFileObjectContext (
    IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
    IN PVOID OwnerId OPTIONAL,
    IN PVOID InstanceId OPTIONAL
    );

BOOLEAN
STDCALL
FsRtlMdlRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			LockKey,
	OUT	PMDL			*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus
	);

BOOLEAN
STDCALL
FsRtlMdlReadComplete (
	IN	PFILE_OBJECT	FileObject,
	IN OUT	PMDL		MdlChain
	);
BOOLEAN
STDCALL
FsRtlMdlReadCompleteDev (
	IN	PFILE_OBJECT	FileObject,
	IN	PMDL		MdlChain,
	IN	PDEVICE_OBJECT	DeviceObject
	);
BOOLEAN
STDCALL
FsRtlMdlReadDev (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			LockKey,
	OUT	PMDL			*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	);
BOOLEAN
STDCALL
FsRtlMdlWriteComplete (
	IN	PFILE_OBJECT	FileObject,
	IN	PLARGE_INTEGER	FileOffset,
	IN	PMDL		MdlChain
	);
BOOLEAN
STDCALL
FsRtlMdlWriteCompleteDev (
	IN	PFILE_OBJECT	FileObject,
	IN	PLARGE_INTEGER	FileOffset,
	IN	PMDL		MdlChain,
	IN	PDEVICE_OBJECT	DeviceObject
	);
VOID
STDCALL
FsRtlNotifyChangeDirectory (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PVOID		FsContext,
	IN	PSTRING		FullDirectoryName,
	IN	PLIST_ENTRY	NotifyList,
	IN	BOOLEAN		WatchTree,
	IN	ULONG		CompletionFilter,
	IN	PIRP		NotifyIrp
	);
VOID
STDCALL
FsRtlNotifyCleanup (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PVOID		FsContext
	);
typedef
BOOLEAN (*PCHECK_FOR_TRAVERSE_ACCESS) (
	IN	PVOID				NotifyContext,
	IN	PVOID				TargetContext,
	IN	PSECURITY_SUBJECT_CONTEXT	SubjectContext
	);

VOID
STDCALL
FsRtlNotifyFilterChangeDirectory (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY NotifyList,
    IN PVOID FsContext,
    IN PSTRING FullDirectoryName,
    IN BOOLEAN WatchTree,
    IN BOOLEAN IgnoreBuffer,
    IN ULONG CompletionFilter,
    IN PIRP NotifyIrp,
    IN PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback OPTIONAL,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext OPTIONAL,
    IN PFILTER_REPORT_CHANGE FilterCallback OPTIONAL
    );

VOID
STDCALL
FsRtlNotifyFilterReportChange (
    IN PNOTIFY_SYNC NotifySync,
    IN PLIST_ENTRY NotifyList,
    IN PSTRING FullTargetName,
    IN USHORT TargetNameOffset,
    IN PSTRING StreamName OPTIONAL,
    IN PSTRING NormalizedParentName OPTIONAL,
    IN ULONG FilterMatch,
    IN ULONG Action,
    IN PVOID TargetContext,
    IN PVOID FilterContext
    );
VOID
STDCALL
FsRtlNotifyFullChangeDirectory (
	IN	PNOTIFY_SYNC			NotifySync,
	IN	PLIST_ENTRY			NotifyList,
	IN	PVOID				FsContext,
	IN	PSTRING				FullDirectoryName,
	IN	BOOLEAN				WatchTree,
	IN	BOOLEAN				IgnoreBuffer,
	IN	ULONG				CompletionFilter,
	IN	PIRP				NotifyIrp,
	IN	PCHECK_FOR_TRAVERSE_ACCESS	TraverseCallback	OPTIONAL,
	IN	PSECURITY_SUBJECT_CONTEXT	SubjectContext		OPTIONAL
	);
VOID
STDCALL
FsRtlNotifyFullReportChange (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PSTRING		FullTargetName,
	IN	USHORT		TargetNameOffset,
	IN	PSTRING		StreamName OPTIONAL,
	IN	PSTRING		NormalizedParentName	OPTIONAL,
	IN	ULONG		FilterMatch,
	IN	ULONG		Action,
	IN	PVOID		TargetContext
	);
VOID
STDCALL
FsRtlNotifyUninitializeSync (
	IN OUT	PNOTIFY_SYNC NotifySync
	);
VOID
STDCALL
FsRtlNotifyReportChange (
	IN	PNOTIFY_SYNC	NotifySync,
	IN	PLIST_ENTRY	NotifyList,
	IN	PSTRING		FullTargetName,
	IN	PUSHORT		FileNamePartLength,
	IN	ULONG		FilterMatch
	);

VOID STDCALL
FsRtlNotifyInitializeSync(IN OUT PNOTIFY_SYNC NotifySync);

NTSTATUS STDCALL
FsRtlNotifyVolumeEvent(IN PFILE_OBJECT FileObject,
		       IN ULONG EventCode);

NTSTATUS STDCALL
FsRtlOplockFsctrl(IN POPLOCK Oplock,
		  IN PIRP Irp,
		  IN ULONG OpenCount);

BOOLEAN STDCALL
FsRtlOplockIsFastIoPossible(IN POPLOCK Oplock);

BOOLEAN
STDCALL
FsRtlPrepareMdlWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			LockKey,
	OUT	PMDL			*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus
	);

BOOLEAN
STDCALL
FsRtlPrepareMdlWriteDev (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			LockKey,
	OUT	PMDL			*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject
	);

NTSTATUS STDCALL
FsRtlNormalizeNtstatus(IN NTSTATUS NtStatusToNormalize,
		       IN NTSTATUS NormalizedNtStatus);

ULONG STDCALL
FsRtlNumberOfRunsInLargeMcb(IN PLARGE_MCB Mcb);

ULONG STDCALL
FsRtlNumberOfRunsInMcb (IN PMCB Mcb);

VOID
STDCALL
FsRtlPostPagingFileStackOverflow (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);

VOID
STDCALL
FsRtlPostStackOverflow (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);

BOOLEAN
STDCALL
FsRtlPrivateLock (
    IN PFILE_LOCK           FileLock,
    IN PFILE_OBJECT         FileObject,
    IN PLARGE_INTEGER       FileOffset,
    IN PLARGE_INTEGER       Length,
    IN PEPROCESS            Process,
    IN ULONG                Key,
    IN BOOLEAN              FailImmediately,
    IN BOOLEAN              ExclusiveLock,
    OUT PIO_STATUS_BLOCK    IoStatus,
    IN PIRP                 Irp OPTIONAL,
    IN PVOID                Context,
    IN BOOLEAN              AlreadySynchronized
    );

NTSTATUS
STDCALL
FsRtlProcessFileLock (
    IN PFILE_LOCK   FileLock,
    IN PIRP         Irp,
    IN PVOID        Context OPTIONAL
    );

NTSTATUS
STDCALL
FsRtlRegisterFileSystemFilterCallbacks (
    IN PVOID		Unknown1,
    IN PVOID		Unknown2
    );

NTSTATUS STDCALL
FsRtlRegisterUncProvider(IN OUT PHANDLE Handle,
			 IN PUNICODE_STRING RedirectorDeviceName,
			 IN BOOLEAN MailslotsSupported);

VOID
STDCALL
FsRtlReleaseFile (
    IN PFILE_OBJECT FileObject
    );

VOID STDCALL
FsRtlRemoveLargeMcbEntry(IN PLARGE_MCB Mcb,
			 IN LONGLONG Vbn,
			 IN LONGLONG SectorCount);

VOID STDCALL
FsRtlRemoveMcbEntry (IN PMCB     Mcb,
		     IN VBN      Vbn,
		     IN ULONG    SectorCount);

PFSRTL_PER_STREAM_CONTEXT
STDCALL
FsRtlRemovePerStreamContext (
    IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
    IN PVOID Unknown1 OPTIONAL,
    IN PVOID Unknown2 OPTIONAL
    );

PVOID /* PFSRTL_PER_FILE_OBJECT_CONTEXT*/
STDCALL
FsRtlRemovePerFileObjectContext (
   IN PFSRTL_ADVANCED_FCB_HEADER PerFileObjectContext,
    IN PVOID OwnerId OPTIONAL,
    IN PVOID InstanceId OPTIONAL
    );

VOID
STDCALL
FsRtlResetLargeMcb (
    IN PLARGE_MCB Mcb,
    IN BOOLEAN SelfSynchronized
    );

BOOLEAN STDCALL
FsRtlSplitLargeMcb(IN PLARGE_MCB Mcb,
		   IN LONGLONG Vbn,
		   IN LONGLONG Amount);

NTSTATUS
STDCALL
FsRtlSyncVolumes (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	);

VOID
STDCALL
FsRtlTeardownPerStreamContexts (
  IN PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader
  );

VOID STDCALL
FsRtlTruncateLargeMcb(IN PLARGE_MCB Mcb,
		      IN LONGLONG Vbn);

VOID STDCALL
FsRtlTruncateMcb (IN PMCB Mcb,
		  IN VBN  Vbn);

VOID STDCALL
FsRtlUninitializeFileLock (IN PFILE_LOCK FileLock);

VOID STDCALL
FsRtlUninitializeLargeMcb(IN PLARGE_MCB Mcb);

VOID STDCALL
FsRtlUninitializeMcb (IN PMCB Mcb);

VOID STDCALL
FsRtlUninitializeOplock(IN POPLOCK Oplock);

#endif /* __INCLUDE_DDK_FSFUNCS_H */
