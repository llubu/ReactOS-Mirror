/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/mminit.c
 * PURPOSE:         Memory Manager Initialization
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

PCHAR
MemType[] =
{
    "ExceptionBlock    ",
    "SystemBlock       ",
    "Free              ",
    "Bad               ",
    "LoadedProgram     ",
    "FirmwareTemporary ",
    "FirmwarePermanent ",
    "OsloaderHeap      ",
    "OsloaderStack     ",
    "SystemCode        ",
    "HalCode           ",
    "BootDriver        ",
    "ConsoleInDriver   ",
    "ConsoleOutDriver  ",
    "StartupDpcStack   ",
    "StartupKernelStack",
    "StartupPanicStack ",
    "StartupPcrPage    ",
    "StartupPdrPage    ",
    "RegistryData      ",
    "MemoryData        ",
    "NlsData           ",
    "SpecialMemory     ",
    "BBTMemory         ",
    "LoaderReserve     ",
    "LoaderXIPRom      "
};

BOOLEAN IsThisAnNtAsSystem = FALSE;
MM_SYSTEMSIZE MmSystemSize = MmSmallSystem;
PHYSICAL_ADDRESS MmSharedDataPagePhysicalAddress;
PVOID MiNonPagedPoolStart;
ULONG MiNonPagedPoolLength;
ULONG MmBootImageSize;
ULONG MmNumberOfPhysicalPages, MmHighestPhysicalPage, MmLowestPhysicalPage;
ULONG_PTR MiKSeg0Start, MiKSeg0End;
PVOID MmPfnDatabase;
ULONG_PTR MmPfnDatabaseEnd;
PMEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptor;
MEMORY_ALLOCATION_DESCRIPTOR MiFreeDescriptorOrg;
extern KMUTANT MmSystemLoadLock;
extern HANDLE MpwThreadHandle;
extern BOOLEAN MpwThreadShouldTerminate;
extern KEVENT MpwThreadEvent;
BOOLEAN MiDbgEnableMdDump =
#ifdef _ARM_
TRUE;
#else
FALSE;
#endif

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
MiShutdownMemoryManager(VOID)
{
#if 0
    ULONG PagesWritten;
    PETHREAD Thread;

    /* Ask MPW thread to shutdown */
    MpwThreadShouldTerminate = TRUE;
    KeSetEvent(&MpwThreadEvent, IO_NO_INCREMENT, FALSE);

    /* Wait for it */
    ObReferenceObjectByHandle(MpwThreadHandle,
                              THREAD_ALL_ACCESS,
                              PsThreadType,
                              KernelMode,
                              (PVOID*)&Thread,
                              NULL);

    KeWaitForSingleObject(Thread,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    ObDereferenceObject(Thread);

    /* Check if there are any dirty pages, and flush them.
       There will be no other chance to do this later, since filesystems
       are going to be shut down. */
    CcRosFlushDirtyPages(128, &PagesWritten);
#endif
}

VOID
INIT_FUNCTION
NTAPI
MmInitVirtualMemory()
{
   PVOID BaseAddress;
   ULONG Length;
   NTSTATUS Status;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   PMEMORY_AREA MArea;

   BoundaryAddressMultiple.QuadPart = 0;

   MmInitMemoryAreas();

   DPRINT("NonPagedPool %x - %x, PagedPool %x - %x\n", MiNonPagedPoolStart, (ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength - 1,
           MmPagedPoolBase, (ULONG_PTR)MmPagedPoolBase + MmPagedPoolSize - 1);

   MiInitializeNonPagedPool();

   /*
    * Setup the system area descriptor list
    */
   MiInitPageDirectoryMap();

   BaseAddress = (PVOID)PCR;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      PAGE_SIZE * MAXIMUM_PROCESSORS,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   /* Local APIC base */
   BaseAddress = (PVOID)0xFEE00000;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      PAGE_SIZE,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   /* i/o APIC base */
   BaseAddress = (PVOID)0xFEC00000;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      PAGE_SIZE,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   BaseAddress = (PVOID)0xFF3A0000;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      0x20000,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   BaseAddress = MiNonPagedPoolStart;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      MiNonPagedPoolLength,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   BaseAddress = MmPagedPoolBase;
   Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                               MEMORY_AREA_PAGED_POOL,
                               &BaseAddress,
                               MmPagedPoolSize,
                               PAGE_READWRITE,
                               &MArea,
                               TRUE,
                               0,
                               BoundaryAddressMultiple);

   MmInitializePagedPool();

   /*
    * Create the kernel mapping of the user/kernel shared memory.
    */
   BaseAddress = (PVOID)KI_USER_SHARED_DATA;
   Length = PAGE_SIZE;
   MmCreateMemoryArea(MmGetKernelAddressSpace(),
                      MEMORY_AREA_SYSTEM,
                      &BaseAddress,
                      Length,
                      PAGE_READWRITE,
                      &MArea,
                      TRUE,
                      0,
                      BoundaryAddressMultiple);

   /* Shared data are always located the next page after PCR */
   MmSharedDataPagePhysicalAddress = MmGetPhysicalAddress((PVOID)PCR);
   MmSharedDataPagePhysicalAddress.QuadPart += PAGE_SIZE;

   /*
    *
    */
   MmInitializeMemoryConsumer(MC_USER, MmTrimUserMemory);
}

VOID
NTAPI
MiCountFreePagesInLoaderBlock(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    ULONG FreePages = 0;

    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
        NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
        NextEntry = NextEntry->Flink)
    {
        Md = CONTAINING_RECORD(NextEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);

        /* Skip invisible memory */
        if ((Md->MemoryType != LoaderFirmwarePermanent) &&
            (Md->MemoryType != LoaderSpecialMemory) &&
            (Md->MemoryType != LoaderHALCachedMemory) &&
            (Md->MemoryType != LoaderBBTMemory))
        {
            /* Check if BURNMEM was used */
            if (Md->MemoryType != LoaderBad)
            {
                /* Count this in the total of pages */
                MmNumberOfPhysicalPages += Md->PageCount;
            }
            
            /* Check if this is the new lowest page */
            if (Md->BasePage < MmLowestPhysicalPage)
            {
                /* Update the lowest page */
                MmLowestPhysicalPage = Md->BasePage;
            }
            
            /* Check if this is the new highest page */
            if ((Md->BasePage + Md->PageCount) > MmHighestPhysicalPage)
            {
                /* Update the highest page */
                MmHighestPhysicalPage = Md->BasePage + Md->PageCount - 1;
            }

            /* Check if this is free memory */
            if ((Md->MemoryType == LoaderFree) ||
                (Md->MemoryType == LoaderLoadedProgram) ||
                (Md->MemoryType == LoaderFirmwareTemporary) ||
                (Md->MemoryType == LoaderOsloaderStack))
            {
                /* Check if this is the largest memory descriptor */
                if (Md->PageCount > FreePages)
                {
                    /* For now, it is */
                    FreePages = Md->PageCount;
                    MiFreeDescriptor = Md;
                }
            }
        }
    }

    /* Save original values of the free descriptor, since it'll be
       altered by early allocations */
    MiFreeDescriptorOrg = *MiFreeDescriptor;
}

