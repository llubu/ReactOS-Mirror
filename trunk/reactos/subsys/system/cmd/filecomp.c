/*
 *  FILECOMP.C - handles filename completion.
 *
 *
 *  Comments:
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *       moved from command.c file
 *       made second TAB display list of filename matches
 *       made filename be lower case if last character typed is lower case
 *
 *    25-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *       Cleanup. Unicode safe!
 *
 *    30-Apr-2004 (Filip Navara <xnavara@volny.cz>)
 *       Make the file listing readable when there is a lot of long names.
 *

 *    05-Jul-2004 (Jens Collin <jens.collin@lakhei.com>)
 *       Now expands lfn even when trailing " is omitted.
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "cmd.h"


#ifdef FEATURE_UNIX_FILENAME_COMPLETION

VOID CompleteFilename (LPTSTR str, INT charcount)
{
	WIN32_FIND_DATA file;
	HANDLE hFile;
	INT   curplace = 0;
	INT   start;
	INT   count;
	INT step, c;
	BOOL  found_dot = FALSE;
	BOOL  perfectmatch = TRUE;
	TCHAR path[MAX_PATH];
	TCHAR fname[MAX_PATH];
	TCHAR maxmatch[MAX_PATH] = _T("");
	TCHAR directory[MAX_PATH];
	LPCOMMAND cmds_ptr;

	/* expand current file name */
	count = charcount - 1;
	if (count < 0)
		count = 0;

	/* find how many '"'s there is typed already.*/
	step = count;
	while (step > 0)
	{
		if (str[step] == _T('"'))
			c++;
		step--;
	}
	/* if c is odd, then user typed " before name, else not.*/

	/* find front of word */
	if (str[count] == _T('"') || (c % 2))
	{
		count--;
		while (count > 0 && str[count] != _T('"'))
			count--;
	}
	else
	{
		while (count > 0 && str[count] != _T(' '))
			count--;
	}

	/* if not at beginning, go forward 1 */
	if (str[count] == _T(' '))
		count++;

	start = count;

	if (str[count] == _T('"'))
		count++;	/* don't increment start */

	/* extract directory from word */
	_tcscpy (directory, &str[count]);
	curplace = _tcslen (directory) - 1;

	if (curplace >= 0 && directory[curplace] == _T('"'))
		directory[curplace--] = _T('\0');

	_tcscpy (path, directory);

	while (curplace >= 0 && directory[curplace] != _T('\\') &&
		   directory[curplace] != _T(':'))
	{
		directory[curplace] = 0;
		curplace--;
	}

	/* look for a '.' in the filename */
	for (count = _tcslen (directory); path[count] != _T('\0'); count++)
	{
		if (path[count] == _T('.'))
		{
			found_dot = TRUE;
			break;
		}
	}

	if (found_dot)
		_tcscat (path, _T("*"));
	else
		_tcscat (path, _T("*.*"));

	/* current fname */
	curplace = 0;

	hFile = FindFirstFile (path, &file);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		/* find anything */
		do
		{
			/* ignore "." and ".." */
			if (!_tcscmp (file.cFileName, _T(".")) ||
				!_tcscmp (file.cFileName, _T("..")))
				continue;

			_tcscpy (fname, file.cFileName);

			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				_tcscat (fname, _T("\\"));

			if (!maxmatch[0] && perfectmatch)
			{
				_tcscpy(maxmatch, fname);
			}
			else
			{
				for (count = 0; maxmatch[count] && fname[count]; count++)
				{
					if (tolower(maxmatch[count]) != tolower(fname[count]))
					{
						perfectmatch = FALSE;
						maxmatch[count] = 0;
						break;
					}
				}

				if (maxmatch[count] == _T('\0') &&
				    fname[count] != _T('\0'))
					perfectmatch = FALSE;
			}
		}
		while (FindNextFile (hFile, &file));

		FindClose (hFile);

		/* only quote if the filename contains spaces */
		if (_tcschr(directory, _T(' ')) ||
		    _tcschr(maxmatch, _T(' ')))
		{
			str[start] = _T('\"');
			_tcscpy (&str[start+1], directory);
			_tcscat (&str[start], maxmatch);
			_tcscat (&str[start], _T("\"") );
		}
		else
		{
			_tcscpy (&str[start], directory);
			_tcscat (&str[start], maxmatch);
		}

		/* append a space if last word is not a directory */
		if(perfectmatch)
		{
			curplace = _tcslen(&str[start]);
			if(str[start+curplace-1] == _T('"'))
				curplace--;

			if(str[start+curplace-1] != _T('\\'))
				_tcscat(&str[start], _T(" "));
		}
		else
		{
#ifdef __REACTOS__
			Beep (440, 50);
#else
			MessageBeep (-1);
#endif
		}
	}
	else
	{
		/* no match found - search for internal command */
		for (cmds_ptr = cmds; cmds_ptr->name; cmds_ptr++)
		{
			if (!_tcsnicmp (&str[start], cmds_ptr->name,
				_tcslen (&str[start])))
			{
				/* return the mach only if it is unique */
				if (_tcsnicmp (&str[start], (cmds_ptr+1)->name, _tcslen (&str[start])))
					_tcscpy (&str[start], cmds_ptr->name);
				break;
			}
		}

#ifdef __REACTOS__
		Beep (440, 50);
#else
		MessageBeep (-1);
#endif
	}
}


