/* $Id: kmap.c,v 1.33 2004/08/01 07:24:58 hbirr Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/kmap.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/pool.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define ALLOC_MAP_SIZE (MM_KERNEL_MAP_SIZE / PAGE_SIZE)

/*
 * One bit for each page in the kmalloc region
 *      If set then the page is used by a kmalloc block
 */
static UCHAR AllocMapBuffer[ROUND_UP(ALLOC_MAP_SIZE, 32) / 8];
static RTL_BITMAP AllocMap;
static KSPIN_LOCK AllocMapLock;
static ULONG AllocMapHint = 0;

extern PVOID MiKernelMapStart;
extern ULONG MiKernelMapLength;

/* FUNCTIONS ***************************************************************/

VOID
ExUnmapPage(PVOID Addr)
{
   KIRQL oldIrql;
   ULONG Base = ((char*)Addr - (char*)MiKernelMapStart) / PAGE_SIZE;

   DPRINT("ExUnmapPage(Addr %x)\n",Addr);

   MmDeleteVirtualMapping(NULL, (PVOID)Addr, FALSE, NULL, NULL);
   KeAcquireSpinLock(&AllocMapLock, &oldIrql);
   RtlClearBits(&AllocMap, Base, 1);
   AllocMapHint = min(AllocMapHint, Base);
   KeReleaseSpinLock(&AllocMapLock, oldIrql);
}

PVOID
ExAllocatePage(VOID)
{
   PFN_TYPE Page;
   NTSTATUS Status;

   Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Page);
   if (!NT_SUCCESS(Status))
   {
      return(NULL);
   }

   return(ExAllocatePageWithPhysPage(Page));
}

NTSTATUS
MiZeroPage(PFN_TYPE Page)
{
   PVOID TempAddress;

   TempAddress = ExAllocatePageWithPhysPage(Page);
   if (TempAddress == NULL)
   {
      return(STATUS_NO_MEMORY);
   }
   memset(TempAddress, 0, PAGE_SIZE);
   ExUnmapPage(TempAddress);
   return(STATUS_SUCCESS);
}

NTSTATUS
MiCopyFromUserPage(PFN_TYPE DestPage, PVOID SourceAddress)
{
   PVOID TempAddress;

   TempAddress = ExAllocatePageWithPhysPage(DestPage);
   if (TempAddress == NULL)
   {
      return(STATUS_NO_MEMORY);
   }
   memcpy(TempAddress, SourceAddress, PAGE_SIZE);
   ExUnmapPage(TempAddress);
   return(STATUS_SUCCESS);
}

PVOID
ExAllocatePageWithPhysPage(PFN_TYPE Page)
{
   KIRQL oldlvl;
   PVOID Addr;
   ULONG Base;
   NTSTATUS Status;

   KeAcquireSpinLock(&AllocMapLock, &oldlvl);
   Base = RtlFindClearBitsAndSet(&AllocMap, 1, AllocMapHint);
   if (Base != 0xFFFFFFFF)
   {
      AllocMapHint = Base + 1;
      KeReleaseSpinLock(&AllocMapLock, oldlvl);
      Addr = (char*)MiKernelMapStart + Base * PAGE_SIZE;
      Status = MmCreateVirtualMapping(NULL,
                                      Addr,
                                      PAGE_READWRITE | PAGE_SYSTEM,
                                      &Page,
                                      1);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("Unable to create virtual mapping\n");
         KEBUGCHECK(0);
      }
      return Addr;
   }
   KeReleaseSpinLock(&AllocMapLock, oldlvl);
   return NULL;
}

VOID INIT_FUNCTION
MiInitKernelMap(VOID)
{
   KeInitializeSpinLock(&AllocMapLock);
   RtlInitializeBitMap(&AllocMap, (PULONG)AllocMapBuffer, ALLOC_MAP_SIZE);
   RtlClearAllBits(&AllocMap);
}

VOID
MiFreeNonPagedPoolRegion(PVOID Addr, ULONG Count, BOOLEAN Free)
{
   ULONG i;
   ULONG Base = ((char*)Addr - (char*)MiKernelMapStart) / PAGE_SIZE;
   KIRQL oldlvl;

   for (i = 0; i < Count; i++)
   {
      MmDeleteVirtualMapping(NULL,
                             (char*)Addr + (i * PAGE_SIZE),
                             Free,
                             NULL,
                             NULL);
   }
   KeAcquireSpinLock(&AllocMapLock, &oldlvl);
   RtlClearBits(&AllocMap, Base, Count);
   AllocMapHint = min(AllocMapHint, Base);
   KeReleaseSpinLock(&AllocMapLock, oldlvl);
}

PVOID
MiAllocNonPagedPoolRegion(ULONG nr_pages)
/*
 * FUNCTION: Allocates a region of pages within the nonpaged pool area
 */
{
   ULONG Base;
   KIRQL oldlvl;

   KeAcquireSpinLock(&AllocMapLock, &oldlvl);
   Base = RtlFindClearBitsAndSet(&AllocMap, nr_pages, AllocMapHint);
   if (Base == 0xFFFFFFFF)
   {
      DbgPrint("CRITICAL: Out of non-paged pool space\n");
      KEBUGCHECK(0);
   }
   if (AllocMapHint == Base)
   {
      AllocMapHint += nr_pages;
   }
   KeReleaseSpinLock(&AllocMapLock, oldlvl);
   //DPRINT("returning %x\n",NonPagedPoolBase + Base * PAGE_SIZE);
   return (char*)MiKernelMapStart + Base * PAGE_SIZE;
}


