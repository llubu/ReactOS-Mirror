/*
 *  ReactOS Task Manager
 *
 *  processpage.h
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
	
#ifndef __PROCESSPAGE_H
#define __PROCESSPAGE_H

extern	HWND		hProcessPage;				/* Process List Property Page */
extern	HWND		hProcessPageListCtrl;			/* Process ListCtrl Window */
extern	HWND		hProcessPageHeaderCtrl;			/* Process Header Control */
extern	HWND		hProcessPageEndProcessButton;		/* Process End Process button */
extern	HWND		hProcessPageShowAllProcessesButton;	/* Process Show All Processes checkbox */

LRESULT CALLBACK	ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void				RefreshProcessPage(void);

#endif /* __PROCESSPAGE_H */
