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
 // startmenu.cpp
 //
 // Explorer start menu
 //
 // Martin Fuchs, 19.08.2003
 //
 // Credits: Thanks to Everaldo (http://www.everaldo.com) for his nice looking icons.
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "../externals.h"
#include "../explorer_intres.h"

#include "desktopbar.h"
#include "startmenu.h"
#include "../dialogs/searchprogram.h"


StartMenu::StartMenu(HWND hwnd)
 :	super(hwnd)
{
	_next_id = IDC_FIRST_MENU;
	_submenu_id = 0;
	_border_left = 0;
	_border_top = 0;
	_last_pos = WindowRect(hwnd).pos();
#ifdef _LIGHT_STARTMENU
	_selected_id = -1;
#endif
}

StartMenu::StartMenu(HWND hwnd, const StartMenuCreateInfo& create_info)
 :	super(hwnd),
	_create_info(create_info)
{
	for(StartMenuFolders::const_iterator it=create_info._folders.begin(); it!=create_info._folders.end(); ++it)
		if (*it)
			_dirs.push_back(ShellDirectory(Desktop(), *it, _hwnd));

	_next_id = IDC_FIRST_MENU;
	_submenu_id = 0;
	_border_left = 0;
	_border_top = create_info._border_top;
	_last_pos = WindowRect(hwnd).pos();
#ifdef _LIGHT_STARTMENU
	_selected_id = -1;
#endif
}

StartMenu::~StartMenu()
{
	SendParent(PM_STARTMENU_CLOSED);
}


 // We need this wrapper function for s_wcStartMenu, it calls the WIN32 API,
 // though static C++ initializers are not allowed for Winelib applications.
BtnWindowClass& StartMenu::GetWndClasss()
{
	static BtnWindowClass s_wcStartMenu(CLASSNAME_STARTMENU);

	return s_wcStartMenu;
}


Window::CREATORFUNC StartMenu::s_def_creator = STARTMENU_CREATOR(StartMenu);

HWND StartMenu::Create(int x, int y, const StartMenuFolders& folders, HWND hwndParent, LPCTSTR title, CREATORFUNC creator)
{
	UINT style, ex_style;
	int top_height;

	if (hwndParent) {
		style = WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE;
		ex_style = 0;
		top_height = STARTMENU_TOP_BTN_SPACE;
	} else {
		style = WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_CLIPCHILDREN|WS_VISIBLE;
		ex_style = WS_EX_TOOLWINDOW;
		top_height = 0;
	}

	RECT rect = {x, y-STARTMENU_LINE_HEIGHT-top_height, x+STARTMENU_WIDTH_MIN, y};

#ifndef _LIGHT_STARTMENU
	rect.top += STARTMENU_LINE_HEIGHT;
#endif

	AdjustWindowRectEx(&rect, style, FALSE, ex_style);

	StartMenuCreateInfo create_info;

	create_info._folders = folders;
	create_info._border_top = top_height;
	create_info._creator = creator;

	if (title)
		create_info._title = title;

	HWND hwnd = Window::Create(creator, &create_info, ex_style, GetWndClasss(), title,
								style, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, hwndParent);

	 // make sure the window is not off the screen
	MoveVisible(hwnd);

	return hwnd;
}


