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
 * PROJECT:         ReactOS International Control Panel
 * FILE:            lib/cpl/intl/numbers.c
 * PURPOSE:         Numbers property page
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "intl.h"
#include "resource.h"

#define SAMPLE_NUMBER               _T("123456789")
#define SAMPLE_NEG_NUMBER           _T("-123456789")
#define MAX_NUM_SEP_SAMPLES         2
#define MAX_FRAC_NUM_SAMPLES        9
#define MAX_FIELD_SEP_SAMPLES       1
#define MAX_FIELD_DIG_SAMPLES       3
#define MAX_NEG_SIGN_SAMPLES        1
#define MAX_NEG_NUMBERS_SAMPLES     5
#define MAX_LEAD_ZEROES_SAMPLES     2
#define MAX_LIST_SEP_SAMPLES        1
#define MAX_UNITS_SYS_SAMPLES       2

/* Init num decimal separator control box */
static VOID
InitNumDecimalSepCB(HWND hwndDlg)
{
    LPTSTR lpNumSepSamples[MAX_NUM_SEP_SAMPLES] = {_T(","), _T(".")};
    TCHAR szNumSep[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Get current decimal separator */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_SDECIMAL,
                  szNumSep,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERDSYMBOL),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Create standard list of decimal separators */
    for (nCBIndex = 0; nCBIndex < MAX_NUM_SEP_SAMPLES; nCBIndex++)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERDSYMBOL),
                    CB_ADDSTRING,
                    nCBIndex,
                    (LPARAM)lpNumSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERDSYMBOL),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)(LPCSTR)szNumSep);

    /* if is not success, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERDSYMBOL),
                    CB_ADDSTRING,
                    MAX_NUM_SEP_SAMPLES,
                    (LPARAM)szNumSep);
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERDSYMBOL),
                    CB_SELECTSTRING,
                    -1,
                    (LPARAM)szNumSep);
    }
}

/* Init number of fractional symbols control box */
static VOID
InitNumOfFracSymbCB(HWND hwndDlg)
{
    TCHAR szFracNum[MAX_SAMPLES_STR_SIZE];
    TCHAR szFracCount[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Get current number of fractional symbols */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_IDIGITS,
                  szFracNum,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNDIGDEC),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Create standard list of fractional symbols */
    for (nCBIndex = 0; nCBIndex < MAX_FRAC_NUM_SAMPLES; nCBIndex++)
    {
        /* convert to wide char */
        _itot(nCBIndex, szFracCount, DECIMAL_RADIX);

        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNDIGDEC),
                    CB_ADDSTRING,
                    nCBIndex,
                    (LPARAM)szFracCount);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNDIGDEC),
                           CB_SETCURSEL,
                           (WPARAM)_ttoi(szFracNum),
                           (LPARAM)0);
}

