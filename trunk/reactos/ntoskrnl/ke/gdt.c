/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/gdt.c
 * PURPOSE:         GDT managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define NR_TASKS 128

USHORT KiGdt[(6 + NR_TASKS) * 4] = {0x0, 0x0, 0x0, 0x0,
                                   0x0, 0x0, 0xfa00, 0xcc,
                                   0x0, 0x0, 0xf200, 0xcc,
                                   0xffff, 0x0, 0x9200, 0xcf,
                                   0xffff, 0x0, 0x9a00, 0xcf,
                                   0xffff, 0x0, 0x9200, 0xcf};
static KSPIN_LOCK GdtLock;

/* FUNCTIONS *****************************************************************/

VOID KeDumpGdtSelector(ULONG Entry)
{
}

VOID KeFreeGdtSelector(ULONG Entry)
/*
 * FUNCTION: Free a gdt selector
 * ARGUMENTS:
 *       Entry = Entry to free
 */
{
   KIRQL oldIrql;
   
   DPRINT("KeFreeGdtSelector(Entry %d)\n",Entry);
   
   if (Entry > (6 + NR_TASKS))
     {
	DPRINT1("Entry too large\n");
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&GdtLock, &oldIrql);
   KiGdt[Entry*4] = 0;
   KiGdt[Entry*4 + 2] = 0;
   KeReleaseSpinLock(&GdtLock, oldIrql);
}

ULONG KeAllocateGdtSelector(ULONG Desc[2])
/*
 * FUNCTION: Allocate a gdt selector
 * ARGUMENTS:
 *      Desc = Contents for descriptor
 * RETURNS: The index of the entry allocated
 */
{
   ULONG i;
   KIRQL oldIrql;
   
   DPRINT("KeAllocateGdtSelector(Desc[0] %x, Desc[1] %x)\n",
	  Desc[0], Desc[1]);
   
   KeAcquireSpinLock(&GdtLock, &oldIrql);   
   for (i=6; i<(6 + NR_TASKS); i++)
     {
	if (KiGdt[i*4] == 0 &&
	    KiGdt[i*4 + 2] == 0)
	  {
	     ((PULONG)KiGdt)[i*2] = Desc[0];
	     ((PULONG)KiGdt)[i*2 + 1] = Desc[1];
	     KeReleaseSpinLock(&GdtLock, oldIrql);
	     DPRINT("KeAllocateGdtSelector() = %x\n",i);
	     return(i);
	  }
     }
   KeReleaseSpinLock(&GdtLock, oldIrql);
   return(0);
}