LRESULT	StartMenu::Init(LPCREATESTRUCT pcs)
{
	try {
		AddEntries();

		if (super::Init(pcs))
			return 1;

		 // create buttons for registered entries in _entries
		for(ShellEntryMap::const_iterator it=_entries.begin(); it!=_entries.end(); ++it) {
			const StartMenuEntry& sme = it->second;
			bool hasSubmenu = false;

			for(ShellEntrySet::const_iterator it=sme._entries.begin(); it!=sme._entries.end(); ++it)
				if ((*it)->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					hasSubmenu = true;

#ifdef _LIGHT_STARTMENU
			_buttons.push_back(SMBtnInfo(sme, it->first, hasSubmenu));
#else
			AddButton(sme._title, sme._hIcon, hasSubmenu, it->first);
#endif
		}

#ifdef _LIGHT_STARTMENU
		if (_buttons.empty())
#else
		if (!GetWindow(_hwnd, GW_CHILD))
#endif
			AddButton(ResString(IDS_EMPTY), 0, false, 0, false);

#ifdef _LIGHT_STARTMENU
		ResizeToButtons();
#endif

#ifdef _LAZY_ICONEXTRACT
		PostMessage(_hwnd, PM_UPDATE_ICONS, 0, 0);
#endif
	} catch(COMException& e) {
		HandleException(e, pcs->hwndParent);	// destroys the start menu window while switching focus
	}

	return 0;
}

void StartMenu::AddEntries()
{
	for(StartMenuShellDirs::iterator it=_dirs.begin(); it!=_dirs.end(); ++it) {
		StartMenuDirectory& smd = *it;
		ShellDirectory& dir = smd._dir;

		if (!dir._scanned) {
			WaitCursor wait;

#ifdef _LAZY_ICONEXTRACT
			dir.smart_scan(SCAN_FILESYSTEM);	// lazy icon extraction, try to read directly from filesystem
#else
			dir.smart_scan(SCAN_EXTRACT_ICONS|SCAN_FILESYSTEM);
#endif
		}

		AddShellEntries(dir, -1, smd._subfolders);
	}
}


void StartMenu::AddShellEntries(const ShellDirectory& dir, int max, bool subfolders)
{
	int cnt = 0;

	for(const Entry*entry=dir._down; entry; entry=entry->_next) {
		 // hide files like "desktop.ini"
		if (entry->_shell_attribs & SFGAO_HIDDEN)
		//not appropriate for drive roots: if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;

		 // hide subfolders if requested
		if (!subfolders)
			if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

		 // only 'max' entries shall be added.
		if (++cnt == max)
			break;

		if (entry->_etype == ET_SHELL)
			AddEntry(dir._folder, static_cast<const ShellEntry*>(entry));
		else
			AddEntry(dir._folder, entry);
	}
}


LRESULT StartMenu::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);
		Paint(canvas);
		break;}

	  case WM_SIZE:
		ResizeButtons(LOWORD(lparam)-_border_left);
		break;

	  case WM_MOVE: {
		POINTS& pos = MAKEPOINTS(lparam);

		 // move open submenus of floating menus
		if (_submenu) {
			int dx = pos.x - _last_pos.x;
			int dy = pos.y - _last_pos.y;

			if (dx || dy) {
				WindowRect rt(_submenu);
				SetWindowPos(_submenu, 0, rt.left+dx, rt.top+dy, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
				//MoveVisible(_submenu);
			}
		}

		_last_pos.x = pos.x;
		_last_pos.y = pos.y;
		goto def;}

	  case WM_NCHITTEST: {
		LRESULT res = super::WndProc(nmsg, wparam, lparam);

		if (res>=HTSIZEFIRST && res<=HTSIZELAST)
			return HTCLIENT;	// disable window resizing

		return res;}

	  case WM_LBUTTONDOWN: {
		RECT rect;

		 // check mouse cursor for coordinates of floating button
		GetFloatingButtonRect(&rect);

		if (PtInRect(&rect, Point(lparam))) {
			 // create a floating copy of the current start menu
 			WindowRect pos(_hwnd);

			///@todo do something similar to StartMenuRoot::TrackStartmenu() in order to automatically close submenus when clicking on the desktop background
			StartMenu::Create(pos.left+3, pos.bottom-3, _create_info._folders, 0, _create_info._title, _create_info._creator);
			CloseStartMenu();
		}

#ifdef _LIGHT_STARTMENU
		int id = ButtonHitTest(Point(lparam));

		if (id)
			Command(id, BN_CLICKED);
#endif
		break;}

	  case WM_SYSCOMMAND:
		if ((wparam&0xFFF0) == SC_SIZE)
			return 0;			// disable window resizing
		goto def;

	  case WM_ACTIVATEAPP:
		 // close start menu when activating another application
		if (!wparam)
			CloseStartMenu();
		break;	// don't call super::WndProc in case "this" has been deleted

	  case WM_CANCELMODE:
		CloseStartMenu();
		break;

#ifdef _LIGHT_STARTMENU
	  case WM_MOUSEMOVE: {
		 // automatically set the focus to startmenu entries when moving the mouse over them
		int new_id = ButtonHitTest(Point(lparam));

		if (new_id != _selected_id)
			SelectButton(new_id);
		break;}

	  case WM_KEYDOWN:
		if (wparam==VK_RETURN && _selected_id)
			Command(_selected_id, BN_CLICKED);
		break;
#else
	  case PM_STARTENTRY_FOCUSED: {	///@todo use TrackMouseEvent() and WM_MOUSEHOVER to wait a bit before opening submenus
		BOOL hasSubmenu = wparam;
		HWND hctrl = (HWND)lparam;

		 // automatically open submenus
		if (hasSubmenu) {
			UpdateWindow(_hwnd);	// draw focused button before waiting on submenu creation
			//SendMessage(_hwnd, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hctrl),BN_CLICKED), (LPARAM)hctrl);
			Command(GetDlgCtrlID(hctrl), BN_CLICKED);
		} else {
			 // close any open submenu
			CloseOtherSubmenus(0);
		}
		break;}
#endif

#ifdef _LAZY_ICONEXTRACT
	  case PM_UPDATE_ICONS:
		UpdateIcons(wparam);
		break;
#endif

	  case PM_STARTENTRY_LAUNCHED:
		if (GetWindowStyle(_hwnd) & WS_CAPTION)	// don't automatically close floating menus
			return 0;

		 // route message to the parent menu and close menus after launching an entry
		if (!SendParent(nmsg, wparam, lparam))
			DestroyWindow(_hwnd);
		return 1;	// signal that we have received and processed the message

	  case PM_STARTMENU_CLOSED:
		_submenu = 0;
		break;

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


#ifdef _LIGHT_STARTMENU

int StartMenu::ButtonHitTest(POINT pt)
{
	ClientRect clnt(_hwnd);
	RECT rect = {_border_left, _border_top, clnt.right, STARTMENU_LINE_HEIGHT};

	if (pt.x<rect.left || pt.x>rect.right)
		return 0;

	for(SMBtnVector::const_iterator it=_buttons.begin(); it!=_buttons.end(); ++it) {
		const SMBtnInfo& info = *it;

		if (rect.top > pt.y)
			break;

		rect.bottom = rect.top + (info._id==-1? STARTMENU_SEP_HEIGHT: STARTMENU_LINE_HEIGHT);

		if (pt.y < rect.bottom)	// PtInRect(&rect, pt)
			return info._id;

		rect.top = rect.bottom;
	}

	return 0;
}

void StartMenu::InvalidateSelection()
{
	if (!_selected_id)
		return;

	ClientRect clnt(_hwnd);
	RECT rect = {_border_left, _border_top, clnt.right, STARTMENU_LINE_HEIGHT};

	for(SMBtnVector::const_iterator it=_buttons.begin(); it!=_buttons.end(); ++it) {
		const SMBtnInfo& info = *it;

		rect.bottom = rect.top + (info._id==-1? STARTMENU_SEP_HEIGHT: STARTMENU_LINE_HEIGHT);

		if (info._id == _selected_id) {
			InvalidateRect(_hwnd, &rect, TRUE);
			break;
		}

		rect.top = rect.bottom;
	}
}

const SMBtnInfo* StartMenu::GetButtonInfo(int id) const
{
	for(SMBtnVector::const_iterator it=_buttons.begin(); it!=_buttons.end(); ++it)
		if (it->_id == id)
			return &*it;

	return NULL;
}

void StartMenu::SelectButton(int id)
{
	InvalidateSelection();

	const SMBtnInfo* btn = GetButtonInfo(id);

	if (btn && btn->_enabled) {
		_selected_id = id;

		InvalidateSelection();

		 // automatically open submenus
		if (btn->_hasSubmenu) {
			UpdateWindow(_hwnd);	// draw focused button before waiting on submenu creation
			Command(_selected_id, BN_CLICKED);
		} else
			CloseOtherSubmenus(0);	// close any open submenu
	} else
		_selected_id = -1;
}

#endif


bool StartMenu::GetButtonRect(int id, PRECT prect) const
{
#ifdef _LIGHT_STARTMENU
	ClientRect clnt(_hwnd);
	RECT rect = {_border_left, _border_top, clnt.right, STARTMENU_LINE_HEIGHT};

	for(SMBtnVector::const_iterator it=_buttons.begin(); it!=_buttons.end(); ++it) {
		const SMBtnInfo& info = *it;

		rect.bottom = rect.top + (info._id==-1? STARTMENU_SEP_HEIGHT: STARTMENU_LINE_HEIGHT);

		if (info._id == id) {
			*prect = rect;
			return true;
		}

		rect.top = rect.bottom;
	}

	return false;
#else
	HWND btn = GetDlgItem(_hwnd, id);

	if (btn) {
		GetWindowRect(btn, prect);
		ScreenToClient(_hwnd, prect);

		return true;
	} else
		return false;
#endif
}


void StartMenu::DrawFloatingButton(HDC hdc)
{
	static ResIconEx floatingIcon(IDI_FLOATING, 8, 4);

	ClientRect clnt(_hwnd);

	DrawIconEx(hdc, clnt.right-12, 0, floatingIcon, 8, 4, 0, 0, DI_NORMAL);
}

void StartMenu::GetFloatingButtonRect(LPRECT prect)
{
	GetClientRect(_hwnd, prect);

	prect->right -= 4;
	prect->left = prect->right - 8;
	prect->bottom = 4;
}


void StartMenu::Paint(PaintCanvas& canvas)
{
	if (_border_top)
		DrawFloatingButton(canvas);

#ifdef _LIGHT_STARTMENU
	ClientRect clnt(_hwnd);
	RECT rect = {_border_left, _border_top, clnt.right, STARTMENU_LINE_HEIGHT};

	int sep_width = rect.right-rect.left - 4;

	FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));
	BkMode bk_mode(canvas, TRANSPARENT);

	for(SMBtnVector::const_iterator it=_buttons.begin(); it!=_buttons.end(); ++it) {
		const SMBtnInfo& btn = *it;

		if (rect.top > canvas.rcPaint.bottom)
			break;

		if (btn._id == -1) {	// a separator?
			rect.bottom = rect.top + STARTMENU_SEP_HEIGHT;

			BrushSelection brush_sel(canvas, GetSysColorBrush(COLOR_BTNSHADOW));
			PatBlt(canvas, rect.left+2, rect.top+STARTMENU_SEP_HEIGHT/2-1, sep_width, 1, PATCOPY);

			SelectBrush(canvas, GetSysColorBrush(COLOR_BTNHIGHLIGHT));
			PatBlt(canvas, rect.left+2, rect.top+STARTMENU_SEP_HEIGHT/2, sep_width, 1, PATCOPY);
		} else {
			rect.bottom = rect.top + STARTMENU_LINE_HEIGHT;

			if (rect.top >= canvas.rcPaint.top)
				DrawStartMenuButton(canvas, rect, btn._title, btn._hIcon,
									btn._hasSubmenu, btn._enabled, btn._id==_selected_id, false);
		}

		rect.top = rect.bottom;
	}
