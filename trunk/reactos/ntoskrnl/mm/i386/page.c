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
/* $Id: page.c,v 1.74 2004/09/26 16:32:17 hbirr Exp $
 *
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/i386/page.c
 * PURPOSE:     low level memory managment manipulation
 * PROGRAMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              9/3/98: Created
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define PA_BIT_PRESENT   (0)
#define PA_BIT_READWRITE (1)
#define PA_BIT_USER      (2)
#define PA_BIT_WT        (3)
#define PA_BIT_CD        (4)
#define PA_BIT_ACCESSED  (5)
#define PA_BIT_DIRTY     (6)
#define PA_BIT_GLOBAL	 (8)

#define PA_PRESENT   (1 << PA_BIT_PRESENT)
#define PA_READWRITE (1 << PA_BIT_READWRITE)
#define PA_USER      (1 << PA_BIT_USER)
#define PA_DIRTY     (1 << PA_BIT_DIRTY)
#define PA_WT        (1 << PA_BIT_WT)
#define PA_CD        (1 << PA_BIT_CD)
#define PA_ACCESSED  (1 << PA_BIT_ACCESSED)
#define PA_GLOBAL    (1 << PA_BIT_GLOBAL)

#define PAGETABLE_MAP     (0xf0000000)
#define PAGEDIRECTORY_MAP (0xf0000000 + (PAGETABLE_MAP / (1024)))

#define PAE_PAGEDIRECTORY_MAP	(0xf0000000 + (PAGETABLE_MAP / (512)))

ULONG MmGlobalKernelPageDirectory[1024];
ULONGLONG MmGlobalKernelPageDirectoryForPAE[512];

#define PTE_TO_PFN(X)  ((X) >> PAGE_SHIFT)
#define PFN_TO_PTE(X)  ((X) << PAGE_SHIFT)	

#define PAE_PTE_TO_PFN(X)   (PAE_PAGE_MASK(X) >> PAGE_SHIFT)
#define PAE_PFN_TO_PTE(X)   ((X) << PAGE_SHIFT)

#if defined(__GNUC__)
#define PTE_TO_PAGE(X) ((LARGE_INTEGER)(LONGLONG)(PAGE_MASK(X)))
#else
__inline LARGE_INTEGER PTE_TO_PAGE(ULONG npage)
{
   LARGE_INTEGER dummy;
   dummy.QuadPart = (LONGLONG)(PAGE_MASK(npage));
   return dummy;
}
#endif

extern ULONG Ke386CpuidFlags;
extern ULONG Ke386Cpuid;
extern BOOLEAN Ke386Pae;
extern BOOLEAN Ke386NoExecute;

/* FUNCTIONS ***************************************************************/

PULONG
MmGetPageDirectory(VOID)
{
   unsigned int page_dir=0;
   Ke386GetPageTableDirectory(page_dir);
   return((PULONG)page_dir);
}

static ULONG
ProtectToPTE(ULONG flProtect)
{
   ULONG Attributes = 0;

   if (flProtect & (PAGE_NOACCESS|PAGE_GUARD))
   {
      Attributes = 0;
   }
   else if (flProtect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE))
   {
      Attributes = PA_PRESENT | PA_READWRITE;
   }
   else if (flProtect & (PAGE_READONLY|PAGE_EXECUTE|PAGE_EXECUTE_READ))
   {
      Attributes = PA_PRESENT;
   }
   else
   {
      DPRINT1("Unknown main protection type.\n");
      KEBUGCHECK(0);
   }
   if (Ke386NoExecute && 
       !(flProtect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE)))
   {
      Attributes = Attributes | 0x80000000;
   }

   if (flProtect & PAGE_SYSTEM)
   {
   }
   else
   {
      Attributes = Attributes | PA_USER;
   }
   if (flProtect & PAGE_NOCACHE)
   {
      Attributes = Attributes | PA_CD;
   }
   if (flProtect & PAGE_WRITETHROUGH)
   {
      Attributes = Attributes | PA_WT;
   }
   return(Attributes);
}

#define ADDR_TO_PAGE_TABLE(v) (((ULONG)(v)) / (1024 * PAGE_SIZE))

#define ADDR_TO_PDE(v) (PULONG)(PAGEDIRECTORY_MAP + \
                                ((((ULONG)(v)) / (1024 * 1024))&(~0x3)))
#define ADDR_TO_PTE(v) (PULONG)(PAGETABLE_MAP + ((((ULONG)(v) / 1024))&(~0x3)))

#define ADDR_TO_PDE_OFFSET(v) ((((ULONG)(v)) / (1024 * PAGE_SIZE)))

#define ADDR_TO_PTE_OFFSET(v)  ((((ULONG)(v)) % (1024 * PAGE_SIZE)) / PAGE_SIZE) 


#define PAE_ADDR_TO_PAGE_TABLE(v)   (((ULONG)(v)) / (512 * PAGE_SIZE))

#define PAE_ADDR_TO_PDE(v)	    (PULONGLONG) (PAE_PAGEDIRECTORY_MAP + \
                                                  ((((ULONG_PTR)(v)) / (512 * 512))&(~0x7)))
#define PAE_ADDR_TO_PTE(v)	    (PULONGLONG) (PAGETABLE_MAP + ((((ULONG_PTR)(v) / 512))&(~0x7)))


#define PAE_ADDR_TO_PDTE_OFFSET(v)  (((ULONG_PTR)(v)) / (512 * 512 * PAGE_SIZE))

#define PAE_ADDR_TO_PDE_OFFSET(v)   ((((ULONG_PTR)(v)) % (512 * 512 * PAGE_SIZE)) / (512 * PAGE_SIZE))

#define PAE_ADDR_TO_PTE_OFFSET(v)   ((((ULONG_PTR)(v)) % (512 * PAGE_SIZE)) / PAGE_SIZE)


