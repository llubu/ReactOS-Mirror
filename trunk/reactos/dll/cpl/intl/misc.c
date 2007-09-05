#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "intl.h"
#include "resource.h"

#define NUM_SHEETS           4

/* Insert the space  */
LPTSTR
InsSpacePos(LPCTSTR szInsStr, const int nPos)
{
    LPTSTR pszDestStr;
    INT nDestStrCnt = 0;
    INT nStrCnt;
    INT nStrSize;

    pszDestStr = (LPTSTR)malloc(MAX_SAMPLES_STR_SIZE * sizeof(TCHAR));

    _tcscpy(pszDestStr, szInsStr);

    nStrSize = _tcslen(szInsStr);

    for (nStrCnt = 0; nStrCnt < nStrSize; nStrCnt++)
    {
        if (nStrCnt == nStrSize - nPos)
        {
            pszDestStr[nDestStrCnt] = _T(' ');
            nDestStrCnt++;
        }

        pszDestStr[nDestStrCnt] = szInsStr[nStrCnt];
        nDestStrCnt++;
    }

    pszDestStr[nDestStrCnt] = _T('\0');

    return pszDestStr;
}

/* Insert the spaces by format string separated by ';' */
LPTSTR
InsSpacesFmt(LPCTSTR szSourceStr, LPCTSTR szFmtStr)
{
    LPTSTR pszDestStr;
    LPTSTR pszTempStr;
    TCHAR szFmtVal[255];
    INT nFmtCount = 0;
    INT nValCount = 0;
    INT nLastVal = 0;
    INT nSpaceOffset = 0;
    BOOL wasNul=FALSE;

    pszDestStr = (LPTSTR)malloc(255 * sizeof(TCHAR));

    _tcscpy(pszDestStr, szSourceStr);

    /* if format is clean return source string */
    if (!*szFmtStr)
        return pszDestStr;

    /* Search for all format values */
    for (nFmtCount = 0; nFmtCount <= _tcslen(szFmtStr); nFmtCount++)
    {
        if (szFmtStr[nFmtCount] == _T(';') || szFmtStr[nFmtCount] == _T('\0'))
        {
            if (_ttoi(szFmtVal) == 0 && !wasNul)
            {
                wasNul=TRUE;
                break;
            }

            /* If was 0, repeat spaces */
            if (wasNul)
            {
                nSpaceOffset += nLastVal;
            }
            else
            {
                nSpaceOffset += _ttoi(szFmtVal);
            }

            szFmtVal[nValCount] = _T('\0');
            nValCount=0;

            /* insert space to finded position plus all pos before */
            pszTempStr = InsSpacePos(pszDestStr, nSpaceOffset);
            _tcscpy(pszDestStr,pszTempStr);
            free(pszTempStr);

            /* num of spaces total increment */
            if (!wasNul)
            {
                nSpaceOffset++;
                nLastVal = _ttoi(szFmtVal);
            }
        }
        else
        {
            szFmtVal[nValCount++] = szFmtStr[nFmtCount];
        }
    }

    /* Create spaces for rest part of string */
    if (wasNul && nLastVal != 0)
    {
        for (nFmtCount = nSpaceOffset + nLastVal; nFmtCount < _tcslen(pszDestStr); nFmtCount += nLastVal + 1)
        {
            pszTempStr = InsSpacePos(pszDestStr, nFmtCount);
            _tcscpy(pszDestStr, pszTempStr);
            free(pszTempStr);
        }
    }

    return pszDestStr;
}

/* Replace given template in source string with string to replace and return recieved string */
LPTSTR
ReplaceSubStr(LPCTSTR szSourceStr,
              LPCTSTR szStrToReplace,
              LPCTSTR szTempl)
{
    LPTSTR szDestStr;
    INT nCharCnt;
    INT nSubStrCnt;
    INT nDestStrCnt;
    INT nFirstCharCnt;

    szDestStr = (LPTSTR)malloc(MAX_SAMPLES_STR_SIZE * sizeof(TCHAR));

    nDestStrCnt = 0;
    nFirstCharCnt = 0;

    _tcscpy(szDestStr, _T(""));

    while (nFirstCharCnt < _tcslen(szSourceStr))
    {
        if (szSourceStr[nFirstCharCnt] == szTempl[0])
        {
            nSubStrCnt = 0;
            for (nCharCnt = nFirstCharCnt; nCharCnt < nFirstCharCnt + _tcslen(szTempl); nCharCnt++)
            {
                if (szSourceStr[nCharCnt] == szTempl[nSubStrCnt])
                {
                    nSubStrCnt++;
                }
                else
                {
                    break;
                }

                if (_tcslen(szTempl) == nSubStrCnt)
                {
                    _tcscat(szDestStr, szStrToReplace);
                    nDestStrCnt = _tcslen(szDestStr);
                    nFirstCharCnt += _tcslen(szTempl) - 1;
                    break;
                }
            }
        }
        else
        {
            szDestStr[nDestStrCnt++] = szSourceStr[nFirstCharCnt];
            szDestStr[nDestStrCnt] = _T('\0');
        }

        nFirstCharCnt++;
    }

    return szDestStr;
}

/* Create applets */
LONG
APIENTRY
SetupApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
    PROPSHEETPAGE PsPage[NUM_SHEETS];
    PROPSHEETHEADER psh;
    TCHAR Caption[MAX_STR_SIZE];

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(hwnd);

    LoadString(hApplet, IDS_CUSTOMIZE_TITLE, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_USECALLBACK | PSH_PROPTITLE;
    psh.hwndParent = NULL;
    psh.hInstance = hApplet;
    psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON));
    psh.pszCaption = Caption;
    psh.nPages = sizeof(PsPage) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = PsPage;

    InitPropSheetPage(&PsPage[0], IDD_NUMBERSPAGE, NumbersPageProc);
    InitPropSheetPage(&PsPage[1], IDD_CURRENCYPAGE, CurrencyPageProc);
    InitPropSheetPage(&PsPage[2], IDD_TIMEPAGE, TimePageProc);
    InitPropSheetPage(&PsPage[3], IDD_DATEPAGE, DatePageProc);

    return (LONG)(PropertySheet(&psh) != -1);
}

/* EOF */
