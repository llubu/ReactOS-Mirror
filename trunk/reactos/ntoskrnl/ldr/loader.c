/* $Id: loader.c,v 1.110 2002/06/12 23:30:36 ekohl Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Jason Filby (jasonfilby@yahoo.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *   DW   22/05/98   Created
 *   RJJ  10/12/98   Completed image loader function and added hooks for MZ/PE
 *   RJJ  10/12/98   Built driver loader function and added hooks for PE/COFF
 *   RJJ  10/12/98   Rolled in David's code to load COFF drivers
 *   JM   14/12/98   Built initial PE user module loader
 *   RJJ  06/03/99   Moved user PE loader into NTDLL
 *   JF   26/01/2000 Recoded some parts to retrieve export details correctly
 *   DW   27/06/2000 Removed redundant header files
 *   CSH  11/04/2001 Added automatic loading of module symbols if they exist
 */


/* INCLUDES *****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/kd.h>
#include <internal/io.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/ldr.h>
#include <internal/pool.h>

#ifdef HALDBG
#include <internal/ntosdbg.h>
#else
#define ps(args...)
#endif

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY ModuleListHead;
KSPIN_LOCK ModuleListLock;

LIST_ENTRY ModuleTextListHead;
STATIC MODULE_TEXT_SECTION NtoskrnlTextSection;
STATIC MODULE_TEXT_SECTION LdrHalTextSection;
ULONG_PTR LdrHalBase;

#define TAG_DRIVER_MEM  TAG('D', 'R', 'V', 'M')
#define TAG_SYM_BUF     TAG('S', 'Y', 'M', 'B')

/* FORWARD DECLARATIONS ******************************************************/

NTSTATUS
LdrProcessModule(PVOID ModuleLoadBase,
		 PUNICODE_STRING ModuleName,
		 PMODULE_OBJECT *ModuleObject);

PVOID
LdrGetExportAddress(PMODULE_OBJECT ModuleObject,
		    char *Name,
		    unsigned short Hint);

static VOID
LdrpBuildModuleBaseName(PUNICODE_STRING BaseName,
			PUNICODE_STRING FullName);

static LONG
LdrpCompareModuleNames(IN PUNICODE_STRING String1,
		       IN PUNICODE_STRING String2);


/*  PE Driver load support  */
static NTSTATUS LdrPEProcessModule(PVOID ModuleLoadBase,
                                   PUNICODE_STRING FileName,
                                   PMODULE_OBJECT *ModuleObject);
static PVOID
LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject,
		      PCHAR Name,
		      USHORT Hint);

static PVOID
LdrSafePEGetExportAddress(PVOID ImportModuleBase,
			  PCHAR Name,
			  USHORT Hint);

static PVOID
LdrPEFixupForward(PCHAR ForwardName);


/* FUNCTIONS *****************************************************************/

VOID
LdrInitDebug(PLOADER_MODULE Module, PWCH Name)
{
  PLIST_ENTRY current_entry;
  MODULE_TEXT_SECTION* current;

  current_entry = ModuleTextListHead.Flink;
  while (current_entry != &ModuleTextListHead)
    {
      current = 
	CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION, ListEntry);
      if (wcscmp(current->Name, Name) == 0)
	{
	  break;
	}
      current_entry = current_entry->Flink;
    }

  if (current_entry == &ModuleTextListHead)
    {
      return;
    }
  
  current->SymbolsBase = (PVOID)Module->ModStart;
  current->SymbolsLength = Module->ModEnd - Module->ModStart;
}

VOID
LdrInit1(VOID)
{
  PIMAGE_DOS_HEADER DosHeader;
  PIMAGE_FILE_HEADER FileHeader;
  PIMAGE_OPTIONAL_HEADER OptionalHeader;
  PIMAGE_SECTION_HEADER SectionList;

  InitializeListHead(&ModuleTextListHead);

  /* Setup ntoskrnl.exe text section */
  DosHeader = (PIMAGE_DOS_HEADER) KERNEL_BASE;
  FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD)KERNEL_BASE + 
			  DosHeader->e_lfanew + sizeof(ULONG));
  OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)FileHeader + sizeof(IMAGE_FILE_HEADER));
  SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  NtoskrnlTextSection.Base = KERNEL_BASE;
  NtoskrnlTextSection.Length = SectionList[0].Misc.VirtualSize +
    SectionList[0].VirtualAddress;
  NtoskrnlTextSection.Name = KERNEL_MODULE_NAME;
  NtoskrnlTextSection.SymbolsBase = NULL;
  NtoskrnlTextSection.SymbolsLength = 0;
  InsertTailList(&ModuleTextListHead, &NtoskrnlTextSection.ListEntry);

  /* Setup hal.dll text section */
  DosHeader = (PIMAGE_DOS_HEADER)LdrHalBase;
  FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD)LdrHalBase + 
			  DosHeader->e_lfanew + sizeof(ULONG));
  OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)FileHeader + sizeof(IMAGE_FILE_HEADER));
  SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  LdrHalTextSection.Base = LdrHalBase;
  LdrHalTextSection.Length = SectionList[0].Misc.VirtualSize +
    SectionList[0].VirtualAddress;
  LdrHalTextSection.Name = HAL_MODULE_NAME;
  LdrHalTextSection.SymbolsBase = NULL;
  LdrHalTextSection.SymbolsLength = 0;
  InsertTailList(&ModuleTextListHead, &LdrHalTextSection.ListEntry);
}


VOID
LdrInitModuleManagement(VOID)
{
  PIMAGE_DOS_HEADER DosHeader;
  PMODULE_OBJECT ModuleObject;

  /* Initialize the module list and spinlock */
  InitializeListHead(&ModuleListHead);
  KeInitializeSpinLock(&ModuleListLock);

  /* Create module object for NTOSKRNL */
  ModuleObject = ExAllocatePool(NonPagedPool, sizeof(MODULE_OBJECT));
  assert(ModuleObject != NULL);
  RtlZeroMemory(ModuleObject, sizeof(MODULE_OBJECT));

  /* Initialize ModuleObject data */
  ModuleObject->Base = (PVOID) KERNEL_BASE;
  ModuleObject->Flags = MODULE_FLAG_PE;
  RtlCreateUnicodeString(&ModuleObject->FullName,
			 KERNEL_MODULE_NAME);
  LdrpBuildModuleBaseName(&ModuleObject->BaseName,
			  &ModuleObject->FullName);

  DosHeader = (PIMAGE_DOS_HEADER) KERNEL_BASE;
  ModuleObject->Image.PE.FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD) ModuleObject->Base +
    DosHeader->e_lfanew + sizeof(ULONG));
  ModuleObject->Image.PE.OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)ModuleObject->Image.PE.FileHeader + sizeof(IMAGE_FILE_HEADER));
  ModuleObject->Image.PE.SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)ModuleObject->Image.PE.OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  ModuleObject->EntryPoint = (PVOID) ((DWORD) ModuleObject->Base +
    ModuleObject->Image.PE.OptionalHeader->AddressOfEntryPoint);
  DPRINT("ModuleObject:%08x  entrypoint at %x\n", ModuleObject, ModuleObject->EntryPoint);
  ModuleObject->Length = ModuleObject->Image.PE.OptionalHeader->SizeOfImage;
  ModuleObject->TextSection = &NtoskrnlTextSection;

  InsertTailList(&ModuleListHead,
		 &ModuleObject->ListEntry);

  /* Create module object for HAL */
  ModuleObject = ExAllocatePool(NonPagedPool, sizeof(MODULE_OBJECT));
  assert(ModuleObject != NULL);
  RtlZeroMemory(ModuleObject, sizeof(MODULE_OBJECT));

  /* Initialize ModuleObject data */
  ModuleObject->Base = (PVOID) LdrHalBase;
  ModuleObject->Flags = MODULE_FLAG_PE;

  RtlCreateUnicodeString(&ModuleObject->FullName,
			 HAL_MODULE_NAME);
  LdrpBuildModuleBaseName(&ModuleObject->BaseName,
			  &ModuleObject->FullName);

  DosHeader = (PIMAGE_DOS_HEADER) LdrHalBase;
  ModuleObject->Image.PE.FileHeader =
    (PIMAGE_FILE_HEADER) ((DWORD) ModuleObject->Base +
    DosHeader->e_lfanew + sizeof(ULONG));
  ModuleObject->Image.PE.OptionalHeader = (PIMAGE_OPTIONAL_HEADER)
    ((DWORD)ModuleObject->Image.PE.FileHeader + sizeof(IMAGE_FILE_HEADER));
  ModuleObject->Image.PE.SectionList = (PIMAGE_SECTION_HEADER)
    ((DWORD)ModuleObject->Image.PE.OptionalHeader + sizeof(IMAGE_OPTIONAL_HEADER));
  ModuleObject->EntryPoint = (PVOID) ((DWORD) ModuleObject->Base +
    ModuleObject->Image.PE.OptionalHeader->AddressOfEntryPoint);
  DPRINT("ModuleObject:%08x  entrypoint at %x\n", ModuleObject, ModuleObject->EntryPoint);
  ModuleObject->Length = ModuleObject->Image.PE.OptionalHeader->SizeOfImage;
  ModuleObject->TextSection = &LdrHalTextSection;

  InsertTailList(&ModuleListHead,
		 &ModuleObject->ListEntry);
}