VOID
NTAPI
MiDbgKernelLayout(VOID)
{
    DPRINT1("%8s%12s\t\t%s\n", "Start", "End", "Type");
    DPRINT1("0x%p - 0x%p\t%s\n",
            KSEG0_BASE, MiKSeg0Start,
            "Undefined region");
    DPRINT1("0x%p - 0x%p\t%s\n",
            MiKSeg0Start, MiKSeg0End,
            "FreeLDR Kernel mapping region");
    DPRINT1("0x%p - 0x%p\t%s\n",
            MmPfnDatabase, MmPfnDatabaseEnd,
            "PFN Database region");
    if (MmPfnDatabaseEnd != (ULONG_PTR)MiNonPagedPoolStart)
    DPRINT1("0x%p - 0x%p\t%s\n",
            MmPfnDatabaseEnd, MiNonPagedPoolStart,
            "Remaining FreeLDR mapping");
    DPRINT1("0x%p - 0x%p\t%s\n",
             MiNonPagedPoolStart, (ULONG_PTR)MiNonPagedPoolStart + MiNonPagedPoolLength,
            "Non paged pool region");
    DPRINT1("0x%p - 0x%p\t%s\n",
            MmPagedPoolBase, (ULONG_PTR)MmPagedPoolBase + MmPagedPoolSize,
            "Paged pool region");
}

