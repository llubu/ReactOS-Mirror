/* $Id: csrss.c,v 1.11 2002/08/20 20:37:17 hyperion Exp $
 *
 * csrss.c - Client/Server Runtime subsystem
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * 
 *	19990417 (Emanuele Aliberti)
 *		Do nothing native application skeleton
 * 	19990528 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 * 	19990605 (Emanuele Aliberti)
 * 		First standalone run under ReactOS (it
 * 		actually does nothing but running).
 */
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <csrss/csrss.h>

#include "api.h"

/* Native process' entry point */

VOID NtProcessStartup(PPEB Peb)
{
   PRTL_USER_PROCESS_PARAMETERS ProcParams;
   PWSTR ArgBuffer;
   PWSTR *argv;
   ULONG argc = 0;
   int i = 0;
   int afterlastspace = 0;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE CsrssInitEvent;
   UNICODE_STRING UnicodeString;
   NTSTATUS Status;

   ProcParams = RtlNormalizeProcessParams (Peb->ProcessParameters);

   argv = (PWSTR *)RtlAllocateHeap (Peb->ProcessHeap,
                                    0, 512 * sizeof(PWSTR));
   ArgBuffer = (PWSTR)RtlAllocateHeap (Peb->ProcessHeap,
                                       0,
                                       ProcParams->CommandLine.Length + sizeof(WCHAR));
   memcpy (ArgBuffer,
           ProcParams->CommandLine.Buffer,
           ProcParams->CommandLine.Length + sizeof(WCHAR));

   while (ArgBuffer[i])
     {
	if (ArgBuffer[i] == L' ')
	  {
	     argc++;
	     ArgBuffer[i] = L'\0';
	     argv[argc-1] = &(ArgBuffer[afterlastspace]);
	     i++;
	     while (ArgBuffer[i] == L' ')
		i++;
	     afterlastspace = i;
	  }
	else
	  {
	     i++;
	  }
     }

   if (ArgBuffer[afterlastspace] != L'\0')
     {
	argc++;
	ArgBuffer[i] = L'\0';
	argv[argc-1] = &(ArgBuffer[afterlastspace]);
     }
   
   RtlInitUnicodeStringFromLiteral(&UnicodeString,
			L"\\CsrssInitDone");
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeString,
			      EVENT_ALL_ACCESS,
			      0,
			      NULL);
   Status = NtOpenEvent(&CsrssInitEvent,
			EVENT_ALL_ACCESS,
			&ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("CSR: Failed to open csrss notification event\n");
     }
   if (CsrServerInitialization (argc, argv) == TRUE)
     {

	NtSetEvent(CsrssInitEvent,
		   NULL);
	
	RtlFreeHeap (Peb->ProcessHeap,
	             0, argv);
	RtlFreeHeap (Peb->ProcessHeap,
	             0,
	             ArgBuffer);

	/* terminate the current thread only */
	NtTerminateThread( NtCurrentThread(), 0 );
     }
   else
     {
	DisplayString( L"CSR: Subsystem initialization failed.\n" );

	RtlFreeHeap (Peb->ProcessHeap,
	             0, argv);
	RtlFreeHeap (Peb->ProcessHeap,
	             0,
	             ArgBuffer);

	/*
	 * Tell SM we failed.
	 */
	NtTerminateProcess( NtCurrentProcess(), 0 );
     }
}

/* EOF */
