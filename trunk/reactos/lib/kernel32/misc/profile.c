/* $Id: profile.c,v 1.4 2002/09/07 15:12:27 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/profile.c
 * PURPOSE:         Profiles functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <wchar.h>
#include <string.h>


/* FUNCTIONS *****************************************************************/

BOOL STDCALL
CloseProfileUserMapping(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


UINT STDCALL
GetPrivateProfileIntW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpKeyName,
	INT	nDefault,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT STDCALL
GetPrivateProfileIntA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	INT	nDefault,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD STDCALL
GetPrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPWSTR	lpReturnedString,
	DWORD	nSize,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD STDCALL
GetPrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPSTR	lpReturnedString,
	DWORD	nSize,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD STDCALL
GetPrivateProfileSectionNamesW (
  LPWSTR lpszReturnBuffer,
  DWORD nSize,
  LPCWSTR lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD STDCALL
GetPrivateProfileSectionNamesA (
  LPSTR lpszReturnBuffer,
  DWORD nSize,
  LPCSTR lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD STDCALL
GetPrivateProfileStringW (
	LPCWSTR lpAppName,
	LPCWSTR lpKeyName,
	LPCWSTR lpDefault,
	LPWSTR	lpReturnedString,
	DWORD	nSize,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD STDCALL
GetPrivateProfileStringA (
	LPCSTR	lpAppName,
	LPCSTR	lpKeyName,
	LPCSTR	lpDefault,
	LPSTR	lpReturnedString,
	DWORD	nSize,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

BOOL STDCALL
GetPrivateProfileStructW (
  LPCWSTR lpszSection,
  LPCWSTR lpszKey,
  LPVOID lpStruct,
  UINT uSizeStruct,
  LPCWSTR szFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL STDCALL
GetPrivateProfileStructA (
  LPCSTR lpszSection,
  LPCSTR lpszKey,
  LPVOID lpStruct,
  UINT uSizeStruct,
  LPCSTR szFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

UINT STDCALL
GetProfileIntW(LPCWSTR lpAppName,
	       LPCWSTR lpKeyName,
	       INT nDefault)
{
   return GetPrivateProfileIntW(lpAppName,
				lpKeyName,
				nDefault,
				NULL);
}


UINT STDCALL
GetProfileIntA(LPCSTR lpAppName,
	       LPCSTR lpKeyName,
	       INT nDefault)
{
   return GetPrivateProfileIntA(lpAppName,
				lpKeyName,
				nDefault,
				NULL);
}


DWORD STDCALL
GetProfileSectionW(LPCWSTR lpAppName,
		   LPWSTR lpReturnedString,
		   DWORD nSize)
{
   return GetPrivateProfileSectionW(lpAppName,
				    lpReturnedString,
				    nSize,
				    NULL);
}


DWORD STDCALL
GetProfileSectionA(LPCSTR lpAppName,
		   LPSTR lpReturnedString,
		   DWORD nSize)
{
   return GetPrivateProfileSectionA(lpAppName,
				    lpReturnedString,
				    nSize,
				    NULL);
}


DWORD STDCALL
GetProfileStringW(LPCWSTR lpAppName,
		  LPCWSTR lpKeyName,
		  LPCWSTR lpDefault,
		  LPWSTR lpReturnedString,
		  DWORD nSize)
{
   return GetPrivateProfileStringW(lpAppName,
				   lpKeyName,
				   lpDefault,
				   lpReturnedString,
				   nSize,
				   NULL);
}


DWORD STDCALL
GetProfileStringA(LPCSTR lpAppName,
		  LPCSTR lpKeyName,
		  LPCSTR lpDefault,
		  LPSTR lpReturnedString,
		  DWORD nSize)
{
   return GetPrivateProfileStringA(lpAppName,
				   lpKeyName,
				   lpDefault,
				   lpReturnedString,
				   nSize,
				   NULL);
}


BOOL STDCALL
OpenProfileUserMapping (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL STDCALL
QueryWin31IniFilesMappedToRegistry (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
WritePrivateProfileSectionA (
	LPCSTR	lpAppName,
	LPCSTR	lpString,
	LPCSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
WritePrivateProfileSectionW (
	LPCWSTR	lpAppName,
	LPCWSTR	lpString,
	LPCWSTR	lpFileName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
WritePrivateProfileStringA(LPCSTR lpAppName,
			   LPCSTR lpKeyName,
			   LPCSTR lpString,
			   LPCSTR lpFileName)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
WritePrivateProfileStringW(LPCWSTR lpAppName,
			   LPCWSTR lpKeyName,
			   LPCWSTR lpString,
			   LPCWSTR lpFileName)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL STDCALL
WritePrivateProfileStructA(
  LPCSTR lpszSection,
  LPCSTR lpszKey,
  LPVOID lpStruct,
  UINT uSizeStruct,
  LPCSTR szFile
  )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL STDCALL
WritePrivateProfileStructW(
  LPCWSTR lpszSection,
  LPCWSTR lpszKey,
  LPVOID lpStruct,
  UINT uSizeStruct,
  LPCWSTR szFile
  )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL STDCALL
WriteProfileSectionA(LPCSTR lpAppName,
		     LPCSTR lpString)
{
   return WritePrivateProfileSectionA(lpAppName,
				      lpString,
				      NULL);
}


WINBOOL STDCALL
WriteProfileSectionW(LPCWSTR lpAppName,
		     LPCWSTR lpString)
{
   return WritePrivateProfileSectionW(lpAppName,
				      lpString,
				      NULL);
}


WINBOOL STDCALL
WriteProfileStringA(LPCSTR lpAppName,
		    LPCSTR lpKeyName,
		    LPCSTR lpString)
{
   return WritePrivateProfileStringA(lpAppName,
				     lpKeyName,
				     lpString,
				     NULL);
}


WINBOOL STDCALL
WriteProfileStringW(LPCWSTR lpAppName,
		    LPCWSTR lpKeyName,
		    LPCWSTR lpString)
{
   return WritePrivateProfileStringW(lpAppName,
				     lpKeyName,
				     lpString,
				     NULL);
}

/* EOF */
