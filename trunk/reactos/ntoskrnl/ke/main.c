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
/* $Id: main.c,v 1.135 2002/09/07 15:12:56 chorns Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/main.c
 * PURPOSE:         Initalizes the kernel
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "../dbg/kdb.h"

#define NDEBUG
#include <internal/debug.h>

#define xbp(Value) \
{ \
  ULONG Port = 0x3f8; \
__asm__("outb %0, %w1\n\t" : : "a" (Value), "d" (Port)); \
}

/* GLOBALS *******************************************************************/

ULONG NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG NtGlobalFlag = 0;
CHAR KeNumberProcessors;
LOADER_PARAMETER_BLOCK KeLoaderBlock;
ULONG KeDcacheFlushCount = 0;
ULONG KeIcacheFlushCount = 0;

static LOADER_MODULE KeLoaderModules[64];
static UCHAR KeLoaderModuleStrings[64][256];
static UCHAR KeLoaderCommandLine[256];
static ADDRESS_RANGE KeMemoryMap[64];
static ULONG KeMemoryMapRangeCount;
static ULONG FirstKrnlPhysAddr;
static ULONG LastKrnlPhysAddr;
static ULONG LastKernelAddress;
volatile BOOLEAN Initialized = FALSE;

extern PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];


/* FUNCTIONS ****************************************************************/

static BOOLEAN
RtlpCheckFileNameExtension(PCHAR FileName,
			   PCHAR Extension)
{
   PCHAR Ext;

   Ext = strrchr(FileName, '.');
   if ((Extension == NULL) || (*Extension == 0))
     {
	if (Ext == NULL)
	  return TRUE;
	else
	  return FALSE;
     }
   if (*Extension != '.')
     Ext++;
   
   if (_stricmp(Ext, Extension) == 0)
     return TRUE;
   else
     return FALSE;
}