#endif
}

#ifdef _LAZY_ICONEXTRACT
void StartMenu::UpdateIcons(int idx)
{
	UpdateWindow(_hwnd);

#ifdef _SINGLE_ICONEXTRACT
	 // extract only one icon per call to allow leaving the folder while the lazy extraction is running
	if (idx >= 0) {
		for(; idx<(int)_buttons.size(); ++idx) {
			SMBtnInfo& btn = _buttons[idx];

			if (!btn._hIcon && btn._id>0) {
				StartMenuEntry& sme = _entries[btn._id];

				btn._hIcon = (HICON)-1;

				for(ShellEntrySet::const_iterator it=sme._entries.begin(); it!=sme._entries.end(); ++it) {
					const Entry* entry = *it;

					HICON hIcon = extract_icon(entry);

					if (hIcon) {
						btn._hIcon = hIcon;
						break;
					}
				}

				if (btn._hIcon != (HICON)-1) {
					RECT rect;
					GetButtonRect(btn._id, &rect);
					WindowCanvas canvas(_hwnd);
					DrawStartMenuButton(canvas, rect, NULL, btn._hIcon, btn._hasSubmenu, btn._enabled, btn._id==_selected_id, false);
					//InvalidateRect(_hwnd, &rect, FALSE);
					//UpdateWindow(_hwnd);
					break;
				}
			}
		}

		if (++idx < (int)_buttons.size())
			PostMessage(_hwnd, PM_UPDATE_ICONS, idx, 0);

		return;
	}
#else
	int icons_extracted = 0;
	int icons_updated = 0;

	for(StartMenuShellDirs::iterator it=_dirs.begin(); it!=_dirs.end(); ++it) {
		ShellDirectory& dir = it->_dir;

		icons_extracted += dir.extract_icons();
	}

	if (icons_extracted) {
		for(ShellEntryMap::iterator it1=_entries.begin(); it1!=_entries.end(); ++it1) {
			StartMenuEntry& sme = it1->second;

			if (!sme._hIcon) {
				sme._hIcon = (HICON)-1;

				for(ShellEntrySet::const_iterator it2=sme._entries.begin(); it2!=sme._entries.end(); ++it2) {
					const Entry* sm_entry = *it2;

					if (sm_entry->_hIcon) {
						sme._hIcon = sm_entry->_hIcon;
						break;
					}
				}
			}
		}

		for(SMBtnVector::iterator it=_buttons.begin(); it!=_buttons.end(); ++it) {
			SMBtnInfo& info = *it;

			if (info._id>0 && !info._hIcon) {
				info._hIcon = _entries[info._id]._hIcon;
				++icons_updated;
			}
		}
	}

	if (icons_updated) {
		InvalidateRect(_hwnd, NULL, FALSE);
		UpdateWindow(_hwnd);
	}
#endif
}
#endif


 // resize child button controls to accomodate for new window size
