/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 *
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Jason Filby (jasonfilby@yahoo.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 */


/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#ifdef HALDBG
#include <internal/ntosdbg.h>
#else
#ifdef __GNUC__
#define ps(args...)
#else
#define ps
#endif /* __GNUC__ */
#endif

#if 0
#undef ps
#define ps(args...) DPRINT1(args)
#endif

#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, LdrInit1)
#pragma alloc_text(INIT, LdrInitModuleManagement)
#pragma alloc_text(INIT, LdrSafePEProcessModule)
#endif

/* GLOBALS *******************************************************************/

LIST_ENTRY ModuleListHead;
KSPIN_LOCK ModuleListLock;
LDR_DATA_TABLE_ENTRY NtoskrnlModuleObject;
LDR_DATA_TABLE_ENTRY HalModuleObject;

ULONG_PTR LdrHalBase;

/* FORWARD DECLARATIONS ******************************************************/

NTSTATUS
LdrProcessModule (
    PVOID ModuleLoadBase,
    PUNICODE_STRING ModuleName,
    PLDR_DATA_TABLE_ENTRY *ModuleObject );

static VOID
LdrpBuildModuleBaseName (
    PUNICODE_STRING BaseName,
    PUNICODE_STRING FullName );

static LONG
LdrpCompareModuleNames (
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2 );


/*  PE Driver load support  */
static NTSTATUS
LdrPEProcessModule (
    PVOID ModuleLoadBase,
    PUNICODE_STRING FileName,
    PLDR_DATA_TABLE_ENTRY *ModuleObject );

static PVOID
LdrPEGetExportByName (
    PVOID BaseAddress,
    PUCHAR SymbolName,
    WORD Hint );

static PVOID
LdrPEFixupForward ( PCHAR ForwardName );

static NTSTATUS
LdrPEPerformRelocations (
    PVOID DriverBase,
    ULONG DriverSize );

static NTSTATUS
LdrPEFixupImports ( PLDR_DATA_TABLE_ENTRY Module );

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
LdrInitDebug ( PLOADER_MODULE Module, PWCH Name )
{
}

VOID
INIT_FUNCTION
NTAPI
LdrInit1 ( VOID )
{
    /* Hook for KDB on initialization of the loader. */
    KDB_LOADERINIT_HOOK(&NtoskrnlModuleObject, &HalModuleObject);
}

VOID
INIT_FUNCTION
NTAPI
LdrInitModuleManagement ( VOID )
{
    PIMAGE_NT_HEADERS NtHeader;

    /* Initialize the module list and spinlock */
    InitializeListHead(&ModuleListHead);
    KeInitializeSpinLock(&ModuleListLock);

    /* Initialize ModuleObject for NTOSKRNL */
    RtlZeroMemory(&NtoskrnlModuleObject, sizeof(LDR_DATA_TABLE_ENTRY));
    NtoskrnlModuleObject.DllBase = (PVOID) KERNEL_BASE;
    RtlInitUnicodeString(&NtoskrnlModuleObject.FullDllName, KERNEL_MODULE_NAME);
    LdrpBuildModuleBaseName(&NtoskrnlModuleObject.BaseDllName, &NtoskrnlModuleObject.FullDllName);

    NtHeader = RtlImageNtHeader((PVOID)KERNEL_BASE);
    NtoskrnlModuleObject.EntryPoint = (PVOID) ((ULONG_PTR) NtoskrnlModuleObject.DllBase + NtHeader->OptionalHeader.AddressOfEntryPoint);
    DPRINT("ModuleObject:%08x  entrypoint at %x\n", &NtoskrnlModuleObject, NtoskrnlModuleObject.EntryPoint);
    NtoskrnlModuleObject.SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;

    InsertTailList(&ModuleListHead, &NtoskrnlModuleObject.InLoadOrderModuleList);

    /* Initialize ModuleObject for HAL */
    RtlZeroMemory(&HalModuleObject, sizeof(LDR_DATA_TABLE_ENTRY));
    HalModuleObject.DllBase = (PVOID) LdrHalBase;

    RtlInitUnicodeString(&HalModuleObject.FullDllName, HAL_MODULE_NAME);
    LdrpBuildModuleBaseName(&HalModuleObject.BaseDllName, &HalModuleObject.FullDllName);

    NtHeader = RtlImageNtHeader((PVOID)LdrHalBase);
    HalModuleObject.EntryPoint = (PVOID) ((ULONG_PTR) HalModuleObject.DllBase + NtHeader->OptionalHeader.AddressOfEntryPoint);
    DPRINT("ModuleObject:%08x  entrypoint at %x\n", &HalModuleObject, HalModuleObject.EntryPoint);
    HalModuleObject.SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;

    InsertTailList(&ModuleListHead, &HalModuleObject.InLoadOrderModuleList);
}

NTSTATUS
NTAPI
LdrpLoadImage (
    PUNICODE_STRING DriverName,
    PVOID *ModuleBase,
    PVOID *SectionPointer,
    PVOID *EntryPoint,
    PVOID *ExportSectionPointer )
{
    PLDR_DATA_TABLE_ENTRY ModuleObject;
    NTSTATUS Status;

    ModuleObject = LdrGetModuleObject(DriverName);
    if (ModuleObject == NULL)
    {
        Status = LdrLoadModule(DriverName, &ModuleObject);
        if (!NT_SUCCESS(Status))
        {
            return(Status);
        }
    }

    if (ModuleBase)
        *ModuleBase = ModuleObject->DllBase;

    if (SectionPointer)
        *SectionPointer = ModuleObject;

    if (EntryPoint)
        *EntryPoint = ModuleObject->EntryPoint;

    //if (ExportSectionPointer)
    //    *ExportSectionPointer = ModuleObject->

    return(STATUS_SUCCESS);
}


NTSTATUS
NTAPI
LdrpUnloadImage ( PVOID ModuleBase )
{
    return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
NTAPI
LdrpLoadAndCallImage ( PUNICODE_STRING ModuleName )
{
    PDRIVER_INITIALIZE DriverEntry;
    PLDR_DATA_TABLE_ENTRY ModuleObject;
    DRIVER_OBJECT DriverObject;
    NTSTATUS Status;

    ModuleObject = LdrGetModuleObject(ModuleName);
    if (ModuleObject != NULL)
    {
        return(STATUS_IMAGE_ALREADY_LOADED);
    }

    Status = LdrLoadModule(ModuleName, &ModuleObject);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    DriverEntry = (PDRIVER_INITIALIZE)ModuleObject->EntryPoint;

    RtlZeroMemory(&DriverObject, sizeof(DriverObject));
//    DriverObject.DriverStart = ModuleObject->DllBase;

    Status = DriverEntry(&DriverObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        LdrUnloadModule(ModuleObject);
    }

    return(Status);
}


NTSTATUS
NTAPI
LdrLoadModule(
    PUNICODE_STRING Filename,
    PLDR_DATA_TABLE_ENTRY *ModuleObject )
{
    PVOID ModuleLoadBase;
    NTSTATUS Status;
    HANDLE FileHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PLDR_DATA_TABLE_ENTRY Module;
    FILE_STANDARD_INFORMATION FileStdInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    *ModuleObject = NULL;

    DPRINT("Loading Module %wZ...\n", Filename);

    /*  Open the Module  */
    InitializeObjectAttributes(&ObjectAttributes,
        Filename,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);
    CHECKPOINT;
    Status = ZwOpenFile(&FileHandle,
        GENERIC_READ,
        &ObjectAttributes,
        &IoStatusBlock,
        FILE_SHARE_READ,
        FILE_SYNCHRONOUS_IO_NONALERT);
    CHECKPOINT;
    if (!NT_SUCCESS(Status))
    {
        CPRINT("Could not open module file: %wZ (Status 0x%08lx)\n", Filename, Status);
        return(Status);
    }
    CHECKPOINT;

    /*  Get the size of the file  */
    Status = ZwQueryInformationFile(FileHandle,
        &IoStatusBlock,
        &FileStdInfo,
        sizeof(FileStdInfo),
        FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        CPRINT("Could not get file size\n");
        NtClose(FileHandle);
        return(Status);
    }
    CHECKPOINT;

    /*  Allocate nonpageable memory for driver  */
    ModuleLoadBase = ExAllocatePoolWithTag(NonPagedPool,
        FileStdInfo.EndOfFile.u.LowPart,
        TAG_DRIVER_MEM);
    if (ModuleLoadBase == NULL)
    {
        CPRINT("Could not allocate memory for module");
        NtClose(FileHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    CHECKPOINT;

    /*  Load driver into memory chunk  */
    Status = ZwReadFile(FileHandle,
        0, 0, 0,
        &IoStatusBlock,
        ModuleLoadBase,
        FileStdInfo.EndOfFile.u.LowPart,
        0, 0);
    if (!NT_SUCCESS(Status))
    {
        CPRINT("Could not read module file into memory");
        ExFreePool(ModuleLoadBase);
        NtClose(FileHandle);
        return(Status);
    }
    CHECKPOINT;

    ZwClose(FileHandle);

    Status = LdrProcessModule(ModuleLoadBase,
        Filename,
        &Module);
    if (!NT_SUCCESS(Status))
    {
        CPRINT("Could not process module\n");
        ExFreePool(ModuleLoadBase);
        return(Status);
    }

    /*  Cleanup  */
    ExFreePool(ModuleLoadBase);

    *ModuleObject = Module;

    /* Hook for KDB on loading a driver. */
    KDB_LOADDRIVER_HOOK(Filename, Module);

    return(STATUS_SUCCESS);
}


NTSTATUS
NTAPI
LdrUnloadModule ( PLDR_DATA_TABLE_ENTRY ModuleObject )
{
    KIRQL Irql;

    /* Remove the module from the module list */
    KeAcquireSpinLock(&ModuleListLock,&Irql);
    RemoveEntryList(&ModuleObject->InLoadOrderModuleList);
    KeReleaseSpinLock(&ModuleListLock, Irql);

    /* Hook for KDB on unloading a driver. */
    KDB_UNLOADDRIVER_HOOK(ModuleObject);

    /* Free module section */
    //  MmFreeSection(ModuleObject->DllBase);

    ExFreePool(ModuleObject->FullDllName.Buffer);
    ExFreePool(ModuleObject);

    return(STATUS_SUCCESS);
}


NTSTATUS
LdrProcessModule(
    PVOID ModuleLoadBase,
    PUNICODE_STRING ModuleName,
    PLDR_DATA_TABLE_ENTRY *ModuleObject )
{
    PIMAGE_DOS_HEADER PEDosHeader;

    /*  If MZ header exists  */
    PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
    if (PEDosHeader->e_magic == IMAGE_DOS_SIGNATURE && PEDosHeader->e_lfanew != 0L)
    {
        return LdrPEProcessModule(ModuleLoadBase,
            ModuleName,
            ModuleObject);
    }

    CPRINT("Module wasn't PE\n");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
LdrpQueryModuleInformation (
    PVOID Buffer,
    ULONG Size,
    PULONG ReqSize )
{
    PLIST_ENTRY current_entry;
    PLDR_DATA_TABLE_ENTRY current;
    ULONG ModuleCount = 0;
    PSYSTEM_MODULE_INFORMATION Smi;
    ANSI_STRING AnsiName;
    PCHAR p;
    KIRQL Irql;

    KeAcquireSpinLock(&ModuleListLock,&Irql);

    /* calculate required size */
    current_entry = ModuleListHead.Flink;
    while (current_entry != (&ModuleListHead))
    {
        ModuleCount++;
        current_entry = current_entry->Flink;
    }

    *ReqSize = sizeof(SYSTEM_MODULE_INFORMATION)+
        (ModuleCount - 1) * sizeof(SYSTEM_MODULE_INFORMATION_ENTRY);

    if (Size < *ReqSize)
    {
        KeReleaseSpinLock(&ModuleListLock, Irql);
        return(STATUS_INFO_LENGTH_MISMATCH);
    }

    /* fill the buffer */
    memset(Buffer, '=', Size);

    Smi = (PSYSTEM_MODULE_INFORMATION)Buffer;
    Smi->Count = ModuleCount;

    ModuleCount = 0;
    current_entry = ModuleListHead.Flink;
    while (current_entry != (&ModuleListHead))
    {
        current = CONTAINING_RECORD(current_entry,LDR_DATA_TABLE_ENTRY,InLoadOrderModuleList);

        Smi->Module[ModuleCount].Unknown1 = 0;                /* Always 0 */
        Smi->Module[ModuleCount].Unknown2 = 0;                /* Always 0 */
        Smi->Module[ModuleCount].Base = current->DllBase;
        Smi->Module[ModuleCount].Size = current->SizeOfImage;
        Smi->Module[ModuleCount].Flags = 0;                /* Flags ??? (GN) */
        Smi->Module[ModuleCount].Index = (USHORT)ModuleCount;
        Smi->Module[ModuleCount].NameLength = 0;
        Smi->Module[ModuleCount].LoadCount = 0; /* FIXME */

        AnsiName.Length = 0;
        AnsiName.MaximumLength = 256;
        AnsiName.Buffer = Smi->Module[ModuleCount].ImageName;
        RtlUnicodeStringToAnsiString(&AnsiName,
            &current->FullDllName,
            FALSE);

        p = strrchr(AnsiName.Buffer, '\\');
        if (p == NULL)
        {
            Smi->Module[ModuleCount].PathLength = 0;
        }
        else
        {
            p++;
            Smi->Module[ModuleCount].PathLength = p - AnsiName.Buffer;
        }

        ModuleCount++;
        current_entry = current_entry->Flink;
    }

    KeReleaseSpinLock(&ModuleListLock, Irql);

    return(STATUS_SUCCESS);
}


static VOID
LdrpBuildModuleBaseName (
    PUNICODE_STRING BaseName,
    PUNICODE_STRING FullName )
{
    PWCHAR p;

    DPRINT("LdrpBuildModuleBaseName()\n");
    DPRINT("FullName %wZ\n", FullName);

    p = wcsrchr(FullName->Buffer, L'\\');
    if (p == NULL)
    {
        p = FullName->Buffer;
    }
    else
    {
        p++;
    }

    DPRINT("p %S\n", p);

    RtlInitUnicodeString(BaseName, p);
}


static LONG
LdrpCompareModuleNames (
    IN PUNICODE_STRING String1,
    IN PUNICODE_STRING String2 )
{
    ULONG len1, len2, i;
    PWCHAR s1, s2, p;
    WCHAR  c1, c2;

    if (String1 && String2)
    {
        /* Search String1 for last path component */
        len1 = String1->Length / sizeof(WCHAR);
        s1 = String1->Buffer;
        for (i = 0, p = String1->Buffer; i < String1->Length; i = i + sizeof(WCHAR), p++)
        {
            if (*p == L'\\')
            {
                if (i == String1->Length - sizeof(WCHAR))
                {
                    s1 = NULL;
                    len1 = 0;
                }
                else
                {
                    s1 = p + 1;
                    len1 = (String1->Length - i) / sizeof(WCHAR);
                }
            }
        }

        /* Search String2 for last path component */
        len2 = String2->Length / sizeof(WCHAR);
        s2 = String2->Buffer;
        for (i = 0, p = String2->Buffer; i < String2->Length; i = i + sizeof(WCHAR), p++)
        {
            if (*p == L'\\')
            {
                if (i == String2->Length - sizeof(WCHAR))
                {
                    s2 = NULL;
                    len2 = 0;
                }
                else
                {
                    s2 = p + 1;
                    len2 = (String2->Length - i) / sizeof(WCHAR);
                }
            }
        }

        /* Compare last path components */
        if (s1 && s2)
        {
            while (1)
            {
                c1 = len1-- ? RtlUpcaseUnicodeChar (*s1++) : 0;
                c2 = len2-- ? RtlUpcaseUnicodeChar (*s2++) : 0;
                if ((c1 == 0 && c2 == L'.') || (c1 == L'.' && c2 == 0))
                    return(0);
                if (!c1 || !c2 || c1 != c2)
                    return(c1 - c2);
            }
        }
    }

    return(0);
}

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrGetModuleObject ( PUNICODE_STRING ModuleName )
{
    PLDR_DATA_TABLE_ENTRY Module;
    PLIST_ENTRY Entry;
    KIRQL Irql;

    DPRINT("LdrGetModuleObject(%wZ) called\n", ModuleName);

    KeAcquireSpinLock(&ModuleListLock,&Irql);

    Entry = ModuleListHead.Flink;
    while (Entry != &ModuleListHead)
    {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);

        DPRINT("Comparing %wZ and %wZ\n",
            &Module->BaseDllName,
            ModuleName);

        if (!LdrpCompareModuleNames(&Module->BaseDllName, ModuleName))
        {
            DPRINT("Module %wZ\n", &Module->BaseDllName);
            KeReleaseSpinLock(&ModuleListLock, Irql);
            return(Module);
        }

        Entry = Entry->Flink;
    }

    KeReleaseSpinLock(&ModuleListLock, Irql);

    DPRINT("Could not find module '%wZ'\n", ModuleName);

    return(NULL);
}


/*  ----------------------------------------------  PE Module support */

static ULONG
LdrLookupPageProtection (
    PVOID PageStart,
    PVOID DriverBase,
    PIMAGE_FILE_HEADER PEFileHeader,
    PIMAGE_SECTION_HEADER PESectionHeaders )
{
    BOOLEAN Write = FALSE;
    BOOLEAN Execute = FALSE;
    ULONG Characteristics;
    ULONG Idx;
    ULONG Length;
    PVOID BaseAddress;

    for (Idx = 0; Idx < PEFileHeader->NumberOfSections && (!Write || !Execute); Idx++)
    {
        Characteristics = PESectionHeaders[Idx].Characteristics;
        if (!(Characteristics & IMAGE_SCN_TYPE_NOLOAD))
        {
            Length = max(PESectionHeaders[Idx].Misc.VirtualSize, PESectionHeaders[Idx].SizeOfRawData);
            BaseAddress = PESectionHeaders[Idx].VirtualAddress + (char*)DriverBase;
            if (BaseAddress < (PVOID)((ULONG_PTR)PageStart + PAGE_SIZE) &&
                PageStart < (PVOID)((ULONG_PTR)BaseAddress + Length))
            {
                if (Characteristics & IMAGE_SCN_CNT_CODE)
                {
                    Execute = TRUE;
                }
                if (Characteristics & (IMAGE_SCN_MEM_WRITE|IMAGE_SCN_CNT_UNINITIALIZED_DATA))
                {
                    Write = TRUE;
                }
            }
        }
    }
    if (Write && Execute)
    {
        return PAGE_EXECUTE_READWRITE;
    }
    else if (Execute)
    {
        return PAGE_EXECUTE_READ;
    }
    else if (Write)
    {
        return PAGE_READWRITE;
    }
    else
    {
        return PAGE_READONLY;
    }
}

static NTSTATUS
LdrPEProcessModule(
    PVOID ModuleLoadBase,
    PUNICODE_STRING FileName,
    PLDR_DATA_TABLE_ENTRY *ModuleObject )
{
    unsigned int DriverSize, Idx;
    DWORD CurrentSize;
    PVOID DriverBase;
    PIMAGE_DOS_HEADER PEDosHeader;
    PIMAGE_NT_HEADERS PENtHeaders;
    PIMAGE_SECTION_HEADER PESectionHeaders;
    PLDR_DATA_TABLE_ENTRY CreatedModuleObject;
    NTSTATUS Status;
    KIRQL Irql;

    DPRINT("Processing PE Module at module base:%08lx\n", ModuleLoadBase);

    /*  Get header pointers  */
    PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
    PENtHeaders = RtlImageNtHeader(ModuleLoadBase);
    PESectionHeaders = IMAGE_FIRST_SECTION(PENtHeaders);
    CHECKPOINT;

    /*  Check file magic numbers  */
    if (PEDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        CPRINT("Incorrect MZ magic: %04x\n", PEDosHeader->e_magic);
        return STATUS_UNSUCCESSFUL;
    }
    if (PEDosHeader->e_lfanew == 0)
    {
        CPRINT("Invalid lfanew offset: %08x\n", PEDosHeader->e_lfanew);
        return STATUS_UNSUCCESSFUL;
    }
    if (PENtHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        CPRINT("Incorrect PE magic: %08x\n", PENtHeaders->Signature);
        return STATUS_UNSUCCESSFUL;
    }
    if (PENtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
    {
        CPRINT("Incorrect Architechture: %04x\n", PENtHeaders->FileHeader.Machine);
        return STATUS_UNSUCCESSFUL;
    }
    CHECKPOINT;

    /* FIXME: if image is fixed-address load, then fail  */

    /* FIXME: check/verify OS version number  */

    DPRINT("OptionalHdrMagic:%04x LinkVersion:%d.%d\n",
        PENtHeaders->OptionalHeader.Magic,
        PENtHeaders->OptionalHeader.MajorLinkerVersion,
        PENtHeaders->OptionalHeader.MinorLinkerVersion);
    DPRINT("Entry Point:%08lx\n", PENtHeaders->OptionalHeader.AddressOfEntryPoint);

    /*  Determine the size of the module  */
    DriverSize = 0;
    for (Idx = 0; Idx < PENtHeaders->FileHeader.NumberOfSections; Idx++)
    {
        if (!(PESectionHeaders[Idx].Characteristics & IMAGE_SCN_TYPE_NOLOAD))
        {
            CurrentSize = PESectionHeaders[Idx].VirtualAddress + PESectionHeaders[Idx].Misc.VirtualSize;
            DriverSize = max(DriverSize, CurrentSize);
        }
    }
    DriverSize = ROUND_UP(DriverSize, PENtHeaders->OptionalHeader.SectionAlignment);
    DPRINT("DriverSize %x, SizeOfImage %x\n",DriverSize, PENtHeaders->OptionalHeader.SizeOfImage);

    /*  Allocate a virtual section for the module  */
    DriverBase = NULL;
    DriverBase = MmAllocateSection(DriverSize, DriverBase);
    if (DriverBase == 0)
    {
        CPRINT("Failed to allocate a virtual section for driver\n");
        return STATUS_UNSUCCESSFUL;
    }
    DPRINT("DriverBase for %wZ: %x\n", FileName, DriverBase);

    /*  Copy headers over */
    memcpy(DriverBase, ModuleLoadBase, PENtHeaders->OptionalHeader.SizeOfHeaders);

    /*  Copy image sections into virtual section  */
    for (Idx = 0; Idx < PENtHeaders->FileHeader.NumberOfSections; Idx++)
    {
        CurrentSize = PESectionHeaders[Idx].VirtualAddress + PESectionHeaders[Idx].Misc.VirtualSize;
        /* Copy current section into current offset of virtual section */
        if (CurrentSize <= DriverSize &&
            PESectionHeaders[Idx].SizeOfRawData)
        {
            DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
                PESectionHeaders[Idx].VirtualAddress + (ULONG_PTR)DriverBase);
            memcpy((PVOID)((ULONG_PTR)DriverBase + PESectionHeaders[Idx].VirtualAddress),
                (PVOID)((ULONG_PTR)ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
                PESectionHeaders[Idx].Misc.VirtualSize > PESectionHeaders[Idx].SizeOfRawData
                ? PESectionHeaders[Idx].SizeOfRawData : PESectionHeaders[Idx].Misc.VirtualSize );
        }
    }

    /*  Perform relocation fixups  */
    Status = LdrPEPerformRelocations(DriverBase, DriverSize);
    if (!NT_SUCCESS(Status))
    {
        //   MmFreeSection(DriverBase);
        return Status;
    }

    /* Create the module */
    CreatedModuleObject = ExAllocatePoolWithTag (
        NonPagedPool, sizeof(LDR_DATA_TABLE_ENTRY), TAG_MODULE_OBJECT );
    if (CreatedModuleObject == NULL)
    {
        //   MmFreeSection(DriverBase);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(CreatedModuleObject, sizeof(LDR_DATA_TABLE_ENTRY));

    /*  Initialize ModuleObject data  */
    CreatedModuleObject->DllBase = DriverBase;

    CreatedModuleObject->FullDllName.Length = 0;
    CreatedModuleObject->FullDllName.MaximumLength = FileName->Length + sizeof(UNICODE_NULL);
    CreatedModuleObject->FullDllName.Buffer =
        ExAllocatePoolWithTag(PagedPool, CreatedModuleObject->FullDllName.MaximumLength, TAG_LDR_WSTR);
    if (CreatedModuleObject->FullDllName.Buffer == NULL)
    {
        ExFreePool(CreatedModuleObject);
        //   MmFreeSection(DriverBase);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&CreatedModuleObject->FullDllName, FileName);
    CreatedModuleObject->FullDllName.Buffer[FileName->Length / sizeof(WCHAR)] = 0;
    LdrpBuildModuleBaseName(&CreatedModuleObject->BaseDllName,
        &CreatedModuleObject->FullDllName);

    CreatedModuleObject->EntryPoint =
        (PVOID)((ULONG_PTR)DriverBase +
        PENtHeaders->OptionalHeader.AddressOfEntryPoint);
    CreatedModuleObject->SizeOfImage = DriverSize;
    DPRINT("EntryPoint at %x\n", CreatedModuleObject->EntryPoint);

    /*  Perform import fixups  */
    Status = LdrPEFixupImports(CreatedModuleObject);
    if (!NT_SUCCESS(Status))
    {
        //   MmFreeSection(DriverBase);
        ExFreePool(CreatedModuleObject->FullDllName.Buffer);
        ExFreePool(CreatedModuleObject);
        return Status;
    }

    MmSetPageProtect(NULL, DriverBase, PAGE_READONLY);
    /* Set the protections for the various parts of the driver */
    for (Idx = 0; Idx < PENtHeaders->FileHeader.NumberOfSections; Idx++)
    {
        ULONG Characteristics = PESectionHeaders[Idx].Characteristics;
        ULONG Length;
        PVOID BaseAddress;
        PVOID PageAddress;
        ULONG Protect;
        Length = PESectionHeaders[Idx].Misc.VirtualSize;
        BaseAddress = PESectionHeaders[Idx].VirtualAddress + (char*)DriverBase;
        PageAddress = (PVOID)PAGE_ROUND_DOWN(BaseAddress);

        Protect = LdrLookupPageProtection(PageAddress, DriverBase, &PENtHeaders->FileHeader, PESectionHeaders);
#if 1
        /*
        * FIXME:
        *   This driver modifies a string in the first page of the text section while initialising.
        */
        if (0 == _wcsicmp(L"fireport.sys", FileName->Buffer))
        {
            Protect = PAGE_EXECUTE_READWRITE;
        }
#endif
        if (PageAddress < RVA(DriverBase, DriverSize))
        {
            MmSetPageProtect(NULL, PageAddress, Protect);
        }

        if (Characteristics & IMAGE_SCN_CNT_CODE)
        {
            if (Characteristics & IMAGE_SCN_MEM_WRITE)
            {
                Protect = PAGE_EXECUTE_READWRITE;
            }
            else
            {
                Protect = PAGE_EXECUTE_READ;
            }
        }
        else if (Characteristics & (IMAGE_SCN_MEM_WRITE|IMAGE_SCN_CNT_UNINITIALIZED_DATA))
        {
            Protect = PAGE_READWRITE;
        }
        else
        {
            Protect = PAGE_READONLY;
        }
        PageAddress = (PVOID)((ULONG_PTR)PageAddress + PAGE_SIZE);
        while ((ULONG_PTR)PageAddress + PAGE_SIZE < (ULONG_PTR)BaseAddress + Length)
        {
            if (PageAddress < RVA(DriverBase, DriverSize))
            {
                MmSetPageProtect(NULL, PageAddress, Protect);
            }
            PageAddress = (PVOID)((ULONG_PTR)PageAddress + PAGE_SIZE);
        }
        if (PageAddress < (PVOID)((ULONG_PTR)BaseAddress + Length) &&
            PageAddress < RVA(DriverBase, DriverSize))
        {
            Protect = LdrLookupPageProtection(PageAddress, DriverBase, &PENtHeaders->FileHeader, PESectionHeaders);
            MmSetPageProtect(NULL, PageAddress, Protect);
        }
    }

    /* Insert module */
    KeAcquireSpinLock(&ModuleListLock, &Irql);
    InsertTailList(&ModuleListHead,
        &CreatedModuleObject->InLoadOrderModuleList);
    KeReleaseSpinLock(&ModuleListLock, Irql);

    *ModuleObject = CreatedModuleObject;

    DPRINT("Loading Module %wZ...\n", FileName);

    DPRINT("Module %wZ loaded at 0x%.08x.\n",
            FileName, CreatedModuleObject->DllBase);

    return STATUS_SUCCESS;
}


PVOID
INIT_FUNCTION
NTAPI
LdrSafePEProcessModule (
    PVOID ModuleLoadBase,
    PVOID DriverBase,
    PVOID ImportModuleBase,
    PULONG DriverSize)
{
    unsigned int Idx;
    ULONG CurrentSize;
    PIMAGE_DOS_HEADER PEDosHeader;
    PIMAGE_NT_HEADERS PENtHeaders;
    PIMAGE_SECTION_HEADER PESectionHeaders;
    NTSTATUS Status;

    ps("Processing PE Module at module base:%08lx\n", ModuleLoadBase);

    /*  Get header pointers  */
    PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
    PENtHeaders = RtlImageNtHeader(ModuleLoadBase);
    PESectionHeaders = IMAGE_FIRST_SECTION(PENtHeaders);
    CHECKPOINT;

    /*  Check file magic numbers  */
    if (PEDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return NULL;
    }
    if (PEDosHeader->e_lfanew == 0)
    {
        return NULL;
    }
    if (PENtHeaders->Signature != IMAGE_NT_SIGNATURE)
    {
        return NULL;
    }
    if (PENtHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
    {
        return NULL;
    }

    ps("OptionalHdrMagic:%04x LinkVersion:%d.%d\n",
        PENtHeaders->OptionalHeader.Magic,
        PENtHeaders->OptionalHeader.MajorLinkerVersion,
        PENtHeaders->OptionalHeader.MinorLinkerVersion);
    ps("Entry Point:%08lx\n", PENtHeaders->OptionalHeader.AddressOfEntryPoint);

    /*  Determine the size of the module  */
    *DriverSize = PENtHeaders->OptionalHeader.SizeOfImage;
    ps("DriverSize %x\n",*DriverSize);

    /*  Copy headers over */
    if (DriverBase != ModuleLoadBase)
    {
        memcpy(DriverBase, ModuleLoadBase, PENtHeaders->OptionalHeader.SizeOfHeaders);
    }

    ps("Hdr: 0x%X\n", PENtHeaders);
    ps("Hdr->SizeOfHeaders: 0x%X\n", PENtHeaders->OptionalHeader.SizeOfHeaders);
    ps("FileHdr->NumberOfSections: 0x%X\n", PENtHeaders->FileHeader.NumberOfSections);

    /* Ntoskrnl.exe need no relocation fixups since it is linked to run at the same
    address as it is mapped */
    if (DriverBase != ModuleLoadBase)
    {
        CurrentSize = 0;

        /*  Copy image sections into virtual section  */
        for (Idx = 0; Idx < PENtHeaders->FileHeader.NumberOfSections; Idx++)
        {
            PIMAGE_SECTION_HEADER Section = &PESectionHeaders[Idx];
            //  Copy current section into current offset of virtual section
            if (Section->SizeOfRawData)
            {
                //            ps("PESectionHeaders[Idx].VirtualAddress (%X) + DriverBase %x\n",
                //                PESectionHeaders[Idx].VirtualAddress, PESectionHeaders[Idx].VirtualAddress + DriverBase);
                memcpy(Section->VirtualAddress   + (char*)DriverBase,
                    Section->PointerToRawData + (char*)ModuleLoadBase,
                    Section->Misc.VirtualSize > Section->SizeOfRawData ? Section->SizeOfRawData : Section->Misc.VirtualSize);
            }
            if (Section->SizeOfRawData < Section->Misc.VirtualSize)
            {
                memset(Section->VirtualAddress + Section->SizeOfRawData + (char*)DriverBase,
                    0,
                    Section->Misc.VirtualSize - Section->SizeOfRawData);
            }
            CurrentSize += ROUND_UP(Section->Misc.VirtualSize,
                PENtHeaders->OptionalHeader.SectionAlignment);
        }

        /*  Perform relocation fixups  */
        Status = LdrPEPerformRelocations(DriverBase, *DriverSize);
        if (!NT_SUCCESS(Status))
        {
            return NULL;
        }
    }

    /*  Perform import fixups  */
    Status = LdrPEFixupImports(DriverBase == ModuleLoadBase ? &NtoskrnlModuleObject : &HalModuleObject);
    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    /*  Set the page protection for the virtual sections */
    MmSetPageProtect(NULL, DriverBase, PAGE_READONLY);
    for (Idx = 0; Idx < PENtHeaders->FileHeader.NumberOfSections; Idx++)
    {
        ULONG Characteristics = PESectionHeaders[Idx].Characteristics;
        ULONG Length;
        PVOID BaseAddress;
        PVOID PageAddress;
        ULONG Protect;
        Length = PESectionHeaders[Idx].Misc.VirtualSize;
        BaseAddress = PESectionHeaders[Idx].VirtualAddress + (char*)DriverBase;
        PageAddress = (PVOID)PAGE_ROUND_DOWN(BaseAddress);

        if (Characteristics & IMAGE_SCN_MEM_EXECUTE)
        {
            if (Characteristics & IMAGE_SCN_MEM_WRITE)
            {
                Protect = PAGE_EXECUTE_READWRITE;
            }
            else
            {
                Protect = PAGE_EXECUTE_READ;
            }
        }
        else if (Characteristics & IMAGE_SCN_MEM_WRITE)
        {
            Protect = PAGE_READWRITE;
        }
        else
        {
            Protect = PAGE_READONLY;
        }
        while ((ULONG_PTR)PageAddress < (ULONG_PTR)BaseAddress + Length)
        {
            MmSetPageProtect(NULL, PageAddress, Protect);
            PageAddress = (PVOID)((ULONG_PTR)PageAddress + PAGE_SIZE);
        }
        if (DriverBase == ModuleLoadBase &&
            Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        {
            /* For ntoskrnl, we must stop after the bss section */
            break;
        }

    }

    return DriverBase;
}

static PVOID
LdrPEFixupForward ( PCHAR ForwardName )
{
    CHAR NameBuffer[128];
    UNICODE_STRING ModuleName;
    PCHAR p;
    PLDR_DATA_TABLE_ENTRY ModuleObject;

    DPRINT("LdrPEFixupForward (%s)\n", ForwardName);

    strcpy(NameBuffer, ForwardName);
    p = strchr(NameBuffer, '.');
    if (p == NULL)
    {
        return NULL;
    }

    *p = 0;

    DPRINT("Driver: %s  Function: %s\n", NameBuffer, p+1);

    RtlCreateUnicodeStringFromAsciiz(&ModuleName,
        NameBuffer);
    ModuleObject = LdrGetModuleObject(&ModuleName);
    RtlFreeUnicodeString(&ModuleName);

    DPRINT("ModuleObject: %p\n", ModuleObject);

    if (ModuleObject == NULL)
    {
        CPRINT("LdrPEFixupForward: failed to find module %s\n", NameBuffer);
        return NULL;
    }
    return LdrPEGetExportByName(ModuleObject->DllBase, (PUCHAR)(p+1), 0xffff);
}

static NTSTATUS
LdrPEPerformRelocations (
    PVOID DriverBase,
    ULONG DriverSize)
{
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_DATA_DIRECTORY RelocationDDir;
    PIMAGE_BASE_RELOCATION RelocationDir, RelocationEnd;
    ULONG Count, i;
    PVOID Address, MaxAddress;
    PUSHORT TypeOffset;
    ULONG_PTR Delta;
    SHORT Offset;
    USHORT Type;
    PUSHORT ShortPtr;
    PULONG LongPtr;

    NtHeaders = RtlImageNtHeader(DriverBase);

    if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    {
        return STATUS_UNSUCCESSFUL;
    }

    RelocationDDir = &NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (RelocationDDir->VirtualAddress == 0 || RelocationDDir->Size == 0)
    {
        return STATUS_SUCCESS;
    }

    Delta = (ULONG_PTR)DriverBase - NtHeaders->OptionalHeader.ImageBase;
    RelocationDir = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)DriverBase + RelocationDDir->VirtualAddress);
    RelocationEnd = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)RelocationDir + RelocationDDir->Size);
    MaxAddress = RVA(DriverBase, DriverSize);

    while (RelocationDir < RelocationEnd &&
        RelocationDir->SizeOfBlock > 0)
    {
        Count = (RelocationDir->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
        Address = RVA(DriverBase, RelocationDir->VirtualAddress);
        TypeOffset = (PUSHORT)(RelocationDir + 1);

        for (i = 0; i < Count; i++)
        {
            Offset = *TypeOffset & 0xFFF;
            Type = *TypeOffset >> 12;
            ShortPtr = (PUSHORT)(RVA(Address, Offset));

            /* Don't relocate after the end of the loaded driver */
            if ((PVOID)ShortPtr >= MaxAddress)
            {
                break;
            }

            /*
            * Don't relocate within the relocation section itself.
            * GCC/LD generates sometimes relocation records for the relocation section.
            * This is a bug in GCC/LD.
            */
            if ((ULONG_PTR)ShortPtr < (ULONG_PTR)RelocationDir ||
                (ULONG_PTR)ShortPtr >= (ULONG_PTR)RelocationEnd)
            {
                switch (Type)
                {
                case IMAGE_REL_BASED_ABSOLUTE:
                    break;

                case IMAGE_REL_BASED_HIGH:
                    *ShortPtr += HIWORD(Delta);
                    break;

                case IMAGE_REL_BASED_LOW:
                    *ShortPtr += LOWORD(Delta);
                    break;

                case IMAGE_REL_BASED_HIGHLOW:
                    LongPtr = (PULONG)ShortPtr;
                    *LongPtr += Delta;
                    break;

                case IMAGE_REL_BASED_HIGHADJ:
                case IMAGE_REL_BASED_MIPS_JMPADDR:
                default:
                    DPRINT1("Unknown/unsupported fixup type %hu.\n", Type);
                    DPRINT1("Address %x, Current %d, Count %d, *TypeOffset %x\n", Address, i, Count, *TypeOffset);
                    return STATUS_UNSUCCESSFUL;
                }
            }
            TypeOffset++;
        }
        RelocationDir = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)RelocationDir + RelocationDir->SizeOfBlock);
    }

    return STATUS_SUCCESS;
}
#ifndef PATH_MAX
#define PATH_MAX 260
#endif

static NTSTATUS
LdrPEGetOrLoadModule (
    PLDR_DATA_TABLE_ENTRY Module,
    PCHAR ImportedName,
    PLDR_DATA_TABLE_ENTRY* ImportedModule)
{
    UNICODE_STRING DriverName;
    UNICODE_STRING NameString;
    WCHAR  NameBuffer[PATH_MAX];
    NTSTATUS Status = STATUS_SUCCESS;

    if (0 == _stricmp(ImportedName, "ntoskrnl") ||
        0 == _stricmp(ImportedName, "ntoskrnl.exe"))
    {
        *ImportedModule = &NtoskrnlModuleObject;
        return STATUS_SUCCESS;
    }

    if (0 == _stricmp(ImportedName, "hal") ||
        0 == _stricmp(ImportedName, "hal.dll"))
    {
        *ImportedModule = &HalModuleObject;
        return STATUS_SUCCESS;
    }

    RtlCreateUnicodeStringFromAsciiz (&DriverName, ImportedName);
    DPRINT("Import module: %wZ\n", &DriverName);

    *ImportedModule = LdrGetModuleObject(&DriverName);
    if (*ImportedModule == NULL)
    {
        PWCHAR PathEnd;
        ULONG PathLength;

        PathEnd = wcsrchr(Module->FullDllName.Buffer, L'\\');
        if (NULL != PathEnd)
        {
            PathLength = (PathEnd - Module->FullDllName.Buffer + 1) * sizeof(WCHAR);
            RtlCopyMemory(NameBuffer, Module->FullDllName.Buffer, PathLength);
            RtlCopyMemory(NameBuffer + (PathLength / sizeof(WCHAR)), DriverName.Buffer, DriverName.Length);
            NameString.Buffer = NameBuffer;
            NameString.MaximumLength = NameString.Length = PathLength + DriverName.Length;

            /* NULL-terminate */
            NameString.MaximumLength++;
            NameBuffer[NameString.Length / sizeof(WCHAR)] = 0;

            Status = LdrLoadModule(&NameString, ImportedModule);
        }
        else
        {
            DPRINT("Module '%wZ' not loaded yet\n", &DriverName);
            wcscpy(NameBuffer, L"\\SystemRoot\\system32\\drivers\\");
            wcsncat(NameBuffer, DriverName.Buffer, DriverName.Length / sizeof(WCHAR));
            RtlInitUnicodeString(&NameString, NameBuffer);
            Status = LdrLoadModule(&NameString, ImportedModule);
        }
        if (!NT_SUCCESS(Status))
        {
            wcscpy(NameBuffer, L"\\SystemRoot\\system32\\");
            wcsncat(NameBuffer, DriverName.Buffer, DriverName.Length / sizeof(WCHAR));
            RtlInitUnicodeString(&NameString, NameBuffer);
            Status = LdrLoadModule(&NameString, ImportedModule);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Unknown import module: %wZ (Status %lx)\n", &DriverName, Status);
            }
        }
    }
    RtlFreeUnicodeString(&DriverName);
    return Status;
}

static PVOID
LdrPEGetExportByName (
    PVOID BaseAddress,
    PUCHAR SymbolName,
    WORD Hint )
{
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    PDWORD * ExFunctions;
    PDWORD * ExNames;
    USHORT * ExOrdinals;
    PVOID ExName;
    ULONG Ordinal;
    PVOID Function;
    LONG minn, maxn, mid, res;
    ULONG ExportDirSize;

    DPRINT("LdrPEGetExportByName %x %s %hu\n", BaseAddress, SymbolName, Hint);

    ExportDir = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(BaseAddress,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &ExportDirSize);
    if (ExportDir == NULL)
    {
        DPRINT1("LdrPEGetExportByName(): no export directory!\n");
        return NULL;
    }


    /* The symbol names may be missing entirely */
    if (ExportDir->AddressOfNames == 0)
    {
        DPRINT("LdrPEGetExportByName(): symbol names missing entirely\n");
        return NULL;
    }

    /*
    * Get header pointers
    */
    ExNames = (PDWORD *)RVA(BaseAddress, ExportDir->AddressOfNames);
    ExOrdinals = (USHORT *)RVA(BaseAddress, ExportDir->AddressOfNameOrdinals);
    ExFunctions = (PDWORD *)RVA(BaseAddress, ExportDir->AddressOfFunctions);

    /*
    * Check the hint first
    */
    if (Hint < ExportDir->NumberOfNames)
    {
        ExName = RVA(BaseAddress, ExNames[Hint]);
        if (strcmp(ExName, (PCHAR)SymbolName) == 0)
        {
            Ordinal = ExOrdinals[Hint];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                DPRINT("Forward: %s\n", (PCHAR)Function);
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DPRINT1("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
                }
                return Function;
            }
            if (Function != NULL)
            {
                return Function;
            }
        }
    }

    /*
    * Binary search
    */
    minn = 0;
    maxn = ExportDir->NumberOfNames - 1;
    while (minn <= maxn)
    {
        mid = (minn + maxn) / 2;

        ExName = RVA(BaseAddress, ExNames[mid]);
        res = strcmp(ExName, (PCHAR)SymbolName);
        if (res == 0)
        {
            Ordinal = ExOrdinals[mid];
            Function = RVA(BaseAddress, ExFunctions[Ordinal]);
            if ((ULONG_PTR)Function >= (ULONG_PTR)ExportDir &&
                (ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize)
            {
                DPRINT("Forward: %s\n", (PCHAR)Function);
                Function = LdrPEFixupForward((PCHAR)Function);
                if (Function == NULL)
                {
                    DPRINT1("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
                }
                return Function;
            }
            if (Function != NULL)
            {
                return Function;
            }
        }
        else if (res > 0)
        {
            maxn = mid - 1;
        }
        else
        {
            minn = mid + 1;
        }
    }

    ExName = RVA(BaseAddress, ExNames[mid]);
    DPRINT1("LdrPEGetExportByName(): failed to find %s\n",SymbolName);
    return (PVOID)NULL;
}

static PVOID
LdrPEGetExportByOrdinal (
    PVOID BaseAddress,
    ULONG Ordinal )
{
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    ULONG ExportDirSize;
    PDWORD * ExFunctions;
    PVOID Function;

    ExportDir = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData (
        BaseAddress,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &ExportDirSize);

    ExFunctions = (PDWORD *)RVA(BaseAddress,
        ExportDir->AddressOfFunctions);
    DPRINT("LdrPEGetExportByOrdinal(Ordinal %d) = %x\n",
        Ordinal,
        RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base]));

    Function = 0 != ExFunctions[Ordinal - ExportDir->Base]
        ? RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base] )
        : NULL;

    if (((ULONG_PTR)Function >= (ULONG_PTR)ExportDir) &&
        ((ULONG_PTR)Function < (ULONG_PTR)ExportDir + ExportDirSize))
    {
        DPRINT("Forward: %s\n", (PCHAR)Function);
        Function = LdrPEFixupForward((PCHAR)Function);
    }

    return Function;
}

