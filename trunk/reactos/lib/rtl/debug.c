/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Run-Time Library
 * FILE:            ntoskrnl/rtl/dbgprint.c
 * PURPOSE:         Debug Print and Prompt routines
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Royce Mitchel III
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS ********************************************************/

#if 0
NTSTATUS
NTAPI
DebugPrint(IN PANSI_STRING DebugString,
           IN ULONG ComponentId,
           IN ULONG Level)
{
    /* Call the INT2D Service */
    return DebugService(BREAKPOINT_PRINT,
                        DebugString->Buffer,
                        DebugString->Length,
                        UlongToPtr(ComponentId),
                        UlongToPtr(Level)); 
}
#else
NTSTATUS
NTAPI
DebugPrint(IN PANSI_STRING DebugString,
           IN ULONG ComponentId,
           IN ULONG Level);
#endif

NTSTATUS
NTAPI
DebugPrompt(IN PANSI_STRING Output,
            IN PANSI_STRING Input)
{
    /* Call the INT2D Service */
    return DebugService(BREAKPOINT_PROMPT,
                        Output->Buffer,
                        Output->Length,
                        Input->Buffer,
                        UlongToPtr(Input->MaximumLength));
}

/* FUNCTIONS ****************************************************************/

ULONG
NTAPI
vDbgPrintExWithPrefixInternal(IN LPCSTR Prefix,
                              IN ULONG ComponentId,
                              IN ULONG Level,
                              IN LPCSTR Format,
                              IN va_list ap,
                              IN BOOLEAN HandleBreakpoint)
{
    NTSTATUS Status;
    ANSI_STRING DebugString;
    CHAR Buffer[512];
    PCHAR pBuffer = Buffer;
    ULONG pBufferSize = sizeof(Buffer);
    ULONG Length;
    EXCEPTION_RECORD ExceptionRecord;

    /* Check if we should print it or not */
    if (ComponentId != -1 && !NtQueryDebugFilterState(ComponentId, Level))
    {
        /* This message is masked */
        return STATUS_SUCCESS;
    }

    /* For user mode, don't recursively DbgPrint */
    if (RtlpSetInDbgPrint(TRUE)) return STATUS_SUCCESS;

    /* Initialize the length to 8 */
    DebugString.Length = 0;

    /* Handle the prefix */
    if (Prefix && *Prefix)
    {
        /* Get the length */
        DebugString.Length = strlen(Prefix);

        /* Normalize it */
        if(DebugString.Length > sizeof(Buffer))
        {
            DebugString.Length = sizeof(Buffer);
        }

        /* Copy it */
        strncpy(Buffer, Prefix, DebugString.Length);

        /* Set the pointer and update the size */
        pBuffer = &Buffer[DebugString.Length];
        pBufferSize -= DebugString.Length;
    }

    /* Setup the ANSI String */
    DebugString.Buffer = Buffer;
    DebugString.MaximumLength = sizeof(Buffer);
    Length = _vsnprintf(pBuffer, pBufferSize, Format, ap);

    /* Check if we went past the buffer */
    if (Length == -1)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 1] = '\n';

        /* Put maximum */
        Length = sizeof(Buffer);
    }

    /* Update length */
    DebugString.Length += Length;

    /* First, let the debugger know as well */
    if (RtlpCheckForActiveDebugger(FALSE))
    {
        /* Fill out an exception record */
        ExceptionRecord.ExceptionCode = DBG_PRINTEXCEPTION_C;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.NumberParameters = 2;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[0] = DebugString.Length + 1;
        ExceptionRecord.ExceptionInformation[1] = (ULONG_PTR)DebugString.Buffer;

        /* Raise the exception */
        RtlRaiseException(&ExceptionRecord);

        /* This code only runs in user-mode, so setting the flag is safe */
        NtCurrentTeb()->InDbgPrint = FALSE;
        return STATUS_SUCCESS;
    }

    /* Call the Debug Print routine */
    Status = DebugPrint(&DebugString, ComponentId, Level);

    /* Check if this was with Control-C */
    if (HandleBreakpoint)
    {
        /* Check if we got a breakpoint */
        if (Status == STATUS_BREAKPOINT)
        {
            /* Breakpoint */
            DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
            Status = STATUS_SUCCESS;
        }
    }

    /* In user-mode, remove the InDbgPrint Flag */
    RtlpSetInDbgPrint(FALSE);

    /* Return */
    return Status;
}

