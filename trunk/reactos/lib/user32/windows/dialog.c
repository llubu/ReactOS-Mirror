/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: dialog.c,v 1.15 2003/08/09 14:25:07 chorns Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/dialog.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      07-26-2003  Code ported from wine
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/
#define __NTAPP__
#include <windows.h>
#include <string.h>
#include <user32.h>
#include <ntos/rtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <debug.h>

#include "user32/regcontrol.h"
#include "../controls/controls.h"


/* MACROS/DEFINITIONS ********************************************************/

#define DF_END  0x0001
#define DF_OWNERENABLED 0x0002
#define CW_USEDEFAULT16 ((short)0x8000)

#define GETDLGINFO(hwnd) ((DIALOGINFO*)GetPropA(hwnd, "ROS_DIALOG_INFO"))
#define SETDLGINFO(hwnd, info) (SetPropA(hwnd, "ROS_DIALOG_INFO", info))
#define GET_WORD(ptr)  (*(WORD *)(ptr))
#define GET_DWORD(ptr) (*(DWORD *)(ptr))
#define MAKEINTATOMA(atom)  ((LPCSTR)((ULONG_PTR)((WORD)(atom))))
#define MAKEINTATOMW(atom)  ((LPCWSTR)((ULONG_PTR)((WORD)(atom))))
#define DIALOG_CLASS_ATOMA   MAKEINTATOMA(32770)  /* Dialog */
#define DIALOG_CLASS_ATOMW   MAKEINTATOMW(32770)  /* Dialog */

/* INTERNAL STRUCTS **********************************************************/

/* Dialog info structure */
typedef struct
{
    HWND      hwndFocus;   /* Current control with focus */
    HFONT     hUserFont;   /* Dialog font */
    HMENU     hMenu;       /* Dialog menu */
    UINT      xBaseUnit;   /* Dialog units (depends on the font) */
    UINT      yBaseUnit;
    INT       idResult;    /* EndDialog() result / default pushbutton ID */
    UINT      flags;       /* EndDialog() called for this dialog */
} DIALOGINFO;

/* Dialog control information */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    short      x;
    short      y;
    short      cx;
    short      cy;
    UINT       id;
    LPCWSTR    className;
    LPCWSTR    windowName;
    LPCVOID    data;
} DLG_CONTROL_INFO;

/* Dialog template */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    WORD       nbItems;
    short      x;
    short      y;
    short      cx;
    short      cy;
    LPCWSTR    menuName;
    LPCWSTR    className;
    LPCWSTR    caption;
    WORD       pointSize;
    WORD       weight;
    BOOL       italic;
    LPCWSTR    faceName;
    BOOL       dialogEx;
} DLG_TEMPLATE;

/* GetDlgItem structure */
typedef struct
{
    INT        nIDDlgItem;
    HWND       control;
} GETDLGITEMINFO;


/*********************************************************************
 * dialog class descriptor
 */
const struct builtin_class_descr DIALOG_builtin_class =
{
    DIALOG_CLASS_ATOMA, /* name */
    CS_GLOBALCLASS | CS_SAVEBITS | CS_DBLCLKS, /* style  */
    (WNDPROC) DefDlgProcA,        /* procA */
    (WNDPROC) DefDlgProcW,        /* procW */
    sizeof(DIALOGINFO *),  /* extra */
    (LPCSTR) IDC_ARROW,           /* cursor */
    0                     /* brush */
};


/* INTERNAL FUNCTIONS ********************************************************/

/***********************************************************************
 *           DIALOG_GetCharSize
 *
 * Despite most of MSDN insisting that the horizontal base unit is
 * tmAveCharWidth it isn't.  Knowledge base article Q145994
 * "HOWTO: Calculate Dialog Units When Not Using the System Font",
 * says that we should take the average of the 52 English upper and lower
 * case characters.
 */
BOOL DIALOG_GetCharSize( HDC hDC, HFONT hFont, SIZE * pSize )
{
    HFONT hFontPrev = 0;
    char *alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    SIZE sz;
    TEXTMETRICA tm;

    if(!hDC) return FALSE;

    if(hFont) hFontPrev = SelectObject(hDC, hFont);
    if(!GetTextMetricsA(hDC, &tm)) return FALSE;
    if(!GetTextExtentPointA(hDC, alphabet, 52, &sz)) return FALSE;

    pSize->cy = tm.tmHeight;
    pSize->cx = (sz.cx / 26 + 1) / 2;

    if (hFontPrev) SelectObject(hDC, hFontPrev);

    return TRUE;
}

 /***********************************************************************
 *           DIALOG_EnableOwner
 *
 * Helper function for modal dialogs to enable again the
 * owner of the dialog box.
 */
void DIALOG_EnableOwner( HWND hOwner )
{
    /* Owner must be a top-level window */
    if (hOwner)
        hOwner = GetAncestor( hOwner, GA_ROOT );
    if (!hOwner) return;
        EnableWindow( hOwner, TRUE );
}

 /***********************************************************************
 *           DIALOG_DisableOwner
 *
 * Helper function for modal dialogs to disable the
 * owner of the dialog box. Returns TRUE if owner was enabled.
 */
BOOL DIALOG_DisableOwner( HWND hOwner )
{
    /* Owner must be a top-level window */
    if (hOwner)
        hOwner = GetAncestor( hOwner, GA_ROOT );
    if (!hOwner) return FALSE;
    if (IsWindowEnabled( hOwner ))
    {
        EnableWindow( hOwner, FALSE );
        return TRUE;
    }
    else
        return FALSE;
}

 /***********************************************************************
 *           DIALOG_GetControl32
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static const WORD *DIALOG_GetControl32( const WORD *p, DLG_CONTROL_INFO *info,
                                        BOOL dialogEx )
{
    if (dialogEx)
    {
        info->helpId  = GET_DWORD(p); p += 2;
        info->exStyle = GET_DWORD(p); p += 2;
        info->style   = GET_DWORD(p); p += 2;
    }
    else
    {
        info->helpId  = 0;
        info->style   = GET_DWORD(p); p += 2;
        info->exStyle = GET_DWORD(p); p += 2;
    }
    info->x       = GET_WORD(p); p++;
    info->y       = GET_WORD(p); p++;
    info->cx      = GET_WORD(p); p++;
    info->cy      = GET_WORD(p); p++;
    
    if (dialogEx)
    {
        /* id is a DWORD for DIALOGEX */
        info->id = GET_DWORD(p);
        p += 2;
    }
    else
    {
        info->id = GET_WORD(p);
        p++;
    }
    
    if (GET_WORD(p) == 0xffff)
    {
        static const WCHAR class_names[6][10] =
        {
            { 'B','u','t','t','o','n', },             /* 0x80 */
            { 'E','d','i','t', },                     /* 0x81 */
            { 'S','t','a','t','i','c', },             /* 0x82 */
            { 'L','i','s','t','B','o','x', },         /* 0x83 */
            { 'S','c','r','o','l','l','B','a','r', }, /* 0x84 */
            { 'C','o','m','b','o','B','o','x', }      /* 0x85 */
        };
        WORD id = GET_WORD(p+1);
        /* Windows treats dialog control class ids 0-5 same way as 0x80-0x85 */
        if ((id >= 0x80) && (id <= 0x85)) id -= 0x80;
        if (id <= 5)
            info->className = class_names[id];
        else
        {
            info->className = NULL;
            /* FIXME: load other classes here? */
        }
        p += 2;
    }
    else
    {
        info->className = (LPCWSTR)p;
        p += wcslen( info->className ) + 1;
    }
    
    if (GET_WORD(p) == 0xffff)  /* Is it an integer id? */
    {
        info->windowName = (LPCWSTR)(UINT)GET_WORD(p + 1);
        p += 2;
    }
    else
    {
        info->windowName = (LPCWSTR)p;
        p += wcslen( info->windowName ) + 1;
    }
    
    if (GET_WORD(p))
    {
        info->data = p + 1;
        p += GET_WORD(p) / sizeof(WORD);
    }
    else info->data = NULL;
    p++;
    
    /* Next control is on dword boundary */
    return (const WORD *)((((int)p) + 3) & ~3);    
}

 /***********************************************************************
 *           DIALOG_CreateControls32
 *
 * Create the control windows for a dialog.
 */
