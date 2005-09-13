/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/win32.c
 * PURPOSE:         win32k support
 *
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

static PW32_PROCESS_CALLBACK PspWin32ProcessCallback = NULL;
static PW32_THREAD_CALLBACK PspWin32ThreadCallback = NULL;

extern OB_OPEN_METHOD ExpWindowStationObjectOpen;
extern OB_PARSE_METHOD ExpWindowStationObjectParse;
extern OB_DELETE_METHOD ExpWindowStationObjectDelete;
extern OB_FIND_METHOD ExpWindowStationObjectFind;
extern OB_CREATE_METHOD ExpDesktopObjectCreate;
extern OB_DELETE_METHOD ExpDesktopObjectDelete;

#ifndef ALEX_CB_REWRITE
typedef struct _NTW32CALL_SAVED_STATE
{
  ULONG_PTR SavedStackLimit;
  PVOID SavedStackBase;
  PVOID SavedInitialStack;
  PVOID CallerResult;
  PULONG CallerResultLength;
  PNTSTATUS CallbackStatus;
  PKTRAP_FRAME SavedTrapFrame;
  PVOID SavedCallbackStack;
  PVOID SavedExceptionStack;
} NTW32CALL_SAVED_STATE, *PNTW32CALL_SAVED_STATE;
#endif

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
VOID 
STDCALL
PsEstablishWin32Callouts(PW32_CALLOUT_DATA CalloutData)
{
    PspWin32ProcessCallback = CalloutData->W32ProcessCallout;
    PspWin32ThreadCallback = CalloutData->W32ThreadCallout;
    ExpWindowStationObjectOpen = CalloutData->WinStaCreate;
    ExpWindowStationObjectParse = CalloutData->WinStaParse;
    ExpWindowStationObjectDelete = CalloutData->WinStaDelete;
    ExpWindowStationObjectFind = CalloutData->WinStaFind;
    ExpDesktopObjectCreate = CalloutData->DesktopCreate;
    ExpDesktopObjectDelete = CalloutData->DesktopDelete;
}

NTSTATUS
NTAPI
PsInitWin32Thread (PETHREAD Thread)
{
    PEPROCESS Process;
    NTSTATUS Status = STATUS_SUCCESS;

    Process = Thread->ThreadsProcess;

    if (Process->Win32Process == NULL)
    {
        if (PspWin32ProcessCallback != NULL)
        {
            Status = PspWin32ProcessCallback(Process, TRUE);
        }
    }

    if (Thread->Tcb.Win32Thread == NULL)
    {
        if (PspWin32ThreadCallback != NULL)
        {
            Status = PspWin32ThreadCallback(Thread, TRUE);
        }
    }

    return Status;
}


VOID
NTAPI
PsTerminateWin32Process (PEPROCESS Process)
{
  if (Process->Win32Process == NULL)
    return;

  if (PspWin32ProcessCallback != NULL)
    {
      PspWin32ProcessCallback (Process, FALSE);
    }

  /* don't delete the W32PROCESS structure at this point, wait until the
     EPROCESS structure is being freed */
}


VOID
NTAPI
PsTerminateWin32Thread (PETHREAD Thread)
{
  if (Thread->Tcb.Win32Thread != NULL)
  {
    if (PspWin32ThreadCallback != NULL)
    {
      PspWin32ThreadCallback (Thread, FALSE);
    }

    /* don't delete the W32THREAD structure at this point, wait until the
       ETHREAD structure is being freed */
  }
}

VOID
STDCALL
DumpEspData(ULONG Esp, ULONG ThLimit, ULONG ThStack, ULONG PcrLimit, ULONG PcrStack, ULONG Esp0)
{
    DPRINT1("Current Esp: %p\n Thread Stack Limit: %p\n Thread Stack: %p\n Pcr Limit: %p, Pcr Stack: %p\n Esp0 :%p\n",Esp, ThLimit, ThStack, PcrLimit, PcrStack, Esp0)   ;
}

 PVOID
STDCALL
 PsAllocateCallbackStack(ULONG StackSize)
 {
   PVOID KernelStack = NULL;
   NTSTATUS Status;
   PMEMORY_AREA StackArea;
   ULONG i, j;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   PPFN_TYPE Pages = alloca(sizeof(PFN_TYPE) * (StackSize /PAGE_SIZE));

    DPRINT1("PsAllocateCallbackStack\n");
   BoundaryAddressMultiple.QuadPart = 0;
   StackSize = PAGE_ROUND_UP(StackSize);
   MmLockAddressSpace(MmGetKernelAddressSpace());
   Status = MmCreateMemoryArea(NULL,
                               MmGetKernelAddressSpace(),
                               MEMORY_AREA_KERNEL_STACK,
                               &KernelStack,
                               StackSize,
                               0,
                               &StackArea,
                               FALSE,
                               FALSE,
                               BoundaryAddressMultiple);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("Failed to create thread stack\n");
       return(NULL);
     }
   for (i = 0; i < (StackSize / PAGE_SIZE); i++)
     {
       Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Pages[i]);
       if (!NT_SUCCESS(Status))
         {
           for (j = 0; j < i; j++)
           {
             MmReleasePageMemoryConsumer(MC_NPPOOL, Pages[j]);
           }
           return(NULL);
         }
     }
   Status = MmCreateVirtualMapping(NULL,
                                  KernelStack,
                                  PAGE_READWRITE,
                                   Pages,
                                   StackSize / PAGE_SIZE);
   if (!NT_SUCCESS(Status))
     {
      for (i = 0; i < (StackSize / PAGE_SIZE); i++)
         {
           MmReleasePageMemoryConsumer(MC_NPPOOL, Pages[i]);
         }
       return(NULL);
     }
     DPRINT1("PsAllocateCallbackStack %x\n", KernelStack);
   return(KernelStack);
}

