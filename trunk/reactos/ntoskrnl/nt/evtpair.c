/* $Id: evtpair.c,v 1.12 2002/09/07 15:13:03 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/evtpair.c
 * PURPOSE:         Support for event pairs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

POBJECT_TYPE ExEventPairObjectType = NULL;

static GENERIC_MAPPING ExEventPairMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
	EVENT_PAIR_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtpCreateEventPair(PVOID ObjectBody,
		   PVOID Parent,
		   PWSTR RemainingPath,
		   POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("NtpCreateEventPair(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }
  
  return(STATUS_SUCCESS);
}

VOID NtInitializeEventPairImplementation(VOID)
{
   ExEventPairObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExEventPairObjectType->TypeName, L"EventPair");
   
   ExEventPairObjectType->MaxObjects = ULONG_MAX;
   ExEventPairObjectType->MaxHandles = ULONG_MAX;
   ExEventPairObjectType->TotalObjects = 0;
   ExEventPairObjectType->TotalHandles = 0;
   ExEventPairObjectType->PagedPoolCharge = 0;
   ExEventPairObjectType->NonpagedPoolCharge = sizeof(KEVENT_PAIR);
   ExEventPairObjectType->Mapping = &ExEventPairMapping;
   ExEventPairObjectType->Dump = NULL;
   ExEventPairObjectType->Open = NULL;
   ExEventPairObjectType->Close = NULL;
   ExEventPairObjectType->Delete = NULL;
   ExEventPairObjectType->Parse = NULL;
   ExEventPairObjectType->Security = NULL;
   ExEventPairObjectType->QueryName = NULL;
   ExEventPairObjectType->OkayToClose = NULL;
   ExEventPairObjectType->Create = NtpCreateEventPair;
   ExEventPairObjectType->DuplicationNotify = NULL;
}


NTSTATUS STDCALL
NtCreateEventPair(OUT PHANDLE EventPairHandle,
		  IN ACCESS_MASK DesiredAccess,
		  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtCreateEventPair()\n");
   Status = ObRosCreateObject(EventPairHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   ExEventPairObjectType,
			   (PVOID*)&EventPair);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   KeInitializeEvent(&EventPair->Event1,
		     SynchronizationEvent,
		     FALSE);
   KeInitializeEvent(&EventPair->Event2,
		     SynchronizationEvent,
		     FALSE);
   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtOpenEventPair(OUT PHANDLE EventPairHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;

   DPRINT("NtOpenEventPair()\n");

   Status = ObOpenObjectByName(ObjectAttributes,
			       ExEventPairObjectType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       EventPairHandle);

   return Status;
}


NTSTATUS STDCALL
NtSetHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtSetHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeSetEvent(&EventPair->Event2,
	      EVENT_INCREMENT,
	      FALSE);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetHighWaitLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtSetHighWaitLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeSetEvent(&EventPair->Event2,
	      EVENT_INCREMENT,
	      FALSE);

   KeWaitForSingleObject(&EventPair->Event1,
			 WrEventPair,
			 UserMode,
			 FALSE,
			 NULL);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtSetLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeSetEvent(&EventPair->Event1,
	      EVENT_INCREMENT,
	      FALSE);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetLowWaitHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtSetLowWaitHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeSetEvent(&EventPair->Event1,
	      EVENT_INCREMENT,
	      FALSE);

   KeWaitForSingleObject(&EventPair->Event2,
			 WrEventPair,
			 UserMode,
			 FALSE,
			 NULL);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtWaitLowEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtWaitLowEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeWaitForSingleObject(&EventPair->Event1,
			 WrEventPair,
			 UserMode,
			 FALSE,
			 NULL);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtWaitHighEventPair(IN HANDLE EventPairHandle)
{
   PKEVENT_PAIR EventPair;
   NTSTATUS Status;

   DPRINT("NtWaitHighEventPair(EventPairHandle %x)\n",
	  EventPairHandle);

   Status = ObReferenceObjectByHandle(EventPairHandle,
				      EVENT_PAIR_ALL_ACCESS,
				      ExEventPairObjectType,
				      UserMode,
				      (PVOID*)&EventPair,
				      NULL);
   if (!NT_SUCCESS(Status))
     return(Status);

   KeWaitForSingleObject(&EventPair->Event2,
			 WrEventPair,
			 UserMode,
			 FALSE,
			 NULL);

   ObDereferenceObject(EventPair);
   return(STATUS_SUCCESS);
}

/* EOF */
