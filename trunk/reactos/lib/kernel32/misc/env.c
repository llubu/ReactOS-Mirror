/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/env.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
DWORD
STDCALL
GetEnvironmentVariableA (
	LPCSTR	lpName,
	LPSTR	lpBuffer,
	DWORD	nSize
	)
{
	ANSI_STRING VarName;
	ANSI_STRING VarValue;
	UNICODE_STRING VarNameU;
	UNICODE_STRING VarValueU;
	NTSTATUS Status;

	/* initialize unicode variable name string */
	RtlInitAnsiString (&VarName,
	                   (LPSTR)lpName);
	RtlAnsiStringToUnicodeString (&VarNameU,
	                              &VarName,
	                              TRUE);

	/* initialize ansi variable value string */
	VarValue.Length = 0;
	VarValue.MaximumLength = nSize;
	VarValue.Buffer = lpBuffer;

	/* initialize unicode variable value string and allocate buffer */
	VarValueU.Length = 0;
	VarValueU.MaximumLength = nSize * sizeof(WCHAR);
	VarValueU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                    0,
	                                    VarValueU.MaximumLength);

	/* get unicode environment variable */
	Status = RtlQueryEnvironmentVariable_U (NULL,
	                                        &VarNameU,
	                                        &VarValueU);
	if (!NT_SUCCESS(Status))
	{
		/* free unicode buffer */
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             VarValueU.Buffer);

		/* free unicode variable name string */
		RtlFreeUnicodeString (&VarNameU);

		SetLastErrorByStatus (Status);
		if (Status == STATUS_BUFFER_TOO_SMALL)
		{
			return VarValueU.Length / sizeof(WCHAR) + 1;
		}
		else
		{
			return 0;
		}
	}

	/* convert unicode value string to ansi */
	RtlUnicodeStringToAnsiString (&VarValue,
	                              &VarValueU,
	                              FALSE);

	/* free unicode buffer */
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             VarValueU.Buffer);

	/* free unicode variable name string */
	RtlFreeUnicodeString (&VarNameU);

	return (VarValueU.Length / sizeof(WCHAR) + 1);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetEnvironmentVariableW (
	LPCWSTR	lpName,
	LPWSTR	lpBuffer,
	DWORD	nSize
	)
{
	UNICODE_STRING VarName;
	UNICODE_STRING VarValue;
	NTSTATUS Status;

	RtlInitUnicodeString (&VarName,
	                      lpName);

	VarValue.Length = 0;
	VarValue.MaximumLength = nSize * sizeof(WCHAR);
	VarValue.Buffer = lpBuffer;

	Status = RtlQueryEnvironmentVariable_U (NULL,
	                                        &VarName,
	                                        &VarValue);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		if (Status == STATUS_BUFFER_TOO_SMALL)
		{
			return (VarValue.Length / sizeof(WCHAR)) +  1;
		}
		else
		{
			return 0;
		}
	}

	return (VarValue.Length / sizeof(WCHAR) + 1);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetEnvironmentVariableA (
	LPCSTR	lpName,
	LPCSTR	lpValue
	)
{
	ANSI_STRING VarName;
	ANSI_STRING VarValue;
	UNICODE_STRING VarNameU;
	UNICODE_STRING VarValueU;
	NTSTATUS Status;

	DPRINT("SetEnvironmentVariableA(Name '%s', Value '%s')\n", lpName, lpValue);

	RtlInitAnsiString (&VarName,
	                   (LPSTR)lpName);
	RtlAnsiStringToUnicodeString (&VarNameU,
	                              &VarName,
	                              TRUE);

	RtlInitAnsiString (&VarValue,
	                   (LPSTR)lpValue);
	RtlAnsiStringToUnicodeString (&VarValueU,
	                              &VarValue,
	                              TRUE);

	Status = RtlSetEnvironmentVariable (NULL,
	                                    &VarNameU,
	                                    &VarValueU);

	RtlFreeUnicodeString (&VarNameU);
	RtlFreeUnicodeString (&VarValueU);

	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetEnvironmentVariableW (
	LPCWSTR	lpName,
	LPCWSTR	lpValue
	)
{
	UNICODE_STRING VarName;
	UNICODE_STRING VarValue;
	NTSTATUS Status;

	DPRINT("SetEnvironmentVariableW(Name '%S', Value '%S')\n", lpName, lpValue);

	RtlInitUnicodeString (&VarName,
	                      lpName);

	RtlInitUnicodeString (&VarValue,
	                      lpValue);

	Status = RtlSetEnvironmentVariable (NULL,
	                                    &VarName,
	                                    &VarValue);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
LPSTR
STDCALL
GetEnvironmentStringsA (
	VOID
	)
{
	UNICODE_STRING UnicodeString;
	ANSI_STRING AnsiString;
	PWCHAR EnvU;
	PWCHAR PtrU;
	ULONG  Length;
	PCHAR EnvPtr = NULL;

	EnvU = (PWCHAR)(NtCurrentPeb ()->ProcessParameters->Environment);

	if (EnvU == NULL)
		return NULL;

	if (*EnvU == 0)
		return NULL;

	/* get environment size */
	PtrU = EnvU;
	while (*PtrU)
	{
		while (*PtrU)
			PtrU++;
		PtrU++;
	}
	Length = (ULONG)(PtrU - EnvU);
	DPRINT("Length %lu\n", Length);

	/* allocate environment buffer */
	EnvPtr = RtlAllocateHeap (RtlGetProcessHeap (),
	                          0,
	                          Length + 1);
	DPRINT("EnvPtr %p\n", EnvPtr);

	/* convert unicode environment to ansi */
	UnicodeString.MaximumLength = Length * sizeof(WCHAR) + sizeof(WCHAR);
	UnicodeString.Buffer = EnvU;

	AnsiString.MaximumLength = Length + 1;
	AnsiString.Length = 0;
	AnsiString.Buffer = EnvPtr;

	DPRINT ("UnicodeString.Buffer \'%S\'\n", UnicodeString.Buffer);

	while (*(UnicodeString.Buffer))
	{
		UnicodeString.Length = wcslen (UnicodeString.Buffer) * sizeof(WCHAR);
		UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);
		if (UnicodeString.Length > 0)
		{
			AnsiString.Length = 0;
			AnsiString.MaximumLength = Length + 1 - (AnsiString.Buffer - EnvPtr);

			RtlUnicodeStringToAnsiString (&AnsiString,
			                              &UnicodeString,
			                              FALSE);

			AnsiString.Buffer += (AnsiString.Length + 1);
			UnicodeString.Buffer += ((UnicodeString.Length / sizeof(WCHAR)) + 1);
		}
	}
	*(AnsiString.Buffer) = 0;

	return EnvPtr;
}


/*
 * @implemented
 */
LPWSTR
STDCALL
GetEnvironmentStringsW (
	VOID
	)
{
	return (LPWSTR)(NtCurrentPeb ()->ProcessParameters->Environment);
}


/*
 * @implemented
 */
BOOL
STDCALL
FreeEnvironmentStringsA (
	LPSTR	EnvironmentStrings
	)
{
	if (EnvironmentStrings == NULL)
		return FALSE;

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             EnvironmentStrings);

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
FreeEnvironmentStringsW (
	LPWSTR	EnvironmentStrings
	)
{
 (void)EnvironmentStrings;
 return TRUE;
}


/*
 * @implemented
 */
DWORD
STDCALL
ExpandEnvironmentStringsA (
	LPCSTR	lpSrc,
	LPSTR	lpDst,
	DWORD	nSize
	)
{
	ANSI_STRING Source;
	ANSI_STRING Destination;
	UNICODE_STRING SourceU;
	UNICODE_STRING DestinationU;
	NTSTATUS Status;
	ULONG Length = 0;

	RtlInitAnsiString (&Source,
	                   (LPSTR)lpSrc);
	RtlAnsiStringToUnicodeString (&SourceU,
	                              &Source,
	                              TRUE);

	Destination.Length = 0;
	Destination.MaximumLength = nSize;
	Destination.Buffer = lpDst;

	DestinationU.Length = 0;
	DestinationU.MaximumLength = nSize * sizeof(WCHAR);
	DestinationU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                       0,
	                                       DestinationU.MaximumLength);

	Status = RtlExpandEnvironmentStrings_U (NULL,
	                                        &SourceU,
	                                        &DestinationU,
	                                        &Length);

	RtlFreeUnicodeString (&SourceU);

	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		if (Status != STATUS_BUFFER_TOO_SMALL)
		{
			RtlFreeHeap (RtlGetProcessHeap (),
			             0,
			             DestinationU.Buffer);
			return 0;
		}
	}

	RtlUnicodeStringToAnsiString (&Destination,
	                              &DestinationU,
	                              FALSE);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             DestinationU.Buffer);

	return (Length / sizeof(WCHAR));
}


/*
 * @implemented
 */
DWORD
STDCALL
ExpandEnvironmentStringsW (
	LPCWSTR	lpSrc,
	LPWSTR	lpDst,
	DWORD	nSize
	)
{
	UNICODE_STRING Source;
	UNICODE_STRING Destination;
	NTSTATUS Status;
	ULONG Length = 0;

	RtlInitUnicodeString (&Source,
	                      (LPWSTR)lpSrc);

	Destination.Length = 0;
	Destination.MaximumLength = nSize * sizeof(WCHAR);
	Destination.Buffer = lpDst;

	Status = RtlExpandEnvironmentStrings_U (NULL,
	                                        &Source,
	                                        &Destination,
	                                        &Length);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		if (Status != STATUS_BUFFER_TOO_SMALL)
			return 0;
	}

	return (Length / sizeof(WCHAR));
}

/* EOF */