NTSTATUS Mmi386ReleaseMmInfo(PEPROCESS Process)
{
   PUSHORT LdtDescriptor;
   ULONG LdtBase;
   ULONG i, j;

   DPRINT("Mmi386ReleaseMmInfo(Process %x)\n",Process);

   LdtDescriptor = (PUSHORT) &Process->Pcb.LdtDescriptor[0];
   LdtBase = LdtDescriptor[1] |
             ((LdtDescriptor[2] & 0xff) << 16) |
             ((LdtDescriptor[3] & ~0xff) << 16);

   DPRINT("LdtBase: %x\n", LdtBase);

   if (LdtBase)
   {
      ExFreePool((PVOID) LdtBase);
   }

   if (Ke386Pae)
   {
      PULONGLONG PageDirTable;
      PULONGLONG PageDir;
      PULONGLONG Pde;
      ULONG k;

      PageDirTable = (PULONGLONG)ExAllocatePageWithPhysPage(Process->Pcb.DirectoryTableBase.QuadPart >> PAGE_SHIFT);
      for (i = 0; i < 4; i++)
      {
         if (i < 3)
	 {
            PageDir = (PULONGLONG)ExAllocatePageWithPhysPage(PageDirTable[i] >> PAGE_SHIFT);
	    for (j = 0; j < 512; j++)
	    {
	       if (PageDir[j] != 0LL)
	       {
                  DPRINT1("ProcessId %d, Pde for %08x - %08x is not freed, RefCount %d\n", 
		          Process->UniqueProcessId,
	                  (i * 512 + j) * 512 * PAGE_SIZE, (i * 512 + j + 1) * 512 * PAGE_SIZE - 1, 
		          Process->AddressSpace.PageTableRefCountTable[i*512 + j]);
                  Pde = ExAllocatePageWithPhysPage(PageDir[j] >> PAGE_SHIFT);
	          for (k = 0; k < 512; k++)
	          {
	             if(Pde[k] != 0)
	             {
	                if (Pde[k] & PA_PRESENT)
	                {
	                   DPRINT1("Page at %08x is not freed\n",
		                   (i * 512 + j) * 512 * PAGE_SIZE + k * PAGE_SIZE);
	                }
	                else
	                {
	                   DPRINT1("Swapentry %x at %x is not freed\n",
		                   (i * 512 + j) * 512 * PAGE_SIZE + k * PAGE_SIZE);
	                }
		     }
		  }
		  ExUnmapPage(Pde);
	    	  MmReleasePageMemoryConsumer(MC_NPPOOL, PAE_PTE_TO_PFN(PageDir[j]));
	       }
	    }
	    ExUnmapPage(PageDir);
	 }
         MmReleasePageMemoryConsumer(MC_NPPOOL, PageDirTable[i] >> PAGE_SHIFT);
      }
      ExUnmapPage((PVOID)PageDirTable);
   }
   else
   {
      PULONG Pde;
      PULONG PageDir;
      PageDir = ExAllocatePageWithPhysPage(Process->Pcb.DirectoryTableBase.QuadPart >> PAGE_SHIFT);
      for (i = 0; i < ADDR_TO_PDE_OFFSET(KERNEL_BASE); i++)
      {
         if (PageDir[i] != 0)
         {
            DPRINT1("Pde for %08x - %08x is not freed, RefCount %d\n", 
	            i * 4 * 1024 * 1024, (i + 1) * 4 * 1024 * 1024 - 1, 
		    Process->AddressSpace.PageTableRefCountTable[i]);
	    Pde = ExAllocatePageWithPhysPage(PageDir[i] >> PAGE_SHIFT);
	    for (j = 0; j < 1024; j++)
	    {
	       if(Pde[j] != 0)
	       {
	          if (Pde[j] & PA_PRESENT)
	          {
	             DPRINT1("Page at %08x is not freed\n",
		             i * 4 * 1024 * 1024 + j * PAGE_SIZE);
	          }
	          else
	          {
	             DPRINT1("Swapentry %x at %x is not freed\n",
		             Pde[j], i * 4 * 1024 * 1024 + j * PAGE_SIZE);
	          }
	       }
	    }
	    ExUnmapPage(Pde);
	    MmReleasePageMemoryConsumer(MC_NPPOOL, PTE_TO_PFN(PageDir[i]));
	 }
      }
      ExUnmapPage(PageDir);
   }

   MmReleasePageMemoryConsumer(MC_NPPOOL, Process->Pcb.DirectoryTableBase.QuadPart >> PAGE_SHIFT);
#if defined(__GNUC__)

   Process->Pcb.DirectoryTableBase.QuadPart = 0LL;
#else

   Process->Pcb.DirectoryTableBase.QuadPart = 0;
#endif

   DPRINT("Finished Mmi386ReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

NTSTATUS MmCopyMmInfo(PEPROCESS Src, PEPROCESS Dest)
{
   PHYSICAL_ADDRESS PhysPageDirectory;
   PKPROCESS KProcess = &Dest->Pcb;

   DPRINT("MmCopyMmInfo(Src %x, Dest %x)\n", Src, Dest);

   if (Ke386Pae)
   {
      PULONGLONG PageDirTable;
      PULONGLONG PageDir;
      PFN_TYPE Pfn[4];
      ULONG i, j;
      NTSTATUS Status;

      PageDirTable = ExAllocatePage();
      if (PageDirTable == NULL)
      {
         return STATUS_UNSUCCESSFUL;
      }
      PhysPageDirectory = MmGetPhysicalAddress(PageDirTable);

      for (i = 0; i < 4; i++)
      {
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn[i]);
	 if (!NT_SUCCESS(Status))
	 {
	    for (j = 0; j < i; j++)
	    {
	       MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn[j]);
	    }
	    ExUnmapPage(PageDirTable);
	    MmReleasePageMemoryConsumer(MC_NPPOOL, PhysPageDirectory.QuadPart >> PAGE_SHIFT);
	    return Status;
	 }
	 PageDirTable[i] = (Pfn[i] << PAGE_SHIFT) | PA_PRESENT;
      }
      ExUnmapPage(PageDirTable);
      PageDir = (PULONGLONG)ExAllocatePageWithPhysPage(Pfn[3]);
      if (PageDir == NULL)
      {
	 for (i = 0; i < 4; i++)
	 {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn[i]);
	 }
	 ExUnmapPage(PageDirTable);
	 MmReleasePageMemoryConsumer(MC_NPPOOL, PhysPageDirectory.QuadPart >> PAGE_SHIFT);
	 return STATUS_UNSUCCESSFUL;
      }
      memcpy(PageDir, &MmGlobalKernelPageDirectoryForPAE[0], 512 * sizeof(ULONGLONG));
      for (i = 0; i < 4; i++)
      {
         PageDir[PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) + i] = (Pfn[i] << PAGE_SHIFT) | PA_PRESENT | PA_READWRITE;
      }
      ExUnmapPage(PageDir);
   }
   else
   {
      PULONG PageDirectory;
      PageDirectory = ExAllocatePage();
      if (PageDirectory == NULL)
      {
         return(STATUS_UNSUCCESSFUL);
      }
      PhysPageDirectory = MmGetPhysicalAddress(PageDirectory);

      memcpy(PageDirectory + ADDR_TO_PDE_OFFSET(KERNEL_BASE),
             MmGlobalKernelPageDirectory + ADDR_TO_PDE_OFFSET(KERNEL_BASE),
             (1024 - ADDR_TO_PDE_OFFSET(KERNEL_BASE)) * sizeof(ULONG));

      DPRINT("Addr %x\n",PAGETABLE_MAP / (4*1024*1024));
      PageDirectory[PAGETABLE_MAP / (4*1024*1024)] =
         PhysPageDirectory.u.LowPart | PA_PRESENT | PA_READWRITE;

      ExUnmapPage(PageDirectory);
   }
   KProcess->DirectoryTableBase = PhysPageDirectory;
   DPRINT("Finished MmCopyMmInfo()\n");
   return(STATUS_SUCCESS);
}

VOID MmDeletePageTable(PEPROCESS Process, PVOID Address)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }

   if (Ke386Pae)
   {
      ULONGLONG ZeroPde = 0LL;
      ExfpInterlockedExchange64(PAE_ADDR_TO_PDE(Address), &ZeroPde);
   }
   else
   {
      *(ADDR_TO_PDE(Address)) = 0;
   }
   if (Address >= (PVOID)KERNEL_BASE)
   {
      KEBUGCHECK(0);
      //       MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
   }
   FLUSH_TLB;
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

VOID MmFreePageTable(PEPROCESS Process, PVOID Address)
{
   PEPROCESS CurrentProcess = PsGetCurrentProcess();
   ULONG i;
   PFN_TYPE Pfn;

   DPRINT("ProcessId %d, Address %x\n", Process->UniqueProcessId, Address);
   if (Process != NULL && Process != CurrentProcess)
   {
      KeAttachProcess(Process);
   }
   if (Ke386Pae)
   {
      PULONGLONG PageTable;
      ULONGLONG ZeroPte = 0LL;
      PageTable = (PULONGLONG)PAGE_ROUND_DOWN((PVOID)PAE_ADDR_TO_PTE(Address));
      for (i = 0; i < 512; i++)
      {
         if (PageTable[i] != 0LL)
         {
            DbgPrint("Page table entry not clear at %x/%x (is %I64x)\n",
                     ((ULONG)Address / (4*1024*1024)), i, PageTable[i]);
            KEBUGCHECK(0);
         }
      }
      Pfn = PAE_PTE_TO_PFN(*(PAE_ADDR_TO_PDE(Address)));
      ExfpInterlockedExchange64(PAE_ADDR_TO_PDE(Address), &ZeroPte);
   }
   else
   {
      PULONG PageTable;
      PageTable = (PULONG)PAGE_ROUND_DOWN((PVOID)ADDR_TO_PTE(Address));
      for (i = 0; i < 1024; i++)
      {
         if (PageTable[i] != 0)
         {
            DbgPrint("Page table entry not clear at %x/%x (is %x)\n",
                     ((ULONG)Address / (4*1024*1024)), i, PageTable[i]);
            KEBUGCHECK(0);
         }
      }
      Pfn = PTE_TO_PFN(*(ADDR_TO_PDE(Address)));
      *(ADDR_TO_PDE(Address)) = 0;
   }
   FLUSH_TLB;

   if (Address >= (PVOID)KERNEL_BASE)
   {
      //    MmGlobalKernelPageDirectory[ADDR_TO_PDE_OFFSET(Address)] = 0;
      KEBUGCHECK(0);
   }
   else
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
   }
   if (Process != NULL && Process != CurrentProcess)
   {
      KeDetachProcess();
   }
}

