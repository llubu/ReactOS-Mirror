/* $Id: desktop.c,v 1.34 2004/08/21 19:50:39 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/desktop.c
 * PURPOSE:         Desktops
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

#include "user32.h"
#include "winsta.h"
#include <string.h>
#include <debug.h>
#include <rosrtl/devmode.h>
#include <rosrtl/logfont.h>
#include <malloc.h>
#include <math.h>

/*
 * @implemented
 */
int STDCALL
GetSystemMetrics(int nIndex)
{
  return(NtUserGetSystemMetrics(nIndex));
}


/*
 * @unimplemented
 */
BOOL STDCALL SetDeskWallpaper(LPCSTR filename)
{
	return SystemParametersInfoA(SPI_SETDESKWALLPAPER,0,(PVOID)filename,1);
}
/*
 * @implemented
 */
BOOL STDCALL
SystemParametersInfoA(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  switch (uiAction)
    {
      case SPI_SETDOUBLECLKWIDTH:
      case SPI_SETDOUBLECLKHEIGHT:
      case SPI_SETDOUBLECLICKTIME:
      case SPI_SETGRADIENTCAPTIONS:
      case SPI_SETFONTSMOOTHING:
      case SPI_SETFOCUSBORDERHEIGHT:
      case SPI_SETFOCUSBORDERWIDTH:
      case SPI_SETWORKAREA:
      case SPI_GETWORKAREA:
      case SPI_GETFONTSMOOTHING:
      case SPI_GETGRADIENTCAPTIONS:
      case SPI_GETFOCUSBORDERHEIGHT:
      case SPI_GETFOCUSBORDERWIDTH:
        {
           return NtUserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
        }
      case SPI_GETNONCLIENTMETRICS:
        {
           LPNONCLIENTMETRICSA nclma = (LPNONCLIENTMETRICSA)pvParam;
           NONCLIENTMETRICSW nclmw;
           nclmw.cbSize = sizeof(NONCLIENTMETRICSW);

           if (!SystemParametersInfoW(uiAction, sizeof(NONCLIENTMETRICSW),
                                      &nclmw, fWinIni))
             return FALSE;

           nclma->iBorderWidth = nclmw.iBorderWidth;
           nclma->iScrollWidth = nclmw.iScrollWidth;
           nclma->iScrollHeight = nclmw.iScrollHeight;
           nclma->iCaptionWidth = nclmw.iCaptionWidth;
           nclma->iCaptionHeight = nclmw.iCaptionHeight;
           nclma->iSmCaptionWidth = nclmw.iSmCaptionWidth;
           nclma->iSmCaptionHeight = nclmw.iSmCaptionHeight;
           nclma->iMenuWidth = nclmw.iMenuWidth;
           nclma->iMenuHeight = nclmw.iMenuHeight;
           RosRtlLogFontW2A(&(nclma->lfCaptionFont), &(nclmw.lfCaptionFont));
           RosRtlLogFontW2A(&(nclma->lfSmCaptionFont), &(nclmw.lfSmCaptionFont));
           RosRtlLogFontW2A(&(nclma->lfMenuFont), &(nclmw.lfMenuFont));
           RosRtlLogFontW2A(&(nclma->lfStatusFont), &(nclmw.lfStatusFont));
           RosRtlLogFontW2A(&(nclma->lfMessageFont), &(nclmw.lfMessageFont));
           return TRUE;
        }
      case SPI_GETICONTITLELOGFONT:
        {
           LOGFONTW lfw;
           if (!SystemParametersInfoW(uiAction, 0, &lfw, fWinIni))
             return FALSE;
           RosRtlLogFontW2A(pvParam, &lfw);
           return TRUE;
        }
      case SPI_GETDESKWALLPAPER:
      {
        HKEY hKey;
        BOOL Ret = FALSE;

#if 0
        /* Get the desktop bitmap handle, this does NOT return the file name! */
        if(!NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, 0, &hbmWallpaper, 0))
        {
          /* Return an empty string, no wallpapaper is set */
          *(CHAR*)pvParam = '\0';
          return TRUE;
        }
#endif
        
        /* FIXME - Read the registry key for now, but what happens if the wallpaper was
                   changed without SPIF_UPDATEINIFILE?! */
        if(RegOpenKeyExW(HKEY_CURRENT_USER,
                         L"Control Panel\\Desktop",
                         0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
          DWORD Type, Size;
          Size = uiParam;
          if(RegQueryValueExA(hKey,
                              "Wallpaper",
                              NULL,
                              &Type,
                              (LPBYTE)pvParam,
                              &Size) == ERROR_SUCCESS
             && Type == REG_SZ)
          {
            Ret = TRUE;
          }
          RegCloseKey(hKey);
        }
        return Ret;
      }
      case SPI_SETDESKWALLPAPER:
      {
        HBITMAP hNewWallpaper;
        BOOL Ret;
        LPSTR lpWallpaper = (LPSTR)pvParam;
        
        if(lpWallpaper != NULL && *lpWallpaper != '\0')
        {
          hNewWallpaper = LoadImageA(0, lpWallpaper, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
          if(hNewWallpaper == NULL)
          {
            return FALSE;
          }
        }
        else
        {
          hNewWallpaper = NULL;
          lpWallpaper = NULL;
        }
        
        /* Set the wallpaper bitmap */
        if(!NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &hNewWallpaper, fWinIni & SPIF_SENDCHANGE))
        {
          if(hNewWallpaper != NULL)
            DeleteObject(hNewWallpaper);
          return FALSE;
        }
        /* Do not use the bitmap handle anymore, it doesn't belong to our process anymore! */
        
        Ret = TRUE;
        if(fWinIni & SPIF_UPDATEINIFILE)
        {
          /* Save the path to the file in the registry */
          HKEY hKey;
          if(RegOpenKeyExW(HKEY_CURRENT_USER,
                           L"Control Panel\\Desktop",
                           0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
          {
            Ret = RegSetValueExA(hKey, "Wallpaper", 0, REG_SZ, (lpWallpaper != NULL ? lpWallpaper : ""),
                                 (lpWallpaper != NULL ? (lstrlenA(lpWallpaper) + 1) * sizeof(CHAR) : sizeof(CHAR)) == ERROR_SUCCESS);
            RegCloseKey(hKey);
          }
        }

        RedrawWindow(GetDesktopWindow(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

        return Ret;
      }
    }

  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
SystemParametersInfoW(UINT uiAction,
		      UINT uiParam,
		      PVOID pvParam,
		      UINT fWinIni)
{
  switch(uiAction)
  {
    case SPI_GETDESKWALLPAPER:
    {
      HKEY hKey;
      BOOL Ret = FALSE;

#if 0
      /* Get the desktop bitmap handle, this does NOT return the file name! */
      if(!NtUserSystemParametersInfo(SPI_GETDESKWALLPAPER, 0, &hbmWallpaper, 0))
      {
        /* Return an empty string, no wallpapaper is set */
        *(WCHAR*)pvParam = L'\0';
        return TRUE;
      }
#endif

      /* FIXME - Read the registry key for now, but what happens if the wallpaper was
                 changed without SPIF_UPDATEINIFILE?! */
      if(RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"Control Panel\\Desktop",
                       0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
      {
        DWORD Type, Size;
        Size = uiParam * sizeof(WCHAR);
        if(RegQueryValueExW(hKey,
                            L"Wallpaper",
                            NULL,
                            &Type,
                            (LPBYTE)pvParam,
                            &Size) == ERROR_SUCCESS
           && Type == REG_SZ)
        {
          Ret = TRUE;
        }
        RegCloseKey(hKey);
      }
      return Ret;
    }
    case SPI_SETDESKWALLPAPER:
    {
      HBITMAP hNewWallpaper;
      BOOL Ret;
      LPWSTR lpWallpaper = (LPWSTR)pvParam;

      if(lpWallpaper != NULL && *lpWallpaper != L'\0')
      {
        hNewWallpaper = LoadImageW(0, lpWallpaper, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

        if(hNewWallpaper == NULL)
        {
          return FALSE;
        }
      }
      else
      {
        hNewWallpaper = NULL;
        lpWallpaper = NULL;
      }

      /* Set the wallpaper bitmap */
      if(!NtUserSystemParametersInfo(SPI_SETDESKWALLPAPER, 0, &hNewWallpaper, fWinIni & SPIF_SENDCHANGE))
      {
        if(hNewWallpaper != NULL)
          DeleteObject(hNewWallpaper);
        return FALSE;
      }
      /* Do not use the bitmap handle anymore, it doesn't belong to our process anymore! */
      Ret = TRUE;
      if(fWinIni & SPIF_UPDATEINIFILE)
      {
        /* Save the path to the file in the registry */
        HKEY hKey;

        if(RegOpenKeyExW(HKEY_CURRENT_USER,
                         L"Control Panel\\Desktop",
                         0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
        {
          Ret = RegSetValueExW(hKey, L"Wallpaper", 0, REG_SZ, (lpWallpaper != NULL ? (LPBYTE)lpWallpaper : (LPBYTE)L""),
                               (lpWallpaper != NULL ? (lstrlenW(lpWallpaper) + 1) * sizeof(WCHAR) : sizeof(WCHAR)) == ERROR_SUCCESS);
          RegCloseKey(hKey);
        }
      }

      RedrawWindow(GetDesktopWindow(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

      return Ret;
    }
  }
  return NtUserSystemParametersInfo(uiAction, uiParam, pvParam, fWinIni);
}


/*
 * @implemented
 */
BOOL
STDCALL
CloseDesktop(
  HDESK hDesktop)
{
  return NtUserCloseDesktop(hDesktop);
}


/*
 * @implemented
 */
HDESK STDCALL
CreateDesktopA(LPCSTR lpszDesktop,
	       LPCSTR lpszDevice,
	       LPDEVMODEA pDevmode,
	       DWORD dwFlags,
	       ACCESS_MASK dwDesiredAccess,
	       LPSECURITY_ATTRIBUTES lpsa)
{
  ANSI_STRING DesktopNameA;
  UNICODE_STRING DesktopNameU;
  HDESK hDesktop;
  DEVMODEW DevmodeW;

  if (lpszDesktop != NULL) 
    {
      RtlInitAnsiString(&DesktopNameA, (LPSTR)lpszDesktop);
      RtlAnsiStringToUnicodeString(&DesktopNameU, &DesktopNameA, TRUE);
    } 
  else 
    {
      RtlInitUnicodeString(&DesktopNameU, NULL);
    }

  RosRtlDevModeA2W ( &DevmodeW, pDevmode );

  hDesktop = CreateDesktopW(DesktopNameU.Buffer,
			    NULL,
			    &DevmodeW,
			    dwFlags,
			    dwDesiredAccess,
			    lpsa);

  RtlFreeUnicodeString(&DesktopNameU);
  return(hDesktop);
}


/*
 * @implemented
 */
HDESK STDCALL
CreateDesktopW(LPCWSTR lpszDesktop,
	       LPCWSTR lpszDevice,
	       LPDEVMODEW pDevmode,
	       DWORD dwFlags,
	       ACCESS_MASK dwDesiredAccess,
	       LPSECURITY_ATTRIBUTES lpsa)
{
  UNICODE_STRING DesktopName;
  HWINSTA hWinSta;
  HDESK hDesktop;

  hWinSta = NtUserGetProcessWindowStation();

  RtlInitUnicodeString(&DesktopName, lpszDesktop);

  hDesktop = NtUserCreateDesktop(&DesktopName,
				 dwFlags,
				 dwDesiredAccess,
				 lpsa,
				 hWinSta);

  return(hDesktop);
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDesktopsA(
  HWINSTA WindowStation,
  DESKTOPENUMPROCA EnumFunc,
  LPARAM Context)
{
   return EnumNamesA(WindowStation, EnumFunc, Context, TRUE);
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDesktopsW(
  HWINSTA WindowStation,
  DESKTOPENUMPROCW EnumFunc,
  LPARAM Context)
{
   return EnumNamesW(WindowStation, EnumFunc, Context, TRUE);
}


/*
 * @implemented
 */
HDESK
STDCALL
GetThreadDesktop(
  DWORD dwThreadId)
{
  return NtUserGetThreadDesktop(dwThreadId, 0);
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenDesktopA(
  LPSTR lpszDesktop,
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  ANSI_STRING DesktopNameA;
  UNICODE_STRING DesktopNameU;
  HDESK hDesktop;

	if (lpszDesktop != NULL) {
		RtlInitAnsiString(&DesktopNameA, lpszDesktop);
		RtlAnsiStringToUnicodeString(&DesktopNameU, &DesktopNameA, TRUE);
  } else {
    RtlInitUnicodeString(&DesktopNameU, NULL);
  }

  hDesktop = OpenDesktopW(
    DesktopNameU.Buffer,
    dwFlags,
    fInherit,
    dwDesiredAccess);

	RtlFreeUnicodeString(&DesktopNameU);

  return hDesktop;
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenDesktopW(
  LPWSTR lpszDesktop,
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  UNICODE_STRING DesktopName;

  RtlInitUnicodeString(&DesktopName, lpszDesktop);

  return NtUserOpenDesktop(
    &DesktopName,
    dwFlags,
    dwDesiredAccess);
}


/*
 * @implemented
 */
HDESK
STDCALL
OpenInputDesktop(
  DWORD dwFlags,
  BOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  return NtUserOpenInputDesktop(
    dwFlags,
    fInherit,
    dwDesiredAccess);
}


/*
 * @implemented
 */
BOOL
STDCALL
PaintDesktop(
  HDC hdc)
{
  return NtUserPaintDesktop(hdc);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetThreadDesktop(
  HDESK hDesktop)
{
  return NtUserSetThreadDesktop(hDesktop);
}


/*
 * @implemented
 */
BOOL
STDCALL
SwitchDesktop(
  HDESK hDesktop)
{
  return NtUserSwitchDesktop(hDesktop);
}


/*
 * @implemented
 */
BOOL STDCALL
SetShellWindowEx(HWND hwndShell, HWND hwndShellListView)
{
	return NtUserSetShellWindowEx(hwndShell, hwndShellListView);
}


/*
 * @implemented
 */
BOOL STDCALL
SetShellWindow(HWND hwndShell)
{
	return SetShellWindowEx(hwndShell, hwndShell);
}


/*
 * @implemented
 */
HWND STDCALL
GetShellWindow(VOID)
{
	return NtUserGetShellWindow();
}


/* EOF */
