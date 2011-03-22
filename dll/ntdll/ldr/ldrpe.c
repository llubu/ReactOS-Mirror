/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/ldrpe.c
 * PURPOSE:         Loader Functions dealing low-level PE Format structures
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/
ULONG LdrpFatalHardErrorCount;
PVOID LdrpManifestProberRoutine;

/* PROTOTYPES ****************************************************************/

#define IMAGE_REL_BASED_HIGH3ADJ 11

NTSTATUS
NTAPI
LdrpLoadImportModule(IN PWSTR DllPath OPTIONAL,
                     IN LPSTR ImportName,
                     IN PVOID DllBase,
                     OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry,
                     OUT PBOOLEAN Existing);

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
LdrpSnapIAT(IN PLDR_DATA_TABLE_ENTRY ExportLdrEntry,
            IN PLDR_DATA_TABLE_ENTRY ImportLdrEntry,
            IN PIMAGE_IMPORT_DESCRIPTOR IatEntry,
            IN BOOLEAN EntriesValid)
{
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    ULONG ExportSize;
    PVOID Iat;
    SIZE_T ImportSize;
    ULONG IatSize;
    //PPEB Peb = NtCurrentPeb();
    NTSTATUS Status;
    PIMAGE_THUNK_DATA Thunk, OriginalThunk, FirstThunk;
    LPSTR ImportName;
    ULONG ForwarderChain;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionHeader;
    ULONG i, Rva;
    ULONG OldProtect;

    /* Get export directory */
    ExportDirectory = RtlImageDirectoryEntryToData(ExportLdrEntry->DllBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                   &ExportSize);

    /* Make sure it has one */
    if (!ExportDirectory) return STATUS_INVALID_IMAGE_FORMAT;

    /* Get the IAT */
    Iat = RtlImageDirectoryEntryToData(ImportLdrEntry->DllBase,
                                       TRUE,
                                       IMAGE_DIRECTORY_ENTRY_IAT,
                                       &IatSize);
    ImportSize = IatSize;

    /* Check if we don't have one */
    if (!Iat)
    {
        /* Get the NT Header and the first section */
        NtHeader = RtlImageNtHeader(ImportLdrEntry->DllBase);
        SectionHeader = IMAGE_FIRST_SECTION(NtHeader);

        /* Get the RVA of the import directory */
        Rva = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

        /* Make sure we got one */
        if (Rva)
        {
            /* Loop all the sections */
            for (i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
            {
                /* Check if we are inside this section */
                if ((Rva >= SectionHeader->VirtualAddress) &&
                    (Rva < (SectionHeader->VirtualAddress +
                     SectionHeader->SizeOfRawData)))
                {
                    /* We are, so set the IAT here */
                    Iat = (PVOID)((ULONG_PTR)(ImportLdrEntry->DllBase) +
                                      SectionHeader->VirtualAddress);

                    /* Set the size */
                    IatSize = SectionHeader->Misc.VirtualSize;

                    /* Deal with Watcom and other retarded compilers */
                    if (!IatSize)
                    {
                        IatSize = SectionHeader->SizeOfRawData;
                    }
                    
                    /* Found it, get out */
                    break;
                }

                /* No match, move to the next section */
                ++SectionHeader;
            }
        }

        /* If we still don't have an IAT, that's bad */
        if (!Iat) return STATUS_INVALID_IMAGE_FORMAT;

        /* Set the right size */
        ImportSize = IatSize;
    }

    /* Unprotect the IAT */
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &Iat,
                                    &ImportSize,
                                    PAGE_READWRITE,
                                    &OldProtect);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if the Thunks are already valid */
    if (EntriesValid)
    {
        /* We'll only do forwarders. Get the import name */
        ImportName = (LPSTR)((ULONG_PTR)ImportLdrEntry->DllBase + IatEntry->Name);

        /* Get the list of forwaders */
        ForwarderChain = IatEntry->ForwarderChain;

        /* Loop them */
        while (ForwarderChain != -1)
        {
            /* Get the cached thunk VA*/
            OriginalThunk = (PIMAGE_THUNK_DATA)
                            ((ULONG_PTR)ImportLdrEntry->DllBase +
                             IatEntry->OriginalFirstThunk +
                             (ForwarderChain * sizeof(IMAGE_THUNK_DATA)));

            /* Get the first thunk */
            FirstThunk = (PIMAGE_THUNK_DATA)
                         ((ULONG_PTR)ImportLdrEntry->DllBase +
                          IatEntry->FirstThunk +
                          (ForwarderChain * sizeof(IMAGE_THUNK_DATA)));

            /* Get the Forwarder from the thunk */
            ForwarderChain = (ULONG)FirstThunk->u1.Ordinal;

            /* Snap the thunk */
            Status = LdrpSnapThunk(ExportLdrEntry->DllBase,
                                   ImportLdrEntry->DllBase,
                                   OriginalThunk,
                                   FirstThunk,
                                   ExportDirectory,
                                   ExportSize,
                                   TRUE,
                                   ImportName);

            /* Move to the next thunk */
            FirstThunk++;

            /* If we messed up, exit */
            if (!NT_SUCCESS(Status)) break;
        }
    }
    else if (IatEntry->FirstThunk)
    {
        /* Full snapping. Get the First thunk */
        FirstThunk = (PIMAGE_THUNK_DATA)
                      ((ULONG_PTR)ImportLdrEntry->DllBase +
                       IatEntry->FirstThunk);

        /* Get the NT Header */
        NtHeader = RtlImageNtHeader(ImportLdrEntry->DllBase);

        /* Get the Original thunk VA, watch out for weird images */
        if ((IatEntry->Characteristics < NtHeader->OptionalHeader.SizeOfHeaders) ||
            (IatEntry->Characteristics >= NtHeader->OptionalHeader.SizeOfImage))
        {
            /* Reuse it, this is a strange linked file */
            OriginalThunk = FirstThunk;
        }
        else
        {
            /* Get the address from the field and convert to VA */
            OriginalThunk = (PIMAGE_THUNK_DATA)
                            ((ULONG_PTR)ImportLdrEntry->DllBase +
                             IatEntry->OriginalFirstThunk);
        }

        /* Get the Import name VA */
        ImportName = (LPSTR)((ULONG_PTR)ImportLdrEntry->DllBase +
                             IatEntry->Name);

        /* Loop while it's valid */
        while (OriginalThunk->u1.AddressOfData)
        {
            /* Snap the Thunk */
            Status = LdrpSnapThunk(ExportLdrEntry->DllBase,
                                   ImportLdrEntry->DllBase,
                                   OriginalThunk,
                                   FirstThunk,
                                   ExportDirectory,
                                   ExportSize,
                                   TRUE,
                                   ImportName);

            /* Next thunks */
            OriginalThunk++;
            Thunk++;

            /* If we failed the snap, break out */
            if (!NT_SUCCESS(Status)) break;
        }
    }

    /* Protect the IAT again */
    NtProtectVirtualMemory(NtCurrentProcess(),
                           &Iat,
                           &ImportSize,
                           OldProtect,
                           &OldProtect);

    /* Also flush out the cache */
    NtFlushInstructionCache(NtCurrentProcess(), Iat, IatSize);

    /* Return to Caller */
    return Status;
}