static PULONGLONG
MmGetPageTableForProcessForPAE(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
   NTSTATUS Status;
   PFN_TYPE Pfn;
   ULONGLONG Entry;
   ULONGLONG ZeroEntry = 0LL;
   PULONGLONG Pt;
   PULONGLONG PageDir;
   PULONGLONG PageDirTable;

   DPRINT("MmGetPageTableForProcessForPAE(%x %x %d)\n",
          Process, Address, Create);
   if (Address >= (PVOID)PAGETABLE_MAP && Address < (PVOID)PAGETABLE_MAP + 0x800000)
   {
      KEBUGCHECK(0);
   }
   if (Address < (PVOID)KERNEL_BASE && Process && Process != PsGetCurrentProcess())
   {
      PageDirTable = ExAllocatePageWithPhysPage(Process->Pcb.DirectoryTableBase.QuadPart >> PAGE_SHIFT);
      if (PageDirTable == NULL)
      {
         KEBUGCHECK(0);
      }
      PageDir = ExAllocatePageWithPhysPage(PageDirTable[PAE_ADDR_TO_PDTE_OFFSET(Address)] >> PAGE_SHIFT);
      ExUnmapPage(PageDirTable);
      if (PageDir == NULL)
      {
         KEBUGCHECK(0);
      }
      PageDir += PAE_ADDR_TO_PDE_OFFSET(Address);
      Entry = ExfInterlockedCompareExchange64(PageDir, &ZeroEntry, &ZeroEntry);
      if (Entry == 0LL)
      {
         if (Create == FALSE)
	 {
	    ExUnmapPage(PageDir);
	    return NULL;
	 }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	 if (!NT_SUCCESS(Status))
	 {
	    KEBUGCHECK(0);
	 }
         Entry = PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER; 
	 Entry = ExfInterlockedCompareExchange64(PageDir, &Entry, &ZeroEntry);
	 if (Entry != 0LL)
	 {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	    Pfn = PAE_PTE_TO_PFN(Entry);
	 }
      }
      else
      {
         Pfn = PAE_PTE_TO_PFN(Entry);
      }
      ExUnmapPage(PageDir);
      Pt = ExAllocatePageWithPhysPage(Pfn);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      return Pt + PAE_ADDR_TO_PTE_OFFSET(Address);
   }
   PageDir = PAE_ADDR_TO_PDE(Address);
   if (0LL == ExfInterlockedCompareExchange64(PageDir, &ZeroEntry, &ZeroEntry))
   {
      if (Address >= (PVOID)KERNEL_BASE)
      {
	 if (MmGlobalKernelPageDirectoryForPAE[PAE_ADDR_TO_PDE_OFFSET(Address)] == 0LL)
	 {
	    if (Create == FALSE)
	    {
               return NULL;
	    }
            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	    if (!NT_SUCCESS(Status))
	    {
	       KEBUGCHECK(0);
	    }
	    Entry = PAE_PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE;
            if (Ke386CpuidFlags & X86_FEATURE_PGE)
	    {
	       Entry |= PA_GLOBAL;
	    }
	    if (0LL != ExfInterlockedCompareExchange64(&MmGlobalKernelPageDirectoryForPAE[PAE_ADDR_TO_PDE_OFFSET(Address)], &Entry, &ZeroEntry))
	    {
	       MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	    }
	 }
	 ExfInterlockedCompareExchange64(PageDir, &MmGlobalKernelPageDirectoryForPAE[PAE_ADDR_TO_PDE_OFFSET(Address)], &ZeroEntry);
      }
      else
      {
	 if (Create == FALSE)
	 {
            return NULL;
	 }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	 if (!NT_SUCCESS(Status))
	 {
	    KEBUGCHECK(0);
	 }
	 Entry = PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER;
         Entry = ExfInterlockedCompareExchange64(PageDir, &Entry, &ZeroEntry);
	 if (Entry != 0LL)
	 {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	 }
      }
   }
   return (PULONGLONG)PAE_ADDR_TO_PTE(Address); 
}

static PULONG 
MmGetPageTableForProcess(PEPROCESS Process, PVOID Address, BOOLEAN Create)
{
   ULONG PdeOffset = ADDR_TO_PDE_OFFSET(Address);
   NTSTATUS Status;
   PFN_TYPE Pfn;
   ULONG Entry;
   PULONG Pt, PageDir;
   
   if (Address < (PVOID)KERNEL_BASE && Process && Process != PsGetCurrentProcess())
   {
      PageDir = ExAllocatePageWithPhysPage(Process->Pcb.DirectoryTableBase.QuadPart >> PAGE_SHIFT);
      if (PageDir == NULL)
      {
         KEBUGCHECK(0);
      }
      if (PageDir[PdeOffset] == 0)
      {
         if (Create == FALSE)
	 {
	    ExUnmapPage(PageDir);
	    return NULL;
	 }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	 if (!NT_SUCCESS(Status) || Pfn == 0)
	 {
	    KEBUGCHECK(0);
	 }
         Entry = InterlockedCompareExchange(&PageDir[PdeOffset], PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER, 0);
	 if (Entry != 0)
	 {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	    Pfn = PTE_TO_PFN(Entry);
	 }
      }
      else
      {
         Pfn = PTE_TO_PFN(PageDir[PdeOffset]);
      }
      ExUnmapPage(PageDir);
      Pt = ExAllocatePageWithPhysPage(Pfn);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      return Pt + ADDR_TO_PTE_OFFSET(Address);
   }
   PageDir = ADDR_TO_PDE(Address);
   if (*PageDir == 0)
   {
      if (Address >= (PVOID)KERNEL_BASE)
      {
         if (MmGlobalKernelPageDirectory[PdeOffset] == 0)
	 {
	    if (Create == FALSE)
	    {
               return NULL;
	    }
            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	    if (!NT_SUCCESS(Status) || Pfn == 0)
	    {
	       KEBUGCHECK(0);
	    }
	    Entry = PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE;
            if (Ke386CpuidFlags & X86_FEATURE_PGE)
	    {
	       Entry |= PA_GLOBAL;
	    }
	    if(0 != InterlockedCompareExchange(&MmGlobalKernelPageDirectory[PdeOffset], Entry, 0))
	    {
	       MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	    }
	 }
         *PageDir =MmGlobalKernelPageDirectory[PdeOffset];
      }
      else
      {
	 if (Create == FALSE)
	 {
            return NULL;
	 }
         Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
	 if (!NT_SUCCESS(Status) || Pfn == 0)
	 {
	    KEBUGCHECK(0);
	 }
         Entry = InterlockedCompareExchange(PageDir, PFN_TO_PTE(Pfn) | PA_PRESENT | PA_READWRITE | PA_USER, 0);
	 if (Entry != 0)
	 {
	    MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
	 }
      }
   }
   return (PULONG)ADDR_TO_PTE(Address); 
}

BOOLEAN MmUnmapPageTable(PULONG Pt)
{
   if (Ke386Pae)
   {
      if ((PULONGLONG)Pt >= (PULONGLONG)PAGETABLE_MAP && (PULONGLONG)Pt < (PULONGLONG)PAGETABLE_MAP + 4*512*512)
      {
         return TRUE;
      }
   }
   else
   {
      if (Pt >= (PULONG)PAGETABLE_MAP && Pt < (PULONG)PAGETABLE_MAP + 1024*1024)
      {
         return TRUE;
      }
   }
   if (Pt)
   {
      ExUnmapPage((PVOID)PAGE_ROUND_DOWN(Pt));
   }
   return FALSE;
}

