/*
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/ob/ntobj.c
 * PURPOSE:       User mode interface to object manager
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *               10/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <wstring.h>
#include <string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ************************************************************/

NTSTATUS STDCALL NtSetInformationObject(IN HANDLE ObjectHandle,
					IN CINT ObjectInformationClass,
					IN PVOID ObjectInformation,
					IN ULONG Length)
{
   return(ZwSetInformationObject(ObjectHandle,
				 ObjectInformationClass,
				 ObjectInformation,
				 Length));
}

NTSTATUS STDCALL ZwSetInformationObject(IN HANDLE ObjectHandle,
					IN CINT ObjectInformationClass,
					IN PVOID ObjectInformation,
					IN ULONG Length)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryObject(IN HANDLE ObjectHandle,
			       IN CINT ObjectInformationClass,
			       OUT PVOID ObjectInformation,
			       IN ULONG Length,
			       OUT PULONG ResultLength)
{
   return(ZwQueryObject(ObjectHandle,
			ObjectInformationClass,
			ObjectInformation,
			Length,
			ResultLength));
}

NTSTATUS STDCALL ZwQueryObject(IN HANDLE ObjectHandle,
			       IN CINT ObjectInformationClass,
			       OUT PVOID ObjectInformation,
			       IN ULONG Length,
			       OUT PULONG ResultLength)
{
   UNIMPLEMENTED
}

VOID ObMakeTemporaryObject(PVOID ObjectBody)
{
   POBJECT_HEADER ObjectHeader;
   
   ObjectHeader = BODY_TO_HEADER(ObjectBody);
   ObjectHeader->Permanent = FALSE;
}

NTSTATUS NtMakeTemporaryObject(HANDLE Handle)
{
   return(ZwMakeTemporaryObject(Handle));
}

NTSTATUS ZwMakeTemporaryObject(HANDLE Handle)
{
   PVOID Object;
   NTSTATUS Status;  
   POBJECT_HEADER ObjectHeader;
   
   Status = ObReferenceObjectByHandle(Handle,
				      0,
				      NULL,
				      KernelMode,
				      &Object,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }

   ObjectHeader = BODY_TO_HEADER(Object);
   ObjectHeader->Permanent = FALSE;
   
   ObDereferenceObject(Object);
   
   return(STATUS_SUCCESS);
}

NTSTATUS NtClose(HANDLE Handle)
{
   return(ZwClose(Handle));
}

NTSTATUS ZwClose(HANDLE Handle)
/*
 * FUNCTION: Closes a handle reference to an object
 * ARGUMENTS:
 *         Handle = handle to close
 * RETURNS: Status
 */
{
   PVOID ObjectBody;
   POBJECT_HEADER Header;
   PHANDLE_REP HandleRep;
   
   assert_irql(PASSIVE_LEVEL);
   
   DPRINT("ZwClose(Handle %x)\n",Handle);
   
   HandleRep = ObTranslateHandle(KeGetCurrentProcess(),Handle);
   if (HandleRep == NULL)
     {
	return(STATUS_INVALID_HANDLE);
     }
   ObjectBody = HandleRep->ObjectBody;
   
   HandleRep->ObjectBody = NULL;
   
   Header = BODY_TO_HEADER(ObjectBody);
   
   Header->RefCount++;
   Header->HandleCount--;
   
   if (Header->ObjectType != NULL &&
       Header->ObjectType->Close != NULL)
     {
	Header->ObjectType->Close(ObjectBody, Header->HandleCount);
     }
   
   Header->RefCount--;
   
   ObPerformRetentionChecks(Header);
   
   return(STATUS_SUCCESS);
}
