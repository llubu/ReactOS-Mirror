/* $Id: virtual.c,v 1.11 2003/07/10 18:50:51 chorns Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/virtual.c
 * PURPOSE:              Passing the Virtualxxx functions onto the kernel
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
LPVOID STDCALL
VirtualAllocEx(HANDLE hProcess,
	       LPVOID lpAddress,
	       DWORD dwSize,
	       DWORD flAllocationType,
	       DWORD flProtect)
{
  NTSTATUS Status;

  Status = NtAllocateVirtualMemory(hProcess,
				   (PVOID *)&lpAddress,
				   0,
				   (PULONG)&dwSize,
				   flAllocationType,
				   flProtect);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(NULL);
    }
  return(lpAddress);
}


/*
 * @implemented
 */
LPVOID STDCALL
VirtualAlloc(LPVOID lpAddress,
	     DWORD dwSize,
	     DWORD flAllocationType,
	     DWORD flProtect)
{
  return(VirtualAllocEx(GetCurrentProcess(),
			lpAddress,
			dwSize,
			flAllocationType,
			flProtect));
}


/*
 * @implemented
 */
WINBOOL STDCALL
VirtualFreeEx(HANDLE hProcess,
	      LPVOID lpAddress,
	      DWORD dwSize,
	      DWORD dwFreeType)
{
  NTSTATUS Status;

  Status = NtFreeVirtualMemory(hProcess,
			       (PVOID *)&lpAddress,
			       (PULONG)&dwSize,
			       dwFreeType);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  return(TRUE);
}


/*
 * @implemented
 */
WINBOOL STDCALL
VirtualFree(LPVOID lpAddress,
	    DWORD dwSize,
	    DWORD dwFreeType)
{
  return(VirtualFreeEx(GetCurrentProcess(),
		       lpAddress,
		       dwSize,
		       dwFreeType));
}


/*
 * @implemented
 */
WINBOOL STDCALL
VirtualProtect(LPVOID lpAddress,
	       DWORD dwSize,
	       DWORD flNewProtect,
	       PDWORD lpflOldProtect)
{
  return(VirtualProtectEx(GetCurrentProcess(),
			  lpAddress,
			  dwSize,
			  flNewProtect,
			  lpflOldProtect));
}


/*
 * @implemented
 */
WINBOOL STDCALL
VirtualProtectEx(HANDLE hProcess,
		 LPVOID lpAddress,
		 DWORD dwSize,
		 DWORD flNewProtect,
		 PDWORD lpflOldProtect)
{
  NTSTATUS Status;

  Status = NtProtectVirtualMemory(hProcess,
				  (PVOID)lpAddress,
				  dwSize,
				  flNewProtect,
				  (PULONG)lpflOldProtect);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  return(TRUE);
}


/*
 * @implemented
 */
WINBOOL STDCALL
VirtualLock(LPVOID lpAddress,
	    DWORD dwSize)
{
  ULONG BytesLocked;
  NTSTATUS Status;

  Status = NtLockVirtualMemory(NtCurrentProcess(),
			       lpAddress,
			       dwSize,
			       &BytesLocked);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  return(TRUE);
}


/*
 * @implemented
 */
DWORD STDCALL
VirtualQuery(LPCVOID lpAddress,
	     PMEMORY_BASIC_INFORMATION lpBuffer,
	     DWORD dwLength)
{
  return(VirtualQueryEx(NtCurrentProcess(),
			lpAddress,
			lpBuffer,
			dwLength));
}


/*
 * @implemented
 */
DWORD STDCALL
VirtualQueryEx(HANDLE hProcess,
	       LPCVOID lpAddress,
	       PMEMORY_BASIC_INFORMATION lpBuffer,
	       DWORD dwLength)
{
  NTSTATUS Status;
  ULONG ResultLength;

  Status = NtQueryVirtualMemory(hProcess,
				(LPVOID)lpAddress,
				MemoryBasicInformation,
				lpBuffer,
				sizeof(MEMORY_BASIC_INFORMATION),
				&ResultLength );
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(ResultLength);
    }
  return(ResultLength);
}


/*
 * @implemented
 */
WINBOOL STDCALL
VirtualUnlock(LPVOID lpAddress,
	      DWORD dwSize)
{
  ULONG BytesLocked;
  NTSTATUS Status;

  Status = NtUnlockVirtualMemory(NtCurrentProcess(),
				 lpAddress,
				 dwSize,
				 &BytesLocked);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  return(TRUE);
}

/* EOF */
