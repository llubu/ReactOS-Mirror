/* $Id: mminit.c,v 1.43 2003/01/11 15:26:59 hbirr Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/mminit.c
 * PURPOSE:     kernel memory managment initialization functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 9/4/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/i386/segment.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/*
 * Size of extended memory (kb) (fixed for now)
 */
#define EXTENDED_MEMORY_SIZE  (3*1024*1024)

/*
 * Compiler defined symbols
 */
extern unsigned int _text_start__;
extern unsigned int _text_end__;

static BOOLEAN IsThisAnNtAsSystem = FALSE;
static MM_SYSTEM_SIZE MmSystemSize = MmSmallSystem;

extern unsigned int _bss_end__;

static MEMORY_AREA* kernel_text_desc = NULL;
static MEMORY_AREA* kernel_data_desc = NULL;
static MEMORY_AREA* kernel_param_desc = NULL;
static MEMORY_AREA* kernel_pool_desc = NULL;
static MEMORY_AREA* kernel_shared_data_desc = NULL;
static MEMORY_AREA* MiPagedPoolDescriptor = NULL;

PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;

/* FUNCTIONS ****************************************************************/

BOOLEAN STDCALL MmIsThisAnNtAsSystem(VOID)
{
   return(IsThisAnNtAsSystem);
}

MM_SYSTEM_SIZE STDCALL MmQuerySystemSize(VOID)
{
   return(MmSystemSize);
}

VOID MiShutdownMemoryManager(VOID)
{
}

VOID MmInitVirtualMemory(ULONG LastKernelAddress,
			 ULONG KernelLength)
/*
 * FUNCTION: Intialize the memory areas list
 * ARGUMENTS:
 *           bp = Pointer to the boot parameters
 *           kernel_len = Length of the kernel
 */
{
   PVOID BaseAddress;
   ULONG Length;
   ULONG ParamLength = KernelLength;
   NTSTATUS Status;
   //ULONG i;
   
   DPRINT("MmInitVirtualMemory(%x, %x)\n",LastKernelAddress, KernelLength);
   
   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);
   
   MmInitMemoryAreas();
   ExInitNonPagedPool(LastKernelAddress + PAGE_SIZE);

   /*
    * Setup the system area descriptor list
    */
   BaseAddress = (PVOID)KERNEL_BASE;
   Length = PAGE_ROUND_UP(((ULONG)&_text_end__)) - KERNEL_BASE;
   ParamLength = ParamLength - Length;
   
   /*
    * No need to lock the address space at this point since no
    * other threads are running.
    */
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_text_desc,
		      FALSE);

   Length = PAGE_ROUND_UP(((ULONG)&_bss_end__)) - 
            PAGE_ROUND_UP(((ULONG)&_text_end__));
   ParamLength = ParamLength - Length;
   DPRINT("Length %x\n",Length);
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&_text_end__));
   DPRINT("BaseAddress %x\n",BaseAddress);

   /*
    * No need to lock the address space at this point since we are
    * the only thread running.
    */
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),		      
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_data_desc,
		      FALSE);
   
   BaseAddress = (PVOID)PAGE_ROUND_UP(((ULONG)&_bss_end__));
//   Length = ParamLength;
   Length = LastKernelAddress - (ULONG)BaseAddress;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),		      
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_param_desc,
		      FALSE);

   BaseAddress = (PVOID)(LastKernelAddress + PAGE_SIZE);
   Length = NONPAGED_POOL_SIZE;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_pool_desc,
		      FALSE);

   MmPagedPoolSize = MM_PAGED_POOL_SIZE;
   BaseAddress = (PVOID)(LastKernelAddress + PAGE_SIZE + NONPAGED_POOL_SIZE +
			 PAGE_SIZE);
   MmPagedPoolBase = BaseAddress;
   Length = MM_PAGED_POOL_SIZE;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_PAGED_POOL,
		      &BaseAddress,
		      Length,
		      0,
		      &MiPagedPoolDescriptor,
		      FALSE);
   MmInitializePagedPool();

   /*
    * Create the kernel mapping of the user/kernel shared memory.
    */
   BaseAddress = (PVOID)KI_USER_SHARED_DATA;
   Length = PAGE_SIZE;
   MmCreateMemoryArea(NULL,
		      MmGetKernelAddressSpace(),
		      MEMORY_AREA_SYSTEM,
		      &BaseAddress,
		      Length,
		      0,
		      &kernel_shared_data_desc,
		      FALSE);
   Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, 
					&MmSharedDataPagePhysicalAddress);
   Status = MmCreateVirtualMapping(NULL,
				   (PVOID)KI_USER_SHARED_DATA,
				   PAGE_READWRITE,
				   MmSharedDataPagePhysicalAddress,
				   TRUE);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Unable to create virtual mapping\n");
	KeBugCheck(0);
     }
   RtlZeroMemory(BaseAddress, Length);

   /*
    *
    */
   MmInitializeMemoryConsumer(MC_USER, MmTrimUserMemory);
}

VOID MmInit1(ULONG FirstKrnlPhysAddr,
	     ULONG LastKrnlPhysAddr,
	     ULONG LastKernelAddress,
	     PADDRESS_RANGE BIOSMemoryMap,
	     ULONG AddressRangeCount)