static NTSTATUS
LdrPEProcessImportDirectoryEntry(
    PVOID DriverBase,
    PLDR_DATA_TABLE_ENTRY ImportedModule,
    PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory )
{
    PVOID* ImportAddressList;
    PULONG FunctionNameList;
    ULONG Ordinal;

    if (ImportModuleDirectory == NULL || ImportModuleDirectory->Name == 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the import address list. */
    ImportAddressList = (PVOID*)RVA(DriverBase, ImportModuleDirectory->FirstThunk);

    /* Get the list of functions to import. */
    if (ImportModuleDirectory->OriginalFirstThunk != 0)
    {
        FunctionNameList = (PULONG)RVA(DriverBase, ImportModuleDirectory->OriginalFirstThunk);
    }
    else
    {
        FunctionNameList = (PULONG)RVA(DriverBase, ImportModuleDirectory->FirstThunk);
    }

    /* Walk through function list and fixup addresses. */
    while (*FunctionNameList != 0L)
    {
        if ((*FunctionNameList) & 0x80000000)
        {
            Ordinal = (*FunctionNameList) & 0x7fffffff;
            *ImportAddressList = LdrPEGetExportByOrdinal(ImportedModule->DllBase, Ordinal);
            if ((*ImportAddressList) == NULL)
            {
                DPRINT1("Failed to import #%ld from %wZ\n", Ordinal, &ImportedModule->FullDllName);
                return STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            IMAGE_IMPORT_BY_NAME *pe_name;
            pe_name = RVA(DriverBase, *FunctionNameList);
            *ImportAddressList = LdrPEGetExportByName(ImportedModule->DllBase, pe_name->Name, pe_name->Hint);
            if ((*ImportAddressList) == NULL)
            {
                DPRINT1("Failed to import %s from %wZ\n", pe_name->Name, &ImportedModule->FullDllName);
                return STATUS_UNSUCCESSFUL;
            }
        }
        ImportAddressList++;
        FunctionNameList++;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
LdrPEFixupImports ( PLDR_DATA_TABLE_ENTRY Module )
{
    PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
    PCHAR ImportedName;
    PLDR_DATA_TABLE_ENTRY ImportedModule;
    NTSTATUS Status;
    ULONG Size;

    /*  Process each import module  */
    ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
        RtlImageDirectoryEntryToData(Module->DllBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_IMPORT,
        &Size);
    DPRINT("Processeing import directory at %p\n", ImportModuleDirectory);
    while (ImportModuleDirectory->Name)
    {
        if (Module->SizeOfImage <= ImportModuleDirectory->Name)
        {
            DPRINT1("Invalid import directory in %wZ\n", &Module->FullDllName);
            return STATUS_SECTION_NOT_IMAGE;
        }

        /*  Check to make sure that import lib is kernel  */
        ImportedName = (PCHAR) Module->DllBase + ImportModuleDirectory->Name;

        Status = LdrPEGetOrLoadModule(Module, ImportedName, &ImportedModule);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        Status = LdrPEProcessImportDirectoryEntry(Module->DllBase, ImportedModule, ImportModuleDirectory);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        ImportModuleDirectory++;
    }
    return STATUS_SUCCESS;
}

/* EOF */
