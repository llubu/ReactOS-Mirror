/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/napi.c
 * PURPOSE:         Native API support routines
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

#include <internal/service.h>
#include <ntdll/napi.h>

/* FUNCTIONS *****************************************************************/




KE_SERVICE_DESCRIPTOR_TABLE_ENTRY
__declspec(dllexport)
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] =
{
	{ MainSSDT, NULL, NUMBER_OF_SYSCALLS, MainSSPT },
	{ NULL,    NULL,   0,   NULL   },
	{ NULL,    NULL,   0,   NULL   },
	{ NULL,    NULL,   0,   NULL   }
};

KE_SERVICE_DESCRIPTOR_TABLE_ENTRY KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES] =
{
	{ MainSSDT, NULL, NUMBER_OF_SYSCALLS, MainSSPT },
	{ NULL,    NULL,   0,   NULL   },
	{ NULL,    NULL,   0,   NULL   },
	{ NULL,    NULL,   0,   NULL   }
};