static BOOL DIALOG_CreateControls32( HWND hwnd, LPCSTR template, const DLG_TEMPLATE *dlgTemplate,
                                     HINSTANCE hInst, BOOL unicode )
{
    DIALOGINFO * dlgInfo;
    DLG_CONTROL_INFO info;
    HWND hwndCtrl, hwndDefButton = 0;
    INT items = dlgTemplate->nbItems;

    if (!(dlgInfo = GETDLGINFO(hwnd))) return -1;

    while (items--)
    {
        template = (LPCSTR)DIALOG_GetControl32( (WORD *)template, &info,
                                                dlgTemplate->dialogEx );
        /* Is this it? */
        if (info.style & WS_BORDER)
        {
            info.style &= ~WS_BORDER;
            info.exStyle |= WS_EX_CLIENTEDGE;
        }
        if (unicode)
        {
            hwndCtrl = CreateWindowExW( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                        info.className, info.windowName,
                                        info.style | WS_CHILD,
                                        MulDiv(info.x, dlgInfo->xBaseUnit, 4),
                                        MulDiv(info.y, dlgInfo->yBaseUnit, 8),
                                        MulDiv(info.cx, dlgInfo->xBaseUnit, 4),
                                        MulDiv(info.cy, dlgInfo->yBaseUnit, 8),
                                        hwnd, (HMENU)info.id,
                                        hInst, (LPVOID)info.data );
        }
        else
        {
            LPSTR class = (LPSTR)info.className;
            LPSTR caption = (LPSTR)info.windowName;

            if (HIWORD(class))
            {
                DWORD len = WideCharToMultiByte( CP_ACP, 0, info.className, -1, NULL, 0, NULL, NULL );
                class = HeapAlloc( GetProcessHeap(), 0, len );
                WideCharToMultiByte( CP_ACP, 0, info.className, -1, class, len, NULL, NULL );
            }
            if (HIWORD(caption))
            {
                DWORD len = WideCharToMultiByte( CP_ACP, 0, info.windowName, -1, NULL, 0, NULL, NULL );
                caption = HeapAlloc( GetProcessHeap(), 0, len );
                WideCharToMultiByte( CP_ACP, 0, info.windowName, -1, caption, len, NULL, NULL );
            }
            hwndCtrl = CreateWindowExA( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                        class, caption, info.style | WS_CHILD,
                                        MulDiv(info.x, dlgInfo->xBaseUnit, 4),
                                        MulDiv(info.y, dlgInfo->yBaseUnit, 8),
                                        MulDiv(info.cx, dlgInfo->xBaseUnit, 4),
                                        MulDiv(info.cy, dlgInfo->yBaseUnit, 8),
                                        hwnd, (HMENU)info.id,
                                        hInst, (LPVOID)info.data );
            if (HIWORD(class)) HeapFree( GetProcessHeap(), 0, class );
            if (HIWORD(caption)) HeapFree( GetProcessHeap(), 0, caption );
        }
        if (!hwndCtrl)
        {
            if (dlgTemplate->style & DS_NOFAILCREATE) continue;
            return FALSE;
        }

        /* Send initialisation messages to the control */
        if (dlgInfo->hUserFont) SendMessageA( hwndCtrl, WM_SETFONT,
                                             (WPARAM)dlgInfo->hUserFont, 0 );
        if (SendMessageA(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
        {
            /* If there's already a default push-button, set it back */
            /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessageA( hwndDefButton, BM_SETSTYLE, BS_PUSHBUTTON, FALSE );
            hwndDefButton = hwndCtrl;
            dlgInfo->idResult = GetWindowLongA( hwndCtrl, GWL_ID );
        }
    }
    return TRUE;
}

 /***********************************************************************
  *           DIALOG_FindMsgDestination
  *
  * The messages that IsDialogMessage sends may not go to the dialog
  * calling IsDialogMessage if that dialog is a child, and it has the
  * DS_CONTROL style set.
  * We propagate up until we hit one that does not have DS_CONTROL, or
  * whose parent is not a dialog.
  *
  * This is undocumented behaviour.
  */
static HWND DIALOG_FindMsgDestination( HWND hwndDlg )
{
    while (GetWindowLongA(hwndDlg, GWL_STYLE) & DS_CONTROL)
    {
        HWND hParent = GetParent(hwndDlg);
        if (!hParent) break;
        
        if (!IsWindow(hParent)) break;

        if (!GETDLGINFO(hParent)) /* TODO: Correct? */
        {
            break;
        }

        hwndDlg = hParent;
    }

    return hwndDlg;
}

 /***********************************************************************
  *           DIALOG_IsAccelerator
  */
static BOOL DIALOG_IsAccelerator( HWND hwnd, HWND hwndDlg, WPARAM wParam )
{
    HWND hwndControl = hwnd;
    HWND hwndNext;
    INT dlgCode;
    WCHAR buffer[128];

    do
    {
        DWORD style = GetWindowLongW( hwndControl, GWL_STYLE );
        if ((style & (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE)
        {
            dlgCode = SendMessageA( hwndControl, WM_GETDLGCODE, 0, 0 );
            if ( (dlgCode & (DLGC_BUTTON | DLGC_STATIC)) &&
                 GetWindowTextW( hwndControl, buffer, sizeof(buffer)/sizeof(WCHAR) ))
            {
                /* find the accelerator key */
                LPWSTR p = buffer - 2;

                do
                {
                    p = wcschr( p + 2, '&' );
                }
                while (p != NULL && p[1] == '&');

                /* and check if it's the one we're looking for */
				/* FIXME: usage of towupper correct? */
                if (p != NULL && towupper( p[1] ) == towupper( wParam ) )
                {
                    if ((dlgCode & DLGC_STATIC) || (style & 0x0f) == BS_GROUPBOX )
                    {
                        /* set focus to the control */
                        SendMessageA( hwndDlg, WM_NEXTDLGCTL, (WPARAM)hwndControl, 1);
                        /* and bump it on to next */
                        SendMessageA( hwndDlg, WM_NEXTDLGCTL, 0, 0);
                    }
                    else if (dlgCode & DLGC_BUTTON)
                    {
                        /* send BM_CLICK message to the control */
                        SendMessageA( hwndControl, BM_CLICK, 0, 0 );
                    }
                    return TRUE;
                }
            }
            hwndNext = GetWindow( hwndControl, GW_CHILD );
        }
        else hwndNext = 0;

        if (!hwndNext) hwndNext = GetWindow( hwndControl, GW_HWNDNEXT );

        while (!hwndNext && hwndControl)
        {
            hwndControl = GetParent( hwndControl );
            if (hwndControl == hwndDlg)
            {
                if(hwnd==hwndDlg)   /* prevent endless loop */
                {
                    hwndNext=hwnd;
                    break;
                }
                hwndNext = GetWindow( hwndDlg, GW_CHILD );
            }
            else
                hwndNext = GetWindow( hwndControl, GW_HWNDNEXT );
        }
        hwndControl = hwndNext;
    }
    while (hwndControl && (hwndControl != hwnd));

    return FALSE;
}

 /***********************************************************************
 *           DIALOG_DoDialogBox
 */
INT DIALOG_DoDialogBox( HWND hwnd, HWND owner )
{
    DIALOGINFO * dlgInfo;
    MSG msg;
    INT retval;
    HWND ownerMsg = GetAncestor( owner, GA_ROOT );
    if (!(dlgInfo = GETDLGINFO(hwnd))) return -1;

    if (!(dlgInfo->flags & DF_END)) /* was EndDialog called in WM_INITDIALOG ? */
    {
        ShowWindow( hwnd, SW_SHOW );
        for (;;)
        {
            if (!(GetWindowLongW( hwnd, GWL_STYLE ) & DS_NOIDLEMSG))
            {
                if (!PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
                {
                    /* No message present -> send ENTERIDLE and wait */
                    SendMessageW( ownerMsg, WM_ENTERIDLE, MSGF_DIALOGBOX, (LPARAM)hwnd );
                    if (!GetMessageW( &msg, 0, 0, 0 )) break;
                }
            }
            else if (!GetMessageW( &msg, 0, 0, 0 )) break;

            if (!IsWindow( hwnd )) return -1;
            if (!(dlgInfo->flags & DF_END) && !IsDialogMessageW( hwnd, &msg))
            {
                TranslateMessage( &msg );
                DispatchMessageW( &msg );
            }
            if (dlgInfo->flags & DF_END) break;
        }
    }
    if (dlgInfo->flags & DF_OWNERENABLED) DIALOG_EnableOwner( owner );
    retval = dlgInfo->idResult;
    DestroyWindow( hwnd );
    return retval;
}

 /***********************************************************************
 *           DIALOG_ParseTemplate32
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static LPCSTR DIALOG_ParseTemplate32( LPCSTR template, DLG_TEMPLATE * result )
{
    const WORD *p = (const WORD *)template;

    result->style = GET_DWORD(p); p += 2;
    if (result->style == 0xffff0001)  /* DIALOGEX resource */
    {
        result->dialogEx = TRUE;
        result->helpId   = GET_DWORD(p); p += 2;
        result->exStyle  = GET_DWORD(p); p += 2;
        result->style    = GET_DWORD(p); p += 2;
    }
    else
    {
        result->dialogEx = FALSE;
        result->helpId   = 0;
        result->exStyle  = GET_DWORD(p); p += 2;
    }
    result->nbItems = GET_WORD(p); p++;
    result->x       = GET_WORD(p); p++;
    result->y       = GET_WORD(p); p++;
    result->cx      = GET_WORD(p); p++;
    result->cy      = GET_WORD(p); p++;

    /* Get the menu name */

    switch(GET_WORD(p))
    {
        case 0x0000:
            result->menuName = NULL;
            p++;
            break;
        case 0xffff:
            result->menuName = (LPCWSTR)(UINT)GET_WORD( p + 1 );
            p += 2;
            break;
        default:
            result->menuName = (LPCWSTR)p;
            p += wcslen( result->menuName ) + 1;
            break;
    }
        
    /* Get the class name */
        
    switch(GET_WORD(p))
    {
        case 0x0000:
            result->className = DIALOG_CLASS_ATOMW;
            p++;
            break;
        case 0xffff:
            result->className = (LPCWSTR)(UINT)GET_WORD( p + 1 );
            p += 2;
            break;
        default:
            result->className = (LPCWSTR)p;
            p += wcslen( result->className ) + 1;
            break;
    }
    
    /* Get the window caption */

    result->caption = (LPCWSTR)p;
    p += wcslen( result->caption ) + 1;

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
        result->pointSize = GET_WORD(p);
        p++;
        if (result->dialogEx)
        {
            result->weight = GET_WORD(p); p++;
            result->italic = LOBYTE(GET_WORD(p)); p++;
        }
        else
        {
            result->weight = FW_DONTCARE;
            result->italic = FALSE;
        }
        result->faceName = (LPCWSTR)p;
        p += wcslen( result->faceName ) + 1;
    }

    /* First control is on dword boundary */
    return (LPCSTR)((((int)p) + 3) & ~3);
}

 /***********************************************************************
 *           DIALOG_CreateIndirect
 *       Creates a dialog box window
 *
 *       modal = TRUE if we are called from a modal dialog box.
 *       (it's more compatible to do it here, as under Windows the owner
 *       is never disabled if the dialog fails because of an invalid template)
 */
static HWND DIALOG_CreateIndirect( HINSTANCE hInst, LPCVOID dlgTemplate,
                                   HWND owner, DLGPROC dlgProc, LPARAM param,
                                   BOOL unicode, BOOL modal )
{
    HWND hwnd;
    RECT rect;
    DLG_TEMPLATE template;
    DIALOGINFO * dlgInfo;
    DWORD units = GetDialogBaseUnits();
    BOOL ownerEnabled = TRUE;

    /* Parse dialog template */

    if (!dlgTemplate) return 0;
    dlgTemplate = DIALOG_ParseTemplate32( dlgTemplate, &template );

    /* Initialise dialog extra data */
    
    if (!(dlgInfo = HeapAlloc( GetProcessHeap(), 0, sizeof(*dlgInfo) ))) return 0;
    dlgInfo->hwndFocus   = 0;
    dlgInfo->hUserFont   = 0;
    dlgInfo->hMenu       = 0;
    dlgInfo->xBaseUnit   = LOWORD(units);
    dlgInfo->yBaseUnit   = HIWORD(units);
    dlgInfo->idResult    = 0;
    dlgInfo->flags       = 0;
    //dlgInfo->hDialogHeap = 0;
    
    /* Load menu */

    if (template.menuName) dlgInfo->hMenu = LoadMenuW( hInst, template.menuName );
    
    /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
        /* We convert the size to pixels and then make it -ve.  This works
        * for both +ve and -ve template.pointSize */
        HDC dc;
        int pixels;
        dc = GetDC(0);
        pixels = MulDiv(template.pointSize, GetDeviceCaps(dc , LOGPIXELSY), 72);
        dlgInfo->hUserFont = CreateFontW( -pixels, 0, 0, 0, template.weight,
                                            template.italic, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
                                            PROOF_QUALITY, FF_DONTCARE,
                                            template.faceName );
        if (dlgInfo->hUserFont)
        {
            SIZE charSize;
            if (DIALOG_GetCharSize( dc, dlgInfo->hUserFont, &charSize ))
            {
                dlgInfo->xBaseUnit = charSize.cx;
                dlgInfo->yBaseUnit = charSize.cy;
            }
        }
        ReleaseDC(0, dc);
    }

    /* Create dialog main window */

    rect.left = rect.top = 0;
    rect.right = MulDiv(template.cx, dlgInfo->xBaseUnit, 4);
    rect.bottom =  MulDiv(template.cy, dlgInfo->yBaseUnit, 8);
    if (template.style & DS_MODALFRAME)
        template.exStyle |= WS_EX_DLGMODALFRAME;
    AdjustWindowRectEx( &rect, template.style, (dlgInfo->hMenu != 0), template.exStyle );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    if (template.x == CW_USEDEFAULT16)
    {
        rect.left = rect.top = CW_USEDEFAULT;
    }
    else
    {
        if (template.style & DS_CENTER)
        {
            rect.left = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
            rect.top = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2;
        }
        else
        {
            rect.left += MulDiv(template.x, dlgInfo->xBaseUnit, 4);
            rect.top += MulDiv(template.y, dlgInfo->yBaseUnit, 8);
        }
        if ( !(template.style & WS_CHILD) )
        {
            INT dX, dY;

            if( !(template.style & DS_ABSALIGN) )
                ClientToScreen( owner, (POINT *)&rect );

            /* try to fit it into the desktop */

            if( (dX = rect.left + rect.right + GetSystemMetrics(SM_CXDLGFRAME)
                   - GetSystemMetrics(SM_CXSCREEN)) > 0 ) rect.left -= dX;
            if( (dY = rect.top + rect.bottom + GetSystemMetrics(SM_CYDLGFRAME)
                   - GetSystemMetrics(SM_CYSCREEN)) > 0 ) rect.top -= dY;
            if( rect.left < 0 ) rect.left = 0;
            if( rect.top < 0 ) rect.top = 0;
        }
    }
    
    if (modal)
    {
        ownerEnabled = DIALOG_DisableOwner( owner );
        if (ownerEnabled) dlgInfo->flags |= DF_OWNERENABLED;
    }

    if (unicode)
    {
        hwnd = CreateWindowExW(template.exStyle, template.className, template.caption,
                               template.style & ~WS_VISIBLE,
                               rect.left, rect.top, rect.right, rect.bottom,
                               owner, dlgInfo->hMenu, hInst, NULL );
    }
    else
    {
        LPSTR class = (LPSTR)template.className;
        LPSTR caption = (LPSTR)template.caption;

        if (HIWORD(class))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, template.className, -1, NULL, 0, NULL, NULL );
            class = HeapAlloc( GetProcessHeap(), 0, len );
            WideCharToMultiByte( CP_ACP, 0, template.className, -1, class, len, NULL, NULL );
        }
        if (HIWORD(caption))
        {
            DWORD len = WideCharToMultiByte( CP_ACP, 0, template.caption, -1, NULL, 0, NULL, NULL );
            caption = HeapAlloc( GetProcessHeap(), 0, len );
            WideCharToMultiByte( CP_ACP, 0, template.caption, -1, caption, len, NULL, NULL );
        }
        hwnd = CreateWindowExA(template.exStyle, class, caption,
                               template.style & ~WS_VISIBLE,
                               rect.left, rect.top, rect.right, rect.bottom,
                               owner, dlgInfo->hMenu, hInst, NULL );
        if (HIWORD(class)) HeapFree( GetProcessHeap(), 0, class );
        if (HIWORD(caption)) HeapFree( GetProcessHeap(), 0, caption );
    }    

    if (!hwnd)
    {
        if (dlgInfo->hUserFont) DeleteObject( dlgInfo->hUserFont );
        if (dlgInfo->hMenu) DestroyMenu( dlgInfo->hMenu );
        if (modal && (dlgInfo->flags & DF_OWNERENABLED)) DIALOG_EnableOwner(owner);
        HeapFree( GetProcessHeap(), 0, dlgInfo );
        return 0;
    }

    if (template.helpId)
        SetWindowContextHelpId( hwnd, template.helpId );

    if (unicode)
    {
        SETDLGINFO(hwnd, dlgInfo); /* maybe SetPropW? */
        SetWindowLongW( hwnd, DWL_DLGPROC, (LONG)dlgProc );
    }
    else
    {
        SETDLGINFO(hwnd, dlgInfo);
        SetWindowLongA( hwnd, DWL_DLGPROC, (LONG)dlgProc );
    }
    
    if (dlgInfo->hUserFont)
        SendMessageA( hwnd, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0 );
    
    /* Create controls */
    
    if (DIALOG_CreateControls32( hwnd, dlgTemplate, &template, hInst, unicode ))
    {
        HWND hwndPreInitFocus;

        /* Send initialisation messages and set focus */

       dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE );

        hwndPreInitFocus = GetFocus();
        if (SendMessageA( hwnd, WM_INITDIALOG, (WPARAM)dlgInfo->hwndFocus, param ))
        {
            /* check where the focus is again,
             * some controls status might have changed in WM_INITDIALOG */
            dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE);
            if( dlgInfo->hwndFocus )
                SetFocus( dlgInfo->hwndFocus );
        }
        else
        {
            /* If the dlgproc has returned FALSE (indicating handling of keyboard focus)
               but the focus has not changed, set the focus where we expect it. */
            if ((GetFocus() == hwndPreInitFocus) &&
                (GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE))
            {
                dlgInfo->hwndFocus = GetNextDlgTabItem( hwnd, 0, FALSE);
                if( dlgInfo->hwndFocus )
                    SetFocus( dlgInfo->hwndFocus );
            }
        }

        if (template.style & WS_VISIBLE && !(GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE))
        {
           ShowWindow( hwnd, SW_SHOWNORMAL );   /* SW_SHOW doesn't always work */
        }
        return hwnd;
    }
    
    if( IsWindow(hwnd) ) DestroyWindow( hwnd );
    if (modal && ownerEnabled) DIALOG_EnableOwner(owner);
    return 0;
}