static ULONGLONG MmGetPageEntryForProcessForPAE(PEPROCESS Process, PVOID Address)
{
   ULONGLONG Pte;
   PULONGLONG Pt;

   Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
   if (Pt)
   {
      Pte = *Pt;
      MmUnmapPageTable((PULONG)Pt);
      return Pte;
   }
   return 0;
}

static ULONG MmGetPageEntryForProcess(PEPROCESS Process, PVOID Address)
{
   ULONG Pte;
   PULONG Pt;

   Pt = MmGetPageTableForProcess(Process, Address, FALSE);
   if (Pt)
   {
      Pte = *Pt;
      MmUnmapPageTable(Pt);
      return Pte;
   }
   return 0;
}
	    
PFN_TYPE
MmGetPfnForProcess(PEPROCESS Process,
                   PVOID Address)
{

   if (Ke386Pae)
   {
      ULONGLONG Entry;
      Entry = MmGetPageEntryForProcessForPAE(Process, Address);
      if (!(Entry & PA_PRESENT))
      {
         return 0;
      }
      return(PAE_PTE_TO_PFN(Entry));
   }
   else
   {
      ULONG Entry;
      Entry = MmGetPageEntryForProcess(Process, Address);
      if (!(Entry & PA_PRESENT))
      {
         return 0;
      }
      return(PTE_TO_PFN(Entry));
   }
}

VOID
MmDisableVirtualMapping(PEPROCESS Process, PVOID Address, BOOL* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   BOOLEAN WasValid;
   if (Ke386Pae)
   {
      ULONGLONG Pte;
      ULONGLONG tmpPte;
      PULONGLONG Pt;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      /*
       * Atomically disable the present bit and get the old value.
       */
      do
      {
        Pte = *Pt;
	tmpPte = Pte & ~PA_PRESENT;
      } while (Pte != ExfInterlockedCompareExchange64(Pt, &tmpPte, &Pte)); 

      if (MmUnmapPageTable((PULONG)Pt) || Address >= (PVOID)KERNEL_BASE)
      {
         FLUSH_TLB_ONE(Address);
      }
      WasValid = PAE_PAGE_MASK(Pte) != 0LL ? TRUE : FALSE;
      if (!WasValid)
      {
         KEBUGCHECK(0);
      }

      /*
       * Return some information to the caller
       */
      if (WasDirty != NULL)
      {
	  *WasDirty = Pte & PA_DIRTY ? TRUE : FALSE;
      }
      if (Page != NULL)
      {
         *Page = PAE_PTE_TO_PFN(Pte);
      }
   }
   else
   {
      ULONG Pte;
      PULONG Pt;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      /*
       * Atomically disable the present bit and get the old value.
       */
      do
      {
        Pte = *Pt;
      } while (Pte != InterlockedCompareExchange(Pt, Pte & ~PA_PRESENT, Pte)); 

      if (MmUnmapPageTable(Pt) || Address >= (PVOID)KERNEL_BASE)
      {
         FLUSH_TLB_ONE(Address);
      }
      WasValid = (PAGE_MASK(Pte) != 0);
      if (!WasValid)
      {
         KEBUGCHECK(0);
      }

      /*
       * Return some information to the caller
       */
      if (WasDirty != NULL)
      {
         *WasDirty = Pte & PA_DIRTY;
      }
      if (Page != NULL)
      {
         *Page = PTE_TO_PFN(Pte);
      }
   }
}

VOID
MmRawDeleteVirtualMapping(PVOID Address)
{
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG ZeroPte = 0LL;
      Pt = MmGetPageTableForProcessForPAE(NULL, Address, FALSE);
      if (Pt)
      {
         /*
          * Set the entry to zero
          */
	 ExfpInterlockedExchange64(Pt, &ZeroPte);
         FLUSH_TLB_ONE(Address);
      }
   }
   else
   {
      PULONG Pt;

      Pt = MmGetPageTableForProcess(NULL, Address, FALSE);
      if (Pt && *Pt)
      {
         /*
          * Set the entry to zero
          */
         *Pt = 0;
         FLUSH_TLB_ONE(Address);
      }
   }
}

VOID
MmDeleteVirtualMapping(PEPROCESS Process, PVOID Address, BOOL FreePage,
                       BOOL* WasDirty, PPFN_TYPE Page)
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   BOOLEAN WasValid = FALSE;
   PFN_TYPE Pfn;

   DPRINT("MmDeleteVirtualMapping(%x, %x, %d, %x, %x)\n",
          Process, Address, FreePage, WasDirty, Page);
   if (Ke386Pae)
   {
      ULONGLONG Pte;
      PULONGLONG Pt;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         if (WasDirty != NULL)
         {
            *WasDirty = FALSE;
         }
         if (Page != NULL)
         {
            *Page = 0;
         }
         return;
      }

      /*
       * Atomically set the entry to zero and get the old value.
       */
      Pte = 0LL;
      Pte = ExfpInterlockedExchange64(Pt, &Pte);

      if ((MmUnmapPageTable((PULONG)Pt) || Address >=(PVOID)KERNEL_BASE) && Pte)
      {
         FLUSH_TLB_ONE(Address);
      }

      WasValid = PAE_PAGE_MASK(Pte) != 0 ? TRUE : FALSE;
      if (WasValid)
      {
         Pfn = PAE_PTE_TO_PFN(Pte);
         MmMarkPageUnmapped(Pfn);
      }
      else
      {
         Pfn = 0;
      }

      if (FreePage && WasValid)
      {
         MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
      }

      /*
       * Return some information to the caller
       */
      if (WasDirty != NULL)
      {
         *WasDirty = Pte & PA_DIRTY ? TRUE : FALSE;
      }
      if (Page != NULL)
      {
         *Page = Pfn;
      }
   }
   else
   {
      ULONG Pte;
      PULONG Pt;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);

      if (Pt == NULL)
      {
         if (WasDirty != NULL)
         {
            *WasDirty = FALSE;
         }
         if (Page != NULL)
         {
            *Page = 0;
         }
         return;
      }

      /*
       * Atomically set the entry to zero and get the old value.
       */
      Pte = InterlockedExchange(Pt, 0);

      if ((MmUnmapPageTable(Pt) || Address >=(PVOID)KERNEL_BASE) && Pte)
      {
         FLUSH_TLB_ONE(Address);
      }

      WasValid = (PAGE_MASK(Pte) != 0);
      if (WasValid)
      {
         Pfn = PTE_TO_PFN(Pte);
         MmMarkPageUnmapped(Pfn);
      }
      else
      {
         Pfn = 0;
      }

      if (FreePage && WasValid)
      {
         MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
      }

      /*
       * Return some information to the caller
       */
      if (WasDirty != NULL)
      {
         *WasDirty = Pte & PA_DIRTY ? TRUE : FALSE;
      }
      if (Page != NULL)
      {
         *Page = Pfn;
      }
   }
   /*
    * Decrement the reference count for this page table.
    */
   if (Process != NULL && WasValid &&
       Process->AddressSpace.PageTableRefCountTable != NULL &&
       Address < (PVOID)KERNEL_BASE)
   {
      PUSHORT Ptrc;
      ULONG Idx;
      
      Ptrc = Process->AddressSpace.PageTableRefCountTable;
      Idx = Ke386Pae ? PAE_ADDR_TO_PAGE_TABLE(Address) : ADDR_TO_PAGE_TABLE(Address);

      Ptrc[Idx]--;
      if (Ptrc[Idx] == 0)
      {
         MmFreePageTable(Process, Address);
      }
   }
}

VOID
MmDeletePageFileMapping(PEPROCESS Process, PVOID Address,
                        SWAPENTRY* SwapEntry)
