/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: prop.c,v 1.9 2003/08/07 04:03:24 royce Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
int STDCALL
EnumPropsA(HWND hWnd, PROPENUMPROCA lpEnumFunc)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int STDCALL
EnumPropsExA(HWND hWnd, PROPENUMPROCEXA lpEnumFunc, LPARAM lParam)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int STDCALL
EnumPropsExW(HWND hWnd, PROPENUMPROCEXW lpEnumFunc, LPARAM lParam)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int STDCALL
EnumPropsW(HWND hWnd, PROPENUMPROCW lpEnumFunc)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @implemented
 */
HANDLE STDCALL
GetPropA(HWND hWnd, LPCSTR lpString)
{
  PWSTR lpWString;
  UNICODE_STRING UString;
  HANDLE Ret;
  if (HIWORD(lpString))
    {
      RtlCreateUnicodeStringFromAsciiz(&UString, (LPSTR)lpString);
      lpWString = UString.Buffer;
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = GetPropW(hWnd, lpWString);
      RtlFreeUnicodeString(&UString);
    }
  else
    {
      Ret = GetPropW(hWnd, (LPWSTR)lpString);
    }  
  return(Ret);
}


/*
 * @implemented
 */
HANDLE STDCALL
GetPropW(HWND hWnd, LPCWSTR lpString)
{
  ATOM Atom;
  if (HIWORD(lpString))
    {
      Atom = GlobalFindAtomW(lpString);
    }
  else
    {
      Atom = LOWORD((DWORD)lpString);
    }
  return(NtUserGetProp(hWnd, Atom));
}


/*
 * @implemented
 */
HANDLE STDCALL
RemovePropA(HWND hWnd, LPCSTR lpString)
{
  PWSTR lpWString;
  UNICODE_STRING UString;
  HANDLE Ret;

  if (HIWORD(lpString))
    {
      RtlCreateUnicodeStringFromAsciiz(&UString, (LPSTR)lpString);
      lpWString = UString.Buffer;
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = RemovePropW(hWnd, lpWString);
      RtlFreeUnicodeString(&UString);
    }
  else
    {
      Ret = RemovePropW(hWnd, (LPCWSTR)lpString);
    }
  return(Ret);
}


/*
 * @implemented
 */
HANDLE STDCALL
RemovePropW(HWND hWnd,
	    LPCWSTR lpString)
{
  ATOM Atom;
  if (HIWORD(lpString))
    {
      Atom = GlobalFindAtomW(lpString);
    }
  else
    {
      Atom = LOWORD((DWORD)lpString);
    }
  return(NtUserRemoveProp(hWnd, Atom));
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetPropA(HWND hWnd, LPCSTR lpString, HANDLE hData)
{
  PWSTR lpWString;
  UNICODE_STRING UString;
  BOOL Ret;
  
  if (HIWORD(lpString))
    {
      RtlCreateUnicodeStringFromAsciiz(&UString, (LPSTR)lpString);
      lpWString = UString.Buffer;
      if (lpWString == NULL)
	{
	  return(FALSE);
	}
      Ret = SetPropW(hWnd, lpWString, hData);
      RtlFreeUnicodeString(&UString);
    }
  else
    {
      Ret = SetPropW(hWnd, (LPWSTR)lpString, hData);
    }
  return(Ret);
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetPropW(HWND hWnd, LPCWSTR lpString, HANDLE hData)
{
  ATOM Atom;
  if (HIWORD(lpString))
    {
      Atom = GlobalFindAtomW(lpString);
    }
  else
    {
      Atom = LOWORD((DWORD)lpString);
    }
  
  return(NtUserSetProp(hWnd, Atom, hData));
}
