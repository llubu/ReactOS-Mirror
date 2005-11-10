/*
 * IMAGEHLP library
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2005 Alex Ionescu
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

//#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

CHAR BoundLibraries[4096];
LPSTR BoundLibrariesPointer;

/* FUNCTIONS *****************************************************************/

LPSTR
IMAGEAPI
BindpCaptureImportModuleName(LPSTR ModuleName)
{
    LPSTR Name = BoundLibraries;

    /* Check if it hasn't been initialized yet */
    if (!BoundLibrariesPointer)
    {
        /* Start with a null char and set the pointer */
        *Name = ANSI_NULL;
        BoundLibrariesPointer = Name;
    }

    /* Loop the current buffer */
    while (*Name)
    {
        /* Try to match this DLL's name and return it */
        if (!_stricmp(Name, ModuleName)) return Name;

        /* Move on to the next DLL Name */
        Name += strlen(Name) + sizeof(CHAR);
    }

    /* If we got here, we didn't find one, so add this one to our buffer */
    strcpy(Name, ModuleName);

    /* Set the new position of the buffer, and null-terminate it */
    BoundLibrariesPointer = Name + strlen(Name) + sizeof(CHAR);
    *BoundLibrariesPointer = ANSI_NULL;

    /* Return the pointer to the name */
    return Name;
}

PIMAGE_BOUND_IMPORT_DESCRIPTOR
IMAGEAPI
BindpCreateNewImportSection(PIMPORT_DESCRIPTOR *BoundImportDescriptor,
                            PULONG BoundImportsSize)
{
    ULONG BoundLibraryNamesSize = 0, BoundImportTableSize = 0;
    PBOUND_FORWARDER_REFS Forwarder, *NextForwarder;
    PIMPORT_DESCRIPTOR Descriptor, *NextDescriptor;
    LPSTR BoundLibraryNames;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundTableEntry, BoundTable;
    PIMAGE_BOUND_FORWARDER_REF BoundForwarder;

    /* Zero the outoging size */
    *BoundImportsSize = 0;

    /* Loop the descriptors and forwarders to get the size */
    NextDescriptor = BoundImportDescriptor;
    while ((Descriptor = *NextDescriptor))
    {
        /* Add to the size of the Bound Import Table */
        BoundImportTableSize += sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR);

        /* Check Forwarders */
        NextForwarder = &Descriptor->Forwarders;
        while ((Forwarder = *NextForwarder))
        {
            /* Add to size of Bound Import Table */
            BoundImportTableSize += sizeof(IMAGE_BOUND_FORWARDER_REF);

            /* Next Forwarder */
            NextForwarder = &Forwarder->Next;
        }

        /* Read Next Internal Descriptor */
        NextDescriptor = &Descriptor->Next;
    }

    /* Add Terminator for PE Loader*/
    BoundImportTableSize += sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR);
    DPRINT("Table size: %lx\n", BoundImportTableSize);

    /* Name of Libraries Bound in Bound Import Table */
    BoundLibraryNamesSize = (ULONG)((ULONG_PTR)BoundLibrariesPointer -
                                    (ULONG_PTR)BoundLibraries);
    BoundLibrariesPointer = NULL;

    /* Size of the whole table, dword aligned */
    *BoundImportsSize = BoundImportTableSize + 
                        ((BoundLibraryNamesSize + sizeof(ULONG) - 1) &
                        ~(sizeof(ULONG) - 1));

    /* Allocate it */
    BoundTable = HeapAlloc(IMAGEHLP_hHeap, HEAP_ZERO_MEMORY, *BoundImportsSize);
    
    /* Pointer Library Names inside the Bound Import Table */
    BoundLibraryNames = (LPSTR)BoundTable + BoundImportTableSize;

    /* Copy the Library Names */
    RtlCopyMemory(BoundLibraryNames, BoundLibraries, BoundLibraryNamesSize);

    /* Now loop both tables */
    BoundTableEntry = BoundTable;
    NextDescriptor = BoundImportDescriptor;
    while ((Descriptor = *NextDescriptor))
    {
        /* Copy the data */
        BoundTableEntry->TimeDateStamp = Descriptor->TimeDateStamp;
        BoundTableEntry->OffsetModuleName = (USHORT)(BoundImportTableSize +
                                                     (Descriptor->ModuleName -
                                                      (ULONG_PTR)BoundLibraries));
        BoundTableEntry->NumberOfModuleForwarderRefs = Descriptor->ForwaderReferences;

        /* Now loop the forwarders */
        BoundForwarder = (PIMAGE_BOUND_FORWARDER_REF)BoundTableEntry + 1;
        NextForwarder = &Descriptor->Forwarders;
        while ((Forwarder = *NextForwarder))
        {
            /* Copy the data */
            BoundForwarder->TimeDateStamp = Forwarder->TimeDateStamp;
            BoundForwarder->OffsetModuleName = (USHORT)(BoundImportTableSize +
                                                       (Forwarder->ModuleName -
                                                       (ULONG_PTR)BoundLibraries));

            /* Move to the next new forwarder, and move to the next entry */
            BoundForwarder++;
            NextForwarder = &Forwarder->Next;
        }

        /* Move to next Bound Import Table Entry */
        BoundTableEntry = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)BoundForwarder;

        /* Move to the next descriptor */
        NextDescriptor = &Descriptor->Next;
    }

    /* Loop the descriptors and forwarders to free them */
    NextDescriptor = BoundImportDescriptor;
    while ((Descriptor = *NextDescriptor))
    {
        /* Read next internal descriptor */
        *NextDescriptor = Descriptor->Next;

        /* Loop its forwarders */
        NextForwarder = &Descriptor->Forwarders;
        while ((Forwarder = *NextForwarder))
        {
            /* Next Forwarder */
            *NextForwarder = Forwarder->Next;

            /* Free it */
            HeapFree(IMAGEHLP_hHeap, 0, Forwarder);
        }

        /* Free it */
        HeapFree(IMAGEHLP_hHeap, 0, Descriptor);
    }

    /* Return the Bound Import Table */
    return BoundTable;
}

