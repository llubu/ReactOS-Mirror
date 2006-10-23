/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/audit.c
 * PURPOSE:         Audit functions
 *
 * PROGRAMMERS:     Eric Kohl <eric.kohl@t-online.de>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* INTERNAL *****************************************************************/

BOOLEAN
NTAPI
SeDetailedAuditingWithToken(IN PTOKEN Token)
{
    /* FIXME */
    return FALSE;
}

VOID
NTAPI
SeAuditProcessCreate(IN PEPROCESS Process)
{
    /* FIXME */
}

VOID
NTAPI
SeAuditProcessExit(IN PEPROCESS Process)
{
    /* FIXME */
}

NTSTATUS
NTAPI
SeInitializeProcessAuditName(IN PFILE_OBJECT FileObject,
                             IN BOOLEAN DoAudit,
                             OUT POBJECT_NAME_INFORMATION *AuditInfo)
{
    OBJECT_NAME_INFORMATION LocalNameInfo;
    POBJECT_NAME_INFORMATION ObjectNameInfo = NULL;
    ULONG ReturnLength = 8;
    NTSTATUS Status;
    PAGED_CODE();
    ASSERT(AuditInfo);

    /* Check if we should do auditing */
    if (DoAudit)
    {
        /* FIXME: TODO */
    }

    /* Now query the name */
    Status = ObQueryNameString(FileObject,
                               &LocalNameInfo,
                               sizeof(LocalNameInfo),
                               &ReturnLength);
    if (((Status == STATUS_BUFFER_OVERFLOW) ||
         (Status == STATUS_BUFFER_TOO_SMALL)) &&
        (ReturnLength != sizeof(LocalNameInfo)))
    {
        /* Allocate required size */
        ObjectNameInfo = ExAllocatePoolWithTag(NonPagedPool,
                                               ReturnLength,
                                               TAG_SEPA);
        if (ObjectNameInfo)
        {
            /* Query the name again */
            Status = ObQueryNameString(FileObject,
                                       ObjectNameInfo,
                                       ReturnLength,
                                       &ReturnLength);
        }
    }

    /* Check if we got here due to failure */
    if ((ObjectNameInfo) &&
        (!(NT_SUCCESS(Status)) || (ReturnLength == sizeof(LocalNameInfo))))
    {
        /* First, free any buffer we might've allocated */
        KEBUGCHECK(0);
        if (ObjectNameInfo) ExFreePool(ObjectNameInfo);

        /* Now allocate a temporary one */
        ReturnLength = sizeof(OBJECT_NAME_INFORMATION);
        ObjectNameInfo = ExAllocatePoolWithTag(NonPagedPool,
                                               sizeof(OBJECT_NAME_INFORMATION),
                                               TAG_SEPA);
        if (ObjectNameInfo)
        {
            /* Clear it */
            RtlZeroMemory(ObjectNameInfo, ReturnLength);
            Status = STATUS_SUCCESS;
        }
    }

    /* Check if memory allocation failed */
    if (!ObjectNameInfo) Status = STATUS_NO_MEMORY;

    /* Return the audit name */
    *AuditInfo = ObjectNameInfo;

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
SeLocateProcessImageName(IN PEPROCESS Process,
                         OUT PUNICODE_STRING *ProcessImageName)
{
    POBJECT_NAME_INFORMATION AuditName;
    PUNICODE_STRING ImageName;
    PFILE_OBJECT FileObject;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Assume failure */
    *ProcessImageName = NULL;

    /* Check if we have audit info */
    AuditName = Process->SeAuditProcessCreationInfo.ImageFileName;
    if (!AuditName)
    {
        /* Get the file object */
        Status = PsReferenceProcessFilePointer(Process, &FileObject);
        if (!NT_SUCCESS(Status)) return Status;

        /* Initialize the audit structure */
        Status = SeInitializeProcessAuditName(FileObject, TRUE, &AuditName);
        if (NT_SUCCESS(Status))
        {
            /* Set it */
            if (InterlockedCompareExchangePointer(&Process->
                                                  SeAuditProcessCreationInfo,
                                                  AuditName,
                                                  NULL))
            {
                /* Someone beat us to it, deallocate our copy */
                ExFreePool(AuditName);
            }
        }

        /* Dereference the file object */
        ObDereferenceObject(FileObject);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Allocate the output string */
    ImageName = ExAllocatePoolWithTag(NonPagedPool,
                                      AuditName->Name.MaximumLength +
                                      sizeof(UNICODE_STRING),
                                      TAG_SEPA);
    if (ImageName)
    {
        /* Make a copy of it */
        RtlCopyMemory(ImageName,
                      &AuditName->Name,
                      AuditName->Name.MaximumLength + sizeof(UNICODE_STRING));

        /* Fix up the buffer */
        ImageName->Buffer = (PWSTR)(ImageName + 1);

        /* Return it */
        *ProcessImageName = ImageName;
    }
    else
    {
        /* Otherwise, fail */
        Status = STATUS_NO_MEMORY;
    }

    /* Return status */
    return Status;
}

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
NtAccessCheckAndAuditAlarm(IN PUNICODE_STRING SubsystemName,
                           IN HANDLE HandleId,
                           IN PUNICODE_STRING ObjectTypeName,
                           IN PUNICODE_STRING ObjectName,
                           IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                           IN ACCESS_MASK DesiredAccess,
                           IN PGENERIC_MAPPING GenericMapping,
                           IN BOOLEAN ObjectCreation,
                           OUT PACCESS_MASK GrantedAccess,
                           OUT PNTSTATUS AccessStatus,
                           OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL
NtCloseObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
			IN PVOID HandleId,
			IN BOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtDeleteObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
			 IN PVOID HandleId,
			 IN BOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtOpenObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
		       IN PVOID HandleId,
		       IN PUNICODE_STRING ObjectTypeName,
		       IN PUNICODE_STRING ObjectName,
		       IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		       IN HANDLE ClientToken,
		       IN ULONG DesiredAccess,
		       IN ULONG GrantedAccess,
		       IN PPRIVILEGE_SET Privileges,
		       IN BOOLEAN ObjectCreation,
		       IN BOOLEAN AccessGranted,
		       OUT PBOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtPrivilegedServiceAuditAlarm(IN PUNICODE_STRING SubsystemName,
			      IN PUNICODE_STRING ServiceName,
			      IN HANDLE ClientToken,
			      IN PPRIVILEGE_SET Privileges,
			      IN BOOLEAN AccessGranted)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtPrivilegeObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
			    IN PVOID HandleId,
			    IN HANDLE ClientToken,
			    IN ULONG DesiredAccess,
			    IN PPRIVILEGE_SET Privileges,
			    IN BOOLEAN AccessGranted)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
VOID
STDCALL
SeAuditHardLinkCreation(
	IN PUNICODE_STRING FileName,
	IN PUNICODE_STRING LinkName,
	IN BOOLEAN bSuccess
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingFileEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingFileEventsWithContext(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingHardLinkEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingHardLinkEventsWithContext(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingFileOrGlobalEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SeCloseObjectAuditAlarm(
	IN PVOID Object,
	IN HANDLE Handle,
	IN BOOLEAN PerformAction
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID STDCALL
SeDeleteObjectAuditAlarm(IN PVOID Object,
			 IN HANDLE Handle)
{
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID STDCALL
SeOpenObjectAuditAlarm(IN PUNICODE_STRING ObjectTypeName,
		       IN PVOID Object OPTIONAL,
		       IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
		       IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		       IN PACCESS_STATE AccessState,
		       IN BOOLEAN ObjectCreated,
		       IN BOOLEAN AccessGranted,
		       IN KPROCESSOR_MODE AccessMode,
		       OUT PBOOLEAN GenerateOnClose)
{
    DPRINT1("SeOpenObjectAuditAlarm is UNIMPLEMENTED!\n");
}


/*
 * @unimplemented
 */
VOID STDCALL
SeOpenObjectForDeleteAuditAlarm(IN PUNICODE_STRING ObjectTypeName,
				IN PVOID Object OPTIONAL,
				IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
				IN PSECURITY_DESCRIPTOR SecurityDescriptor,
				IN PACCESS_STATE AccessState,
				IN BOOLEAN ObjectCreated,
				IN BOOLEAN AccessGranted,
				IN KPROCESSOR_MODE AccessMode,
				OUT PBOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SePrivilegeObjectAuditAlarm(
	IN HANDLE Handle,
	IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
	IN ACCESS_MASK DesiredAccess,
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN AccessGranted,
	IN KPROCESSOR_MODE CurrentMode
	)
{
	UNIMPLEMENTED;
}

/* EOF */
