/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/catch.c
 * PURPOSE:              Exception handling
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID ExRaiseStatus(NTSTATUS Status)
{
   DbgPrint("ExRaiseStatus(%x)\n",Status);
   for(;;);
}


NTSTATUS
STDCALL
NtRaiseException (
	IN	PEXCEPTION_RECORD	ExceptionRecord,
	IN	PCONTEXT		Context,
	IN	BOOL			IsDebugger		OPTIONAL
	)
{
	UNIMPLEMENTED;
}