NTSTATUS
NTAPI
LdrpHandleOneNewFormatImportDescriptor(IN LPWSTR DllPath OPTIONAL,
                                       IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                       IN PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry,
                                       IN PIMAGE_BOUND_IMPORT_DESCRIPTOR FirstEntry)
{
    LPSTR ImportName = NULL, BoundImportName, ForwarderName;
    NTSTATUS Status;
    BOOLEAN AlreadyLoaded = FALSE, Stale;
    PIMAGE_IMPORT_DESCRIPTOR ImportEntry;
    PLDR_DATA_TABLE_ENTRY DllLdrEntry, ForwarderLdrEntry;
    PIMAGE_BOUND_FORWARDER_REF ForwarderEntry;
    PPEB Peb = NtCurrentPeb();
    ULONG i, IatSize;

    /* Get the name's VA */
    BoundImportName = (LPSTR)(BoundEntry + BoundEntry->OffsetModuleName);

    /* Show debug mesage */
    if (ShowSnaps)
    {
        DPRINT1("LDR: %wZ bound to %s\n", &LdrEntry->BaseDllName, BoundImportName);
    }

    /* Load the module for this entry */
    Status = LdrpLoadImportModule(DllPath,
                                  BoundImportName,
                                  LdrEntry->DllBase,
                                  &DllLdrEntry,
                                  &AlreadyLoaded);
    if (!NT_SUCCESS(Status))
    {
        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ failed to load import module %s; status = %x\n",
                    &LdrEntry->BaseDllName,
                    BoundImportName,
                    Status);
        }
        goto Quickie;
    }

    /* Check if it wasn't already loaded */
    if (!AlreadyLoaded)
    {
        /* Add it to our list */
        InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                       &DllLdrEntry->InInitializationOrderModuleList);
    }

    /* Check if the Bound Entry is now invalid */
    if ((BoundEntry->TimeDateStamp != DllLdrEntry->TimeDateStamp) ||
        (DllLdrEntry->Flags & LDRP_IMAGE_NOT_AT_BASE))
    {
        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ has stale binding to %s\n",
                    &DllLdrEntry->BaseDllName,
                    BoundImportName);
        }

        /* Remember it's become stale */
        Stale = TRUE;
    }
    else
    {
        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ has correct binding to %s\n",
                    &DllLdrEntry->BaseDllName,
                    BoundImportName);
        }

        /* Remember it's valid */
        Stale = FALSE;
    }

    /* Get the forwarders */
    ForwarderEntry = (PIMAGE_BOUND_FORWARDER_REF)(BoundEntry + 1);

    /* Loop them */
    for (i = 0; i < BoundEntry->NumberOfModuleForwarderRefs; i++)
    {
        /* Get the name */
        ForwarderName = (LPSTR)(FirstEntry + ForwarderEntry->OffsetModuleName);

        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: %wZ bound to %s via forwarder(s) from %wZ\n",
                    &LdrEntry->BaseDllName,
                    ForwarderName,
                    &DllLdrEntry->BaseDllName);
        }

        /* Load the module */
        Status = LdrpLoadImportModule(DllPath,
                                      ForwarderName,
                                      LdrEntry->DllBase,
                                      &ForwarderLdrEntry,
                                      &AlreadyLoaded);
        if (NT_SUCCESS(Status))
        {
            /* Loaded it, was it already loaded? */
            if (!AlreadyLoaded)
            {
                /* Add it to our list */
                InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                               &ForwarderLdrEntry->InInitializationOrderModuleList);
            }
        }

        /* Check if the Bound Entry is now invalid */
        if (!(NT_SUCCESS(Status)) ||
            (ForwarderEntry->TimeDateStamp != ForwarderLdrEntry->TimeDateStamp) ||
            (ForwarderLdrEntry->Flags & LDRP_IMAGE_NOT_AT_BASE))
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: %wZ has stale binding to %s\n",
                        &ForwarderLdrEntry->BaseDllName,
                        ForwarderName);
            }

            /* Remember it's become stale */
            Stale = TRUE;
        }
        else
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: %wZ has correct binding to %s\n",
                        &ForwarderLdrEntry->BaseDllName,
                        ForwarderName);
            }

            /* Remember it's valid */
            Stale = FALSE;
        }

        /* Move to the next one */
        ForwarderEntry++;
    }

    /* Set the next bound entry to the forwarder */
    FirstEntry = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)ForwarderEntry;

    /* Check if the binding was stale */
    if (Stale)
    {
        /* It was, so find the IAT entry for it */
        ImportEntry = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                   &IatSize);

        /* Make sure it has a name */
        while (ImportEntry->Name)
        {
            /* Get the name */
            ImportName = (LPSTR)((ULONG_PTR)LdrEntry->DllBase + ImportEntry->Name);

            /* Compare it */
            if (!_stricmp(ImportName, BoundImportName)) break;

            /* Move to next entry */
            ImportEntry += 1;
        }

        /* If we didn't find a name, fail */
        if (!ImportEntry->Name)
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: LdrpWalkImportTable - failing with"
                        "STATUS_OBJECT_NAME_INVALID due to no import descriptor name\n");
            }

            /* Return error */
            Status = STATUS_OBJECT_NAME_INVALID;
            goto Quickie;
        }

        /* Show debug message */
        if (ShowSnaps)
        {
            DPRINT1("LDR: Stale Bind %s from %wZ\n",
                    ImportName,
                    &LdrEntry->BaseDllName);
        }

        /* Snap the IAT Entry*/
        Status = LdrpSnapIAT(DllLdrEntry,
                             LdrEntry,
                             ImportEntry,
                             FALSE);

        /* Make sure we didn't fail */
        if (!NT_SUCCESS(Status))
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: %wZ failed to load import module %s; status = %x\n",
                        &LdrEntry->BaseDllName,
                        BoundImportName,
                        Status);
            }

            /* Return */
            goto Quickie;
        }
    }

    /* All done */
    Status = STATUS_SUCCESS;