PCHAR
IMAGEAPI
BindpAddForwarderReference(LPSTR ModuleName,
                           LPSTR ImportName,
                           PIMPORT_DESCRIPTOR BoundImportDescriptor,
                           LPSTR DllPath,
                           PCHAR ForwarderString,
                           PBOOL ForwarderBound)
{
    CHAR DllName[256];
    PCHAR TempDllName, FunctionName;
    PLOADED_IMAGE Library;
    SIZE_T DllNameSize;
    USHORT OrdinalNumber;
    USHORT HintIndex;
    ULONG ExportSize;
    PIMAGE_EXPORT_DIRECTORY Exports;
    ULONG_PTR ExportsBase;
    PULONG AddressOfNames;
    PUSHORT AddressOfOrdinals;
    PULONG AddressOfPointers;
    LPSTR ExportName;
    ULONG_PTR ForwardedAddress;
    PBOUND_FORWARDER_REFS Forwarder, *NextForwarder;
    PIMAGE_OPTIONAL_HEADER OptionalHeader = NULL;

NextForwarder:

    /* Get the DLL Name */
    TempDllName = ForwarderString;
    while (*TempDllName && *TempDllName != '.') TempDllName++;
    if (*TempDllName != '.') return ForwarderString;

    /* Get the size */
    DllNameSize = (SIZE_T)(TempDllName - ForwarderString);
    if (DllNameSize >= MAX_PATH) return ForwarderString;

    /* Now copy the name and append the extension */
    strncpy(DllName, ForwarderString, DllNameSize);
    DllName[DllNameSize] = ANSI_NULL;
    strcat(DllName, ".DLL");

    /* Load it */
    DPRINT("Loading the Thunk Library: %s \n", DllName);
    Library = ImageLoad(DllName, DllPath);
    if (!Library) return ForwarderString;

    /* Move past the name */
    DPRINT("It Loaded at: %p \n", Library->MappedAddress);
    FunctionName = TempDllName += 1;

    /* Load Exports */
    Exports = ImageDirectoryEntryToData(Library->MappedAddress,
                                        FALSE,
                                        IMAGE_DIRECTORY_ENTRY_EXPORT,
                                        &ExportSize);
    if (!Exports) return ForwarderString;

    /* Get the Optional Header */
    OptionalHeader = &Library->FileHeader->OptionalHeader;

    /* Check if we're binding by ordinal */
    if (*FunctionName == '#')
    {
        /* We are, get the number and validate it */
        OrdinalNumber = atoi(FunctionName + 1) - (USHORT)Exports->Base;
        if (OrdinalNumber >= Exports->NumberOfFunctions) return ForwarderString;
    }
    else
    {
        /* Binding by name... */
        OrdinalNumber = -1;
    }

    /* Get the Pointers to the Tables */
    AddressOfNames = ImageRvaToVa(Library->FileHeader,
                                  Library->MappedAddress,
                                  Exports->AddressOfNames,
                                  &Library->LastRvaSection);
    AddressOfOrdinals = ImageRvaToVa(Library->FileHeader,
                                     Library->MappedAddress, 
                                     Exports->AddressOfNameOrdinals, 
                                     &Library->LastRvaSection);
    AddressOfPointers = ImageRvaToVa(Library->FileHeader, 
                                     Library->MappedAddress, 
                                     Exports->AddressOfFunctions, 
                                     &Library->LastRvaSection);

    /* Check if we're binding by name... */
    if (OrdinalNumber == 0xffff)
    {
        /* Do a full search for the ordinal */
        for (HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++)
        {
            /* Get the Export Name */
            ExportName = ImageRvaToVa(Library->FileHeader,
                                      Library->MappedAddress, 
                                      (ULONG)AddressOfNames[HintIndex], 
                                      &Library->LastRvaSection);
            
            /* Check if it's the one we want */
            if (!strcmp(FunctionName, ExportName))
            {
                OrdinalNumber = AddressOfOrdinals[HintIndex];
                break;
            }
        }

        /* Make sure it's valid */
        if (HintIndex >= Exports->NumberOfNames) return ForwarderString;
    }

    /* Get the Forwarded Address */
    ForwardedAddress = AddressOfPointers[OrdinalNumber] +
                       OptionalHeader->ImageBase;

    /* Loop the forwarders to see if this DLL was already processed */
    NextForwarder = &BoundImportDescriptor->Forwarders;
    while ((Forwarder = *NextForwarder))
    {
        /* Check for a name match */
        if (!_stricmp(DllName, Forwarder->ModuleName)) break;

        /* Move to the next one */
        NextForwarder = &Forwarder->Next;
    }

    /* Check if we've went through them all without luck */
    if (!Forwarder)
    {
        /* Allocate a forwarder structure */
        Forwarder = HeapAlloc(IMAGEHLP_hHeap,
                              HEAP_ZERO_MEMORY,
                              sizeof(BOUND_FORWARDER_REFS));

        /* Set the name */
        Forwarder->ModuleName = BindpCaptureImportModuleName(DllName);        

        /* Increase the number of forwarders */
        BoundImportDescriptor->ForwaderReferences++;

        /* Link it */
        *NextForwarder = Forwarder;
    }

    /* Set the timestamp */
    Forwarder->TimeDateStamp = Library->FileHeader->FileHeader.TimeDateStamp;

    /* Load DLL's Exports */
    ExportsBase = (ULONG_PTR)ImageDirectoryEntryToData(Library->MappedAddress,
                                                       TRUE,
                                                       IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                       &ExportSize) -
                  (ULONG_PTR)Library->MappedAddress;
    
    /* Convert to VA */
    ExportsBase += OptionalHeader->ImageBase;

    /* Is this yet another Forward? */
    DPRINT("I've thunked it\n");
    if ((ForwardedAddress > ExportsBase) &&
        (ForwardedAddress < (ExportsBase + ExportSize)))
    {
        /* Update the string pointer */
        ForwarderString = ImageRvaToVa(Library->FileHeader, 
                                       Library->MappedAddress, 
                                       AddressOfPointers[OrdinalNumber], 
                                       &Library->LastRvaSection); 
        goto NextForwarder;
    }
    else
    {
        /* Update the pointer and return success */
        ForwarderString = (PUCHAR)ForwardedAddress;
        *ForwarderBound = TRUE;
    }

    /* Return the pointer */
    return ForwarderString;
}