void StartMenu::ResizeButtons(int cx)
{
	HDWP hdwp = BeginDeferWindowPos(10);

	for(HWND ctrl=GetWindow(_hwnd,GW_CHILD); ctrl; ctrl=GetNextWindow(ctrl,GW_HWNDNEXT)) {
		ClientRect rt(ctrl);

		if (rt.right != cx) {
			int height = rt.bottom - rt.top;

			 // special handling for separator controls
			if (!height && (GetWindowStyle(ctrl)&SS_TYPEMASK)==SS_ETCHEDHORZ)
				height = 2;

			hdwp = DeferWindowPos(hdwp, ctrl, 0, 0, 0, cx, height, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}

	EndDeferWindowPos(hdwp);
}


int StartMenu::Command(int id, int code)
{
	switch(id) {
	  case IDCANCEL:
		DestroyWindow(_hwnd);
		break;

	  default: {
		ShellEntryMap::iterator found = _entries.find(id);

		if (found != _entries.end()) {
			ActivateEntry(id, found->second._entries);
			break;
		}

		return super::Command(id, code);}
	}

	return 0;
}


StartMenuEntry& StartMenu::AddEntry(const String& title, HICON hIcon, const Entry* entry)
{
	 // search for an already existing subdirectory entry with the same name
	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		for(ShellEntryMap::iterator it=_entries.begin(); it!=_entries.end(); ++it) {
			StartMenuEntry& sme = it->second;

			if (sme._title == title)	///@todo speed up by using a map indexed by name
				for(ShellEntrySet::iterator it2=sme._entries.begin(); it2!=sme._entries.end(); ++it2) {
					if ((*it2)->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						 // merge the new shell entry with the existing of the same name
						sme._entries.insert(entry);
						return sme;
					}
				}
		}

	StartMenuEntry& sme = AddEntry(title, hIcon);

	sme._entries.insert(entry);

	return sme;
}

StartMenuEntry& StartMenu::AddEntry(const String& title, HICON hIcon, int id)
{
	if (id == -1)
		id = ++_next_id;

	StartMenuEntry& sme = _entries[id];

	sme._title = title;
	sme._hIcon = hIcon;

	return sme;
}

StartMenuEntry& StartMenu::AddEntry(const ShellFolder folder, const ShellEntry* entry)
{
	HICON hIcon = entry->_hIcon;

	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		hIcon = SmallIcon(IDI_FOLDER);

	return AddEntry(folder.get_name(entry->_pidl), hIcon, entry);
}

StartMenuEntry& StartMenu::AddEntry(const ShellFolder folder, const Entry* entry)
{
	HICON hIcon = entry->_hIcon;

	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		hIcon = SmallIcon(IDI_FOLDER);

	return AddEntry(entry->_display_name, hIcon, entry);
}


void StartMenu::AddButton(LPCTSTR title, HICON hIcon, bool hasSubmenu, int id, bool enabled)
{
#ifdef _LIGHT_STARTMENU
	_buttons.push_back(SMBtnInfo(title, hIcon, id, hasSubmenu, enabled));
#else
	DWORD style = enabled? WS_VISIBLE|WS_CHILD|BS_OWNERDRAW: WS_VISIBLE|WS_CHILD|BS_OWNERDRAW|WS_DISABLED;

	WindowRect rect(_hwnd);
	ClientRect clnt(_hwnd);

	 // increase window height to make room for the new button
	rect.top -= STARTMENU_LINE_HEIGHT;

	 // move down if we are too high now
	if (rect.top < 0) {
		rect.top += STARTMENU_LINE_HEIGHT;
		rect.bottom += STARTMENU_LINE_HEIGHT;
	}

	WindowCanvas canvas(_hwnd);
	FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));

	 // widen window, if it is too small
	int text_width = GetStartMenuBtnTextWidth(canvas, title, _hwnd) + 16/*icon*/ + 10/*placeholder*/ + 16/*arrow*/;

	int cx = clnt.right - _border_left;
	if (text_width > cx)
		rect.right += text_width-cx;

	MoveWindow(_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);

	StartMenuCtrl(_hwnd, _border_left, clnt.bottom, rect.right-rect.left-_border_left,
					title, id, hIcon, hasSubmenu, style);
#endif
}

void StartMenu::AddSeparator()
{
#ifdef _LIGHT_STARTMENU
	_buttons.push_back(SMBtnInfo(NULL, 0, -1, false));
#else
	WindowRect rect(_hwnd);
	ClientRect clnt(_hwnd);

	 // increase window height to make room for the new separator
	rect.top -= STARTMENU_SEP_HEIGHT;

	 // move down if we are too high now
	if (rect.top < 0) {
		rect.top += STARTMENU_LINE_HEIGHT;
		rect.bottom += STARTMENU_LINE_HEIGHT;
	}

	MoveWindow(_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);

	StartMenuSeparator(_hwnd, _border_left, clnt.bottom, rect.right-rect.left-_border_left);
#endif
}


bool StartMenu::CloseOtherSubmenus(int id)
{
	if (_submenu) {
		if (IsWindow(_submenu)) {
			if (_submenu_id == id)
				return false;
			else {
				_submenu_id = 0;
				DestroyWindow(_submenu);
				// _submenu should be reset automatically by PM_STARTMENU_CLOSED, but safety first...
			}
		}

		_submenu = 0;
	}

	return true;
}


void StartMenu::CreateSubmenu(int id, LPCTSTR title, CREATORFUNC creator)
{
	CreateSubmenu(id, StartMenuFolders(), title, creator);
}

void StartMenu::CreateSubmenu(int id, int folder_id, LPCTSTR title, CREATORFUNC creator)
{
	try {
		SpecialFolderPath folder(folder_id, _hwnd);

		StartMenuFolders new_folders;
		new_folders.push_back(folder);

		CreateSubmenu(id, new_folders, title, creator);
	} catch(COMException&) {
		// ignore Exception and don't display anything
	}
}

void StartMenu::CreateSubmenu(int id, int folder_id1, int folder_id2, LPCTSTR title, CREATORFUNC creator)
{
	StartMenuFolders new_folders;

	try {
		new_folders.push_back(SpecialFolderPath(folder_id1, _hwnd));
	} catch(COMException&) {
	}

	try {
		new_folders.push_back(SpecialFolderPath(folder_id2, _hwnd));
	} catch(COMException&) {
	}

	if (!new_folders.empty())
		CreateSubmenu(id, new_folders, title, creator);
}

void StartMenu::CreateSubmenu(int id, const StartMenuFolders& new_folders, LPCTSTR title, CREATORFUNC creator)
{
	 // Only open one submenu at a time.
	if (!CloseOtherSubmenus(id))
		return;

	RECT rect;
	int x, y;

	if (GetButtonRect(id, &rect)) {
		ClientToScreen(_hwnd, &rect);

		x = rect.right;	// Submenus should overlap their parent a bit.
		y = rect.top+STARTMENU_LINE_HEIGHT-_border_top;
	} else {
		WindowRect pos(_hwnd);

		x = pos.right;
		y = pos.top;
	}

	_submenu_id = id;
	_submenu = StartMenu::Create(x, y, new_folders, _hwnd, title, creator);
}


