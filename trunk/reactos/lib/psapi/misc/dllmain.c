/* $Id: dllmain.c,v 1.6 2004/11/03 22:43:00 weiden Exp $
*/
/*
 * COPYRIGHT:   None
 * LICENSE:     Public domain
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/misc/malloc.c
 * PURPOSE:     PSAPI.DLL main procedure
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              28/11/2001: Created (Emanuele Aliberti <eal@users.sf.net>)
 *              30/08/2002: Minimal tweak (KJK::Hyperion <noog@libero.it>)
 */
#include "precomp.h"

#define NDEBUG
#include <debug.h>

BOOLEAN STDCALL DllMain
(
 PVOID hinstDll,
 ULONG dwReason,
 PVOID reserved
)
{
 if(dwReason == DLL_PROCESS_ATTACH)
  /* don't bother calling the entry point on thread startup - PSAPI.DLL doesn't
     store any per-thread data */
  LdrDisableThreadCalloutsForDll(hinstDll);

 return (TRUE);
}

/* EOF */
