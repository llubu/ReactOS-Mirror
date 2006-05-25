/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/mem/isbad.c
 * PURPOSE:         
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Ge van Geldorp
 *                  Filip Navara
 * UPDATE HISTORY:
 *                  Created 03/10/99
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FIXME: Stubs. What is it for? */
/*
 * @implemented
 */
UINT
wcsnlen (
	LPCWSTR	lpsz,
	UINT	ucchMax
	)
{
  UINT i = 0;
  while( i < ucchMax && lpsz[i] ) i++;
  return i;
}


/* FIXME: Stubs. What is it for? */
/*
 * @implemented
 */
UINT
strnlen (
	LPCSTR	lpsz,
	UINT	uiMax
	)
{
  UINT i = 0;
  while( i < uiMax && lpsz[i] ) i++;
  return i;
}

/* --- --- --- */

/*
 * @implemented
 */
BOOL
STDCALL
IsBadReadPtr (
	CONST VOID	* lp,
	UINT		ucb
	)
{
	MEMORY_BASIC_INFORMATION MemoryInformation;

	if ( ucb == 0 )
	{
		return TRUE;
	}

	VirtualQuery (
		lp,
		& MemoryInformation,
		sizeof (MEMORY_BASIC_INFORMATION)
		);

	if ( MemoryInformation.State != MEM_COMMIT )
	{
		return TRUE;
	}

	if ( MemoryInformation.RegionSize < ucb )
	{
		return TRUE;
	}

	if ( MemoryInformation.Protect == PAGE_EXECUTE )
	{
		return TRUE;
	}

	if ( MemoryInformation.Protect == PAGE_NOACCESS )
	{
		return TRUE;
	}

	return FALSE;

}


/*
 * @implemented
 */
BOOL
STDCALL
IsBadHugeReadPtr (
	CONST VOID	* lp,
	UINT		ucb
	)
{
	return IsBadReadPtr (lp, ucb);
}


/*
 * @implemented
 */
BOOL
STDCALL
IsBadCodePtr (
	FARPROC	lpfn
	)
{
	MEMORY_BASIC_INFORMATION MemoryInformation;


	VirtualQuery (
		lpfn,
		& MemoryInformation,
		sizeof (MEMORY_BASIC_INFORMATION)
		);

	if ( MemoryInformation.State != MEM_COMMIT )
	{
		return TRUE;
	}

	if (	(MemoryInformation.Protect == PAGE_EXECUTE)
		|| (MemoryInformation.Protect == PAGE_EXECUTE_READ)
		)
	{
		return FALSE;
	}

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
IsBadWritePtr (
	LPVOID	lp,
	UINT	ucb
	)
{
	MEMORY_BASIC_INFORMATION MemoryInformation;

	if ( ucb == 0 )
	{
		return TRUE;
	}

	VirtualQuery (
		lp,
		& MemoryInformation,
		sizeof (MEMORY_BASIC_INFORMATION)
		);

	if ( MemoryInformation.State != MEM_COMMIT )
	{
		return TRUE;
	}

	if ( MemoryInformation.RegionSize < ucb )
	{
		return TRUE;
	}


	if ( MemoryInformation.Protect == PAGE_READONLY)
	{
		return TRUE;
	}

	if (	(MemoryInformation.Protect == PAGE_EXECUTE)
		|| (MemoryInformation.Protect == PAGE_EXECUTE_READ)
		)
	{
		return TRUE;
	}

	if ( MemoryInformation.Protect == PAGE_NOACCESS )
	{
		return TRUE;
	}

	return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
IsBadHugeWritePtr (
	LPVOID	lp,
	UINT	ucb
	)
{
	return IsBadWritePtr (lp, ucb);
}


/*
 * @implemented
 */
BOOL
STDCALL
IsBadStringPtrW (
	LPCWSTR	lpsz,
	UINT	ucchMax
	)
{
	UINT Len = wcsnlen (
			lpsz + 1,
			ucchMax >> 1
			);
	return IsBadReadPtr (
			lpsz,
			Len << 1
			);
}


/*
 * @implemented
 */
BOOL
STDCALL
IsBadStringPtrA (
	LPCSTR	lpsz,
	UINT	ucchMax
	)
{
	UINT Len = strnlen (
			lpsz + 1,
			ucchMax
			);
	return IsBadReadPtr (
			lpsz,
			Len
			);
}


/* EOF */
