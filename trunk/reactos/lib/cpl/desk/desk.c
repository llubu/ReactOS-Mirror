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
/* $Id: desk.c,v 1.1 2004/08/07 00:05:23 kuehng Exp $
 *
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/desk.c
 * PURPOSE:         ReactOS Display Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 * UPDATE HISTORY:
 *      06-17-2004  Created
 *      08-07-2004  Initial Checkin
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <windows.h>

#ifdef _MSC_VER
#include <commctrl.h>
#include <cpl.h>
#endif

#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#include "resource.h"
#include "desk.h"

#define NUM_APPLETS	(1)

LONG CALLBACK DisplayApplet(VOID);
BOOL CALLBACK BackgroundPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ScreenSaverPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] = 
{
	{IDI_CPLSYSTEM, IDS_CPLSYSTEMNAME, IDS_CPLSYSTEMDESCRIPTION, DisplayApplet}
};



/* Property page dialog callback */
BOOL CALLBACK BackgroundPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
    case WM_INITDIALOG:	
		break;
	case WM_COMMAND:
		break;
	}
	return FALSE;
}
/* Property page dialog callback */
BOOL CALLBACK ScreenSaverPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
    case WM_INITDIALOG:	
		break;
	case WM_COMMAND:
		break;
	}
	return FALSE;
}
/* Property page dialog callback */
BOOL CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
    case WM_INITDIALOG:	
		break;
	case WM_COMMAND:
		break;
	}
	return FALSE;
}
/* Property page dialog callback */
BOOL CALLBACK SettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
    case WM_INITDIALOG:	
		break;
	case WM_COMMAND:
		break;
	}
	return FALSE;
}

static void InitPropSheetPage(PROPSHEETPAGE *psp, WORD idDlg, DLGPROC DlgProc)
{
	ZeroMemory(psp, sizeof(PROPSHEETPAGE));
	psp->dwSize = sizeof(PROPSHEETPAGE);
	psp->dwFlags = PSP_DEFAULT;
	psp->hInstance = hApplet;
#ifdef _MSC_VER
	psp->pszTemplate = MAKEINTRESOURCE(idDlg);
#else
	psp->u1.pszTemplate = MAKEINTRESOURCE(idDlg);
#endif
	psp->pfnDlgProc = DlgProc;
}


/* First Applet */

LONG CALLBACK
DisplayApplet(VOID)
{
	PROPSHEETPAGE psp[4];
	PROPSHEETHEADER psh;
	TCHAR Caption[1024];
	
	LoadString(hApplet, IDS_CPLSYSTEMNAME, Caption, sizeof(Caption) / sizeof(TCHAR));
	
	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
	psh.hwndParent = NULL;
	psh.hInstance = hApplet;
#ifdef _MSC_VER
	psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#else
	psh.u1.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDI_CPLSYSTEM));
#endif
	psh.pszCaption = Caption;
	psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
#ifdef _MSC_VER
	psh.nStartPage = 0;
	psh.ppsp = psp;
#else
	psh.u2.nStartPage = 0;
	psh.u3.ppsp = psp;
#endif
	psh.pfnCallback = NULL;
	

	InitPropSheetPage(&psp[0], IDD_PROPPAGEBACKGROUND, BackgroundPageProc);
	InitPropSheetPage(&psp[1], IDD_PROPPAGESCREENSAVER, ScreenSaverPageProc);
	InitPropSheetPage(&psp[2], IDD_PROPPAGEAPPEARANCE, AppearancePageProc);
	InitPropSheetPage(&psp[3], IDD_PROPPAGESETTINGS, SettingsPageProc);
	
	return (LONG)(PropertySheet(&psh) != -1);
}

/* Control Panel Callback */
LONG CALLBACK CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
	int i = (int)lParam1;
	
	switch(uMsg)
	{
    case CPL_INIT:
		{
			return TRUE;
		}
    case CPL_GETCOUNT:
		{
			return NUM_APPLETS;
		}
    case CPL_INQUIRE:
		{
			CPLINFO *CPlInfo = (CPLINFO*)lParam2;
			CPlInfo->lData = 0;
			CPlInfo->idIcon = Applets[i].idIcon;
			CPlInfo->idName = Applets[i].idName;
			CPlInfo->idInfo = Applets[i].idDescription;
			break;
		}
    case CPL_DBLCLK:
		{
			Applets[i].AppletProc();
			break;
		}
	}
	return FALSE;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	switch(dwReason)
	{
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
		hApplet = hinstDLL;
		break;
	}
	return TRUE;
}