/***********************************************************************
 *           DEFDLG_SetFocus
 *
 * Set the focus to a control of the dialog, selecting the text if
 * the control is an edit dialog.
 */
static void DEFDLG_SetFocus( HWND hwndDlg, HWND hwndCtrl )
{
    HWND hwndPrev = GetFocus();

    if (IsChild( hwndDlg, hwndPrev ))
    {
        if (SendMessageW( hwndPrev, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
            SendMessageW( hwndPrev, EM_SETSEL, -1, 0 );
    }
    if (SendMessageW( hwndCtrl, WM_GETDLGCODE, 0, 0 ) & DLGC_HASSETSEL)
        SendMessageW( hwndCtrl, EM_SETSEL, 0, -1 );
    SetFocus( hwndCtrl );
}

/***********************************************************************
 *           DEFDLG_SaveFocus
 */
static void DEFDLG_SaveFocus( HWND hwnd )
{
    DIALOGINFO *infoPtr;	
    HWND hwndFocus = GetFocus();

    if (!hwndFocus || !IsChild( hwnd, hwndFocus )) return;
	if (!(infoPtr = GETDLGINFO(hwnd))) return;
    infoPtr->hwndFocus = hwndFocus;
    /* Remove default button */
}

/***********************************************************************
 *           DEFDLG_RestoreFocus
 */
static void DEFDLG_RestoreFocus( HWND hwnd )
{
    DIALOGINFO *infoPtr;

    if (IsIconic( hwnd )) return;
    if (!(infoPtr = GETDLGINFO(hwnd))) return;
    if (!IsWindow( infoPtr->hwndFocus )) return;
    /* Don't set the focus back to controls if EndDialog is already called.*/
    if (!(infoPtr->flags & DF_END))
    {
        DEFDLG_SetFocus( hwnd, infoPtr->hwndFocus );
        return;
    }
    /* This used to set infoPtr->hwndFocus to NULL for no apparent reason,
       sometimes losing focus when receiving WM_SETFOCUS messages. */
}

/***********************************************************************
 *           DEFDLG_FindDefButton
 *
 * Find the current default push-button.
 */
static HWND DEFDLG_FindDefButton( HWND hwndDlg )
{
    HWND hwndChild = GetWindow( hwndDlg, GW_CHILD );
    while (hwndChild)
    {
        if (SendMessageW( hwndChild, WM_GETDLGCODE, 0, 0 ) & DLGC_DEFPUSHBUTTON)
            break;
        hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
    }
    return hwndChild;
}

/***********************************************************************
 *           DEFDLG_SetDefButton
 *
 * Set the new default button to be hwndNew.
 */
static BOOL DEFDLG_SetDefButton( HWND hwndDlg, DIALOGINFO *dlgInfo,
                                 HWND hwndNew )
{
    DWORD dlgcode=0; /* initialize just to avoid a warning */
    if (hwndNew &&
        !((dlgcode=SendMessageW(hwndNew, WM_GETDLGCODE, 0, 0 ))
            & (DLGC_UNDEFPUSHBUTTON | DLGC_BUTTON)))
        return FALSE;  /* Destination is not a push button */

    if (dlgInfo->idResult)  /* There's already a default pushbutton */
    {
        HWND hwndOld = GetDlgItem( hwndDlg, dlgInfo->idResult );
        if (SendMessageA( hwndOld, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
            SendMessageA( hwndOld, BM_SETSTYLE, BS_PUSHBUTTON, TRUE );
    }
    if (hwndNew)
    {
        if(dlgcode==DLGC_UNDEFPUSHBUTTON)
            SendMessageA( hwndNew, BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE );
        dlgInfo->idResult = GetDlgCtrlID( hwndNew );
    }
    else dlgInfo->idResult = 0;
    return TRUE;
}

/***********************************************************************
 *           DEFDLG_Proc
 *
 * Implementation of DefDlgProc(). Only handle messages that need special
 * handling for dialogs.
 */
static LRESULT DEFDLG_Proc( HWND hwnd, UINT msg, WPARAM wParam,
                            LPARAM lParam, DIALOGINFO *dlgInfo )
{
    switch(msg)
    {
        case WM_ERASEBKGND:
        {
            HBRUSH brush = (HBRUSH)SendMessageW( hwnd, WM_CTLCOLORDLG, wParam, (LPARAM)hwnd );
            if (!brush) brush = (HBRUSH)DefWindowProcW( hwnd, WM_CTLCOLORDLG, wParam, (LPARAM)hwnd );
            if (brush)
            {
                RECT rect;
                HDC hdc = (HDC)wParam;
                GetClientRect( hwnd, &rect );
                DPtoLP( hdc, (LPPOINT)&rect, 2 );
                FillRect( hdc, &rect, brush );
            }
            return 1;
        }
        case WM_NCDESTROY:
            if ((dlgInfo = GETDLGINFO(hwnd)))
            {
                /* Free dialog heap (if created) */
                /*if (dlgInfo->hDialogHeap)
                {
                    GlobalUnlock16(dlgInfo->hDialogHeap);
                    GlobalFree16(dlgInfo->hDialogHeap);
                }*/
                if (dlgInfo->hUserFont) DeleteObject( dlgInfo->hUserFont );
                if (dlgInfo->hMenu) DestroyMenu( dlgInfo->hMenu );                
                HeapFree( GetProcessHeap(), 0, dlgInfo );
            }
             /* Window clean-up */
            return DefWindowProcA( hwnd, msg, wParam, lParam );

			case WM_SHOWWINDOW:
            if (!wParam) DEFDLG_SaveFocus( hwnd );
            return DefWindowProcA( hwnd, msg, wParam, lParam );

        case WM_ACTIVATE:
            if (wParam) DEFDLG_RestoreFocus( hwnd );
            else DEFDLG_SaveFocus( hwnd );
            return 0;

        case WM_SETFOCUS:
            DEFDLG_RestoreFocus( hwnd );
            return 0;

        case DM_SETDEFID:
            if (dlgInfo && !(dlgInfo->flags & DF_END))
                DEFDLG_SetDefButton( hwnd, dlgInfo, wParam ? GetDlgItem( hwnd, wParam ) : 0 );
            return 1;

        case DM_GETDEFID:
            if (dlgInfo && !(dlgInfo->flags & DF_END))
            {
                HWND hwndDefId;
                if (dlgInfo->idResult)
                    return MAKELONG( dlgInfo->idResult, DC_HASDEFID );
                if ((hwndDefId = DEFDLG_FindDefButton( hwnd )))
                    return MAKELONG( GetDlgCtrlID( hwndDefId ), DC_HASDEFID);
            }
            return 0;

        case WM_NEXTDLGCTL:
            if (dlgInfo)
            {
                HWND hwndDest = (HWND)wParam;
                if (!lParam)
                    hwndDest = GetNextDlgTabItem(hwnd, GetFocus(), wParam);
                if (hwndDest) DEFDLG_SetFocus( hwnd, hwndDest );
                DEFDLG_SetDefButton( hwnd, dlgInfo, hwndDest );
            }
            return 0;

        case WM_ENTERMENULOOP:
        case WM_LBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
            {
                HWND hwndFocus = GetFocus();
                if (hwndFocus)
                {
                    /* always make combo box hide its listbox control */
                    if (!SendMessageA( hwndFocus, CB_SHOWDROPDOWN, FALSE, 0 ))
                        SendMessageA( GetParent(hwndFocus), CB_SHOWDROPDOWN, FALSE, 0 );
                }
            }
            return DefWindowProcA( hwnd, msg, wParam, lParam );

        case WM_GETFONT:
            return dlgInfo ? (LRESULT)dlgInfo->hUserFont : 0;

        case WM_CLOSE:
            PostMessageA( hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED),
                            (LPARAM)GetDlgItem( hwnd, IDCANCEL ) );
            return 0;

        case WM_NOTIFYFORMAT:
             return DefWindowProcA( hwnd, msg, wParam, lParam );
    }
    return 0;
}

/***********************************************************************
 *           DEFDLG_Epilog
 */
static LRESULT DEFDLG_Epilog(HWND hwnd, UINT msg, BOOL fResult)
{
	// TODO: where's wine's WM_CTLCOLOR from?
    if ((msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) ||
         /*msg == WM_CTLCOLOR || */ msg == WM_COMPAREITEM ||
         msg == WM_VKEYTOITEM || msg == WM_CHARTOITEM ||
         msg == WM_QUERYDRAGICON || msg == WM_INITDIALOG)
        return fResult;

    return GetWindowLongA( hwnd, DWL_MSGRESULT );
}

/***********************************************************************
 *           DIALOG_GetNextTabItem
 *
 * Helper for GetNextDlgTabItem
 */
static HWND DIALOG_GetNextTabItem( HWND hwndMain, HWND hwndDlg, HWND hwndCtrl, BOOL fPrevious )
{
    LONG dsStyle;
    LONG exStyle;
    UINT wndSearch = fPrevious ? GW_HWNDPREV : GW_HWNDNEXT;
    HWND retWnd = 0;
    HWND hChildFirst = 0;

    if(!hwndCtrl)
    {
        hChildFirst = GetWindow(hwndDlg,GW_CHILD);
        if(fPrevious) hChildFirst = GetWindow(hChildFirst,GW_HWNDLAST);
    }
    else if (IsChild( hwndMain, hwndCtrl ))
    {
        hChildFirst = GetWindow(hwndCtrl,wndSearch);
        if(!hChildFirst)
        {
            if(GetParent(hwndCtrl) != hwndMain)
                hChildFirst = GetWindow(GetParent(hwndCtrl),wndSearch);
            else
            {
                if(fPrevious)
                    hChildFirst = GetWindow(hwndCtrl,GW_HWNDLAST);
                else
                    hChildFirst = GetWindow(hwndCtrl,GW_HWNDFIRST);
            }
        }
    }

    while(hChildFirst)
    {
        BOOL bCtrl = FALSE;
        while(hChildFirst)
        {
            dsStyle = GetWindowLongA(hChildFirst,GWL_STYLE);
            exStyle = GetWindowLongA(hChildFirst,GWL_EXSTYLE);
            if( (dsStyle & DS_CONTROL || exStyle & WS_EX_CONTROLPARENT) && (dsStyle & WS_VISIBLE) && !(dsStyle & WS_DISABLED))
            {
                bCtrl=TRUE;
                break;
            }
            else if( (dsStyle & WS_TABSTOP) && (dsStyle & WS_VISIBLE) && !(dsStyle & WS_DISABLED))
                break;
            hChildFirst = GetWindow(hChildFirst,wndSearch);
        }
        if(hChildFirst)
        {
            if(bCtrl)
                retWnd = DIALOG_GetNextTabItem(hwndMain,hChildFirst,NULL,fPrevious );
            else
                retWnd = hChildFirst;
        }
        if(retWnd) break;
        hChildFirst = GetWindow(hChildFirst,wndSearch);
    }
    if(!retWnd && hwndCtrl)
    {
        HWND hParent = GetParent(hwndCtrl);
        while(hParent)
        {
            if(hParent == hwndMain) break;
            retWnd = DIALOG_GetNextTabItem(hwndMain,GetParent(hParent),hParent,fPrevious );
            if(retWnd) break;
            hParent = GetParent(hParent);
        }
        if(!retWnd)
            retWnd = DIALOG_GetNextTabItem(hwndMain,hwndMain,NULL,fPrevious );
    }
    return retWnd;
}

/***********************************************************************
 *           GetDlgItemEnumProc
 *
 * Callback for GetDlgItem
 */
BOOL CALLBACK GetDlgItemEnumProc (HWND hwnd, LPARAM lParam )
{
    GETDLGITEMINFO * info = (GETDLGITEMINFO *)lParam;
    if(info->nIDDlgItem == GetWindowLongW( hwnd, GWL_ID ))
    {        
        info->control = hwnd;        
        return FALSE;
    }
    return TRUE;
}


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HWND
STDCALL
CreateDialogIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
	return DIALOG_CreateIndirect( hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit, FALSE, FALSE );
}


/*
 * @unimplemented
 */
HWND
STDCALL
CreateDialogIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
	  /* FIXME:
	 This function might be obsolete since I don't think it is exported by NT
     Also wine has one more parameter identifying weather it should call
	 the function with unicode or not */
	UNIMPLEMENTED;
	return (HWND)0;
}


/*
 * @implemented
 */
HWND
STDCALL
CreateDialogIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  	return DIALOG_CreateIndirect( hInstance, lpTemplate, hWndParent, lpDialogFunc, lParamInit, TRUE, FALSE );
}


