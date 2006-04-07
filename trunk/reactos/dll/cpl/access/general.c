/* $Id$
 *
 * PROJECT:         ReactOS System Control Panel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cpl/system/advanced.c
 * PURPOSE:         Memory, start-up and profiles settings
 * COPYRIGHT:       Copyright 2004 Johannes Anderwald (j_anderw@sbox.tugraz.at)
 * UPDATE HISTORY:
 *      03-04-2004  Created
 */
#include <windows.h>
#include <stdlib.h>
#include "resource.h"
#include "access.h"

/* Property page dialog callback */
INT_PTR CALLBACK
GeneralPageProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {      
        case IDC_RESET_BOX:
        {
          break;
        }
        case IDC_NOTIFICATION_MESSAGE:
        {
          break;
        }
        case IDC_NOTIFICATION_SOUND:
        {
          break;
        }
        case IDC_SERIAL_BOX:
        {
          break;
        }
        case IDC_SERIAL_BUTTON:
        {
          break;
        }
        case IDC_ADMIN_LOGON_BOX:
        {
          break;
        }
        case IDC_ADMIN_USERS_BOX:
        {
          break;
        }
        default:
         break;
      }
    }
  }
  return FALSE;
}
