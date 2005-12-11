/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/atom.c
 * PURPOSE:         Atom functions
 * PROGRAMMER:      Eric Kohl ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  Full rewrite 27/05/2001
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* GLOBALS *******************************************************************/

static PRTL_ATOM_TABLE LocalAtomTable = NULL;

static PRTL_ATOM_TABLE GetLocalAtomTable(VOID);


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
ATOM STDCALL
GlobalAddAtomA(LPCSTR lpString)
{
   UNICODE_STRING AtomName;
   NTSTATUS Status;
   ATOM Atom;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   if (lstrlenA(lpString) > 255)
   {
      /* This limit does not exist with NtAddAtom so the limit is probably
       * added for compability. -Gunnar
       */
      SetLastError(ERROR_INVALID_PARAMETER);
      return (ATOM)0;
   }

   RtlCreateUnicodeStringFromAsciiz(&AtomName,
				    (LPSTR)lpString);

   Status = NtAddAtom(AtomName.Buffer,
                      AtomName.Length,
		      &Atom);
   RtlFreeUnicodeString(&AtomName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
GlobalAddAtomW(LPCWSTR lpString)
{
   ATOM Atom;
   NTSTATUS Status;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   if (lstrlenW(lpString) > 255)
   {
      /* This limit does not exist with NtAddAtom so the limit is probably
       * added for compability. -Gunnar
       */
      SetLastError(ERROR_INVALID_PARAMETER);
      return (ATOM)0;
   }

   Status = NtAddAtom((LPWSTR)lpString,
                      wcslen(lpString),
		      &Atom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
GlobalDeleteAtom(ATOM nAtom)
{
   NTSTATUS Status;

   if (nAtom < 0xC000)
     {
	return 0;
     }

   Status = NtDeleteAtom(nAtom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return nAtom;
     }

   return 0;
}


/*
 * @implemented
 */
ATOM STDCALL
GlobalFindAtomA(LPCSTR lpString)
{
   UNICODE_STRING AtomName;
   NTSTATUS Status;
   ATOM Atom;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   if (lstrlenA(lpString) > 255)
   {
      /* This limit does not exist with NtAddAtom so the limit is probably
       * added for compability. -Gunnar
       */
      SetLastError(ERROR_INVALID_PARAMETER);
      return (ATOM)0;
   }

   RtlCreateUnicodeStringFromAsciiz(&AtomName,
				    (LPSTR)lpString);
   Status = NtFindAtom(AtomName.Buffer,
                       AtomName.Length,
		       &Atom);
   RtlFreeUnicodeString(&AtomName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
GlobalFindAtomW(LPCWSTR lpString)
{
   ATOM Atom;
   NTSTATUS Status;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   if (lstrlenW(lpString) > 255)
   {
      /* This limit does not exist with NtAddAtom so the limit is probably
       * added for compability. -Gunnar
       */
      SetLastError(ERROR_INVALID_PARAMETER);
      return (ATOM)0;
   }

   Status = NtFindAtom((LPWSTR)lpString,
                       wcslen(lpString),
		       &Atom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


UINT STDCALL
GlobalGetAtomNameA(ATOM nAtom,
		   LPSTR lpBuffer,
		   int nSize)
{
   PATOM_BASIC_INFORMATION Buffer;
   UNICODE_STRING AtomNameU;
   ANSI_STRING AtomName;
   ULONG BufferSize;
   ULONG ReturnLength;
   NTSTATUS Status;

   BufferSize = sizeof(ATOM_BASIC_INFORMATION) + nSize * sizeof(WCHAR);
   Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    BufferSize);
   if (Buffer == NULL)
   {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return 0;
   }

   Status = NtQueryInformationAtom(nAtom,
				   AtomBasicInformation,
				   Buffer,
				   BufferSize,
				   &ReturnLength);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    Buffer);
        SetLastErrorByStatus(Status);
	return 0;
     }

   RtlInitUnicodeString(&AtomNameU,
			Buffer->Name);
   AtomName.Buffer = lpBuffer;
   AtomName.Length = 0;
   AtomName.MaximumLength = nSize;
   RtlUnicodeStringToAnsiString(&AtomName,
				&AtomNameU,
				FALSE);

   ReturnLength = AtomName.Length;
   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       Buffer);

   return ReturnLength;
}


/*
 * @implemented
 */
UINT STDCALL
GlobalGetAtomNameW(ATOM nAtom,
		   LPWSTR lpBuffer,
		   int nSize)
{
   PATOM_BASIC_INFORMATION Buffer;
   ULONG BufferSize;
   ULONG ReturnLength;
   NTSTATUS Status;

   BufferSize = sizeof(ATOM_BASIC_INFORMATION) + nSize * sizeof(WCHAR);
   Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    BufferSize);
   if (Buffer == NULL)
   {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return 0;
   }

   Status = NtQueryInformationAtom(nAtom,
				   AtomBasicInformation,
				   Buffer,
				   BufferSize,
				   &ReturnLength);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    Buffer);
        SetLastErrorByStatus(Status);
	return 0;
     }

   memcpy(lpBuffer, Buffer->Name, Buffer->NameLength);
   ReturnLength = Buffer->NameLength / sizeof(WCHAR);
   *(lpBuffer + ReturnLength) = 0;
   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       Buffer);

   return ReturnLength;
}