void StartMenu::ActivateEntry(int id, const ShellEntrySet& entries)
{
	StartMenuFolders new_folders;
	String title;

	for(ShellEntrySet::const_iterator it=entries.begin(); it!=entries.end(); ++it) {
		Entry* entry = const_cast<Entry*>(*it);

		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

			///@todo If the user explicitely clicked on a submenu, display this folder as floating start menu.

			if (entry->_etype == ET_SHELL)
				new_folders.push_back(static_cast<const ShellEntry*>(entry)->create_absolute_pidl());
			else {
				TCHAR path[MAX_PATH];

				if (entry->get_path(path))
					new_folders.push_back(path);
			}

			if (title.empty())
				title = entry->_display_name;
		} else {
			 // If the entry is no subdirectory, there can only be one shell entry.
			assert(entries.size()==1);

			entry->launch_entry(_hwnd);	///@todo launch in the background; specify correct HWND for error message box titles

			 // close start menus after launching the selected entry
			CloseStartMenu(id);

			 // we deleted 'this' - ensure we leave loop and function
			return;
		}
	}

	if (!new_folders.empty()) {
		 // Only open one submenu at a time.
		if (!CloseOtherSubmenus(id))
			return;

		CreateSubmenu(id, new_folders, title);
	}
}


 /// close all windows of the start menu popup
void StartMenu::CloseStartMenu(int id)
{
	if (!(GetWindowStyle(_hwnd) & WS_CAPTION)) {	// don't automatically close floating menus
		if (!SendParent(PM_STARTENTRY_LAUNCHED, id, (LPARAM)_hwnd))
			DestroyWindow(_hwnd);
	} else if (_submenu)	// instead close submenus of floating parent menus
		CloseSubmenus();
}


int GetStartMenuBtnTextWidth(HDC hdc, LPCTSTR title, HWND hwnd)
{
	RECT rect = {0, 0, 0, 0};
	DrawText(hdc, title, -1, &rect, DT_SINGLELINE|DT_NOPREFIX|DT_CALCRECT);

	return rect.right-rect.left;
}

void DrawStartMenuButton(HDC hdc, const RECT& rect, LPCTSTR title, HICON hIcon,
						 bool hasSubmenu, bool enabled, bool has_focus, bool pushed)
{
	UINT style = DFCS_BUTTONPUSH;

	if (!enabled)
		style |= DFCS_INACTIVE;

	POINT iconPos = {rect.left+2, (rect.top+rect.bottom-16)/2};
	RECT textRect = {rect.left+16+4, rect.top+2, rect.right-4, rect.bottom-4};

	if (pushed) {
		style |= DFCS_PUSHED;
		++iconPos.x;		++iconPos.y;
		++textRect.left;	++textRect.top;
		++textRect.right;	++textRect.bottom;
	}

	int bk_color = COLOR_BTNFACE;
	int text_color = COLOR_BTNTEXT;

	if (has_focus) {
		bk_color = COLOR_HIGHLIGHT;
		text_color = COLOR_HIGHLIGHTTEXT;
	}

	HBRUSH bk_brush = GetSysColorBrush(bk_color);

	if (title)
		FillRect(hdc, &rect, bk_brush);

	DrawIconEx(hdc, iconPos.x, iconPos.y, hIcon, 16, 16, 0, bk_brush, DI_NORMAL);

	 // draw submenu arrow at the right
	if (hasSubmenu) {
		static SmallIcon arrowIcon(IDI_ARROW);
		static SmallIcon selArrowIcon(IDI_ARROW_SELECTED);

		DrawIconEx(hdc, rect.right-16, iconPos.y,
					has_focus? selArrowIcon: arrowIcon,
					16, 16, 0, bk_brush, DI_NORMAL);
	}

	if (title) {
		BkMode bk_mode(hdc, TRANSPARENT);

		if (!enabled)	// dis->itemState & (ODS_DISABLED|ODS_GRAYED)
			DrawGrayText(hdc, &textRect, title, DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
		else {
			TextColor lcColor(hdc, GetSysColor(text_color));
			DrawText(hdc, title, -1, &textRect, DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
		}
	}
}


#ifdef _LIGHT_STARTMENU

void StartMenu::ResizeToButtons()
{
	WindowRect rect(_hwnd);

	WindowCanvas canvas(_hwnd);
	FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));

	int max_width = STARTMENU_WIDTH_MIN;
	int height = 0;

	for(SMBtnVector::const_iterator it=_buttons.begin(); it!=_buttons.end(); ++it) {
		int w = GetStartMenuBtnTextWidth(canvas, it->_title, _hwnd);

		if (w > max_width)
			max_width = w;

		if (it->_id == -1)
			height += STARTMENU_SEP_HEIGHT;
		else
			height += STARTMENU_LINE_HEIGHT;
	}

	 // calculate new window size
	int text_width = max_width + 16/*icon*/ + 10/*placeholder*/ + 16/*arrow*/;

	RECT rt_hgt = {rect.left, rect.bottom-_border_top-height, rect.left+_border_left+text_width, rect.bottom};
	AdjustWindowRectEx(&rt_hgt, GetWindowStyle(_hwnd), FALSE, GetWindowExStyle(_hwnd));

	 // ignore movement, only look at the size change
	rect.right = rect.left + (rt_hgt.right-rt_hgt.left);
	rect.top = rect.bottom - (rt_hgt.bottom-rt_hgt.top);

	 // move down if we are too high
	if (rect.top < 0) {
		rect.top += STARTMENU_LINE_HEIGHT;
		rect.bottom += STARTMENU_LINE_HEIGHT;
	}

	MoveWindow(_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);
}

#else // _LIGHT_STARTMENU

LRESULT StartMenuButton::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_MOUSEMOVE:
		 // automatically set the focus to startmenu entries when moving the mouse over them
		if (GetFocus()!=_hwnd && !(GetWindowStyle(_hwnd)&WS_DISABLED))
			SetFocus(_hwnd);
		break;

	  case WM_SETFOCUS:
		PostParent(PM_STARTENTRY_FOCUSED, _hasSubmenu, (LPARAM)_hwnd);
		goto def;

	  case WM_CANCELMODE:
		 // route WM_CANCELMODE to the startmenu window
		return SendParent(nmsg, wparam, lparam);

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