static VOID
InitSystemSharedUserPage (PCSZ ParameterLine)
{
   UNICODE_STRING ArcDeviceName;
   UNICODE_STRING ArcName;
   UNICODE_STRING BootPath;
   UNICODE_STRING DriveDeviceName;
   UNICODE_STRING DriveName;
   WCHAR DriveNameBuffer[20];
   PCHAR ParamBuffer;
   PWCHAR ArcNameBuffer;
   PCHAR p;
   NTSTATUS Status;
   ULONG Length;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE Handle;
   ULONG i;
   BOOLEAN BootDriveFound;

   /*
    * NOTE:
    *   The shared user page has been zeroed-out right after creation.
    *   There is NO need to do this again.
    */

   SharedUserData->NtProductType = NtProductWinNt;

   BootDriveFound = FALSE;

   /*
    * Retrieve the current dos system path
    * (e.g.: C:\reactos) from the given arc path
    * (e.g.: multi(0)disk(0)rdisk(0)partititon(1)\reactos)
    * Format: "<arc_name>\<path> [options...]"
    */

   /* create local parameter line copy */
   ParamBuffer = ExAllocatePool (PagedPool, 256);
   strcpy (ParamBuffer, (char *)ParameterLine);
   DPRINT("%s\n", ParamBuffer);

   /* cut options off */
   p = strchr (ParamBuffer, ' ');
   if (p)
     {
	*p = 0;
     }
   DPRINT("%s\n", ParamBuffer);

   /* extract path */
   p = strchr (ParamBuffer, '\\');
   if (p)
     {
       DPRINT("Boot path: %s\n", p);
       RtlCreateUnicodeStringFromAsciiz (&BootPath, p);
       *p = 0;
     }
   else
     {
       DPRINT("Boot path: %s\n", "\\");
       RtlCreateUnicodeStringFromAsciiz (&BootPath, "\\");
     }
   DPRINT("Arc name: %s\n", ParamBuffer);
   
   /* Only arc name left - build full arc name */
   ArcNameBuffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));
   swprintf (ArcNameBuffer, L"\\ArcName\\%S", ParamBuffer);
   RtlInitUnicodeString (&ArcName, ArcNameBuffer);
   DPRINT("Arc name: %wZ\n", &ArcName);

   /* free ParamBuffer */
   ExFreePool (ParamBuffer);

   /* allocate arc device name string */
   ArcDeviceName.Length = 0;
   ArcDeviceName.MaximumLength = 256 * sizeof(WCHAR);
   ArcDeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

   InitializeObjectAttributes (&ObjectAttributes,
			       &ArcName,
			       OBJ_OPENLINK,
			       NULL,
			       NULL);

   Status = NtOpenSymbolicLinkObject (&Handle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes);
   RtlFreeUnicodeString (&ArcName);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeUnicodeString (&BootPath);
	RtlFreeUnicodeString (&ArcDeviceName);
	CPRINT("NtOpenSymbolicLinkObject() failed (Status %x)\n",
	         Status);

	KeBugCheck (0x0);
     }

   Status = NtQuerySymbolicLinkObject (Handle,
				       &ArcDeviceName,
				       &Length);
   NtClose (Handle);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeUnicodeString (&BootPath);
	RtlFreeUnicodeString (&ArcDeviceName);
	CPRINT("NtQuerySymbolicObject() failed (Status %x)\n",
		 Status);

	KeBugCheck (0x0);
     }
   DPRINT("Length: %lu ArcDeviceName: %wZ\n", Length, &ArcDeviceName);


   /* allocate device name string */
   DriveDeviceName.Length = 0;
   DriveDeviceName.MaximumLength = 256 * sizeof(WCHAR);
   DriveDeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

   for (i = 0; i < 26; i++)
     {
	swprintf (DriveNameBuffer, L"\\??\\%C:", 'A' + i);
	RtlInitUnicodeString (&DriveName,
			      DriveNameBuffer);

	InitializeObjectAttributes (&ObjectAttributes,
				    &DriveName,
				    OBJ_OPENLINK,
				    NULL,
				    NULL);

	Status = NtOpenSymbolicLinkObject (&Handle,
					   SYMBOLIC_LINK_ALL_ACCESS,
					   &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT("Failed to open link %wZ\n",
		    &DriveName);
	     continue;
	  }

	Status = NtQuerySymbolicLinkObject (Handle,
					    &DriveDeviceName,
					    &Length);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT("Failed query open link %wZ\n",
		    &DriveName);
	     continue;
	  }
	DPRINT("Opened link: %wZ ==> %wZ\n",
	       &DriveName, &DriveDeviceName);

	if (!RtlCompareUnicodeString (&ArcDeviceName, &DriveDeviceName, FALSE))
	  {
	     DPRINT("DOS Boot path: %c:%wZ\n", 'A' + i, &BootPath);
	     swprintf(SharedUserData->NtSystemRoot,
		      L"%C:%wZ", 'A' + i, &BootPath);
	     
	     BootDriveFound = TRUE;
	  }

	NtClose (Handle);
     }

   RtlFreeUnicodeString (&BootPath);
   RtlFreeUnicodeString (&DriveDeviceName);
   RtlFreeUnicodeString (&ArcDeviceName);

   DPRINT("DosDeviceMap: 0x%x\n", SharedUserData->DosDeviceMap);

   if (BootDriveFound == FALSE)
     {
	DbgPrint("No system drive found!\n");
	KeBugCheck (0x0);
     }
}