/*
 * @implemented
 */
HWND
STDCALL
CreateDialogParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
	HRSRC hrsrc;
	LPCDLGTEMPLATE ptr;

    if (!(hrsrc = FindResourceA( hInstance, lpTemplateName, (LPCSTR)RT_DIALOG ))) return 0;
    if (!(ptr = (LPCDLGTEMPLATE)LoadResource(hInstance, hrsrc))) return 0;
	return CreateDialogIndirectParamA( hInstance, ptr, hWndParent, lpDialogFunc, dwInitParam );
}


/*
 * @implemented
 */
HWND
STDCALL
CreateDialogParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
	HRSRC hrsrc;
	LPCDLGTEMPLATE ptr;

	if (!(hrsrc = FindResourceW( hInstance, lpTemplateName, (LPCWSTR)RT_DIALOG ))) return 0;
	if (!(ptr = (LPCDLGTEMPLATE)LoadResource(hInstance, hrsrc))) return 0;
	return CreateDialogIndirectParamW( hInstance, ptr, hWndParent, lpDialogFunc, dwInitParam );
}


/*
 * @implemented
 */
LRESULT
STDCALL
DefDlgProcA(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
    WNDPROC dlgproc;
    BOOL result = FALSE;
    DIALOGINFO * dlgInfo;

	/* if there's no dialog info property then call default windows proc?? */
    if (!(dlgInfo = GETDLGINFO(hDlg))) 
	    return DefWindowProcA( hDlg, Msg, wParam, lParam );

    SetWindowLongW( hDlg, DWL_MSGRESULT, 0 );

    if ((dlgproc = (WNDPROC)GetWindowLongA( hDlg, DWL_DLGPROC )))
    {
        /* Call dialog procedure */
        result = CallWindowProcA( dlgproc, hDlg, Msg, wParam, lParam );
    }

    if (!result && IsWindow(hDlg))
    {
        /* callback didn't process this message */

        switch(Msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
            case WM_ENTERMENULOOP:
            case WM_LBUTTONDOWN:
            case WM_NCLBUTTONDOWN:
                 return DEFDLG_Proc( hDlg, Msg, wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProcA( hDlg, Msg, wParam, lParam );
        }
    }
    return DEFDLG_Epilog(hDlg, Msg, result);
}


/*
 * @implemented
 */
LRESULT
STDCALL
DefDlgProcW(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
    WNDPROC dlgproc;
    BOOL result = FALSE;
    DIALOGINFO * dlgInfo;

	/* if there's no dialog info property then call default windows proc?? */
    if (!(dlgInfo = GETDLGINFO(hDlg))) 
	    return DefWindowProcW( hDlg, Msg, wParam, lParam );

    SetWindowLongW( hDlg, DWL_MSGRESULT, 0 );

    if ((dlgproc = (WNDPROC)GetWindowLongW( hDlg, DWL_DLGPROC )))
    {
        /* Call dialog procedure */
        result = CallWindowProcW( dlgproc, hDlg, Msg, wParam, lParam );
    }

    if (!result && IsWindow(hDlg))
    {
        /* callback didn't process this message */

        switch(Msg)
        {
            case WM_ERASEBKGND:
            case WM_SHOWWINDOW:
            case WM_ACTIVATE:
            case WM_SETFOCUS:
            case DM_SETDEFID:
            case DM_GETDEFID:
            case WM_NEXTDLGCTL:
            case WM_GETFONT:
            case WM_CLOSE:
            case WM_NCDESTROY:
            case WM_ENTERMENULOOP:
            case WM_LBUTTONDOWN:
            case WM_NCLBUTTONDOWN:
                 return DEFDLG_Proc( hDlg, Msg, wParam, lParam, dlgInfo );
            case WM_INITDIALOG:
            case WM_VKEYTOITEM:
            case WM_COMPAREITEM:
            case WM_CHARTOITEM:
                 break;

            default:
                 return DefWindowProcW( hDlg, Msg, wParam, lParam );
        }
    }
    return DEFDLG_Epilog(hDlg, Msg, result);
}


/*
 * @implemented
 */
INT_PTR
STDCALL
DialogBoxIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
    HWND hwnd = DIALOG_CreateIndirect( hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam, FALSE, TRUE );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, hWndParent );
    return -1;
}


