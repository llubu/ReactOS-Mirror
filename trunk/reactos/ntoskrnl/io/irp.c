/* $Id: irp.c,v 1.71 2004/12/26 15:55:14 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/irp.c
 * PURPOSE:         Handle IRPs
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 *                  24/05/98: Created 
 */

/* NOTES *******************************************************************
 * 
 * Layout of an IRP 
 * 
 *             ################
 *             #    Headers   #
 *             ################
 *             #              #
 *             #   Variable   #
 *             # length list  #
 *             # of io stack  #
 *             #  locations   #
 *             #              #
 *             ################
 * 
 * 
 * 
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_IRP     TAG('I', 'R', 'P', ' ')


/* FUNCTIONS ****************************************************************/

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
IoForwardIrpSynchronously(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @implemented
 */
VOID STDCALL
IoFreeIrp(PIRP Irp)
/*
 * FUNCTION: Releases a caller allocated irp
 * ARGUMENTS:
 *      Irp = Irp to free
 */
{
  ExFreePool(Irp);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
IoGetRequestorProcessId(
    IN PIRP Irp
    )
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoGetRequestorSessionId(
	IN PIRP Irp,
	OUT PULONG pSessionId
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
BOOLEAN
STDCALL
IoIsValidNameGraftingBuffer(
    IN PIRP Irp,
    IN PREPARSE_DATA_BUFFER ReparseBuffer
    )
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @implemented
 */
PIRP STDCALL
IoMakeAssociatedIrp(PIRP Irp,
		    CCHAR StackSize)
/*
 * FUNCTION: Allocates and initializes an irp to associated with a master irp
 * ARGUMENTS:
 *       Irp = Master irp
 *       StackSize = Number of stack locations to be allocated in the irp
 * RETURNS: The irp allocated
 * NOTE: The caller is responsible for incrementing
 *       Irp->AssociatedIrp.IrpCount.
 */
{
   PIRP AssocIrp;

   /* Allocate the IRP */
   AssocIrp = IoAllocateIrp(StackSize,FALSE);
   if (AssocIrp == NULL)
      return NULL;

   /* Set the Flags */
   AssocIrp->Flags |= IRP_ASSOCIATED_IRP;

   /* Set the Thread */
   AssocIrp->Tail.Overlay.Thread = Irp->Tail.Overlay.Thread;

   /* Associate them */
   AssocIrp->AssociatedIrp.MasterIrp = Irp;
 
   return AssocIrp;
}


/*
 * @implemented
 */
VOID STDCALL
IoInitializeIrp(PIRP Irp,
		USHORT PacketSize,
		CCHAR StackSize)
/*
 * FUNCTION: Initalizes an irp allocated by the caller
 * ARGUMENTS:
 *          Irp = IRP to initalize
 *          PacketSize = Size in bytes of the IRP
 *          StackSize = Number of stack locations in the IRP
 */
{
  ASSERT(Irp != NULL);

  DPRINT("IoInitializeIrp(StackSize %x, Irp %x)\n",StackSize, Irp);
  memset(Irp, 0, PacketSize);
  Irp->Size = PacketSize;
  Irp->StackCount = StackSize;
  Irp->CurrentLocation = StackSize;
  InitializeListHead(&Irp->ThreadListEntry);
  Irp->Tail.Overlay.CurrentStackLocation = (PIO_STACK_LOCATION)(Irp + 1) + StackSize;
  DPRINT("Irp->Tail.Overlay.CurrentStackLocation %x\n", Irp->Tail.Overlay.CurrentStackLocation);
  Irp->ApcEnvironment =  KeGetCurrentThread()->ApcStateIndex;
}


/*
 * @implemented
 */
NTSTATUS FASTCALL
IofCallDriver(PDEVICE_OBJECT DeviceObject,
	      PIRP Irp)
/*
  * FUNCTION: Sends an IRP to the next lower driver
 */
{
  PDRIVER_OBJECT DriverObject;
  PIO_STACK_LOCATION Param;

  DPRINT("IofCallDriver(DeviceObject %x, Irp %x)\n",DeviceObject,Irp);

  ASSERT(Irp);
  ASSERT(DeviceObject);

  DriverObject = DeviceObject->DriverObject;

  ASSERT(DriverObject);

  IoSetNextIrpStackLocation(Irp);
  Param = IoGetCurrentIrpStackLocation(Irp);

  DPRINT("IrpSp 0x%X\n", Param);

  Param->DeviceObject = DeviceObject;

  DPRINT("MajorFunction %d\n", Param->MajorFunction);
  DPRINT("DriverObject->MajorFunction[Param->MajorFunction] %x\n",
    DriverObject->MajorFunction[Param->MajorFunction]);

  return DriverObject->MajorFunction[Param->MajorFunction](DeviceObject, Irp);
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
IoCallDriver (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  return(IofCallDriver(DeviceObject,
		       Irp));
}


/*
 * @implemented
 */
PIRP STDCALL
IoAllocateIrp(CCHAR StackSize,
	      BOOLEAN ChargeQuota)
/*
 * FUNCTION: Allocates an IRP
 * ARGUMENTS:
 *          StackSize = the size of the stack required for the irp
 *          ChargeQuota = Charge allocation to current threads quota
 * RETURNS: Irp allocated
 */
{
  PIRP Irp;

#if 0
  DbgPrint("IoAllocateIrp(StackSize %d ChargeQuota %d)\n",
	   StackSize,
	   ChargeQuota);
  KeDumpStackFrames(0,8);
#endif
  
  if (ChargeQuota)
    {
//      Irp = ExAllocatePoolWithQuota(NonPagedPool,IoSizeOfIrp(StackSize));
      Irp = ExAllocatePoolWithTag(NonPagedPool,
				  IoSizeOfIrp(StackSize),
				  TAG_IRP);
    }
  else
    {
      Irp = ExAllocatePoolWithTag(NonPagedPool,
				  IoSizeOfIrp(StackSize),
				  TAG_IRP);
    }

  if (Irp==NULL)
    {
      return(NULL);
    }

  RtlZeroMemory(Irp, IoSizeOfIrp(StackSize));
  IoInitializeIrp(Irp,
		  IoSizeOfIrp(StackSize),
		  StackSize);

//  DPRINT("Irp %x Irp->StackPtr %d\n", Irp, Irp->CurrentLocation);

  return(Irp);
}


/*
 * @implemented
 */
VOID FASTCALL
IofCompleteRequest(PIRP Irp,
         CCHAR PriorityBoost)
/*
 * FUNCTION: Indicates the caller has finished all processing for a given
 * I/O request and is returning the given IRP to the I/O manager
 * ARGUMENTS:
 *         Irp = Irp to be cancelled
 *         PriorityBoost = Increment by which to boost the priority of the
 *                         thread making the request
 */
{
   ULONG             i;
   NTSTATUS          Status;
   PFILE_OBJECT      OriginalFileObject;
   PDEVICE_OBJECT    DeviceObject;
   KIRQL             oldIrql;
   PMDL              Mdl;
   PIO_STACK_LOCATION Stack = (PIO_STACK_LOCATION)(Irp + 1);

   DPRINT("IoCompleteRequest(Irp %x, PriorityBoost %d) Event %x THread %x\n",
      Irp,PriorityBoost, Irp->UserEvent, PsGetCurrentThread());

   ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
   ASSERT(Irp->CancelRoutine == NULL);
   ASSERT(Irp->IoStatus.Status != STATUS_PENDING);

   Irp->PendingReturned = IoGetCurrentIrpStackLocation(Irp)->Control & SL_PENDING_RETURNED;
   
   /*
    * Run the completion routines.
    */

   for (i=Irp->CurrentLocation;i<(ULONG)Irp->StackCount;i++)
   {
      /*
      Completion routines expect the current irp stack location to be the same as when
      IoSetCompletionRoutine was called to set them. A side effect is that completion
      routines set by highest level drivers without their own stack location will receive
      an invalid current stack location (at least it should be considered as invalid).
      Since the DeviceObject argument passed is taken from the current stack, this value
      is also invalid (NULL).
      */
      if (Irp->CurrentLocation < Irp->StackCount - 1)
      {
         IoSetPreviousIrpStackLocation(Irp);
         DeviceObject = IoGetCurrentIrpStackLocation(Irp)->DeviceObject;
      }
      else
      {
         DeviceObject = NULL;
      }

      if (Stack[i].CompletionRoutine != NULL &&
         ((NT_SUCCESS(Irp->IoStatus.Status) && (Stack[i].Control & SL_INVOKE_ON_SUCCESS)) ||
         (!NT_SUCCESS(Irp->IoStatus.Status) && (Stack[i].Control & SL_INVOKE_ON_ERROR)) ||
         (Irp->Cancel && (Stack[i].Control & SL_INVOKE_ON_CANCEL))))
      {
         Status = Stack[i].CompletionRoutine(DeviceObject,
                                                  Irp,
                                                  Stack[i].Context);

         if (Status == STATUS_MORE_PROCESSING_REQUIRED)
         {
            return;
         }
      }
   
      if (IoGetCurrentIrpStackLocation(Irp)->Control & SL_PENDING_RETURNED)
      {
         Irp->PendingReturned = TRUE;
      }
   }

   /* Windows NT File System Internals, page 165 */
   if (Irp->Flags & IRP_ASSOCIATED_IRP)
   {
      ULONG MasterIrpCount;
      PIRP MasterIrp = Irp->AssociatedIrp.MasterIrp;

      MasterIrpCount = InterlockedDecrement(&MasterIrp->AssociatedIrp.IrpCount);
      while ((Mdl = Irp->MdlAddress))
      {
         Irp->MdlAddress = Mdl->Next;
         IoFreeMdl(Mdl);
      }
      IoFreeIrp(Irp);
      if (MasterIrpCount == 0)
      {
         IofCompleteRequest(MasterIrp, IO_NO_INCREMENT);
      }
      return;
   }

   /*
    * Were done calling completion routines. Now do any cleanup that can be 
    * done in an arbitrarily context.
    */

   /* Windows NT File System Internals, page 165 */
   if (Irp->Flags & (IRP_PAGING_IO|IRP_CLOSE_OPERATION))
   {
      /* 
       * If MDL_IO_PAGE_READ is set, then the caller is responsible 
       * for deallocating of the mdl. 
       */
      if (Irp->Flags & IRP_PAGING_IO &&
          Irp->MdlAddress &&
          !(Irp->MdlAddress->MdlFlags & MDL_IO_PAGE_READ))
      {

         if (Irp->MdlAddress->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
         {
            MmUnmapLockedPages(Irp->MdlAddress->MappedSystemVa, Irp->MdlAddress);
         }

         ExFreePool(Irp->MdlAddress);
      }

      if (Irp->UserIosb)
      {
         *Irp->UserIosb = Irp->IoStatus;
      }

      if (Irp->UserEvent)
      {
         KeSetEvent(Irp->UserEvent, PriorityBoost, FALSE);
      }
      
      /* io manager frees the irp for close operations */
//      if (Irp->Flags & IRP_PAGING_IO)
//      {
         IoFreeIrp(Irp);
//      }
      
      return;
   }
   
   
   /*
   Hi Dave,
   
    I went through my old notes. You are correct and in most cases
   IoCompleteRequest() will issue an MmUnlockPages() for each MDL in the IRP
   chain. There are however few exceptions: one is MDLs for associated IRPs,
   it's expected that those MDLs have been initialized with
   IoBuildPartialMdl(). Another exception is PAGING_IO irps, the i/o completion
   code doesn't do anything to MDLs of those IRPs.
   
   sara
   
*/
   

   for (Mdl = Irp->MdlAddress; Mdl; Mdl = Mdl->Next)
   {
      /* 
       * Undo the MmProbeAndLockPages. If MmGetSystemAddressForMdl was called
       * on this mdl, this mapping (if any) is also undone by MmUnlockPages.
       */
      MmUnlockPages(Mdl);
   }
    
   //Windows NT File System Internals, page 154
   OriginalFileObject = Irp->Tail.Overlay.OriginalFileObject;

   if (NULL != Stack->FileObject
       && NULL != Stack->FileObject->CompletionContext
       && Irp->PendingReturned)
   {
      PFILE_OBJECT FileObject = Stack->FileObject;
      IoSetIoCompletion(FileObject->CompletionContext->Port,
                        FileObject->CompletionContext->Key,
                        Irp->Overlay.AsynchronousParameters.UserApcContext,
                        Irp->IoStatus.Status,
                        Irp->IoStatus.Information,
                        FALSE);
   }

   if (Irp->PendingReturned || KeGetCurrentIrql() == DISPATCH_LEVEL)
   {
      BOOLEAN bStatus;
      
      DPRINT("Dispatching APC\n");

      KeInitializeApc(  &Irp->Tail.Apc,
                             &Irp->Tail.Overlay.Thread->Tcb,
                             Irp->ApcEnvironment,
                             IoSecondStageCompletion,//kernel routine
                             NULL,
                             (PKNORMAL_ROUTINE) NULL,
                             KernelMode,
                             NULL);
      
      bStatus = KeInsertQueueApc(&Irp->Tail.Apc,
                                      (PVOID)OriginalFileObject,
                                      NULL, // This is used for REPARSE stuff
                                      PriorityBoost);

      if (bStatus == FALSE)
      {
         DPRINT1("Error queueing APC for thread. Thread has probably exited.\n");
      }

      DPRINT("Finished dispatching APC\n");
   }
   else
   {
      DPRINT("Calling IoSecondStageCompletion routine directly\n");
      KeRaiseIrql(APC_LEVEL, &oldIrql);
      IoSecondStageCompletion(&Irp->Tail.Apc,NULL,NULL,(PVOID)&OriginalFileObject, NULL);
      KeLowerIrql(oldIrql);
      DPRINT("Finished completition routine\n");
   }
}


/*
 * @implemented
 */
VOID STDCALL
IoCompleteRequest(PIRP Irp,
		  CCHAR PriorityBoost)
{
  IofCompleteRequest(Irp, PriorityBoost);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	IoIsOperationSynchronous@4
 *
 * DESCRIPTION
 *	Check if the I/O operation associated with the given IRP
 *	is synchronous.
 *
 * ARGUMENTS
 * 	Irp 	Packet to check.
 *
 * RETURN VALUE
 * 	TRUE if Irp's operation is synchronous; otherwise FALSE.
 *
 * @implemented
 */
BOOLEAN STDCALL
IoIsOperationSynchronous(IN PIRP Irp)
{
   PFILE_OBJECT FileObject = NULL;

   FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;
  
   if (Irp->Flags & IRP_SYNCHRONOUS_PAGING_IO)
   {
      return TRUE;
   }

   if (Irp->Flags & IRP_PAGING_IO)
   {
      return FALSE;
   }

   //NOTE: Windows 2000 crash if IoStack->FileObject == NULL, so I guess we should too;-)
   if (Irp->Flags & IRP_SYNCHRONOUS_API || FileObject->Flags & FO_SYNCHRONOUS_IO)
   {
      return TRUE;
   }

   /* Otherwise, it is an asynchronous operation. */
   return FALSE;
}


/*
 * @implemented
 */
VOID STDCALL
IoEnqueueIrp(IN PIRP Irp)
{
  IoQueueThreadIrp(Irp);
}


/*
 * @implemented
 */
VOID STDCALL
IoSetTopLevelIrp(IN PIRP Irp)
{
    PETHREAD Thread;

    Thread = PsGetCurrentThread();
    Thread->TopLevelIrp = Irp;
}


/*
 * @implemented
 */
PIRP STDCALL
IoGetTopLevelIrp(VOID)
{
    return(PsGetCurrentThread()->TopLevelIrp);
}


/*
 * @implemented
 */
VOID STDCALL
IoQueueThreadIrp(IN PIRP Irp)
{
/* undefine this when (if ever) implementing irp cancellation */
#if 0
  KIRQL oldIrql;
  
  oldIrql = KfRaiseIrql(APC_LEVEL);
  
  /* Synchronous irp's are queued to requestor thread. If they are not completed
  when the thread exits, they are canceled (cleaned up).
  -Gunnar */
  InsertTailList(&PsGetCurrentThread()->IrpList, &Irp->ThreadListEntry);
    
  KfLowerIrql(oldIrql);    
#endif
}


/*
 * @implemented
 */
VOID STDCALL
IoReuseIrp(
  IN OUT PIRP Irp,
  IN NTSTATUS Status)
{
  
  UCHAR AllocationFlags;
  
  /* Reference: Chris Cant's "Writing WDM Device Drivers" */
  AllocationFlags = Irp->AllocationFlags;
  IoInitializeIrp(Irp, Irp->Size, Irp->StackCount);
  Irp->IoStatus.Status = Status;
  Irp->AllocationFlags = AllocationFlags;
}

/* EOF */