NTSTATUS
STDCALL
NtW32Call(IN ULONG RoutineIndex,
          IN PVOID Argument,
          IN ULONG ArgumentLength,
          OUT PVOID* Result OPTIONAL,
          OUT PULONG ResultLength OPTIONAL)
{
    NTSTATUS CallbackStatus;

    DPRINT("NtW32Call(RoutineIndex %d, Argument %X, ArgumentLength %d)\n",
            RoutineIndex, Argument, ArgumentLength);

    /* FIXME: SEH!!! */

    /* Call kernel function */
    CallbackStatus = KeUserModeCallback(RoutineIndex,
                                        Argument,
                                        ArgumentLength,
                                        Result,
                                        ResultLength);

    /* Return the result */
    return(CallbackStatus);
}

#ifndef ALEX_CB_REWRITE
NTSTATUS STDCALL
NtCallbackReturn (PVOID		Result,
		  ULONG		ResultLength,
		  NTSTATUS	Status)
{
  PULONG OldStack;
  PETHREAD Thread;
  PNTSTATUS CallbackStatus;
  PULONG CallerResultLength;
  PVOID* CallerResult;
  PVOID InitialStack;
  PVOID StackBase;
  ULONG_PTR StackLimit;
  KIRQL oldIrql;
  PNTW32CALL_SAVED_STATE State;
  PKTRAP_FRAME SavedTrapFrame;
  PVOID SavedCallbackStack;
  PVOID SavedExceptionStack;

  PAGED_CODE();

  Thread = PsGetCurrentThread();
  if (Thread->Tcb.CallbackStack == NULL)
    {
      return(STATUS_NO_CALLBACK_ACTIVE);
    }

  OldStack = (PULONG)Thread->Tcb.CallbackStack;

  /*
   * Get the values that NtW32Call left on the inactive stack for us.
   */
  State = (PNTW32CALL_SAVED_STATE)OldStack[0];
  CallbackStatus = State->CallbackStatus;
  CallerResultLength = State->CallerResultLength;
  CallerResult = State->CallerResult;
  InitialStack = State->SavedInitialStack;
  StackBase = State->SavedStackBase;
  StackLimit = State->SavedStackLimit;
  SavedTrapFrame = State->SavedTrapFrame;
  SavedCallbackStack = State->SavedCallbackStack;
  SavedExceptionStack = State->SavedExceptionStack;

  /*
   * Copy the callback status and the callback result to NtW32Call
   */
  *CallbackStatus = Status;
  if (CallerResult != NULL && CallerResultLength != NULL)
    {
      if (Result == NULL)
	{
	  *CallerResultLength = 0;
	}
      else
	{
	  *CallerResultLength = min(ResultLength, *CallerResultLength);
	  RtlCopyMemory(*CallerResult, Result, *CallerResultLength);
	}
    }

  /*
   * Restore the old stack.
   */
  KeRaiseIrql(HIGH_LEVEL, &oldIrql);
  if ((Thread->Tcb.NpxState & NPX_STATE_VALID) &&
      &Thread->Tcb != KeGetCurrentPrcb()->NpxThread)
    {
      RtlCopyMemory((char*)InitialStack - sizeof(FX_SAVE_AREA),
                    (char*)Thread->Tcb.InitialStack - sizeof(FX_SAVE_AREA),
                    sizeof(FX_SAVE_AREA));
    }
  Thread->Tcb.InitialStack = InitialStack;
  Thread->Tcb.StackBase = StackBase;
  Thread->Tcb.StackLimit = StackLimit;
  Thread->Tcb.TrapFrame = SavedTrapFrame;
  Thread->Tcb.CallbackStack = SavedCallbackStack;
  KeGetCurrentKPCR()->TSS->Esp0 = (ULONG)SavedExceptionStack;
  KeStackSwitchAndRet((PVOID)(OldStack + 1));

  /* Should never return. */
  KEBUGCHECK(0);
  return(STATUS_UNSUCCESSFUL);
}
#endif
/* EOF */