/*
 * load the auto config drivers.
 */
#if 0
static VOID
LdrLoadAutoConfigDriver(LPWSTR RelativeDriverName)
{
   WCHAR TmpFileName [MAX_PATH];
   CHAR Buffer [256];
   UNICODE_STRING	DriverName;
   ULONG x, y, cx, cy;

   HalQueryDisplayParameters(&x, &y, &cx, &cy);
   RtlFillMemory(Buffer, x, ' ');
   Buffer[x] = '\0';
   HalSetDisplayParameters(0, y-1);
   HalDisplayString(Buffer);

   sprintf(Buffer, "Loading %S...\n",RelativeDriverName);
   HalSetDisplayParameters(0, y-1);
   HalDisplayString(Buffer);
   HalSetDisplayParameters(cx, cy);
   //CPRINT("Loading %S\n",RelativeDriverName);

   wcscpy(TmpFileName, L"\\SystemRoot\\system32\\drivers\\");
   wcscat(TmpFileName, RelativeDriverName);
   RtlInitUnicodeString (&DriverName, TmpFileName);

   NtLoadDriver(&DriverName);
}
#endif

#ifdef KDBG

BOOLEAN LdrpReadLine(PCHAR Line,
                     ULONG LineSize,
                     PVOID *Buffer,
                     PULONG Size)
{
  CHAR ch;
  PCHAR Block;
  ULONG Count;

  if (*Size == 0)
    return FALSE;

  ch = ' ';
  Count = 0;
  Block = *Buffer;
  while ((*Size > 0) && (Count < LineSize) && ((ch = *Block) != (CHAR)13))
    {
      *Line = ch;
      Line++;
      Block++;
      Count++;
      *Size -= 1;
    }
  *Line = (CHAR)0;

  if (ch == (CHAR)13)
    {
      Block++;
      *Size -= 1;
    }

  if ((*Size > 0) && (*Block == (CHAR)10))
    {
      Block++;
      *Size -= 1;
    }

  *Buffer = Block;

  return TRUE;
}

ULONG HexL(PCHAR Buffer)
{
  CHAR ch;
  UINT i, j;
  ULONG Value;

  j     = 32;
  i     = 0;
  Value = 0;
  while ((j > 0) && ((ch = Buffer[i]) != ' '))
    {
      j -= 4;
      if ((ch >= '0') && (ch <= '9'))
        Value |= ((ch - '0') << j);
        if ((ch >= 'A') && (ch <= 'F'))
          Value |= ((10 + (ch - 'A')) << j);
        else
          if ((ch >= 'a') && (ch <= 'f'))
            Value |= ((10 + (ch - 'a')) << j);
      i++;
    }
  return Value;
}

PSYMBOL LdrpParseLine(PCHAR Line,
                      PULONG TextBase,
                      PBOOLEAN TextBaseValid,
                      PULONG Alignment)
/*
    Line format: [ADDRESS] <TYPE> <NAME>
    TYPE:
      U = ?
      A = Image information
      t = Symbol in text segment
      T = Symbol in text segment
      d = Symbol in data segment
      D = Symbol in data segment
      b = Symbol in BSS segment
      B = Symbol in BSS segment
      ? = Unknown segment or symbol in unknown segment
*/
{
  ANSI_STRING AnsiString;
  CHAR Buffer[128];
  PSYMBOL Symbol;
  ULONG Address;
  PCHAR Str;
  CHAR Type;

  if ((Line[0] == (CHAR)0) || (Line[0] == ' '))
    return NULL;

  Address = HexL(Line);

  Line = strchr(Line, ' ');
  if (Line == NULL)
    return NULL;

  Line++;
  Type = *Line;

  Line = strchr(Line, ' ');
  if (Line == NULL)
    return NULL;

  Line++;
  Str = strchr(Line, ' ');
  if (Str == NULL)
    strcpy((char*)&Buffer, Line);
  else
    strncpy((char*)&Buffer, Line, Str - Line);

  if ((Type == 'A') && (strcmp((char*)&Buffer, "__section_alignment__")) == 0)
    {
      *Alignment = Address;
      return NULL;
    }

  /* We only want symbols in the .text segment */
  if ((Type != 't') && (Type != 'T'))
    return NULL;

  /* Discard other symbols we can't use */
  if ((Buffer[0] != '_') || ((Buffer[0] == '_') && (Buffer[1] == '_')))
    return NULL;

  Symbol = ExAllocatePool(NonPagedPool, sizeof(SYMBOL));
  if (!Symbol)
    return NULL;

  Symbol->Next = NULL;

  Symbol->RelativeAddress = Address;

  RtlInitAnsiString(&AnsiString, (PCSZ)&Buffer);
  RtlAnsiStringToUnicodeString(&Symbol->Name, &AnsiString, TRUE);

  if (!(*TextBaseValid))
    {
      *TextBase = Address - *Alignment;
      *TextBaseValid = TRUE;
    }

  return Symbol;
}

VOID LdrpLoadModuleSymbolsFromBuffer(
  PMODULE_OBJECT ModuleObject,
  PVOID Buffer,
  ULONG Length)
/*
   Symbols must be sorted by address, e.g.
   "nm --numeric-sort module.sys > module.sym"
 */
{
  PSYMBOL Symbol, CurrentSymbol = NULL;
  BOOLEAN TextBaseValid;
  BOOLEAN Valid;
  ULONG TextBase = 0;
  ULONG Alignment = 0;
  CHAR Line[256];
  ULONG Tmp;

  assert(ModuleObject);

  if (ModuleObject->TextSection == NULL)
    {
      ModuleObject->TextSection = &NtoskrnlTextSection;
    }

  if (ModuleObject->TextSection->Symbols.SymbolCount > 0)
    {
      CPRINT("Symbols are already loaded for %wZ\n", &ModuleObject->BaseName);
      return;
    }

  ModuleObject->TextSection->Symbols.SymbolCount = 0;
  ModuleObject->TextSection->Symbols.Symbols = NULL;
  TextBaseValid = FALSE;
  Valid = FALSE;
  while (LdrpReadLine((PCHAR)&Line, 256, &Buffer, &Length))
    {
      Symbol = LdrpParseLine((PCHAR)&Line, &Tmp, &Valid, &Alignment);

      if ((Valid) && (!TextBaseValid))
        {
          TextBase = Tmp;
          TextBaseValid = TRUE;
        }

      if (Symbol != NULL)
        {
          Symbol->RelativeAddress -= TextBase;

          if (ModuleObject->TextSection->Symbols.Symbols == NULL)
            ModuleObject->TextSection->Symbols.Symbols = Symbol;
          else
            CurrentSymbol->Next = Symbol;

          CurrentSymbol = Symbol;

          ModuleObject->TextSection->Symbols.SymbolCount++;
        }
    }
}

VOID LdrpLoadUserModuleSymbolsFromBuffer(
  PLDR_MODULE ModuleObject,
  PVOID Buffer,
  ULONG Length)