Quickie:
    /* Write where we are now and return */
    *BoundEntry = *FirstEntry;
    return Status;
}

NTSTATUS
NTAPI
LdrpHandleNewFormatImportDescriptors(IN LPWSTR DllPath OPTIONAL,
                                    IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                    IN PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry)
{
    PIMAGE_BOUND_IMPORT_DESCRIPTOR FirstEntry = BoundEntry;
    NTSTATUS Status;

    /* Make sure we have a name */
    while (BoundEntry->OffsetModuleName)
    {
        /* Parse this descriptor */
        Status = LdrpHandleOneNewFormatImportDescriptor(DllPath,
                                                        LdrEntry,
                                                        BoundEntry,
                                                        FirstEntry);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpHandleOneOldFormatImportDescriptor(IN LPWSTR DllPath OPTIONAL,
                                       IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                       IN PIMAGE_IMPORT_DESCRIPTOR ImportEntry)
{
    //ULONG IatSize, i;
    LPSTR ImportName;
    NTSTATUS Status;
    BOOLEAN AlreadyLoaded = FALSE, StaticEntriesValid = FALSE, SkipSnap = TRUE;
    PLDR_DATA_TABLE_ENTRY DllLdrEntry;
    PIMAGE_THUNK_DATA FirstThunk;
    PPEB Peb = NtCurrentPeb();

    /* Get the import name's VA */
    ImportName = (LPSTR)((ULONG_PTR)LdrEntry->DllBase + ImportEntry->Name);

    /* Get the first thunk */
    FirstThunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)LdrEntry->DllBase +
                                     ImportEntry->FirstThunk);

    /* Make sure it's valid */
    if (!FirstThunk->u1.Function) goto SkipEntry;

    /* Show debug message */
    if (ShowSnaps)
    {
        DPRINT1("LDR: %s used by %wZ\n",
                ImportName,
                &LdrEntry->BaseDllName);
    }

    /* Load the module associated to it */
    Status = LdrpLoadImportModule(DllPath,
                                  ImportName,
                                  LdrEntry->DllBase,
                                  &DllLdrEntry,
                                  &AlreadyLoaded);
    if (!NT_SUCCESS(Status)) return Status;

    /* Show debug message */
    if (ShowSnaps)
    {
        DPRINT1("LDR: Snapping imports for %wZ from %s\n",
                &LdrEntry->BaseDllName,
                ImportName);
    }

    /* Check if the image was bound when compiled */
    if (ImportEntry->OriginalFirstThunk)
    {
        /* It was, so check if the static IAT entries are still valid */
        if ((ImportEntry->TimeDateStamp) &&
            (ImportEntry->TimeDateStamp == DllLdrEntry->TimeDateStamp) &&
            (!(DllLdrEntry->Flags & LDRP_IMAGE_NOT_AT_BASE)))
        {
            /* Show debug message */
            if (ShowSnaps)
            {
                DPRINT1("LDR: Snap bypass %s from %wZ\n",
                        ImportName,
                        &LdrEntry->BaseDllName);
            }

            /*
             * They are still valid, so we can skip snapping them.
             * Additionally, if we have no forwarders, we are totally
             * done.
             */
            if (ImportEntry->ForwarderChain == -1)
            {
                /* Totally skip LdrpSnapIAT */
                SkipSnap = TRUE;
            }
            else
            {
                /* Set this so LdrpSnapIAT will only do forwarders */
                StaticEntriesValid = TRUE;
            }
        }
    }

    /* Check if it wasn't already loaded */
    if (!AlreadyLoaded)
    {
        /* Add the DLL to our list */
        InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                       &DllLdrEntry->InInitializationOrderModuleList);
    }

    /* Check if we should snap at all */
    if (!SkipSnap)
    {
        /* Now snap the IAT Entry */
        Status = LdrpSnapIAT(DllLdrEntry,
                             LdrEntry,
                             ImportEntry,
                             StaticEntriesValid);
        if (!NT_SUCCESS(Status)) return Status;
    }