VOID
ExpVerifyOffsets()
{
  /*
   * Fail at runtime if someone has changed various structures without
   * updating the offsets used for the assembler code.
   */
  assert(FIELD_OFFSET(KTHREAD, InitialStack) == KTHREAD_INITIAL_STACK);
  assert(FIELD_OFFSET(KTHREAD, Teb) == KTHREAD_TEB);
  assert(FIELD_OFFSET(KTHREAD, KernelStack) == KTHREAD_KERNEL_STACK);
  assert(FIELD_OFFSET(KTHREAD, PreviousMode) == KTHREAD_PREVIOUS_MODE);
  assert(FIELD_OFFSET(KTHREAD, TrapFrame) == KTHREAD_TRAP_FRAME);
  assert(FIELD_OFFSET(KTHREAD, CallbackStack) == KTHREAD_CALLBACK_STACK);
  assert(FIELD_OFFSET(ETHREAD, ThreadsProcess) == ETHREAD_THREADS_PROCESS);
  assert(FIELD_OFFSET(KPROCESS, DirectoryTableBase) == 
	 KPROCESS_DIRECTORY_TABLE_BASE);
  assert(FIELD_OFFSET(KTRAP_FRAME, Reserved9) == KTRAP_FRAME_RESERVED9);
  assert(FIELD_OFFSET(KV86M_TRAP_FRAME, regs) == TF_REGS);
  assert(FIELD_OFFSET(KV86M_TRAP_FRAME, orig_ebp) == TF_ORIG_EBP);
  assert(FIELD_OFFSET(IKPCR, KPCR.Tib.ExceptionList) == KPCR_EXCEPTION_LIST);
  assert(FIELD_OFFSET(IKPCR, KPCR.Self) == KPCR_SELF);
  assert(FIELD_OFFSET(IKPCR, KPCR.TSS) == KPCR_TSS);
  assert(FIELD_OFFSET(IKPCR, CurrentThread) == KPCR_CURRENT_THREAD);
  assert(FIELD_OFFSET(KTSS, Esp0) == KTSS_ESP0);
  assert(FIELD_OFFSET(KV86M_REGISTERS, Ebp) == KV86M_REGISTERS_EBP);
  assert(FIELD_OFFSET(KV86M_REGISTERS, Edi) == KV86M_REGISTERS_EDI);
  xbp('S');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Esi) == KV86M_REGISTERS_ESI);
  xbp('T');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Edx) == KV86M_REGISTERS_EDX);
  xbp('U');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Ecx) == KV86M_REGISTERS_ECX);
  xbp('V');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Ebx) == KV86M_REGISTERS_EBX);
  xbp('W');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Eax) == KV86M_REGISTERS_EAX);
  xbp('X');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Ds) == KV86M_REGISTERS_DS);
  xbp('Y');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Es) == KV86M_REGISTERS_ES);
  xbp('Z');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Fs) == KV86M_REGISTERS_FS);
  xbp('A');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Gs) == KV86M_REGISTERS_GS);
  xbp('B');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Eip) == KV86M_REGISTERS_EIP);
  xbp('C');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Cs) == KV86M_REGISTERS_CS);
  xbp('D');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Eflags) == KV86M_REGISTERS_EFLAGS);
  xbp('E');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Esp) == KV86M_REGISTERS_ESP);
  xbp('F');
  assert(FIELD_OFFSET(KV86M_REGISTERS, Ss) == KV86M_REGISTERS_SS);
  xbp('G');
}

