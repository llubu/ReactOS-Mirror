/*
 *  ReactOS regedt32
 *
 *  main.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"


#define STATUS_WINDOW   2001
#define TREE_WINDOW     2002
#define LIST_WINDOW     2003

#define MAX_LOADSTRING  100
#define	SPLIT_WIDTH		5
#define MAX_NAME_LEN    500


#define ID_WINDOW_CLOSE                 798
#define ID_WINDOW_CLOSEALL              799

#define	IDW_FIRST_CHILD		0xC000	//0x200

////////////////////////////////////////////////////////////////////////////////

enum OPTION_FLAGS {
    OPTIONS_AUTO_REFRESH               = 0x01,
    OPTIONS_READ_ONLY_MODE             = 0x02,
    OPTIONS_CONFIRM_ON_DELETE          = 0x04,
    OPTIONS_SAVE_ON_EXIT          	   = 0x08,
    OPTIONS_DISPLAY_BINARY_DATA    	   = 0x10,
    OPTIONS_VIEW_TREE_ONLY       	   = 0x20,
    OPTIONS_VIEW_DATA_ONLY      	   = 0x40,
};

typedef struct {
    HWND    hWnd;
    HWND    hTreeWnd;
    HWND    hListWnd;
    int     nFocusPanel;      // 0: left  1: right
	int		nSplitPos;
    WINDOWPLACEMENT pos;
	TCHAR	szKeyName[MAX_PATH];
    HKEY    hKey;
} ChildWnd;

////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//
extern HINSTANCE hInst;
extern HACCEL    hAccel;
extern HWND      hFrameWnd;
extern HMENU     hMenuFrame;
extern HWND      hMDIClient;
extern HWND      hStatusBar;
//extern HWND      hToolBar;
extern HFONT     hFont;
extern enum OPTION_FLAGS Options;

extern TCHAR szTitle[];
extern TCHAR szFrameClass[];
extern TCHAR szChildClass[];

#if __MINGW32_MAJOR_VERSION == 1
/*
typedef struct tagNMITEMACTIVATE{
    NMHDR   hdr;
    int     iItem;
    int     iSubItem;
    UINT    uNewState;
    UINT    uOldState;
    UINT    uChanged;
    POINT   ptAction;
    LPARAM  lParam;
    UINT    uKeyFlags;
} NMITEMACTIVATE, FAR *LPNMITEMACTIVATE;
 */
#endif

#ifdef __cplusplus
};
#endif

#endif // __MAIN_H__