void StartMenuButton::DrawItem(LPDRAWITEMSTRUCT dis)
{
	TCHAR title[BUFFER_LEN];

	GetWindowText(_hwnd, title, BUFFER_LEN);

	DrawStartMenuButton(dis->hDC, dis->rcItem, title, _hIcon,
						_hasSubmenu,
						!(dis->itemState & ODS_DISABLED),
						dis->itemState&ODS_FOCUS? true: false,
						dis->itemState&ODS_SELECTED? true: false);
}

#endif


StartMenuRoot::StartMenuRoot(HWND hwnd)
 :	super(hwnd)
{
#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NOCOMMONGROUPS))
#endif
		try {
			 // insert directory "All Users\Start Menu"
			ShellDirectory cmn_startmenu(Desktop(), SpecialFolderPath(CSIDL_COMMON_STARTMENU, _hwnd), _hwnd);
			_dirs.push_back(StartMenuDirectory(cmn_startmenu, false));	// don't add subfolders
		} catch(COMException&) {
			// ignore exception and don't show additional shortcuts
		}

	try {
		 // insert directory "<user name>\Start Menu"
		ShellDirectory usr_startmenu(Desktop(), SpecialFolderPath(CSIDL_STARTMENU, _hwnd), _hwnd);
		_dirs.push_back(StartMenuDirectory(usr_startmenu, false));	// don't add subfolders
	} catch(COMException&) {
		// ignore exception and don't show additional shortcuts
	}

	 // read size of logo bitmap
	BITMAP bmp_hdr;
	GetObject(ResBitmap(IDB_LOGOV), sizeof(BITMAP), &bmp_hdr);
	_logo_size.cx = bmp_hdr.bmWidth;
	_logo_size.cy = bmp_hdr.bmHeight;

	_border_left = _logo_size.cx;
}


HWND StartMenuRoot::Create(HWND hwndDesktopBar)
{
	WindowRect pos(hwndDesktopBar);

	RECT rect = {pos.left, pos.top-STARTMENU_LINE_HEIGHT-4, pos.left+STARTMENU_WIDTH_MIN, pos.top};

#ifndef _LIGHT_STARTMENU
	rect.top += STARTMENU_LINE_HEIGHT;
#endif

	AdjustWindowRectEx(&rect, WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, FALSE, 0);

	return Window::Create(WINDOW_CREATOR(StartMenuRoot), 0, GetWndClasss(), TITLE_STARTMENU,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE,
							rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, hwndDesktopBar);
}


void StartMenuRoot::TrackStartmenu()
{
	MSG msg;
	HWND hwnd = _hwnd;

	while(IsWindow(hwnd)) {
		if (!GetMessage(&msg, 0, 0, 0)) {
			PostQuitMessage(msg.wParam);
			break;
		}

		 // Check for a mouse click on any window, which is not part of the start menu
		if (msg.message==WM_LBUTTONDOWN || msg.message==WM_MBUTTONDOWN || msg.message==WM_RBUTTONDOWN) {
			StartMenu* menu_wnd = NULL;

			for(HWND hwnd=msg.hwnd; hwnd; hwnd=GetParent(hwnd)) {
				menu_wnd = WINDOW_DYNAMIC_CAST(StartMenu, hwnd);

				if (menu_wnd)
					break;
			}

			if (!menu_wnd) {
				DestroyWindow(_hwnd);
				break;
			}
		}

		try {
			if (pretranslate_msg(&msg))
				continue;

			if (dispatch_dialog_msg(&msg))
				continue;

			TranslateMessage(&msg);

			try {
				DispatchMessage(&msg);
			} catch(COMException& e) {
				HandleException(e, _hwnd);
			}
		} catch(COMException& e) {
			HandleException(e, _hwnd);
		}
	}
}


LRESULT	StartMenuRoot::Init(LPCREATESTRUCT pcs)
{
	 // add buttons for entries in _entries
	if (super::Init(pcs))
		return 1;

	AddSeparator();


#ifdef __MINGW32__
	HKEY hkey, hkeyAdv;
	DWORD value, len;

	if (RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), &hkey))
		hkey = 0;

	if (RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), &hkeyAdv))
		hkeyAdv = 0;

#define	IS_VALUE_ZERO(hk, name) \
	(!hk || (len=sizeof(value),RegQueryValueEx(hk, name, NULL, NULL, (LPBYTE)&value, &len) || !value))

#define	IS_VALUE_NOT_ZERO(hk, name) \
	(!hk || (len=sizeof(value),RegQueryValueEx(hk, name, NULL, NULL, (LPBYTE)&value, &len) || value>0))
#endif


	 // insert hard coded start entries
	AddButton(ResString(IDS_PROGRAMS),		SmallIcon(IDI_APPS), true, IDC_PROGRAMS);

	AddButton(ResString(IDS_DOCUMENTS),		SmallIcon(IDI_DOCUMENTS), true, IDC_DOCUMENTS);

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NORECENTDOCSMENU))
#else
	if (IS_VALUE_ZERO(hkey, _T("NoRecentDocsMenu")))
#endif
		AddButton(ResString(IDS_RECENT),	SmallIcon(IDI_DOCUMENTS), true, IDC_RECENT);

	AddButton(ResString(IDS_FAVORITES),		SmallIcon(IDI_FAVORITES), true, IDC_FAVORITES);

	AddButton(ResString(IDS_SETTINGS),		SmallIcon(IDI_CONFIG), true, IDC_SETTINGS);

	AddButton(ResString(IDS_BROWSE),		SmallIcon(IDI_FOLDER), true, IDC_BROWSE);

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NOFIND))
#else
	if (IS_VALUE_ZERO(hkey, _T("NoFind")))
#endif
		AddButton(ResString(IDS_SEARCH),	SmallIcon(IDI_SEARCH), true, IDC_SEARCH);

	AddButton(ResString(IDS_START_HELP),	SmallIcon(IDI_INFO), false, IDC_START_HELP);

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NORUN))
#else
	if (IS_VALUE_ZERO(hkey, _T("NoRun")))
#endif
		AddButton(ResString(IDS_LAUNCH),	SmallIcon(IDI_ACTION), false, IDC_LAUNCH);


	AddSeparator();


#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NOCLOSE))
#else
	if (IS_VALUE_NOT_ZERO(hkeyAdv, _T("StartMenuLogoff")))
