/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sec.c
 * PURPOSE:         Security descriptor functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Steven Edwards ( Steven_Ed4153@yahoo.com )
 *                  Andrew Greenwood ( silverblade_uk@hotmail.com )
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <advapi32.h>
#include <debug.h>

/*
 * @implemented
 */
BOOL
STDCALL
GetSecurityDescriptorControl (
	PSECURITY_DESCRIPTOR		pSecurityDescriptor,
	PSECURITY_DESCRIPTOR_CONTROL	pControl,
	LPDWORD				lpdwRevision
	)
{
	NTSTATUS Status;

	Status = RtlGetControlSecurityDescriptor (pSecurityDescriptor,
	                                          pControl,
	                                          (PULONG)lpdwRevision);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetSecurityDescriptorDacl (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	LPBOOL			lpbDaclPresent,
	PACL			*pDacl,
	LPBOOL			lpbDaclDefaulted
	)
{
	BOOLEAN DaclPresent;
	BOOLEAN DaclDefaulted;
	NTSTATUS Status;

	Status = RtlGetDaclSecurityDescriptor (pSecurityDescriptor,
	                                       &DaclPresent,
	                                       pDacl,
	                                       &DaclDefaulted);
	*lpbDaclPresent = (BOOL)DaclPresent;
	*lpbDaclDefaulted = (BOOL)DaclDefaulted;

	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetSecurityDescriptorGroup (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	PSID			*pGroup,
	LPBOOL			lpbGroupDefaulted
	)
{
	BOOLEAN GroupDefaulted;
	NTSTATUS Status;

	Status = RtlGetGroupSecurityDescriptor (pSecurityDescriptor,
	                                        pGroup,
	                                        &GroupDefaulted);
	*lpbGroupDefaulted = (BOOL)GroupDefaulted;

	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetSecurityDescriptorOwner (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	PSID			*pOwner,
	LPBOOL			lpbOwnerDefaulted
	)
{
	BOOLEAN OwnerDefaulted;
	NTSTATUS Status;

	Status = RtlGetOwnerSecurityDescriptor (pSecurityDescriptor,
	                                        pOwner,
	                                        &OwnerDefaulted);
	*lpbOwnerDefaulted = (BOOL)OwnerDefaulted;

	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetSecurityDescriptorRMControl (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	PUCHAR			RMControl)
{
  if (!RtlGetSecurityDescriptorRMControl(SecurityDescriptor,
					 RMControl))
    return ERROR_INVALID_DATA;

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetSecurityDescriptorSacl (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	LPBOOL			lpbSaclPresent,
	PACL			*pSacl,
	LPBOOL			lpbSaclDefaulted
	)
{
	BOOLEAN SaclPresent;
	BOOLEAN SaclDefaulted;
	NTSTATUS Status;

	Status = RtlGetSaclSecurityDescriptor (pSecurityDescriptor,
	                                       &SaclPresent,
	                                       pSacl,
	                                       &SaclDefaulted);
	*lpbSaclPresent = (BOOL)SaclPresent;
	*lpbSaclDefaulted = (BOOL)SaclDefaulted;

	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
InitializeSecurityDescriptor (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	DWORD			dwRevision
	)
{
	NTSTATUS Status;

	Status = RtlCreateSecurityDescriptor (pSecurityDescriptor,
	                                      dwRevision);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
IsValidSecurityDescriptor (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor
	)
{
	BOOLEAN Result;

	Result = RtlValidSecurityDescriptor (pSecurityDescriptor);
	if (Result == FALSE)
		SetLastError (RtlNtStatusToDosError (STATUS_INVALID_SECURITY_DESCR));

	return (BOOL)Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
MakeAbsoluteSD (
	PSECURITY_DESCRIPTOR	pSelfRelativeSecurityDescriptor,
	PSECURITY_DESCRIPTOR	pAbsoluteSecurityDescriptor,
	LPDWORD			lpdwAbsoluteSecurityDescriptorSize,
	PACL			pDacl,
	LPDWORD			lpdwDaclSize,
	PACL			pSacl,
	LPDWORD			lpdwSaclSize,
	PSID			pOwner,
	LPDWORD			lpdwOwnerSize,
	PSID			pPrimaryGroup,
	LPDWORD			lpdwPrimaryGroupSize
	)
{
	NTSTATUS Status;

	Status = RtlSelfRelativeToAbsoluteSD ((PISECURITY_DESCRIPTOR_RELATIVE)pSelfRelativeSecurityDescriptor,
	                                      pAbsoluteSecurityDescriptor,
	                                      lpdwAbsoluteSecurityDescriptorSize,
	                                      pDacl,
	                                      lpdwDaclSize,
	                                      pSacl,
	                                      lpdwSaclSize,
	                                      pOwner,
	                                      lpdwOwnerSize,
	                                      pPrimaryGroup,
	                                      lpdwPrimaryGroupSize);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
MakeSelfRelativeSD (
	PSECURITY_DESCRIPTOR	pAbsoluteSecurityDescriptor,
	PSECURITY_DESCRIPTOR	pSelfRelativeSecurityDescriptor,
	LPDWORD			lpdwBufferLength
	)
{
	NTSTATUS Status;

	Status = RtlAbsoluteToSelfRelativeSD (pAbsoluteSecurityDescriptor,
	                                      (PISECURITY_DESCRIPTOR_RELATIVE)pSelfRelativeSecurityDescriptor,
	                                      (PULONG)lpdwBufferLength);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetSecurityDescriptorControl (
	PSECURITY_DESCRIPTOR		pSecurityDescriptor,
	SECURITY_DESCRIPTOR_CONTROL	ControlBitsOfInterest,
	SECURITY_DESCRIPTOR_CONTROL	ControlBitsToSet)
{
	NTSTATUS Status;

	Status = RtlSetControlSecurityDescriptor(pSecurityDescriptor,
	                                         ControlBitsOfInterest,
	                                         ControlBitsToSet);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetSecurityDescriptorDacl (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	BOOL			bDaclPresent,
	PACL			pDacl,
	BOOL			bDaclDefaulted
	)
{
	NTSTATUS Status;

	Status = RtlSetDaclSecurityDescriptor (pSecurityDescriptor,
	                                       bDaclPresent,
	                                       pDacl,
	                                       bDaclDefaulted);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetSecurityDescriptorGroup (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	PSID			pGroup,
	BOOL			bGroupDefaulted
	)
{
	NTSTATUS Status;

	Status = RtlSetGroupSecurityDescriptor (pSecurityDescriptor,
	                                        pGroup,
	                                        bGroupDefaulted);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetSecurityDescriptorOwner (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	PSID			pOwner,
	BOOL			bOwnerDefaulted
	)
{
	NTSTATUS Status;

	Status = RtlSetOwnerSecurityDescriptor (pSecurityDescriptor,
	                                        pOwner,
	                                        bOwnerDefaulted);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
DWORD
STDCALL
SetSecurityDescriptorRMControl (
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	PUCHAR			RMControl)
{
  RtlSetSecurityDescriptorRMControl(SecurityDescriptor,
				    RMControl);

  return ERROR_SUCCESS;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetSecurityDescriptorSacl (
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	BOOL			bSaclPresent,
	PACL			pSacl,
	BOOL			bSaclDefaulted
	)
{
	NTSTATUS Status;

	Status = RtlSetSaclSecurityDescriptor (pSecurityDescriptor,
	                                       bSaclPresent,
	                                       pSacl,
	                                       bSaclDefaulted);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ConvertToAutoInheritPrivateObjectSecurity(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                          IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
                                          OUT PSECURITY_DESCRIPTOR* NewSecurityDescriptor,
                                          IN GUID* ObjectType,
                                          IN BOOLEAN IsDirectoryObject,
                                          IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
