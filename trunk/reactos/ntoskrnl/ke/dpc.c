/*
 *  ReactOS kernel
 *  Copyright (C) 2000, 1999, 1998 David Welch <welch@cwcom.net>,
 *                                 Philip Susi <phreak@iag.net>,
 *                                 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: dpc.c,v 1.37 2004/10/13 01:42:14 ion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/dpc.c
 * PURPOSE:         Handle DPCs (Delayed Procedure Calls)
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                28/05/98: Created
 *                12/3/99:  Phillip Susi: Fixed IRQL problem
 */

/*
 * NOTE: See also the higher level support routines in ntoskrnl/io/dpc.c
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

static LIST_ENTRY DpcQueueHead; /* Head of the list of pending DPCs */
static KSPIN_LOCK DpcQueueLock; /* Lock for the above list */

/*
 * Number of pending DPCs. This is inspected by
 * the idle thread to determine if the queue needs to
 * be run down
 */
ULONG DpcQueueSize = 0;

/*
 * Number of DPC's Processed.
 */
ULONG DpcCount = 0;

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
KeInitializeDpc (PKDPC			Dpc,
		 PKDEFERRED_ROUTINE	DeferredRoutine,
		 PVOID			DeferredContext)
/*
 * FUNCTION: Initalizes a DPC
 * ARGUMENTS:
 *          Dpc = Caller supplied DPC to be initialized
 *          DeferredRoutine = Associated DPC callback
 *          DeferredContext = Parameter to be passed to the callback
 * NOTE: Callers must be running at IRQL PASSIVE_LEVEL
 */
{
   Dpc->Type = 0;
   Dpc->DeferredRoutine = DeferredRoutine;
   Dpc->DeferredContext = DeferredContext;
   Dpc->Lock = 0;
}

/*
 * @implemented
 */
VOID STDCALL
KiDispatchInterrupt(VOID)
/*
 * FUNCTION: Called to execute queued dpcs
 */
{
   PLIST_ENTRY current_entry;
   PKDPC current;
   KIRQL oldlvl;

   assert_irql(DISPATCH_LEVEL);

   if (DpcQueueSize == 0)
     {
	return;
     }

   KeRaiseIrql(HIGH_LEVEL, &oldlvl);
   KiAcquireSpinLock(&DpcQueueLock);

   DpcCount = DpcCount + DpcQueueSize;
   
   while (!IsListEmpty(&DpcQueueHead))
   {
      current_entry = RemoveHeadList(&DpcQueueHead);
      DpcQueueSize--;

      assert(DpcQueueSize || IsListEmpty(&DpcQueueHead));

      current = CONTAINING_RECORD(current_entry,KDPC,DpcListEntry);
      current->Lock=FALSE;
      KiReleaseSpinLock(&DpcQueueLock);
      KeLowerIrql(oldlvl);
      current->DeferredRoutine(current,current->DeferredContext,
			       current->SystemArgument1,
			       current->SystemArgument2);

      KeRaiseIrql(HIGH_LEVEL, &oldlvl);
      KiAcquireSpinLock(&DpcQueueLock);
   }

   KiReleaseSpinLock(&DpcQueueLock);
   KeLowerIrql(oldlvl);
}

/*
 * @unimplemented
 */
VOID
STDCALL
KeFlushQueuedDpcs(
	VOID
	)
{
	UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN 
STDCALL
KeIsExecutingDpc(
	VOID
)
{
 	return KeGetCurrentKPCR()->PrcbData.DpcRoutineActive;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeRemoveQueueDpc (PKDPC	Dpc)
/*
 * FUNCTION: Removes DPC object from the system dpc queue
 * ARGUMENTS:
 *          Dpc = DPC to remove
 * RETURNS: TRUE if the DPC was in the queue
 *          FALSE otherwise
 */
{
   KIRQL oldIrql;
   BOOLEAN WasInQueue;

   KeRaiseIrql(HIGH_LEVEL, &oldIrql);
   KiAcquireSpinLock(&DpcQueueLock);
   WasInQueue = Dpc->Lock ? TRUE : FALSE;
   if (WasInQueue)
     {
	RemoveEntryList(&Dpc->DpcListEntry);
	DpcQueueSize--;
	Dpc->Lock=0;
     }

   assert(DpcQueueSize || IsListEmpty(&DpcQueueHead));

   KiReleaseSpinLock(&DpcQueueLock);
   KeLowerIrql(oldIrql);

   return WasInQueue;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeInsertQueueDpc (PKDPC	Dpc,
		  PVOID	SystemArgument1,
		  PVOID	SystemArgument2)
/*
 * FUNCTION: Queues a DPC for execution when the IRQL of a processor
 * drops below DISPATCH_LEVEL
 * ARGUMENTS:
 *          Dpc = Initalizes DPC
 *          SystemArguments[1-2] = Undocumented
 * RETURNS: TRUE if the DPC object wasn't already in the queue
 *          FALSE otherwise
 */
{
   KIRQL oldlvl;
   DPRINT("KeInsertQueueDpc(dpc %x, SystemArgument1 %x, SystemArgument2 %x)\n",
	  Dpc, SystemArgument1, SystemArgument2);

   assert(KeGetCurrentIrql()>=DISPATCH_LEVEL);

   Dpc->Number=0;
   Dpc->Importance=MediumImportance;
   Dpc->SystemArgument1=SystemArgument1;
   Dpc->SystemArgument2=SystemArgument2;
   if (Dpc->Lock)
     {
	return(FALSE);
     }
   KeRaiseIrql(HIGH_LEVEL, &oldlvl);
   KiAcquireSpinLock(&DpcQueueLock);
   assert(DpcQueueSize || IsListEmpty(&DpcQueueHead));
   InsertHeadList(&DpcQueueHead,&Dpc->DpcListEntry);
   DPRINT("Dpc->DpcListEntry.Flink %x\n", Dpc->DpcListEntry.Flink);
   DpcQueueSize++;
   Dpc->Lock=(PULONG)1;
   KiReleaseSpinLock(&DpcQueueLock);
   KeLowerIrql(oldlvl);
   DPRINT("DpcQueueHead.Flink %x\n",DpcQueueHead.Flink);
   DPRINT("Leaving KeInsertQueueDpc()\n",0);
   return(TRUE);
}

/*
 * FUNCTION: Specifies the DPCs importance
 * ARGUMENTS:
 *          Dpc = Initalizes DPC
 *          Importance = DPC importance
 * RETURNS: None
 *
 * @implemented
 */
VOID STDCALL
KeSetImportanceDpc (IN	PKDPC		Dpc,
		    IN	KDPC_IMPORTANCE	Importance)
{
	Dpc->Importance = Importance;
}

/*
 * FUNCTION: Specifies on which processor the DPC will run
 * ARGUMENTS:
 *          Dpc = Initalizes DPC
 *          Number = Processor number
 * RETURNS: None
 *
 * @unimplemented
 */
VOID STDCALL
KeSetTargetProcessorDpc (IN	PKDPC	Dpc,
			 IN	CCHAR	Number)
{
	UNIMPLEMENTED;
}

VOID INIT_FUNCTION
KeInitDpc(VOID)
/*
 * FUNCTION: Initialize DPC handling
 */
{
   InitializeListHead(&DpcQueueHead);
   KeInitializeSpinLock(&DpcQueueLock);
}

/* EOF */