/*
   Symbols must be sorted by address, e.g.
   "nm --numeric-sort module.dll > module.sym"
 */
{
  PSYMBOL Symbol, CurrentSymbol = NULL;
  BOOLEAN TextBaseValid;
  BOOLEAN Valid;
  ULONG TextBase = 0;
  ULONG Alignment = 0;
  CHAR Line[256];
  ULONG Tmp;

  if (ModuleObject->Symbols.SymbolCount > 0)
    {
      DPRINT("Symbols are already loaded for %wZ\n", &ModuleObject->BaseDllName);
      return;
    }

  ModuleObject->Symbols.SymbolCount = 0;
  ModuleObject->Symbols.Symbols = NULL;
  TextBaseValid = FALSE;
  Valid = FALSE;
  while (LdrpReadLine((PCHAR)&Line, 256, &Buffer, &Length))
    {
      Symbol = LdrpParseLine((PCHAR)&Line, &Tmp, &Valid, &Alignment);

      if ((Valid) && (!TextBaseValid))
        {
          TextBase = Tmp;
          TextBaseValid = TRUE;
        }

      if (Symbol != NULL)
        {
          Symbol->RelativeAddress -= TextBase;

          if (ModuleObject->Symbols.Symbols == NULL)
            ModuleObject->Symbols.Symbols = Symbol;
          else
            CurrentSymbol->Next = Symbol;

          CurrentSymbol = Symbol;

          ModuleObject->Symbols.SymbolCount++;
        }
    }
}

VOID
LdrpLoadModuleSymbols(PMODULE_OBJECT ModuleObject)
{
  FILE_STANDARD_INFORMATION FileStdInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR TmpFileName[MAX_PATH];
  UNICODE_STRING Filename;
  LPWSTR Start, Ext;
  HANDLE FileHandle;
  PVOID FileBuffer;
  NTSTATUS Status;
  ULONG Length;
  IO_STATUS_BLOCK IoStatusBlock;

  ModuleObject->TextSection->Symbols.SymbolCount = 0;
  ModuleObject->TextSection->Symbols.Symbols = NULL;

  /*  Get the path to the symbol store  */
  wcscpy(TmpFileName, L"\\SystemRoot\\symbols\\");

  /*  Get the symbol filename from the module name  */
  Start = wcsrchr(ModuleObject->BaseName.Buffer, L'\\');
  if (Start == NULL)
    Start = ModuleObject->BaseName.Buffer;
  else
    Start++;

  Ext = wcsrchr(ModuleObject->BaseName.Buffer, L'.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = wcslen(Start);

  wcsncat(TmpFileName, Start, Length);
  wcscat(TmpFileName, L".sym");
  RtlInitUnicodeString(&Filename, TmpFileName);

  /*  Open the file  */
  InitializeObjectAttributes(&ObjectAttributes,
                             &Filename,
                             0,
                             NULL,
                             NULL);

  Status = ZwOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      0,
                      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not open symbol file: %wZ\n", &Filename);
      return;
    }

  CPRINT("Loading symbols from %wZ...\n", &Filename);

  /*  Get the size of the file  */
  Status = ZwQueryInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileStdInfo,
                                  sizeof(FileStdInfo),
                                  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not get file size\n");
      return;
    }

  /*  Allocate nonpageable memory for symbol file  */
  FileBuffer = ExAllocatePool(NonPagedPool,
                              FileStdInfo.EndOfFile.u.LowPart);

  if (FileBuffer == NULL)
    {
      DPRINT("Could not allocate memory for symbol file\n");
      return;
    }
   
  /*  Load file into memory chunk  */
  Status = ZwReadFile(FileHandle,
                      0, 0, 0,
                      &IoStatusBlock,
                      FileBuffer,
                      FileStdInfo.EndOfFile.u.LowPart,
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not read symbol file into memory\n");
      ExFreePool(FileBuffer);
      return;
    }

  ZwClose(FileHandle);

  LdrpLoadModuleSymbolsFromBuffer(ModuleObject,
                                  FileBuffer,
                                  FileStdInfo.EndOfFile.u.LowPart);

  ExFreePool(FileBuffer);
}

VOID LdrLoadUserModuleSymbols(PLDR_MODULE ModuleObject)
{
  FILE_STANDARD_INFORMATION FileStdInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR TmpFileName[MAX_PATH];
  UNICODE_STRING Filename;
  LPWSTR Start, Ext;
  HANDLE FileHandle;
  PVOID FileBuffer;
  NTSTATUS Status;
  ULONG Length;
  IO_STATUS_BLOCK IoStatusBlock;

  ModuleObject->Symbols.SymbolCount = 0;
  ModuleObject->Symbols.Symbols = NULL;

  /*  Get the path to the symbol store  */
  wcscpy(TmpFileName, L"\\SystemRoot\\symbols\\");

  /*  Get the symbol filename from the module name  */
  Start = wcsrchr(ModuleObject->BaseDllName.Buffer, L'\\');
  if (Start == NULL)
    Start = ModuleObject->BaseDllName.Buffer;
  else
    Start++;

  Ext = wcsrchr(ModuleObject->BaseDllName.Buffer, L'.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = wcslen(Start);

  wcsncat(TmpFileName, Start, Length);
  wcscat(TmpFileName, L".sym");
  RtlInitUnicodeString(&Filename, TmpFileName);

  /*  Open the file  */
  InitializeObjectAttributes(&ObjectAttributes,
                             &Filename,
                             0,
                             NULL,
                             NULL);

  Status = ZwOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      0,
                      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not open symbol file: %wZ\n", &Filename);
      return;
    }

  CPRINT("Loading symbols from %wZ...\n", &Filename);

  /*  Get the size of the file  */
  Status = ZwQueryInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileStdInfo,
                                  sizeof(FileStdInfo),
                                  FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not get file size\n");
      return;
    }

  /*  Allocate nonpageable memory for symbol file  */
  FileBuffer = ExAllocatePool(NonPagedPool,
                              FileStdInfo.EndOfFile.u.LowPart);

  if (FileBuffer == NULL)
    {
      DPRINT("Could not allocate memory for symbol file\n");
      return;
    }
   
  /*  Load file into memory chunk  */
  Status = ZwReadFile(FileHandle,
                      0, 0, 0,
                      &IoStatusBlock,
                      FileBuffer,
                      FileStdInfo.EndOfFile.u.LowPart,
                      0, 0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Could not read symbol file into memory\n");
      ExFreePool(FileBuffer);
      return;
    }

  ZwClose(FileHandle);

  LdrpLoadUserModuleSymbolsFromBuffer(ModuleObject,
                                      FileBuffer,
                                      FileStdInfo.EndOfFile.u.LowPart);

  ExFreePool(FileBuffer);
}
#endif /* KDBG */


#if 0
VOID LdrLoadAutoConfigDrivers (VOID)
{

#ifdef KDBG
  NTSTATUS Status;
  UNICODE_STRING ModuleName;
  PMODULE_OBJECT ModuleObject;

  /* Load symbols for ntoskrnl.exe and hal.dll because \SystemRoot
     is created after their module entries */

  RtlInitUnicodeString(&ModuleName, KERNEL_MODULE_NAME);
  Status = LdrFindModuleObject(&ModuleName, &ModuleObject);
  if (NT_SUCCESS(Status))
  {
    LdrpLoadModuleSymbols(ModuleObject);
  }

  RtlInitUnicodeString(&ModuleName, HAL_MODULE_NAME);
  Status = LdrFindModuleObject(&ModuleName, &ModuleObject);
  if (NT_SUCCESS(Status))
  {
    LdrpLoadModuleSymbols(ModuleObject);
  }

#endif /* KDBG */

   /*
    * PCI bus driver
    */
   //LdrLoadAutoConfigDriver( L"pci.sys" );

   /*
    * Keyboard driver
    */
   LdrLoadAutoConfigDriver( L"keyboard.sys" );

   if ((KdDebuggerEnabled) && (KdDebugState & KD_DEBUG_PICE))
     {
			 /*
			  * Private ICE debugger
			  */
		   LdrLoadAutoConfigDriver( L"pice.sys" );
     }
   
   /*
    * Raw console driver
    */
   LdrLoadAutoConfigDriver( L"blue.sys" );
   
   /*
    * 
    */
   LdrLoadAutoConfigDriver(L"vidport.sys");
   
#if 0
   /*
    * Minix filesystem driver
    */
   LdrLoadAutoConfigDriver(L"minixfs.sys");

   /*
    * Mailslot filesystem driver
    */
   LdrLoadAutoConfigDriver(L"msfs.sys");
#endif   
   /*
    * Named pipe filesystem driver
    */
   LdrLoadAutoConfigDriver(L"npfs.sys");

   /*
    * Mouse drivers
    */
//   LdrLoadAutoConfigDriver(L"l8042prt.sys");
   LdrLoadAutoConfigDriver(L"psaux.sys");
   LdrLoadAutoConfigDriver(L"mouclass.sys");

   /*
    * Networking
    */
#if 1
   /*
    * NDIS library
    */
   LdrLoadAutoConfigDriver(L"ndis.sys");
#endif
}
#endif


