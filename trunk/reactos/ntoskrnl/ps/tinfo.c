/* $Id: tinfo.c,v 1.25 2004/03/19 12:45:07 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/tinfo.c
 * PURPOSE:         Getting/setting thread information
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Updated 09/08/2003 by Skywing (skywing@valhallalegends.com)
 *                   to suppport thread-eventpairs.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/ex.h>
#include <internal/safe.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtSetInformationThread (IN HANDLE ThreadHandle,
			IN THREADINFOCLASS ThreadInformationClass,
			IN PVOID ThreadInformation,
			IN ULONG ThreadInformationLength)
{
  PETHREAD Thread;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle (ThreadHandle,
				      THREAD_SET_INFORMATION,
				      PsThreadType,
				      ExGetPreviousMode (),
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

  switch (ThreadInformationClass)
    {
      case ThreadBasicInformation:
	/* Can only be queried */
	Status = STATUS_INVALID_INFO_CLASS;
	break;
	
      case ThreadTimes:
	/* Can only be queried */
	Status = STATUS_INVALID_INFO_CLASS;
	break;
	
      case ThreadPriority:
	  {
	    KPRIORITY Priority;

	    if (ThreadInformationLength != sizeof(KPRIORITY))
	      {
		Status = STATUS_INFO_LENGTH_MISMATCH;
		break;
	      }
	    Priority = *(KPRIORITY*)ThreadInformation;
	    if (Priority < LOW_PRIORITY || Priority >= MAXIMUM_PRIORITY)
	      {
		Status = STATUS_INVALID_PARAMETER;
		break;
	      }
	    KeSetPriorityThread(&Thread->Tcb, Priority);
	    Status = STATUS_SUCCESS;
	    break;
	  }
	
      case ThreadBasePriority:
	  {
	    LONG Increment;

	    if (ThreadInformationLength != sizeof(LONG))
	      {
	        Status = STATUS_INFO_LENGTH_MISMATCH;
	        break;
	      }
	    Status = MmCopyFromCaller(&Increment,
				      ThreadInformation,
				      sizeof(ULONG));
	    if (NT_SUCCESS(Status))
	      {
		KeSetBasePriorityThread (&Thread->Tcb, Increment);
	      }
	  }
        break;
	
      case ThreadAffinityMask:
	Thread->Tcb.UserAffinity = *((PULONG)ThreadInformation);
	break;
	
      case ThreadImpersonationToken:
	{
	  HANDLE TokenHandle;

	  if (ThreadInformationLength != sizeof(HANDLE))
	    {
	      Status = STATUS_INFO_LENGTH_MISMATCH;
	      break;
	    }
	  TokenHandle = *((PHANDLE)ThreadInformation);
	  Status = PsAssignImpersonationToken (Thread,
					       TokenHandle);
	  break;
	}
	
      case ThreadDescriptorTableEntry:
	/* Can only be queried */
	Status = STATUS_INVALID_INFO_CLASS;
	break;
	
      case ThreadEventPair:
	{
	  PKEVENT_PAIR EventPair;

	  if (ThreadInformationLength != sizeof(HANDLE))
	    {
	      Status = STATUS_INFO_LENGTH_MISMATCH;
	      break;
	    }

	  if (ExGetPreviousMode() == UserMode) /* FIXME: Validate this for all infoclasses and system services */
	    {
	      DPRINT("NtSetInformationThread:ThreadEventPair: Checking user pointer %08x...\n", ThreadInformation);
	      ProbeForRead(ThreadInformation, sizeof(HANDLE), sizeof(HANDLE)); /* FIXME: This entire function should be
	       * wrapped in an SEH frame... return (NTSTATUS)GetExceptionCode() on exception */
	    }

	  Status = ObReferenceObjectByHandle(*(PHANDLE)ThreadInformation,
					     STANDARD_RIGHTS_ALL,
					     ExEventPairObjectType,
					     ExGetPreviousMode(),
					     (PVOID*)&EventPair,
					     NULL);

	  if (!NT_SUCCESS(Status))
	    {
	      break;
	    }

	  ExpSwapThreadEventPair(Thread, EventPair); /* Note that the extra reference is kept intentionally */
	  Status = STATUS_SUCCESS;
	  break;
	}
	
      case ThreadQuerySetWin32StartAddress:
	if (ThreadInformationLength != sizeof(ULONG))
	  {
	    Status = STATUS_INFO_LENGTH_MISMATCH;
	    break;
	  }
	Thread->u2.Win32StartAddress = (PVOID)*((PULONG)ThreadInformation);
	Status = STATUS_SUCCESS;
	break;

      case ThreadZeroTlsCell:
	{
	  Status = STATUS_NOT_IMPLEMENTED;
	  break;
	}

      case ThreadPerformanceCount:
	/* Can only be queried */
	Status = STATUS_INVALID_INFO_CLASS;
	break;

      case ThreadAmILastThread:
	/* Can only be queried */
	Status = STATUS_INVALID_INFO_CLASS;
	break;

      case ThreadIdealProcessor:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ThreadPriorityBoost:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ThreadSetTlsArrayAddress:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

       case ThreadIsIoPending:
	/* Can only be queried */
	Status = STATUS_INVALID_INFO_CLASS;
	break;

      case ThreadHideFromDebugger:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      default:
	Status = STATUS_UNSUCCESSFUL;
    }

  ObDereferenceObject (Thread);

  return Status;
}


