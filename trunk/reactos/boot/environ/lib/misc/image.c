/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/misc/image.c
 * PURPOSE:         Boot Library Image Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"
#include <bcd.h>

/* DATA VARIABLES ************************************************************/

ULONG IapAllocatedTableEntries;
ULONG IapTableEntries;
PVOID* IapImageTable;

/* FUNCTIONS *****************************************************************/

NTSTATUS
ImgpGetFileSize (
    _In_ PBL_IMG_FILE File,
    _Out_ PULONG FileSize
    )
{
    NTSTATUS Status;
    ULONG Size;
    BL_FILE_INFORMATION FileInformation;

    /* Check if the file was memory mapped */
    if (File->Flags & BL_IMG_MEMORY_FILE)
    {
        /* Just read the size of the mapping */
        Size = File->FileSize;
    }
    else
    {
        /* Do file I/O to get the file size */
        Status = BlFileGetInformation(File->FileId,
                                      &FileInformation);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* We only support files less than 4GB in the Image Mapped */
        Size = FileInformation.Size;
        if (FileInformation.Size > ULONG_MAX)
        {
            return STATUS_NOT_SUPPORTED;
        }
    }

    /* Return the size and success */
    *FileSize = Size;
    return STATUS_SUCCESS;
}

NTSTATUS
ImgpReadAtFileOffset (
    _In_ PBL_IMG_FILE File,
    _In_ ULONG Size,
    _In_ ULONGLONG ByteOffset,
    _In_ PVOID Buffer,
    _Out_ PULONG BytesReturned
    )
{
    NTSTATUS Status;

    /* Check what if this is a mapped file or not */
    if (File->Flags & BL_IMG_MEMORY_FILE)
    {
        /* Check if the boundaries are within the file size */
        if ((ByteOffset + Size) <= File->FileSize)
        {
            /* Yep, copy into the caller-supplied buffer */
            RtlCopyMemory(Buffer,
                          (PVOID)((ULONG_PTR)File->BaseAddress + (ULONG_PTR)ByteOffset),
                          Size);

            /* If caller wanted to know, return the size copied */
            if (BytesReturned)
            {
                *BytesReturned = Size;
            }

            /* All good */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Doesn't fit */
            Status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* Issue the file I/O instead */
        Status = BlFileReadAtOffsetEx(File->FileId,
                                      Size,
                                      ByteOffset,
                                      Buffer,
                                      BytesReturned,
                                      0);
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
ImgpOpenFile (
    _In_ ULONG DeviceId,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags,
    _Out_ PBL_IMG_FILE NewFile
    )
{
    NTSTATUS Status;
    ULONG FileSize;
    ULONGLONG RemoteFileSize;
    PVOID RemoteFileAddress;
    ULONG FileId;

    /* First, try to see if BD has this file remotely */
    Status = BlBdPullRemoteFile(FileName,
                                &RemoteFileAddress,
                                &RemoteFileSize);
    if (NT_SUCCESS(Status))
    {
        /* Yep, get the file size and make sure it's < 4GB */
        FileSize = RemoteFileSize;
        if (RemoteFileSize <= ULONG_MAX)
        {
            /* Remember this is a memory mapped remote file */
            NewFile->Flags |= (BL_IMG_MEMORY_FILE | BL_IMG_REMOTE_FILE);
            NewFile->FileSize = FileSize;
            NewFile->BaseAddress = RemoteFileAddress;
            goto Quickie;
        }
    }

    /* Use File I/O instead */
    Status = BlFileOpen(DeviceId,
                        FileName,
                        BL_FILE_READ_ACCESS,
                        &FileId);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on failure */
        return Status;
    }

    /* Make sure nobody thinks this is a memory file */
    NewFile->Flags &= ~BL_IMG_MEMORY_FILE;
    NewFile->FileId = FileId;

Quickie:
    /* Set common data for both memory and I/O based file */
    NewFile->Flags |= BL_IMG_VALID_FILE;
    NewFile->FileName = FileName;
    return Status;
}

NTSTATUS
ImgpCloseFile (
    _In_ PBL_IMG_FILE File
    )
{
    NTSTATUS Status;

    /* Make sure this is a valid file, otherwise no-op */
    Status = STATUS_SUCCESS;
    if (File->Flags & BL_IMG_VALID_FILE)
    {
        /* Is this a memory mapped file? */
        if (!(File->Flags & BL_IMG_MEMORY_FILE))
        {
            /* Nope, close the file handle */
            return BlFileClose(File->FileId);
        }

        /* Is this a remote file? */
        if (File->Flags & BL_IMG_REMOTE_FILE)
        {
            /* Then only free the memory in that scenario */
            EfiPrintf(L"TODO\r\n");
            //return MmPapFreePages(File->BaseAddress, TRUE);
        }
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
BlImgUnallocateImageBuffer (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ ULONG ImageFlags
    )
{
    EfiPrintf(L"leaking the shit out of %p\r\n", ImageBase);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlImgAllocateImageBuffer (
    _Inout_ PVOID* ImageBuffer,
    _In_ ULONG MemoryType,
    _In_ ULONGLONG ImageSize,
    _In_ ULONG Flags
    )
{
    ULONG Attributes;
    ULONGLONG Pages, Size;
    PVOID MappedBase, CurrentBuffer;
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Read and reset the current buffer address */
    CurrentBuffer = *ImageBuffer;
    *ImageBuffer = NULL;

    /* Align the image size to page */
    Size = ROUND_TO_PAGES(ImageSize);

    /* Not sure what this attribute does yet */
    Attributes = 0;
    if (Flags & BL_LOAD_IMG_UNKNOWN_BUFFER_FLAG)
    {
        Attributes = 0x10000;
    }

    /* Check if the caller wants a virtual buffer */
    if (Flags & BL_LOAD_IMG_VIRTUAL_BUFFER)
    {
        /* Set the physical address to the current buffer */
        PhysicalAddress.QuadPart = (ULONG_PTR)CurrentBuffer;
        Pages = Size >> PAGE_SHIFT;

        /* Allocate the physical pages */
        Status = BlMmAllocatePhysicalPages(&PhysicalAddress,
                                           Pages,
                                           MemoryType,
                                           Attributes,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            /* If that failed, remove allocation attributes */
            PhysicalAddress.QuadPart = 0;
            Attributes &= ~BlMemoryValidAllocationAttributeMask,
            Status = BlMmAllocatePhysicalPages(&PhysicalAddress,
                                               Pages,
                                               MemoryType,
                                               Attributes,
                                               0);
        }

        /* Check if either attempts succeeded */
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Now map the physical buffer at the address requested */
        MappedBase = (PVOID)PhysicalAddress.LowPart;
        Status = BlMmMapPhysicalAddressEx(&MappedBase,
                                          BlMemoryFixed,
                                          Size,
                                          PhysicalAddress);
        if (!NT_SUCCESS(Status))
        {
            /* Free on failure if needed */
            BlMmFreePhysicalPages(PhysicalAddress);
            return Status;
        }
    }
    else
    {
        /* Otherwise, allocate raw physical pages */
        MappedBase = CurrentBuffer;
        Pages = Size >> PAGE_SHIFT;
        Status = MmPapAllocatePagesInRange(&MappedBase,
                                           MemoryType,
                                           Pages,
                                           Attributes,
                                           0,
                                           NULL,
                                           0);
        if (!NT_SUCCESS(Status))
        {
            /* If that failed, try without allocation attributes */
            MappedBase = NULL;
            Attributes &= ~BlMemoryValidAllocationAttributeMask,
            Status = MmPapAllocatePagesInRange(&MappedBase,
                                               MemoryType,
                                               Pages,
                                               Attributes,
                                               0,
                                               NULL,
                                               0);
        }

        /* Check if either attempts succeeded */
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Success path, returned allocated address */
    *ImageBuffer = MappedBase;
    return STATUS_SUCCESS;
}

NTSTATUS
BlImgLoadImageWithProgress2 (
    _In_ ULONG DeviceId,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ PWCHAR FileName,
    _Inout_ PVOID* MappedBase,
    _Inout_ PULONG MappedSize,
    _In_ ULONG ImageFlags,
    _In_ BOOLEAN ShowProgress,
    _Out_opt_ PUCHAR* HashBuffer,
    _Out_opt_ PULONG HashSize
    )
{
    NTSTATUS Status;
    PVOID BaseAddress, Buffer;
    ULONG RemainingLength, CurrentSize, ImageSize, ReadSize;
    BOOLEAN ComputeSignature, ComputeHash, Completed;
    BL_IMG_FILE FileHandle;
    ULONGLONG ByteOffset;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Initialize variables */
    BaseAddress = 0;
    ImageSize = 0;
    Completed = FALSE;
    RtlZeroMemory(&FileHandle, sizeof(FileHandle));

    /* Check for missing parameters */
    if (!MappedBase)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }
    if (!FileName)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }
    if (!MappedSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Check if the image buffer is being provided */
    if (ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER)
    {
        /* An existing base must already exist */
        if (!(*MappedBase))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
    }

    /* Check of a hash is being requested */
    if (ImageFlags & BL_LOAD_IMG_COMPUTE_HASH)
    {
        /* Make sure we can return the hash */
        if (!HashBuffer)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
        if (!HashSize)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
    }

    /* Check for invalid combination of parameters */
    if ((ImageFlags & BL_LOAD_IMG_COMPUTE_HASH) && (ImageFlags & 0x270))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Initialize hash if requested by caller */
    if (HashBuffer)
    {
        *HashBuffer = 0;
    }

    /* Do the same for the hash size */
    if (HashSize)
    {
        *HashSize = 0;
    }

    /* Open the image file */
    Status = ImgpOpenFile(DeviceId, FileName, DeviceId, &FileHandle);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Error opening file: %lx\r\n", Status);
        goto Quickie;
    }

    /* Get the size of the image */
    Status = ImgpGetFileSize(&FileHandle, &ImageSize);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Error getting file size: %lx\r\n", Status);
        goto Quickie;
    }

    /* Read the current base address */
    BaseAddress = *MappedBase;
    if (ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER)
    {
        /* Check if the current buffer is too small */
        if (*MappedSize < ImageSize)
        {
            /* Return the required size of the buffer */
            *MappedSize = ImageSize;
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    else
    {
        /* A buffer was not provided, allocate one ourselves */
        Status = BlImgAllocateImageBuffer(&BaseAddress,
                                          MemoryType,
                                          ImageSize,
                                          ImageFlags);
    }

    /* Bail out if allocation failed */
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    /* Set the initial byte offset and length to read */
    RemainingLength = ImageSize;
    ByteOffset = 0;
    Buffer = BaseAddress;

    /* Update the initial progress */
    Completed = FALSE;
    if (ShowProgress)
    {
        BlUtlUpdateProgress(0, &Completed);
        ShowProgress &= (Completed != 0) - 1;
    }

    /* Set the chunk size for each read */
    ReadSize = 0x100000;
    if (ReadSize > ImageSize)
    {
        ReadSize = ImageSize;
    }

    /* Check if we should compute hash and/or signatures */
    ComputeSignature = ImageFlags & BL_LOAD_IMG_COMPUTE_SIGNATURE;
    if ((ComputeSignature) || (ImageFlags & BL_LOAD_IMG_COMPUTE_HASH))
    {
        ComputeHash = TRUE;
        // todo: crypto is hard
    }

    /* Begin the read loop */
    while (RemainingLength)
    {
        /* Check if we've got more than a chunk left to read */
        if (RemainingLength > ReadSize)
        {
            /* Read a chunk*/
            CurrentSize = ReadSize;
        }
        else
        {
            /* Read only what's left */
            CurrentSize = RemainingLength;
        }

        /* Read the chunk */
        Status = ImgpReadAtFileOffset(&FileHandle,
                                      CurrentSize,
                                      ByteOffset,
                                      Buffer,
                                      0);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }

        /* Check if we need to compute the hash of this chunk */
        if (ComputeHash)
        {
            // todo: crypto is hard
        }

        /* Update our position and read information */
        Buffer = (PVOID)((ULONG_PTR)Buffer + CurrentSize);
        RemainingLength -= CurrentSize;
        ByteOffset += CurrentSize;

        /* Check if we should update the progress bar */
        if (ShowProgress)
        {
            /* Compute new percentage completed, check if we're done */
            BlUtlUpdateProgress(100 - 100 * RemainingLength / ImageSize,
                                &Completed);
            ShowProgress &= (Completed != 0) - 1;
        }
    }

    /* Is the read fully complete? We need to finalize the hash if requested */
    if (ComputeHash != RemainingLength)
    {
        // todo: CRYPTO IS HARD
    }

    /* Success path, return back the buffer and the size of the image */
    *MappedBase = BaseAddress;
    *MappedSize = ImageSize;

Quickie:
    /* Close the file handle */
    ImgpCloseFile(&FileHandle);

    /* Check if we failed and had allocated a buffer */
    if (!(NT_SUCCESS(Status)) &&
        (BaseAddress) &&
        !(ImageFlags & BL_LOAD_IMG_EXISTING_BUFFER))
    {
        /* Check what kind of buffer we had allocated */
        if (ImageFlags & BL_LOAD_IMG_VIRTUAL_BUFFER)
        {
            /* Unmap and free the virtual buffer */
            PhysicalAddress.QuadPart = (ULONG_PTR)BaseAddress;
            BlMmUnmapVirtualAddressEx(BaseAddress, ImageSize);
            BlMmFreePhysicalPages(PhysicalAddress);
        }
        else
        {
            /* Free the physical buffer */
            //MmPapFreePages(VirtualAddress, 1);
            EfiPrintf(L"Leaking memory\r\n");
        }
    }

    /* If we hadn't gotten to 100% yet, do it now */
    if (ShowProgress)
    {
        BlUtlUpdateProgress(100, &Completed);
    }

    /* Return the final status */
    return Status;
}

PIMAGE_SECTION_HEADER 
BlImgFindSection (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize
    )
{
    PIMAGE_SECTION_HEADER FoundSection;
    ULONG i;
    PIMAGE_SECTION_HEADER SectionHeader;
    PIMAGE_NT_HEADERS NtHeader;
    NTSTATUS Status;

    /* Assume failure */
    FoundSection = NULL;

    /* Make sure the image is valid */
    Status = RtlImageNtHeaderEx(0, ImageBase, ImageSize, &NtHeader);
    if (NT_SUCCESS(Status))
    {
        /* Get the first section and loop through them all */
        SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
        for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
        {
            /* Check if this is the resource section */
            if (!_stricmp((PCCH)SectionHeader->Name, ".rsrc"))
            {
                /* Yep, we're done */
                FoundSection = SectionHeader;
                break;
            }

            /* Nope, keep going */
            SectionHeader++;
        }
    }

    /* Return the matching section */
    return FoundSection;
}

VOID
BlImgQueryCodeIntegrityBootOptions (
    _In_ PBL_LOADED_APPLICATION_ENTRY ApplicationEntry,
    _Out_ PBOOLEAN IntegrityChecksDisabled, 
    _Out_ PBOOLEAN TestSigning
    )
{
    
    NTSTATUS Status;
    BOOLEAN Value;

    /* Check if /DISABLEINTEGRITYCHECKS is on */
    Status = BlGetBootOptionBoolean(ApplicationEntry->BcdData,
                                    BcdLibraryBoolean_DisableIntegrityChecks,
                                    &Value);
    *IntegrityChecksDisabled = NT_SUCCESS(Status) && (Value);

    /* Check if /TESTSIGNING is on */
    Status = BlGetBootOptionBoolean(ApplicationEntry->BcdData,
                                    BcdLibraryBoolean_AllowPrereleaseSignatures,
                                    &Value);
    *TestSigning = NT_SUCCESS(Status) && (Value);
}

NTSTATUS
BlImgUnLoadImage (
    _In_ PVOID ImageBase,
    _In_ ULONG ImageSize,
    _In_ ULONG ImageFlags
    )
{
    /* Check for missing parameters */
    if (!(ImageSize) || !(ImageBase))
    {
        /* Bail out */
        return STATUS_INVALID_PARAMETER;
    }

    /* Unallocate the image buffer */
    return BlImgUnallocateImageBuffer(ImageBase, ImageSize, ImageFlags);
}

NTSTATUS
BlImgLoadPEImageEx (
    _In_ ULONG DeviceId,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ PWCHAR Path,
    _Out_ PVOID* ImageBase,
    _Out_ PULONG ImageSize,
    _Out_ PVOID Hash,
    _In_ ULONG Flags
    )
{
    EfiPrintf(L"PE not implemented\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlImgLoadBootApplication (
    _In_ PBL_LOADED_APPLICATION_ENTRY BootEntry,
    _Out_ PULONG AppHandle
    )
{
    NTSTATUS Status;
    PULONGLONG AllowedList;
    ULONGLONG AllowedCount;
    ULONG i;
    LARGE_INTEGER Frequency;
    PVOID UnlockCode;
    PBL_DEVICE_DESCRIPTOR Device, BitLockerDevice;
    ULONG DeviceId;
    PWCHAR Path;
    PBL_APPLICATION_ENTRY AppEntry;
    PBL_IMG_FILE ImageFile;
    PVOID ImageBase;
    ULONG ImageSize;
    BOOLEAN DisableIntegrity, TestSigning;
    UCHAR Hash[64];
    ULONG Flags;
    ULONG ListSize;
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry;

    BitLockerDevice = NULL;
    UnlockCode = NULL;
    ImageFile = NULL;
    DeviceId = -1;
    Device = NULL;
    ImageAppEntry = NULL;
    Path = NULL;
    ImageSize = 0;
    ImageBase = NULL;

    EfiPrintf(L"Loading application %p\r\n", BootEntry);

    Status = BlpGetBootOptionIntegerList(BootEntry->BcdData,
                                         BcdLibraryIntegerList_AllowedInMemorySettings,
                                         &AllowedList,
                                         &AllowedCount,
                                         TRUE);
    if (Status == STATUS_SUCCESS)
    {
        for (i = 0; i < AllowedCount; i++)
        {
            if (AllowedList[i] == BcdLibraryInteger_UndocumentedMagic)
            {
                BlTimeQueryPerformanceCounter(&Frequency);
                BlAppendBootOptionInteger(BootEntry,
                                          BcdLibraryInteger_UndocumentedMagic,
                                          Frequency.QuadPart);
            }
        }
    }

#if BL_BITLOCKER_SUPPORT
    Status = BlFveSecureBootUnlockBootDevice(BootEntry, &BitLockerDevice, &UnlockCode);
#else
    Status = STATUS_SUCCESS;
#endif
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    Status = BlGetBootOptionDevice(BootEntry->BcdData, BcdLibraryDevice_ApplicationDevice, &Device, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    Status = BlGetBootOptionString(BootEntry->BcdData, BcdLibraryString_ApplicationPath, &Path);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    Status = BlpDeviceOpen(Device, BL_DEVICE_READ_ACCESS, 0, &DeviceId);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    BlImgQueryCodeIntegrityBootOptions(BootEntry, &DisableIntegrity, &TestSigning);

#if BL_TPM_SUPPORT
    RtlZeroMemory(&Context, sizeof(Context);
    Context.BootEntry = BootEntry;
    BlEnNotifyEvent(0x10000003, &Context);
#endif

    Flags = 0;
    if (!DisableIntegrity)
    {
        Flags = 0x8070;
    }

    Status = BlImgLoadPEImageEx(DeviceId, BlLoaderMemory, Path, &ImageBase, &ImageSize, Hash, Flags);
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

#if BL_KD_SUPPORT
    if (BdDebugTransitions)
    {
        BdForceDebug = 1;
        Status = BlBdInitialize();
        if (NT_SUCCESS(Status))
        {
            if (BlBdDebuggerEnabled())
            {
                BdDebuggerNotPresent = FALSE;
                RtlInitUnicodeString(&PathString, Path);
                BlBdLoadImageSymbols(&PathString, ImageBase);
            }
        }
    }
#endif

#if BL_BITLOCKER_SUPPORT
    Status = BlSecureBootCheckPolicyOnFveDevice(BitLockerDevice);
#else
    Status = STATUS_SUCCESS;
#endif
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

#if BL_BITLOCKER_SUPPORT
    Status = BlFveSecureBootCheckpointBootApp(BootEntry, BitLockerDevice, Hash, UnlockCode);
#else
Status = STATUS_SUCCESS;
#endif
    if (!NT_SUCCESS(Status))
    {
        goto Quickie;
    }

    ListSize = BlGetBootOptionListSize(BootEntry->BcdData);

    AppEntry = BlMmAllocateHeap(ListSize + sizeof(*AppEntry));
    if (!AppEntry)
    {
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    RtlZeroMemory(AppEntry, sizeof(AppEntry));

    strcpy(AppEntry->Signature, "BTAPENT");
    AppEntry->Guid = BootEntry->Guid;
    AppEntry->Flags = BootEntry->Flags;
    RtlCopyMemory(&AppEntry->BcdData, BootEntry->BcdData, ListSize);


    ImageAppEntry = BlMmAllocateHeap(sizeof(*ImageAppEntry));
    ImageAppEntry->ImageBase = ImageBase;
    ImageAppEntry->ImageSize = ImageSize;
    ImageAppEntry->AppEntry = AppEntry;

    if (!IapTableEntries)
    {
        IapAllocatedTableEntries = 0;
        IapTableEntries = 2;
        IapImageTable = BlMmAllocateHeap(IapTableEntries * sizeof(PVOID));
        if (!IapImageTable)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        RtlZeroMemory(IapImageTable, sizeof(IapTableEntries * sizeof(PVOID)));
    }

    Status = BlTblSetEntry(&IapImageTable,
                           &IapTableEntries,
                           ImageAppEntry,
                           AppHandle,
                           TblDoNotPurgeEntry);

Quickie:
    if (DeviceId != 1)
    {
        BlDeviceClose(DeviceId);
    }

    if (Device)
    {
        BlMmFreeHeap(Device);
    }

    if (Path)
    {
        BlMmFreeHeap(Path);
    }

    if (BitLockerDevice)
    {
        BlMmFreeHeap(BitLockerDevice);
    }

    if (UnlockCode)
    {
        BlMmFreeHeap(UnlockCode);
    }

    if (NT_SUCCESS(Status))
    {
        IapAllocatedTableEntries++;
    }
    else
    {
        if (ImageBase)
        {
            BlImgUnLoadImage(ImageBase, ImageSize, 0);
        }

        if (AppEntry)
        {
            BlMmFreeHeap(AppEntry);
        }

        if (ImageFile)
        {
            BlMmFreeHeap(ImageFile);
        }

        if (!(IapAllocatedTableEntries) && (IapImageTable))
        {
            BlMmFreeHeap(IapImageTable);
            IapTableEntries = 0;
            IapImageTable = NULL;
        }
    }

    return Status;
}

NTSTATUS
BlpPdParseReturnArguments (
    _In_ PBL_RETURN_ARGUMENTS ReturnArguments
    )
{
    if (ReturnArguments->DataPage == 0)
    {
        return STATUS_SUCCESS;
    }

    EfiPrintf(L"Return arguments not supported\r\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
BlImgStartBootApplication (
    _In_ ULONG AppHandle,
    _Inout_opt_ PBL_RETURN_ARGUMENTS ReturnArguments
    )
{
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry;
    BL_RETURN_ARGUMENTS LocalReturnArgs;
    PBL_FILE_SYSTEM_ENTRY FileSystem;
    PLIST_ENTRY NextEntry, ListHead;
    NTSTATUS Status;

    if (!ReturnArguments)
    {
        LocalReturnArgs.Version = 1;
        LocalReturnArgs.Status = STATUS_SUCCESS;
        LocalReturnArgs.Flags = 0;
        LocalReturnArgs.DataPage = 0;
        LocalReturnArgs.DataSize = 0;

        ReturnArguments = &LocalReturnArgs;
    }


    if (IapTableEntries <= AppHandle)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ImageAppEntry = IapImageTable[AppHandle];
    if (!ImageAppEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ListHead = &RegisteredFileSystems;
    NextEntry = RegisteredFileSystems.Flink;
    while (NextEntry != ListHead)
    {
        FileSystem = CONTAINING_RECORD(NextEntry, BL_FILE_SYSTEM_ENTRY, ListEntry);

        if (FileSystem->PurgeCallback)
        {
            FileSystem->PurgeCallback();
        }

        NextEntry = NextEntry->Flink;
    }

    /* TODO */
    //BlockIoPurgeCache();

    Status = ImgArchEfiStartBootApplication(ImageAppEntry->AppEntry,
                                            ImageAppEntry->ImageBase,
                                            ImageAppEntry->ImageSize,
                                            ReturnArguments);

    BlpPdParseReturnArguments(ReturnArguments);

#if BL_BITLOCKER_SUPPORT
    FvebpCheckAllPartitions(TRUE);
#endif

#if BL_TPM_SUPPORT
    BlEnNotifyEvent(0x10000005, NULL);
#endif

    /* TODO */
    //BlpDisplayReinitialize();

    /* TODO */
    //BlpLogInitialize();

    return Status;
}

NTSTATUS
BlImgUnloadBootApplication (
    _In_ ULONG AppHandle
    )
{
    PBL_IMAGE_APPLICATION_ENTRY ImageAppEntry; // esi@2
    NTSTATUS Status;

    if (IapTableEntries <= AppHandle)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ImageAppEntry = IapImageTable[AppHandle];
    if (!ImageAppEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = BlImgUnLoadImage(ImageAppEntry->ImageBase,
                              ImageAppEntry->ImageSize,
                              0);
    if (NT_SUCCESS(Status))
    {
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_MEMORY_NOT_ALLOCATED;
    }

    BlMmFreeHeap(ImageAppEntry->AppEntry);
    BlMmFreeHeap(ImageAppEntry);

    IapImageTable[AppHandle] = NULL;

    if (!(--IapAllocatedTableEntries))
    {
        BlMmFreeHeap(IapImageTable);
        IapImageTable = NULL;
        IapTableEntries = 0;
    }

    return Status;
}