/*
 * returns 1 if at least one match, else returns 0
 */

BOOL ShowCompletionMatches (LPTSTR str, INT charcount)
{
	WIN32_FIND_DATA file;
	HANDLE hFile;
	BOOL  found_dot = FALSE;
	INT   curplace = 0;
	INT   start;
	INT   count;
	TCHAR path[MAX_PATH];
	TCHAR fname[MAX_PATH];
	TCHAR directory[MAX_PATH];
	INT   longestfname = 0;
	SHORT screenwidth;

	/* expand current file name */
	count = charcount - 1;
	if (count < 0)
		count = 0;

	/* find front of word */
	if (str[count] == _T('"'))
	{
		count--;
		while (count > 0 && str[count] != _T('"'))
			count--;
	}
	else
	{
		while (count > 0 && str[count] != _T(' '))
			count--;
	}

	/* if not at beginning, go forward 1 */
	if (str[count] == _T(' '))
		count++;

	start = count;

	if (str[count] == _T('"'))
		count++;	/* don't increment start */

	/* extract directory from word */
	_tcscpy (directory, &str[count]);
	curplace = _tcslen (directory) - 1;

	if (curplace >= 0 && directory[curplace] == _T('"'))
		directory[curplace--] = _T('\0');

	_tcscpy (path, directory);

	while (curplace >= 0 &&
		   directory[curplace] != _T('\\') &&
		   directory[curplace] != _T(':'))
	{
		directory[curplace] = 0;
		curplace--;
	}

	/* look for a . in the filename */
	for (count = _tcslen (directory); path[count] != _T('\0'); count++)
	{
		if (path[count] == _T('.'))
		{
			found_dot = TRUE;
			break;
		}
	}

	if (found_dot)
		_tcscat (path, _T("*"));
	else
		_tcscat (path, _T("*.*"));

	/* current fname */
	curplace = 0;

	hFile = FindFirstFile (path, &file);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		/* Get the size of longest filename first. */
		do
		{
			if (_tcslen(file.cFileName) > longestfname)
			{
				longestfname = _tcslen(file.cFileName);
				/* Directories get extra brackets around them. */
				if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					longestfname += 2;
			}
		}
		while (FindNextFile (hFile, &file));
		FindClose (hFile);

		hFile = FindFirstFile (path, &file);

		/* Count the highest number of columns */
		GetScreenSize(&screenwidth, 0);

		/* For counting columns of output */
		count = 0;

		/* Increase by the number of spaces behind file name */
		longestfname += 3;

		/* find anything */
		ConOutChar (_T('\n'));
		do
		{
			/* ignore . and .. */
			if (!_tcscmp (file.cFileName, _T(".")) ||
				!_tcscmp (file.cFileName, _T("..")))
				continue;

			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				_stprintf (fname, _T("[%s]"), file.cFileName);
			else
				_tcscpy (fname, file.cFileName);

			ConOutPrintf (_T("%*s"), - longestfname, fname);
			count++;
			/* output as much columns as fits on the screen */
			if (count >= (screenwidth / longestfname))
			{
				/* print the new line only if we aren't on the
				 * last column, in this case it wraps anyway */
				if (count * longestfname != screenwidth)
					ConOutPrintf (_T("\n"));
				count = 0;
			}
		}
		while (FindNextFile (hFile, &file));

		FindClose (hFile);

		if (count)
			ConOutChar (_T('\n'));
	}
	else
	{
		/* no match found */
#ifdef __REACTOS__
		Beep (440, 50);
#else
		MessageBeep (-1);
#endif
		return FALSE;
	}

	return TRUE;
}
#endif

#ifdef FEATURE_4NT_FILENAME_COMPLETION

//static VOID BuildFilenameMatchList (...)

// VOID CompleteFilenameNext (LPTSTR, INT)
// VOID CompleteFilenamePrev (LPTSTR, INT)

// VOID RemoveFilenameMatchList (VOID)

#endif