static PRTL_ATOM_TABLE
GetLocalAtomTable(VOID)
{
   if (LocalAtomTable != NULL)
     {
	return LocalAtomTable;
     }
   RtlCreateAtomTable(37,
		      &LocalAtomTable);
   return LocalAtomTable;
}


/*
 * @implemented
 */
BOOL STDCALL
InitAtomTable(DWORD nSize)
{
   NTSTATUS Status;

   /* nSize should be a prime number */

   if ( nSize < 4 || nSize >= 512 )
     {
	nSize = 37;
     }

   if (LocalAtomTable == NULL)
    {
	Status = RtlCreateAtomTable(nSize,
				    &LocalAtomTable);
	if (!NT_SUCCESS(Status))
	  {
	     SetLastErrorByStatus(Status);
	     return FALSE;
	  }
    }

  return TRUE;
}


/*
 * @implemented
 */
ATOM STDCALL
AddAtomA(LPCSTR lpString)
{
   PRTL_ATOM_TABLE AtomTable;
   UNICODE_STRING AtomName;
   NTSTATUS Status;
   ATOM Atom;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   AtomTable = GetLocalAtomTable();

   RtlCreateUnicodeStringFromAsciiz(&AtomName,
				    (LPSTR)lpString);

   Status = RtlAddAtomToAtomTable(AtomTable,
				  AtomName.Buffer,
				  &Atom);
   RtlFreeUnicodeString(&AtomName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
AddAtomW(LPCWSTR lpString)
{
   PRTL_ATOM_TABLE AtomTable;
   ATOM Atom;
   NTSTATUS Status;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   AtomTable = GetLocalAtomTable();

   Status = RtlAddAtomToAtomTable(AtomTable,
				  (LPWSTR)lpString,
				  &Atom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
DeleteAtom(ATOM nAtom)
{
   PRTL_ATOM_TABLE AtomTable;
   NTSTATUS Status;

   if (nAtom < 0xC000)
     {
	return 0;
     }

   AtomTable = GetLocalAtomTable();

   Status = RtlDeleteAtomFromAtomTable(AtomTable,
				       nAtom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return nAtom;
     }

   return 0;
}


/*
 * @implemented
 */
ATOM STDCALL
FindAtomA(LPCSTR lpString)
{
   PRTL_ATOM_TABLE AtomTable;
   UNICODE_STRING AtomName;
   NTSTATUS Status;
   ATOM Atom;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   AtomTable = GetLocalAtomTable();
   RtlCreateUnicodeStringFromAsciiz(&AtomName,
				    (LPSTR)lpString);
   Status = RtlLookupAtomInAtomTable(AtomTable,
				     AtomName.Buffer,
				     &Atom);
   RtlFreeUnicodeString(&AtomName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
FindAtomW(LPCWSTR lpString)
{
   PRTL_ATOM_TABLE AtomTable;
   ATOM Atom;
   NTSTATUS Status;

   if (HIWORD((ULONG)lpString) == 0)
     {
	if ((ULONG)lpString >= 0xC000)
	  {
	     SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	     return (ATOM)0;
	  }
	return (ATOM)LOWORD((ULONG)lpString);
     }

   AtomTable = GetLocalAtomTable();

   Status = RtlLookupAtomInAtomTable(AtomTable,
				     (LPWSTR)lpString,
				     &Atom);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return (ATOM)0;
     }

   return Atom;
}


/*
 * @implemented
 */
UINT STDCALL
GetAtomNameA(ATOM nAtom,
	     LPSTR lpBuffer,
	     int nSize)
{
   PRTL_ATOM_TABLE AtomTable;
   PWCHAR Buffer;
   UNICODE_STRING AtomNameU;
   ANSI_STRING AtomName;
   ULONG NameLength;
   NTSTATUS Status;

   AtomTable = GetLocalAtomTable();

   NameLength = nSize * sizeof(WCHAR);
   Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    NameLength);
   if (Buffer == NULL)
   {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return 0;
   }

   Status = RtlQueryAtomInAtomTable(AtomTable,
				    nAtom,
				    NULL,
				    NULL,
				    Buffer,
				    &NameLength);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    Buffer);
        SetLastErrorByStatus(Status);
	return 0;
     }

   RtlInitUnicodeString(&AtomNameU,
			Buffer);
   AtomName.Buffer = lpBuffer;
   AtomName.Length = 0;
   AtomName.MaximumLength = nSize;
   RtlUnicodeStringToAnsiString(&AtomName,
				&AtomNameU,
				FALSE);

   NameLength = AtomName.Length;
   RtlFreeHeap(RtlGetProcessHeap(),
	       0,
	       Buffer);

   return NameLength;
}


/*
 * @implemented
 */
UINT STDCALL
GetAtomNameW(ATOM nAtom,
	     LPWSTR lpBuffer,
	     int nSize)
{
   PRTL_ATOM_TABLE AtomTable;
   ULONG NameLength;
   NTSTATUS Status;

   AtomTable = GetLocalAtomTable();

   NameLength = nSize * sizeof(WCHAR);
   Status = RtlQueryAtomInAtomTable(AtomTable,
				    nAtom,
				    NULL,
				    NULL,
				    lpBuffer,
				    &NameLength);
   if (!NT_SUCCESS(Status))
     {
	return 0;
     }

   return(NameLength / sizeof(WCHAR));
}

/* EOF */
