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
 // globals.h
 //
 // Martin Fuchs, 23.07.2003
 //


 /// management of file types
struct FileTypeInfo {
	String	_classname;
	bool	_neverShowExt;
};

struct FileTypeManager : public map<String, FileTypeInfo>
{
	typedef map<String, FileTypeInfo> super;

	const FileTypeInfo& operator[](String ext);
};


enum ICON_TYPE {
	IT_STATIC,
	IT_CACHED,
	IT_DYNAMIC,
	IT_SYSCACHE
};

enum ICON_ID {
	ICID_UNKNOWN,
	ICID_NONE,

	ICID_FOLDER,
	//ICID_DOCUMENT,
	ICID_APP,
	ICID_EXPLORER,

	ICID_CONFIG,
	ICID_DOCUMENTS,
	ICID_FAVORITES,
	ICID_INFO,
	ICID_APPS,
	ICID_SEARCH,
	ICID_ACTION,
	ICID_SEARCH_DOC,
	ICID_PRINTER,
	ICID_NETWORK,
	ICID_COMPUTER,
	ICID_LOGOFF,

	ICID_DYNAMIC
};

struct Icon {
	ICON_ID	_id;
	ICON_TYPE _itype;
	HICON	_hIcon;

	Icon();
	Icon(ICON_ID id, UINT nid);
	Icon(ICON_TYPE itype, int id, HICON hIcon);
};

struct IconCache {
	void	init();

	const Icon&	extract(String path);
	const Icon&	extract(LPCTSTR path, int idx);
	const Icon&	extract(IExtractIcon* pExtract, LPCTSTR path, int idx);

	const Icon&	add(HICON hIcon, ICON_TYPE type=IT_DYNAMIC);

	const Icon&	get_icon(int icon_id);
	HBITMAP	get_icon_bitmap(int icon_id, HBRUSH hbrBkgnd, HDC hdc);

	void	free_icon(int icon_id);

protected:
	static int s_next_id;

	typedef map<int, Icon> IconMap;
	IconMap	_icons;

	typedef map<String, ICON_ID> PathMap;
	PathMap	_pathMap;

	typedef pair<String, int> CachePair;
	typedef map<CachePair, ICON_ID> PathIdxMap;
	PathIdxMap _pathIdxMap;
};


 /// create a bitmap from an icon
extern HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd);


 /// structure containing global variables of Explorer
extern struct ExplorerGlobals
{
	ExplorerGlobals();

	void		init(HINSTANCE hInstance);

	HINSTANCE	_hInstance;
	ATOM		_hframeClass;
	UINT		_cfStrFName;
	HWND		_hMainWnd;
	bool		_prescan_nodes;
	bool		_desktop_mode;

	FILE*		_log;

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	DWORD(STDAPICALLTYPE* _SHRestricted)(RESTRICTIONS);
#endif

	FileTypeManager	_ftype_mgr;
	IconCache	_icon_cache;
} g_Globals;


 /// convenient loading of string resources
struct ResString : public String
{
	ResString(UINT nid);
};

 /// convenient loading of standard (32x32) icon resources
struct ResIcon
{
	ResIcon(UINT nid);

	operator HICON() const {return _hIcon;}

protected:
	HICON	_hIcon;
};

 /// convenient loading of small (16x16) icon resources
struct SmallIcon
{
	SmallIcon(UINT nid);

	operator HICON() const {return _hIcon;}

protected:
	HICON	_hIcon;
};

 /// convenient loading of icon resources with specified sizes
struct ResIconEx
{
	ResIconEx(UINT nid, int w, int h);

	operator HICON() const {return _hIcon;}

protected:
	HICON	_hIcon;
};

 /// set big and small icons out of the resources for a window
extern void SetWindowIcon(HWND hwnd, UINT nid);

 /// convenient loading of bitmap resources
struct ResBitmap
{
	ResBitmap(UINT nid);
	~ResBitmap() {DeleteObject(_hBmp);}

	operator HBITMAP() const {return _hBmp;}

protected:
	HBITMAP	_hBmp;
};
