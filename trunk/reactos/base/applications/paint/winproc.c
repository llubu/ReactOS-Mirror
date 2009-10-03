/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        winproc.c
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>
#include <commctrl.h>
//#include <htmlhelp.h>
#include <stdio.h>
#include <tchar.h>
#include "definitions.h"
#include "globalvar.h"
#include "dialogs.h"
#include "dib.h"
#include "drawing.h"
#include "history.h"
#include "mouse.h"
#include "registry.h"

/* FUNCTIONS ********************************************************/

void selectTool(int tool)
{
    ShowWindow(hSelection, SW_HIDE);
    activeTool = tool;
    pointSP = 0; // resets the point-buffer of the polygon and bezier functions
    SendMessage(hToolSettings, WM_PAINT, 0, 0);
    if (tool==6)
        ShowWindow(hTrackbarZoom, SW_SHOW);
    else
        ShowWindow(hTrackbarZoom, SW_HIDE);
}

void updateCanvasAndScrollbars()
{
    ShowWindow(hSelection, SW_HIDE);
    MoveWindow(hImageArea, 3, 3, imgXRes*zoom/1000, imgYRes*zoom/1000, FALSE);
    InvalidateRect(hScrollbox, NULL, TRUE);
    InvalidateRect(hImageArea, NULL, FALSE);
    
    SetScrollPos(hScrollbox, SB_HORZ, 0, TRUE);
    SetScrollPos(hScrollbox, SB_VERT, 0, TRUE);
}

void ZoomTo(int newZoom)
{
    zoom = newZoom;
    updateCanvasAndScrollbars();
    int tbPos = 0;
    int tempZoom = newZoom;
    while (tempZoom>125)
    {
        tbPos++;
        tempZoom = tempZoom>>1;
    }
    SendMessage(hTrackbarZoom, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)tbPos);
}