VOID
NTAPI
MiDbgDumpMemoryDescriptors(VOID)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    ULONG TotalPages = 0;
    
    DPRINT1("Base\t\tLength\t\tType\n");
    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
         NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
         NextEntry = NextEntry->Flink)
    {
        Md = CONTAINING_RECORD(NextEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
        DPRINT1("%08lX\t%08lX\t%s\n", Md->BasePage, Md->PageCount, MemType[Md->MemoryType]);
        TotalPages += Md->PageCount;
    }

    DPRINT1("Total: %08lX (%d MB)\n", TotalPages, (TotalPages * PAGE_SIZE) / 1024 / 1024);
}

ULONG_PTR
NTAPI
MiGetLastKernelAddress(VOID)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    ULONG_PTR LastKrnlPhysAddr = 0;

    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
         NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
         NextEntry = NextEntry->Flink)
    {
        Md = CONTAINING_RECORD(NextEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);

        if (Md->MemoryType != LoaderFree &&
            Md->MemoryType != LoaderFirmwareTemporary &&
            Md->MemoryType != LoaderSpecialMemory)
        {
            if (Md->BasePage+Md->PageCount > LastKrnlPhysAddr)
                LastKrnlPhysAddr = Md->BasePage+Md->PageCount;
        }
    }

    /* Convert to a physical address */
    return LastKrnlPhysAddr << PAGE_SHIFT;
}

VOID
INIT_FUNCTION
NTAPI
MmInit1(VOID)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    
    /* Dump memory descriptors */
    if (MiDbgEnableMdDump) MiDbgDumpMemoryDescriptors();

    /* Set the page directory */
    PsGetCurrentProcess()->Pcb.DirectoryTableBase.LowPart = (ULONG)MmGetPageDirectory();

    /* Get the size of FreeLDR's image allocations */
    MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned;
    MmBootImageSize *= PAGE_SIZE;

    /* Set memory limits */
    MmSystemRangeStart = (PVOID)KSEG0_BASE;
    MmUserProbeAddress = (ULONG_PTR)MmSystemRangeStart - 0x10000;
    MmHighestUserAddress = (PVOID)(MmUserProbeAddress - 1);
    DPRINT("MmSystemRangeStart:  %08x\n", MmSystemRangeStart);
    DPRINT("MmUserProbeAddress:  %08x\n", MmUserProbeAddress);
    DPRINT("MmHighestUserAddress:%08x\n", MmHighestUserAddress);

    /* Initialize memory managment statistics */
    RtlZeroMemory(&MmStats, sizeof(MmStats));
    
    /* Count RAM */
    MiCountFreePagesInLoaderBlock(KeLoaderBlock);
    DbgPrint("Used memory %dKb\n", (MmNumberOfPhysicalPages * PAGE_SIZE) / 1024);
    
    /* Initialize the kernel address space */
    MmInitializeKernelAddressSpace();
    MmInitGlobalKernelPageDirectory();
    
    /* Get kernel address boundaries */
    LdrEntry = CONTAINING_RECORD(KeLoaderBlock->LoadOrderListHead.Flink,
                                 LDR_DATA_TABLE_ENTRY,
                                 InLoadOrderLinks);
    MiKSeg0Start = (ULONG_PTR)LdrEntry->DllBase | KSEG0_BASE;
    MiKSeg0End = PAGE_ROUND_UP(MiGetLastKernelAddress()  | KSEG0_BASE);

    /* We'll put the PFN array right after the loaded modules */
    MmPfnDatabase = (PVOID)MiKSeg0End;
    MmPfnDatabaseEnd = (ULONG_PTR)MmPfnDatabase + (MmHighestPhysicalPage * sizeof(PHYSICAL_PAGE));
    MmPfnDatabaseEnd = PAGE_ROUND_UP(MmPfnDatabaseEnd);
    
    /*
     * FreeLDR maps 6MB starting at the kernel base address, followed by the
     * PFN database. If the PFN database doesn't go over the FreeLDR allocation
     * then choose the end of the FreeLDR block. If it does go past the FreeLDR
     * allocation, then choose the next PAGE_SIZE boundary.
     */
    if ((ULONG_PTR)MmPfnDatabaseEnd < (MiKSeg0Start + 0x600000))
    {
        /* Use the first memory following FreeLDR's 6MB mapping */
        MiNonPagedPoolStart = (PVOID)((ULONG_PTR)MiKSeg0Start + 0x600000);
    }
    else
    {
        /* Use the next free available page */
        MiNonPagedPoolStart = (PVOID)MmPfnDatabaseEnd;
    }
    
    /* Length of non-paged pool */
    MiNonPagedPoolLength = MM_NONPAGED_POOL_SIZE;

    /* Put the paged pool after the non-paged pool */
    MmPagedPoolBase = (PVOID)PAGE_ROUND_UP((ULONG_PTR)MiNonPagedPoolStart +
                                           MiNonPagedPoolLength);
    MmPagedPoolSize = MM_PAGED_POOL_SIZE;
    
    /* Dump kernel memory layout */
    MiDbgKernelLayout();
    
    /* Initialize the page list */
    MmInitializePageList();
    
    /* Unmap low memory */
    MmDeletePageTable(NULL, 0);

    /* Intialize memory areas */
    MmInitVirtualMemory();

    /* Initialize MDLs */
    MmInitializeMdlImplementation();
}

