/*
 * Copyright 2003 Martin Fuchs
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


 //
 // Explorer clone
 //
 // taskbar.cpp
 //
 // Martin Fuchs, 16.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"

#include "taskbar.h"


TaskBarEntry::TaskBarEntry()
{
	_id = 0;
	_hbmp = 0;
	_bmp_idx = 0;
	_used = 0;
	_btn_idx = 0;
	_fsState = 0;
}

TaskBarMap::~TaskBarMap()
{
	while(!empty()) {
		iterator it = begin();
		DeleteBitmap(it->second._hbmp);
		erase(it);
	}
}


TaskBar::TaskBar(HWND hwnd)
 :	super(hwnd)
{
}

TaskBar::~TaskBar()
{
	KillTimer(_hwnd, 0);

	//DeinstallShellHook();
}

HWND TaskBar::Create(HWND hwndParent)
{
	ClientRect clnt(hwndParent);

	return Window::Create(WINDOW_CREATOR(TaskBar), 0,
							BtnWindowClass(CLASSNAME_TASKBAR), TITLE_TASKBAR, WS_CHILD|WS_VISIBLE,
							TASKBAR_LEFT, 0, clnt.right-TASKBAR_LEFT/*-(NOTIFYAREA_WIDTH+1)*/, clnt.bottom, hwndParent);
}

LRESULT TaskBar::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	_htoolbar = CreateToolbarEx(_hwnd,
		WS_CHILD|WS_VISIBLE|CCS_NODIVIDER|CCS_TOP|
		TBSTYLE_LIST|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE,
		IDW_TASKTOOLBAR, 0, 0, 0, NULL, 0, 0, 0, 16, 16, sizeof(TBBUTTON));

	SendMessage(_htoolbar, TB_SETBUTTONWIDTH, 0, MAKELONG(80,160));
	//SendMessage(_htoolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
	//SendMessage(_htoolbar, TB_SETDRAWTEXTFLAGS, DT_CENTER|DT_VCENTER, DT_CENTER|DT_VCENTER);
	//SetWindowFont(_htoolbar, GetStockFont(ANSI_VAR_FONT), FALSE);

	_next_id = IDC_FIRST_APP;

	//InstallShellHook(_hwnd, PM_SHELLHOOK_NOTIFY);

	Refresh();

	SetTimer(_hwnd, 0, 200, NULL);

	return 0;
}

LRESULT TaskBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
/*	  case WM_CLOSE:
		break; */

	  case WM_SIZE:
		SendMessage(_htoolbar, WM_SIZE, 0, 0);
		break;

	  case WM_TIMER:
		Refresh();
		return 0;
/*
//#define PM_SHELLHOOK_NOTIFY		(WM_APP+0x10)

	  case PM_SHELLHOOK_NOTIFY: {
		int code = lparam;

		switch(code) {
		  case HSHELL_WINDOWCREATED:
		  case HSHELL_WINDOWDESTROYED:
		  case HSHELL_WINDOWACTIVATED:
		  case HSHELL_WINDOWREPLACED:
			Refresh();
			break;
		}
		Refresh();
		break;}
*/
	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int TaskBar::Command(int id, int code)
{
	TaskBarMap::iterator found = _map.find_id(id);

	if (found != _map.end()) {
		HWND hwnd = found->first;

		if (hwnd==GetForegroundWindow() || hwnd==_last_foreground_wnd) {
			ShowWindowAsync(hwnd, SW_MINIMIZE);
			_last_foreground_wnd = 0;
		} else {
			 // switch to selected application window
			if (IsIconic(hwnd))
				ShowWindowAsync(hwnd, SW_RESTORE);

			SetForegroundWindow(hwnd);

			_last_foreground_wnd = hwnd;
		}

		Refresh();

		return 0;
	}

	return super::Command(id, code);
}


static HICON get_window_icon(HWND hwnd)
{
	HICON hIcon = 0;

	SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hIcon);

	if (!hIcon)
		SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hIcon);

	if (!hIcon)
		SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hIcon);

	if (!hIcon)
		hIcon = (HICON)GetClassLong(hwnd, GCL_HICONSM);

	if (!hIcon)
		hIcon = (HICON)GetClassLong(hwnd, GCL_HICON);

	if (!hIcon)
		SendMessageTimeout(hwnd, WM_QUERYDRAGICON, 0, 0, 0, 1000, (LPDWORD)&hIcon);

	if (!hIcon)
		hIcon = LoadIcon(0, IDI_APPLICATION);

	return hIcon;
}

HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd)
{
	HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, 16, 16);

	HDC hdc = CreateCompatibleDC(0);
	HBITMAP hbmp_org = SelectBitmap(hdc, hbmp);

	RECT rect = {0, 0, 16, 16};
	FillRect(hdc, &rect, hbrush_bkgnd);

	DrawIconEx(hdc, 0, 0, hIcon, 16, 16, 0, hbrush_bkgnd, DI_NORMAL);

	SelectBitmap(hdc, hbmp_org);
	DeleteDC(hdc);

	return hbmp;
}

 // fill task bar with buttons for enumerated top level windows
