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
 // shellfs.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "../utility/utility.h"
#include "../utility/shellclasses.h"

#include "../globals.h"

#include "entries.h"
#include "shellfs.h"
#include "winfs.h"

#include <shlwapi.h>


bool ShellDirectory::fill_w32fdata_shell(LPCITEMIDLIST pidl, SFGAOF attribs, WIN32_FIND_DATA* pw32fdata, BY_HANDLE_FILE_INFORMATION* pbhfi, bool do_access)
{
	CONTEXT("ShellDirectory::fill_w32fdata_shell()");

	bool bhfi_valid = false;

	if (do_access && !( (attribs&SFGAO_FILESYSTEM) && SUCCEEDED(
				SHGetDataFromIDList(_folder, pidl, SHGDFIL_FINDDATA, pw32fdata, sizeof(WIN32_FIND_DATA))) )) {
		WIN32_FILE_ATTRIBUTE_DATA fad;
		IDataObject* pDataObj;

		STGMEDIUM medium = {0, {0}, 0};
		FORMATETC fmt = {g_Globals._cfStrFName, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

		HRESULT hr = _folder->GetUIObjectOf(0, 1, &pidl, IID_IDataObject, 0, (LPVOID*)&pDataObj);

		if (SUCCEEDED(hr)) {
			hr = pDataObj->GetData(&fmt, &medium);

			pDataObj->Release();

			if (SUCCEEDED(hr)) {
				LPCTSTR path = (LPCTSTR)GlobalLock(medium.UNION_MEMBER(hGlobal));
				UINT sem_org = SetErrorMode(SEM_FAILCRITICALERRORS);

				 // fill with drive names "C:", ...
				_tcscpy(pw32fdata->cFileName, path);

				if (GetFileAttributesEx(path, GetFileExInfoStandard, &fad)) {
					pw32fdata->dwFileAttributes = fad.dwFileAttributes;
					pw32fdata->ftCreationTime = fad.ftCreationTime;
					pw32fdata->ftLastAccessTime = fad.ftLastAccessTime;
					pw32fdata->ftLastWriteTime = fad.ftLastWriteTime;

					if (!(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
						pw32fdata->nFileSizeLow = fad.nFileSizeLow;
						pw32fdata->nFileSizeHigh = fad.nFileSizeHigh;
					}
				}

				HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
											0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);

				if (hFile != INVALID_HANDLE_VALUE) {
					if (GetFileInformationByHandle(hFile, pbhfi))
						bhfi_valid = true;

					CloseHandle(hFile);
				}

				SetErrorMode(sem_org);

				GlobalUnlock(medium.UNION_MEMBER(hGlobal));
				GlobalFree(medium.UNION_MEMBER(hGlobal));
			}
		}
	}

	if (!do_access || !(attribs&SFGAO_FILESYSTEM))	// Archiv files should not be displayed as folders in explorer view.
		if (attribs & (SFGAO_FOLDER|SFGAO_HASSUBFOLDER))
			pw32fdata->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

	if (attribs & SFGAO_READONLY)
		pw32fdata->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

	if (attribs & SFGAO_COMPRESSED)
		pw32fdata->dwFileAttributes |= FILE_ATTRIBUTE_COMPRESSED;

	return bhfi_valid;
}


ShellPath ShellEntry::create_absolute_pidl() const
{
	CONTEXT("ShellEntry::create_absolute_pidl()");

	if (_up)
		if (_up->_etype==ET_SHELL) {
			ShellDirectory* dir = static_cast<ShellDirectory*>(_up);

			if (dir->_pidl->mkid.cb)	// Caching of absolute PIDLs could enhance performance.
				return _pidl.create_absolute_pidl(dir->create_absolute_pidl());
		} else {
			ShellPath shell_path;

			if (get_entry_pidl(_up, shell_path))
				return _pidl.create_absolute_pidl(shell_path);
		}

	return _pidl;
}


 // get full path of a shell entry
bool ShellEntry::get_path(PTSTR path) const
{
/*
	path[0] = TEXT('\0');

	if (FAILED(path_from_pidl(get_parent_folder(), &*_pidl, path, MAX_PATH)))
		return false;
*/
	FileSysShellPath fs_path(create_absolute_pidl());

	if (!(LPCTSTR)fs_path)
		return false;

	_tcscpy(path, fs_path);

	return true;
}


HRESULT ShellEntry::GetUIObjectOf(HWND hWnd, REFIID riid, LPVOID* ppvOut)
{
	LPCITEMIDLIST pidl = _pidl;

	return get_parent_folder()->GetUIObjectOf(hWnd, 1, &pidl, riid, NULL, ppvOut);
}


 // get full path of a shell folder
bool ShellDirectory::get_path(PTSTR path) const
{
	CONTEXT("ShellDirectory::get_path()");

	path[0] = TEXT('\0');

	SFGAOF attribs = 0;

	if (!_folder.empty())
		if (FAILED(const_cast<ShellFolder&>(_folder)->GetAttributesOf(1, (LPCITEMIDLIST*)&_pidl, &attribs)))
			return false;

	if (!(attribs & SFGAO_FILESYSTEM))
		return false;

	if (FAILED(path_from_pidl(get_parent_folder(), &*_pidl, path, MAX_PATH)))
		return false;

	return true;
}


BOOL ShellEntry::launch_entry(HWND hwnd, UINT nCmdShow)
{
	CONTEXT("ShellEntry::launch_entry()");

	SHELLEXECUTEINFO shexinfo;

	shexinfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shexinfo.fMask = SEE_MASK_INVOKEIDLIST;	// SEE_MASK_IDLIST is also possible.
	shexinfo.hwnd = hwnd;
	shexinfo.lpVerb = NULL;
	shexinfo.lpFile = NULL;
	shexinfo.lpParameters = NULL;
	shexinfo.lpDirectory = NULL;
	shexinfo.nShow = nCmdShow;

	ShellPath shell_path = create_absolute_pidl();
	shexinfo.lpIDList = &*shell_path;

	BOOL ret = TRUE;

	if (!ShellExecuteEx(&shexinfo)) {
		display_error(hwnd, GetLastError());
		ret = FALSE;
	}

	return ret;
}


static HICON extract_icon(IShellFolder* folder, LPCITEMIDLIST pidl)
{
	CONTEXT("extract_icon()");

	IExtractIcon* pExtract;

	if (SUCCEEDED(folder->GetUIObjectOf(0, 1, (LPCITEMIDLIST*)&pidl, IID_IExtractIcon, 0, (LPVOID*)&pExtract))) {
		TCHAR path[MAX_PATH];
		unsigned flags;
		HICON hIcon;
		int idx;

		if (SUCCEEDED(pExtract->GetIconLocation(GIL_FORSHELL, path, MAX_PATH, &idx, &flags))) {
			if (!(flags & GIL_NOTFILENAME)) {
				if (idx == -1)
					idx = 0;	// special case for some control panel applications

				if ((int)ExtractIconEx(path, idx, 0, &hIcon, 1) > 0)
					flags &= ~GIL_DONTCACHE;
			} else {
				HICON hIconLarge = 0;

				HRESULT hr = pExtract->Extract(path, idx, &hIconLarge, &hIcon, MAKELONG(0/*GetSystemMetrics(SM_CXICON)*/,GetSystemMetrics(SM_CXSMICON)));

				if (SUCCEEDED(hr))
					DestroyIcon(hIconLarge);
			}

			if (!hIcon) {
				SHFILEINFO sfi;

				if (SHGetFileInfo(path, 0, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_SMALLICON))
					hIcon = sfi.hIcon;
			}
/*
			if (!hIcon) {
				LPBYTE b = (LPBYTE) alloca(0x10000);
				SHFILEINFO sfi;

				FILE* file = fopen(path, "rb");
				if (file) {
					int l = fread(b, 1, 0x10000, file);
					fclose(file);

					if (l)
						hIcon = CreateIconFromResourceEx(b, l, TRUE, 0x00030000, 16, 16, LR_DEFAULTCOLOR);
				}
			}
*/
			return hIcon;
		}
	}

	return 0;
}

static HICON extract_icon(IShellFolder* folder, const ShellEntry* entry)
{
	HICON hIcon = extract_icon(folder, entry->_pidl);

	if (!hIcon) {
		SHFILEINFO sfi;

		ShellPath pidl_abs = entry->create_absolute_pidl();
		LPCITEMIDLIST pidl = pidl_abs;

		if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON))
			hIcon = sfi.hIcon;
	}

	return hIcon;
}

