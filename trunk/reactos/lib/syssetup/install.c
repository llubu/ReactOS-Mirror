/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: install.c,v 1.21 2004/12/28 14:41:46 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              lib/syssetup/install.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntos.h>
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>

#include <samlib.h>
#include <syssetup.h>
#include <userenv.h>
#include <setupapi.h>

#include <shlobj.h>
#include <objidl.h>
#include <shlwapi.h>

#include "globals.h"
#include "resource.h"

#define VMWINST


/* GLOBALS ******************************************************************/

PSID DomainSid = NULL;
PSID AdminSid = NULL;

HINF hSysSetupInf = INVALID_HANDLE_VALUE;

/* FUNCTIONS ****************************************************************/

void
DebugPrint(char* fmt,...)
{
  char buffer[512];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  strcat(buffer, "\nRebooting now!");
  MessageBoxA(NULL,
	      buffer,
	      "ReactOS Setup",
	      MB_OK);
}


#ifdef VMWINST
static BOOL
RunVMWInstall(VOID)
{
  PROCESS_INFORMATION ProcInfo;
  STARTUPINFO si;
  
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  
  if(CreateProcess(NULL, _T("vmwinst.exe"), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, 
                   NULL, NULL, &si, &ProcInfo))
  {
    WaitForSingleObject(ProcInfo.hProcess, INFINITE);
    CloseHandle(ProcInfo.hThread);
    CloseHandle(ProcInfo.hProcess);
    return TRUE;
  }
  return FALSE;
}
#endif


HRESULT CreateShellLink(LPCSTR linkPath, LPCSTR cmd, LPCSTR arg, LPCSTR dir, LPCSTR iconPath, int icon_nr, LPCSTR comment)
{
  IShellLinkA* psl;
  IPersistFile* ppf;
  WCHAR buffer[MAX_PATH];

  HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID*)&psl);

  if (SUCCEEDED(hr))
    {
      hr = psl->lpVtbl->SetPath(psl, cmd);

      if (arg)
        {
          hr = psl->lpVtbl->SetArguments(psl, arg);
        }

      if (dir)
        {
          hr = psl->lpVtbl->SetWorkingDirectory(psl, dir);
        }

      if (iconPath)
        {
          hr = psl->lpVtbl->SetIconLocation(psl, iconPath, icon_nr);
        }

      if (comment)
        {
          hr = psl->lpVtbl->SetDescription(psl, comment);
        }

      hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

      if (SUCCEEDED(hr))
        {
          MultiByteToWideChar(CP_ACP, 0, linkPath, -1, buffer, MAX_PATH);

          hr = ppf->lpVtbl->Save(ppf, buffer, TRUE);

          ppf->lpVtbl->Release(ppf);
        }

      psl->lpVtbl->Release(psl);
    }

  return hr;
}


static VOID
CreateCmdLink(VOID)
{
  char path[MAX_PATH];
  LPSTR p;

  CoInitialize(NULL);

  SHGetSpecialFolderPathA(0, path, CSIDL_DESKTOP, TRUE);
  p = PathAddBackslashA(path);

  strcpy(p, "Command Prompt.lnk");
  CreateShellLink(path, "cmd.exe", "", NULL, NULL, 0, "Open command prompt");

  CoUninitialize();
}


static VOID
CreateRandomSid (PSID *Sid)
{
  SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
  LARGE_INTEGER SystemTime;
  PULONG Seed;

  NtQuerySystemTime (&SystemTime);
  Seed = &SystemTime.u.LowPart;

  RtlAllocateAndInitializeSid (&SystemAuthority,
			       4,
			       SECURITY_NT_NON_UNIQUE_RID,
			       RtlUniform (Seed),
			       RtlUniform (Seed),
			       RtlUniform (Seed),
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       SECURITY_NULL_RID,
			       Sid);
}