/* Init field separator control box */
static VOID
InitNumFieldSepCB(HWND hwndDlg)
{
    LPTSTR lpFieldSepSamples[MAX_FIELD_SEP_SAMPLES] = {_T(" ")};
    TCHAR szFieldSep[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Get current field separator */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_STHOUSAND,
                  szFieldSep,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDIGITGRSYM),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Create standart list of field separators */
    for (nCBIndex = 0; nCBIndex < MAX_FIELD_SEP_SAMPLES; nCBIndex++)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDIGITGRSYM),
                    CB_ADDSTRING,
                    nCBIndex,
                    (LPARAM)lpFieldSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDIGITGRSYM),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)szFieldSep);

    /* if is not success, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDIGITGRSYM),
                    CB_ADDSTRING,
                    MAX_FIELD_SEP_SAMPLES+1,
                    (LPARAM)szFieldSep);
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDIGITGRSYM),
                    CB_SELECTSTRING,
                    -1,
                    (LPARAM)szFieldSep);
    }
}

/* Init number of digidts in field control box */
static VOID
InitFieldDigNumCB(HWND hwndDlg)
{
    LPTSTR lpFieldDigNumSamples[MAX_FIELD_DIG_SAMPLES] =
    {
        _T("0;0"),
        _T("3;0"),
        _T("3;2;0")
    };

    TCHAR szFieldDigNum[MAX_SAMPLES_STR_SIZE];
    LPTSTR pszFieldDigNumSmpl;
    INT nCBIndex;
    INT nRetCode;

    /* Get current field digits num */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_SGROUPING,
                  szFieldDigNum,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDGROUPING),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Create standart list of field digits num */
    for (nCBIndex = 0; nCBIndex < MAX_FIELD_DIG_SAMPLES; nCBIndex++)
    {

        pszFieldDigNumSmpl = InsSpacesFmt(SAMPLE_NUMBER, lpFieldDigNumSamples[nCBIndex]);
        SendMessageW(GetDlgItem(hwndDlg, IDC_NUMBERSDGROUPING),
                     CB_ADDSTRING,
                     nCBIndex,
                     (LPARAM)pszFieldDigNumSmpl);
        free(pszFieldDigNumSmpl);
    }

    pszFieldDigNumSmpl = InsSpacesFmt(SAMPLE_NUMBER, szFieldDigNum);
    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDGROUPING),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)pszFieldDigNumSmpl);

    /* if is not success, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDGROUPING),
                    CB_ADDSTRING,
                    MAX_FIELD_DIG_SAMPLES+1,
                    (LPARAM)pszFieldDigNumSmpl);
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDGROUPING),
                    CB_SELECTSTRING,
                    -1,
                    (LPARAM)pszFieldDigNumSmpl);
    }

    free(pszFieldDigNumSmpl);
}

/* Init negative sign control box */
static VOID
InitNegSignCB(HWND hwndDlg)
{
    LPTSTR lpNegSignSamples[MAX_NEG_SIGN_SAMPLES] = {_T("-")};
    TCHAR szNegSign[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Get current negative sign */
    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_SNEGATIVESIGN,
                   szNegSign,
                   MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNSIGNSYM),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Create standart list of signs */
    for (nCBIndex = 0; nCBIndex < MAX_NEG_SIGN_SAMPLES; nCBIndex++)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNSIGNSYM),
                    CB_ADDSTRING,
                    nCBIndex,
                    (LPARAM)lpNegSignSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNSIGNSYM),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)szNegSign);

    /* if is not success, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNSIGNSYM),
                    CB_ADDSTRING,
                    MAX_NUM_SEP_SAMPLES+1,
                    (LPARAM)szNegSign);
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNSIGNSYM),
                    CB_SELECTSTRING,
                    -1,
                    (LPARAM)szNegSign);
    }
}

/* Init negative numbers format control box */
static VOID
InitNegNumFmtCB(HWND hwndDlg)
{
    LPTSTR lpNegNumFmtSamples[MAX_NEG_NUMBERS_SAMPLES] =
    {
        _T("(1,1)"),
        _T("-1,1"),
        _T("- 1,1"),
        _T("1,1-"),
        _T("1,1 -")
    };

    TCHAR szNegNumFmt[MAX_SAMPLES_STR_SIZE];
    TCHAR szNumSep[MAX_SAMPLES_STR_SIZE];
    TCHAR szNegSign[MAX_SAMPLES_STR_SIZE];
    TCHAR szNewSample[MAX_SAMPLES_STR_SIZE];
    LPTSTR pszResultStr;
    INT nCBIndex;
    INT nRetCode;

    /* Get current negative numbers format */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_INEGNUMBER,
                  szNegNumFmt,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNNUMFORMAT),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Get current decimal separator */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_SDECIMAL,
                  szNumSep,
                  MAX_SAMPLES_STR_SIZE);

    /* Get current negative sign */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_SNEGATIVESIGN,
                  szNegSign,
                  MAX_SAMPLES_STR_SIZE);

    /* Create standart list of negative numbers formats */
    for (nCBIndex = 0; nCBIndex < MAX_NEG_NUMBERS_SAMPLES; nCBIndex++)
    {
        /* Replace standart separator to setted */
        pszResultStr = ReplaceSubStr(lpNegNumFmtSamples[nCBIndex],
                                     szNumSep,
                                     _T(","));
        _tcscpy(szNewSample, pszResultStr);
        free(pszResultStr);
        /* Replace standart negative sign to setted */
        pszResultStr = ReplaceSubStr(szNewSample,
                                     szNegSign,
                                     _T("-"));
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNNUMFORMAT),
                    CB_ADDSTRING,
                    nCBIndex,
                    (LPARAM)pszResultStr);
        free(pszResultStr);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNNUMFORMAT),
                           CB_SETCURSEL,
                           (WPARAM)_ttoi(szNegNumFmt),
                           (LPARAM)0);
}