NTSTATUS
LdrLoadGdiDriver(PUNICODE_STRING DriverName,
		 PVOID *ImageAddress,
		 PVOID *SectionPointer,
		 PVOID *EntryPoint,
		 PVOID *ExportSectionPointer)
{
  PMODULE_OBJECT ModuleObject;
  NTSTATUS Status;

  Status = LdrLoadModule(DriverName, &ModuleObject);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  if (ImageAddress)
    *ImageAddress = ModuleObject->Base;

//  if (SectionPointer)
//    *SectionPointer = ModuleObject->

  if (EntryPoint)
    *EntryPoint = ModuleObject->EntryPoint;

//  if (ExportSectionPointer)
//    *ExportSectionPointer = ModuleObject->

  return(STATUS_SUCCESS);
}


NTSTATUS
LdrLoadModule(PUNICODE_STRING Filename,
	      PMODULE_OBJECT *ModuleObject)
{
  PVOID ModuleLoadBase;
  NTSTATUS Status;
  HANDLE FileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PMODULE_OBJECT Module;
  FILE_STANDARD_INFORMATION FileStdInfo;
  IO_STATUS_BLOCK IoStatusBlock;

  *ModuleObject = NULL;

  /*  Check for module already loaded  */
  Module = LdrGetModuleObject(Filename);
  if (Module != NULL)
    {
      *ModuleObject = Module;
      return(STATUS_SUCCESS);
    }

  DPRINT("Loading Module %wZ...\n", Filename);

  /*  Open the Module  */
  InitializeObjectAttributes(&ObjectAttributes,
                             Filename,
                             0,
                             NULL,
                             NULL);
  CHECKPOINT;
  Status = NtOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      &IoStatusBlock,
                      0,
                      0);
  CHECKPOINT;
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Could not open module file: %wZ\n", Filename);
      return(Status);
    }
  CHECKPOINT;

  /*  Get the size of the file  */
  Status = NtQueryInformationFile(FileHandle,
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
  Status = NtReadFile(FileHandle,
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

  NtClose(FileHandle);

  Status = LdrProcessModule(ModuleLoadBase,
                            Filename,
                            &Module);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Could not process module");
      ExFreePool(ModuleLoadBase);
      return(Status);
    }

  /*  Cleanup  */
  ExFreePool(ModuleLoadBase);

#ifdef KDBG

  /* Load symbols for module if available */
  LdrpLoadModuleSymbols(Module);

#endif /* KDBG */

  *ModuleObject = Module;

  return(STATUS_SUCCESS);
}


NTSTATUS
LdrInitializeBootStartDriver(PVOID ModuleLoadBase,
			     PCHAR FileName,
			     ULONG ModuleLength)
{
  PMODULE_OBJECT ModuleObject;
  UNICODE_STRING ModuleName;
  PDEVICE_NODE DeviceNode;
  NTSTATUS Status;

  WCHAR Buffer[MAX_PATH];
  ULONG Length;
  LPWSTR Start;
  LPWSTR Ext;

  CHAR TextBuffer [256];
  ULONG x, y, cx, cy;

#ifdef KDBG
  CHAR TmpBaseName[MAX_PATH];
  CHAR TmpFileName[MAX_PATH];
  ANSI_STRING AnsiString;
  ULONG Length;
  PCHAR Ext;
#endif

  HalQueryDisplayParameters(&x, &y, &cx, &cy);
  RtlFillMemory(TextBuffer, x, ' ');
  TextBuffer[x] = '\0';
  HalSetDisplayParameters(0, y-1);
  HalDisplayString(TextBuffer);

  sprintf(TextBuffer, "Initializing %s...\n", FileName);
  HalSetDisplayParameters(0, y-1);
  HalDisplayString(TextBuffer);
  HalSetDisplayParameters(cx, cy);

#ifdef KDBG
  /*  Split the filename into base name and extension  */
  Ext = strrchr(FileName, '.');
  if (Ext != NULL)
    Length = Ext - FileName;
  else
    Length = strlen(FileName);

  if ((Ext != NULL) && (strcmp(Ext, ".sym") == 0))
    {
      DPRINT("Module %s is a symbol file\n", FileName);

      strncpy(TmpBaseName, FileName, Length);
      TmpBaseName[Length] = '\0';

      DPRINT("base: %s (Length %d)\n", TmpBaseName, Length);

      strcpy(TmpFileName, TmpBaseName);
      strcat(TmpFileName, ".sys");
      RtlInitAnsiString(&AnsiString, TmpFileName);

      DPRINT("dasdsad: %s\n", TmpFileName);

      RtlAnsiStringToUnicodeString(&ModuleName, &AnsiString, TRUE);
      Status = LdrFindModuleObject(&ModuleName, &ModuleObject);
      RtlFreeUnicodeString(&ModuleName);
      if (!NT_SUCCESS(Status))
	{
	  strcpy(TmpFileName, TmpBaseName);
	  strcat(TmpFileName, ".exe");
	  RtlInitAnsiString(&AnsiString, TmpFileName);
	  RtlAnsiStringToUnicodeString(&ModuleName, &AnsiString, TRUE);
	  Status = LdrFindModuleObject(&ModuleName, &ModuleObject);
	  RtlFreeUnicodeString(&ModuleName);
	}
      if (NT_SUCCESS(Status))
	{
	  LdrpLoadModuleSymbolsFromBuffer(ModuleObject,
					  ModuleLoadBase,
					  ModuleLength);
	}
      return(STATUS_SUCCESS);
    }
  else
    {
      DPRINT("Module %s is executable\n", FileName);
    }
#endif /* KDBG */

  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("Driver load failed, status (%x)\n", Status);
      return(Status);
    }

  RtlCreateUnicodeStringFromAsciiz(&ModuleName,
				   FileName);
  Status = LdrProcessModule(ModuleLoadBase,
			    &ModuleName,
			    &ModuleObject);
  RtlFreeUnicodeString(&ModuleName);
  if (ModuleObject == NULL)
    {
      IopFreeDeviceNode(DeviceNode);
      CPRINT("Driver load failed, status (%x)\n", Status);
      return(STATUS_UNSUCCESSFUL);
    }


  /* Get the service name from the module name */
  Start = wcsrchr(ModuleObject->BaseName.Buffer, L'\\');
  if (Start == NULL)
    Start = ModuleObject->BaseName.Buffer;
  else
    Start++;

  Ext = wcsrchr(ModuleObject->BaseName.Buffer, L'.');
  if (Ext != NULL)
    Length = Ext - Start;
  else
    Length = wcslen(Start);

  wcsncpy(Buffer, Start, Length);
  RtlCreateUnicodeString(&DeviceNode->ServiceName, Buffer);

  Status = IopInitializeDriver(ModuleObject->EntryPoint,
			       DeviceNode, FALSE);
  if (!NT_SUCCESS(Status))
    {
      IopFreeDeviceNode(DeviceNode);
      CPRINT("Driver load failed, status (%x)\n", Status);
    }

  return(Status);
}


