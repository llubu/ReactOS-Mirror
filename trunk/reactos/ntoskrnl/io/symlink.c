/* $Id: symlink.c,v 1.35 2004/10/22 20:25:54 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/symlink.c
 * PURPOSE:         Implements symbolic links
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	IoCreateSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
IoCreateSymbolicLink(PUNICODE_STRING SymbolicLinkName,
		     PUNICODE_STRING DeviceName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE Handle;
  NTSTATUS Status;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("IoCreateSymbolicLink(SymbolicLinkName %wZ, DeviceName %wZ)\n",
	 SymbolicLinkName,
	 DeviceName);

  InitializeObjectAttributes(&ObjectAttributes,
			     SymbolicLinkName,
			     OBJ_PERMANENT,
			     NULL,
			     SePublicDefaultSd);

  Status = NtCreateSymbolicLinkObject(&Handle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes,
				      DeviceName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSymbolicLinkObject() failed (Status %lx)\n", Status);
      return(Status);
    }

  NtClose(Handle);

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	IoCreateUnprotectedSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
IoCreateUnprotectedSymbolicLink(PUNICODE_STRING SymbolicLinkName,
				PUNICODE_STRING DeviceName)
{
  SECURITY_DESCRIPTOR SecurityDescriptor;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE Handle;
  NTSTATUS Status;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("IoCreateUnprotectedSymbolicLink(SymbolicLinkName %wZ, DeviceName %wZ)\n",
	 SymbolicLinkName,
	 DeviceName);

  Status = RtlCreateSecurityDescriptor(&SecurityDescriptor,
				       SECURITY_DESCRIPTOR_REVISION);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlCreateSecurityDescriptor() failed (Status %lx)\n", Status);
      return(Status);
    }

  Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor,
					TRUE,
					NULL,
					TRUE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlSetDaclSecurityDescriptor() failed (Status %lx)\n", Status);
      return(Status);
    }

  InitializeObjectAttributes(&ObjectAttributes,
			     SymbolicLinkName,
			     OBJ_PERMANENT,
			     NULL,
			     &SecurityDescriptor);

  Status = NtCreateSymbolicLinkObject(&Handle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes,
				      DeviceName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateSymbolicLinkObject() failed (Status %lx)\n", Status);
      return(Status);
    }

  NtClose(Handle);

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	IoDeleteSymbolicLink
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
IoDeleteSymbolicLink(PUNICODE_STRING SymbolicLinkName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE Handle;
  NTSTATUS Status;

  ASSERT_IRQL(PASSIVE_LEVEL);

  DPRINT("IoDeleteSymbolicLink (SymbolicLinkName %S)\n",
	 SymbolicLinkName->Buffer);

  InitializeObjectAttributes(&ObjectAttributes,
			     SymbolicLinkName,
			     OBJ_OPENLINK,
			     NULL,
			     NULL);

  Status = NtOpenSymbolicLinkObject(&Handle,
				    SYMBOLIC_LINK_ALL_ACCESS,
				    &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    return(Status);

  Status = NtMakeTemporaryObject(Handle);
  NtClose(Handle);

  return(Status);
}

/* EOF */
