/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            mkernel/hal/eisa.c
 * PURPOSE:         Interfaces to the PCI bus
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               05/06/98: Created
 */

/*
 * NOTES: Sections copied from the Linux pci support
 */

/* INCLUDES ***************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/kernel.h>
#include <internal/mm.h>
#include <internal/string.h>
#include <internal/hal/page.h>

/* FUNCTIONS **************************************************************/

#define PCI_SERVICE (('$' << 0) + ('P' << 8) + ('C' << 16) + ('I'<<24))

BOOL HalPciProbe()
/*
 * FUNCTION: Probes for an PCI bus
 * RETURNS: True if detected
 */
{
   if (Hal_bios32_is_service_present(PCI_SERVICE))
     {
	printk("Detected PCI service\n");
	return(TRUE);
     }
   return(FALSE);
}