NTSTATUS
LdrProcessModule(PVOID ModuleLoadBase,
		 PUNICODE_STRING ModuleName,
		 PMODULE_OBJECT *ModuleObject)
{
  PIMAGE_DOS_HEADER PEDosHeader;

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  if (PEDosHeader->e_magic == IMAGE_DOS_MAGIC && PEDosHeader->e_lfanew != 0L)
    {
      return LdrPEProcessModule(ModuleLoadBase,
				ModuleName,
				ModuleObject);
    }

  CPRINT("Module wasn't PE\n");
  return STATUS_UNSUCCESSFUL;
}


PVOID
LdrGetExportAddress(PMODULE_OBJECT ModuleObject,
                    char *Name,
                    unsigned short Hint)
{
  if (ModuleObject->Flags & MODULE_FLAG_PE)
    {
      return LdrPEGetExportAddress(ModuleObject, Name, Hint);
    }
  else
    {
      return 0;
    }
}


NTSTATUS
LdrpQueryModuleInformation(PVOID Buffer,
			   ULONG Size,
			   PULONG ReqSize)
{
  PLIST_ENTRY current_entry;
  PMODULE_OBJECT current;
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
    (ModuleCount - 1) * sizeof(SYSTEM_MODULE_ENTRY);

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
      current = CONTAINING_RECORD(current_entry,MODULE_OBJECT,ListEntry);

      Smi->Module[ModuleCount].Unknown2 = 0;		/* Always 0 */
      Smi->Module[ModuleCount].BaseAddress = current->Base;
      Smi->Module[ModuleCount].Size = current->Length;
      Smi->Module[ModuleCount].Flags = 0;		/* Flags ??? (GN) */
      Smi->Module[ModuleCount].EntryIndex = ModuleCount;

      AnsiName.Length = 0;
      AnsiName.MaximumLength = 256;
      AnsiName.Buffer = Smi->Module[ModuleCount].Name;
      RtlUnicodeStringToAnsiString(&AnsiName,
				   &current->FullName,
				   FALSE);

      p = strrchr(AnsiName.Buffer, '\\');
      if (p == NULL)
	{
	  Smi->Module[ModuleCount].PathLength = 0;
	  Smi->Module[ModuleCount].NameLength = strlen(AnsiName.Buffer);
	}
      else
	{
	  p++;
	  Smi->Module[ModuleCount].PathLength = p - AnsiName.Buffer;
	  Smi->Module[ModuleCount].NameLength = strlen(p);
	}

      ModuleCount++;
      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&ModuleListLock, Irql);

  return(STATUS_SUCCESS);
}


static VOID
LdrpBuildModuleBaseName(PUNICODE_STRING BaseName,
			PUNICODE_STRING FullName)
{
   UNICODE_STRING Name;
   PWCHAR p;
   PWCHAR q;

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

   RtlCreateUnicodeString(&Name, p);

   q = wcschr(Name.Buffer, L'.');
   if (q != NULL)
     {
	*q = (WCHAR)0;
     }

   DPRINT("p %S\n", p);

   RtlCreateUnicodeString(BaseName, Name.Buffer);
   RtlFreeUnicodeString(&Name);
}


static LONG
LdrpCompareModuleNames(IN PUNICODE_STRING String1,
		       IN PUNICODE_STRING String2)
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


PMODULE_OBJECT
LdrGetModuleObject(PUNICODE_STRING ModuleName)
{
  PMODULE_OBJECT Module;
  PLIST_ENTRY Entry;
  KIRQL Irql;

  DPRINT("LdrpGetModuleObject(%wZ) called\n", ModuleName);

  KeAcquireSpinLock(&ModuleListLock,&Irql);

  Entry = ModuleListHead.Flink;
  while (Entry != &ModuleListHead)
    {
      Module = CONTAINING_RECORD(Entry, MODULE_OBJECT, ListEntry);

      DPRINT("Comparing %wZ and %wZ\n",
	     &Module->BaseName,
	     ModuleName);

      if (!LdrpCompareModuleNames(&Module->BaseName, ModuleName))
	{
	  DPRINT("Module %wZ\n", &Module->BaseName);
	  KeReleaseSpinLock(&ModuleListLock, Irql);
	  return(Module);
	}

      Entry = Entry->Flink;
    }

  KeReleaseSpinLock(&ModuleListLock, Irql);

  CPRINT("LdrpGetModuleObject: Failed to find dll %wZ\n", ModuleName);

  return(NULL);
}


/*  ----------------------------------------------  PE Module support */

static NTSTATUS
LdrPEProcessModule(PVOID ModuleLoadBase,
		   PUNICODE_STRING FileName,
		   PMODULE_OBJECT *ModuleObject)
{
  unsigned int DriverSize, Idx;
  ULONG RelocDelta, NumRelocs;
  DWORD CurrentSize, TotalRelocs;
  PVOID DriverBase;
  PULONG PEMagic;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptionalHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  PRELOCATION_DIRECTORY RelocDir;
  PRELOCATION_ENTRY RelocEntry;
  PMODULE_OBJECT  LibraryModuleObject;
  PMODULE_OBJECT CreatedModuleObject;
  PVOID *ImportAddressList;
  PULONG FunctionNameList;
  PCHAR pName;
  WORD Hint;
  UNICODE_STRING ModuleName;
  WCHAR  NameBuffer[60];
  MODULE_TEXT_SECTION* ModuleTextSection;
  NTSTATUS Status;
  KIRQL Irql;

  DPRINT("Processing PE Module at module base:%08lx\n", ModuleLoadBase);

  /*  Get header pointers  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  PEMagic = (PULONG) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew);
  PEFileHeader = (PIMAGE_FILE_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG));
  PEOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER));
  PESectionHeaders = (PIMAGE_SECTION_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
    sizeof(IMAGE_OPTIONAL_HEADER));
  CHECKPOINT;

  /*  Check file magic numbers  */
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC)
    {
      CPRINT("Incorrect MZ magic: %04x\n", PEDosHeader->e_magic);
      return STATUS_UNSUCCESSFUL;
    }
  if (PEDosHeader->e_lfanew == 0)
    {
      CPRINT("Invalid lfanew offset: %08x\n", PEDosHeader->e_lfanew);
      return STATUS_UNSUCCESSFUL;
    }
  if (*PEMagic != IMAGE_PE_MAGIC)
    {
      CPRINT("Incorrect PE magic: %08x\n", *PEMagic);
      return STATUS_UNSUCCESSFUL;
    }
  if (PEFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
    {
      CPRINT("Incorrect Architechture: %04x\n", PEFileHeader->Machine);
      return STATUS_UNSUCCESSFUL;
    }
  CHECKPOINT;

  /* FIXME: if image is fixed-address load, then fail  */

  /* FIXME: check/verify OS version number  */

  DPRINT("OptionalHdrMagic:%04x LinkVersion:%d.%d\n", 
         PEOptionalHeader->Magic,
         PEOptionalHeader->MajorLinkerVersion,
         PEOptionalHeader->MinorLinkerVersion);
  DPRINT("Entry Point:%08lx\n", PEOptionalHeader->AddressOfEntryPoint);
  CHECKPOINT;

  /*  Determine the size of the module  */
  DriverSize = PEOptionalHeader->SizeOfImage;
  DPRINT("DriverSize %x\n",DriverSize);

  /*  Allocate a virtual section for the module  */
  DriverBase = MmAllocateSection(DriverSize);
  if (DriverBase == 0)
    {
      CPRINT("Failed to allocate a virtual section for driver\n");
      return STATUS_UNSUCCESSFUL;
    }
   CPRINT("DriverBase: %x\n", DriverBase);
  CHECKPOINT;
  /*  Copy headers over */
  memcpy(DriverBase, ModuleLoadBase, PEOptionalHeader->SizeOfHeaders);
   CurrentSize = 0;
  /*  Copy image sections into virtual section  */
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      //  Copy current section into current offset of virtual section
      if (PESectionHeaders[Idx].Characteristics & 
          (IMAGE_SECTION_CHAR_CODE | IMAGE_SECTION_CHAR_DATA))
        {
	   DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
		  PESectionHeaders[Idx].VirtualAddress + DriverBase);
           memcpy(PESectionHeaders[Idx].VirtualAddress + DriverBase,
                  (PVOID)(ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
                  PESectionHeaders[Idx].Misc.VirtualSize > PESectionHeaders[Idx].SizeOfRawData ? PESectionHeaders[Idx].SizeOfRawData : PESectionHeaders[Idx].Misc.VirtualSize );
        }
      else
        {
	   DPRINT("PESectionHeaders[Idx].VirtualAddress + DriverBase %x\n",
		  PESectionHeaders[Idx].VirtualAddress + DriverBase);
	   memset(PESectionHeaders[Idx].VirtualAddress + DriverBase, 
		  '\0', PESectionHeaders[Idx].Misc.VirtualSize);

        }
      CurrentSize += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize,
                              PEOptionalHeader->SectionAlignment);