BOOL CALLBACK TaskBar::EnumWndProc(HWND hwnd, LPARAM lparam)
{
	TaskBar* pThis = (TaskBar*)lparam;

	DWORD style = GetWindowStyle(hwnd);
	DWORD ex_style = GetWindowExStyle(hwnd);

	if ((style&WS_VISIBLE) && !(ex_style&WS_EX_TOOLWINDOW) &&
		!GetParent(hwnd) && !GetWindow(hwnd,GW_OWNER)) {
		TCHAR title[BUFFER_LEN];

		if (!GetWindowText(hwnd, title, BUFFER_LEN))
			title[0] = '\0';

		TaskBarMap::iterator found = pThis->_map.find(hwnd);
		int last_id = 0;

		if (found != pThis->_map.end()) {
			last_id = found->second._id;

			if (!last_id)
				found->second._id = pThis->_next_id++;
		} else {
			HICON hIcon = get_window_icon(hwnd);
			HBITMAP hbmp = create_bitmap_from_icon(hIcon, GetSysColorBrush(COLOR_BTNFACE), WindowCanvas(pThis->_htoolbar));

			TBADDBITMAP ab = {0, (UINT_PTR)hbmp};
			int bmp_idx = SendMessage(pThis->_htoolbar, TB_ADDBITMAP, 1, (LPARAM)&ab);

			TaskBarEntry entry;

			entry._id = pThis->_next_id++;
			entry._hbmp = hbmp;
			entry._bmp_idx = bmp_idx;
			entry._title = title;

			pThis->_map[hwnd] = entry;
			found = pThis->_map.find(hwnd);
		}

		TBBUTTON btn = {-2/*I_IMAGENONE*/, 0, TBSTATE_ENABLED/*|TBSTATE_ELLIPSES*/, BTNS_BUTTON, {0, 0}, 0, 0};
		TaskBarEntry& entry = found->second;

		++entry._used;
		btn.idCommand = entry._id;

		if (hwnd == GetForegroundWindow())
			btn.fsState |= TBSTATE_PRESSED|TBSTATE_CHECKED;

		if (!last_id) {
			 // create new toolbar buttons for new windows
			if (title[0])
				btn.iString = (INT_PTR)title;

			btn.iBitmap = entry._bmp_idx;
			entry._btn_idx = SendMessage(pThis->_htoolbar, TB_BUTTONCOUNT, 0, 0);

			SendMessage(pThis->_htoolbar, TB_INSERTBUTTON, entry._btn_idx, (LPARAM)&btn);
		} else {
			 // refresh attributes of existing buttons
			if (btn.fsState != entry._fsState)
				SendMessage(pThis->_htoolbar, TB_SETSTATE, entry._id, MAKELONG(btn.fsState,0));

			if (entry._title != title) {
				TBBUTTONINFO info;

				info.cbSize = sizeof(TBBUTTONINFO);
				info.dwMask = TBIF_TEXT;
				info.pszText = title;

				SendMessage(pThis->_htoolbar, TB_SETBUTTONINFO, entry._id, (LPARAM)&info);

				entry._title = title;
			}
		}

		entry._fsState = btn.fsState;

		 // move minimized windows out of sight
		if (IsIconic(hwnd)) {
			RECT rect;

			GetWindowRect(hwnd, &rect);

			if (rect.bottom > 0)
				SetWindowPos(hwnd, 0, -32000, -32000, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}

	return TRUE;
}

void TaskBar::Refresh()
{
	for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it)
		it->second._used = 0;

	EnumWindows(EnumWndProc, (LPARAM)this);
	//EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()), EnumWndProc, (LPARAM)_htoolbar);

	set<int> btn_idx_to_delete;

	for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it) {
		TaskBarEntry& entry = it->second;

		if (!entry._used && entry._id) {
			 // store button indexes to remove
			btn_idx_to_delete.insert(entry._btn_idx);
			entry._id = 0;
		}
	}

	 // remove buttons from right to left
	for(set<int>::reverse_iterator it=btn_idx_to_delete.rbegin(); it!=btn_idx_to_delete.rend(); ++it) {
		int idx = *it;

		SendMessage(_htoolbar, TB_DELETEBUTTON, idx, 0);

		for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it) {
			TaskBarEntry& entry = it->second;

			 // adjust button indexes
			if (entry._btn_idx > idx)
				--entry._btn_idx;
		}
	}
}

TaskBarMap::iterator TaskBarMap::find_id(int id)
{
	for(iterator it=begin(); it!=end(); ++it)
		if (it->second._id == id)
			return it;

	return end();
}