/*
 * @unimplemented
 */
INT_PTR
STDCALL
DialogBoxIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  /* FIXME:
	 This function might be obsolete since I don't think it is exported by NT
     Also wine has one more parameter identifying weather it should call
	 the function with unicode or not */
  UNIMPLEMENTED;
  return (INT_PTR)NULL;
}


/*
 * @implemented
 */
INT_PTR
STDCALL
DialogBoxIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
    HWND hwnd = DIALOG_CreateIndirect( hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam, TRUE, TRUE );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, hWndParent );
    return -1;
}


/*
 * @implemented
 */
INT_PTR
STDCALL
DialogBoxParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
    HWND hwnd;
    HRSRC hrsrc;
    LPCDLGTEMPLATE ptr;
    
	if (!(hrsrc = FindResourceA( hInstance, lpTemplateName, (LPCSTR)RT_DIALOG ))) return 0;
	if (!(ptr = (LPCDLGTEMPLATE)LoadResource(hInstance, hrsrc))) return 0;
    hwnd = DIALOG_CreateIndirect(hInstance, ptr, hWndParent, lpDialogFunc, dwInitParam, FALSE, TRUE);
    if (hwnd) return DIALOG_DoDialogBox(hwnd, hWndParent);
    return -1;
}


/*
 * @implemented
 */
