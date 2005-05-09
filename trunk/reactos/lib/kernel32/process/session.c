/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/session.c
 * PURPOSE:         Win32 session (TS) functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *     2001-12-07 created
 */
#include <k32.h>

DWORD ActiveConsoleSessionId = 0;


/*
 * @unimplemented
 */
DWORD STDCALL
DosPathToSessionPathW (DWORD SessionID, LPWSTR InPath, LPWSTR * OutPath)
{
	return 0;
}

/*
 * From: ActiveVB.DE
 *
 * Declare Function DosPathToSessionPath _
 * Lib "kernel32.dll" _
 * Alias "DosPathToSessionPathA" ( _
 *     ByVal SessionId As Long, _
 *     ByVal pInPath As String, _
 *     ByVal ppOutPath As String ) _
 * As Long
 *
 * @unimplemented
 */
DWORD STDCALL
DosPathToSessionPathA (DWORD SessionId, LPSTR InPath, LPSTR * OutPath)
{
	//DosPathToSessionPathW (SessionId,InPathW,OutPathW);
	return 0;
}

/*
 * @implemented
 */
BOOL STDCALL ProcessIdToSessionId (IN  DWORD dwProcessId,
				   OUT DWORD* pSessionId)
{
  PROCESS_SESSION_INFORMATION SessionInformation;
  OBJECT_ATTRIBUTES ObjectAttributes;
  CLIENT_ID ClientId;
  HANDLE ProcessHandle;
  NTSTATUS Status;

  if(IsBadWritePtr(pSessionId, sizeof(DWORD)))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  ClientId.UniqueProcess = (HANDLE)dwProcessId;
  ClientId.UniqueThread = INVALID_HANDLE_VALUE;

  InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

  Status = NtOpenProcess(&ProcessHandle,
                         PROCESS_QUERY_INFORMATION,
                         &ObjectAttributes,
                         &ClientId);
  if(NT_SUCCESS(Status))
  {
    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessSessionInformation,
                                       &SessionInformation,
                                       sizeof(SessionInformation),
                                       NULL);
    NtClose(ProcessHandle);

    if(NT_SUCCESS(Status))
    {
      *pSessionId = SessionInformation.SessionId;
      return TRUE;
    }
  }

  SetLastErrorByStatus(Status);
  return FALSE;
}

/*
 * @implemented
 */
DWORD STDCALL
WTSGetActiveConsoleSessionId (VOID)
{
	return ActiveConsoleSessionId;
}

/* EOF */
