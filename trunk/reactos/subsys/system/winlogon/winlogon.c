/* $Id: winlogon.c,v 1.13 2002/12/27 13:54:28 robd Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/winlogon/winlogon.c
 * PURPOSE:         Logon 
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntos.h>
#include <windows.h>
#include <stdio.h>
#include <lsass/ntsecapi.h>

#include <wchar.h>

#define DBG
#include <debug.h>

/* GLOBALS ******************************************************************/

HWINSTA InteractiveWindowStation;   /* WinSta0 */
HDESK ApplicationDesktop;           /* WinSta0\Default */
HDESK WinlogonDesktop;              /* WinSta0\Winlogon */
HDESK ScreenSaverDesktop;           /* WinSta0\Screen-Saver */

/* FUNCTIONS *****************************************************************/

void PrintString (char* fmt,...)
{
   char buffer[512];
   va_list ap;

   va_start(ap, fmt);
   vsprintf(buffer, fmt, ap);
   va_end(ap);

   OutputDebugString(buffer);
}


BOOLEAN StartServices(VOID)
{
   HANDLE ServicesInitEvent;
   BOOLEAN Result;
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION ProcessInformation;
   CHAR CommandLine[MAX_PATH];
   DWORD Count;

   /* Start the service control manager (services.exe) */   
   GetSystemDirectory(CommandLine, MAX_PATH);
   strcat(CommandLine, "\\services.exe");
      
   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;
   
   PrintString("WL: Creating new process - \"services.exe\".\n");

   Result = CreateProcess(CommandLine,
                          NULL,
                          NULL,
                          NULL,
                          FALSE,
                          DETACHED_PROCESS,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &ProcessInformation);
   if (!Result)
     {
        PrintString("WL: Failed to execute services\n");
        return FALSE;
     }
   
   /* wait for event creation (by SCM) for max. 20 seconds */
   for (Count = 0; Count < 20; Count++)
     {
        Sleep(1000);
   
        //DbgPrint("WL: Attempting to open event \"SvcctrlStartEvent_A3725DX\"\n");
        ServicesInitEvent = OpenEvent(EVENT_ALL_ACCESS, //SYNCHRONIZE,
                                      FALSE,
                                      "SvcctrlStartEvent_A3725DX");
        if (ServicesInitEvent != NULL)
          {
             break;
          }
     }
   
   if (ServicesInitEvent == NULL)
     {
        DbgPrint("WL: Failed to open event \"SvcctrlStartEvent_A3725DX\"\n");
        return FALSE;
     }

   /* wait for event signalization */
   //DbgPrint("WL: Waiting forever on event handle: %x\n", ServicesInitEvent);
   WaitForSingleObject(ServicesInitEvent, INFINITE);
   //DbgPrint("WL: Closing event object \"SvcctrlStartEvent_A3725DX\"\n");
   CloseHandle(ServicesInitEvent);
   DbgPrint("WL: StartServices() Done.\n");
      
   return TRUE;
}

BOOLEAN StartLsass(VOID)
{
   HANDLE LsassInitEvent;
   BOOLEAN Result;
   STARTUPINFO StartupInfo;
   PROCESS_INFORMATION ProcessInformation;
   CHAR CommandLine[MAX_PATH];
   
   LsassInitEvent = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                "\\LsassInitDone");
   
   if (LsassInitEvent == NULL)
     {
        DbgPrint("WL: Failed to create lsass notification event\n");
        return(FALSE);
     }
   
   /* Start the local security authority subsystem (lsass.exe) */
   
   GetSystemDirectory(CommandLine, MAX_PATH);
   strcat(CommandLine, "\\lsass.exe");
   
   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;
   
   Result = CreateProcess(CommandLine,
                          NULL,
                          NULL,
                          NULL,
                          FALSE,
                          DETACHED_PROCESS,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &ProcessInformation);
   if (!Result)
     {
        DbgPrint("WL: Failed to execute lsass\n");
        return(FALSE);
     }
   
   DPRINT("WL: Waiting for lsass\n");
   WaitForSingleObject(LsassInitEvent, INFINITE);
   CloseHandle(LsassInitEvent);
   
   return(TRUE);
}

VOID DoLoginUser(PCHAR Name, PCHAR Password)
{
   PROCESS_INFORMATION ProcessInformation;
   STARTUPINFO StartupInfo;
   BOOLEAN Result;
   CHAR CommandLine[MAX_PATH];
   CHAR CurrentDirectory[MAX_PATH];
   
   GetSystemDirectory(CommandLine, MAX_PATH);
   strcat(CommandLine, "\\shell.exe");

   GetWindowsDirectory(CurrentDirectory, MAX_PATH);

   StartupInfo.cb = sizeof(StartupInfo);
   StartupInfo.lpReserved = NULL;
   StartupInfo.lpDesktop = NULL;
   StartupInfo.lpTitle = NULL;
   StartupInfo.dwFlags = 0;
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved2 = 0;
   
   Result = CreateProcess(CommandLine,
                          NULL,
                          NULL,
                          NULL,
                          FALSE,
                          DETACHED_PROCESS,
                          NULL,
                          CurrentDirectory,
                          &StartupInfo,
                          &ProcessInformation);
   if (!Result)
     {
        DbgPrint("WL: Failed to execute user shell\n");
        return;
     }
   WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
   CloseHandle( ProcessInformation.hProcess );
   CloseHandle( ProcessInformation.hThread );
}