//      CurrentBase = (PVOID)((DWORD)CurrentBase + 
  //      ROUND_UP(PESectionHeaders[Idx].SizeOfRawData.Misc.VirtualSize,
    //             PEOptionalHeader->SectionAlignment));
    }

  /*  Perform relocation fixups  */
  RelocDelta = (DWORD) DriverBase - PEOptionalHeader->ImageBase;
  RelocDir = (PRELOCATION_DIRECTORY)(PEOptionalHeader->DataDirectory[
    IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
  DPRINT("DrvrBase:%08lx ImgBase:%08lx RelocDelta:%08lx\n", 
         DriverBase,
         PEOptionalHeader->ImageBase,
         RelocDelta);   
  DPRINT("RelocDir %x\n",RelocDir);
#if 1
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
       if (PESectionHeaders[Idx].VirtualAddress == (DWORD)RelocDir)
	 {
	    DPRINT("Name %.8s PESectionHeader[Idx].PointerToRawData %x\n",
		   PESectionHeaders[Idx].Name,
		   PESectionHeaders[Idx].PointerToRawData);
	    RelocDir = PESectionHeaders[Idx].PointerToRawData +
	      ModuleLoadBase;
            CurrentSize = PESectionHeaders[Idx].Misc.VirtualSize;
	    break;
	 }
    }
#else
   RelocDir = RelocDir + (ULONG)DriverBase;
   CurrentSize = PEOptionalHeader->DataDirectory
		  [IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
#endif
  DPRINT("RelocDir %08lx CurrentSize %08lx\n", RelocDir, CurrentSize);
  TotalRelocs = 0;
  while (TotalRelocs < CurrentSize && RelocDir->SizeOfBlock != 0)
    {
      NumRelocs = (RelocDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY)) / 
        sizeof(WORD);
/*      DPRINT("RelocDir at %08lx for VA %08lx with %08lx relocs\n",
             RelocDir, 
             RelocDir->VirtualAddress,
             NumRelocs);*/
      RelocEntry = (PRELOCATION_ENTRY) ((DWORD)RelocDir + 
        sizeof(RELOCATION_DIRECTORY));
      for (Idx = 0; Idx < NumRelocs; Idx++)
        {
	   ULONG Offset;
	   ULONG Type;
	   PDWORD RelocItem;
	   
	   Offset = RelocEntry[Idx].TypeOffset & 0xfff;
	   Type = (RelocEntry[Idx].TypeOffset >> 12) & 0xf;
	   RelocItem = (PDWORD)(DriverBase + RelocDir->VirtualAddress + 
				Offset);
/*	   DPRINT("  reloc at %08lx %x %s old:%08lx new:%08lx\n", 
		  RelocItem,
		  Type,
		  Type ? "HIGHLOW" : "ABS",
		  *RelocItem,
		  (*RelocItem) + RelocDelta); */
          if (Type == 3)
            {
              (*RelocItem) += RelocDelta;
            }
          else if (Type != 0)
            {
              CPRINT("Unknown relocation type %x at %x\n",Type, &Type);
              return STATUS_UNSUCCESSFUL;
            }
        }
      TotalRelocs += RelocDir->SizeOfBlock;
      RelocDir = (PRELOCATION_DIRECTORY)((DWORD)RelocDir + 
        RelocDir->SizeOfBlock);
//      DPRINT("TotalRelocs: %08lx  CurrentSize: %08lx\n", TotalRelocs, CurrentSize);
    }
   
  DPRINT("PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] %x\n",
         PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
         .VirtualAddress);
  /*  Perform import fixups  */
  if (PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    {
      PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;

      /*  Process each import module  */
      ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
        ((DWORD)DriverBase + PEOptionalHeader->
          DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
      DPRINT("Processeing import directory at %p\n", ImportModuleDirectory);
      while (ImportModuleDirectory->dwRVAModuleName)
        {
          /*  Check to make sure that import lib is kernel  */
          pName = (PCHAR) DriverBase + 
            ImportModuleDirectory->dwRVAModuleName;

          RtlCreateUnicodeStringFromAsciiz(&ModuleName, pName);
          DPRINT("Import module: %wZ\n", &ModuleName);

          Status = LdrLoadModule(&ModuleName, &LibraryModuleObject);
          if (!NT_SUCCESS(Status))
            {
              CPRINT("Unknown import module: %wZ (Status %lx)\n", &ModuleName, Status);
            }
          /*  Get the import address list  */
          ImportAddressList = (PVOID *) ((DWORD)DriverBase + 
            ImportModuleDirectory->dwRVAFunctionAddressList);

          /*  Get the list of functions to import  */
          if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
            {
              FunctionNameList = (PULONG) ((DWORD)DriverBase + 
                ImportModuleDirectory->dwRVAFunctionNameList);
            }
          else
            {
              FunctionNameList = (PULONG) ((DWORD)DriverBase + 
                ImportModuleDirectory->dwRVAFunctionAddressList);
            }
          /*  Walk through function list and fixup addresses  */
          while (*FunctionNameList != 0L)
            {
              if ((*FunctionNameList) & 0x80000000) // hint
                {
                  pName = NULL;


                  Hint = (*FunctionNameList) & 0xffff;
                }
              else // hint-name
                {
                  pName = (PCHAR)((DWORD)DriverBase + 
                                  *FunctionNameList + 2);
                  Hint = *(PWORD)((DWORD)DriverBase + *FunctionNameList);
                }
              DPRINT("  Hint:%04x  Name:%s\n", Hint, pName);

              /*  Fixup the current import symbol  */
              if (LibraryModuleObject != NULL)
                {
                  *ImportAddressList = LdrGetExportAddress(LibraryModuleObject, 
                                                           pName, 
                                                           Hint);
                }
              else
                {
                  CPRINT("Unresolved kernel symbol: %s\n", pName);
                  return STATUS_UNSUCCESSFUL;
                }
              ImportAddressList++;
              FunctionNameList++;
            }

          RtlFreeUnicodeString(&ModuleName);

          ImportModuleDirectory++;
        }
    }

  /* Create the module */
  CreatedModuleObject = ExAllocatePool(NonPagedPool, sizeof(MODULE_OBJECT));
  if (CreatedModuleObject == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  RtlZeroMemory(CreatedModuleObject, sizeof(MODULE_OBJECT));

   /*  Initialize ModuleObject data  */
   CreatedModuleObject->Base = DriverBase;
   CreatedModuleObject->Flags = MODULE_FLAG_PE;

   RtlCreateUnicodeString(&CreatedModuleObject->FullName,
			  FileName->Buffer);
   LdrpBuildModuleBaseName(&CreatedModuleObject->BaseName,
			   &CreatedModuleObject->FullName);

  CreatedModuleObject->EntryPoint = (PVOID)((DWORD)DriverBase + 
    PEOptionalHeader->AddressOfEntryPoint);
  CreatedModuleObject->Length = DriverSize;
  DPRINT("EntryPoint at %x\n", CreatedModuleObject->EntryPoint);

  CreatedModuleObject->Image.PE.FileHeader =
    (PIMAGE_FILE_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG));

  DPRINT("FileHeader at %x\n", CreatedModuleObject->Image.PE.FileHeader);
  CreatedModuleObject->Image.PE.OptionalHeader = 
    (PIMAGE_OPTIONAL_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG) +
    sizeof(IMAGE_FILE_HEADER));
  DPRINT("OptionalHeader at %x\n", CreatedModuleObject->Image.PE.OptionalHeader);
  CreatedModuleObject->Image.PE.SectionList = 
    (PIMAGE_SECTION_HEADER) ((unsigned int) DriverBase + PEDosHeader->e_lfanew + sizeof(ULONG) +
    sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER));
  DPRINT("SectionList at %x\n", CreatedModuleObject->Image.PE.SectionList);

  /* Insert module */
  KeAcquireSpinLock(&ModuleListLock, &Irql);
  InsertTailList(&ModuleListHead,
		 &CreatedModuleObject->ListEntry);
  KeReleaseSpinLock(&ModuleListLock, Irql);


  ModuleTextSection = ExAllocatePool(NonPagedPool, 
				     sizeof(MODULE_TEXT_SECTION));
  ModuleTextSection->Base = (ULONG)DriverBase;
  ModuleTextSection->Length = DriverSize;
  ModuleTextSection->SymbolsBase = NULL;
  ModuleTextSection->SymbolsLength = 0;
  ModuleTextSection->Name = 
    ExAllocatePool(NonPagedPool, 
		   (wcslen(NameBuffer) + 1) * sizeof(WCHAR));
  wcscpy(ModuleTextSection->Name, NameBuffer);
  InsertTailList(&ModuleTextListHead, &ModuleTextSection->ListEntry);

  CreatedModuleObject->TextSection = ModuleTextSection;

  *ModuleObject = CreatedModuleObject;

  DPRINT("Loading Module %wZ...\n", FileName);

  if ((KdDebuggerEnabled == TRUE) && (KdDebugState & KD_DEBUG_GDB))
    {
      DbgPrint("Module %wZ loaded at 0x%.08x.\n",
	       FileName, CreatedModuleObject->Base);
    }

  return STATUS_SUCCESS;
}


