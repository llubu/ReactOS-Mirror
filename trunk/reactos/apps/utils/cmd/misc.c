/*
 *  MISC.C - misc. functions.
 *
 *
 *  History:
 *
 *    07/12/98 (Rob Lake)
 *        started
 *
 *    07/13/98 (Rob Lake)
 *        moved functions in here
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    18-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Changed split() to accept quoted arguments.
 *        Removed parse_firstarg().
 *
 *    23-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed an ugly bug in split(). In rare cases (last character
 *        of the string is a space) it ignored the NULL character and
 *        tried to add the following to the argument list.
 *
 *    28-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        FileGetString() seems to be working now.
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "cmd.h"


/*
 * get a character out-of-band and honor Ctrl-Break characters
 */
TCHAR cgetchar (VOID)
{
	HANDLE hInput = GetStdHandle (STD_INPUT_HANDLE);
	INPUT_RECORD irBuffer;
	DWORD  dwRead;

	do
	{
		ReadConsoleInput (hInput, &irBuffer, 1, &dwRead);
		if ((irBuffer.EventType == KEY_EVENT) &&
			(irBuffer.Event.KeyEvent.bKeyDown == TRUE))
		{
			if ((irBuffer.Event.KeyEvent.dwControlKeyState &
				 (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) &
				(irBuffer.Event.KeyEvent.wVirtualKeyCode == 'C'))
				bCtrlBreak = TRUE;

			break;
		}
	}
	while (TRUE);

#ifdef __REACTOS__
	return irBuffer.Event.KeyEvent.AsciiChar;
#else
#ifndef _UNICODE
	return irBuffer.Event.KeyEvent.uChar.AsciiChar;
#else
	return irBuffer.Event.KeyEvent.uChar.UnicodeChar;
#endif /* _UNICODE */
#endif /* __REACTOS__ */
}


/*
 * Check if Ctrl-Break was pressed during the last calls
 */

BOOL CheckCtrlBreak (INT mode)
{
	static BOOL bLeaveAll = FALSE; /* leave all batch files */
	TCHAR c;

	switch (mode)
	{
		case BREAK_OUTOFBATCH:
			bLeaveAll = 0;
			return FALSE;

		case BREAK_BATCHFILE:
			if (bLeaveAll)
				return TRUE;

			if (!bCtrlBreak)
				return FALSE;

			/* we need to be sure the string arrives on the screen! */
			do
				ConOutPuts (_T("\r\nCtrl-Break pressed.  Cancel batch file? (Yes/No/All) "));
			while (!_tcschr ("YNA\3", c = _totupper (cgetchar())) || !c);

			ConOutPuts (_T("\r\n"));

			if (c == _T('N'))
				return bCtrlBreak = FALSE; /* ignore */

			/* leave all batch files */
			bLeaveAll = ((c == _T('A')) || (c == _T('\3')));
			break;

		case BREAK_INPUT:
			if (!bCtrlBreak)
				return FALSE;
			break;
	}

	/* state processed */
	bCtrlBreak = FALSE;
	return TRUE;
}


/*
 * split - splits a line up into separate arguments, deliminators
 *         are spaces and slashes ('/').
 */

LPTSTR *split (LPTSTR s, LPINT args)
{
	LPTSTR *arg;
	LPTSTR *p;
	LPTSTR start;
	LPTSTR q;
	INT  ac;
	INT  len;
	BOOL bQuoted = FALSE;

	arg = malloc (sizeof (LPTSTR));
	if (!arg)
		return NULL;
	*arg = NULL;

	ac = 0;
	while (*s)
	{
		/* skip leading spaces */
		while (*s && (_istspace (*s) || _istcntrl (*s)))
			++s;

		/* if quote (") then set bQuoted */
		if (*s == _T('\"'))
		{
			++s;
			bQuoted = TRUE;
		}

		start = s;

		/* the first character can be '/' */
		if (*s == _T('/'))
			++s;

		/* skip to next word delimiter or start of next option */
		if (bQuoted)
		{
			while (_istprint (*s) && (*s != _T('\"')) && (*s != _T('/')))
				++s;
		}
		else
		{
			while (_istprint (*s) && !_istspace (*s) && (*s != _T('/')))
				++s;
		}

		/* a word was found */
		if (s != start)
		{
			/* add new entry for new argument */
			arg = realloc (p = arg, (ac + 2) * sizeof (LPTSTR));
			if (!arg)
			{
				freep (p);
				return NULL;
			}

			/* create new entry */
			q = arg[ac] = malloc (((len = s - start) + 1) * sizeof (TCHAR));
			arg[++ac] = NULL;
			if (!q)
			{
				freep (arg);
				return NULL;
			}
			memcpy (q, start, len * sizeof (TCHAR));
			q[len] = _T('\0');
		}

		/* adjust string pointer if quoted (") */
		if (bQuoted)
		{
			++s;
			bQuoted = FALSE;
		}
	}

	*args = ac;

	return arg;
}


/*
 * freep -- frees memory used for a call to split
 *
 */
VOID freep (LPTSTR *p)
{
	LPTSTR *q;

	if (!p)
		return;

	q = p;
	while (*q)
		free(*q++);

	free(p);
}


LPTSTR stpcpy (LPTSTR dest, LPTSTR src)
{
	_tcscpy (dest, src);
	return (dest + _tcslen (src));
}



/*
 * Checks if a path is valid (accessible)
 */

BOOL IsValidPathName (LPCTSTR pszPath)
{
	TCHAR szOldPath[MAX_PATH];
	BOOL  bResult;

	GetCurrentDirectory (MAX_PATH, szOldPath);
	bResult = SetCurrentDirectory (pszPath);

	SetCurrentDirectory (szOldPath);

	return bResult;
}


/*
 * Checks if a file exists (accessible)
 */

BOOL IsValidFileName (LPCTSTR pszPath)
{
	return (GetFileAttributes (pszPath) != 0xFFFFFFFF);
}


BOOL IsValidDirectory (LPCTSTR pszPath)
{
	return (GetFileAttributes (pszPath) & FILE_ATTRIBUTE_DIRECTORY);
}


BOOL FileGetString (HANDLE hFile, LPTSTR lpBuffer, INT nBufferLength)
{
	LPTSTR lpString;
	TCHAR  ch;
	DWORD  dwRead;

	lpString = lpBuffer;

	while ((--nBufferLength >  0) &&
		   ReadFile(hFile, &ch, 1, &dwRead, NULL) && dwRead)
	{
		*lpString++ = ch;
		if (ch == _T('\r'))
		{
			/* overread '\n' */
			ReadFile (hFile, &ch, 1, &dwRead, NULL);
			break;
		}
	}

	if (!dwRead && lpString == lpBuffer)
		return FALSE;

	*lpString++ = _T('\0');

	return TRUE;
}