int STDCALL
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nShowCmd)
{
#if 0
  LSA_STRING ProcessName;
  NTSTATUS Status;
  HANDLE LsaHandle;
  LSA_OPERATIONAL_MODE Mode;
#endif
  CHAR LoginPrompt[] = "login:";
  CHAR PasswordPrompt[] = "password:";
  DWORD Result;
  CHAR LoginName[255];
  CHAR Password[255];
  BOOL Success;
  ULONG i;
  NTSTATUS Status;
  
  /*
   * FIXME: Create a security descriptor with
   *        one ACE containing the Winlogon SID
   */
  
  /*
   * Create the interactive window station
   */
   InteractiveWindowStation = 
     CreateWindowStationW(L"WinSta0", 0, GENERIC_ALL, NULL);
   if (InteractiveWindowStation == NULL)
     {
       DbgPrint("WL: Failed to create window station (0x%X)\n", GetLastError());
       NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED, 0, 0, 0, 0, 0);
       ExitProcess(1);
     }
   
   /*
    * Set the process window station
    */
   SetProcessWindowStation(InteractiveWindowStation);

   /*
    * Create the application desktop
    */
   ApplicationDesktop = 
     CreateDesktopW(L"Default",
                    NULL,
                    NULL,
                    0,      /* FIXME: Set some flags */
                    GENERIC_ALL,
                    NULL); 

   /*
    * Create the winlogon desktop
    */
   WinlogonDesktop = CreateDesktopW(L"Winlogon",
                                    NULL,
                                    NULL,
                                    0,      /* FIXME: Set some flags */
                                    GENERIC_ALL,
                                    NULL);  
   
   /*
    * Create the screen saver desktop
    */
   ScreenSaverDesktop = CreateDesktopW(L"Screen-Saver",
                                       NULL,
                                       NULL,
                                       0,      /* FIXME: Set some flags */
                                       GENERIC_ALL,
                                       NULL);  
   
   /*
    * Switch to winlogon desktop
    */
   /* FIXME: Do start up in the application desktop for now. */
   Status = NtSetInformationProcess(NtCurrentProcess(),
                                    ProcessDesktop,
                                    &ApplicationDesktop,
                                    sizeof(ApplicationDesktop));
   if (!NT_SUCCESS(Status))
     {
       DbgPrint("WL: Cannot set default desktop for winlogon.\n");
     }
   SetThreadDesktop(ApplicationDesktop);
   Success = SwitchDesktop(ApplicationDesktop);
   if (!Success)
     {
       DbgPrint("WL: Cannot switch to Winlogon desktop (0x%X)\n", GetLastError());
     }
   
   AllocConsole();
   SetConsoleTitle( "Winlogon" );
   /* start system processes (services.exe & lsass.exe) */
   Success = StartServices();
   if (!Success)
     {
       DbgPrint("WL: Failed to Start Services (0x%X)\n", GetLastError());
     }
#if 0
   Success = StartLsass();
   if (!Success)
     {
       DbgPrint("WL: Failed to Start Security System (0x%X)\n", GetLastError());
     }
#endif
   
   /* FIXME: What name does the real WinLogon use? */
#if 0
   RtlInitUnicodeString((PUNICODE_STRING)&ProcessName, L"WinLogon");
   Status = LsaRegisterLogonProcess(&ProcessName, &LsaHandle, &Mode);
   if (!NT_SUCCESS(Status))
     {
        DbgPrint("WL: Failed to connect to lsass\n");
        return(1);
     }
#endif
   
   /* FIXME: Create a window class and associate a Winlogon
    *        window procedure with it.
    *        Register SAS with the window.
    *        Register for logoff notification
    */
   
   /* Main loop */
   for (;;)
     {
#if 0
       /* Display login prompt */
       WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                    LoginPrompt,
                    strlen(LoginPrompt),  // wcslen(LoginPrompt),
                    &Result,
                    NULL);
       i = 0;
       do
         {
           ReadConsole(GetStdHandle(STD_INPUT_HANDLE),
                       &LoginName[i],
                       1,
                       &Result,
                       NULL);
           i++;
         } while (LoginName[i - 1] != '\n');
       LoginName[i - 1] = 0;
       
        /* Display password prompt */
       WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                    PasswordPrompt,
                    strlen(PasswordPrompt),  // wcslen(PasswordPrompt),
                    &Result,
                    NULL);
       i = 0;
       do
         {
           ReadConsole(GetStdHandle(STD_INPUT_HANDLE),
                       &Password[i],
                       1,
                       &Result,
                       NULL);
           i++;
         } while (Password[i - 1] != '\n');
       Password[i - 1] =0;
#endif
       DoLoginUser(LoginName, Password);
     }
   
   ExitProcess(0);
   
   return 0;
}