/* Init leading zeroes control box */
static VOID
InitLeadingZeroesCB(HWND hwndDlg)
{
    LPTSTR lpLeadNumFmtSamples[MAX_LEAD_ZEROES_SAMPLES] =
    {
        _T(",7"),
        _T("0,7")
    };

    TCHAR szLeadNumFmt[MAX_SAMPLES_STR_SIZE];
    TCHAR szNumSep[MAX_SAMPLES_STR_SIZE];
    LPTSTR pszResultStr;
    INT nCBIndex;
    INT nRetCode;

    /* Get current leading zeroes format */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_ILZERO,
                  szLeadNumFmt,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDISPLEADZER),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Get current decimal separator */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_SDECIMAL,
                  szNumSep,
                  MAX_SAMPLES_STR_SIZE);

    /* Create list of standart leading zeroes formats */
    for (nCBIndex = 0; nCBIndex < MAX_LEAD_ZEROES_SAMPLES; nCBIndex++)
    {
        pszResultStr = ReplaceSubStr(lpLeadNumFmtSamples[nCBIndex],
                                     szNumSep,
                                     _T(","));
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDISPLEADZER),
                    CB_ADDSTRING,
                    nCBIndex,
                    (LPARAM)pszResultStr);
        free(pszResultStr);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDISPLEADZER),
                           CB_SETCURSEL,
                           (WPARAM)_ttoi(szLeadNumFmt),
                           (LPARAM)0);
}

static VOID
InitListSepCB(HWND hwndDlg)
{
    LPTSTR lpListSepSamples[MAX_LIST_SEP_SAMPLES] = {_T(";")};
    TCHAR szListSep[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Get current list separator */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_SLIST,
                  szListSep,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSLSEP),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Create standart list of signs */
    for (nCBIndex = 0; nCBIndex < MAX_LIST_SEP_SAMPLES; nCBIndex++)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSLSEP),
                    CB_ADDSTRING,
                    nCBIndex,
                    (LPARAM)lpListSepSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSLSEP),
                           CB_SELECTSTRING,
                           -1,
                           (LPARAM)szListSep);

    /* if is not success, add new value to list and select them */
    if (nRetCode == CB_ERR)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSLSEP),
                    CB_ADDSTRING,
                    MAX_LIST_SEP_SAMPLES+1,
                    (LPARAM)szListSep);
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSLSEP),
                    CB_SELECTSTRING,
                    -1,
                    (LPARAM)szListSep);
    }
}