VOID
ExpInitializeExecutive(VOID)
{
  ULONG BootDriverCount;
  ULONG i;
  ULONG start;
  ULONG length;
  PCHAR name;
  CHAR str[50];
  NTSTATUS Status;

  ExpVerifyOffsets();

  LdrInit1();
  xbp('0');

  KeLowerIrql(DISPATCH_LEVEL);
  xbp('1');
  
  NtEarlyInitVdm();
  xbp('2');
  
  MmInit1(FirstKrnlPhysAddr,
	  LastKrnlPhysAddr,
	  LastKernelAddress,
	  (PADDRESS_RANGE)&KeMemoryMap,
	  KeMemoryMapRangeCount);
  xbp('3');

  /* create default nls tables */
  RtlpInitNlsTables();
  xbp('4');
  
  /*
   * Initialize the kernel debugger
   */
  KdInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  MmInit2();
  KeInit2();
  
  KeLowerIrql(PASSIVE_LEVEL);

  if (!SeInit1())
    KeBugCheck(SECURITY_INITIALIZATION_FAILED);

  ObInit();

  if (!SeInit2())
    KeBugCheck(SECURITY1_INITIALIZATION_FAILED);

  PiInitProcessManager();

  KdInit1();

  if (KdPollBreakIn ())
    {
      DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
    }

  /*
   * Display version number and copyright/warranty message
   */
  HalDisplayString("Starting ReactOS "KERNEL_VERSION_STR" (Build "
		   KERNEL_VERSION_BUILD_STR")\n");
  HalDisplayString(RES_STR_LEGAL_COPYRIGHT);
  HalDisplayString("\n\nReactOS is free software, covered by the GNU General "
		   "Public License, and you\n");
  HalDisplayString("are welcome to change it and/or distribute copies of it "
		   "under certain\n"); 
  HalDisplayString("conditions. There is absolutely no warranty for "
		   "ReactOS.\n\n");
  xbp('A');

  /* Initialize all processors */
  KeNumberProcessors = 0;
  while (!HalAllProcessorsStarted())
    {
      PVOID ProcessorStack;
  xbp('B');
      if (KeNumberProcessors != 0)
	{
	  KePrepareForApplicationProcessorInit(KeNumberProcessors);
	  PsPrepareForApplicationProcessorInit(KeNumberProcessors);
	}
  xbp('C');
      /* Allocate a stack for use when booting the processor */
      /* FIXME: The nonpaged memory for the stack is not released after use */
      ProcessorStack = 
	ExAllocatePool(NonPagedPool, MM_STACK_SIZE) + MM_STACK_SIZE;
  xbp('D');

      Ki386InitialStackArray[((int)KeNumberProcessors)] = 
	(PVOID)(ProcessorStack - MM_STACK_SIZE);
  xbp('E');
      HalInitializeProcessor(KeNumberProcessors, ProcessorStack);
  xbp('F');
      KeNumberProcessors++;
  xbp('G');
    }
  xbp('H');

  if (KeNumberProcessors > 1)
    {
      sprintf(str,
	      "Found %d system processors. [%lu MB Memory]\n",
	      KeNumberProcessors,
	      (KeLoaderBlock.MemHigher + 1088)/ 1024);
    }
  else
    {
      sprintf(str,
	      "Found 1 system processor. [%lu MB Memory]\n",
	      (KeLoaderBlock.MemHigher + 1088)/ 1024);
    }
  HalDisplayString(str);

  /*
   * Initialize various critical subsystems
   */
  HalInitSystem(1, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  ExInit();
  xbp('Q');
  IoInit();
  xbp('R');
  PoInit();
  xbp('A');
  LdrInitModuleManagement();
  xbp('B');
  CmInitializeRegistry();
  xbp('C');
  NtInit();
  xbp('D');
  MmInit3();
  xbp('E');
  CcInit();
  xbp('F');
  KdInit2();
  xbp('G');
  
  /* Report all resources used by hal */
  HalReportResourceUsage();
  
  /*
   * Initalize services loaded at boot time
   */
  DPRINT("%d files loaded\n",KeLoaderBlock.ModsCount);
  for (i=0; i < KeLoaderBlock.ModsCount; i++)
    {
      CPRINT("Module: '%s' at %08lx, length 0x%08lx\n",
       KeLoaderModules[i].String,
       KeLoaderModules[i].ModStart,
       KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart);
    }

  /*  Pass 1: load nls files  */
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      name = (PCHAR)KeLoaderModules[i].String;
      if (RtlpCheckFileNameExtension(name, ".nls"))
	{
	  ULONG Mod2Start = 0;
	  ULONG Mod2End = 0;
	  ULONG Mod3Start = 0;
	  ULONG Mod3End = 0;

	  name = (PCHAR)KeLoaderModules[i+1].String;
	  if (RtlpCheckFileNameExtension(name, ".nls"))
	    {
	      Mod2Start = (ULONG)KeLoaderModules[i+1].ModStart;
	      Mod2End = (ULONG)KeLoaderModules[i+1].ModEnd;

	      name = (PCHAR)KeLoaderModules[i+2].String;
	      if (RtlpCheckFileNameExtension(name, ".nls"))
	        {
		  Mod3Start = (ULONG)KeLoaderModules[i+2].ModStart;
		  Mod3End = (ULONG)KeLoaderModules[i+2].ModEnd;
	        }
	    }

	  /* Initialize nls sections */
	  RtlpInitNlsSections((ULONG)KeLoaderModules[i].ModStart,
			      (ULONG)KeLoaderModules[i].ModEnd,
			      Mod2Start,
			      Mod2End,
			      Mod3Start,
			      Mod3End);
	  break;
	}
    }

  /*  Pass 2: load registry chunks passed in  */
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;
      name = (PCHAR)KeLoaderModules[i].String;
      if (RtlpCheckFileNameExtension(name, "") ||
	  RtlpCheckFileNameExtension(name, ".hiv"))
	{
	  CPRINT("Process registry chunk at %08lx\n", start);
	  CmImportHive((PCHAR)start, length);
	}
    }

  /* Initialize volatile registry settings */
  CmInit2((PCHAR)KeLoaderBlock.CommandLine);

  /*
   * Enter the kernel debugger before starting up the boot drivers
   */
#ifdef KDBG
  KdbEnter();