SkipEntry:
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpHandleOldFormatImportDescriptors(IN LPWSTR DllPath OPTIONAL,
                                     IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                                     IN PIMAGE_IMPORT_DESCRIPTOR ImportEntry)
{
    NTSTATUS Status;

    /* Check for Name and Thunk */
    while (ImportEntry->Name && ImportEntry->FirstThunk)
    {
        /* Parse this descriptor */
        Status = LdrpHandleOneOldFormatImportDescriptor(DllPath,
                                                        LdrEntry,
                                                        ImportEntry);
        if (!NT_SUCCESS(Status)) return Status;

        /* Move to the next entry */
        ImportEntry++;
    }

    /* Done */
    return STATUS_SUCCESS;
}

USHORT NTAPI
LdrpNameToOrdinal(LPSTR ImportName,
                  ULONG NumberOfNames,
                  PVOID ExportBase,
                  PULONG NameTable,
                  PUSHORT OrdinalTable)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
LdrpWalkImportDescriptor(IN LPWSTR DllPath OPTIONAL,
                         IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED ActCtx;
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status = STATUS_SUCCESS;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundEntry = NULL;
    PIMAGE_IMPORT_DESCRIPTOR ImportEntry;
    ULONG BoundSize, IatSize;
DPRINT1("LdrpWalkImportDescriptor('%S' %x)\n", DllPath, LdrEntry);
    /* Set up the Act Ctx */
    ActCtx.Size = sizeof(ActCtx);
    ActCtx.Frame.Flags = ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID;
    RtlZeroMemory(&ActCtx, sizeof(ActCtx));

    /* Check if we have a manifest prober routine */
    if (LdrpManifestProberRoutine)
    {
        DPRINT1("We don't support manifests yet, much less prober routines\n");
    }

    /* Check if we failed above */
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the Active ActCtx */
    Status = RtlGetActiveActivationContext(&LdrEntry->EntryPointActivationContext);
    if (!NT_SUCCESS(Status)) return Status;

    /* Activate the ActCtx */
    RtlActivateActivationContextUnsafeFast(&ActCtx,
                                           LdrEntry->EntryPointActivationContext);

    /* Check if we were directed */
    if (!(LdrEntry->Flags & LDRP_REDIRECTED))
    {
        /* Get the Bound IAT */
        BoundEntry = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                  TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                                                  &BoundSize);
    }

    /* Get the regular IAT, for fallback */
    ImportEntry = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                               TRUE,
                                               IMAGE_DIRECTORY_ENTRY_IMPORT,
                                               &IatSize);

    /* Check if we got at least one */
    if (BoundEntry || ImportEntry)
    {
        /* Do we have a Bound IAT */
        if (BoundEntry)
        {
            /* Handle the descriptor */
            Status = LdrpHandleNewFormatImportDescriptors(DllPath,
                                                          LdrEntry,
                                                          BoundEntry);
        }
        else
        {
            /* Handle the descriptor */
            Status = LdrpHandleOldFormatImportDescriptors(DllPath,
                                                          LdrEntry,
                                                          ImportEntry);
        }

        /* Check the status of the handlers */
        if (NT_SUCCESS(Status))
        {
            /* Check for Per-DLL Heap Tagging */
            if (Peb->NtGlobalFlag & FLG_HEAP_ENABLE_TAG_BY_DLL)
            {
                /* FIXME */
                DPRINT1("We don't support Per-DLL Heap Tagging yet!\n");
            }

            /* Check if Page Heap was enabled */
            if (Peb->NtGlobalFlag & FLG_HEAP_PAGE_ALLOCS)
            {
                /* FIXME */
                DPRINT1("We don't support Page Heaps yet!\n");
            }

            /* Check if Application Verifier was enabled */
            if (Peb->NtGlobalFlag & FLG_HEAP_ENABLE_TAIL_CHECK)
            {
                /* FIXME */
                DPRINT1("We don't support Application Verifier yet!\n");
            }

            /* Just to be safe */
            Status = STATUS_SUCCESS;
        }
    }

    /* Release the activation context */
    RtlDeactivateActivationContextUnsafeFast(&ActCtx);

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
LdrpLoadImportModule(IN PWSTR DllPath OPTIONAL,
                     IN LPSTR ImportName,
                     IN PVOID DllBase,
                     OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry,
                     OUT PBOOLEAN Existing)
{
    ANSI_STRING AnsiString;
    PUNICODE_STRING ImpDescName;
    NTSTATUS Status;
    PPEB Peb = RtlGetCurrentPeb();
    PTEB Teb = NtCurrentTeb();
DPRINT1("LdrpLoadImportModule('%S' '%s' %p %p %p)\n", DllPath, ImportName, DllBase, DataTableEntry, Existing);
    /* Convert import descriptor name to unicode string */
    ImpDescName = &Teb->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString, ImportName);
    Status = RtlAnsiStringToUnicodeString(ImpDescName, &AnsiString, FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if it's loaded */
    if (LdrpCheckForLoadedDll(DllPath,
                              ImpDescName,
                              TRUE,
                              FALSE,
                              DataTableEntry))
    {
        /* It's already existing in the list */
        *Existing = TRUE;
        return STATUS_SUCCESS;
    }

    /* We're loading it for the first time */
    *Existing = FALSE;

    /* Map it */
    Status = LdrpMapDll(DllPath,
                        ImpDescName->Buffer,
                        NULL,
                        NULL,
                        TRUE,
                        FALSE,
                        DataTableEntry);

    if (!NT_SUCCESS(Status)) return Status;

    /* Walk its import descriptor table */
    Status = LdrpWalkImportDescriptor(DllPath,
                                      *DataTableEntry);
    if (!NT_SUCCESS(Status))
    {
        /* Add it to the in-init-order list in case of failure */
        InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                       &(*DataTableEntry)->InInitializationOrderModuleList);
    }

    return Status;
}

