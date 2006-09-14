/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id$
 *
 * PROJECT:         ReactOS System Control Panel
 * FILE:            lib/cpl/system/hardware.c
 * PURPOSE:         Hardware devices
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      03-04-2004  Created
 */

#include "precomp.h"

typedef BOOL (STDCALL *PDEVMGREXEC)(HWND hWndParent, HINSTANCE hInst, PVOID Unknown, int nCmdShow);
BOOL LaunchDeviceManager(HWND hWndParent)
{
  HMODULE hDll;
  PDEVMGREXEC DevMgrExec;
  BOOL Ret;

  hDll = LoadLibrary(_TEXT("devmgr.dll"));
  if(!hDll)
    return FALSE;
  DevMgrExec = (PDEVMGREXEC)GetProcAddress(hDll, "DeviceManager_ExecuteW");
  if(!DevMgrExec)
  {
    FreeLibrary(hDll);
    return FALSE;
  }
  /* run the Device Manager */
  Ret = DevMgrExec(hWndParent, hApplet, NULL /* ??? */, SW_SHOW);
  FreeLibrary(hDll);
  return Ret;
}

/* Property page dialog callback */
INT_PTR CALLBACK
HardwarePageProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  UNREFERENCED_PARAMETER(lParam);
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDC_HARDWARE_DEVICE_MANAGER:
          if(!LaunchDeviceManager(hwndDlg))
          {
            /* FIXME */
          }
          break;
      }
      break;
  }
  return FALSE;
}

