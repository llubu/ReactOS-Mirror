/* $Id$
 *
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/kmap.c
 * PURPOSE:      Implements the kernel memory pool
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/* FUNCTIONS ***************************************************************/
NTSTATUS
MiZeroPage(PFN_TYPE Page)
{
   PVOID TempAddress;

   TempAddress = MmCreateHyperspaceMapping(Page);
   if (TempAddress == NULL)
   {
      return(STATUS_NO_MEMORY);
   }
   memset(TempAddress, 0, PAGE_SIZE);
   MmDeleteHyperspaceMapping(TempAddress);
   return(STATUS_SUCCESS);
}

NTSTATUS
MiCopyFromUserPage(PFN_TYPE DestPage, PVOID SourceAddress)
{
   PVOID TempAddress;

   TempAddress = MmCreateHyperspaceMapping(DestPage);
   if (TempAddress == NULL)
   {
      return(STATUS_NO_MEMORY);
   }
   memcpy(TempAddress, SourceAddress, PAGE_SIZE);
   MmDeleteHyperspaceMapping(TempAddress);
   return(STATUS_SUCCESS);
}