/*
 * FUNCTION: Delete a virtual mapping 
 */
{
   if (Ke386Pae)
   {
      ULONGLONG Pte;
      PULONGLONG Pt;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         *SwapEntry = 0;
         return;
      }

      /*
       * Atomically set the entry to zero and get the old value.
       */
      Pte = 0LL;
      Pte = ExfpInterlockedExchange64(Pt, &Pte);

      if (MmUnmapPageTable((PULONG)Pt) || Address >= (PVOID)KERNEL_BASE)
      {
         FLUSH_TLB_ONE(Address);
      }

      /*
       * Decrement the reference count for this page table.
       */
      if (Process != NULL && Pte &&
          Process->AddressSpace.PageTableRefCountTable != NULL &&
          Address < (PVOID)KERNEL_BASE)
      {
         PUSHORT Ptrc;

         Ptrc = Process->AddressSpace.PageTableRefCountTable;

         Ptrc[PAE_ADDR_TO_PAGE_TABLE(Address)]--;
         if (Ptrc[PAE_ADDR_TO_PAGE_TABLE(Address)] == 0)
         {
            MmFreePageTable(Process, Address);
         }
      }


      /*
       * Return some information to the caller
       */
      *SwapEntry = Pte >> 1;
   }
   else
   {
      ULONG Pte;
      PULONG Pt;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
 
      if (Pt == NULL)
      {
         *SwapEntry = 0;
         return;
      }

      /*
       * Atomically set the entry to zero and get the old value.
       */
      Pte = InterlockedExchange(Pt, 0);

      if (MmUnmapPageTable(Pt) || Address >= (PVOID)KERNEL_BASE)
      {
         FLUSH_TLB_ONE(Address);
      }

      /*
       * Decrement the reference count for this page table.
       */
      if (Process != NULL && Pte &&
          Process->AddressSpace.PageTableRefCountTable != NULL &&
          Address < (PVOID)KERNEL_BASE)
      {
         PUSHORT Ptrc;

         Ptrc = Process->AddressSpace.PageTableRefCountTable;

         Ptrc[ADDR_TO_PAGE_TABLE(Address)]--;
         if (Ptrc[ADDR_TO_PAGE_TABLE(Address)] == 0)
         {
            MmFreePageTable(Process, Address);
         }
      }


      /*
       * Return some information to the caller
       */
      *SwapEntry = Pte >> 1;
   }
}

BOOLEAN
Mmi386MakeKernelPageTableGlobal(PVOID PAddress)
{
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      PULONGLONG Pde;
      Pde = PAE_ADDR_TO_PDE(PAddress);
      if (*Pde == 0LL)
      {
         Pt = MmGetPageTableForProcessForPAE(NULL, PAddress, FALSE);
#if 0
         /* Non existing mappings are not cached within the tlb. We must not invalidate this entry */
         FLASH_TLB_ONE(PAddress);
#endif
         if (Pt != NULL)
         {
            return TRUE;
         }
      }
   }
   else
   {
      PULONG Pt, Pde;
      Pde = ADDR_TO_PDE(PAddress);
      if (*Pde == 0)
      {
         Pt = MmGetPageTableForProcess(NULL, PAddress, FALSE);
#if 0
         /* Non existing mappings are not cached within the tlb. We must not invalidate this entry */
         FLASH_TLB_ONE(PAddress);
#endif
         if (Pt != NULL)
         {
            return TRUE;
         }
      }
   }
   return(FALSE);
}

BOOLEAN MmIsDirtyPage(PEPROCESS Process, PVOID Address)
{
   if (Ke386Pae)
   {
      return MmGetPageEntryForProcessForPAE(Process, Address) & PA_DIRTY ? TRUE : FALSE;
   }
   else
   {
      return MmGetPageEntryForProcess(Process, Address) & PA_DIRTY ? TRUE : FALSE;
   }
}

