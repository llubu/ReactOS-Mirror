/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            mkernel/hal/eisa.c
 * PURPOSE:         Interfaces to the ISA bus
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               05/06/98: Created
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

/* FUNCTIONS *****************************************************************/

BOOL HalIsaProbe()
/*
 * FUNCTION: Probes for an ISA bus
 * RETURNS: True if detected
 * NOTE: Since ISA is the default we are called last and always return
 * true
 */
{
   DbgPrint("Assuming ISA bus\n");
   
   /*
    * Probe for plug and play support
    */
   
}
