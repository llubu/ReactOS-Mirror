/* $Id: except.c,v 1.14 2004/06/13 20:04:55 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/except.c
 * PURPOSE:         Exception functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>

UINT GlobalErrMode;
LPTOP_LEVEL_EXCEPTION_FILTER GlobalTopLevelExceptionFilter;

UINT GetErrorMode(void)
{
	return GlobalErrMode;
}


/*
 * @implemented
 */
UINT 
STDCALL
SetErrorMode(  UINT uMode  )
{
	UINT OldErrMode = GetErrorMode();
	GlobalErrMode = uMode;
	return OldErrMode;
}


/*
 * @implemented
 */
LPTOP_LEVEL_EXCEPTION_FILTER
STDCALL
SetUnhandledExceptionFilter(
    LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    )
{
    LPTOP_LEVEL_EXCEPTION_FILTER OldTopLevelExceptionFilter =
					 GlobalTopLevelExceptionFilter;
    GlobalTopLevelExceptionFilter = lpTopLevelExceptionFilter;
    return OldTopLevelExceptionFilter;
}


/*
 * @unimplemented
 */
LONG
STDCALL
UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	DWORD	dbgRet;
	HANDLE DebugPort;
	NTSTATUS errCode;

	if(ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) {
		// might check read only resource
		// Is there a debugger running ?
		errCode = NtQueryInformationProcess(NtCurrentProcess(),ProcessDebugPort,&DebugPort,sizeof(HANDLE),NULL);
		if ( !NT_SUCCESS(errCode) ) {
			SetLastErrorByStatus(errCode);
			return EXCEPTION_EXECUTE_HANDLER;
		}
		if ( DebugPort ) {
			//return EXCEPTION_CONTINUE_SEARCH;
		}
		if(GlobalTopLevelExceptionFilter != NULL) {
		dbgRet = GlobalTopLevelExceptionFilter(ExceptionInfo);
        	if(dbgRet == EXCEPTION_EXECUTE_HANDLER) 
        		return EXCEPTION_EXECUTE_HANDLER;
        	else if(dbgRet == EXCEPTION_CONTINUE_EXECUTION) 
         		return EXCEPTION_CONTINUE_EXECUTION;
		}

	}

	//if ( GetErrorMode() & SEM_NOGPFAULTERRORBOX == SEM_NOGPFAULTERRORBOX ) {
		// produce a stack trace or pop a message
		//sprintf( message, "Unhandled exception 0x%08lx at address 0x%08lx.",
             	//	ExceptionInfo->ExceptionRecord->ExceptionCode,
             	//	(DWORD)ExceptionInfo->ExceptionRecord->ExceptionAddress );
		//MessageBoxA( 0, message, "Error", MB_OK | MB_ICONHAND );
	
	//}
	// Returning EXCEPTION_EXECUTE_HANDLER means that the code in 
	// the __execept block will be executed. Normally this will end up in a
	// Terminate process.

	return EXCEPTION_EXECUTE_HANDLER;
	
}


/*
 * @implemented
 */
VOID
STDCALL
RaiseException (
	DWORD		dwExceptionCode,
	DWORD		dwExceptionFlags,
	DWORD		nNumberOfArguments,
	CONST DWORD	* lpArguments		OPTIONAL
	)
{
	EXCEPTION_RECORD ExceptionRecord;

	/* Do NOT normalize dwExceptionCode: it will be done in
	 * NTDLL.RtlRaiseException().
	 */
	ExceptionRecord.ExceptionCode = dwExceptionCode;
	ExceptionRecord.ExceptionRecord = NULL;
	ExceptionRecord.ExceptionAddress = (PVOID) RaiseException;
	/*
	 * Normalize dwExceptionFlags.
	 */
	ExceptionRecord.ExceptionFlags = (dwExceptionFlags & EXCEPTION_NONCONTINUABLE);
	/*
	 * Normalize nNumberOfArguments.
	 */
	if (EXCEPTION_MAXIMUM_PARAMETERS < nNumberOfArguments)
	{
		nNumberOfArguments = EXCEPTION_MAXIMUM_PARAMETERS;
	}
	/*
	 * If the exception has no argument,
	 * ignore nNumberOfArguments and lpArguments.
	 */
	if (NULL == lpArguments)
	{
		ExceptionRecord.NumberParameters = 0;
	}
	else
	{
		ExceptionRecord.NumberParameters = nNumberOfArguments;
		for (	nNumberOfArguments = 0;
			(nNumberOfArguments < ExceptionRecord.NumberParameters); 
			nNumberOfArguments ++
			)
		{
			ExceptionRecord.ExceptionInformation [nNumberOfArguments]
				= *lpArguments ++;
		}
	}
	RtlRaiseException (& ExceptionRecord);
}

/* EOF */
