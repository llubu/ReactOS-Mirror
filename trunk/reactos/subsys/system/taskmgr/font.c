/*
 *  ReactOS Task Manager
 *
 *  font.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
    
#define WIN32_LEAN_AND_MEAN    /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
    
#include "taskmgr.h"
#include "font.h"

void Font_DrawText(HDC hDC, LPCTSTR lpszText, int x, int y)
{
    HDC        hFontDC;
    HBITMAP    hFontBitmap;
    HBITMAP    hOldBitmap;
    int        i;
    
    hFontDC = CreateCompatibleDC(hDC);
    hFontBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FONT));
    hOldBitmap = (HBITMAP)SelectObject(hFontDC, hFontBitmap);
    
    for (i = 0; i < (int)_tcslen(lpszText); i++) {
        if ((lpszText[i] >= '0') && (lpszText[i] <= '9')) {
            BitBlt(hDC, x + (i * 8), y, 8, 11, hFontDC, (lpszText[i] - '0') * 8, 0, SRCCOPY);
        }
        else if (lpszText[i] == 'K')
        {
            BitBlt(hDC, x + (i * 8), y, 8, 11, hFontDC, 80, 0, SRCCOPY);
        }
        else if (lpszText[i] == '%')
        {
            BitBlt(hDC, x + (i * 8), y, 8, 11, hFontDC, 88, 0, SRCCOPY);
        }
    }
    SelectObject(hFontDC, hOldBitmap);
    DeleteObject(hFontBitmap);
    DeleteDC(hFontDC);
}

