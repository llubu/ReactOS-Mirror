/* $Id: isa.c,v 1.6 2003/12/28 22:38:09 fireball Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/isa.c
 * PURPOSE:         Interfaces to the ISA bus
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               05/06/98: Created
 */

/* INCLUDES ***************************************************************/

#include <roscfg.h>
#include <ddk/ntddk.h>
#include <bus.h>
#ifdef MP
#include <mps.h>
#endif


/* FUNCTIONS *****************************************************************/

BOOL HalIsaProbe(VOID)
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
   return(TRUE);
}


BOOLEAN STDCALL
HalpTranslateIsaBusAddress(PBUS_HANDLER BusHandler,
			   ULONG BusNumber,
			   PHYSICAL_ADDRESS BusAddress,
			   PULONG AddressSpace,
			   PPHYSICAL_ADDRESS TranslatedAddress)
{
   BOOLEAN Result;

   Result = HalTranslateBusAddress(PCIBus,
				   BusNumber,
				   BusAddress,
				   AddressSpace,
				   TranslatedAddress);
   if (Result != FALSE)
     return Result;

   Result = HalTranslateBusAddress(Internal,
				   BusNumber,
				   BusAddress,
				   AddressSpace,
				   TranslatedAddress);
   return Result;
}

ULONG STDCALL
HalpGetIsaInterruptVector(PVOID BusHandler,
			  ULONG BusNumber,
			  ULONG BusInterruptLevel,
			  ULONG BusInterruptVector,
			  PKIRQL Irql,
			  PKAFFINITY Affinity)
{
#ifdef MP
  *Irql = (KIRQL)(PROFILE_LEVEL - BusInterruptVector);
  *Affinity = 0xFFFFFFFF;
  return IRQ2VECTOR(BusInterruptVector);
#else
  *Irql = (KIRQL)(PROFILE_LEVEL - BusInterruptVector);
  *Affinity = 0xFFFFFFFF;
  return BusInterruptVector;
#endif
}
/* EOF */