NTSTATUS
NTAPI
LdrpSnapThunk(IN PVOID ExportBase,
              IN PVOID ImportBase,
              IN PIMAGE_THUNK_DATA OriginalThunk,
              IN OUT PIMAGE_THUNK_DATA Thunk,
              IN PIMAGE_EXPORT_DIRECTORY ExportEntry,
              IN ULONG ExportSize,
              IN BOOLEAN Static,
              IN LPSTR DllName)
{
    BOOLEAN IsOrdinal;
    USHORT Ordinal;
    ULONG OriginalOrdinal = 0;
    PIMAGE_IMPORT_BY_NAME AddressOfData;
    PULONG NameTable;
    PUSHORT OrdinalTable;
    LPSTR ImportName = NULL;
    USHORT Hint;
    NTSTATUS Status;
    ULONG_PTR HardErrorParameters[3];
    UNICODE_STRING HardErrorDllName, HardErrorEntryPointName;
    ANSI_STRING TempString;
    ULONG Mask;
    ULONG Response;
    PULONG AddressOfFunctions;
    UNICODE_STRING TempUString;
    ANSI_STRING ForwarderName;
    PANSI_STRING ForwardName;
    PVOID ForwarderHandle;
    ULONG ForwardOrdinal;

    /* Check if the snap is by ordinal */
    if ((IsOrdinal = IMAGE_SNAP_BY_ORDINAL(OriginalThunk->u1.Ordinal)))
    {
        /* Get the ordinal number, and its normalized version */
        OriginalOrdinal = IMAGE_ORDINAL(OriginalThunk->u1.Ordinal);
        Ordinal = (USHORT)(OriginalOrdinal - ExportEntry->Base);
    }
    else
    {
        /* First get the data VA */
        AddressOfData = (PIMAGE_IMPORT_BY_NAME)
                        ((ULONG_PTR)ImportBase +
                        ((ULONG_PTR)OriginalThunk->u1.AddressOfData & 0xffffffff));

        /* Get the name */
        ImportName = (LPSTR)AddressOfData->Name;

        /* Now get the VA of the Name and Ordinal Tables */
        NameTable = (PULONG)((ULONG_PTR)ExportBase +
                             (ULONG_PTR)ExportEntry->AddressOfNames);
        OrdinalTable = (PUSHORT)((ULONG_PTR)ExportBase +
                                 (ULONG_PTR)ExportEntry->AddressOfNameOrdinals);

        /* Get the hint */
        Hint = AddressOfData->Hint;

        /* Try to get a match by using the hint */
        if (((ULONG)Hint < ExportEntry->NumberOfNames) &&
             (!strcmp(ImportName, ((LPSTR)((ULONG_PTR)ExportBase + NameTable[Hint])))))
        {
            /* We got a match, get the Ordinal from the hint */
            Ordinal = OrdinalTable[Hint];
        }
        else
        {
            /* Well bummer, hint didn't work, do it the long way */
            Ordinal = LdrpNameToOrdinal(ImportName,
                                        ExportEntry->NumberOfNames,
                                        ExportBase,
                                        NameTable,
                                        OrdinalTable);
        }
    }

    /* Check if the ordinal is invalid */
    if ((ULONG)Ordinal >= ExportEntry->NumberOfFunctions)
    {
FailurePath:
        /* Is this a static snap? */
        if (Static)
        {
            /* These are critical errors. Setup a string for the DLL name */
            RtlInitAnsiString(&TempString, DllName ? DllName : "Unknown");
            RtlAnsiStringToUnicodeString(&HardErrorDllName, &TempString, TRUE);

            /* Set it as the parameter */
            HardErrorParameters[1] = (ULONG_PTR)&HardErrorDllName;
            Mask = 2;

            /* Check if we have an ordinal */
            if (IsOrdinal)
            {
                /* Then set the ordinal as the 1st parameter */
                HardErrorParameters[0] = OriginalOrdinal;
            }
            else
            {
                /* We don't, use the entrypoint. Set up a string for it */
                RtlInitAnsiString(&TempString, ImportName);
                RtlAnsiStringToUnicodeString(&HardErrorEntryPointName,
                                             &TempString,
                                             TRUE);

                /* Set it as the parameter */
                HardErrorParameters[0] = (ULONG_PTR)&HardErrorEntryPointName;
                Mask = 3;
            }

            /* Raise the error */
            NtRaiseHardError(IsOrdinal ? STATUS_ORDINAL_NOT_FOUND :
                                         STATUS_ENTRYPOINT_NOT_FOUND,
                             2,
                             Mask,
                             HardErrorParameters,
                             OptionOk,
                             &Response);

            /* Increase the error count */
            if (LdrpInLdrInit) LdrpFatalHardErrorCount++;

            /* Free our string */
            RtlFreeUnicodeString(&HardErrorDllName);
            if (!IsOrdinal)
            {
                /* Free our second string. Return entrypoint error */
                RtlFreeUnicodeString(&HardErrorEntryPointName);
                RtlRaiseStatus(STATUS_ENTRYPOINT_NOT_FOUND);
            }

            /* Return ordinal error */
            RtlRaiseStatus(STATUS_ORDINAL_NOT_FOUND);
        }

        /* Set this as a bad DLL */
        Thunk->u1.Function = (ULONG_PTR)0xffbadd11;

        /* Return the right error code */
        Status = IsOrdinal ? STATUS_ORDINAL_NOT_FOUND :
                             STATUS_ENTRYPOINT_NOT_FOUND;
    }
    else
    {
        /* The ordinal seems correct, get the AddressOfFunctions VA */
        AddressOfFunctions = (PULONG)
                             ((ULONG_PTR)ExportBase +
                              (ULONG_PTR)ExportEntry->AddressOfFunctions);

        /* Write the function pointer*/
        Thunk->u1.Function = (ULONG_PTR)ExportBase + AddressOfFunctions[Ordinal];

        /* Make sure it's within the exports */
        if ((Thunk->u1.Function > (ULONG_PTR)ExportEntry) &&
            (Thunk->u1.Function < ((ULONG_PTR)ExportEntry + ExportSize)))
        {
            /* Get the Import and Forwarder Names */
            ImportName = (LPSTR)Thunk->u1.Function;
            ForwarderName.Buffer = ImportName;
            ForwarderName.Length = (USHORT)(strchr(ImportName, '.') - ImportName);
            ForwarderName.MaximumLength = ForwarderName.Length;
            Status = RtlAnsiStringToUnicodeString(&TempUString,
                                                  &ForwarderName,
                                                  TRUE);

            /* Make sure the conversion was OK */
            if (NT_SUCCESS(Status))
            {
                /* Load the forwarder, free the temp string */
                Status = LdrpLoadDll(FALSE,
                                     NULL,
                                     NULL,
                                     &TempUString,
                                     &ForwarderHandle,
                                     FALSE);
                RtlFreeUnicodeString(&TempUString);
            }

            /* If the load or conversion failed, use the failure path */
            if (!NT_SUCCESS(Status)) goto FailurePath;

            /* Now set up a name for the actual forwarder dll */
            RtlInitAnsiString(&ForwarderName,
                              ImportName + ForwarderName.Length + sizeof(CHAR));

            /* Check if it's an ordinal forward */
            if ((ForwarderName.Length > 1) && (*ForwarderName.Buffer == '#'))
            {
                /* We don't have an actual function name */
                ForwardName = NULL;

                /* Convert the string into an ordinal */
                Status = RtlCharToInteger(ForwarderName.Buffer + sizeof(CHAR),
                                          0,
                                          &ForwardOrdinal);

                /* If this fails, then error out */
                if (!NT_SUCCESS(Status)) goto FailurePath;
            }
            else
            {
                /* Import by name */
                ForwardName = &ForwarderName;
            }

            /* Get the pointer */
            Status = LdrpGetProcedureAddress(ForwarderHandle,
                                             ForwardName,
                                             ForwardOrdinal,
                                             (PVOID*)&Thunk->u1.Function,
                                             FALSE);
            /* If this fails, then error out */
            if (!NT_SUCCESS(Status)) goto FailurePath;
        }
        else
        {
            /* It's not within the exports, let's hope it's valid */
            if (!AddressOfFunctions[Ordinal]) goto FailurePath;
        }

        /* If we got here, then it's success */
        Status = STATUS_SUCCESS;
    }

    /* Return status */
    return Status;
}

/* EOF */