/*
 * @implemented
 */
ULONG
NTAPI
vDbgPrintExWithPrefix(IN LPCSTR Prefix,
                      IN ULONG ComponentId,
                      IN ULONG Level,
                      IN LPCSTR Format,
                      IN va_list ap)
{
    /* Call the internal routine that also handles ControlC */
    return vDbgPrintExWithPrefixInternal(Prefix,
                                         ComponentId,
                                         Level,
                                         Format,
                                         ap,
                                         TRUE);
}

/*
 * @implemented
 */
ULONG
NTAPI
vDbgPrintEx(IN ULONG ComponentId,
            IN ULONG Level,
            IN LPCSTR Format,
            IN va_list ap)
{
    /* Call the internal routine that also handles ControlC */
    return vDbgPrintExWithPrefixInternal(NULL,
                                         ComponentId,
                                         Level,
                                         Format,
                                         ap,
                                         TRUE);
}

/*
 * @implemented
 */
ULONG
__cdecl
DbgPrint(PCH Format,
         ...)
{
    va_list ap;

    /* Call the internal routine that also handles ControlC */
    va_start(ap, Format);
    return vDbgPrintExWithPrefixInternal(NULL,
                                         -1,
                                         DPFLTR_ERROR_LEVEL,
                                         Format,
                                         ap,
                                         TRUE);
    va_end(ap);
}

/*
 * @implemented
 */
ULONG
__cdecl
DbgPrintEx(IN ULONG ComponentId,
           IN ULONG Level,
           IN PCH Format,
           ...)
{
    va_list ap;

    /* Call the internal routine that also handles ControlC */
    va_start(ap, Format);
    return vDbgPrintExWithPrefixInternal(NULL,
                                         ComponentId,
                                         Level,
                                         Format,
                                         ap,
                                         TRUE);
    va_end(ap);
}

/*
 * @implemented
 */
ULONG
__cdecl
DbgPrintReturnControlC(PCH Format,
                       ...)
{
    va_list ap;

    /* Call the internal routine that also handles ControlC */
    return vDbgPrintExWithPrefixInternal(NULL,
                                         -1,
                                         DPFLTR_ERROR_LEVEL,
                                         Format,
                                         ap,
                                         FALSE);
}

/*
 * @implemented
 */
ULONG
STDCALL
DbgPrompt(PCH OutputString,
          PCH InputString,
          USHORT InputSize)
{
    ANSI_STRING Output;
    ANSI_STRING Input;

    /* Setup the input string */
    Input.MaximumLength = InputSize;
    Input.Buffer = InputString;

    /* Setup the output string */
    Output.Length = strlen (OutputString);
    Output.Buffer = OutputString;

    /* Call the system service */
    return DebugPrompt(&Output, &Input);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
DbgQueryDebugFilterState(IN ULONG ComponentId,
                         IN ULONG Level)
{
    /* Call the Nt routine */
    return NtQueryDebugFilterState(ComponentId, Level);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
DbgSetDebugFilterState(IN ULONG ComponentId,
                       IN ULONG Level,
                       IN BOOLEAN State)
{
    /* Call the Nt routine */
    return NtSetDebugFilterState(ComponentId, Level, State);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
DbgLoadImageSymbols(IN PUNICODE_STRING Name,
                    IN ULONG Base,
                    IN ULONG Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
/* EOF */
