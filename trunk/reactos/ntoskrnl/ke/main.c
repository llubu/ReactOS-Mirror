/* $Id: main.c,v 1.52 2000/07/04 11:11:03 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/main.c
 * PURPOSE:         Initalizes the kernel
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                28/05/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <reactos/buildno.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/module.h>
#include <internal/ldr.h>
#include <internal/ex.h>
#include <internal/ps.h>
#include <internal/hal.h>

#include <internal/mmhal.h>
#include <internal/i386/segment.h>

//#define NDEBUG
#include <internal/debug.h>

/* DATA *********************************************************************/

ULONG EXPORTED NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG EXPORTED NtGlobalFlag = 0;
CHAR  EXPORTED KeNumberProcessors = 1;
LOADER_PARAMETER_BLOCK KeLoaderBlock;

/* FUNCTIONS ****************************************************************/

static VOID CreateSystemRootLink (LPWSTR Device)
{
	UNICODE_STRING LinkName;
	UNICODE_STRING DeviceName;

	RtlInitUnicodeString (&LinkName,
			      L"\\SystemRoot");

	RtlInitUnicodeString (&DeviceName,
			      Device);

	IoCreateSymbolicLink (&LinkName,
			      &DeviceName);
}

void _main (PLOADER_PARAMETER_BLOCK LoaderBlock)
/*
 * FUNCTION: Called by the boot loader to start the kernel
 * ARGUMENTS:
 *          _bp = Pointer to boot parameters initialized by the boot loader
 * NOTE: The boot parameters are stored in low memory which will become
 * invalid after the memory managment is initialized so we make a local copy.
 */
{
   unsigned int i;
   unsigned int last_kernel_address;
   ULONG start, start1;
   
   /*
    * Copy the parameters to a local buffer because lowmem will go away
    */
   memcpy (&KeLoaderBlock, LoaderBlock, sizeof(LOADER_PARAMETER_BLOCK));

   /*
    * FIXME: Preliminary hack!!!!
    * Initializes the kernel parameter line.
    * This should be done by the boot loader.
    */
   strcpy (KeLoaderBlock.kernel_parameters, "/DEBUGPORT=SCREEN");

   /*
    * Initialization phase 0
    */
   HalInitSystem (0, &KeLoaderBlock);

   HalDisplayString("Starting ReactOS "KERNEL_VERSION_STR" (Build "KERNEL_VERSION_BUILD_STR")\n");

   last_kernel_address = KERNEL_BASE;
   for (i=0; i <= KeLoaderBlock.nr_files; i++)
     {
	last_kernel_address = last_kernel_address +
	  PAGE_ROUND_UP(KeLoaderBlock.module_length[i]);
     }

   MmInit1(&KeLoaderBlock, last_kernel_address);

   /*
    * Initialize the kernel debugger
    */
   KdInitSystem (0, &KeLoaderBlock);
   if (KdPollBreakIn ())
     {
	DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);
     }

   /*
    * Initialization phase 1
    * Initalize various critical subsystems
    */
   HalInitSystem (1, &KeLoaderBlock);
   MmInit2();
   KeInit();
   ExInit();
   ObInit();
   PiInitProcessManager();
   IoInit();
   LdrInitModuleManagement();
   CmInitializeRegistry();
   NtInit();
   MmInit3();
   
   /* Report all resources used by hal */
   HalReportResourceUsage ();
   
   /*
    * Initalize services loaded at boot time
    */
   DPRINT1("%d files loaded\n",KeLoaderBlock.nr_files);

  /*  Pass 1: load registry chunks passed in  */
  start = KERNEL_BASE + PAGE_ROUND_UP(KeLoaderBlock.module_length[0]);
  for (i = 1; i < KeLoaderBlock.nr_files; i++)
    {
      if (!strcmp ((PCHAR) start, "REGEDIT4"))
        {
          DPRINT1("process registry chunk at %08lx\n", start);
          CmImportHive((PCHAR) start);
        }
      start = start + KeLoaderBlock.module_length[i];
    }

  /*  Pass 2: process boot loaded drivers  */
  start = KERNEL_BASE + PAGE_ROUND_UP(KeLoaderBlock.module_length[0]);
  start1 = start + KeLoaderBlock.module_length[1];
  for (i=1;i<KeLoaderBlock.nr_files;i++)
    {
      if (strcmp ((PCHAR) start, "REGEDIT4"))
        {
          DPRINT1("process module at %08lx\n", start);
          LdrProcessDriver((PVOID)start);
        }
      start = start + KeLoaderBlock.module_length[i];
    }
   
   /* Create the SystemRoot symbolic link */
   /* Hardcoded to 'C:\reactos' but this will change. */
   CreateSystemRootLink (L"\\Device\\Harddisk0\\Partition1\\reactos");
   
   /*
    * Load Auto configured drivers
    */
   LdrLoadAutoConfigDrivers();
   
   /*
    * Assign drive letters
    */
   IoAssignDriveLetters (&KeLoaderBlock,
                         NULL,
                         NULL,
                         NULL);

  /*
   *  Launch initial process
   */
   LdrLoadInitialProcess();
   
   DbgPrint("Finished main()\n");
   PsTerminateSystemThread(STATUS_SUCCESS);
}

/* EOF */