INT_PTR
STDCALL
DialogBoxParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
    HWND hwnd;
    HRSRC hrsrc;
    LPCDLGTEMPLATE ptr;
    
	if (!(hrsrc = FindResourceW( hInstance, lpTemplateName, (LPCWSTR)RT_DIALOG ))) return 0;
	if (!(ptr = (LPCDLGTEMPLATE)LoadResource(hInstance, hrsrc))) return 0;
    hwnd = DIALOG_CreateIndirect(hInstance, ptr, hWndParent, lpDialogFunc, dwInitParam, TRUE, TRUE);
    if (hwnd) return DIALOG_DoDialogBox(hwnd, hWndParent);
    return -1;
}


/*
 * @unimplemented
 */
int
STDCALL
DlgDirListA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
DlgDirListComboBoxA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
DlgDirListComboBoxW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
DlgDirListW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DlgDirSelectComboBoxExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDComboBox)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DlgDirSelectComboBoxExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDComboBox)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DlgDirSelectExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDListBox)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
DlgDirSelectExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDListBox)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
EndDialog(
  HWND hDlg,
  INT_PTR nResult)
{
    BOOL wasEnabled = TRUE;
    DIALOGINFO * dlgInfo;
    HWND owner;

    if (!(dlgInfo = GETDLGINFO(hDlg))) return FALSE;
    
    dlgInfo->idResult = nResult;
    dlgInfo->flags |= DF_END;
    wasEnabled = (dlgInfo->flags & DF_OWNERENABLED);

    if (wasEnabled && (owner = GetWindow( hDlg, GW_OWNER )))
        DIALOG_EnableOwner( owner );

    /* Windows sets the focus to the dialog itself in EndDialog */

    if (IsChild(hDlg, GetFocus()))
       SetFocus( hDlg );

    /* Don't have to send a ShowWindow(SW_HIDE), just do
       SetWindowPos with SWP_HIDEWINDOW as done in Windows */

    SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE
                 | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);

    /* unblock dialog loop */
    PostMessageA(hDlg, WM_NULL, 0, 0);
    return TRUE;
}


