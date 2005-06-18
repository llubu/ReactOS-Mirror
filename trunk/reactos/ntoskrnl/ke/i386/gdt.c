/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/gdt.c
 * PURPOSE:         GDT managment
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

PUSHORT KiGdtArray[MAXIMUM_PROCESSORS];

USHORT KiBootGdt[11 * 4] =
{
 0x0, 0x0, 0x0, 0x0,              /* Null */
 0xffff, 0x0, 0x9a00, 0xcf,       /* Kernel CS */
 0xffff, 0x0, 0x9200, 0xcf,       /* Kernel DS */
 0xffff, 0x0, 0xfa00, 0xcf,       /* User CS */
 0xffff, 0x0, 0xf200, 0xcf,       /* User DS */
 0x0, 0x0, 0x0, 0x0,              /* TSS */
 0x0fff, 0x0000, 0x9200, 0xff00,  /* PCR */
 0x0fff, 0x0, 0xf200, 0x0,        /* TEB */
 0x0, 0x0, 0x0, 0x0,              /* Reserved */
 0x0, 0x0, 0x0, 0x0,              /* LDT */
 0x0, 0x0, 0x0, 0x0               /* Trap TSS */
};


#include <pshpack1.h>

struct LocalGdtDescriptor_t
{
  USHORT Length;
  ULONG Base;
} KiGdtDescriptor = { 11 * 8, (ULONG)KiBootGdt };

#include <poppack.h>


static KSPIN_LOCK GdtLock;

/* FUNCTIONS *****************************************************************/

VOID
KiGdtPrepareForApplicationProcessorInit(ULONG Id)
{
  KiGdtArray[Id] = ExAllocatePool(NonPagedPool, sizeof(USHORT) * 4 * 11);
}