NTSTATUS STDCALL
NtQueryInformationThread (IN	HANDLE		ThreadHandle,
			  IN	THREADINFOCLASS	ThreadInformationClass,
			  OUT	PVOID		ThreadInformation,
			  IN	ULONG		ThreadInformationLength,
			  OUT	PULONG		ReturnLength)
{
   PETHREAD Thread;
   NTSTATUS Status;

   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_QUERY_INFORMATION,
				      PsThreadType,
				      ExGetPreviousMode(),
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   switch (ThreadInformationClass)
     {
     case ThreadBasicInformation:
       {
	 PTHREAD_BASIC_INFORMATION TBI;
	 
	 TBI = (PTHREAD_BASIC_INFORMATION)ThreadInformation;
	 
	 if (ThreadInformationLength != sizeof(THREAD_BASIC_INFORMATION))
	   {
	     Status = STATUS_INFO_LENGTH_MISMATCH;
	     break;
	   }
	 
	 TBI->ExitStatus = Thread->ExitStatus;
	 TBI->TebBaseAddress = Thread->Tcb.Teb;
	 TBI->ClientId = Thread->Cid;
	 TBI->AffinityMask = Thread->Tcb.Affinity;
	 TBI->Priority = Thread->Tcb.Priority;
	 TBI->BasePriority = Thread->Tcb.BasePriority;
	 Status = STATUS_SUCCESS;
	 break;
       }
       
     case ThreadTimes:
       Status = STATUS_NOT_IMPLEMENTED;
       break;
       
     case ThreadPriority:
       /* Can be set only */
       Status = STATUS_INVALID_INFO_CLASS;
       break;
       
     case ThreadBasePriority:
       /* Can be set only */
       Status = STATUS_INVALID_INFO_CLASS;
       break;
       
     case ThreadAffinityMask:
       /* Can be set only */
       Status = STATUS_INVALID_INFO_CLASS;
       break;

     case ThreadImpersonationToken:
       /* Can be set only */
       Status = STATUS_INVALID_INFO_CLASS;
       break;
       
     case ThreadDescriptorTableEntry:
       /* Nebbett says nothing about this */
       Status = STATUS_NOT_IMPLEMENTED;
       break;

     case ThreadEnableAlignmentFaultFixup:
       /* Can be set only */
       Status = STATUS_INVALID_INFO_CLASS;
       break;
       
     case ThreadEventPair:
       /* Can be set only */
       Status = STATUS_INVALID_INFO_CLASS;
       break;

     case ThreadQuerySetWin32StartAddress:
       if (ThreadInformationLength != sizeof(PVOID))
	 {
	   Status = STATUS_INFO_LENGTH_MISMATCH;
	   break;
	 }
       *((PVOID*)ThreadInformation) = Thread->u2.Win32StartAddress;
       Status = STATUS_SUCCESS;
       break;

     case ThreadZeroTlsCell:
       /* Can only be set */
       Status = STATUS_INVALID_INFO_CLASS;
       break;

     case ThreadPerformanceCount:
       /* Nebbett says this class is always zero */
       if (ThreadInformationLength != sizeof(LARGE_INTEGER))
	 {
	   Status = STATUS_INFO_LENGTH_MISMATCH;
	   break;
	 }
       ((PLARGE_INTEGER)ThreadInformation)->QuadPart = 0;
       Status = STATUS_SUCCESS;
       break;

     case ThreadAmILastThread:
       {
	 if (ThreadInformationLength != sizeof(BOOLEAN))
	   {
	     Status = STATUS_INFO_LENGTH_MISMATCH;
	     break;
	   }
	 if (Thread->ThreadsProcess->ThreadListHead.Flink->Flink ==
	     &Thread->ThreadsProcess->ThreadListHead)
	   {
	     *((PBOOLEAN)ThreadInformation) = TRUE;
	   }
	 else
	   {
	     *((PBOOLEAN)ThreadInformation) = FALSE;
	   }
	 Status = STATUS_SUCCESS;
	 break;
       }

     case ThreadIdealProcessor:
       /* Can only be set */
       Status = STATUS_INFO_LENGTH_MISMATCH;
       break;

     case ThreadPriorityBoost:
       Status = STATUS_NOT_IMPLEMENTED;
       break;

     case ThreadSetTlsArrayAddress:
       /* Can only be set */
       Status = STATUS_INVALID_INFO_CLASS;
       break;

     case ThreadIsIoPending:
       Status = STATUS_NOT_IMPLEMENTED;
       break;
       
     case ThreadHideFromDebugger:
       /* Can only be set */
       Status = STATUS_INVALID_INFO_CLASS;
       break;

      default:
	Status = STATUS_INVALID_INFO_CLASS;
     }
   ObDereferenceObject(Thread);
   return(Status);
}


VOID
KeSetPreviousMode (ULONG Mode)
{
  PsGetCurrentThread()->Tcb.PreviousMode = (UCHAR)Mode;
}


/*
 * @implemented
 */
ULONG STDCALL
KeGetPreviousMode (VOID)
{
  return (ULONG)PsGetCurrentThread()->Tcb.PreviousMode;
}


/*
 * @implemented
 */
KPROCESSOR_MODE STDCALL
ExGetPreviousMode (VOID)
{
  return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/* EOF */