BOOL
IMAGEAPI
BindpLookupThunk(PIMAGE_THUNK_DATA Thunk,
                 PLOADED_IMAGE Image,
                 PIMAGE_THUNK_DATA BoundThunk,
                 PIMAGE_THUNK_DATA ThunkFunction,
                 PLOADED_IMAGE Library,
                 PIMAGE_EXPORT_DIRECTORY Exports,
                 PIMPORT_DESCRIPTOR BoundImportDescriptor,
                 LPSTR DllPath,
                 PULONG *Forwarders)
{
    PULONG AddressOfNames;
    PUSHORT AddressOfOrdinals;
    PULONG AddressOfPointers;
    PIMAGE_IMPORT_BY_NAME ImportName;
    ULONG OrdinalNumber = 0;
    USHORT HintIndex;
    LPSTR ExportName;
    ULONG_PTR ExportsBase;
    ULONG ExportSize;
    UCHAR NameBuffer[32];
    PIMAGE_OPTIONAL_HEADER OptionalHeader = NULL;
    PIMAGE_OPTIONAL_HEADER LibraryOptionalHeader = NULL;
    BOOL ForwarderBound = FALSE;
    PUCHAR ForwarderName;
    DPRINT("Binding a Thunk\n");

    /* Get the Pointers to the Tables */
    AddressOfNames = ImageRvaToVa(Library->FileHeader,
                                  Library->MappedAddress,
                                  Exports->AddressOfNames,
                                  &Library->LastRvaSection);
    AddressOfOrdinals = ImageRvaToVa(Library->FileHeader,
                                     Library->MappedAddress, 
                                     Exports->AddressOfNameOrdinals, 
                                     &Library->LastRvaSection);
    AddressOfPointers = ImageRvaToVa(Library->FileHeader, 
                                     Library->MappedAddress, 
                                     Exports->AddressOfFunctions, 
                                     &Library->LastRvaSection);

    /* Get the Optional Headers */
    OptionalHeader = &Image->FileHeader->OptionalHeader;
    LibraryOptionalHeader = &Library->FileHeader->OptionalHeader;
    
    /* Import by Ordinal */
    if (IMAGE_SNAP_BY_ORDINAL(Thunk->u1.Ordinal) == TRUE)
    {
        /* Get the ordinal number and pointer to the name */
        OrdinalNumber = (IMAGE_ORDINAL(Thunk->u1.Ordinal) - Exports->Base);
        ImportName = (PIMAGE_IMPORT_BY_NAME)NameBuffer;

        /* Setup the name for this ordinal */
        sprintf(ImportName->Name, "Ordinal%lx\n", OrdinalNumber);
    }
    else
    {    
        /* Import by Name, get the data */
        ImportName = ImageRvaToVa(Image->FileHeader,
                                  Image->MappedAddress, 
                                  (ULONG)Thunk->u1.AddressOfData, 
                                  &Image->LastRvaSection);

        /* Get the hint and see if we can use it */
        OrdinalNumber = (USHORT)(Exports->NumberOfFunctions + 1);
        HintIndex = ImportName->Hint;
        if (HintIndex < Exports->NumberOfNames)
        {
            /* Hint seems valid, get the export name */
            ExportName = ImageRvaToVa(Library->FileHeader,
                                      Library->MappedAddress, 
                                      (ULONG)AddressOfNames[HintIndex], 
                                      &Library->LastRvaSection);
            /* Check if it's the one we want */
            if (!strcmp(ImportName->Name, ExportName))
            {
                OrdinalNumber = AddressOfOrdinals[HintIndex];
            }
        }

        /* If the ordinal isn't valid, we'll have to do a long loop */
        if (OrdinalNumber >= Exports->NumberOfFunctions)
        {
            for (HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++)
            {
                /* Get the Export Name */
                ExportName = ImageRvaToVa(Library->FileHeader,
                                          Library->MappedAddress, 
                                          (ULONG)AddressOfNames[HintIndex], 
                                          &Library->LastRvaSection);
            
                /* Check if it's the one we want */
                if (!strcmp(ImportName->Name, ExportName))
                {
                    OrdinalNumber = AddressOfOrdinals[HintIndex];
                    break;
                }
            }

            /* Make sure it's valid now */
            if (OrdinalNumber >= Exports->NumberOfFunctions) return FALSE;
        }
    }

    /* Write the Pointer */
    ThunkFunction->u1.Function = AddressOfPointers[OrdinalNumber] +
                                 LibraryOptionalHeader->ImageBase;

    /* Load DLL's Exports */
    ExportsBase = (ULONG_PTR)ImageDirectoryEntryToData(Library->MappedAddress,
                                                       TRUE, 
                                                       IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                       &ExportSize) -
                  (ULONG_PTR)Library->MappedAddress;
    
    /* RVA to VA */
    ExportsBase += LibraryOptionalHeader->ImageBase;

    /* Check if the Export is forwarded (meaning that it's pointer is inside the Export Table) */
    if ((ThunkFunction->u1.Function > ExportsBase) &&
        (ThunkFunction->u1.Function < ExportsBase + ExportSize))
    {
        /* Make sure we have a descriptor */
        if (BoundImportDescriptor)
        {
            DPRINT("This Thunk is a forward...calling forward thunk bounder\n");

            /* Get the VA of the pointer containg the name */
            ForwarderName = ImageRvaToVa(Library->FileHeader,
                                         Library->MappedAddress,
                                         AddressOfPointers[OrdinalNumber],
                                         &Library->LastRvaSection);

            /* Replace the Forwarder String by the actual name */
            ThunkFunction->u1.ForwarderString =
                PtrToUlong(BindpAddForwarderReference(Image->ModuleName,
                                                      ImportName->Name,
                                                      BoundImportDescriptor,
                                                      DllPath,
                                                      ForwarderName,
                                                      &ForwarderBound));
        }

        /* Check if it wasn't bound */
        if (!ForwarderBound)
        {
            /* Set the chain to the ordinal to reflect this */
            **Forwarders = (ULONG)(ThunkFunction - BoundThunk);
            *Forwarders = (PULONG)&ThunkFunction->u1.Ordinal;
        }
    }

    /* Return Success */
    return TRUE;
}