BOOLEAN
MmIsAccessedAndResetAccessPage(PEPROCESS Process, PVOID Address)
{
   if (Address < (PVOID)KERNEL_BASE && Process == NULL)
   {
      DPRINT1("MmIsAccessedAndResetAccessPage is called for user space without a process.\n");
      KEBUGCHECK(0);
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
   
      do
      {
         Pte = *Pt;
	 tmpPte = Pte & ~PA_ACCESSED;
      } while (Pte != ExfInterlockedCompareExchange64(Pt, &tmpPte, &Pte));

      if (Pte & PA_ACCESSED)
      {
         if (MmUnmapPageTable((PULONG)Pt) || Address >= (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
         return TRUE;
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
         return FALSE;
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
   
      do
      {
         Pte = *Pt;
      } while (Pte != InterlockedCompareExchange(Pt, Pte & ~PA_ACCESSED, Pte));

      if (Pte & PA_ACCESSED)
      {
         if (MmUnmapPageTable(Pt) || Address >= (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
         return TRUE;
      }
      else
      {
         MmUnmapPageTable(Pt);
         return FALSE;
      }
   }
}

VOID MmSetCleanPage(PEPROCESS Process, PVOID Address)
{
   if (Address < (PVOID)KERNEL_BASE && Process == NULL)
   {
      DPRINT1("MmSetCleanPage is called for user space without a process.\n");
      KEBUGCHECK(0);
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);

      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
	 tmpPte = Pte & ~PA_DIRTY;
      } while (Pte != ExfInterlockedCompareExchange64(Pt, &tmpPte, &Pte));

      if (Pte & PA_DIRTY)
      {
         if (MmUnmapPageTable((PULONG)Pt) || Address > (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);

      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
      } while (Pte != InterlockedCompareExchange(Pt, Pte & ~PA_DIRTY, Pte));

      if (Pte & PA_DIRTY)
      {
         if (MmUnmapPageTable(Pt) || Address > (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
      }
      else
      {
         MmUnmapPageTable(Pt);
      }
   }
}

VOID MmSetDirtyPage(PEPROCESS Process, PVOID Address)
{
   if (Address < (PVOID)KERNEL_BASE && Process == NULL)
   {
      DPRINT1("MmSetDirtyPage is called for user space without a process.\n");
      KEBUGCHECK(0);
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
	 tmpPte = Pte | PA_DIRTY; 
      } while (Pte != ExfInterlockedCompareExchange64(Pt, &tmpPte, &Pte));
      if (!(Pte & PA_DIRTY))
      {
         if (MmUnmapPageTable((PULONG)Pt) || Address > (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
      } while (Pte != InterlockedCompareExchange(Pt, Pte | PA_DIRTY, Pte));
      if (!(Pte & PA_DIRTY))
      {
         if (MmUnmapPageTable(Pt) || Address > (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
      }
      else
      {
         MmUnmapPageTable(Pt);
      }
   }
}

VOID MmEnableVirtualMapping(PEPROCESS Process, PVOID Address)
{
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
	 tmpPte = Pte | PA_PRESENT;
      } while (Pte != ExfInterlockedCompareExchange64(Pt, &tmpPte, &Pte));
      if (!(Pte & PA_PRESENT))
      {
         if (MmUnmapPageTable((PULONG)Pt) || Address > (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }

      do
      {
         Pte = *Pt;
      } while (Pte != InterlockedCompareExchange(Pt, Pte | PA_PRESENT, Pte));
      if (!(Pte & PA_PRESENT))
      {
         if (MmUnmapPageTable(Pt) || Address > (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
      }
      else
      {
         MmUnmapPageTable(Pt);
      }
   }
}

BOOLEAN MmIsPagePresent(PEPROCESS Process, PVOID Address)
{
   if (Ke386Pae)
   {
      return MmGetPageEntryForProcessForPAE(Process, Address) & PA_PRESENT ? TRUE : FALSE;
   }
   else
   {
      return MmGetPageEntryForProcess(Process, Address) & PA_PRESENT ? TRUE : FALSE;
   }
}

BOOLEAN MmIsPageSwapEntry(PEPROCESS Process, PVOID Address)
{
   if (Ke386Pae)
   {
      ULONGLONG Entry;
      Entry = MmGetPageEntryForProcessForPAE(Process, Address);
      return !(Entry & PA_PRESENT) && Entry != 0 ? TRUE : FALSE;
   }
   else
   {
      ULONG Entry;
      Entry = MmGetPageEntryForProcess(Process, Address);
      return !(Entry & PA_PRESENT) && Entry != 0 ? TRUE : FALSE;
   }
}

NTSTATUS
MmCreateVirtualMappingForKernel(PVOID Address,
                                ULONG flProtect,
                                PPFN_TYPE Pages,
				ULONG PageCount)
{
   ULONG Attributes;
   ULONG i;
   PVOID Addr;
   ULONG PdeOffset, oldPdeOffset;
   BOOLEAN NoExecute = FALSE;

   DPRINT("MmCreateVirtualMappingForKernel(%x, %x, %x, %d)\n",
           Address, flProtect, Pages, PageCount);

   if (Address < (PVOID)KERNEL_BASE)
   {
      DPRINT1("MmCreateVirtualMappingForKernel is called for user space\n");
      KEBUGCHECK(0);
   }

   Attributes = ProtectToPTE(flProtect);
   if (Attributes & 0x80000000)
   {
      NoExecute = TRUE;
   }
   Attributes &= 0xfff;
   if (Ke386CpuidFlags & X86_FEATURE_PGE)
   {
      Attributes |= PA_GLOBAL;
   }

   Addr = Address;

   if (Ke386Pae)
   {
      PULONGLONG Pt = NULL;
      ULONGLONG Pte;

      oldPdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr) + 1;
      for (i = 0; i < PageCount; i++, Addr += PAGE_SIZE)
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            KEBUGCHECK(0);
         }

         PdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            Pt = MmGetPageTableForProcessForPAE(NULL, Addr, TRUE);
	    if (Pt == NULL)
	    {
	       KEBUGCHECK(0);
	    }
         }
         else
         {
            Pt++;
         }
         oldPdeOffset = PdeOffset;

	 Pte = PFN_TO_PTE(Pages[i]) | Attributes;
	 if (NoExecute)
	 {
	    Pte |= 0x8000000000000000LL;
	 }
         Pte = ExfpInterlockedExchange64(Pt, &Pte);
         if (Pte != 0LL)
         {
            KEBUGCHECK(0);
         }
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      oldPdeOffset = ADDR_TO_PDE_OFFSET(Addr);
      Pt = MmGetPageTableForProcess(NULL, Addr, TRUE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      Pt--;

      for (i = 0; i < PageCount; i++, Addr += PAGE_SIZE)
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            KEBUGCHECK(0);
         }

         PdeOffset = ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            Pt = MmGetPageTableForProcess(NULL, Addr, TRUE);
	    if (Pt == NULL)
	    {
	       KEBUGCHECK(0);
	    }
         }
         else
         {
            Pt++;
         }
         oldPdeOffset = PdeOffset;

         Pte = *Pt;
         if (Pte != 0)
         {
            KEBUGCHECK(0);
         }
         *Pt = PFN_TO_PTE(Pages[i]) | Attributes;
      }
   }

   return(STATUS_SUCCESS);
}

NTSTATUS
MmCreatePageFileMapping(PEPROCESS Process,
                        PVOID Address,
                        SWAPENTRY SwapEntry)
{
   if (Process == NULL && Address < (PVOID)KERNEL_BASE)
   {
      DPRINT1("No process\n");
      KEBUGCHECK(0);
   }
   if (Process != NULL && Address >= (PVOID)KERNEL_BASE)
   {
      DPRINT1("Setting kernel address with process context\n");
      KEBUGCHECK(0);
   }
   if (SwapEntry & (1 << 31))
   {
      KEBUGCHECK(0);
   }

   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG Pte;
      ULONGLONG tmpPte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, TRUE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      tmpPte = SwapEntry << 1;
      Pte = ExfpInterlockedExchange64(Pt, &tmpPte);
      if (PAE_PAGE_MASK((Pte)) != 0)
      {
         MmMarkPageUnmapped(PAE_PTE_TO_PFN((Pte)));
      }

      if (Pte != 0) 
      {
         if (MmUnmapPageTable((PULONG)Pt) || Address > (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
      }
      else
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt;
      ULONG Pte;

      Pt = MmGetPageTableForProcess(Process, Address, TRUE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      Pte = *Pt;
      if (PAGE_MASK((Pte)) != 0)
      {
         MmMarkPageUnmapped(PTE_TO_PFN((Pte)));
      }
      *Pt = SwapEntry << 1;
      if (Pte != 0) 
      {
         if (MmUnmapPageTable(Pt) || Address > (PVOID)KERNEL_BASE)
         {
            FLUSH_TLB_ONE(Address);
         }
      }
      else
      {
         MmUnmapPageTable(Pt);
      }
   }
   if (Process != NULL &&
       Process->AddressSpace.PageTableRefCountTable != NULL &&
       Address < (PVOID)KERNEL_BASE)
   {
     PUSHORT Ptrc;
     ULONG Idx;

     Ptrc = Process->AddressSpace.PageTableRefCountTable;
     Idx = Ke386Pae ? PAE_ADDR_TO_PAGE_TABLE(Address) : ADDR_TO_PAGE_TABLE(Address);
     Ptrc[Idx]++;
   }
   return(STATUS_SUCCESS);
}


NTSTATUS
MmCreateVirtualMappingUnsafe(PEPROCESS Process,
                             PVOID Address,
                             ULONG flProtect,
                             PPFN_TYPE Pages,
                             ULONG PageCount)
{
   ULONG Attributes;
   PVOID Addr;
   ULONG i;
   ULONG oldPdeOffset, PdeOffset;
   BOOLEAN NoExecute = FALSE;

   DPRINT("MmCreateVirtualMappingUnsafe(%x, %x, %x, %x (%x), %d)\n",
          Process, Address, flProtect, Pages, *Pages, PageCount);
   
   if (Process == NULL)
   {
      if (Address < (PVOID)KERNEL_BASE)
      {
         DPRINT1("No process\n");
         KEBUGCHECK(0);
      }
      if (PageCount > 0x10000 || 
	  (ULONG_PTR) Address / PAGE_SIZE + PageCount > 0x100000)
      {
         DPRINT1("Page count to large\n");
	 KEBUGCHECK(0);
      }
   }
   else
   {
      if (Address >= (PVOID)KERNEL_BASE)
      {
         DPRINT1("Setting kernel address with process context\n");
         KEBUGCHECK(0);
      }
      if (PageCount > KERNEL_BASE / PAGE_SIZE ||
	  (ULONG_PTR) Address / PAGE_SIZE + PageCount > KERNEL_BASE / PAGE_SIZE)
      {
         DPRINT1("Page Count to large\n");
	 KEBUGCHECK(0);
      }
   }

   Attributes = ProtectToPTE(flProtect);
   if (Attributes & 0x80000000)
   {
      NoExecute = TRUE;
   }
   Attributes &= 0xfff;
   if (Address >= (PVOID)KERNEL_BASE)
   {
      Attributes &= ~PA_USER;
      if (Ke386CpuidFlags & X86_FEATURE_PGE)
      {
	 Attributes |= PA_GLOBAL;
      }
   }
   else
   {
      Attributes |= PA_USER;
   }

   Addr = Address;

   if (Ke386Pae)
   {
      ULONGLONG Pte, tmpPte;
      PULONGLONG Pt = NULL;

      oldPdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr) + 1;
      for (i = 0; i < PageCount; i++, Addr += PAGE_SIZE)
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            KEBUGCHECK(0);
         }
         PdeOffset = PAE_ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            MmUnmapPageTable((PULONG)Pt);
	    Pt = MmGetPageTableForProcessForPAE(Process, Addr, TRUE);
	    if (Pt == NULL)
	    {
	       KEBUGCHECK(0);
	    }
         }
         else
         {
            Pt++;
         }
         oldPdeOffset = PdeOffset;

         MmMarkPageMapped(Pages[i]);
	 tmpPte = PAE_PFN_TO_PTE(Pages[i]) | Attributes;
	 if (NoExecute)
	 {
	    tmpPte |= 0x8000000000000000LL;
	 }
         Pte = ExfpInterlockedExchange64(Pt, &tmpPte);
         if (PAE_PAGE_MASK((Pte)) != 0LL && !((Pte) & PA_PRESENT))
         {
            KEBUGCHECK(0);
         }
         if (PAE_PAGE_MASK((Pte)) != 0LL)
         {
            MmMarkPageUnmapped(PAE_PTE_TO_PFN((Pte)));
         }
         if (Address < (PVOID)KERNEL_BASE &&
	     Process->AddressSpace.PageTableRefCountTable != NULL &&
             Attributes & PA_PRESENT)
         {
            PUSHORT Ptrc;

            Ptrc = Process->AddressSpace.PageTableRefCountTable;

            Ptrc[PAE_ADDR_TO_PAGE_TABLE(Addr)]++;
         }
         if (Pte != 0LL)
         {
            if (Address > (PVOID)KERNEL_BASE ||
                (Pt >= (PULONGLONG)PAGETABLE_MAP && Pt < (PULONGLONG)PAGETABLE_MAP + 4*512*512))
            {
               FLUSH_TLB_ONE(Address);
            }
         }
      }
      if (Addr > Address)
      {
         MmUnmapPageTable((PULONG)Pt);
      }
   }
   else
   {
      PULONG Pt = NULL;
      ULONG Pte;
      oldPdeOffset = ADDR_TO_PDE_OFFSET(Addr) + 1;
      for (i = 0; i < PageCount; i++, Addr += PAGE_SIZE)
      {
         if (!(Attributes & PA_PRESENT) && Pages[i] != 0)
         {
            DPRINT1("Setting physical address but not allowing access at address "
                    "0x%.8X with attributes %x/%x.\n",
                    Addr, Attributes, flProtect);
            KEBUGCHECK(0);
         }
         PdeOffset = ADDR_TO_PDE_OFFSET(Addr);
         if (oldPdeOffset != PdeOffset)
         {
            MmUnmapPageTable(Pt);
	    Pt = MmGetPageTableForProcess(Process, Addr, TRUE);
	    if (Pt == NULL)
	    {
	       KEBUGCHECK(0);
	    }
         }
         else
         {
            Pt++;
         }
         oldPdeOffset = PdeOffset;

         Pte = *Pt;
         MmMarkPageMapped(Pages[i]);
         if (PAGE_MASK((Pte)) != 0 && !((Pte) & PA_PRESENT))
         {
            KEBUGCHECK(0);
         }
         if (PAGE_MASK((Pte)) != 0)
         {
            MmMarkPageUnmapped(PTE_TO_PFN((Pte)));
         }
         *Pt = PFN_TO_PTE(Pages[i]) | Attributes;
         if (Address < (PVOID)KERNEL_BASE &&
	     Process->AddressSpace.PageTableRefCountTable != NULL &&
             Attributes & PA_PRESENT)
         {
            PUSHORT Ptrc;

            Ptrc = Process->AddressSpace.PageTableRefCountTable;

            Ptrc[ADDR_TO_PAGE_TABLE(Addr)]++;
         }
         if (Pte != 0)
         {
            if (Address > (PVOID)KERNEL_BASE ||
                (Pt >= (PULONG)PAGETABLE_MAP && Pt < (PULONG)PAGETABLE_MAP + 1024*1024))
            {
               FLUSH_TLB_ONE(Address);
            }
         }
      }
      if (Addr > Address)
      {
         MmUnmapPageTable(Pt);
      }
   }
   return(STATUS_SUCCESS);
}

NTSTATUS
MmCreateVirtualMapping(PEPROCESS Process,
                       PVOID Address,
                       ULONG flProtect,
                       PPFN_TYPE Pages,
                       ULONG PageCount)
{
   ULONG i;

   for (i = 0; i < PageCount; i++)
   {
      if (!MmIsUsablePage(Pages[i]))
      {
         DPRINT1("Page at address %x not usable\n", Pages[i] << PAGE_SHIFT);
         KEBUGCHECK(0);
      }
   }

   return(MmCreateVirtualMappingUnsafe(Process,
                                       Address,
                                       flProtect,
                                       Pages,
                                       PageCount));
}

ULONG
MmGetPageProtect(PEPROCESS Process, PVOID Address)
{
   ULONG Entry;
   ULONG Protect;
   if (Ke386Pae)
   {
      Entry = MmGetPageEntryForProcessForPAE(Process, Address);
   }
   else
   {
      Entry = MmGetPageEntryForProcess(Process, Address);
   }

   if (!(Entry & PA_PRESENT))
   {
      Protect = PAGE_NOACCESS;
   }
   else 
   {
      if (Entry & PA_READWRITE)
      {
         Protect = PAGE_READWRITE;
      }
      else
      {
         Protect = PAGE_EXECUTE_READ;
      }
      if (Entry & PA_CD)
      {
         Protect |= PAGE_NOCACHE;
      }
      if (Entry & PA_WT)
      {
         Protect |= PAGE_WRITETHROUGH;
      }
      if (!(Entry & PA_USER))
      {
         Protect |= PAGE_SYSTEM;
      }
      		
   }
   return(Protect);
}

VOID
MmSetPageProtect(PEPROCESS Process, PVOID Address, ULONG flProtect)
{
   ULONG Attributes = 0;
   BOOLEAN NoExecute = FALSE;

   DPRINT("MmSetPageProtect(Process %x  Address %x  flProtect %x)\n",
          Process, Address, flProtect);

   Attributes = ProtectToPTE(flProtect);
   if (Attributes & 0x80000000)
   {
      NoExecute = TRUE;
   }
   Attributes &= 0xfff;
   if (Address >= (PVOID)KERNEL_BASE)
   {
      Attributes &= ~PA_USER;
      if (Ke386CpuidFlags & X86_FEATURE_PGE)
      {
	 Attributes |= PA_GLOBAL;
      }
   }
   else
   {
      Attributes |= PA_USER;
   }
   if (Ke386Pae)
   {
      PULONGLONG Pt;
      ULONGLONG tmpPte, Pte;

      Pt = MmGetPageTableForProcessForPAE(Process, Address, FALSE);
      if (Pt == NULL)
      {
         DPRINT1("Address %x\n", Address);
         KEBUGCHECK(0);
      }
      do
      {
        Pte = *Pt;
	tmpPte = PAE_PAGE_MASK(Pte) | Attributes | (Pte & (PA_ACCESSED|PA_DIRTY));
	if (NoExecute)
	{
	   tmpPte |= 0x8000000000000000LL;
	}
	else
	{
	   tmpPte &= ~0x8000000000000000LL;
	}
      } while (Pte != ExfInterlockedCompareExchange64(Pt, &tmpPte, &Pte));

      if (MmUnmapPageTable((PULONG)Pt) || Address > (PVOID)KERNEL_BASE)
      {
         FLUSH_TLB_ONE(Address);
      }
   }
   else
   {
      PULONG Pt;

      Pt = MmGetPageTableForProcess(Process, Address, FALSE);
      if (Pt == NULL)
      {
         KEBUGCHECK(0);
      }
      *Pt = PAGE_MASK(*Pt) | Attributes | (*Pt & (PA_ACCESSED|PA_DIRTY));
      if (MmUnmapPageTable(Pt) || Address > (PVOID)KERNEL_BASE)
      {
         FLUSH_TLB_ONE(Address);
      }
   }
}

/*
 * @implemented
 */
PHYSICAL_ADDRESS STDCALL
MmGetPhysicalAddress(PVOID vaddr)
/*
 * FUNCTION: Returns the physical address corresponding to a virtual address
 */
{
   PHYSICAL_ADDRESS p;

   DPRINT("MmGetPhysicalAddress(vaddr %x)\n", vaddr);
   if (Ke386Pae)
   {
      ULONGLONG Pte;
      Pte = MmGetPageEntryForProcessForPAE(NULL, vaddr);
      if (Pte != 0 && Pte & PA_PRESENT)
      {
         p.QuadPart = PAE_PAGE_MASK(Pte);
         p.u.LowPart |= (ULONG_PTR)vaddr & (PAGE_SIZE - 1);
      }
      else
      {
         p.QuadPart = 0;
      }
   }
   else
   {
      ULONG Pte;
      Pte = MmGetPageEntryForProcess(NULL, vaddr);
      if (Pte != 0 && Pte & PA_PRESENT)
      {
         p.QuadPart = PAGE_MASK(Pte);
         p.u.LowPart |= (ULONG_PTR)vaddr & (PAGE_SIZE - 1);
      }
      else
      {
         p.QuadPart = 0;
      }
   }
   return p;
}

VOID MmUpdatePageDir(PEPROCESS Process, PVOID Address, ULONG Size)
{
   ULONG StartOffset, EndOffset, Offset; 

   if (Address < (PVOID)KERNEL_BASE)
   {
      KEBUGCHECK(0);
   }
   if (Ke386Pae)
   {
      PULONGLONG PageDirTable;
      PULONGLONG Pde;
      ULONGLONG ZeroPde = 0LL;
      StartOffset = PAE_ADDR_TO_PDE_OFFSET(Address);
      EndOffset = PAE_ADDR_TO_PDE_OFFSET(Address + Size);

      if (Process != NULL && Process != PsGetCurrentProcess())
      {
         PageDirTable = ExAllocatePageWithPhysPage(Process->Pcb.DirectoryTableBase.u.LowPart >> PAGE_SHIFT);
         Pde = (PULONGLONG)ExAllocatePageWithPhysPage(PageDirTable[3] >> PAGE_SHIFT);
	 ExUnmapPage(PageDirTable);
      }
      else
      {
         Pde = (PULONGLONG)PAE_PAGEDIRECTORY_MAP + 3*512;
      }

      for (Offset = StartOffset; Offset <= EndOffset; Offset++) 
      {
         if (Offset < PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) || Offset >= PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP)+4)
         {
            ExfInterlockedCompareExchange64(&Pde[Offset], &MmGlobalKernelPageDirectoryForPAE[Offset], &ZeroPde);
         }
      }
      MmUnmapPageTable((PULONG)Pde);
   }
   else
   {
      PULONG Pde;
      StartOffset = ADDR_TO_PDE_OFFSET(Address);
      EndOffset = ADDR_TO_PDE_OFFSET(Address + Size);
   
      if (Process != NULL && Process != PsGetCurrentProcess())
      {
         Pde = ExAllocatePageWithPhysPage(Process->Pcb.DirectoryTableBase.u.LowPart >> PAGE_SHIFT);
      }
      else
      {
         Pde = (PULONG)PAGEDIRECTORY_MAP;
      }
      for (Offset = StartOffset; Offset <= EndOffset; Offset++) 
      {
         if (Offset != ADDR_TO_PDE_OFFSET(PAGETABLE_MAP))
         {
            InterlockedCompareExchange(&Pde[Offset], MmGlobalKernelPageDirectory[Offset], 0);
         }
      }
      if (Pde != (PULONG)PAGEDIRECTORY_MAP)
      {
         ExUnmapPage(Pde);
      }
   }
}
	    
VOID INIT_FUNCTION
MmInitGlobalKernelPageDirectory(VOID)
{
   ULONG i;
   if (Ke386Pae)
   {
      PULONGLONG CurrentPageDirectory = (PULONGLONG)PAE_PAGEDIRECTORY_MAP + 3*512;
      for (i = 0; i < 512; i++)
      {
         if ((i < PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) || i >= PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) + 4) &&
             0LL == MmGlobalKernelPageDirectoryForPAE[i] && 0LL != CurrentPageDirectory[i])
         {
            ExfpInterlockedExchange64(&MmGlobalKernelPageDirectoryForPAE[i], &CurrentPageDirectory[i]);
	    if (Ke386CpuidFlags & X86_FEATURE_PGE)
	    {
               MmGlobalKernelPageDirectoryForPAE[i] |= PA_GLOBAL;
	    }
	 }
      }
   }
   else
   {
      PULONG CurrentPageDirectory = (PULONG)PAGEDIRECTORY_MAP;
      for (i = ADDR_TO_PDE_OFFSET(KERNEL_BASE); i < 1024; i++)
      {
         if (i != ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) &&
             0 == MmGlobalKernelPageDirectory[i] && 0 != CurrentPageDirectory[i])
         {
            MmGlobalKernelPageDirectory[i] = CurrentPageDirectory[i];
	    if (Ke386CpuidFlags & X86_FEATURE_PGE)
	    {
               MmGlobalKernelPageDirectory[i] |= PA_GLOBAL;
	    }
         }
      }
   }
}

