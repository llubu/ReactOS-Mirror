/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#define UNICODE
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wstring.h>
#include <string.h>
#include <ddk/rtl.h>
#include <ddk/li.h>

/* GLOBALS ******************************************************************/

static unsigned char CommandLineA[MAX_PATH];

/* FUNCTIONS ****************************************************************/

LPSTR STDCALL GetCommandLineA(VOID)
{
	WCHAR *CommandLineW;
	ULONG i = 0;
  	
	CommandLineW = GetCommandLineW();
   	while ((CommandLineW[i])!=0 && i < MAX_PATH)
     	{
		CommandLineA[i] = (unsigned char)CommandLineW[i];
		i++;
     	}
   	CommandLineA[i] = 0;
	return CommandLineA;
}

LPWSTR STDCALL GetCommandLineW(VOID)
{
	return GetCurrentPeb()->StartupInfo->CommandLine;
}