VOID
KiInitializeGdt(PKPCR Pcr)
{
  PUSHORT Gdt;
  struct LocalGdtDescriptor_t Descriptor;
  ULONG Entry;
  ULONG Base;

  if (Pcr == NULL)
    {
      KiGdtArray[0] = KiBootGdt;
      return;
    }

  /*
   * Allocate a GDT
   */
  Gdt = KiGdtArray[Pcr->Number];
  if (Gdt == NULL)
    {
      DbgPrint("No GDT (%d)\n", Pcr->Number);
      KEBUGCHECK(0);
    }

  /*
   * Copy the boot processor's GDT onto this processor's GDT. Note that
   * the only entries that can change are the PCR, TEB and LDT descriptors.
   * We will be initializing these later so their current values are
   * irrelevant.
   */
  memcpy(Gdt, KiBootGdt, sizeof(USHORT) * 4 * 11);
  Pcr->GDT = Gdt;

  /*
   * Set the base address of the PCR
   */
  Base = (ULONG)Pcr;
  Entry = PCR_SELECTOR / 2;
  Gdt[Entry + 1] = (USHORT)(((ULONG)Base) & 0xffff);

  Gdt[Entry + 2] = Gdt[Entry + 2] & ~(0xff);
  Gdt[Entry + 2] = (USHORT)(Gdt[Entry + 2] | ((((ULONG)Base) & 0xff0000) >> 16));

  Gdt[Entry + 3] = Gdt[Entry + 3] & ~(0xff00);
  Gdt[Entry + 3] = (USHORT)(Gdt[Entry + 3] | ((((ULONG)Base) & 0xff000000) >> 16));

  /*
   * Load the GDT
   */
  Descriptor.Length = 8 * 11;
  Descriptor.Base = (ULONG)Gdt;
#if defined(__GNUC__)
  __asm__ ("lgdt %0\n\t" : /* no output */ : "m" (Descriptor));

  /*
   * Reload the selectors
   */
  __asm__ ("movl %0, %%ds\n\t"
	   "movl %0, %%es\n\t"
	   "movl %1, %%fs\n\t"
	   "movl %0, %%gs\n\t"
	   : /* no output */
	   : "a" (KERNEL_DS), "d" (PCR_SELECTOR));
  __asm__ ("pushl %0\n\t"
	   "pushl $.l4\n\t"
	   "lret\n\t"
	   ".l4:\n\t"
	   : /* no output */
	   : "a" (KERNEL_CS));
#elif defined(_MSC_VER)
  __asm
  {
    lgdt Descriptor;
    mov ax, KERNEL_DS;
    mov dx, PCR_SELECTOR;
    mov ds, ax;
    mov es, ax;
    mov fs, dx;
    mov gs, ax;
    push KERNEL_CS;
    push offset l4 ;
    retf
l4:
  }
#else
#error Unknown compiler for inline assembler
#endif
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
KeI386FlatToGdtSelector(
	IN ULONG	Base,
	IN USHORT	Length,
	IN USHORT	Selector
)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
KeI386ReleaseGdtSelectors(
	OUT PULONG SelArray,
	IN ULONG NumOfSelectors
)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
KeI386AllocateGdtSelectors(
	OUT PULONG SelArray,
	IN ULONG NumOfSelectors
)
{
	UNIMPLEMENTED;
	return 0;
}

VOID
KeSetBaseGdtSelector(ULONG Entry,
		     PVOID Base)
{
   KIRQL oldIrql;
   PUSHORT Gdt;

   DPRINT("KeSetBaseGdtSelector(Entry %x, Base %x)\n",
	   Entry, Base);

   KeAcquireSpinLock(&GdtLock, &oldIrql);

   Gdt = KeGetCurrentKPCR()->GDT;
   Entry = (Entry & (~0x3)) / 2;

   Gdt[Entry + 1] = (USHORT)(((ULONG)Base) & 0xffff);

   Gdt[Entry + 2] = Gdt[Entry + 2] & ~(0xff);
   Gdt[Entry + 2] = (USHORT)(Gdt[Entry + 2] |
     ((((ULONG)Base) & 0xff0000) >> 16));

   Gdt[Entry + 3] = Gdt[Entry + 3] & ~(0xff00);
   Gdt[Entry + 3] = (USHORT)(Gdt[Entry + 3] |
     ((((ULONG)Base) & 0xff000000) >> 16));

   DPRINT("%x %x %x %x\n",
	   Gdt[Entry + 0],
	   Gdt[Entry + 1],
	   Gdt[Entry + 2],
	   Gdt[Entry + 3]);

   KeReleaseSpinLock(&GdtLock, oldIrql);
}

VOID
KeSetGdtSelector(ULONG Entry,
                 ULONG Value1,
                 ULONG Value2)
{
   KIRQL oldIrql;
   PULONG Gdt;

   DPRINT("KeSetGdtSelector(Entry %x, Value1 %x, Value2 %x)\n",
	   Entry, Value1, Value2);

   KeAcquireSpinLock(&GdtLock, &oldIrql);

   Gdt = (PULONG) KeGetCurrentKPCR()->GDT;
   Entry = (Entry & (~0x3)) / 4;

   Gdt[Entry] = Value1;
   Gdt[Entry + 1] = Value2;

   DPRINT("%x %x\n",
	   Gdt[Entry + 0],
	   Gdt[Entry + 1]);

   KeReleaseSpinLock(&GdtLock, oldIrql);
}

VOID
KeDumpGdtSelector(ULONG Entry)
{
   USHORT a, b, c, d;
   ULONG RawLimit;

   a = KiBootGdt[Entry*4];
   b = KiBootGdt[Entry*4 + 1];
   c = KiBootGdt[Entry*4 + 2];
   d = KiBootGdt[Entry*4 + 3];

   DbgPrint("Base: %x\n", b + ((c & 0xff) * (1 << 16)) +
	    ((d & 0xff00) * (1 << 16)));
   RawLimit = a + ((d & 0xf) * (1 << 16));
   if (d & 0x80)
     {
	DbgPrint("Limit: %x\n", RawLimit * 4096);
     }
   else
     {
	DbgPrint("Limit: %x\n", RawLimit);
     }
   DbgPrint("Accessed: %d\n", (c & 0x100) >> 8);
   DbgPrint("Type: %x\n", (c & 0xe00) >> 9);
   DbgPrint("System: %d\n", (c & 0x1000) >> 12);
   DbgPrint("DPL: %d\n", (c & 0x6000) >> 13);
   DbgPrint("Present: %d\n", (c & 0x8000) >> 15);
   DbgPrint("AVL: %x\n", (d & 0x10) >> 4);
   DbgPrint("D: %d\n", (d & 0x40) >> 6);
   DbgPrint("G: %d\n", (d & 0x80) >> 7);
}

/* EOF */