/*
 * @implemented
 */
LONG
STDCALL
GetDialogBaseUnits(VOID)
{
    static DWORD units;

    if (!units)
    {
        HDC hdc;
        SIZE size;

        if ((hdc = GetDC(0)))
        {
            if (DIALOG_GetCharSize( hdc, 0, &size )) units = MAKELONG( size.cx, size.cy );
            ReleaseDC( 0, hdc );
        }
    }
    return units;
}


/*
 * @implemented
 */
int
STDCALL
GetDlgCtrlID(
  HWND hwndCtl)
{
	return GetWindowLongW( hwndCtl, GWL_ID );
}


/*
 * @implemented
 */
HWND
STDCALL
GetDlgItem(
  HWND hDlg,
  int nIDDlgItem)
{
    GETDLGITEMINFO info;
    info.nIDDlgItem = nIDDlgItem;
    info.control = 0;
    if(hDlg && !EnumChildWindows(hDlg, (ENUMWINDOWSPROC)&GetDlgItemEnumProc, (LPARAM)&info))
        return info.control;
    else
        return 0;
}


/*
 * @implemented
 */
UINT
STDCALL
GetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  WINBOOL *lpTranslated,
  WINBOOL bSigned)
{
	char str[30];
    char * endptr;
    long result = 0;

    if (lpTranslated) *lpTranslated = FALSE;
    if (!SendDlgItemMessageA(hDlg, nIDDlgItem, WM_GETTEXT, sizeof(str), (LPARAM)str))
        return 0;
    if (bSigned)
    {
        result = strtol( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
		/* FIXME: errno? */
        if (((result == LONG_MIN) || (result == LONG_MAX))/* && (errno == ERANGE) */)
            return 0;
    }
    else
    {
        result = strtoul( str, &endptr, 10 );
        if (!endptr || (endptr == str))  /* Conversion was unsuccessful */
            return 0;
		/* FIXME: errno? */
        if ((result == LONG_MAX)/* && (errno == ERANGE) */) return 0;
    }
    if (lpTranslated) *lpTranslated = TRUE;
    return (UINT)result;
}


/*
 * @implemented
 */
UINT
STDCALL
GetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPSTR lpString,
  int nMaxCount)
{
  return (UINT)SendDlgItemMessageA( hDlg, nIDDlgItem, WM_GETTEXT, nMaxCount, (LPARAM)lpString );
}


/*
 * @implemented
 */
UINT
STDCALL
GetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPWSTR lpString,
  int nMaxCount)
{
  return (UINT)SendDlgItemMessageW( hDlg, nIDDlgItem, WM_GETTEXT, nMaxCount, (LPARAM)lpString );
}


/*
 * @implemented
 */
HWND
STDCALL
GetNextDlgGroupItem(
  HWND hDlg,
  HWND hCtl,
  WINBOOL bPrevious)
{
	HWND hwnd, retvalue;

    if(hCtl)
    {
        /* if the hwndCtrl is the child of the control in the hwndDlg,
         * then the hwndDlg has to be the parent of the hwndCtrl */
        if(GetParent(hCtl) != hDlg && GetParent(GetParent(hCtl)) == hDlg)
            hDlg = GetParent(hCtl);
    }

    if (hCtl)
    {
        /* Make sure hwndCtrl is a top-level child */
        HWND parent = GetParent( hCtl );
        while (parent && parent != hDlg) parent = GetParent(parent);
        if (parent != hDlg) return 0;
    }
    else
    {
        /* No ctrl specified -> start from the beginning */
        if (!(hCtl = GetWindow( hDlg, GW_CHILD ))) return 0;
        if (bPrevious) hCtl = GetWindow( hCtl, GW_HWNDLAST );
    }

    retvalue = hCtl;
    hwnd = GetWindow( hCtl, GW_HWNDNEXT );
    while (1)
    {
        if (!hwnd || (GetWindowLongW( hwnd, GWL_STYLE ) & WS_GROUP))
        {
            /* Wrap-around to the beginning of the group */
            HWND tmp;

            hwnd = GetWindow( hDlg, GW_CHILD );
            for (tmp = hwnd; tmp; tmp = GetWindow( tmp, GW_HWNDNEXT ) )
            {
                if (GetWindowLongW( tmp, GWL_STYLE ) & WS_GROUP) hwnd = tmp;
                if (tmp == hCtl) break;
            }
        }
        if (hwnd == hCtl) break;
        if ((GetWindowLongW( hwnd, GWL_STYLE ) & (WS_VISIBLE|WS_DISABLED)) == WS_VISIBLE)
        {
            retvalue = hwnd;
            if (!bPrevious) break;
        }
        hwnd = GetWindow( hwnd, GW_HWNDNEXT );
    }
    return retvalue;
}


/*
 * @implemented
 */
HWND
STDCALL
GetNextDlgTabItem(
  HWND hDlg,
  HWND hCtl,
  WINBOOL bPrevious)
{
	return DIALOG_GetNextTabItem(hDlg, hDlg, hCtl, bPrevious);
}

#if 0
WINBOOL
STDCALL
IsDialogMessage(
  HWND hDlg,
  LPMSG lpMsg)
{
	return IsDialogMessageW(hDlg, lpMsg);
}
#endif


/*
 * @implemented
 */
