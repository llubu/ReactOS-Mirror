/* $Id: exception.c,v 1.8 2002/01/13 22:02:30 ea Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode exception support
 * FILE:              lib/ntdll/rtl/exception.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>
#include <string.h>

/* FUNCTIONS ***************************************************************/

VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{
#if 0
	CONTEXT	Context;

	/* FIXME: fill in the CONTEXT */
	NtRaiseException (
		ExceptionRecord,
		& Context,
		TRUE
		);
	/* FIXME: ExceptionRecord */
	RtlRaiseException (ExceptionRecord);
#endif
}

ULONG
RtlDispatchException(PEXCEPTION_RECORD ExceptionRecord,
		     PCONTEXT Context)
{
  
}

VOID STDCALL
KiUserExceptionDispatcher(PEXCEPTION_RECORD ExceptionRecord,
			  PCONTEXT Context)
{
  EXCEPTION_RECORD NestedExceptionRecord;
  NTSTATUS Status;

  if (RtlDispatchException(ExceptionRecord, Context) == 1)
    {
      Status = ZwContinue(Context, FALSE);
    }
  else
    {
      Status = ZwRaiseException(ExceptionRecord, Context, FALSE);
    }

  NestedExceptionRecord.ExceptionCode = Status;
  NestedExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
  NestedExceptionRecord.ExceptionRecord = ExceptionRecord;

  RtlRaiseException(&NestedExceptionRecord);
}

VOID STDCALL
RtlRaiseStatus(NTSTATUS Status)
{
  EXCEPTION_RECORD ExceptionRecord;

  ExceptionRecord.ExceptionCode    = Status;
  ExceptionRecord.ExceptionRecord  = NULL;
  ExceptionRecord.NumberParameters = 0;
  ExceptionRecord.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
  RtlRaiseException (& ExceptionRecord);
}


VOID STDCALL
RtlUnwind(PEXCEPTION_REGISTRATION RegistrationFrame,
  PVOID ReturnAddress,
  PEXCEPTION_RECORD ExceptionRecord,
  DWORD EaxValue)
{

}

/* EOF */