PVOID
LdrSafePEProcessModule(PVOID ModuleLoadBase,
		       PVOID DriverBase,
		       PVOID ImportModuleBase,
		       PULONG DriverSize)
{
  unsigned int Idx;
  ULONG RelocDelta, NumRelocs;
  ULONG CurrentSize, TotalRelocs;
  PULONG PEMagic;
  PIMAGE_DOS_HEADER PEDosHeader;
  PIMAGE_FILE_HEADER PEFileHeader;
  PIMAGE_OPTIONAL_HEADER PEOptionalHeader;
  PIMAGE_SECTION_HEADER PESectionHeaders;
  PRELOCATION_DIRECTORY RelocDir;
  PRELOCATION_ENTRY RelocEntry;
  PVOID *ImportAddressList;
  PULONG FunctionNameList;
  PCHAR pName;
  USHORT Hint;

  ps("Processing PE Module at module base:%08lx\n", ModuleLoadBase);

  /*  Get header pointers  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ModuleLoadBase;
  PEMagic = (PULONG) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew);
  PEFileHeader = (PIMAGE_FILE_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG));
  PEOptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER));
  PESectionHeaders = (PIMAGE_SECTION_HEADER) ((unsigned int) ModuleLoadBase + 
    PEDosHeader->e_lfanew + sizeof(ULONG) + sizeof(IMAGE_FILE_HEADER) +
    sizeof(IMAGE_OPTIONAL_HEADER));
  CHECKPOINT;

  /*  Check file magic numbers  */
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC)
    {
      return 0;
    }
  if (PEDosHeader->e_lfanew == 0)
    {
      return 0;
    }
  if (*PEMagic != IMAGE_PE_MAGIC)
    {
      return 0;
    }
  if (PEFileHeader->Machine != IMAGE_FILE_MACHINE_I386)
    {
      return 0;
    }

  ps("OptionalHdrMagic:%04x LinkVersion:%d.%d\n", 
         PEOptionalHeader->Magic,
         PEOptionalHeader->MajorLinkerVersion,
         PEOptionalHeader->MinorLinkerVersion);
  ps("Entry Point:%08lx\n", PEOptionalHeader->AddressOfEntryPoint);

  /*  Determine the size of the module  */
  *DriverSize = PEOptionalHeader->SizeOfImage;
  ps("DriverSize %x\n",*DriverSize);

  /*  Copy headers over */
  if (DriverBase != ModuleLoadBase)
    {
      memcpy(DriverBase, ModuleLoadBase, PEOptionalHeader->SizeOfHeaders);
    }

  ps("Hdr: 0x%X\n", (ULONG)PEOptionalHeader);
  ps("Hdr->SizeOfHeaders: 0x%X\n", (ULONG)PEOptionalHeader->SizeOfHeaders);
  ps("FileHdr->NumberOfSections: 0x%X\n", (ULONG)PEFileHeader->NumberOfSections);

  /* Ntoskrnl.exe need no relocation fixups since it is linked to run at the same
     address as it is mapped */
  if (DriverBase != ModuleLoadBase)
    {
      CurrentSize = 0;

  /*  Copy image sections into virtual section  */
  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      //  Copy current section into current offset of virtual section
      if (PESectionHeaders[Idx].Characteristics & 
          (IMAGE_SECTION_CHAR_CODE | IMAGE_SECTION_CHAR_DATA))
	{
	  //ps("PESectionHeaders[Idx].VirtualAddress (%X) + DriverBase %x\n",
	  //PESectionHeaders[Idx].VirtualAddress, PESectionHeaders[Idx].VirtualAddress + DriverBase);
	  memcpy(PESectionHeaders[Idx].VirtualAddress + DriverBase,
		 (PVOID)(ModuleLoadBase + PESectionHeaders[Idx].PointerToRawData),
		 PESectionHeaders[Idx].Misc.VirtualSize > PESectionHeaders[Idx].SizeOfRawData ?
		   PESectionHeaders[Idx].SizeOfRawData : PESectionHeaders[Idx].Misc.VirtualSize );
	}
      else
	{
	  ps("PESectionHeaders[Idx].VirtualAddress (%X) + DriverBase %x\n",
	     PESectionHeaders[Idx].VirtualAddress, PESectionHeaders[Idx].VirtualAddress + DriverBase);
	  memset(PESectionHeaders[Idx].VirtualAddress + DriverBase, 
		 '\0',
		 PESectionHeaders[Idx].Misc.VirtualSize);
	}
      CurrentSize += ROUND_UP(PESectionHeaders[Idx].Misc.VirtualSize,
                              PEOptionalHeader->SectionAlignment);
    }

  /*  Perform relocation fixups  */
  RelocDelta = (ULONG) DriverBase - PEOptionalHeader->ImageBase;
  RelocDir = (PRELOCATION_DIRECTORY)(PEOptionalHeader->DataDirectory[
    IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
  ps("DrvrBase:%08lx ImgBase:%08lx RelocDelta:%08lx\n", 
         DriverBase,
         PEOptionalHeader->ImageBase,
         RelocDelta);   
  ps("RelocDir %x\n",RelocDir);

  for (Idx = 0; Idx < PEFileHeader->NumberOfSections; Idx++)
    {
      if (PESectionHeaders[Idx].VirtualAddress == (ULONG)RelocDir)
	{
	  DPRINT("Name %.8s PESectionHeader[Idx].PointerToRawData %x\n",
		 PESectionHeaders[Idx].Name,
		 PESectionHeaders[Idx].PointerToRawData);
	  RelocDir = PESectionHeaders[Idx].PointerToRawData + ModuleLoadBase;
	  CurrentSize = PESectionHeaders[Idx].Misc.VirtualSize;
	  break;
	}
    }

  ps("RelocDir %08lx CurrentSize %08lx\n", RelocDir, CurrentSize);

  TotalRelocs = 0;
  while (TotalRelocs < CurrentSize && RelocDir->SizeOfBlock != 0)
    {
      NumRelocs = (RelocDir->SizeOfBlock - sizeof(RELOCATION_DIRECTORY)) / 
        sizeof(USHORT);
      RelocEntry = (PRELOCATION_ENTRY)((ULONG)RelocDir + 
        sizeof(RELOCATION_DIRECTORY));
      for (Idx = 0; Idx < NumRelocs; Idx++)
        {
	  ULONG Offset;
	  ULONG Type;
	  PDWORD RelocItem;

	  Offset = RelocEntry[Idx].TypeOffset & 0xfff;
	  Type = (RelocEntry[Idx].TypeOffset >> 12) & 0xf;
	  RelocItem = (PULONG)(DriverBase + RelocDir->VirtualAddress + Offset);
	  if (Type == 3)
	    {
	      (*RelocItem) += RelocDelta;
	    }
	  else if (Type != 0)
	    {
	      CPRINT("Unknown relocation type %x at %x\n",Type, &Type);
	      return(0);
	    }
	}
      TotalRelocs += RelocDir->SizeOfBlock;
      RelocDir = (PRELOCATION_DIRECTORY)((ULONG)RelocDir + 
        RelocDir->SizeOfBlock);
    }

    ps("PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] %x\n",
         PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
         .VirtualAddress);
  }

  /*  Perform import fixups  */
  if (PEOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
    {
      PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;

      /*  Process each import module  */
      ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
        ((ULONG)DriverBase + PEOptionalHeader->
          DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

      ps("Processeing import directory at %p\n", ImportModuleDirectory);

      /*  Check to make sure that import lib is kernel  */
      pName = (PCHAR)DriverBase + ImportModuleDirectory->dwRVAModuleName;

      ps("Import module: %s\n", pName);

      /*  Get the import address list  */
      ImportAddressList = (PVOID *)((ULONG)DriverBase + 
	ImportModuleDirectory->dwRVAFunctionAddressList);

      ps("  ImportModuleDirectory->dwRVAFunctionAddressList: 0x%X\n",
	 ImportModuleDirectory->dwRVAFunctionAddressList);
      ps("  ImportAddressList: 0x%X\n", ImportAddressList);

      /*  Get the list of functions to import  */
      if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
	{
	  ps("Using function name list.\n");

	  FunctionNameList = (PULONG)((ULONG)DriverBase + 
	    ImportModuleDirectory->dwRVAFunctionNameList);
	}
      else
	{
	  ps("Using function address list.\n");

	  FunctionNameList = (PULONG)((ULONG)DriverBase + 
	    ImportModuleDirectory->dwRVAFunctionAddressList);
	}

      /* Walk through function list and fixup addresses */
      while (*FunctionNameList != 0L)
	{
	  if ((*FunctionNameList) & 0x80000000)
	    {
	       /* Hint */
	      pName = NULL;
	      Hint = (*FunctionNameList) & 0xffff;
	    }
	  else
	    {
	      /* Hint name */
	      pName = (PCHAR)((ULONG)DriverBase + *FunctionNameList + 2);
	      Hint = *(PWORD)((ULONG)DriverBase + *FunctionNameList);
	    }
	  //ps("  Hint:%04x  Name:%s(0x%X)(%x)\n", Hint, pName, pName, ImportAddressList);

	  *ImportAddressList = LdrSafePEGetExportAddress(ImportModuleBase,
							 pName,
							 Hint);

	  ImportAddressList++;
	  FunctionNameList++;
	}
    }

  ps("Finished importing.\n");

  return(0);
}


static PVOID
LdrPEGetExportAddress(PMODULE_OBJECT ModuleObject,
		      PCHAR Name,
		      USHORT Hint)
{
  PIMAGE_EXPORT_DIRECTORY ExportDir;
  ULONG ExportDirSize;
  USHORT Idx;
  PVOID  ExportAddress;
  PWORD  OrdinalList;
  PDWORD FunctionList, NameList;

   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
     RtlImageDirectoryEntryToData(ModuleObject->Base,
				  TRUE,
				  IMAGE_DIRECTORY_ENTRY_EXPORT,
				  &ExportDirSize);
   DPRINT("ExportDir %p ExportDirSize %lx\n", ExportDir, ExportDirSize);
   if (ExportDir == NULL)
     {
	return NULL;
     }

   FunctionList = (PDWORD)((DWORD)ExportDir->AddressOfFunctions + ModuleObject->Base);
   NameList = (PDWORD)((DWORD)ExportDir->AddressOfNames + ModuleObject->Base);
   OrdinalList = (PWORD)((DWORD)ExportDir->AddressOfNameOrdinals + ModuleObject->Base);

  ExportAddress = 0;

  if (Name != NULL)
    {
      for (Idx = 0; Idx < ExportDir->NumberOfNames; Idx++)
        {
#if 0
          DPRINT("  Name:%s  NameList[%d]:%s\n", 
                 Name, 
                 Idx, 
                 (DWORD) ModuleObject->Base + NameList[Idx]);

#endif
          if (!strcmp(Name, (PCHAR) ((DWORD)ModuleObject->Base + NameList[Idx])))
            {
              ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
                FunctionList[OrdinalList[Idx]]);
		  if (((ULONG)ExportAddress >= (ULONG)ExportDir) &&
		      ((ULONG)ExportAddress < (ULONG)ExportDir + ExportDirSize))
		    {
		       DPRINT("Forward: %s\n", (PCHAR)ExportAddress);
		       ExportAddress = LdrPEFixupForward((PCHAR)ExportAddress);
		       DPRINT("ExportAddress: %p\n", ExportAddress);
		    }

              break;
            }
        }
    }
  else  /*  use hint  */
    {
      ExportAddress = (PVOID) ((DWORD)ModuleObject->Base +
        FunctionList[Hint - ExportDir->Base]);
    }

  if (ExportAddress == 0)
    {
      CPRINT("Export not found for %d:%s\n",
	     Hint,
	     Name != NULL ? Name : "(Ordinal)");
      KeBugCheck(0);
    }

  return(ExportAddress);
}


static PVOID
LdrSafePEGetExportAddress(PVOID ImportModuleBase,
			  PCHAR Name,
			  USHORT Hint)
{
  USHORT Idx;
  PVOID  ExportAddress;
  PWORD  OrdinalList;
  PDWORD FunctionList, NameList;
  PIMAGE_EXPORT_DIRECTORY  ExportDir;
  ULONG ExportDirSize;

  static BOOLEAN EP = FALSE;

  ExportDir = (PIMAGE_EXPORT_DIRECTORY)
    RtlImageDirectoryEntryToData(ImportModuleBase,
	  TRUE,
		IMAGE_DIRECTORY_ENTRY_EXPORT,
		&ExportDirSize);

  if (!EP) {
    EP = TRUE;
    ps("ExportDir %x\n", ExportDir);
  }

  FunctionList = (PDWORD)((DWORD)ExportDir->AddressOfFunctions + ImportModuleBase);
  NameList = (PDWORD)((DWORD)ExportDir->AddressOfNames + ImportModuleBase);
  OrdinalList = (PWORD)((DWORD)ExportDir->AddressOfNameOrdinals + ImportModuleBase);

  ExportAddress = 0;

  if (Name != NULL)
    {
      for (Idx = 0; Idx < ExportDir->NumberOfNames; Idx++)
        {
          if (!strcmp(Name, (PCHAR) ((DWORD)ImportModuleBase + NameList[Idx])))
      			{
              ExportAddress = (PVOID) ((DWORD)ImportModuleBase +
                FunctionList[OrdinalList[Idx]]);
              break;
            }
        }
    }
  else  /*  use hint  */
    {
      ExportAddress = (PVOID) ((DWORD)ImportModuleBase +

        FunctionList[Hint - ExportDir->Base]);
    }

  if (ExportAddress == 0)
    {
      ps("Export not found for %d:%s\n",
	 Hint,
	 Name != NULL ? Name : "(Ordinal)");
      KeBugCheck(0);
    }
  return ExportAddress;
}


static PVOID
LdrPEFixupForward(PCHAR ForwardName)
{
   CHAR NameBuffer[128];
   UNICODE_STRING ModuleName;
   PCHAR p;
   PMODULE_OBJECT ModuleObject;

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

  return(LdrPEGetExportAddress(ModuleObject, p+1, 0));
}

/* EOF */
