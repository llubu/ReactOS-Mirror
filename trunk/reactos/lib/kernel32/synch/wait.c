/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/wait.c
 * PURPOSE:         Wait  functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <windows.h>
#include <wchar.h>
#include <ddk/ntddk.h>

HANDLE
STDCALL
CreateSemaphoreA(
		 LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		 LONG lInitialCount,
		 LONG lMaximumCount,
		 LPCSTR lpName
		 )
{
	WCHAR NameW[MAX_PATH];
	ULONG i = 0;
  
   	while ((*lpName)!=0 && i < MAX_PATH)
     	{
		NameW[i] = *lpName;
		lpName++;
		i++;
     	}
   	NameW[i] = 0;
	return CreateSemaphoreW(
		 lpSemaphoreAttributes,
		 lInitialCount,
		 lMaximumCount,
		 NameW
		 );
}

HANDLE
STDCALL
CreateSemaphoreW(
		 LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		 LONG lInitialCount,
		 LONG lMaximumCount,
		 LPCWSTR lpName
		 )
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS errCode;
	UNICODE_STRING NameString;
	HANDLE SemaphoreHandle;

	NameString.Length = lstrlenW(lpName)*sizeof(WCHAR);
	NameString.Buffer = (WCHAR *)lpName;
   	NameString.MaximumLength = NameString.Length;
   
   	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   	ObjectAttributes.RootDirectory = NULL;
   	ObjectAttributes.ObjectName = &NameString;
   	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
   	ObjectAttributes.SecurityDescriptor = NULL;
   	ObjectAttributes.SecurityQualityOfService = NULL;

	if ( lpSemaphoreAttributes != NULL ) {
		ObjectAttributes.SecurityDescriptor = lpSemaphoreAttributes->lpSecurityDescriptor;
		if ( lpSemaphoreAttributes->bInheritHandle == TRUE )
			ObjectAttributes.Attributes |= OBJ_INHERIT;
	}
	

	errCode = NtCreateSemaphore(
	&SemaphoreHandle,
	GENERIC_ALL,
	&ObjectAttributes,
	lInitialCount,
	lMaximumCount
	);
	if (!NT_SUCCESS(errCode))
     	{
		SetLastError(RtlNtStatusToDosError(errCode));
		return NULL;
     	}

	return SemaphoreHandle;

		
   
}

HANDLE
STDCALL
CreateMutexA(
	     LPSECURITY_ATTRIBUTES lpMutexAttributes,
	     WINBOOL bInitialOwner,
	     LPCSTR lpName
	     )
{
	WCHAR NameW[MAX_PATH];
	ULONG i = 0;
  
   	while ((*lpName)!=0 && i < MAX_PATH)
     	{
		NameW[i] = *lpName;
		lpName++;
		i++;
     	}
   	NameW[i] = 0;
	
	return CreateMutexW(
	     lpMutexAttributes,
	     bInitialOwner,
	     NameW
	     );
}

HANDLE
STDCALL
CreateMutexW(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    WINBOOL bInitialOwner,
    LPCWSTR lpName
    )
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS errCode;
	UNICODE_STRING NameString;
	HANDLE MutantHandle;

	NameString.Length = lstrlenW(lpName)*sizeof(WCHAR);
	NameString.Buffer = (WCHAR *)lpName;
   	NameString.MaximumLength = NameString.Length;
   
   	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   	ObjectAttributes.RootDirectory = NULL;
   	ObjectAttributes.ObjectName = &NameString;
   	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
   	ObjectAttributes.SecurityDescriptor = NULL;
   	ObjectAttributes.SecurityQualityOfService = NULL;

	if ( lpMutexAttributes != NULL ) {
		ObjectAttributes.SecurityDescriptor = lpMutexAttributes->lpSecurityDescriptor;
		if ( lpMutexAttributes->bInheritHandle == TRUE )
			ObjectAttributes.Attributes |= OBJ_INHERIT;
	}
	

	errCode = NtCreateMutant(&MutantHandle,GENERIC_ALL, &ObjectAttributes,(BOOLEAN)bInitialOwner);
	if (!NT_SUCCESS(errCode))
     	{
		SetLastError(RtlNtStatusToDosError(errCode));
		return NULL;
     	}

	return MutantHandle;

		
   
}


DWORD 
STDCALL
WaitForSingleObject(
    HANDLE  hHandle,
    DWORD  dwMilliseconds 	
   )
{
	return WaitForSingleObjectEx(hHandle,dwMilliseconds,FALSE);	
}

DWORD STDCALL WaitForSingleObjectEx(HANDLE  hHandle,	
				    DWORD  dwMilliseconds,	
				    BOOL  bAlertable)
{
   
   NTSTATUS  errCode;
   PLARGE_INTEGER TimePtr;
   LARGE_INTEGER Time;
   DWORD retCode;
   
   if (dwMilliseconds == INFINITE)
     {
	TimePtr = NULL;
     }
   else
     {
	SET_LARGE_INTEGER_LOW_PART(Time,dwMilliseconds);
	TimePtr = &Time;
     }

   errCode = NtWaitForSingleObject(hHandle,
				   (BOOLEAN) bAlertable,
				   TimePtr);
   
   retCode = RtlNtStatusToDosError(errCode);
   SetLastError(retCode);
   return retCode;
}

DWORD 
STDCALL
WaitForMultipleObjects(
    DWORD  nCount,
    CONST HANDLE *  lpHandles,	
    BOOL  bWaitAll,	
    DWORD  dwMilliseconds 	
   )
{
	return WaitForMultipleObjectsEx(nCount,lpHandles,bWaitAll,dwMilliseconds,FALSE);
}

DWORD 
STDCALL
WaitForMultipleObjectsEx(
    DWORD  nCount,	
    CONST HANDLE *  lpHandles,	
    BOOL  bWaitAll,	
    DWORD  dwMilliseconds,	
    BOOL  bAlertable 	
   )
{
	NTSTATUS  errCode;
	LARGE_INTEGER Time;
	DWORD retCode;

	SET_LARGE_INTEGER_LOW_PART(Time,dwMilliseconds);

	
	errCode = NtWaitForMultipleObjects (
	nCount,
	(PHANDLE)lpHandles,
	(CINT)bWaitAll,
	(BOOLEAN)bAlertable,
	&Time 
	);

	retCode = RtlNtStatusToDosError(errCode);
	SetLastError(retCode);
	return retCode;
	

}
