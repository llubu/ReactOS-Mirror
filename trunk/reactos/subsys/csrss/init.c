/* $Id: init.c,v 1.10 2000/07/07 01:16:18 phreak Exp $
 * 
 * reactos/subsys/csrss/init.c
 *
 * Initialize the CSRSS subsystem server process.
 *
 * ReactOS Operating System
 *
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <csrss/csrss.h>

#include "api.h"

/* GLOBALS ******************************************************************/

/*
 * Server's named ports.
 */
static HANDLE ApiPortHandle;


HANDLE CsrInitEvent = INVALID_HANDLE_VALUE;
HANDLE CsrHeap = INVALID_HANDLE_VALUE;

HANDLE CsrObjectDirectory = INVALID_HANDLE_VALUE;
HANDLE CsrApiPort = INVALID_HANDLE_VALUE;
HANDLE CsrSbApiPort = INVALID_HANDLE_VALUE;

UNICODE_STRING CsrDirectoryName;

extern HANDLE CsrssApiHeap;

static NTSTATUS
CsrParseCommandLine (
	ULONG ArgumentCount,
	PWSTR *ArgumentArray
	)
{
   NTSTATUS Status;
   OBJECT_ATTRIBUTES Attributes;
   ANSI_STRING       AnsiString;

   ULONG i;

   /*   DbgPrint ("Arguments: %ld\n", ArgumentCount);
   for (i = 0; i < ArgumentCount; i++)
     {
	DbgPrint ("Argument %ld: %S\n", i, ArgumentArray[i]);
	}*/


	/* create object directory ('\Windows') */
	RtlCreateUnicodeString (&CsrDirectoryName,
	                        L"\\Windows");

	InitializeObjectAttributes (&Attributes,
	                            &CsrDirectoryName,
	                            0,
	                            NULL,
	                            NULL);

	Status = NtCreateDirectoryObject(&CsrObjectDirectory,
	                                 0xF000F,
	                                 &Attributes);

	return Status;
}


/**********************************************************************
 * NAME
 * 	CsrServerInitialization
 *
 * DESCRIPTION
 * 	Create a directory object (\windows) and two named LPC ports:
 *
 * 	1. \windows\ApiPort
 * 	2. \windows\SbApiPort
 *
 * RETURN VALUE
 * 	TRUE: Initialization OK; otherwise FALSE.
 */
BOOL
STDCALL
CsrServerInitialization (
	ULONG ArgumentCount,
	PWSTR *ArgumentArray
	)
{
   NTSTATUS		Status;
   OBJECT_ATTRIBUTES	ObAttributes;
   UNICODE_STRING PortName;
   OBJECT_ATTRIBUTES RefreshEventAttr;
   UNICODE_STRING RefreshEventName;
   HANDLE RefreshEventHandle;

   Status = CsrParseCommandLine (ArgumentCount, ArgumentArray);
   if (!NT_SUCCESS(Status))
     {
	PrintString("CSR: Unable to parse the command line (Status: %x)\n", Status);
	return(FALSE);
     }
   
   /* NEW NAMED PORT: \ApiPort */
   RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
   InitializeObjectAttributes(&ObAttributes,
			      &PortName,
			      0,
			      NULL,
			      NULL);

   Status = NtCreatePort(&ApiPortHandle,
			 &ObAttributes,
			 260,
			 328,
			 0);
   if (!NT_SUCCESS(Status))
     {
	PrintString("CSR: Unable to create \\ApiPort (Status %x)\n", Status);
	return(FALSE);
     }
   CsrssApiHeap = RtlCreateHeap(HEAP_GROWABLE,
				NULL,
				65536,
				65536,
				NULL,
				NULL);
   if (CsrssApiHeap == NULL)
     {
	PrintString("CSR: Failed to create private heap, aborting\n");
	return FALSE;
     }

   CsrInitConsoleSupport();
   Status = RtlCreateUserThread(NtCurrentProcess(),
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				(PTHREAD_START_ROUTINE)Thread_Api,
				ApiPortHandle,
				NULL,
				NULL);
   if (!NT_SUCCESS(Status))
     {
	PrintString("CSR: Unable to create server thread\n");
	NtClose(ApiPortHandle);
	return FALSE;
     }
   RtlInitUnicodeString( &RefreshEventName, L"\\TextConsoleRefreshEvent" );
   InitializeObjectAttributes( &RefreshEventAttr, &RefreshEventName, NULL, NULL, NULL );
   Status = NtCreateEvent( &RefreshEventHandle, STANDARD_RIGHTS_ALL, &RefreshEventAttr, FALSE, FALSE );
   if( !NT_SUCCESS( Status ) )
     {
       PrintString( "CSR: Unable to create refresh event!\n" );
       return FALSE;
     }
   Status = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE, 0, NULL, NULL, (PTHREAD_START_ROUTINE)Console_Api, (DWORD) RefreshEventHandle, NULL, NULL );
   if( !NT_SUCCESS( Status ) )
     {
       PrintString( "CSR: Unable to create console thread\n" );
       return FALSE;
     }
   return TRUE;
}

/* EOF */
