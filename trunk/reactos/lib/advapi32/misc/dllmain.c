/* $Id: dllmain.c,v 1.8 2004/08/15 17:03:14 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/misc/dllmain.c
 * PURPOSE:         Library main function
 * PROGRAMMER:      ???
 * UPDATE HISTORY:
 *                  Created ???
 */

#include "advapi32.h"
#define NDEBUG
#include <debug.h>

extern BOOL RegInitialize(VOID);
extern BOOL RegCleanup(VOID);

INT STDCALL
DllMain(PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{
   switch (dwReason)
     {
     case DLL_PROCESS_ATTACH:
	DisableThreadLibraryCalls(hinstDll);
	RegInitialize();
	break;

     case DLL_PROCESS_DETACH:
	RegCleanup();
	break;
     }

   return TRUE;
}

/* EOF */