#endif
		AddButton(ResString(IDS_LOGOFF),	SmallIcon(IDI_LOGOFF), false, IDC_LOGOFF);


#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || SHRestricted(REST_STARTMENULOGOFF) != 1)
#else
	if (IS_VALUE_ZERO(hkey, _T("NoClose")))
#endif
		AddButton(ResString(IDS_SHUTDOWN),	SmallIcon(IDI_LOGOFF), false, IDC_SHUTDOWN);


#ifdef __MINGW32__
	RegCloseKey(hkeyAdv);
	RegCloseKey(hkey);
#endif


#ifdef _LIGHT_STARTMENU
	 // set the window size to fit all buttons
	ResizeToButtons();
#endif

	return 0;
}


void StartMenuRoot::AddEntries()
{
	super::AddEntries();

	AddButton(ResString(IDS_EXPLORE),	SmallIcon(IDI_EXPLORER), false, IDC_EXPLORE);
}


LRESULT	StartMenuRoot::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);
		Paint(canvas);
		break;}

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

void StartMenuRoot::Paint(PaintCanvas& canvas)
{
	int clr_bits;
	{WindowCanvas dc(_hwnd); clr_bits=GetDeviceCaps(dc, BITSPIXEL);}
	bool logo256 = clr_bits<=8;

	MemCanvas mem_dc;
	ResBitmap bmp(logo256? IDB_LOGOV256: IDB_LOGOV);
	BitmapSelection sel(mem_dc, bmp);

	ClientRect clnt(_hwnd);
	int h = min(_logo_size.cy, clnt.bottom);

	RECT rect = {0, 0, _logo_size.cx-1, clnt.bottom-h};
	HBRUSH hbr = CreateSolidBrush(logo256? RGB(166,202,240): RGB(255,255,255));	// same color as the background color in the logo bitmap
	FillRect(canvas, &rect, hbr);
	DeleteObject(hbr);
	//PatBlt(canvas, _logo_size.cx-1, 0, 1, clnt.bottom-h, WHITENESS);
	PatBlt(canvas, _logo_size.cx-1, 0, 1, clnt.bottom-h, WHITENESS);

	BitBlt(canvas, 0, clnt.bottom-h, _logo_size.cx, h, mem_dc, 0, 0, SRCCOPY);

	if (!logo256) {
		rect.left = rect.right++;
		rect.bottom = clnt.bottom;
		HBRUSH hbr_border = GetStockBrush(GRAY_BRUSH);	//CreateSolidBrush(RGB(71,88,85));
		FillRect(canvas, &rect, hbr_border);
		//DeleteObject(hbr_border);
	}

	super::Paint(canvas);
}


int StartMenuHandler::Command(int id, int code)
{
	switch(id) {

	// start menu root

	  case IDC_PROGRAMS:
		CreateSubmenu(id, CSIDL_COMMON_PROGRAMS, CSIDL_PROGRAMS, ResString(IDS_PROGRAMS));
		break;

	  case IDC_EXPLORE:
		CloseStartMenu(id);
		explorer_show_frame(_hwnd, SW_SHOWNORMAL);
		break;

	  case IDC_LAUNCH: {
		HWND hwndDesktopBar = GetWindow(_hwnd, GW_OWNER);
		CloseStartMenu(id);
		ShowLaunchDialog(hwndDesktopBar);
		break;}

	  case IDC_DOCUMENTS:
		CreateSubmenu(id, CSIDL_PERSONAL, ResString(IDS_DOCUMENTS));
		break;

	  case IDC_RECENT:
		CreateSubmenu(id, CSIDL_RECENT, ResString(IDS_RECENT), STARTMENU_CREATOR(RecentStartMenu));
		break;

	  case IDC_FAVORITES:
		CreateSubmenu(id, CSIDL_FAVORITES, ResString(IDS_FAVORITES));
		break;

	  case IDC_BROWSE:
		CreateSubmenu(id, ResString(IDS_BROWSE), STARTMENU_CREATOR(BrowseMenu));
		break;

	  case IDC_SETTINGS:
		CreateSubmenu(id, ResString(IDS_SETTINGS), STARTMENU_CREATOR(SettingsMenu));
		break;

	  case IDC_SEARCH:
		CreateSubmenu(id, ResString(IDS_SEARCH), STARTMENU_CREATOR(SearchMenu));
		break;

	  case IDC_START_HELP: {
		HWND hwndDesktopBar = GetWindow(_hwnd, GW_OWNER);
		CloseStartMenu(id);
		MessageBox(hwndDesktopBar, TEXT("Help not yet implemented"), ResString(IDS_TITLE), MB_OK);
		break;}

	  case IDC_LOGOFF:
		/* The shell32 Dialog prompts about some system setting change. This is not what we want to display here.
		HWND hwndDesktopBar = GetWindow(_hwnd, GW_OWNER);
		CloseStartMenu(id);
		ShowRestartDialog(hwndDesktopBar, EWX_LOGOFF);*/
		DestroyWindow(GetParent(_hwnd));
		break;

	  case IDC_SHUTDOWN: {
		HWND hwndDesktopBar = GetWindow(_hwnd, GW_OWNER);
		CloseStartMenu(id);
		ShowExitWindowsDialog(hwndDesktopBar);
		break;}


	// settings menu

	  case IDC_SETTINGS_MENU:
		CreateSubmenu(id, CSIDL_CONTROLS, ResString(IDS_SETTINGS_MENU));
		break;

	  case IDC_PRINTERS:
		CreateSubmenu(id, CSIDL_PRINTERS, CSIDL_PRINTHOOD, ResString(IDS_PRINTERS));
		break;

	  case IDC_CONTROL_PANEL:
		CloseStartMenu(id);
		MainFrame::Create(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}"), FALSE);
		break;

	  case IDC_ADMIN:
		CreateSubmenu(id, CSIDL_COMMON_ADMINTOOLS, CSIDL_ADMINTOOLS, ResString(IDS_ADMIN));
		break;

	  case IDC_CONNECTIONS:
		CreateSubmenu(id, CSIDL_CONNECTIONS, ResString(IDS_CONNECTIONS));
		break;


	// browse menu

	  case IDC_NETWORK:
		CreateSubmenu(id, CSIDL_NETWORK, ResString(IDS_NETWORK));
		break;

	  case IDC_DRIVES:
		///@todo exclude removeable drives
		CreateSubmenu(id, CSIDL_DRIVES, ResString(IDS_DRIVES));
		break;


	// search menu

	  case IDC_SEARCH_PROGRAM:
		CloseStartMenu(id);
		Dialog::DoModal(IDD_SEARCH_PROGRAM, WINDOW_CREATOR(FindProgramDlg));
		break;

	  case IDC_SEARCH_FILES:
		CloseStartMenu(id);
		ShowSearchDialog();
		break;

	  case IDC_SEARCH_COMPUTER:
		CloseStartMenu(id);
		ShowSearchComputer();
		break;


	  default:
		return super::Command(id, code);
	}

	return 0;
}