#endif /* KDBG */

  IoCreateDriverList();

  /*  Pass 3: process boot loaded drivers  */
  BootDriverCount = 0;
  for (i=1; i < KeLoaderBlock.ModsCount; i++)
    {
      start = KeLoaderModules[i].ModStart;
      length = KeLoaderModules[i].ModEnd - start;
      name = (PCHAR)KeLoaderModules[i].String;
      if (RtlpCheckFileNameExtension(name, ".sys") ||
	  RtlpCheckFileNameExtension(name, ".sym"))
	{
	  CPRINT("Initializing driver '%s' at %08lx, length 0x%08lx\n",
	         name, start, length);
	  LdrInitializeBootStartDriver((PVOID)start, name, length);
	}
      if (RtlpCheckFileNameExtension(name, ".sys"))
	BootDriverCount++;
    }

  if (BootDriverCount == 0)
    {
      DbgPrint("No boot drivers available.\n");
      KeBugCheck(0);
    }

  /* Create ARC names for boot devices */
  IoCreateArcNames();

  /* Create the SystemRoot symbolic link */
  CPRINT("CommandLine: %s\n", (PUCHAR)KeLoaderBlock.CommandLine);
  Status = IoCreateSystemRootLink((PUCHAR)KeLoaderBlock.CommandLine);
  if (!NT_SUCCESS(Status))
    KeBugCheck(INACCESSIBLE_BOOT_DEVICE);

#ifdef DBGPRINT_FILE_LOG
  /* On the assumption that we can now access disks start up the debug
     logger thread */
  DebugLogInit2();
