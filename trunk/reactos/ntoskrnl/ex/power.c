/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/power.c
 * PURPOSE:         Power managment
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Added reboot support 30/01/99
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL 
NtSetSystemPowerState(IN POWER_ACTION SystemAction,
		      IN SYSTEM_POWER_STATE MinSystemState,
		      IN ULONG Flags)
{
  /* Windows 2000 only */
  return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @implemented
 */
NTSTATUS STDCALL 
NtShutdownSystem(IN SHUTDOWN_ACTION Action)
{
   if (Action > ShutdownPowerOff)
     return STATUS_INVALID_PARAMETER;

   IoShutdownRegisteredDevices();
   CmShutdownRegistry();
   IoShutdownRegisteredFileSystems();

   PiShutdownProcessManager();
   MiShutdownMemoryManager();
   
   if (Action == ShutdownNoReboot)
     {
#if 0
        /* Switch off */
        HalReturnToFirmware (FIRMWARE_OFF);
#else
        PopSetSystemPowerState(PowerSystemShutdown);

	while (TRUE)
	  {
	    Ke386DisableInterrupts();
	    Ke386HaltProcessor();
	  }
#endif
     }
   else if (Action == ShutdownReboot)
     {
        HalReturnToFirmware (FIRMWARE_REBOOT);
     }
   else
     {
        HalReturnToFirmware (FIRMWARE_HALT);
     }
   
   return STATUS_SUCCESS;
}

/* EOF */

