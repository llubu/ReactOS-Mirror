/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: balance.c,v 1.22 2003/10/23 20:28:08 hbirr Exp $
 *
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/balance.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 27/12/01
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef struct _MM_MEMORY_CONSUMER
{
  ULONG PagesUsed;
  ULONG PagesTarget;
  NTSTATUS (*Trim)(ULONG Target, ULONG Priority, PULONG NrFreed);
} MM_MEMORY_CONSUMER, *PMM_MEMORY_CONSUMER;

typedef struct _MM_ALLOCATION_REQUEST
{
  PHYSICAL_ADDRESS Page;
  LIST_ENTRY ListEntry;
  KEVENT Event;
} MM_ALLOCATION_REQUEST, *PMM_ALLOCATION_REQUEST;

/* GLOBALS ******************************************************************/

static MM_MEMORY_CONSUMER MiMemoryConsumers[MC_MAXIMUM];
static ULONG MiMinimumAvailablePages;
static ULONG MiNrAvailablePages;
static ULONG MiNrTotalPages;
static LIST_ENTRY AllocationListHead;
static KSPIN_LOCK AllocationListLock;
static ULONG MiPagesRequired = 0;
static ULONG MiMinimumPagesPerRun = 10;

/* FUNCTIONS ****************************************************************/

VOID MmPrintMemoryStatistic(VOID)
{
  DbgPrint("MC_CACHE %d, MC_USER %d, MC_PPOOL %d, MC_NPPOOL %d, MiNrAvailablePages %d\n",
           MiMemoryConsumers[MC_CACHE].PagesUsed, MiMemoryConsumers[MC_USER].PagesUsed, 
           MiMemoryConsumers[MC_PPOOL].PagesUsed, MiMemoryConsumers[MC_NPPOOL].PagesUsed,
	   MiNrAvailablePages); 
}

VOID INIT_FUNCTION
MmInitializeBalancer(ULONG NrAvailablePages, ULONG NrSystemPages)
{
  memset(MiMemoryConsumers, 0, sizeof(MiMemoryConsumers));
  InitializeListHead(&AllocationListHead);
  KeInitializeSpinLock(&AllocationListLock);

  MiNrAvailablePages = MiNrTotalPages = NrAvailablePages;

  /* Set up targets. */
  MiMinimumAvailablePages = 64;
  MiMemoryConsumers[MC_CACHE].PagesTarget = NrAvailablePages / 2;
  MiMemoryConsumers[MC_USER].PagesTarget = 
    NrAvailablePages - MiMinimumAvailablePages;
  MiMemoryConsumers[MC_PPOOL].PagesTarget = NrAvailablePages / 2;
  MiMemoryConsumers[MC_NPPOOL].PagesTarget = 0xFFFFFFFF;
  MiMemoryConsumers[MC_NPPOOL].PagesUsed = NrSystemPages;
}

VOID INIT_FUNCTION
MmInitializeMemoryConsumer(ULONG Consumer, 
			   NTSTATUS (*Trim)(ULONG Target, ULONG Priority, 
					    PULONG NrFreed))
{
  MiMemoryConsumers[Consumer].Trim = Trim;
}

NTSTATUS
MmReleasePageMemoryConsumer(ULONG Consumer, PHYSICAL_ADDRESS Page)
{
  PMM_ALLOCATION_REQUEST Request;
  PLIST_ENTRY Entry;
  KIRQL oldIrql;

  if (Page.QuadPart == 0LL)
    {
      DPRINT1("Tried to release page zero.\n");
      KEBUGCHECK(0);
    }

  KeAcquireSpinLock(&AllocationListLock, &oldIrql);
  if (MmGetReferenceCountPage(Page) == 1)
    {
      InterlockedDecrement((LONG *)&MiMemoryConsumers[Consumer].PagesUsed);
      InterlockedIncrement((LONG *)&MiNrAvailablePages);
      if (IsListEmpty(&AllocationListHead))
	{
	  KeReleaseSpinLock(&AllocationListLock, oldIrql);
	  MmDereferencePage(Page);
	}
      else
	{
	  Entry = RemoveHeadList(&AllocationListHead);
	  Request = CONTAINING_RECORD(Entry, MM_ALLOCATION_REQUEST, ListEntry);
	  KeReleaseSpinLock(&AllocationListLock, oldIrql);
	  Request->Page = Page;
	  KeSetEvent(&Request->Event, IO_NO_INCREMENT, FALSE);
	}
    }
  else
    {
      KeReleaseSpinLock(&AllocationListLock, oldIrql);
      MmDereferencePage(Page);
    }

  return(STATUS_SUCCESS);
}

