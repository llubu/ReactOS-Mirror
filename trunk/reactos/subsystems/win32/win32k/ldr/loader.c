/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* $Id$
 *
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


typedef struct _DRIVERS
{
	LIST_ENTRY ListEntry;
	HANDLE ImageHandle;
	UNICODE_STRING DriverName;
}DRIVERS, *PDRIVERS;

extern LIST_ENTRY GlobalDriverListHead;

/*
 * Blatantly stolen from ldr/utils.c in ntdll.  I can't link ntdll from
 * here, though.
 */
NTSTATUS APIENTRY
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PULONG AddressPtr;
   ULONG i = 0;

   DPRINT("LdrGetProcedureAddress (BaseAddress %x Name %Z Ordinal %lu ProcedureAddress %x)\n",
          BaseAddress, Name, Ordinal, ProcedureAddress);

   /* Get the pointer to the export directory */
   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData (BaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_EXPORT,
                                              &i);

   DPRINT("ExportDir %x i %lu\n", ExportDir, i);

   if (!ExportDir || !i || !ProcedureAddress)
     {
        return STATUS_INVALID_PARAMETER;
     }

   AddressPtr = (PULONG)((ULONG_PTR)BaseAddress + (ULONG)ExportDir->AddressOfFunctions);
   if (Name && Name->Length)
     {
        /* by name */
        OrdinalPtr = (PUSHORT)((ULONG_PTR)BaseAddress + (ULONG)ExportDir->AddressOfNameOrdinals);
        NamePtr = (PULONG)((ULONG_PTR)BaseAddress + (ULONG)ExportDir->AddressOfNames);
        for( i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
          {
             if (!strcmp(Name->Buffer, (char*)((ULONG_PTR)BaseAddress + *NamePtr)))
               {
                  *ProcedureAddress = (PVOID)((ULONG_PTR)BaseAddress + (ULONG)AddressPtr[*OrdinalPtr]);
                  return STATUS_SUCCESS;
               }
          }
        DPRINT1("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
     }
   else
     {
        /* by ordinal */
        Ordinal &= 0x0000FFFF;
        if (Ordinal - ExportDir->Base < ExportDir->NumberOfFunctions)
          {
             *ProcedureAddress = (PVOID)((ULONG_PTR)BaseAddress + (ULONG_PTR)AddressPtr[Ordinal - ExportDir->Base]);
             return STATUS_SUCCESS;
          }
        DPRINT1("LdrGetProcedureAddress: Can't resolve symbol @%d\n", Ordinal);
  }

   return STATUS_PROCEDURE_NOT_FOUND;
}

PVOID APIENTRY
EngFindImageProcAddress(IN HANDLE Module,
			IN LPSTR ProcName)
{
  PVOID Function;
  NTSTATUS Status;
  ANSI_STRING ProcNameString;
  unsigned i;
  static struct
    {
      PCSTR ProcName;
      PVOID ProcAddress;
    }
  Win32kExports[] =
    {
      { "BRUSHOBJ_hGetColorTransform",    BRUSHOBJ_hGetColorTransform    },
      { "EngAlphaBlend",                  EngAlphaBlend                  },
      { "EngClearEvent",                  EngClearEvent                  },
      { "EngControlSprites",              EngControlSprites              },
      { "EngCreateEvent",                 EngCreateEvent                 },
      { "EngDeleteEvent",                 EngDeleteEvent                 },
      { "EngDeleteFile",                  EngDeleteFile                  },
      { "EngDeleteSafeSemaphore",         EngDeleteSafeSemaphore         },
      { "EngDeleteWnd",                   EngDeleteWnd                   },
      { "EngDitherColor",                 EngDitherColor                 },
      { "EngGetPrinterDriver",            EngGetPrinterDriver            },
      { "EngGradientFill",                EngGradientFill                },
      { "EngHangNotification",            EngHangNotification            },
      { "EngInitializeSafeSemaphore",     EngInitializeSafeSemaphore     },
      { "EngLockDirectDrawSurface",       EngLockDirectDrawSurface       },
      { "EngLpkInstalled",                EngLpkInstalled                },
      { "EngMapEvent",                    EngMapEvent                    },
      { "EngMapFile",                     EngMapFile                     },
      { "EngMapFontFileFD",               EngMapFontFileFD               },
      { "EngModifySurface",               EngModifySurface               },
      { "EngMovePointer",                 EngMovePointer                 },
      { "EngPlgBlt",                      EngPlgBlt                      },
      { "EngQueryDeviceAttribute",        EngQueryDeviceAttribute        },
      { "EngQueryPalette",                EngQueryPalette                },
      { "EngQuerySystemAttribute",        EngQuerySystemAttribute        },
      { "EngReadStateEvent",              EngReadStateEvent              },
      { "EngRestoreFloatingPointState",   EngRestoreFloatingPointState   },
      { "EngSaveFloatingPointState",      EngSaveFloatingPointState      },
      { "EngSetEvent",                    EngSetEvent                    },
      { "EngSetPointerShape",             EngSetPointerShape             },
      { "EngSetPointerTag",               EngSetPointerTag               },
      { "EngStretchBltROP",               EngStretchBltROP               },
      { "EngTransparentBlt",              EngTransparentBlt              },
      { "EngUnlockDirectDrawSurface",     EngUnlockDirectDrawSurface     },
      { "EngUnmapEvent",                  EngUnmapEvent                  },
      { "EngUnmapFile",                   EngUnmapFile                   },
      { "EngUnmapFontFileFD",             EngUnmapFontFileFD             },
      { "EngWaitForSingleObject",         EngWaitForSingleObject         },
      { "FONTOBJ_pfdg",                   FONTOBJ_pfdg                   },
      { "FONTOBJ_pjOpenTypeTablePointer", FONTOBJ_pjOpenTypeTablePointer },
      { "FONTOBJ_pQueryGlyphAttrs",       FONTOBJ_pQueryGlyphAttrs       },
      { "FONTOBJ_pwszFontFilePaths",      FONTOBJ_pwszFontFilePaths      },
      { "HeapVidMemAllocAligned",         HeapVidMemAllocAligned         },
      { "HT_Get8BPPMaskPalette",          HT_Get8BPPMaskPalette          },
      { "STROBJ_bEnumPositionsOnly",      STROBJ_bEnumPositionsOnly      },
      { "STROBJ_bGetAdvanceWidths",       STROBJ_bGetAdvanceWidths       },
      { "STROBJ_fxBreakExtra",            STROBJ_fxBreakExtra            },
      { "STROBJ_fxCharacterExtra",        STROBJ_fxCharacterExtra        },
      { "VidMemFree",                     VidMemFree                     },
      { "XLATEOBJ_hGetColorTransform",    XLATEOBJ_hGetColorTransform    }
    };

  if (NULL == Module)
    {
      DPRINT("Looking for win32k export %s\n", ProcName);
      for (i = 0; i < sizeof(Win32kExports) / sizeof(Win32kExports[0]); i++)
        {
          if (0 == strcmp(ProcName, Win32kExports[i].ProcName))
            {
              DPRINT("Found it index %u address %p\n", i, Win32kExports[i].ProcName);
              return Win32kExports[i].ProcAddress;
            }
        }
      return NULL;
    }
  RtlInitAnsiString(&ProcNameString, ProcName);
  Status = LdrGetProcedureAddress(Module,
				  &ProcNameString,
				  0,
				  &Function);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }
  return(Function);
}


