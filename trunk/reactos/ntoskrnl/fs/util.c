/* $Id: util.c,v 1.18 2004/08/21 20:40:27 tamlin Exp $
 *
 * reactos/ntoskrnl/fs/util.c
 *
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsTotalDeviceFailure@4
 *
 * DESCRIPTION
 *	Check if an NTSTATUS error code represents a
 *	disk hardware failure.
 *	
 * ARGUMENTS
 *	NtStatus
 *		NTSTATUS to test.
 *
 * RETURN VALUE
 *	FALSE if either (NtStatus >= STATUS_SUCCESS), 
 *	STATUS_CRC_ERROR, STATUS_DEVICE_DATA_ERROR;
 *	TRUE otherwise.
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlIsTotalDeviceFailure (
	IN	NTSTATUS	NtStatus
	)
{
	return (
		(NT_SUCCESS(NtStatus))
		|| (STATUS_CRC_ERROR == NtStatus)
		|| (STATUS_DEVICE_DATA_ERROR == NtStatus)
		? FALSE
		: TRUE
		);
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsNtstatusExpected/1
 *	stack32 = 4
 *
 * DESCRIPTION
 *	Check an NTSTATUS value is expected by the FS kernel
 *	subsystem.
 *
 * ARGUMENTS
 *	NtStatus
 *		NTSTATUS to test.
 *
 * RETURN VALUE
 *	TRUE if NtStatus is NOT one out of:
 *	- STATUS_ACCESS_VIOLATION
 *	- STATUS_ILLEGAL_INSTRUCTION
 *	- STATUS_DATATYPE_MISALIGNMENT
 *	- STATUS_INSTRUCTION_MISALIGNMENT
 *	which are the forbidden return stati in the FsRtl
 *	subsystem; FALSE otherwise.
 *
 * REVISIONS
 *	2002-01-17 Fixed a bad bug reported by Bo Brant�n.
 *	Up to version 1.8, this function's semantics was
 *	exactly the opposite! Thank you Bo.
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlIsNtstatusExpected (
	IN	NTSTATUS	NtStatus
	)
{
	return (
		(STATUS_DATATYPE_MISALIGNMENT == NtStatus)
		|| (STATUS_ACCESS_VIOLATION == NtStatus)
		|| (STATUS_ILLEGAL_INSTRUCTION == NtStatus)
		|| (STATUS_INSTRUCTION_MISALIGNMENT == NtStatus)
		)
		? FALSE
		: TRUE;
}
	
/*
 * @unimplemented
 */
ULONG
FsRtlIsPagingFile (
    IN PFILE_OBJECT FileObject
    )
{
	UNIMPLEMENTED;
	return 0;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlNormalizeNtstatus@8
 *
 * DESCRIPTION
 *	Normalize an NTSTATUS value for using in the FS subsystem.
 *
 * ARGUMENTS
 *	NtStatusToNormalize
 *		NTSTATUS to normalize.
 *	NormalizedNtStatus
 *		NTSTATUS to return if the NtStatusToNormalize
 *		value is unexpected by the FS kernel subsystem.
 *
 * RETURN VALUE
 * 	NtStatusToNormalize if it is an expected value,
 * 	otherwise NormalizedNtStatus.
 *
 * @implemented
 */
NTSTATUS
STDCALL
FsRtlNormalizeNtstatus (
	IN	NTSTATUS	NtStatusToNormalize,
	IN	NTSTATUS	NormalizedNtStatus
	)
{
	return
		(TRUE == FsRtlIsNtstatusExpected(NtStatusToNormalize))
		? NtStatusToNormalize
		: NormalizedNtStatus;
}


/**********************************************************************
 *	Miscellanea (they may fit somewhere else)
 *********************************************************************/


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAllocateResource@0
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 *
 * @unimplemented
 */
DWORD
STDCALL
FsRtlAllocateResource (VOID)
{
	return 0;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlBalanceReads@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 *
 * @unimplemented
 */
DWORD
STDCALL
FsRtlBalanceReads (
	DWORD	Unknown0
	)
{
	return 0;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCopyRead@32
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * 	From Bo Branten's ntifs.h v12.
 * 
 * @unimplemented
 */
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
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCopyWrite@32
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 * NOTE
 * 	From Bo Branten's ntifs.h v12.
 *
 * @unimplemented
 */
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
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlGetFileSize@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 * @implemented
 */
NTSTATUS
STDCALL
FsRtlGetFileSize (
    IN PFILE_OBJECT         FileObject,
    IN OUT PLARGE_INTEGER   FileSize
    )
{
	FILE_STANDARD_INFORMATION Info;
	NTSTATUS Status;
	ULONG Length;

	Status = IoQueryFileInformation(FileObject,
		FileStandardInformation,
		sizeof(Info),
		&Info,
		&Length);
	if (NT_SUCCESS(Status))
		{
			FileSize->QuadPart = Info.EndOfFile.QuadPart;
		}

	return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
FsRtlInsertPerStreamContext (
    IN PFSRTL_ADVANCED_FCB_HEADER PerStreamContext,
    IN PFSRTL_PER_STREAM_CONTEXT Ptr
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PFSRTL_PER_STREAM_CONTEXT
STDCALL
FsRtlRemovePerStreamContext (
    IN PFSRTL_ADVANCED_FCB_HEADER StreamContext,
    IN PVOID OwnerId OPTIONAL,
    IN PVOID InstanceId OPTIONAL
    )
{
	UNIMPLEMENTED;
	return NULL;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
FsRtlInsertPerFileObjectContext (
    IN PFSRTL_ADVANCED_FCB_HEADER PerFileObjectContext,
    IN PVOID /* PFSRTL_PER_FILE_OBJECT_CONTEXT*/ Ptr
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlPostPagingFileStackOverflow@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 * @unimplemented
 */
VOID
STDCALL
FsRtlPostPagingFileStackOverflow (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlPostStackOverflow@12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 * 
 * @unimplemented
 */
VOID
STDCALL
FsRtlPostStackOverflow (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
}


/*
 * @unimplemented
 */
PVOID /* PFSRTL_PER_FILE_OBJECT_CONTEXT*/
STDCALL
FsRtlRemovePerFileObjectContext (
   IN PFSRTL_ADVANCED_FCB_HEADER PerFileObjectContext,
    IN PVOID OwnerId OPTIONAL,
    IN PVOID InstanceId OPTIONAL
    )
{
	UNIMPLEMENTED;
	return NULL;
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlSyncVolumes@12
 *
 * DESCRIPTION
 *	Obsolete function.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *	It always returns STATUS_SUCCESS.
 *
 * @implemented
 */
NTSTATUS
STDCALL
FsRtlSyncVolumes (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
STDCALL
FsRtlTeardownPerStreamContexts (
  IN PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader
  )
{
	UNIMPLEMENTED;
}

/* EOF */