/* Init system of units control box */
VOID
InitUnitsSysCB(HWND hwndDlg)
{
    LPTSTR lpUnitsSysSamples[MAX_UNITS_SYS_SAMPLES] =
    {
        _T("Metrics"),
        _T("Americans")
    };

    TCHAR szUnitsSys[MAX_SAMPLES_STR_SIZE];
    INT nCBIndex;
    INT nRetCode;

    /* Get current system of units */
    GetLocaleInfo(LOCALE_USER_DEFAULT,
                  LOCALE_IMEASURE,
                  szUnitsSys,
                  MAX_SAMPLES_STR_SIZE);

    /* Clear all box content */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSMEASSYS),
                CB_RESETCONTENT,
                (WPARAM)0,
                (LPARAM)0);

    /* Create list of standart system of units */
    for (nCBIndex = 0; nCBIndex < MAX_UNITS_SYS_SAMPLES; nCBIndex++)
    {
        SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSMEASSYS),
                    CB_ADDSTRING,
                    nCBIndex,
                    (LPARAM)lpUnitsSysSamples[nCBIndex]);
    }

    /* Set current item to value from registry */
    nRetCode = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSMEASSYS),
                           CB_SETCURSEL,
                           (WPARAM)_ttoi(szUnitsSys),
                           (LPARAM)0);
}

/* Update all numbers locale samples */
static VOID
UpdateNumSamples(HWND hwndDlg,
                 LCID lcidLocale)
{
    TCHAR OutBuffer[MAX_FMT_SIZE];

    /* Get positive number format sample */
    GetNumberFormat(lcidLocale,
                    0,
                    SAMPLE_NUMBER,
                    NULL,
                    OutBuffer,
                    MAX_FMT_SIZE);

    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSPOSSAMPLE),
                WM_SETTEXT,
                0,
                (LPARAM)OutBuffer);

    /* Get positive number format sample */
    GetNumberFormat(lcidLocale,
                    0,
                    SAMPLE_NEG_NUMBER,
                    NULL,
                    OutBuffer,
                    MAX_FMT_SIZE);

    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNEGSAMPLE),
                WM_SETTEXT,
                0,
                (LPARAM)OutBuffer);
}

/* Set num decimal separator */
static BOOL
SetNumDecimalSep(HWND hwndDlg)
{
    TCHAR szDecimalSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted decimal separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERDSYMBOL),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szDecimalSep);

    /* Save decimal separator */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSep);

    return TRUE;
}

/* Set number of fractional symbols */
static BOOL
SetFracSymNum(HWND hwndDlg)
{
    TCHAR szFracSymNum[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted number of fractional symbols */
    nCurrSel = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNDIGDEC),
                           CB_GETCURSEL,
                           (WPARAM)0,
                           (LPARAM)0);

    /* convert to wide char */
    _itot(nCurrSel, szFracSymNum, DECIMAL_RADIX);

    /* Save number of fractional symbols */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDIGITS, szFracSymNum);

    return TRUE;
}

/* Set field separator */
static BOOL
SetNumFieldSep(HWND hwndDlg)
{
    TCHAR szFieldSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted field separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSDGROUPING),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szFieldSep);

    /* Save field separator */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szFieldSep);

    return TRUE;
}

/* Set number of digidts in field  */
static BOOL
SetFieldDigNum(HWND hwndDlg)
{
    LPTSTR lpFieldDigNumSamples[MAX_FIELD_DIG_SAMPLES] =
    {
        _T("0;0"),
        _T("3;0"),
        _T("3;2;0")
    };

    INT nCurrSel;

    /* Get setted number of digidts in field */
    nCurrSel=SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNSIGNSYM),
                         CB_GETCURSEL,
                         (WPARAM)0,
                         (LPARAM)0);

    /* Save number of digidts in field */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, lpFieldDigNumSamples[nCurrSel]);

    return TRUE;
}

/* Set negative sign */
static BOOL
SetNumNegSign(HWND hwndDlg)
{
    TCHAR szNegSign[MAX_SAMPLES_STR_SIZE];

    /* Get setted negative sign */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNSIGNSYM),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szNegSign);

    /* Save negative sign */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SNEGATIVESIGN, szNegSign);

    return TRUE;
}

/* Set negative sum format */
static BOOL
SetNegSumFmt(HWND hwndDlg)
{
    TCHAR szNegSumFmt[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted negative sum format */
    nCurrSel = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSNNUMFORMAT),
                           CB_GETCURSEL,
                           (WPARAM)0,
                           (LPARAM)0);

    /* convert to wide char */
    _itot(nCurrSel, szNegSumFmt,DECIMAL_RADIX);

    /* Save negative sum format */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, szNegSumFmt);

    return TRUE;
}

