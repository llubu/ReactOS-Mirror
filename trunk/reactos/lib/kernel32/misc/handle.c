/* $Id: handle.c,v 1.11 2003/03/09 21:37:57 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/handle.c
 * PURPOSE:         Object  functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* GLOBALS *******************************************************************/

WINBOOL STDCALL
GetProcessId (HANDLE hProcess, LPDWORD lpProcessId);

HANDLE STDCALL
DuplicateConsoleHandle (HANDLE	hConsole,
			DWORD   dwDesiredAccess,
			BOOL	bInheritHandle,
			DWORD	dwOptions);

/* FUNCTIONS *****************************************************************/

WINBOOL WINAPI GetHandleInformation(HANDLE hObject, LPDWORD lpdwFlags)
{
   OBJECT_DATA_INFORMATION HandleInfo;
   ULONG BytesWritten;
   NTSTATUS errCode;
   
   errCode = NtQueryObject(hObject,
			   ObjectDataInformation, 
			   &HandleInfo, 
			   sizeof(OBJECT_DATA_INFORMATION),
			   &BytesWritten);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   if ( HandleInfo.bInheritHandle )
     *lpdwFlags &= HANDLE_FLAG_INHERIT;
   if ( HandleInfo.bProtectFromClose )
     *lpdwFlags &= HANDLE_FLAG_PROTECT_FROM_CLOSE;
   return TRUE;
}


WINBOOL STDCALL SetHandleInformation(HANDLE hObject,
				     DWORD dwMask,
				     DWORD dwFlags)
{
   OBJECT_DATA_INFORMATION HandleInfo;
   NTSTATUS errCode;
   ULONG BytesWritten;

   errCode = NtQueryObject(hObject,
			   ObjectDataInformation,
			   &HandleInfo,
			   sizeof(OBJECT_DATA_INFORMATION),
			   &BytesWritten);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   if (dwMask & HANDLE_FLAG_INHERIT)
     {
	HandleInfo.bInheritHandle = dwFlags & HANDLE_FLAG_INHERIT;
     }	
   if (dwMask & HANDLE_FLAG_PROTECT_FROM_CLOSE) 
     {
	HandleInfo.bProtectFromClose = dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE;
     }
   
   errCode = NtSetInformationObject(hObject,
				    ObjectDataInformation,
				    &HandleInfo,
				    sizeof(OBJECT_DATA_INFORMATION));
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   
   return TRUE;
}


WINBOOL STDCALL CloseHandle(HANDLE  hObject)
/*
 * FUNCTION: Closes an open object handle
 * PARAMETERS:
 *       hObject = Identifies an open object handle
 * RETURNS: If the function succeeds, the return value is nonzero
 *          If the function fails, the return value is zero
 */
{
   NTSTATUS errCode;
   
   if (IsConsoleHandle(hObject))
     {
	return(CloseConsoleHandle(hObject));
     }
   
   errCode = NtClose(hObject);
   if (!NT_SUCCESS(errCode)) 
     {     
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   
   return TRUE;
}


WINBOOL STDCALL DuplicateHandle(HANDLE hSourceProcessHandle,
				HANDLE hSourceHandle,
				HANDLE hTargetProcessHandle,
				LPHANDLE lpTargetHandle,
				DWORD dwDesiredAccess,
				BOOL bInheritHandle,
				DWORD dwOptions)
{
   NTSTATUS errCode;
   DWORD SourceProcessId, TargetProcessId;
   if (IsConsoleHandle(hSourceHandle))
   {
      if (FALSE == GetProcessId(hSourceProcessHandle, &SourceProcessId) || 
	  FALSE == GetProcessId(hTargetProcessHandle, &TargetProcessId) ||
	  SourceProcessId != TargetProcessId ||
	  SourceProcessId != GetCurrentProcessId())
      {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
      }

      *lpTargetHandle = DuplicateConsoleHandle(hSourceHandle, dwDesiredAccess, bInheritHandle, dwOptions);
      return *lpTargetHandle != INVALID_HANDLE_VALUE ? TRUE : FALSE;
   }
      
   errCode = NtDuplicateObject(hSourceProcessHandle,
			       hSourceHandle,
			       hTargetProcessHandle,
			       lpTargetHandle, 
			       dwDesiredAccess, 
			       (BOOLEAN)bInheritHandle,
			       dwOptions);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   
   return TRUE;
}

UINT STDCALL SetHandleCount(UINT nCount)
{
   return(nCount);
}


/* EOF */