#endif /* DBGPRINT_FILE_LOG */


  PiInitDefaultLocale();

  /*
   * Start the motherboard enumerator (the HAL)
   */
  HalInitSystem(2, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
#if 0
  /*
   * Load boot start drivers
   */
  IopLoadBootStartDrivers();
#else
  /*
   * Load Auto configured drivers
   */
  LdrLoadAutoConfigDrivers();
#endif

  IoDestroyDriverList();

  /*
   * Assign drive letters
   */
  IoAssignDriveLetters ((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock,
			NULL,
			NULL,
			NULL);

  /*
   * Initialize shared user page:
   *  - set dos system path, dos device map, etc.
   */
  InitSystemSharedUserPage ((PUCHAR)KeLoaderBlock.CommandLine);

  /*
   *  Launch initial process
   */
  LdrLoadInitialProcess();

  PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID
KiSystemStartup(BOOLEAN BootProcessor)
{
  HalInitSystem (0, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

  if (BootProcessor)
    {
      /* Never returns */
      ExpInitializeExecutive();
      KeBugCheck(0);
    }
  /* Do application processor initialization */
  KeApplicationProcessorInit();
  PsApplicationProcessorInit();
  KeLowerIrql(PASSIVE_LEVEL);
  PsIdleThreadMain(NULL);
  KeBugCheck(0);
  for(;;);
}

VOID
_main (ULONG MultiBootMagic, PLOADER_PARAMETER_BLOCK _LoaderBlock)
/*
 * FUNCTION: Called by the boot loader to start the kernel
 * ARGUMENTS:
 *          LoaderBlock = Pointer to boot parameters initialized by the boot 
 *                        loader
 * NOTE: The boot parameters are stored in low memory which will become
 * invalid after the memory managment is initialized so we make a local copy.
 */
{
  ULONG i;
  ULONG size;
  ULONG last_kernel_address;
  extern ULONG _bss_end__;
  ULONG HalBase;
  ULONG DriverBase;
  ULONG DriverSize;

  /* Low level architecture specific initialization */
  KeInit1();

  /*
   * Copy the parameters to a local buffer because lowmem will go away
   */
  memcpy(&KeLoaderBlock, _LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));
  memcpy(&KeLoaderModules[1], (PVOID)KeLoaderBlock.ModsAddr,
	 sizeof(LOADER_MODULE) * KeLoaderBlock.ModsCount);
  KeLoaderBlock.ModsCount++;
  KeLoaderBlock.ModsAddr = (ULONG)&KeLoaderModules;

  /*
   * Convert a path specification in the grub format to one understood by the
   * rest of the kernel.
   */
  if (((PUCHAR)_LoaderBlock->CommandLine)[0] == '(')
    {
      ULONG DiskNumber, PartNumber;
      PCH p;
      CHAR Temp[256];
      PCH options;
      PCH s1;

      if (((PUCHAR)_LoaderBlock->CommandLine)[1] == 'h' &&
	  ((PUCHAR)_LoaderBlock->CommandLine)[2] == 'd')
	{
	  DiskNumber = ((PUCHAR)_LoaderBlock->CommandLine)[3] - '0';
	  PartNumber = ((PUCHAR)_LoaderBlock->CommandLine)[5] - '0';
	}
      strcpy(Temp, &((PUCHAR)_LoaderBlock->CommandLine)[7]);
      if ((options = strchr(Temp, ' ')) != NULL)
	{
	  *options = 0;
	  options++;
	}
      else
	{
	  options = "";
	}
      if ((s1 = strrchr(Temp, '/')) != NULL)
	{
	  *s1 = 0;
	  if ((s1 = strrchr(Temp, '/')) != NULL)
	    {
	      *s1 = 0;
	    }
	}
      sprintf(KeLoaderCommandLine, 
	      "multi(0)disk(0)rdisk(%ld)partition(%ld)%s %s",
	      DiskNumber, PartNumber + 1, Temp, options);

      p = KeLoaderCommandLine;
      while (*p != 0 && *p != ' ')
	{
	  if ((*p) == '/')
	    {
	      (*p) = '\\';
	    }
	  p++;
	}
      DPRINT1("Command Line: %s\n", KeLoaderCommandLine);
    }
  else
    {
      strcpy(KeLoaderCommandLine, (PUCHAR)_LoaderBlock->CommandLine);
    }
  KeLoaderBlock.CommandLine = (ULONG)KeLoaderCommandLine;
 
  strcpy(KeLoaderModuleStrings[0], "ntoskrnl.exe");
  KeLoaderModules[0].String = (ULONG)KeLoaderModuleStrings[0];
  KeLoaderModules[0].ModStart = 0xC0000000;
  KeLoaderModules[0].ModEnd = PAGE_ROUND_UP((ULONG)&_bss_end__);
  for (i = 1; i < KeLoaderBlock.ModsCount; i++)
    {
      strcpy(KeLoaderModuleStrings[i], (PUCHAR)KeLoaderModules[i].String);
      KeLoaderModules[i].ModStart -= 0x200000;
      KeLoaderModules[i].ModStart += 0xc0000000;
      KeLoaderModules[i].ModEnd -= 0x200000;
      KeLoaderModules[i].ModEnd += 0xc0000000;
      KeLoaderModules[i].String = (ULONG)KeLoaderModuleStrings[i];
    }

#ifdef HAL_DBG
  HalnInitializeDisplay((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);
#endif

  HalBase = KeLoaderModules[1].ModStart;
  DriverBase = 
    PAGE_ROUND_UP(KeLoaderModules[KeLoaderBlock.ModsCount - 1].ModEnd);

  /*
   * Process hal.dll
   */
  LdrSafePEProcessModule((PVOID)HalBase, (PVOID)DriverBase, (PVOID)0xC0000000, &DriverSize);

  LdrHalBase = (ULONG_PTR)DriverBase;
  last_kernel_address = DriverBase + DriverSize;

  /*
   * Process ntoskrnl.exe
   */
  LdrSafePEProcessModule((PVOID)0xC0000000, (PVOID)0xC0000000, (PVOID)DriverBase, &DriverSize);

  FirstKrnlPhysAddr = KeLoaderModules[0].ModStart - 0xc0000000 + 0x200000;
  LastKrnlPhysAddr = last_kernel_address - 0xc0000000 + 0x200000;
  LastKernelAddress = last_kernel_address;

#ifndef ACPI
  /* FIXME: VMware does not like it when ReactOS is using the BIOS memory map */
  KeLoaderBlock.Flags &= ~MB_FLAGS_MMAP_INFO;
#endif

  KeMemoryMapRangeCount = 0;
  if (KeLoaderBlock.Flags & MB_FLAGS_MMAP_INFO)
    {
      /* We have a memory map from the nice BIOS */
      size = *((PULONG)(KeLoaderBlock.MmapAddr - sizeof(ULONG)));
      i = 0;
      while (i < KeLoaderBlock.MmapLength)
        {
          memcpy (&KeMemoryMap[KeMemoryMapRangeCount],
            (PVOID)(KeLoaderBlock.MmapAddr + i),
	          sizeof(ADDRESS_RANGE));
          KeMemoryMapRangeCount++;
          i += size;
        }
    }

  KiSystemStartup(1);
}

/* EOF */