HDC hdc;
BOOL drawing;

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        case WM_CLOSE:
            if (hwnd==hwndMiniature)
            {
                ShowWindow(hwndMiniature, SW_HIDE);
                showMiniature = FALSE;
                break;
            }
            if (!imageSaved)
            {
                TCHAR programname[20];
                TCHAR saveprompttext[100];
                TCHAR temptext[500];
                LoadString(hProgInstance, IDS_PROGRAMNAME, programname, SIZEOF(programname));
                LoadString(hProgInstance, IDS_SAVEPROMPTTEXT, saveprompttext, SIZEOF(saveprompttext));
                _stprintf(temptext, saveprompttext, filename);
                switch (MessageBox(hwnd, temptext, programname, MB_YESNOCANCEL | MB_ICONQUESTION))
                {
                    case IDNO:
                        DestroyWindow(hwnd);
                        break;
                    case IDYES:
                        SendMessage(hwnd, WM_COMMAND, IDM_FILESAVEAS, 0);
                        DestroyWindow(hwnd);
                        break;
                }
            }
            else
            {
                DestroyWindow(hwnd);
            }
            break;
        case WM_INITMENUPOPUP:
            switch (lParam)
            {
                case 0:
                    if (isAFile)
                    {
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERPLANE, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERCENTERED, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERSTRETCHED, MF_ENABLED | MF_BYCOMMAND);
                    }
                    else
                    {
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERPLANE, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERCENTERED, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_FILEASWALLPAPERSTRETCHED, MF_GRAYED | MF_BYCOMMAND);
                    }
                    break;
                case 1:
                    if (undoSteps>0)
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITUNDO, MF_ENABLED | MF_BYCOMMAND);
                    else
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITUNDO, MF_GRAYED | MF_BYCOMMAND);
                    if (redoSteps>0)
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITREDO, MF_ENABLED | MF_BYCOMMAND);
                    else
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITREDO, MF_GRAYED | MF_BYCOMMAND);
                    if (IsWindowVisible(hSelection))
                    {
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCUT, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCOPY, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITDELETESELECTION, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITINVERTSELECTION, MF_ENABLED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCOPYTO, MF_ENABLED | MF_BYCOMMAND);
                    }
                    else
                    {
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCUT, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCOPY, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITDELETESELECTION, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITINVERTSELECTION, MF_GRAYED | MF_BYCOMMAND);
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITCOPYTO, MF_GRAYED | MF_BYCOMMAND);
                    }
                    OpenClipboard(hMainWnd);
                    if (GetClipboardData(CF_BITMAP)!=NULL)
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITPASTE, MF_ENABLED | MF_BYCOMMAND);
                    else
                        EnableMenuItem(GetMenu(hMainWnd), IDM_EDITPASTE, MF_GRAYED | MF_BYCOMMAND);
                    CloseClipboard();
                    break;
                case 3:
                    if (IsWindowVisible(hSelection))
                        EnableMenuItem(GetMenu(hMainWnd), IDM_IMAGECROP, MF_ENABLED | MF_BYCOMMAND);
                    else
                        EnableMenuItem(GetMenu(hMainWnd), IDM_IMAGECROP, MF_GRAYED | MF_BYCOMMAND);
                    if (transpBg==0)
                        CheckMenuItem(GetMenu(hMainWnd), IDM_IMAGEDRAWOPAQUE, MF_CHECKED | MF_BYCOMMAND);
                    else
                        CheckMenuItem(GetMenu(hMainWnd), IDM_IMAGEDRAWOPAQUE, MF_UNCHECKED | MF_BYCOMMAND);
                    break;
            }
            if (showGrid)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWSHOWGRID, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWSHOWGRID, MF_UNCHECKED | MF_BYCOMMAND);
            if (showMiniature)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWSHOWMINIATURE, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWSHOWMINIATURE, MF_UNCHECKED | MF_BYCOMMAND);

            if (zoom==125)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM125, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM125, MF_UNCHECKED | MF_BYCOMMAND);
            if (zoom==250)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM25, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM25, MF_UNCHECKED | MF_BYCOMMAND);
            if (zoom==500)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM50, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM50, MF_UNCHECKED | MF_BYCOMMAND);
            if (zoom==1000)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM100, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM100, MF_UNCHECKED | MF_BYCOMMAND);
            if (zoom==2000)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM200, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM200, MF_UNCHECKED | MF_BYCOMMAND);
            if (zoom==4000)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM400, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM400, MF_UNCHECKED | MF_BYCOMMAND);
            if (zoom==8000)
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM800, MF_CHECKED | MF_BYCOMMAND);
            else
                CheckMenuItem(GetMenu(hMainWnd), IDM_VIEWZOOM800, MF_UNCHECKED | MF_BYCOMMAND);

            break;
        case WM_SIZE:
            if (hwnd==hMainWnd)
            { 
                int test[] = {LOWORD(lParam)-260, LOWORD(lParam)-140, LOWORD(lParam)-20};
                SendMessage(hStatusBar, WM_SIZE, wParam, lParam);
                SendMessage(hStatusBar, SB_SETPARTS, 3, (int)&test);
                MoveWindow(hScrollbox, 56, 49,LOWORD(lParam)-56, HIWORD(lParam)-72, TRUE);
                //InvalidateRect(hwnd, NULL, TRUE);
            }
            if (hwnd==hImageArea)
            {
                MoveWindow(hSizeboxLeftTop, 
                    0, 
                    0, 3, 3, TRUE);
                MoveWindow(hSizeboxCenterTop, 
                    imgXRes*zoom/2000+3*3/4, 
                    0, 3, 3, TRUE);
                MoveWindow(hSizeboxRightTop, 
                    imgXRes*zoom/1000+3, 
                    0, 3, 3, TRUE);
                MoveWindow(hSizeboxLeftCenter, 
                    0, 
                    imgYRes*zoom/2000+3*3/4, 3, 3, TRUE);
                MoveWindow(hSizeboxRightCenter, 
                    imgXRes*zoom/1000+3, 
                    imgYRes*zoom/2000+3*3/4, 3, 3, TRUE);
                MoveWindow(hSizeboxLeftBottom, 
                    0, 
                    imgYRes*zoom/1000+3, 3, 3, TRUE);
                MoveWindow(hSizeboxCenterBottom, 
                    imgXRes*zoom/2000+3*3/4, 
                    imgYRes*zoom/1000+3, 3, 3, TRUE);
                MoveWindow(hSizeboxRightBottom, 
                    imgXRes*zoom/1000+3, 
                    imgYRes*zoom/1000+3, 3, 3, TRUE);
            }
            if ((hwnd==hImageArea)||(hwnd==hScrollbox))
            {
                long clientRectScrollbox[4];
                long clientRectImageArea[4];
                SCROLLINFO horzScroll;
                SCROLLINFO vertScroll;
                GetClientRect(hScrollbox, (LPRECT)&clientRectScrollbox);
                GetClientRect(hImageArea, (LPRECT)&clientRectImageArea);
                MoveWindow(hScrlClient, 0, 0, max(clientRectImageArea[2]+6, clientRectScrollbox[2]), max(clientRectImageArea[3]+6, clientRectScrollbox[3]), TRUE);
                horzScroll.cbSize       = sizeof(SCROLLINFO);
                horzScroll.fMask        = SIF_PAGE | SIF_RANGE;
                horzScroll.nMax         = 10000;
                horzScroll.nMin         = 0;
                horzScroll.nPage        = clientRectScrollbox[2]*10000/(clientRectImageArea[2]+6);
                horzScroll.nPos         = 0;
                horzScroll.nTrackPos    = 0;
                SetScrollInfo(hScrollbox, SB_HORZ, &horzScroll, TRUE);
                GetClientRect(hScrollbox, (LPRECT)clientRectScrollbox);
                vertScroll.cbSize       = sizeof(SCROLLINFO);
                vertScroll.fMask        = SIF_PAGE | SIF_RANGE;
                vertScroll.nMax         = 10000;
                vertScroll.nMin         = 0;
                vertScroll.nPage        = clientRectScrollbox[3]*10000/(clientRectImageArea[3]+6);
                vertScroll.nPos         = 0;
                vertScroll.nTrackPos    = 0;
                SetScrollInfo(hScrollbox, SB_VERT, &vertScroll, TRUE);
            }
            break;
        case WM_HSCROLL:
            if (hwnd==hScrollbox)
            {
                if ((LOWORD(wParam)==SB_THUMBPOSITION)||(LOWORD(wParam)==SB_THUMBTRACK))
                {
                    SetScrollPos(hScrollbox, SB_HORZ, HIWORD(wParam), TRUE);
                    MoveWindow(hScrlClient, -(imgXRes*zoom/1000+6)*GetScrollPos(hScrollbox, SB_HORZ)/10000, 
                        -(imgYRes*zoom/1000+6)*GetScrollPos(hScrollbox, SB_VERT)/10000, imgXRes*zoom/1000+6, imgYRes*zoom/1000+6, TRUE);
                }
            }
            break;
        case WM_VSCROLL:
            if (hwnd==hScrollbox)
            {
                if ((LOWORD(wParam)==SB_THUMBPOSITION)||(LOWORD(wParam)==SB_THUMBTRACK))
                {
                    SetScrollPos(hScrollbox, SB_VERT, HIWORD(wParam), TRUE);
                    MoveWindow(hScrlClient, -(imgXRes*zoom/1000+6)*GetScrollPos(hScrollbox, SB_HORZ)/10000, 
                        -(imgYRes*zoom/1000+6)*GetScrollPos(hScrollbox, SB_VERT)/10000, imgXRes*zoom/1000+6, imgYRes*zoom/1000+6, TRUE);
                }
            }
            break;
        case WM_GETMINMAXINFO:
            if (hwnd==hMainWnd)
            {
                MINMAXINFO *mm = (LPMINMAXINFO)lParam;
                (*mm).ptMinTrackSize.x = 330;
                (*mm).ptMinTrackSize.y = 430;
            }
            break;
        case WM_PAINT:
            DefWindowProc (hwnd, message, wParam, lParam);
            if (hwnd==hImageArea)
            {
                HDC hdc = GetDC(hImageArea);
                StretchBlt(hdc, 0, 0, imgXRes*zoom/1000, imgYRes*zoom/1000, hDrawingDC, 0, 0, imgXRes, imgYRes, SRCCOPY);
                if (showGrid && (zoom>=4000))
                {
                    HPEN oldPen = SelectObject(hdc, CreatePen(PS_SOLID, 1, 0x00a0a0a0));
                    int counter;
                    for (counter = 0; counter <= imgYRes; counter++)
                    {
                        MoveToEx(hdc, 0, counter*zoom/1000, NULL);
                        LineTo(hdc, imgXRes*zoom/1000, counter*zoom/1000);
                    }
                    for (counter = 0; counter <= imgXRes; counter++)
                    {
                        MoveToEx(hdc, counter*zoom/1000, 0, NULL);
                        LineTo(hdc, counter*zoom/1000, imgYRes*zoom/1000);
                    }
                    DeleteObject(SelectObject(hdc, oldPen));
                }
                ReleaseDC(hImageArea, hdc);
                SendMessage(hSelection, WM_PAINT, 0, 0);
                SendMessage(hwndMiniature, WM_PAINT, 0, 0);
            }else
            if (hwnd==hwndMiniature)
            {
                long mclient[4];
                HDC hdc;
                GetClientRect(hwndMiniature, (LPRECT)&mclient);
                hdc = GetDC(hwndMiniature);
                BitBlt(hdc, -min(imgXRes*GetScrollPos(hScrollbox, SB_HORZ)/10000, imgXRes-mclient[2]), 
                    -min(imgYRes*GetScrollPos(hScrollbox, SB_VERT)/10000, imgYRes-mclient[3]), imgXRes, imgYRes, hDrawingDC, 0, 0, SRCCOPY);
                ReleaseDC(hwndMiniature, hdc);
            }
            break; 
            
        // mouse events used for drawing   
        
        case WM_SETCURSOR:
            if (hwnd==hImageArea)
            {
                switch (activeTool)
                {
                    case 4:
                        SetCursor(hCurFill);
                        break;
                    case 5:
                        SetCursor(hCurColor);
                        break;
                    case 6:
                        SetCursor(hCurZoom);
                        break;
                    case 7:
                        SetCursor(hCurPen);
                        break;
                    case 9:
                        SetCursor(hCurAirbrush);
                        break;
                    default:
                        SetCursor(LoadCursor(NULL, IDC_CROSS));
                }
            } else DefWindowProc(hwnd, message, wParam, lParam);
            break;
        case WM_LBUTTONDOWN:
            if (hwnd==hImageArea)
            { 
                if ((!drawing)||(activeTool==5))
                {
                    SetCapture(hImageArea);
                    drawing = TRUE;
                    startPaintingL(hDrawingDC, LOWORD(lParam)*1000/zoom, HIWORD(lParam)*1000/zoom, fgColor, bgColor);
                }else
                {
                    SendMessage(hwnd, WM_LBUTTONUP, wParam, lParam);
                    undo();
                }
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                if ((activeTool==6)&&(zoom<8000)) ZoomTo(zoom*2);
            }
            break; 
        case WM_RBUTTONDOWN:
            if (hwnd==hImageArea)
            { 
                if ((!drawing)||(activeTool==5))
                {
                    SetCapture(hImageArea);
                    drawing = TRUE;
                    startPaintingR(hDrawingDC, LOWORD(lParam)*1000/zoom, HIWORD(lParam)*1000/zoom, fgColor, bgColor);
                }else
                {
                    SendMessage(hwnd, WM_RBUTTONUP, wParam, lParam);
                    undo();
                }
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                if ((activeTool==6)&&(zoom>125)) ZoomTo(zoom/2);
            }
            break; 
        case WM_LBUTTONUP:
            if ((hwnd==hImageArea)&&drawing)
            { 
                ReleaseCapture();
                drawing = FALSE;
                endPaintingL(hDrawingDC, LOWORD(lParam)*1000/zoom, HIWORD(lParam)*1000/zoom, fgColor, bgColor);
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                if (activeTool==5)
                {
                    int tempColor = GetPixel(hDrawingDC, LOWORD(lParam)*1000/zoom, HIWORD(lParam)*1000/zoom);
                    if (tempColor!=CLR_INVALID) fgColor = tempColor;
                    SendMessage(hPalWin, WM_PAINT, 0, 0);
                }
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)"");
            }
            break;
        case WM_RBUTTONUP:
            if ((hwnd==hImageArea)&&drawing)
            { 
                ReleaseCapture();
                drawing = FALSE;
                endPaintingR(hDrawingDC, LOWORD(lParam)*1000/zoom, HIWORD(lParam)*1000/zoom, fgColor, bgColor);
                SendMessage(hImageArea, WM_PAINT, 0, 0);
                if (activeTool==5)
                {
                    int tempColor = GetPixel(hDrawingDC, LOWORD(lParam)*1000/zoom, HIWORD(lParam)*1000/zoom);
                    if (tempColor!=CLR_INVALID) bgColor = tempColor;
                    SendMessage(hPalWin, WM_PAINT, 0, 0);
                }
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)"");
            }
            break;
        case WM_MOUSEMOVE:
            if (hwnd==hImageArea)
            {
                if ((!drawing)||(activeTool<=9))
                {
                    TCHAR coordStr[100];
                    _stprintf(coordStr, _T("%d, %d"), (short)LOWORD(lParam)*1000/zoom, (short)HIWORD(lParam)*1000/zoom);
                    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)coordStr);
                }
                if (drawing)
                {
                    if ((wParam&MK_LBUTTON)!=0)
                    {
                        whilePaintingL(hDrawingDC, (short)LOWORD(lParam)*1000/zoom, (short)HIWORD(lParam)*1000/zoom, fgColor, bgColor);
                        SendMessage(hImageArea, WM_PAINT, 0, 0);
                        if ((activeTool>=10)||(activeTool==2))
                        {
                            TCHAR sizeStr[100];
                            _stprintf(sizeStr, _T("%d x %d"), (short)LOWORD(lParam)*1000/zoom-startX, (short)HIWORD(lParam)*1000/zoom-startY);
                            SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)sizeStr);
                        }
                    }
                    if ((wParam&MK_RBUTTON)!=0)
                    {
                        whilePaintingR(hDrawingDC, (short)LOWORD(lParam)*1000/zoom, (short)HIWORD(lParam)*1000/zoom, fgColor, bgColor);
                        SendMessage(hImageArea, WM_PAINT, 0, 0);
                        if (activeTool>=10)
                        {
                            TCHAR sizeStr[100];
                            _stprintf(sizeStr, _T("%d x %d"), (short)LOWORD(lParam)*1000/zoom-startX, (short)HIWORD(lParam)*1000/zoom-startY);
                            SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)sizeStr);
                        }
                    }
                }
            } else
            {
                SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)_T(""));
            }
            break;
            
        // menu and button events
        
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDM_HELPINFO:
                    {
                        HICON paintIcon = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON));
                        TCHAR infotitle[100];
                        TCHAR infotext[200];
                        LoadString(hProgInstance, IDS_INFOTITLE, infotitle, SIZEOF(infotitle));
                        LoadString(hProgInstance, IDS_INFOTEXT, infotext, SIZEOF(infotext));
                        ShellAbout(hMainWnd, infotitle, infotext, paintIcon);
                        DeleteObject(paintIcon);
                    }
                    break;
                case IDM_HELPHELPTOPICS:
                    //HtmlHelp(hMainWnd, "help\\Paint.chm", 0, 0);
                    break;
                case IDM_FILEEXIT:
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                    break;
                case IDM_FILENEW:
                    Rectangle(hDrawingDC, 0-1, 0-1, imgXRes+1, imgYRes+1);
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                case IDM_FILEOPEN:
                    if (GetOpenFileName(&ofn)!=0)
                    {
                        HBITMAP bmNew = (HBITMAP)LoadDIBFromFile(ofn.lpstrFile);
                        if (bmNew!=NULL)
                        {
                            TCHAR tempstr[1000];
                            TCHAR resstr[100];
                            insertReversible(bmNew);
                            updateCanvasAndScrollbars();
                            CopyMemory(filename, ofn.lpstrFileTitle, sizeof(filename));
                            CopyMemory(filepathname, ofn.lpstrFileTitle, sizeof(filepathname));
                            LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
                            _stprintf(tempstr, resstr, filename);
                            SetWindowText(hMainWnd, tempstr);
                            clearHistory();
                            isAFile = TRUE;
                        }
                    }
                    break;
                case IDM_FILESAVE:
                    if (isAFile)
                    {
                        SaveDIBToFile(hBms[currInd], filepathname, hDrawingDC);
                        imageSaved = TRUE;
                    }
                    else
                        SendMessage(hwnd, WM_COMMAND, IDM_FILESAVEAS, 0);
                    break;
                case IDM_FILESAVEAS:
                    if (GetSaveFileName(&sfn)!=0)
                    {
                        TCHAR tempstr[1000];
                        TCHAR resstr[100];
                        SaveDIBToFile(hBms[currInd], sfn.lpstrFile, hDrawingDC);
                        CopyMemory(filename, sfn.lpstrFileTitle, sizeof(filename));
                        CopyMemory(filepathname, sfn.lpstrFile, sizeof(filepathname));
                        LoadString(hProgInstance, IDS_WINDOWTITLE, resstr, SIZEOF(resstr));
                        _stprintf(tempstr, resstr, filename);
                        SetWindowText(hMainWnd, tempstr);
                        isAFile = TRUE;
                        imageSaved = TRUE;
                    }
                    break;
                case IDM_FILEASWALLPAPERPLANE:
                    SetWallpaper(filepathname, 1, 1);
                    break;
                case IDM_FILEASWALLPAPERCENTERED:
                    SetWallpaper(filepathname, 1, 0);
                    break;
                case IDM_FILEASWALLPAPERSTRETCHED:
                    SetWallpaper(filepathname, 2, 0);
                    break;
                case IDM_EDITUNDO:
                    undo();
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                case IDM_EDITREDO:
                    redo();
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                case IDM_EDITCOPY:
                    OpenClipboard(hMainWnd);
                    EmptyClipboard();
                    SetClipboardData(CF_BITMAP, CopyImage(hSelBm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
                    CloseClipboard();
                    break;
                case IDM_EDITPASTE:
                    OpenClipboard(hMainWnd);
                    if (GetClipboardData(CF_BITMAP)!=NULL)
                    {
                        DeleteObject(SelectObject(hSelDC, hSelBm = CopyImage(GetClipboardData(CF_BITMAP), IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG)));
                        newReversible();
                        rectSel_src[0] = rectSel_src[1] = rectSel_src[2] = rectSel_src[3] = 0;
                        rectSel_dest[0] = rectSel_dest[1] = 0;
                        rectSel_dest[2] = GetDIBWidth(hSelBm);
                        rectSel_dest[3] = GetDIBHeight(hSelBm);
                        BitBlt(hDrawingDC, rectSel_dest[0], rectSel_dest[1], rectSel_dest[2], rectSel_dest[3], hSelDC, 0, 0, SRCCOPY);
                        placeSelWin();
                        ShowWindow(hSelection, SW_SHOW);
                    }
                    CloseClipboard();
                    break;
                case IDM_EDITDELETESELECTION:
                    ShowWindow(hSelection, SW_HIDE);
                    break;
                case IDM_EDITSELECTALL:
                    if (activeTool==2)
                    {
                        startPaintingL(hDrawingDC, 0, 0, fgColor, bgColor);
                        whilePaintingL(hDrawingDC, imgXRes, imgYRes, fgColor, bgColor);
                        endPaintingL(hDrawingDC, imgXRes, imgYRes, fgColor, bgColor);
                    }
                    break;
                case IDM_EDITCOPYTO:
                    if (GetSaveFileName(&ofn)!=0) SaveDIBToFile(hSelBm, ofn.lpstrFile, hDrawingDC);
                    break;
                case IDM_COLORSEDITPALETTE:
                    if (ChooseColor(&choosecolor))
                    {
                        fgColor = choosecolor.rgbResult;
                        SendMessage(hPalWin, WM_PAINT, 0, 0);
                    }
                    break;
                case IDM_IMAGEINVERTCOLORS:
                    {
                        RECT tempRect;
                        newReversible();
                        SetRect(&tempRect, 0, 0, imgXRes, imgYRes);
                        InvertRect(hDrawingDC, &tempRect);
                        SendMessage(hImageArea, WM_PAINT, 0, 0);
                    }
                    break; 
                case IDM_IMAGEDELETEIMAGE:
                    newReversible();
                    Rect(hDrawingDC, 0, 0, imgXRes, imgYRes, bgColor, bgColor, 0, TRUE);
                    SendMessage(hImageArea, WM_PAINT, 0, 0);
                    break;
                case IDM_IMAGEROTATEMIRROR:
                    switch (mirrorRotateDlg())
                    {
                        case 1:
                            newReversible();
                            StretchBlt(hDrawingDC, imgXRes-1, 0, -imgXRes, imgYRes, hDrawingDC, 0, 0, imgXRes, imgYRes, SRCCOPY);
                            SendMessage(hImageArea, WM_PAINT, 0, 0);
                            break;
                        case 2:
                            newReversible();
                            StretchBlt(hDrawingDC, 0, imgYRes-1, imgXRes, -imgYRes, hDrawingDC, 0, 0, imgXRes, imgYRes, SRCCOPY);
                            SendMessage(hImageArea, WM_PAINT, 0, 0);
                            break;
                        case 4:
                            newReversible();
                            StretchBlt(hDrawingDC, imgXRes-1, imgYRes-1, -imgXRes, -imgYRes, hDrawingDC, 0, 0, imgXRes, imgYRes, SRCCOPY);
                            SendMessage(hImageArea, WM_PAINT, 0, 0);
                            break;
                    }
                    break;
                case IDM_IMAGEATTRIBUTES:
                    {
                        int retVal = attributesDlg();
                        if ((LOWORD(retVal)!=0)&&(HIWORD(retVal)!=0))
                        {
                            cropReversible(LOWORD(retVal), HIWORD(retVal), 0, 0);
                            updateCanvasAndScrollbars();
                        }
                    }
                    break;
                case IDM_IMAGECHANGESIZE:
                    {
                        int retVal = changeSizeDlg();
                        if ((LOWORD(retVal)!=0)&&(HIWORD(retVal)!=0))
                        {
                            insertReversible(CopyImage(hBms[currInd], IMAGE_BITMAP, imgXRes*LOWORD(retVal)/100, imgYRes*HIWORD(retVal)/100, 0));
                            updateCanvasAndScrollbars();
                        }
                    }
                    break;
                case IDM_IMAGEDRAWOPAQUE:
                    transpBg = 1-transpBg;
                    SendMessage(hToolSettings, WM_PAINT, 0, 0);
                    break;
                case IDM_IMAGECROP:
                    insertReversible(CopyImage(hSelBm, IMAGE_BITMAP, 0, 0, LR_COPYRETURNORG));
                    updateCanvasAndScrollbars();
                    break;

                case IDM_VIEWSHOWGRID:
                    showGrid = !showGrid;
                    break;
                case IDM_VIEWSHOWMINIATURE:
                    showMiniature = !showMiniature;
                    if (showMiniature)
                        ShowWindow(hwndMiniature, SW_SHOW);
                    else
                        ShowWindow(hwndMiniature, SW_HIDE);
                    break;
                    
                case IDM_VIEWZOOM125:
                    ZoomTo(125);
                    break;
                case IDM_VIEWZOOM25:
                    ZoomTo(250);
                    break;
                case IDM_VIEWZOOM50:
                    ZoomTo(500);
                    break;
                case IDM_VIEWZOOM100:
                    ZoomTo(1000);
                    break;
                case IDM_VIEWZOOM200:
                    ZoomTo(2000);
                    break;
                case IDM_VIEWZOOM400:
                    ZoomTo(4000);
                    break;
                case IDM_VIEWZOOM800:
                    ZoomTo(8000);
                    break;
                case ID_FREESEL:
                    selectTool(1);
                    break; 
                case ID_RECTSEL:
                    selectTool(2);
                    break; 
                case ID_RUBBER:
                    selectTool(3);
                    break; 
                case ID_FILL:
                    selectTool(4);
                    break; 
                case ID_COLOR:
                    selectTool(5);
                    break; 
                case ID_ZOOM:
                    selectTool(6);
                    break; 
                case ID_PEN:
                    selectTool(7);
                    break; 
                case ID_BRUSH:
                    selectTool(8);
                    break; 
                case ID_AIRBRUSH:
                    selectTool(9);
                    break; 
                case ID_TEXT:
                    selectTool(10);
                    break; 
                case ID_LINE:
                    selectTool(11);
                    break; 
                case ID_BEZIER:
                    selectTool(12);
                    break; 
                case ID_RECT:
                    selectTool(13);
                    break; 
                case ID_SHAPE:
                    selectTool(14);
                    break; 
                case ID_ELLIPSE:
                    selectTool(15);
                    break; 
                case ID_RRECT:
                    selectTool(16);
                    break;
            }
            break;
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

