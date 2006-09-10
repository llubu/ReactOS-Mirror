/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/rtl.c
 * PURPOSE:         Runtime Library
 */

#define RTL_H

#define NTOS_MODE_USER
#define WIN32_NO_STATUS
#include <ntddk.h>
#include <bitmap.c>

/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID NTAPI
RtlInitAnsiString(
	IN OUT PANSI_STRING DestinationString,
	IN PCSTR SourceString)
{
	SIZE_T DestSize;

	if(SourceString)
	{
		DestSize = strlen(SourceString);
		DestinationString->Length = (USHORT)DestSize;
		DestinationString->MaximumLength = (USHORT)DestSize + sizeof(CHAR);
	}
	else
	{
		DestinationString->Length = 0;
		DestinationString->MaximumLength = 0;
	}

	DestinationString->Buffer = (PCHAR)SourceString;
}

/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID NTAPI
RtlInitUnicodeString(
	IN OUT PUNICODE_STRING DestinationString,
	IN PCWSTR SourceString)
{
	SIZE_T DestSize;

	if(SourceString)
	{
		DestSize = wcslen(SourceString) * sizeof(WCHAR);
		DestinationString->Length = (USHORT)DestSize;
		DestinationString->MaximumLength = (USHORT)DestSize + sizeof(WCHAR);
	}
	else
	{
		DestinationString->Length = 0;
		DestinationString->MaximumLength = 0;
	}

	DestinationString->Buffer = (PWCHAR)SourceString;
}

NTSTATUS NTAPI
RtlAnsiStringToUnicodeString(
	IN OUT PUNICODE_STRING UniDest,
	IN PANSI_STRING AnsiSource,
	IN BOOLEAN AllocateDestinationString)
{
	ULONG Length;
	PUCHAR WideString;
	USHORT i;

	Length = AnsiSource->Length * sizeof(WCHAR);
	if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;
	UniDest->Length = (USHORT)Length;

	if (AllocateDestinationString)
	{
		UniDest->MaximumLength = (USHORT)Length + sizeof(WCHAR);
		UniDest->Buffer = malloc(UniDest->MaximumLength);
		if (!UniDest->Buffer)
			return STATUS_NO_MEMORY;
	}
	else if (UniDest->Length >= UniDest->MaximumLength)
	{
		return STATUS_BUFFER_OVERFLOW;
	}

	WideString = (PUCHAR)UniDest->Buffer;
	for (i = 0; i <= AnsiSource->Length; i++)
	{
		WideString[2 * i + 0] = AnsiSource->Buffer[i];
		WideString[2 * i + 1] = 0;
	}
	return STATUS_SUCCESS;
}

WCHAR NTAPI
RtlUpcaseUnicodeChar(
	IN WCHAR Source)
{
	USHORT Offset;

	if (Source < 'a')
		return Source;

	if (Source <= 'z')
		return (Source - ('a' - 'A'));

	Offset = 0;

	return Source + (SHORT)Offset;
}

VOID NTAPI
KeQuerySystemTime(
	OUT PLARGE_INTEGER CurrentTime)
{
	DPRINT1("KeQuerySystemTime() unimplemented\n");
}

PVOID NTAPI
ExAllocatePool(
	IN POOL_TYPE PoolType,
	IN SIZE_T NumberOfBytes)
{
	return malloc(NumberOfBytes);
}

VOID NTAPI
ExFreePool(
	IN PVOID p)
{
	free(p);
}

