/* $Id: handle.c,v 1.9 2003/01/15 21:24:35 chorns Exp $
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
	HandleInfo.bInheritHandle = TRUE;
     }	
   if (dwMask & HANDLE_FLAG_PROTECT_FROM_CLOSE) 
     {
	HandleInfo.bProtectFromClose = TRUE;
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
   if (IsConsoleHandle(hSourceHandle))
   {
      /* FIXME: call CSRSS for console handle duplication */
      if (hSourceProcessHandle == hTargetProcessHandle)
      {
	 *lpTargetHandle = hSourceHandle;
	 return TRUE;
      }
      else
      {
	 return FALSE;
      }
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