HICON extract_icon(const Entry* entry)
{
	if (entry->_etype == ET_SHELL) {
		const ShellEntry* shell_entry = static_cast<const ShellEntry*>(entry);

		return extract_icon(shell_entry->get_parent_folder(), shell_entry);
	} else {
		TCHAR path[MAX_PATH];

		if (entry->get_path(path)) {
			SHFILEINFO sfi;

			ShellPath shell_path(path);
			LPCITEMIDLIST pidl = shell_path;

			if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_ICON|SHGFI_SMALLICON))
				return sfi.hIcon;
		}
	}

	return 0;
}


void ShellDirectory::read_directory(int scan_flags)
{
	CONTEXT("ShellDirectory::read_directory()");

	int level = _level + 1;

	Entry* first_entry = NULL;
	Entry* last = NULL;

	/*if (_folder.empty())
		return;*/

	TCHAR buffer[MAX_PATH];

	if ((scan_flags&SCAN_FILESYSTEM) && get_path(buffer)) {
		Entry* entry;

		LPTSTR p = buffer + _tcslen(buffer);

		lstrcpy(p, TEXT("\\*"));

		WIN32_FIND_DATA w32fd;
		HANDLE hFind = FindFirstFile(buffer, &w32fd);

		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				 // ignore hidden files (usefull in the start menu)
				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
					continue;

				 // ignore directory entries "." and ".."
				if ((w32fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) &&
					w32fd.cFileName[0]==TEXT('.') &&
					(w32fd.cFileName[1]==TEXT('\0') ||
					(w32fd.cFileName[1]==TEXT('.') && w32fd.cFileName[2]==TEXT('\0'))))
					continue;

				lstrcpy(p+1, w32fd.cFileName);

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					entry = new WinDirectory(this, buffer);
				else
					entry = new WinEntry(this);

				if (!first_entry)
					first_entry = entry;

				if (last)
					last->_next = entry;

				memcpy(&entry->_data, &w32fd, sizeof(WIN32_FIND_DATA));

				if (!(w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					if (scan_flags & SCAN_EXTRACT_ICONS) {
						entry->_hIcon = extract_icon(entry);

						if (!entry->_hIcon)
							entry->_hIcon = (HICON)-1;	// don't try again later
					} else
						entry->_hIcon = 0;
				} else
					entry->_hIcon = (HICON)-1;	// don't try again later

				entry->_down = NULL;
				entry->_expanded = false;
				entry->_scanned = false;
				entry->_level = level;
				entry->_bhfi_valid = false;

				if (scan_flags & SCAN_DO_ACCESS) {
					HANDLE hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
												0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);

					if (hFile != INVALID_HANDLE_VALUE) {
						if (GetFileInformationByHandle(hFile, &entry->_bhfi))
							entry->_bhfi_valid = true;

						if (ScanNTFSStreams(entry, hFile))
							entry->_scanned = true;	// There exist named NTFS sub-streams in this file.

						CloseHandle(hFile);
					}
				}

				LPCTSTR ext = _tcsrchr(entry->_data.cFileName, TEXT('.'));

				if (ext) {//@@
					int len = ext - entry->_data.cFileName;
					entry->_display_name = (LPTSTR) malloc((len+1)*sizeof(TCHAR));
					_tcsncpy(entry->_display_name, entry->_data.cFileName, len);
					entry->_display_name[len] = TEXT('\0');
				}

				DWORD attribs = SFGAO_FILESYSTEM;

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					attribs |= SFGAO_FOLDER;

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
					attribs |= SFGAO_READONLY;

				//if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				//	attribs |= SFGAO_HIDDEN;

				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
					attribs |= SFGAO_COMPRESSED;

				if (ext && !_tcsicmp(ext, _T(".lnk")))
					attribs |= SFGAO_LINK;

				entry->_shell_attribs = attribs;

				last = entry;
			} while(FindNextFile(hFind, &w32fd));

			FindClose(hFind);
		}

	} else { // !SCAN_FILESYSTEM

		ShellItemEnumerator enumerator(_folder, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN|SHCONTF_SHAREABLE|SHCONTF_STORAGE);

		TCHAR name[MAX_PATH];
		HRESULT hr_next = S_OK;

		do {
#define FETCH_ITEM_COUNT	32
			LPITEMIDLIST pidls[FETCH_ITEM_COUNT];
			ULONG cnt = 0;
			ULONG n;

			memset(pidls, 0, sizeof(pidls));

			hr_next = enumerator->Next(FETCH_ITEM_COUNT, pidls, &cnt);

			/* don't break yet now: Registry Explorer Plugin returns E_FAIL!
			if (!SUCCEEDED(hr_next))
				break; */

			if (hr_next == S_FALSE)
				break;

			for(n=0; n<cnt; ++n) {
				WIN32_FIND_DATA w32fd;
				BY_HANDLE_FILE_INFORMATION bhfi;
				bool bhfi_valid = false;

				memset(&w32fd, 0, sizeof(WIN32_FIND_DATA));

				SFGAOF attribs_before = ~SFGAO_READONLY & ~SFGAO_VALIDATE;
				SFGAOF attribs = attribs_before;
				HRESULT hr = _folder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidls[n], &attribs);
				bool removeable = false;

				if (SUCCEEDED(hr) && attribs!=attribs_before) {
					 // avoid accessing floppy drives when browsing "My Computer"
					if (attribs & SFGAO_REMOVABLE) {
						attribs |= SFGAO_HASSUBFOLDER;
						removeable = true;
					} else if (scan_flags & SCAN_DO_ACCESS) {
						DWORD attribs2 = SFGAO_READONLY;

						HRESULT hr = _folder->GetAttributesOf(1, (LPCITEMIDLIST*)&pidls[n], &attribs2);

						if (SUCCEEDED(hr))
							attribs |= attribs2;
					}
				} else
					attribs = 0;

				bhfi_valid = fill_w32fdata_shell(pidls[n], attribs, &w32fd, &bhfi,
												 (scan_flags&SCAN_DO_ACCESS) && !removeable);

				try {
					Entry* entry;

					if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						entry = new ShellDirectory(this, pidls[n], _hwnd);
					else
						entry = new ShellEntry(this, pidls[n]);

					if (!first_entry)
						first_entry = entry;

					if (last)
						last->_next = entry;

					memcpy(&entry->_data, &w32fd, sizeof(WIN32_FIND_DATA));

					if (bhfi_valid)
						memcpy(&entry->_bhfi, &bhfi, sizeof(BY_HANDLE_FILE_INFORMATION));

					if (SUCCEEDED(name_from_pidl(_folder, pidls[n], name, MAX_PATH, SHGDN_INFOLDER|0x2000/*0x2000=SHGDN_INCLUDE_NONFILESYS*/))) {
						if (!entry->_data.cFileName[0])
							_tcscpy(entry->_data.cFileName, name);
						else if (_tcscmp(entry->_display_name, name))
							entry->_display_name = _tcsdup(name);	// store display name separate from file name; sort display by file name
					}

					 // get icons for files and virtual objects
					if (!(entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
						!(attribs & SFGAO_FILESYSTEM)) {
						if (scan_flags & SCAN_EXTRACT_ICONS) {
							entry->_hIcon = extract_icon(_folder, static_cast<ShellEntry*>(entry));

							if (!entry->_hIcon)
								entry->_hIcon = (HICON)-1;	// don't try again later
						} else
							entry->_hIcon = 0;
					} else
						entry->_hIcon = (HICON)-1;	// don't try again later

					entry->_down = NULL;
					entry->_expanded = false;
					entry->_scanned = false;
					entry->_level = level;
					entry->_shell_attribs = attribs;
					entry->_bhfi_valid = bhfi_valid;

					last = entry;
				} catch(COMException& e) {
					HandleException(e, _hwnd);
				}
			}
		} while(SUCCEEDED(hr_next));
	}

	if (last)
		last->_next = NULL;

	_down = first_entry;
	_scanned = true;
}