PIMPORT_DESCRIPTOR
IMAGEAPI
BindpAddImportDescriptor(PIMPORT_DESCRIPTOR *BoundImportDescriptor,
                         PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor,
                         LPSTR DllName,
                         PLOADED_IMAGE Image)
{
    PIMPORT_DESCRIPTOR Descriptor, *NextDescriptor;

    /* Loop descriptors and check if this library has already been bound */
    NextDescriptor = BoundImportDescriptor;
    while ((Descriptor = *NextDescriptor))
    {
        /* Compare the names and return the descriptor if found */
        if (!_stricmp(Descriptor->ModuleName, DllName)) return Descriptor;
        
        /* Move to the next one */
        NextDescriptor = &Descriptor->Next;
    }

    /* Allocate a new descriptor */
    Descriptor = HeapAlloc(IMAGEHLP_hHeap,
                           HEAP_ZERO_MEMORY,
                           sizeof(IMPORT_DESCRIPTOR));

    /* Set its Data and check if we have a valid loaded image */
    Descriptor->ModuleName = BindpCaptureImportModuleName(DllName);
    *NextDescriptor = Descriptor;
    if (Image)
    {
        /* Save the time stamp */
        Descriptor->TimeDateStamp = Image->FileHeader->FileHeader.TimeDateStamp;
    }
    
    /* Return the descriptor */
    return Descriptor;
}

