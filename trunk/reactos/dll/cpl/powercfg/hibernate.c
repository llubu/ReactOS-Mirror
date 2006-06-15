/* $Id: hibernate.c 21500 2006-04-07 10:20:39Z janderwald $
 *
 * PROJECT:         ReactOS Power Configuration Applet
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/cpl/powercfg/hibernate.c
 * PURPOSE:         hibernate tab of applet
 * PROGRAMMERS:     Alexander Wurzinger (Lohnegrim at gmx dot net)
 *                  Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 *                  Martin Rottensteiner
 */

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <stdio.h>

#include "resource.h"
#include "powercfg.h"
#include "powrprof.h"

void Hib_InitDialog(HWND);
INT_PTR Hib_SaveData(HWND);
BOOLEAN Pos_InitData();
void Adv_InitDialog();

/* Property page dialog callback */
INT_PTR CALLBACK
hibernateProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
		Hib_InitDialog(hwndDlg);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_HIBERNATEFILE:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;
			if (lpnm->code == (UINT)PSN_APPLY)
			{
				return Hib_SaveData(hwndDlg);
			}
		}
  }
  return FALSE;
}

void Hib_InitDialog(HWND hwndDlg)
{
	SYSTEM_POWER_CAPABILITIES PowerCaps;
	MEMORYSTATUSEX msex;
	WCHAR szSize[MAX_PATH];
	WCHAR szTemp[MAX_PATH];
	ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;

	if (GetPwrCapabilities(&PowerCaps))
	{
		if (PowerCaps.HiberFilePresent)
		{
			SendMessageW(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE),
						 BM_SETCHECK,
						 (WPARAM)BST_CHECKED,
						 (LPARAM)0);
		}
		else
		{
			SendMessageW(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE),
						 BM_SETCHECK,
						 (WPARAM)BST_UNCHECKED,
						 (LPARAM)0);
		}

		msex.dwLength = sizeof(msex);
		if (!GlobalMemoryStatusEx(&msex))
		{
			return; //FIXME
		}

		if (GetWindowsDirectory(szTemp,MAX_PATH))
		{
			if (!GetDiskFreeSpaceEx(szTemp,&FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
				TotalNumberOfFreeBytes.QuadPart = 0;
		}
		else
		{
			if (!GetDiskFreeSpaceEx(NULL,&FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
				TotalNumberOfFreeBytes.QuadPart = 0;
		}
		if (TotalNumberOfFreeBytes.QuadPart > 0x100000)
		{
			if (LoadString(hApplet, IDS_SIZEMB, szTemp, MAX_PATH))
			{
				swprintf(szSize,szTemp,TotalNumberOfFreeBytes.QuadPart / 0x100000);
				SetWindowText(GetDlgItem(hwndDlg, IDC_FREESPACE),szSize);
			}
		}
		else
		{
			if (LoadString(hApplet, IDS_SIZEBYTS, szTemp, MAX_PATH))
			{
				swprintf(szSize,szTemp,TotalNumberOfFreeBytes.QuadPart);
				SetWindowText(GetDlgItem(hwndDlg, IDC_FREESPACE),szSize);
			}
		}

		if (msex.ullTotalPhys>0x100000)
		{
			if (LoadString(hApplet, IDS_SIZEMB, szTemp, MAX_PATH))
			{
				swprintf(szSize,szTemp,msex.ullTotalPhys/0x100000);
				SetWindowText(GetDlgItem(hwndDlg, IDC_SPACEFORHIBERNATEFILE),szSize);
			}
		}
		else
		{
			if (LoadString(hApplet, IDS_SIZEBYTS, szTemp, MAX_PATH))
			{
				swprintf(szSize,szTemp,msex.ullTotalPhys);
				SetWindowText(GetDlgItem(hwndDlg, IDC_SPACEFORHIBERNATEFILE),szSize);
			}
		}
		if (TotalNumberOfFreeBytes.QuadPart < msex.ullTotalPhys && !PowerCaps.HiberFilePresent)
		{
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), FALSE);		
			ShowWindow(GetDlgItem(hwndDlg, IDC_TOLESSFREESPACE), TRUE);
		}
		else
		{
			ShowWindow(GetDlgItem(hwndDlg, IDC_TOLESSFREESPACE), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE), TRUE);
		}
	}
}

INT_PTR Hib_SaveData(HWND hwndDlg)
{
	BOOLEAN bHibernate;
	
	bHibernate = (BOOLEAN)SendMessageW(GetDlgItem(hwndDlg, IDC_HIBERNATEFILE),
		BM_GETCHECK,
		(WPARAM)0,
		(LPARAM)0);
	if (CallNtPowerInformation(SystemReserveHiberFile,&bHibernate, sizeof(bHibernate), NULL, 0) == STATUS_SUCCESS)
	{
		Pos_InitData();
		Adv_InitDialog();
		Hib_InitDialog(hwndDlg);
		return TRUE;
	}
	return FALSE;
}