/*
 * FUNCTION: Initalize memory managment
 */
{
   ULONG i;
   ULONG kernel_len;
#ifndef MP
   extern unsigned int unmap_me, unmap_me2, unmap_me3;
#endif

   DPRINT("MmInit1(FirstKrnlPhysAddr, %x, LastKrnlPhysAddr %x, LastKernelAddress %x)\n",
		  FirstKrnlPhysAddr,
		  LastKrnlPhysAddr,
		  LastKernelAddress);

   if ((BIOSMemoryMap != NULL) && (AddressRangeCount > 0))
     {
	// If we have a bios memory map, recalulate the the memory size
	ULONG last = 0;
	for (i = 0; i < AddressRangeCount; i++)
	  {
	    if (BIOSMemoryMap[i].Type == 1
	        && (BIOSMemoryMap[i].BaseAddrLow + BIOSMemoryMap[i].LengthLow + PAGE_SIZE -1) / PAGE_SIZE > last)
	      {
		 last = (BIOSMemoryMap[i].BaseAddrLow + BIOSMemoryMap[i].LengthLow + PAGE_SIZE -1) / PAGE_SIZE;
	      }
	  }
	if ((last - 256) * 4 > KeLoaderBlock.MemHigher)
	  {
	     KeLoaderBlock.MemHigher = (last - 256) * 4;
	  }
     }

   /*
    * FIXME: Set this based on the system command line
    */
   MmUserProbeAddress = (PVOID)0x7fff0000;
   MmHighestUserAddress = (PVOID)0x7ffeffff;
   
   /*
    * Initialize memory managment statistics
    */
   MmStats.NrTotalPages = 0;
   MmStats.NrSystemPages = 0;
   MmStats.NrUserPages = 0;
   MmStats.NrReservedPages = 0;
   MmStats.NrUserPages = 0;
   MmStats.NrFreePages = 0;
   MmStats.NrLockedPages = 0;
   MmStats.PagingRequestsInLastMinute = 0;
   MmStats.PagingRequestsInLastFiveMinutes = 0;
   MmStats.PagingRequestsInLastFifteenMinutes = 0;
   
   /*
    * Initialize the kernel address space
    */
   MmInitializeKernelAddressSpace();

   /*
    * Unmap low memory
    */
#ifndef MP
   /* In SMP mode we unmap the low memory in MmInit3. 
      The APIC needs the mapping of the first pages
      while the processors are starting up. */
   MmDeletePageTable(NULL, 0);
#endif
   /*
    * Free all pages not used for kernel memory
    * (we assume the kernel occupies a continuous range of physical
    * memory)
    */
   DPRINT("first krnl %x\nlast krnl %x\n",FirstKrnlPhysAddr,
	  LastKrnlPhysAddr);
   
   /*
    * Free physical memory not used by the kernel
    */
   MmStats.NrTotalPages = KeLoaderBlock.MemHigher/4;
   if (!MmStats.NrTotalPages)
     {
       DbgPrint("Memory not detected, default to 8 MB\n");
       MmStats.NrTotalPages = 2048;
     }
   else
     {
       /* add 1MB for standard memory (not extended) */
       MmStats.NrTotalPages += 256;
     }
#ifdef BIOS_MEM_FIX
  MmStats.NrTotalPages += 16;
#endif
   DbgPrint("Used memory %dKb\n", (MmStats.NrTotalPages * PAGE_SIZE) / 1024);

   LastKernelAddress = (ULONG)MmInitializePageList(
					   (PVOID)FirstKrnlPhysAddr,
					   (PVOID)LastKrnlPhysAddr,
					   MmStats.NrTotalPages,
					   PAGE_ROUND_UP(LastKernelAddress),
             BIOSMemoryMap,
             AddressRangeCount);
   kernel_len = LastKrnlPhysAddr - FirstKrnlPhysAddr;
   
   /*
    * Create a trap for null pointer references and protect text
    * segment
    */
   CHECKPOINT;
   DPRINT("_text_start__ %x _text_end__ %x\n",(int)&_text_start__,(int)&_text_end__);
   for (i=PAGE_ROUND_UP(((int)&_text_start__));
	i<PAGE_ROUND_DOWN(((int)&_text_end__));i=i+PAGE_SIZE)
     {
	MmSetPageProtect(NULL,
			 (PVOID)i,
			 PAGE_EXECUTE_READ);
   }

   DPRINT("Invalidating between %x and %x\n",
	  LastKernelAddress,
	  KERNEL_BASE + 2 * PAGE_TABLE_SIZE);
   for (i=(LastKernelAddress); 
	 i<(KERNEL_BASE + 2 * PAGE_TABLE_SIZE); 
	 i=i+PAGE_SIZE)
     {
	 MmDeleteVirtualMapping(NULL, (PVOID)(i), FALSE, NULL, NULL);
     }
   DPRINT("Almost done MmInit()\n");
#ifndef MP
   /* FIXME: This is broken in SMP mode */
   MmDeleteVirtualMapping(NULL, (PVOID)&unmap_me, FALSE, NULL, NULL);
   MmDeleteVirtualMapping(NULL, (PVOID)&unmap_me2, FALSE, NULL, NULL);
   MmDeleteVirtualMapping(NULL, (PVOID)&unmap_me3, FALSE, NULL, NULL);
#endif
   /*
    * Intialize memory areas
    */
   MmInitVirtualMemory(LastKernelAddress, kernel_len);

   MmInitializeMdlImplementation();
}

VOID MmInit2(VOID)
{
   MmInitSectionImplementation();
   MmInitPagingFile();
}

VOID MmInit3(VOID)
{
   /*
    * Unmap low memory
    */
#ifdef MP
   /* In SMP mode we can unmap the low memory  
      if all processors are started. */
   MmDeletePageTable(NULL, 0);
#endif
   MmInitPagerThread();
   MmCreatePhysicalMemorySection();
   MmInitializeRmapList();
   MmInitializePageOp();

   /*
    * Initialise the modified page writer.
    */
   MmInitMpwThread();

   /* FIXME: Read parameters from memory */
}