VOID
IMAGEAPI
BindpWalkAndProcessImports(PLOADED_IMAGE File,
                           LPSTR DllPath,
                           PBOOLEAN UpdateImage)
{
    PIMAGE_IMPORT_DESCRIPTOR Imports;
    PIMAGE_EXPORT_DIRECTORY Exports;
    ULONG SizeOfImports;
    ULONG SizeOfExports;
    ULONG SizeOfThunks;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_FILE_HEADER FileHeader;
    LPSTR ImportedLibrary;
    PLOADED_IMAGE LoadedLibrary;
    ULONG TopForwarderChain;
    PULONG ForwarderChain;
    PIMPORT_DESCRIPTOR TopBoundDescriptor = NULL, BoundImportDescriptor;
    PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportTable, OldBoundImportTable;
    PIMAGE_THUNK_DATA Thunks, TempThunk;
    PIMAGE_THUNK_DATA BoundThunks, TempBoundThunk;
    ULONG ThunkCount = 0;
    ULONG Thunk;
    ULONG BoundImportTableSize, OldBoundImportTableSize;
    ULONG VirtBytesFree, HeaderBytesFree, FirstFreeByte, PhysBytesFree;
    BOOL ThunkStatus;
    DPRINT("BindpWalkAndBindImports Called\n");

    /* Assume untouched image */
    *UpdateImage = FALSE;

    /* Load the Import Descriptor */
    Imports = ImageDirectoryEntryToData(File->MappedAddress,
                                        FALSE, 
                                        IMAGE_DIRECTORY_ENTRY_IMPORT, 
                                        &SizeOfImports);
    if (!Imports) return;

    /* Read the File Header */
    FileHeader = &File->FileHeader->FileHeader;
    OptionalHeader = &File->FileHeader->OptionalHeader;

    /* Get the old Bound Import Table, if any */
    OldBoundImportTable = ImageDirectoryEntryToData(File->MappedAddress,
                                                    FALSE,
                                                    IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                                                    &OldBoundImportTableSize);

    /* For each Import */
    while(Imports)
    {
        /* Make sure we have a name */
        if (!Imports->Name) break;

        /* Which DLL is being Imported */
        ImportedLibrary = ImageRvaToVa(File->FileHeader, 
                                       File->MappedAddress, 
                                       Imports->Name, 
                                       &File->LastRvaSection);
        if (ImportedLibrary)
        {
            DPRINT("Loading Imported DLL: %s \n", ImportedLibrary);
            
            /* Load the DLL */
            LoadedLibrary = ImageLoad(ImportedLibrary, DllPath);
            if (!LoadedLibrary)
            {
                /* Create the descriptor, even if we failed */
                BindpAddImportDescriptor(&TopBoundDescriptor,
                                         Imports,
                                         ImportedLibrary,
                                         LoadedLibrary);

                /* Move on the next file */
                Imports++;
                continue;
            }

            /* Now load the Exports */
            DPRINT("DLL Loaded at: %p \n", LoadedLibrary->MappedAddress);
            Exports = ImageDirectoryEntryToData(LoadedLibrary->MappedAddress, 
                                                FALSE, 
                                                IMAGE_DIRECTORY_ENTRY_EXPORT, 
                                                &SizeOfExports);

            /* Move on, if we don't have exports */
            if (!Exports) continue;

            /* And load the Thunks */
            Thunks = ImageRvaToVa(File->FileHeader, 
                                  File->MappedAddress, 
                                  (ULONG)Imports->OriginalFirstThunk, 
                                  &File->LastRvaSection);

            /* No actual Exports (UPX Packer can do this */
            if (!(Thunks) || !(Thunks->u1.Function)) continue;
        
            /* Create Bound Import Descriptor */
            DPRINT("Creating Bound Descriptor for this DLL\n");
            BoundImportDescriptor = BindpAddImportDescriptor(&TopBoundDescriptor,
                                                             Imports,
                                                             ImportedLibrary,
                                                             LoadedLibrary);

            /* Count how many Thunks we have */
            ThunkCount = 0;
            TempThunk = Thunks;
            while (TempThunk->u1.AddressOfData)
            {
                ThunkCount++;
                TempThunk++;
            }

            /* Allocate Memory for the Thunks we will Bind */
            SizeOfThunks = ThunkCount * sizeof(*TempBoundThunk);
            BoundThunks = HeapAlloc(IMAGEHLP_hHeap,
                                    HEAP_ZERO_MEMORY,
                                    SizeOfThunks);

            /* Setup the initial data pointers */
            DPRINT("Binding Thunks\n");
            TempThunk = Thunks;
            TempBoundThunk = BoundThunks;
            TopForwarderChain = -1;
            ForwarderChain = &TopForwarderChain;

            /* Loop for every thunk */
            for (Thunk = 0; Thunk < ThunkCount; Thunk++)
            {
                /* Bind it */
                ThunkStatus = BindpLookupThunk(TempThunk,
                                               File,
                                               BoundThunks,
                                               TempBoundThunk,
                                               LoadedLibrary,
                                               Exports,
                                               BoundImportDescriptor,
                                               DllPath,
                                               &ForwarderChain);
                /* Check if binding failed */
                if (!ThunkStatus)
                {
                    /* If we have a descriptor */
                    if (BoundImportDescriptor)
                    {
                        /* Zero the timestamp */
                        BoundImportDescriptor->TimeDateStamp = 0;
                    }

                    /* Quit the loop */
                    break;
                }

                /* Move on */
                TempThunk++;
                TempBoundThunk++;
            }

            /* Load the Second Thunk Array */
            TempThunk = ImageRvaToVa(File->FileHeader, 
                                     File->MappedAddress, 
                                     (ULONG)Imports->FirstThunk, 
                                     &File->LastRvaSection);
            if (TempThunk)
            {
                /* Check if the forwarder chain changed */
                if (TopForwarderChain != -1)
                {
                    /* It did. Update the chain and let caller know */
                    *ForwarderChain = -1;
                    *UpdateImage = TRUE;
                }

                /* Check if we're not pointing at the new top chain */
                if (Imports->ForwarderChain != TopForwarderChain)
                {
                    /* Update it, and let the caller know */
                    Imports->ForwarderChain = TopForwarderChain;
                    *UpdateImage = TRUE;
                }

                /* Check if thunks have changed */
                if (memcmp(TempThunk, BoundThunks, SizeOfThunks))
                {
                    /* Copy the Pointers and let caller know */
                    DPRINT("Copying Bound Thunks\n");
                    RtlCopyMemory(TempThunk, BoundThunks, SizeOfThunks);
                    *UpdateImage = TRUE;
                }

                /* Check if we have no bound entries */
                if (!TopBoundDescriptor)
                {
                    /* Check if the timestamp is different */
                    if (Imports->TimeDateStamp != FileHeader->TimeDateStamp)
                    {
                        /* Update it, and let the caller knmow */
                        Imports->TimeDateStamp = FileHeader->TimeDateStamp;
                        *UpdateImage = TRUE;
                    }
                }
                else if ((Imports->TimeDateStamp != 0xFFFFFFFF))
                {
                    /* Invalidate the timedate stamp */
                    Imports->TimeDateStamp = 0xFFFFFFFF;
                }
            }

            /* Free the Allocated Memory */
            HeapFree(IMAGEHLP_hHeap, 0, BoundThunks);

            DPRINT("Moving to next File\n");
            Imports++;
        }
    }

    /* Create the Bound Import Table */
    DPRINT("Creating Bound Import Section\n");
    BoundImportTable = BindpCreateNewImportSection(&TopBoundDescriptor,
                                                   &BoundImportTableSize);

    /* Check if the import table changed */
    if (OldBoundImportTableSize != BoundImportTableSize)
    {
        /* Let the caller know */
        *UpdateImage = TRUE;
    }

    /* 
     * At this point, check if anything that we've done until now has resulted
     * in the image being touched. If not, then we'll simply return to caller.
     */
    if (!(*UpdateImage)) return;

    /* Check if we have a new table */
    if (BoundImportTable)
    {
        /* Zero it out */
        OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
        OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

        /* Check if we have enough space */
        DPRINT("Calculating Space\n");
        FirstFreeByte = GetImageUnusedHeaderBytes(File, &VirtBytesFree);
        HeaderBytesFree = File->Sections->VirtualAddress -
                          OptionalHeader->SizeOfHeaders + VirtBytesFree;
        PhysBytesFree = File->Sections->PointerToRawData -
                        OptionalHeader->SizeOfHeaders + VirtBytesFree;

        /* Check if we overflowed */
        if (BoundImportTableSize > VirtBytesFree)
        {
            /* Check if we have no space a tall */
            if (BoundImportTableSize > HeaderBytesFree)
            {
                DPRINT1("Not enough Space\n");
                return; /* Fail...not enough space */
            }

            /* Check if we have space on disk to enlarge it */
            if (BoundImportTableSize <= PhysBytesFree)
            {
                /* We have enough NULLs to add it, simply enlarge header data */
                DPRINT("Header Recalculation\n");
                OptionalHeader->SizeOfHeaders = OptionalHeader->SizeOfHeaders -
                                                VirtBytesFree +
                                                BoundImportTableSize +
                                                ((OptionalHeader->FileAlignment - 1) &
                                                ~(OptionalHeader->FileAlignment - 1));
            }
            else 
            {
                /* Resize the Headers */
                DPRINT1("UNIMPLEMENTED: Header Resizing\n");

                /* Recalculate Headers */
                FileHeader = &File->FileHeader->FileHeader;
                OptionalHeader = &File->FileHeader->OptionalHeader;
            }
        }
    
        /* Set Bound Import Table Data */
        OptionalHeader->DataDirectory
            [IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = FirstFreeByte;
        OptionalHeader->DataDirectory
            [IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = BoundImportTableSize;
    
        /* Copy the Bound Import Table */
        DPRINT("Copying Bound Import Table\n");
        RtlCopyMemory(File->MappedAddress + FirstFreeByte,
                      BoundImportTable,
                      BoundImportTableSize);

        /* Free the data */
        HeapFree(IMAGEHLP_hHeap, 0, BoundImportTable);
    }
    
}

/*
 * @implemented
 */
BOOL 
IMAGEAPI 
BindImageEx(IN DWORD Flags,
            IN LPSTR ImageName,
            IN LPSTR DllPath,
            IN LPSTR SymbolPath,
            IN PIMAGEHLP_STATUS_ROUTINE StatusRoutine)
{
    LOADED_IMAGE FileData;
    PLOADED_IMAGE File;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader;
    ULONG CheckSum, HeaderCheckSum, OldChecksum;
    SYSTEMTIME SystemTime;
    FILETIME LastWriteTime;
    BOOLEAN UpdateImage;
    DWORD DataSize;
    DPRINT("BindImageEx Called for: %s \n", ImageName);

    /* Set and Clear Buffer */
    File = &FileData;
    RtlZeroMemory(File, sizeof(*File));

    /* Request Image Data */
    if (MapAndLoad(ImageName, DllPath, File, TRUE, FALSE))
    {
        /* Write the image's name */
        DPRINT("Image Mapped and Loaded\n");
        File->ModuleName = ImageName;

        /* Check if the image is valid and if it should be bound */
        if ((File->FileHeader) &&
            ((Flags & BIND_ALL_IMAGES) || (!File->fSystemImage)))
        {
            /* Get the optional header */
            FileHeader = &File->FileHeader->FileHeader;
            OptionalHeader = &File->FileHeader->OptionalHeader;

            /* Check if this image should be bound */
            if (OptionalHeader->DllCharacteristics &
                IMAGE_DLLCHARACTERISTICS_NO_BIND)
            {
                /* Don't bind it */
                goto Skip;
            }

            /* Check if the image has security data */
            if ((ImageDirectoryEntryToData(File->MappedAddress,
                                           FALSE,
                                           IMAGE_DIRECTORY_ENTRY_SECURITY,
                                           &DataSize)) || DataSize)
            {
                /* It does, skip it */
                goto Skip;
            }

            /* Read Import Table */
            BindpWalkAndProcessImports(File, DllPath, &UpdateImage);

            /* Check if we need to update the image */
            if ((UpdateImage) && (File->hFile != INVALID_HANDLE_VALUE))
            {
                /* FIXME: Update symbols */
        
                /* Update Checksum */
                DPRINT("Binding Completed, getting Checksum\n");
                OldChecksum = File->FileHeader->OptionalHeader.CheckSum;
                CheckSumMappedFile(File->MappedAddress,
                                   GetFileSize(File->hFile, NULL),
                                   &HeaderCheckSum,
                                   &CheckSum);
                File->FileHeader->OptionalHeader.CheckSum = CheckSum;

                /* Save Changes */
                DPRINT("Saving Changes to file\n");
                FlushViewOfFile(File->MappedAddress, File->SizeOfImage);

                /* Save new Modified Time */
                DPRINT("Setting time\n");
                GetSystemTime(&SystemTime);
                SystemTimeToFileTime(&SystemTime, &LastWriteTime);
                SetFileTime(File->hFile, NULL, NULL, &LastWriteTime);
            }
        }
    }

Skip:

    /* Unmap the image */
    UnmapViewOfFile(File->MappedAddress);

    /* Close the handle if it's valid */
    if (File->hFile != INVALID_HANDLE_VALUE) CloseHandle(File->hFile);

    /* Unload all the images if we're not supposed to cache them */
    if (!(Flags & BIND_CACHE_IMPORT_DLLS)) UnloadAllImages();
   
    /* Return success */
    DPRINT("Done\n");
    return TRUE;
}

/*
 * @implemented
 */
BOOL
IMAGEAPI
BindImage(LPSTR ImageName,
          LPSTR DllPath,
          LPSTR SymbolPath)
{
    /* Call the newer API */
    return BindImageEx(0,
                       ImageName,
                       DllPath,
                       SymbolPath,
                       NULL);
}

/*
 * @unimplemented
 */
BOOL
IMAGEAPI
ReBaseImage(LPSTR CurrentImageName,
            LPSTR SymbolPath,
            BOOL fReBase,
            BOOL fRebaseSysfileOk,
            BOOL fGoingDown,
            ULONG CheckImageSize,
            ULONG *OldImageSize,
            ULONG *OldImageBase,
            ULONG *NewImageSize,
            ULONG *NewImageBase,
            ULONG TimeStamp)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
VOID
IMAGEAPI
RemoveRelocations(PCHAR ImageName)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @implemented
 */
BOOL
IMAGEAPI
TouchFileTimes(HANDLE FileHandle,
               LPSYSTEMTIME lpSystemTime)
{
    FILETIME FileTime;
    SYSTEMTIME SystemTime;
  
    if(lpSystemTime == NULL)
    {
        GetSystemTime(&SystemTime);
        lpSystemTime = &SystemTime;
    }

    return (SystemTimeToFileTime(lpSystemTime,
                                 &FileTime) &&
            SetFileTime(FileHandle,
                        NULL,
                        NULL,
                        &FileTime));
}