const void* ShellDirectory::get_next_path_component(const void* p)
{
	LPITEMIDLIST pidl = (LPITEMIDLIST)p;

	if (!pidl || !pidl->mkid.cb)
		return NULL;

	 // go to next element
	pidl = (LPITEMIDLIST)((LPBYTE)pidl+pidl->mkid.cb);

	return pidl;
}

Entry* ShellDirectory::find_entry(const void* p)
{
	LPITEMIDLIST pidl = (LPITEMIDLIST) p;

	for(Entry*entry=_down; entry; entry=entry->_next)
		if (entry->_etype == ET_SHELL) {
			ShellEntry* se = static_cast<ShellEntry*>(entry);

			if (se->_pidl && se->_pidl->mkid.cb==pidl->mkid.cb && !memcmp(se->_pidl, pidl, se->_pidl->mkid.cb))
				return entry;
		}

	return NULL;
}

int ShellDirectory::extract_icons()
{
	int cnt = 0;

	for(Entry*entry=_down; entry; entry=entry->_next)
		if (!entry->_hIcon) {
			if (entry->_etype == ET_SHELL)
				entry->_hIcon = extract_icon(_folder, static_cast<ShellEntry*>(entry));
			else // !ET_SHELL
				entry->_hIcon = extract_icon(entry);

			if (entry->_hIcon)
				++cnt;
			else
				entry->_hIcon = (HICON)-1;	// don't try again later
		}

	return cnt;
}