static VOID
AppendRidToSid (PSID *Dst,
		PSID Src,
		ULONG NewRid)
{
  ULONG Rid[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  UCHAR RidCount;
  ULONG i;

  RidCount = *RtlSubAuthorityCountSid (Src);

  for (i = 0; i < RidCount; i++)
    Rid[i] = *RtlSubAuthoritySid (Src, i);

  if (RidCount < 8)
    {
      Rid[RidCount] = NewRid;
      RidCount++;
    }

  RtlAllocateAndInitializeSid (RtlIdentifierAuthoritySid (Src),
			       RidCount,
			       Rid[0],
			       Rid[1],
			       Rid[2],
			       Rid[3],
			       Rid[4],
			       Rid[5],
			       Rid[6],
			       Rid[7],
			       Dst);
}

INT_PTR CALLBACK
RestartDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch(msg)
   {
      case WM_INITDIALOG:
         SendDlgItemMessage(hWnd, IDC_RESTART_PROGRESS, PBM_SETRANGE, 0,
            MAKELPARAM(0, 300)); 
         SetTimer(hWnd, 0, 50, NULL);
         return TRUE;

      case WM_TIMER:
         {
            INT Position;
            HWND hWndProgress;

            hWndProgress = GetDlgItem(hWnd, IDC_RESTART_PROGRESS);
            Position = SendMessage(hWndProgress, PBM_GETPOS, 0, 0);
            if (Position == 300)
               EndDialog(hWnd, 0);
            else
               SendMessage(hWndProgress, PBM_SETPOS, Position + 1, 0);
         }
         return TRUE;

      case WM_COMMAND:
         switch (wParam)
         {
            case IDOK:
            case IDCANCEL:
               EndDialog(hWnd, 0);
               return TRUE;
         }
         break;
   }

   return FALSE;
}

static VOID
CreateTempDir(LPCWSTR VarName)
{
  TCHAR szTempDir[MAX_PATH];
  TCHAR szBuffer[MAX_PATH];
  DWORD dwLength;
  HKEY hKey;

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		   _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
		   0,
		   KEY_ALL_ACCESS,
		   &hKey))
    {
      DebugPrint("Error: %lu\n", GetLastError());
      return;
    }

  /* Get temp dir */
  dwLength = MAX_PATH * sizeof(TCHAR);
  if (RegQueryValueEx(hKey,
		      VarName,
		      NULL,
		      NULL,
		      (LPBYTE)szBuffer,
		      &dwLength))
    {
      DebugPrint("Error: %lu\n", GetLastError());
      RegCloseKey(hKey);
      return;
    }

  /* Expand it */
  if (!ExpandEnvironmentStrings(szBuffer,
				szTempDir,
				MAX_PATH))
    {
      DebugPrint("Error: %lu\n", GetLastError());
      RegCloseKey(hKey);
      return;
    }

  /* Create profiles directory */
  if (!CreateDirectory(szTempDir, NULL))
    {
      if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
	  DebugPrint("Error: %lu\n", GetLastError());
	  RegCloseKey(hKey);
	  return;
	}
    }

  RegCloseKey(hKey);
}


BOOL
ProcessSysSetupInf(VOID)
{
  INFCONTEXT InfContext;
  TCHAR LineBuffer[256];
  DWORD LineLength;

  if (!SetupFindFirstLine(hSysSetupInf,
			  _T("DeviceInfsToInstall"),
			  NULL,
			  &InfContext))
  {
    return FALSE;
  }

  do
  {
    if (!SetupGetStringField(&InfContext,
			     0,
			     LineBuffer,
			     256,
			     &LineLength))
    {
      return FALSE;
    }

    if (!SetupDiInstallClass(NULL, LineBuffer, DI_QUIETINSTALL, NULL))
    {
      return FALSE;
    }
  }
  while (SetupFindNextLine(&InfContext, &InfContext));

  return TRUE;
}