extern ULONGLONG pae_pagedirtable;

VOID INIT_FUNCTION
MiEnablePAE(PVOID* LastKernelAddress)
{
   PULONGLONG PageDirTable;
   PULONGLONG PageDir;

   ULONG i, k;

   ULONG Flags;

   PageDirTable = &pae_pagedirtable;

   if (LastKernelAddress)
   {
      /* This is the boot processor */
      memcpy(PageDirTable, (PVOID)PAGEDIRECTORY_MAP, PAGE_SIZE);

      PageDir = (PULONGLONG)*LastKernelAddress;
      (*LastKernelAddress) += 4 * PAGE_SIZE;
      PageDirTable[0] = MmGetPhysicalAddress((PVOID)PageDir).QuadPart | PA_PRESENT;
      PageDirTable[1] = PageDirTable[0] + PAGE_SIZE;
      PageDirTable[2] = PageDirTable[1] + PAGE_SIZE;
      PageDirTable[3] = PageDirTable[2] + PAGE_SIZE;

      memset(PageDir, 0, 4 * PAGE_SIZE);

      for (i = 0; i < 4; i++)
      {
         PageDir[3*512+PAE_ADDR_TO_PDE_OFFSET(PAGETABLE_MAP) + i] = PageDirTable[i] | PA_READWRITE;
      }

      for (i = 0; i < 2048; i++)
      {
         if (i < /*PAE_ADDR_TO_PDE_OFFSET(PAE_PAGEDIRECTORY_MAP)*/0x780 || i >= /*PAE_ADDR_TO_PDE_OFFSET(PAE_PAGEDIRECTORY_MAP)*/0x780 + 4)
         {
            PVOID Address = (PVOID)(i * 512 * PAGE_SIZE);
	    PULONG Pde = ADDR_TO_PDE(Address);
	    PULONGLONG PtePAE=NULL;

 	    if (*Pde != 0)
	    {
	       if (PageDir[i] == 0LL)
	       {
	          PtePAE = (PULONGLONG)(ULONG_PTR)*LastKernelAddress;
	          (*LastKernelAddress) += PAGE_SIZE;
	          PageDir[i] = MmGetPhysicalAddress((PVOID)PtePAE).QuadPart | PA_PRESENT | PA_READWRITE;
	          memset(PtePAE, 0, PAGE_SIZE);
	       }
	       for (k = 0; k < 512; k++)
	       {
	          PULONG Pte = ADDR_TO_PTE(Address + k * PAGE_SIZE);
	          if (*Pte != 0)
	          {
		     PtePAE[k] = *Pte;
 
	          }
	       }
	    }
	 }
      }
   }
   else
   {
      /* this is an application processor in a mp system */
      if (!Ke386Pae)
      {
         return;
      }
   }

   Ke386SaveFlags(Flags);
   Ke386DisableInterrupts();

   Ke386SetPageTableDirectory(MmGetPhysicalAddress(PageDirTable).u.LowPart)

   Ke386SetCr4(Ke386GetCr4() | X86_CR4_PAE);
   if (LastKernelAddress)
   {
      Ke386Pae = TRUE;
   }
   Ke386RestoreFlags(Flags);
}

ULONG
MiGetUserPageDirectoryCount(VOID)
{
   return Ke386Pae ? 1536 : 768;
}

VOID INIT_FUNCTION
MiInitPageDirectoryMap(VOID)
{
   MEMORY_AREA* kernel_map_desc = NULL;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   PVOID BaseAddress;

   BoundaryAddressMultiple.QuadPart = 0;
   BaseAddress = (PVOID)PAGETABLE_MAP;
   MmCreateMemoryArea(NULL,
                      MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
		      Ke386Pae ? 0x800000 : 0x400000,
                      0,
                      &kernel_map_desc,
                      TRUE,
                      FALSE,
                      BoundaryAddressMultiple);
}

/* EOF */
