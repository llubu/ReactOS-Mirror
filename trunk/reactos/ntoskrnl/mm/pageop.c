/* $Id: pageop.c,v 1.22 2004/12/24 17:06:59 navaraf Exp $
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/pageop.c
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY: 
 *               27/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define PAGEOP_HASH_TABLE_SIZE       (32)

static KSPIN_LOCK MmPageOpHashTableLock;
static PMM_PAGEOP MmPageOpHashTable[PAGEOP_HASH_TABLE_SIZE];
static NPAGED_LOOKASIDE_LIST MmPageOpLookasideList;

#define TAG_MM_PAGEOP   TAG('M', 'P', 'O', 'P')

/* FUNCTIONS *****************************************************************/

VOID
MmReleasePageOp(PMM_PAGEOP PageOp)
/*
 * FUNCTION: Release a reference to a page operation descriptor
 */
{
   KIRQL oldIrql;
   PMM_PAGEOP PrevPageOp;

   KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);
   PageOp->ReferenceCount--;
   if (PageOp->ReferenceCount > 0)
   {
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return;
   }
   InterlockedDecrementUL(&PageOp->MArea->PageOpCount);
   PrevPageOp = MmPageOpHashTable[PageOp->Hash];
   if (PrevPageOp == PageOp)
   {
      MmPageOpHashTable[PageOp->Hash] = PageOp->Next;
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      ExFreeToNPagedLookasideList(&MmPageOpLookasideList, PageOp);
      return;
   }
   while (PrevPageOp->Next != NULL)
   {
      if (PrevPageOp->Next == PageOp)
      {
         PrevPageOp->Next = PageOp->Next;
         KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
         ExFreeToNPagedLookasideList(&MmPageOpLookasideList, PageOp);
         return;
      }
      PrevPageOp = PrevPageOp->Next;
   }
   KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
   KEBUGCHECK(0);
}

PMM_PAGEOP
MmCheckForPageOp(PMEMORY_AREA MArea, ULONG Pid, PVOID Address,
                 PMM_SECTION_SEGMENT Segment, ULONG Offset)
{
   ULONG Hash;
   KIRQL oldIrql;
   PMM_PAGEOP PageOp;

   /*
    * Calcuate the hash value for pageop structure
    */
   if (MArea->Type == MEMORY_AREA_SECTION_VIEW)
   {
      Hash = (((ULONG)Segment) | (((ULONG)Offset) / PAGE_SIZE));
   }
   else
   {
      Hash = (((ULONG)Pid) | (((ULONG)Address) / PAGE_SIZE));
   }
   Hash = Hash % PAGEOP_HASH_TABLE_SIZE;

   KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);

   /*
    * Check for an existing pageop structure
    */
   PageOp = MmPageOpHashTable[Hash];
   while (PageOp != NULL)
   {
      if (MArea->Type == MEMORY_AREA_SECTION_VIEW)
      {
         if (PageOp->Segment == Segment &&
               PageOp->Offset == Offset)
         {
            break;
         }
      }
      else
      {
         if (PageOp->Pid == Pid &&
               PageOp->Address == Address)
         {
            break;
         }
      }
      PageOp = PageOp->Next;
   }

   /*
    * If we found an existing pageop then increment the reference count
    * and return it.
    */
   if (PageOp != NULL)
   {
      PageOp->ReferenceCount++;
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return(PageOp);
   }
   KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
   return(NULL);
}

PMM_PAGEOP
MmGetPageOp(PMEMORY_AREA MArea, ULONG Pid, PVOID Address,
            PMM_SECTION_SEGMENT Segment, ULONG Offset, ULONG OpType, BOOL First)
/*
 * FUNCTION: Get a page operation descriptor corresponding to
 * the memory area and either the segment, offset pair or the
 * pid, address pair.      
 */
{
   ULONG Hash;
   KIRQL oldIrql;
   PMM_PAGEOP PageOp;

   /*
    * Calcuate the hash value for pageop structure
    */
   if (MArea->Type == MEMORY_AREA_SECTION_VIEW)
   {
      Hash = (((ULONG)Segment) | (((ULONG)Offset) / PAGE_SIZE));
   }
   else
   {
      Hash = (((ULONG)Pid) | (((ULONG)Address) / PAGE_SIZE));
   }
   Hash = Hash % PAGEOP_HASH_TABLE_SIZE;

   KeAcquireSpinLock(&MmPageOpHashTableLock, &oldIrql);

   /*
    * Check for an existing pageop structure
    */
   PageOp = MmPageOpHashTable[Hash];
   while (PageOp != NULL)
   {
      if (MArea->Type == MEMORY_AREA_SECTION_VIEW)
      {
         if (PageOp->Segment == Segment &&
               PageOp->Offset == Offset)
         {
            break;
         }
      }
      else
      {
         if (PageOp->Pid == Pid &&
               PageOp->Address == Address)
         {
            break;
         }
      }
      PageOp = PageOp->Next;
   }

   /*
    * If we found an existing pageop then increment the reference count
    * and return it.
    */
   if (PageOp != NULL)
   {
      if (First)
      {
         PageOp = NULL;
      }
      else
      {
         PageOp->ReferenceCount++;
      }
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      return(PageOp);
   }

   /*
    * Otherwise add a new pageop.
    */
   PageOp = ExAllocateFromNPagedLookasideList(&MmPageOpLookasideList);
   if (PageOp == NULL)
   {
      KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
      KEBUGCHECK(0);
      return(NULL);
   }

   if (MArea->Type != MEMORY_AREA_SECTION_VIEW)
   {
      PageOp->Pid = Pid;
      PageOp->Address = Address;
   }
   else
   {
      PageOp->Segment = Segment;
      PageOp->Offset = Offset;
   }
   PageOp->ReferenceCount = 1;
   PageOp->Next = MmPageOpHashTable[Hash];
   PageOp->Hash = Hash;
   PageOp->Thread = PsGetCurrentThread();
   PageOp->Abandoned = FALSE;
   PageOp->Status = STATUS_PENDING;
   PageOp->OpType = OpType;
   PageOp->MArea = MArea;
   KeInitializeEvent(&PageOp->CompletionEvent, NotificationEvent, FALSE);
   MmPageOpHashTable[Hash] = PageOp;
   InterlockedIncrementUL(&MArea->PageOpCount);

   KeReleaseSpinLock(&MmPageOpHashTableLock, oldIrql);
   return(PageOp);
}

VOID INIT_FUNCTION
MmInitializePageOp(VOID)
{
   memset(MmPageOpHashTable, 0, sizeof(MmPageOpHashTable));
   KeInitializeSpinLock(&MmPageOpHashTableLock);

   ExInitializeNPagedLookasideList (&MmPageOpLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(MM_PAGEOP),
                                    TAG_MM_PAGEOP,
                                    50);
}







