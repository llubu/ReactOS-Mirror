/* $Id: halinit.c,v 1.14 2000/07/24 23:50:13 ekohl Exp $
 *
 * COPYRIGHT:     See COPYING in the top level directory
 * PROJECT:       ReactOS kernel
 * FILE:          ntoskrnl/hal/x86/halinit.c
 * PURPOSE:       Initalize the uniprocessor, x86 hal
 * PROGRAMMER:    David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              11/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal.h>
#include <internal/ntoskrnl.h>
#include <internal/halio.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN
STDCALL
HalInitSystem (
	ULONG			BootPhase,
	PLOADER_PARAMETER_BLOCK	LoaderBlock
	)
{
   if (BootPhase == 0)
   {
      HalInitializeDisplay (LoaderBlock);
      HalpCalibrateStallExecution ();
      HalpInitPICs ();
   }
   else
   {
      HalpInitBusHandlers ();

   }

   return TRUE;
}

/* EOF */
