/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/misc.c
 * PURPOSE:         Various functions
 *
 * PROGRAMMERS:     
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern ULONG NtGlobalFlag;
extern ULONG NtMajorVersion;
extern ULONG NtMinorVersion;
extern ULONG NtOSCSDVersion;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
DebugPrint(IN PANSI_STRING DebugString,
           IN ULONG ComponentId,
           IN ULONG Level)
{
    /* Temporary hack */
    KdpPrintString(DebugString->Buffer, DebugString->Length);
    return STATUS_SUCCESS;
}

/*
* @implemented
*/
ULONG
STDCALL
RtlGetNtGlobalFlags(VOID)
{
	return(NtGlobalFlag);
}


/*
* @implemented
*/
NTSTATUS STDCALL
RtlGetVersion(IN OUT PRTL_OSVERSIONINFOW lpVersionInformation)
{
   if (lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOW) ||
       lpVersionInformation->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW))
   {
      lpVersionInformation->dwMajorVersion = NtMajorVersion;
      lpVersionInformation->dwMinorVersion = NtMinorVersion;
      lpVersionInformation->dwBuildNumber = NtBuildNumber;
      lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
      if(((NtOSCSDVersion >> 8) & 0xFF) != 0)
      {
        int i = _snwprintf(lpVersionInformation->szCSDVersion,
                           (sizeof(lpVersionInformation->szCSDVersion) / sizeof(lpVersionInformation->szCSDVersion[0])) - 1,
                           L"Service Pack %d",
                           ((NtOSCSDVersion >> 8) & 0xFF));
        lpVersionInformation->szCSDVersion[i] = L'\0';
      }
      else
      {
        RtlZeroMemory(lpVersionInformation->szCSDVersion, sizeof(lpVersionInformation->szCSDVersion));
      }
      if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW))
      {
         RTL_OSVERSIONINFOEXW *InfoEx = (RTL_OSVERSIONINFOEXW *)lpVersionInformation;
         InfoEx->wServicePackMajor = (NtOSCSDVersion >> 8) & 0xFF;
         InfoEx->wServicePackMinor = NtOSCSDVersion & 0xFF;
         InfoEx->wSuiteMask = SharedUserData->SuiteMask;
         InfoEx->wProductType = SharedUserData->NtProductType;
      }

      return STATUS_SUCCESS;
   }

   return STATUS_INVALID_PARAMETER;
}