/*
 * @implemented
 */
HANDLE
APIENTRY
EngLoadImage (LPWSTR DriverName)
{
	HANDLE hImageHandle = NULL;
	SYSTEM_GDI_DRIVER_INFORMATION GdiDriverInfo;
	NTSTATUS Status;

	RtlInitUnicodeString(&GdiDriverInfo.DriverName, DriverName);
	if( !IsListEmpty(&GlobalDriverListHead) )
	{
		PLIST_ENTRY CurrentEntry = GlobalDriverListHead.Flink;
		PDRIVERS Current;
		/* probably the driver was already loaded, let's try to find it out */
		while( CurrentEntry != &GlobalDriverListHead )
		{
			Current = CONTAINING_RECORD(CurrentEntry, DRIVERS, ListEntry);
			if( Current && (0 == RtlCompareUnicodeString(&GdiDriverInfo.DriverName, &Current->DriverName, FALSE)) ) {
				hImageHandle = Current->ImageHandle;
				break;
			}
			CurrentEntry = CurrentEntry->Flink;
		};
	}

	if( !hImageHandle )
	{
		/* the driver was not loaded before, so let's do that */
		Status = ZwSetSystemInformation(SystemLoadGdiDriverInformation, &GdiDriverInfo, sizeof(SYSTEM_GDI_DRIVER_INFORMATION));
		if (!NT_SUCCESS(Status)) {
			DPRINT1("ZwSetSystemInformation failed with Status 0x%lx\n", Status);
		}
		else {
            PDRIVERS DriverInfo;
			hImageHandle = (HANDLE)GdiDriverInfo.ImageAddress;
			DriverInfo = ExAllocatePool(PagedPool, sizeof(DRIVERS));
			DriverInfo->DriverName.MaximumLength = GdiDriverInfo.DriverName.MaximumLength;
			DriverInfo->DriverName.Length = GdiDriverInfo.DriverName.Length;
			DriverInfo->DriverName.Buffer = ExAllocatePool(PagedPool, GdiDriverInfo.DriverName.MaximumLength);
			RtlCopyUnicodeString(&DriverInfo->DriverName, &GdiDriverInfo.DriverName);
			DriverInfo->ImageHandle = hImageHandle;
			InsertHeadList(&GlobalDriverListHead, &DriverInfo->ListEntry);
		}
	}

	return hImageHandle;
}

VOID
APIENTRY
EngUnloadImage ( IN HANDLE hModule )
{
  NTSTATUS Status;

  DPRINT("hModule 0x%x\n", hModule);

  Status = ZwSetSystemInformation(SystemUnloadGdiDriverInformation,
    &hModule, sizeof(HANDLE));

  if(!NT_SUCCESS(Status))
  {
    DPRINT1("ZwSetSystemInformation failed with status 0x%08X\n",
      Status);
  }
  else
  {
	  /* remove from the list */
	  if( !IsListEmpty(&GlobalDriverListHead) )
	  {
		  PLIST_ENTRY CurrentEntry = GlobalDriverListHead.Flink;
		  PDRIVERS Current;
		  /* probably the driver was already loaded, let's try to find it out */
		  while( CurrentEntry != &GlobalDriverListHead )
		  {
			  Current = CONTAINING_RECORD(CurrentEntry, DRIVERS, ListEntry);

			  if( Current ) {
				  if(Current->ImageHandle == hModule) {
					  ExFreePool(Current->DriverName.Buffer);
					  RemoveEntryList(&Current->ListEntry);
					  ExFreePool(Current);
					  break;
				  }
			  }
			  CurrentEntry = CurrentEntry->Flink;
		  };
	  }
  }
}

/* EOF */