WINBOOL
STDCALL
IsDialogMessageA(
  HWND hDlg,
  LPMSG lpMsg)
{
     INT dlgCode = 0;
 
     // FIXME: hooks
     //if (CallMsgFilterA( lpMsg, MSGF_DIALOGBOX )) return TRUE;
 
     if ((hDlg != lpMsg->hwnd) && !IsChild( hDlg, lpMsg->hwnd )) return FALSE;
 
     hDlg = DIALOG_FindMsgDestination(hDlg);
 
     switch(lpMsg->message)
     {
     case WM_KEYDOWN:
        dlgCode = SendMessageA( lpMsg->hwnd, WM_GETDLGCODE, lpMsg->wParam, (LPARAM)lpMsg );
         if (dlgCode & DLGC_WANTMESSAGE) break;
 
         switch(lpMsg->wParam)
         {
         case VK_TAB:
             if (!(dlgCode & DLGC_WANTTAB))
             {
                 SendMessageA( hDlg, WM_NEXTDLGCTL, (GetKeyState(VK_SHIFT) & 0x8000), 0 );
                 return TRUE;
             }
             break;
 
         case VK_RIGHT:
         case VK_DOWN:
         case VK_LEFT:
         case VK_UP:
             if (!(dlgCode & DLGC_WANTARROWS))
             {
                 BOOL fPrevious = (lpMsg->wParam == VK_LEFT || lpMsg->wParam == VK_UP);
                 HWND hwndNext = GetNextDlgGroupItem (hDlg, GetFocus(), fPrevious );
                 SendMessageA( hDlg, WM_NEXTDLGCTL, (WPARAM)hwndNext, 1 );
                 return TRUE;
             }
             break;
 
         case VK_CANCEL:
         case VK_ESCAPE:
             SendMessageA( hDlg, WM_COMMAND, IDCANCEL, (LPARAM)GetDlgItem( hDlg, IDCANCEL ) );
             return TRUE;
 
         case VK_EXECUTE:
         case VK_RETURN:
             {
                 DWORD dw = SendMessageA( hDlg, DM_GETDEFID, 0, 0 );
                 if (HIWORD(dw) == DC_HASDEFID)
                 {
                     SendMessageA( hDlg, WM_COMMAND, MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                     (LPARAM)GetDlgItem(hDlg, LOWORD(dw)));
                 }
                 else
                 {
                     SendMessageA( hDlg, WM_COMMAND, IDOK, (LPARAM)GetDlgItem( hDlg, IDOK ) );
 
                 }
             }
             return TRUE;
         }
         break;
 
     case WM_CHAR:
         dlgCode = SendMessageA( lpMsg->hwnd, WM_GETDLGCODE, lpMsg->wParam, (LPARAM)lpMsg );
         if (dlgCode & (DLGC_WANTCHARS|DLGC_WANTMESSAGE)) break;
         if (lpMsg->wParam == '\t' && (dlgCode & DLGC_WANTTAB)) break;
         /* drop through */
 
     case WM_SYSCHAR:
         if (DIALOG_IsAccelerator( lpMsg->hwnd, hDlg, lpMsg->wParam ))
         {
             /* don't translate or dispatch */
             return TRUE;
         }
         break;
     }
 
     TranslateMessage( lpMsg );
     DispatchMessageA( lpMsg );
     return TRUE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
IsDialogMessageW(
  HWND hDlg,
  LPMSG lpMsg)
{
     INT dlgCode = 0;
 
     // FIXME: hooks
     //if (CallMsgFilterW( lpMsg, MSGF_DIALOGBOX )) return TRUE;
 
     if ((hDlg != lpMsg->hwnd) && !IsChild( hDlg, lpMsg->hwnd )) return FALSE;
 
     hDlg = DIALOG_FindMsgDestination(hDlg);
 
     switch(lpMsg->message)
     {
     case WM_KEYDOWN:
        dlgCode = SendMessageW( lpMsg->hwnd, WM_GETDLGCODE, lpMsg->wParam, (LPARAM)lpMsg );
         if (dlgCode & DLGC_WANTMESSAGE) break;
 
         switch(lpMsg->wParam)
         {
         case VK_TAB:
             if (!(dlgCode & DLGC_WANTTAB))
             {
                 SendMessageW( hDlg, WM_NEXTDLGCTL, (GetKeyState(VK_SHIFT) & 0x8000), 0 );
                 return TRUE;
             }
             break;
 
         case VK_RIGHT:
         case VK_DOWN:
         case VK_LEFT:
         case VK_UP:
             if (!(dlgCode & DLGC_WANTARROWS))
             {
                 BOOL fPrevious = (lpMsg->wParam == VK_LEFT || lpMsg->wParam == VK_UP);
                 HWND hwndNext = GetNextDlgGroupItem (hDlg, GetFocus(), fPrevious );
                 SendMessageW( hDlg, WM_NEXTDLGCTL, (WPARAM)hwndNext, 1 );
                 return TRUE;
             }
             break;
 
         case VK_CANCEL:
         case VK_ESCAPE:
             SendMessageW( hDlg, WM_COMMAND, IDCANCEL, (LPARAM)GetDlgItem( hDlg, IDCANCEL ) );
             return TRUE;
 
         case VK_EXECUTE:
         case VK_RETURN:
             {
                 DWORD dw = SendMessageW( hDlg, DM_GETDEFID, 0, 0 );
                 if (HIWORD(dw) == DC_HASDEFID)
                 {
                     SendMessageW( hDlg, WM_COMMAND, MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                     (LPARAM)GetDlgItem(hDlg, LOWORD(dw)));
                 }
                 else
                 {
                     SendMessageW( hDlg, WM_COMMAND, IDOK, (LPARAM)GetDlgItem( hDlg, IDOK ) );
 
                 }
             }
             return TRUE;
         }
         break;
 
     case WM_CHAR:
         dlgCode = SendMessageW( lpMsg->hwnd, WM_GETDLGCODE, lpMsg->wParam, (LPARAM)lpMsg );
         if (dlgCode & (DLGC_WANTCHARS|DLGC_WANTMESSAGE)) break;
         if (lpMsg->wParam == '\t' && (dlgCode & DLGC_WANTTAB)) break;
         /* drop through */
 
     case WM_SYSCHAR:
         if (DIALOG_IsAccelerator( lpMsg->hwnd, hDlg, lpMsg->wParam ))
         {
             /* don't translate or dispatch */
             return TRUE;
         }
         break;
     }
 
     TranslateMessage( lpMsg );
     DispatchMessageW( lpMsg );
     return TRUE;
}


/*
 * @implemented
 */
UINT
STDCALL
IsDlgButtonChecked(
  HWND hDlg,
  int nIDButton)
{
  return (UINT)SendDlgItemMessageA( hDlg, nIDButton, BM_GETCHECK, 0, 0 );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
MapDialogRect(
  HWND hDlg,
  LPRECT lpRect)
{
	DIALOGINFO * dlgInfo;
	if (!(dlgInfo = GETDLGINFO(hDlg))) return FALSE;
	lpRect->left   = MulDiv(lpRect->left, dlgInfo->xBaseUnit, 4);
	lpRect->right  = MulDiv(lpRect->right, dlgInfo->xBaseUnit, 4);
	lpRect->top    = MulDiv(lpRect->top, dlgInfo->yBaseUnit, 8);
	lpRect->bottom = MulDiv(lpRect->bottom, dlgInfo->yBaseUnit, 8);
	return TRUE;
}


/*
 * @implemented
 */
LRESULT
STDCALL
SendDlgItemMessageA(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
	HWND hwndCtrl = GetDlgItem( hDlg, nIDDlgItem );
	if (hwndCtrl) return SendMessageA( hwndCtrl, Msg, wParam, lParam );
	else return 0;
}


/*
 * @implemented
 */
LRESULT
STDCALL
SendDlgItemMessageW(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
	HWND hwndCtrl = GetDlgItem( hDlg, nIDDlgItem );
	if (hwndCtrl) return SendMessageW( hwndCtrl, Msg, wParam, lParam );
	else return 0;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  UINT uValue,
  WINBOOL bSigned)
{
	char str[20];

	if (bSigned) sprintf( str, "%d", (INT)uValue );
	else sprintf( str, "%u", uValue );
	SendDlgItemMessageA( hDlg, nIDDlgItem, WM_SETTEXT, 0, (LPARAM)str );
	return TRUE;
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPCSTR lpString)
{
  return SendDlgItemMessageA( hDlg, nIDDlgItem, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
SetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPCWSTR lpString)
{
  return SendDlgItemMessageW( hDlg, nIDDlgItem, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*
 * @implemented
 */
WINBOOL
STDCALL
CheckDlgButton(
  HWND hDlg,
  int nIDButton,
  UINT uCheck)
{
	SendDlgItemMessageA( hDlg, nIDButton, BM_SETCHECK, uCheck, 0 );
	return TRUE;
}