DWORD STDCALL
InstallReactOS (HINSTANCE hInstance)
{
# if 0
  OutputDebugStringA ("InstallReactOS() called\n");

  if (!InitializeSetupActionLog (FALSE))
    {
      OutputDebugStringA ("InitializeSetupActionLog() failed\n");
    }

  LogItem (SYSSETUP_SEVERITY_INFORMATION,
	   L"ReactOS Setup starting");

  LogItem (SYSSETUP_SEVERITY_FATAL_ERROR,
	   L"Buuuuuuaaaah!");

  LogItem (SYSSETUP_SEVERITY_INFORMATION,
	   L"ReactOS Setup finished");

  TerminateSetupActionLog ();
#endif
#if 0
  UNICODE_STRING SidString;
#endif
  ULONG LastError;

  if (!InitializeProfiles ())
    {
      DebugPrint ("InitializeProfiles() failed\n");
      return 0;
    }

  CreateCmdLink();

  /* Create the semi-random Domain-SID */
  CreateRandomSid (&DomainSid);
  if (DomainSid == NULL)
    {
      DebugPrint ("Domain-SID creation failed!\n");
      return 0;
    }

#if 0
  RtlConvertSidToUnicodeString (&SidString, DomainSid, TRUE);
  DebugPrint ("Domain-SID: %wZ\n", &SidString);
  RtlFreeUnicodeString (&SidString);
#endif

  /* Initialize the Security Account Manager (SAM) */
  if (!SamInitializeSAM ())
    {
      DebugPrint ("SamInitializeSAM() failed!\n");
      RtlFreeSid (DomainSid);
      return 0;
    }

  /* Set the Domain SID (aka Computer SID) */
  if (!SamSetDomainSid (DomainSid))
    {
      DebugPrint ("SamSetDomainSid() failed!\n");
      RtlFreeSid (DomainSid);
      return 0;
    }

  /* Append the Admin-RID */
  AppendRidToSid(&AdminSid, DomainSid, DOMAIN_USER_RID_ADMIN);

#if 0
  RtlConvertSidToUnicodeString (&SidString, DomainSid, TRUE);
  DebugPrint ("Admin-SID: %wZ\n", &SidString);
  RtlFreeUnicodeString (&SidString);
#endif

  /* Create the Administrator account */
  if (!SamCreateUser(L"Administrator", L"", AdminSid))
  {
    /* Check what the error was.
     * If the Admin Account already exists, then it means Setup
     * wasn't allowed to finish properly. Instead of rebooting
     * and not completing it, let it restart instead
     */
    LastError = GetLastError();
    if (LastError != ERROR_USER_EXISTS)
    {
      DebugPrint("SamCreateUser() failed!\n");
      RtlFreeSid(AdminSid);
      RtlFreeSid(DomainSid);
      return 0;
    }
  }

  /* Create the Administrator profile */
  if (!CreateUserProfileW(AdminSid, L"Administrator"))
  {
    DebugPrint("CreateUserProfileW() failed!\n");
    RtlFreeSid(AdminSid);
    RtlFreeSid(DomainSid);
    return 0;
  }

  RtlFreeSid(AdminSid);
  RtlFreeSid(DomainSid);

  CreateTempDir(L"TEMP");
  CreateTempDir(L"TMP");

  hSysSetupInf = SetupOpenInfFileW(L"syssetup.inf",
				   NULL,
				   INF_STYLE_WIN4,
				   NULL);
  if (hSysSetupInf == INVALID_HANDLE_VALUE)
  {
    DebugPrint("SetupOpenInfFileW() failed to open 'syssetup.inf' (Error: %lu)\n", GetLastError());
    return 0;
  }

  if (!ProcessSysSetupInf())
  {
    DebugPrint("ProcessSysSetupInf() failed!\n");
    return 0;
  }

  InstallWizard();

  SetupCloseInfFile(hSysSetupInf);

#ifdef VMWINST
  RunVMWInstall();
#endif

  DialogBox(hDllInstance,
	    MAKEINTRESOURCE(IDD_RESTART),
	    NULL,
	    RestartDlgProc);

  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
SetupChangeFontSize(HANDLE hWnd,
                    LPCWSTR lpszFontSize)
{
  return FALSE;
}
