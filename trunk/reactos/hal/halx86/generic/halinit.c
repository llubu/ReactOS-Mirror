/* $Id$
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/hal/x86/halinit.c
 * PURPOSE:       Initalize the x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

PVOID HalpZeroPageMapping = NULL;
HALP_HOOKS HalpHooks;

/* FUNCTIONS ***************************************************************/

NTSTATUS
STDCALL
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
{
	return STATUS_SUCCESS;
}

BOOLEAN STDCALL
HalInitSystem (ULONG BootPhase,
               PLOADER_PARAMETER_BLOCK LoaderBlock)
{
  if (BootPhase == 0)
    {
      RtlZeroMemory(&HalpHooks, sizeof(HALP_HOOKS));
      HalpInitPhase0(LoaderBlock);      
    }
  else if (BootPhase == 1)
    {
      /* Initialize display and make the screen black */
      HalInitializeDisplay (LoaderBlock);
      HalpInitBusHandlers();
      HalpInitDma();

      /* Enumerate the devices on the motherboard */
      HalpStartEnumerator();
   }
  else if (BootPhase == 2)
    {
      PHYSICAL_ADDRESS Null = {{0}};

      /* Go to blue screen */
      HalClearDisplay (0x17); /* grey on blue */
      
      HalpZeroPageMapping = MmMapIoSpace(Null, PAGE_SIZE, MmNonCached);
    }

  return TRUE;
}

/* EOF */