BOOLEAN
NTAPI
MmInitSystem(IN ULONG Phase,
             IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Flags = 0;
    if (Phase == 0)
    {
        /* Initialize Mm bootstrap */
        MmInit1();

        /* Initialize the Loader Lock */
        KeInitializeMutant(&MmSystemLoadLock, FALSE);

        /* Initialize the address space for the system process */
        MmInitializeProcessAddressSpace(PsGetCurrentProcess(),
                                        NULL,
                                        NULL,
                                        &Flags,
                                        NULL);

        /* Reload boot drivers */
        MiReloadBootLoadedDrivers(LoaderBlock);

        /* Initialize the loaded module list */
        MiInitializeLoadedModuleList(LoaderBlock);

        /* We're done, for now */
        DPRINT("Mm0: COMPLETE\n");
    }
    else if (Phase == 1)
    {
        MmInitializeRmapList();
        MmInitializePageOp();
        MmInitSectionImplementation();
        MmInitPagingFile();
        MmCreatePhysicalMemorySection();

        /* Setup shared user data settings that NT does as well */
        ASSERT(SharedUserData->NumberOfPhysicalPages == 0);
        SharedUserData->NumberOfPhysicalPages = MmStats.NrTotalPages;
        SharedUserData->LargePageMinimum = 0;

        /* For now, we assume that we're always Workstation */
        SharedUserData->NtProductType = NtProductWinNt;
    }
    else if (Phase == 2)
    {
        /*
        * Unmap low memory
        */
        MiInitBalancerThread();

        /*
        * Initialise the modified page writer.
        */
        MmInitMpwThread();

        /* Initialize the balance set manager */
        MmInitBsmThread();

        /* FIXME: Read parameters from memory */
    }

    return TRUE;
}


/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
MmIsThisAnNtAsSystem(VOID)
{
   return IsThisAnNtAsSystem;
}

/*
 * @implemented
 */
MM_SYSTEMSIZE
NTAPI
MmQuerySystemSize(VOID)
{
   return MmSystemSize;
}