/* Set leading zero */
static BOOL
SetNumLeadZero(HWND hwndDlg)
{
    TCHAR szLeadZero[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted leading zero format */
    nCurrSel = SendMessageW(GetDlgItem(hwndDlg, IDC_NUMBERSDISPLEADZER),
                            CB_GETCURSEL,
                            (WPARAM)0,
                            (LPARAM)0);

    /* convert to wide char */
    _itot(nCurrSel, szLeadZero, DECIMAL_RADIX);

    /* Save leading zero format */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ILZERO, szLeadZero);

    return TRUE;
}

/* Set elements list separator */
static BOOL
SetNumListSep(HWND hwndDlg)
{
    TCHAR szListSep[MAX_SAMPLES_STR_SIZE];

    /* Get setted list separator */
    SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSLSEP),
                WM_GETTEXT,
                (WPARAM)MAX_SAMPLES_STR_SIZE,
                (LPARAM)szListSep);

    /* Save list separator */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLIST, szListSep);

    return TRUE;
}

/* Set units system */
static BOOL
SetNumUnitsSys(HWND hwndDlg)
{
    TCHAR szUnitsSys[MAX_SAMPLES_STR_SIZE];
    INT nCurrSel;

    /* Get setted units system */
    nCurrSel = SendMessage(GetDlgItem(hwndDlg, IDC_NUMBERSMEASSYS),
                           CB_GETCURSEL,
                           (WPARAM)0,
                           (LPARAM)0);

    /* convert to wide char */
    _itot(nCurrSel, szUnitsSys, DECIMAL_RADIX);

    /* Save units system */
    SetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, szUnitsSys);

    return TRUE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
NumbersPageProc(HWND hwndDlg,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
	{
		InitNumDecimalSepCB(hwndDlg);
		InitNumOfFracSymbCB(hwndDlg);
		InitNumFieldSepCB(hwndDlg);
		InitFieldDigNumCB(hwndDlg);
		InitNegSignCB(hwndDlg);
		InitNegNumFmtCB(hwndDlg);
		InitLeadingZeroesCB(hwndDlg);
		InitListSepCB(hwndDlg);
		InitUnitsSysCB(hwndDlg);
		UpdateNumSamples(hwndDlg, LOCALE_USER_DEFAULT);
	}
    break;
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			case IDC_NUMBERDSYMBOL:
			case IDC_NUMBERSNDIGDEC:
			case IDC_NUMBERSDIGITGRSYM:
			case IDC_NUMBERSDGROUPING:
			case IDC_NUMBERSNSIGNSYM:
			case IDC_NUMBERSNNUMFORMAT:
			case IDC_NUMBERSDISPLEADZER:
			case IDC_NUMBERSLSEP:
			case IDC_NUMBERSMEASSYS:
			if (HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
            {
                /* Set "Apply" button enabled */
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
		}
	}
	break;
	case WM_NOTIFY:
	{
		LPNMHDR lpnm = (LPNMHDR)lParam;
		/* If push apply button */
		if (lpnm->code == (UINT)PSN_APPLY)
		{
			if(!SetNumDecimalSep(hwndDlg)) break;
            if (!SetFracSymNum(hwndDlg))    break;
            if (!SetNumFieldSep(hwndDlg))   break;
            if (!SetFieldDigNum(hwndDlg))   break;
            if (!SetNumNegSign(hwndDlg))    break;
            if (!SetNegSumFmt(hwndDlg))     break;
            if (!SetNumLeadZero(hwndDlg))   break;
            if (!SetNumListSep(hwndDlg))    break;
            if (!SetNumUnitsSys(hwndDlg))   break;
			
			UpdateNumSamples(hwndDlg, LOCALE_USER_DEFAULT);
		}
	}
	break;
  }
  return FALSE;
}

/* EOF */