VOID
MiTrimMemoryConsumer(ULONG Consumer)
{
  LONG Target;
  ULONG NrFreedPages;

  Target = MiMemoryConsumers[Consumer].PagesUsed - 
    MiMemoryConsumers[Consumer].PagesTarget;
  if (Target < 1)
    {
      Target = 1;
    }

  if (MiMemoryConsumers[Consumer].Trim != NULL)
    {
      MiMemoryConsumers[Consumer].Trim(Target, 0, &NrFreedPages);
    }
}

VOID
MmRebalanceMemoryConsumers(VOID)
{
  LONG Target;
  ULONG i;
  ULONG NrFreedPages;
  NTSTATUS Status;

  Target = (MiMinimumAvailablePages - MiNrAvailablePages) + MiPagesRequired;
  Target = max(Target, (LONG) MiMinimumPagesPerRun);

  for (i = 0; i < MC_MAXIMUM && Target > 0; i++)
    {
      if (MiMemoryConsumers[i].Trim != NULL)
	{
	  Status = MiMemoryConsumers[i].Trim(Target, 0, &NrFreedPages);
	  if (!NT_SUCCESS(Status))
	    {
	      KEBUGCHECK(0);
	    }
	  Target = Target - NrFreedPages;
	}
    }
}

NTSTATUS
MmRequestPageMemoryConsumer(ULONG Consumer, BOOLEAN CanWait, 
			    PHYSICAL_ADDRESS* AllocatedPage)
{
  ULONG OldUsed;
  ULONG OldAvailable;
  PHYSICAL_ADDRESS Page;
  KIRQL oldIrql;
  
  /*
   * Make sure we don't exceed our individual target.
   */
  OldUsed = InterlockedIncrement((LONG *)&MiMemoryConsumers[Consumer].PagesUsed);
  if (OldUsed >= (MiMemoryConsumers[Consumer].PagesTarget - 1) &&
      !MiIsPagerThread())
    {
      if (!CanWait)
	{
	  InterlockedDecrement((LONG *)&MiMemoryConsumers[Consumer].PagesUsed);
	  return(STATUS_NO_MEMORY);
	}
      MiTrimMemoryConsumer(Consumer);
    }

  /*
   * Make sure we don't exceed global targets.
   */
  OldAvailable = InterlockedDecrement((LONG *)&MiNrAvailablePages);
  if (OldAvailable < MiMinimumAvailablePages)
    {
      MM_ALLOCATION_REQUEST Request;

      if (!CanWait)
	{
	  InterlockedIncrement((LONG *)&MiNrAvailablePages);
	  InterlockedDecrement((LONG *)&MiMemoryConsumers[Consumer].PagesUsed);
	  return(STATUS_NO_MEMORY);
	}

      /* Insert an allocation request. */
      Request.Page.QuadPart = 0LL;
      KeInitializeEvent(&Request.Event, NotificationEvent, FALSE);
      InterlockedIncrement((LONG *)&MiPagesRequired);

      KeAcquireSpinLock(&AllocationListLock, &oldIrql);     
      /* Always let the pager thread itself allocate memory. */
      if (MiIsPagerThread())
	{
	  Page = MmAllocPage(Consumer, 0);
	  KeReleaseSpinLock(&AllocationListLock, oldIrql);
	  if (Page.QuadPart == 0LL)
	    {
	      KEBUGCHECK(0);
	    }
	  *AllocatedPage = Page;
	  InterlockedDecrement((LONG *)&MiPagesRequired);
	  return(STATUS_SUCCESS);
	}
      /* Otherwise start the pager thread if it isn't already working. */
      MiStartPagerThread();
      InsertTailList(&AllocationListHead, &Request.ListEntry);
      KeReleaseSpinLock(&AllocationListLock, oldIrql);

      KeWaitForSingleObject(&Request.Event,
			    0,
			    KernelMode,
			    FALSE,
			    NULL);
      
      Page = Request.Page;
      if (Page.QuadPart == 0LL)
	{
	  KEBUGCHECK(0);
	}
      MmTransferOwnershipPage(Page, Consumer);
      *AllocatedPage = Page;
      InterlockedDecrement((LONG *)&MiPagesRequired);
      MiStopPagerThread();
      return(STATUS_SUCCESS);
    }
  
  /*
   * Actually allocate the page.
   */
  Page = MmAllocPage(Consumer, 0);
  if (Page.QuadPart == 0LL)
    {
      KEBUGCHECK(0);
    }
  *AllocatedPage = Page;

  return(STATUS_SUCCESS);
}

/* EOF */