void StartMenuHandler::ShowSearchDialog()
{
	static DynamicFct<SHFINDFILES> SHFindFiles(TEXT("SHELL32"), 90);

	if (SHFindFiles)
		(*SHFindFiles)(NULL, NULL);
	else
		MessageBox(0, TEXT("SHFindFiles() not yet implemented in SHELL32"), ResString(IDS_TITLE), MB_OK);
}

void StartMenuHandler::ShowSearchComputer()
{
	static DynamicFct<SHFINDCOMPUTER> SHFindComputer(TEXT("SHELL32"), 91);

	if (SHFindComputer)
		(*SHFindComputer)(NULL, NULL);
	else
		MessageBox(0, TEXT("SHFindComputer() not yet implemented in SHELL32"), ResString(IDS_TITLE), MB_OK);
}

void StartMenuHandler::ShowLaunchDialog(HWND hwndDesktopBar)
{
	 ///@todo All text phrases should be put into the resources.
	static LPCSTR szTitle = "Create New Task";
	static LPCSTR szText = "Type the name of a program, folder, document, or Internet resource, and Task Manager will open it for you.";

	static DynamicFct<RUNFILEDLG> RunFileDlg(TEXT("SHELL32"), 61);

	 // Show "Run..." dialog
	if (RunFileDlg) {
#define	W_VER_NT 0
		if ((HIWORD(GetVersion())>>14) == W_VER_NT) {
			WCHAR wTitle[40], wText[256];

			MultiByteToWideChar(CP_ACP, 0, szTitle, -1, wTitle, 40);
			MultiByteToWideChar(CP_ACP, 0, szText, -1, wText, 256);

			(*RunFileDlg)(hwndDesktopBar, 0, NULL, (LPCSTR)wTitle, (LPCSTR)wText, RFF_CALCDIRECTORY);
		}
		else
			(*RunFileDlg)(hwndDesktopBar, 0, NULL, szTitle, szText, RFF_CALCDIRECTORY);
	}
}

void StartMenuHandler::ShowRestartDialog(HWND hwndOwner, UINT flags)
{
	static DynamicFct<RESTARTWINDOWSDLG> RestartDlg(TEXT("SHELL32"), 59);

	if (RestartDlg)
		(*RestartDlg)(hwndOwner, (LPWSTR)L"You selected <Log Off>.\n\n", flags);	///@todo ANSI string conversion if needed
	else
		MessageBox(hwndOwner, TEXT("RestartDlg() not yet implemented in SHELL32"), ResString(IDS_TITLE), MB_OK);
}

void ShowExitWindowsDialog(HWND hwndOwner)
{
	static DynamicFct<EXITWINDOWSDLG> ExitWindowsDlg(TEXT("SHELL32"), 60);

	if (ExitWindowsDlg)
		(*ExitWindowsDlg)(hwndOwner);
	else
		MessageBox(hwndOwner, TEXT("ExitWindowsDlg() not yet implemented in SHELL32"), ResString(IDS_TITLE), MB_OK);
}


void SettingsMenu::AddEntries()
{
	super::AddEntries();

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NOCONTROLPANEL))
#endif
		AddButton(ResString(IDS_CONTROL_PANEL),	SmallIcon(IDI_CONFIG), false, IDC_CONTROL_PANEL);

	AddButton(ResString(IDS_PRINTERS),		SmallIcon(IDI_PRINTER), true, IDC_PRINTERS);
	AddButton(ResString(IDS_CONNECTIONS),	SmallIcon(IDI_NETWORK), true, IDC_CONNECTIONS);
	AddButton(ResString(IDS_ADMIN),			SmallIcon(IDI_CONFIG), true, IDC_ADMIN);

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NOCONTROLPANEL))
#endif
		AddButton(ResString(IDS_SETTINGS_MENU),	SmallIcon(IDI_CONFIG), true, IDC_SETTINGS_MENU);
}

void BrowseMenu::AddEntries()
{
	super::AddEntries();

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_NONETHOOD))	// or REST_NOENTIRENETWORK ?
#endif
		AddButton(ResString(IDS_NETWORK),	SmallIcon(IDI_NETWORK), true, IDC_NETWORK);

	AddButton(ResString(IDS_DRIVES),	SmallIcon(IDI_FOLDER), true, IDC_DRIVES);
}

void SearchMenu::AddEntries()
{
	super::AddEntries();

	AddButton(ResString(IDS_SEARCH_PRG),	SmallIcon(IDI_APPS), false, IDC_SEARCH_PROGRAM);

	AddButton(ResString(IDS_SEARCH_FILES),	SmallIcon(IDI_SEARCH_DOC), false, IDC_SEARCH_FILES);

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_HASFINDCOMPUTERS))
#endif
		AddButton(ResString(IDS_SEARCH_COMPUTER),	SmallIcon(IDI_COMPUTER), false, IDC_SEARCH_COMPUTER);
}


void RecentStartMenu::AddEntries()
{
	for(StartMenuShellDirs::iterator it=_dirs.begin(); it!=_dirs.end(); ++it) {
		StartMenuDirectory& smd = *it;
		ShellDirectory& dir = smd._dir;

		if (!dir._scanned) {
			WaitCursor wait;

#ifdef _LAZY_ICONEXTRACT
			dir.smart_scan(SCAN_FILESYSTEM);
#else
			dir.smart_scan(SCAN_EXTRACT_ICONS|SCAN_FILESYSTEM);
#endif
		}

		dir.sort_directory(SORT_DATE);
		AddShellEntries(dir, 16, smd._subfolders);	///@todo read max. count of entries from registry
	}
}
