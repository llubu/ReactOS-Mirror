/*
 *  DEL.C - del internal command.
 *
 *
 *  History:
 *
 *    06/29/98 (Rob Lake rlake@cs.mun.ca)
 *        rewrote del to support wildcards
 *        added my name to the contributors
 *
 *    07/13/98 (Rob Lake)
 *        fixed bug that caused del not to delete file with out
 *        attribute. moved set, del, ren, and ver to there own files
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    09-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
 *        Fixed command line parsing bugs.
 *
 *    21-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
 *        Started major rewrite using a new structure.
 *
 *    03-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
 *        First working version.
 *
 *    30-Mar-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
 *        Added quiet ("/Q"), wipe ("/W") and zap ("/Z") option.
 */

#include "config.h"

#ifdef INCLUDE_CMD_DEL

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


#define PROMPT_NO		0
#define PROMPT_YES		1
#define PROMPT_ALL		2
#define PROMPT_BREAK	3


enum
{
	DEL_ATTRIBUTES = 0x001,   /* /A : not implemented */
	DEL_ERROR      = 0x002,   /* /E : not implemented */
	DEL_NOTHING    = 0x004,   /* /N */
	DEL_PROMPT     = 0x008,   /* /P : not implemented */
	DEL_QUIET      = 0x010,   /* /Q */
	DEL_SUBDIR     = 0x020,   /* /S : not implemented */
	DEL_TOTAL      = 0x040,   /* /T */
	DEL_WIPE       = 0x080,   /* /W */
	DEL_EMPTYDIR   = 0x100,   /* /X : not implemented */
	DEL_YES        = 0x200,   /* /Y : not implemented */
	DEL_ZAP        = 0x400    /* /Z */
};



static BOOL ConfirmDeleteAll (VOID)
{
	TCHAR inp[10];
	LPTSTR p;

	ConOutPrintf (_T("All files in directory will be deleted!\n"
					 "Are you sure (Y/N)? "));
	ConInString (inp, 10);

	_tcsupr (inp);
	for (p = inp; _istspace (*p); p++)
		;

	if (*p == _T('Y'))
		return PROMPT_YES;
	else if (*p == _T('N'))
		return PROMPT_NO;

#if 0
	if (*p == _T('A'))
		return PROMPT_ALL;
	else if (*p == _T('\03'))
		return PROMPT_BREAK;
#endif

	return PROMPT_NO;
}


static INT Prompt (LPTSTR str)
{
	TCHAR inp[10];
	LPTSTR p;

	ConOutPrintf (_T("Delete %s (Yes/No)? "), str);
	ConInString (inp, 10);

	_tcsupr (inp);
	for (p = inp; _istspace (*p); p++)
		;

	if (*p == _T('Y'))
		return PROMPT_YES;
	else if (*p == _T('N'))
		return PROMPT_NO;

#if 0
	if (*p == _T('A'))
		return PROMPT_ALL;
	else if (*p == _T('\03'))
		return PROMPT_BREAK;
#endif

	return PROMPT_NO;
}


static BOOL
RemoveFile (LPTSTR lpFileName, DWORD dwFlags)
{
	if (dwFlags & DEL_WIPE)
	{

		/* FIXME: Wipe the given file */

	}

	return DeleteFile (lpFileName);
}


INT cmd_del (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *arg = NULL;
	INT args;
	INT i;
	INT   nEvalArgs = 0; /* nunber of evaluated arguments */
	DWORD dwFlags = 0;
	DWORD dwFiles = 0;

	HANDLE hFile;
	WIN32_FIND_DATA f;

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Deletes one or more files.\n"
					   "\n"
					   "DEL [/N /P /T /Q /W /Z] file ...\n"
					   "DELETE [/N /P /T /Q /W /Z] file ...\n"
					   "ERASE [/N /P /T /Q /W /Z] file ...\n"
					   "\n"
					   "  file  Specifies the file(s) to delete.\n"
					   "\n"
					   "  /N    Nothing.\n"
					   "  /P    Prompts for confirmation before deleting each file.\n"
					   "        (Not implemented yet!)\n"
					   "  /T    Display total number of deleted files and freed disk space.\n"
					   "  /Q    Quiet.\n"
					   "  /W    Wipe. Overwrite the file with zeros before deleting it.\n"
					   "  /Z    Zap (delete hidden, read-only and system files).\n"
					   ));

		return 0;
	}

	arg = split (param, &args);

	if (args > 0)
	{
		/* check for options anywhere in command line */
		for (i = 0; i < args; i++)
		{
			if (*arg[i] == _T('/'))
			{
				if (_tcslen (arg[i]) >= 2)
				{
					switch (_totupper (arg[i][1]))
					{
						case _T('N'):
							dwFlags |= DEL_NOTHING;
							break;

						case _T('P'):
							dwFlags |= DEL_PROMPT;
							break;

						case _T('Q'):
							dwFlags |= DEL_QUIET;
							break;

						case _T('S'):
							dwFlags |= DEL_SUBDIR;
							break;

						case _T('T'):
							dwFlags |= DEL_TOTAL;
							break;

						case _T('W'):
							dwFlags |= DEL_WIPE;
							break;

						case _T('Z'):
							dwFlags |= DEL_ZAP;
							break;
					}

				}

				nEvalArgs++;
			}
		}

		/* there are only options on the command line --> error!!! */
		if (args == nEvalArgs)
		{
			error_req_param_missing ();
			freep (arg);
			return 1;
		}

		/* check for filenames anywhere in command line */
		for (i = 0; i < args; i++)
		{
			if (!_tcscmp (arg[i], _T("*")) ||
				!_tcscmp (arg[i], _T("*.*")))
			{
				if (!ConfirmDeleteAll ())
					break;

			}

			if (*arg[i] != _T('/'))
			{
#ifdef _DEBUG
				ConErrPrintf (_T("File: %s\n"), arg[i]);
#endif

				if (_tcschr (arg[i], _T('*')) || _tcschr (arg[i], _T('?')))
				{
					/* wildcards in filespec */
#ifdef _DEBUG
					ConErrPrintf (_T("Wildcards!\n\n"));
#endif

					hFile = FindFirstFile (arg[i], &f);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						error_file_not_found ();
						freep (arg);
						return 0;
					}

					do
					{
						/* ignore "." and ".." */
						if (!_tcscmp (f.cFileName, _T(".")) ||
							!_tcscmp (f.cFileName, _T("..")))
							continue;

						if (!(dwFlags & DEL_QUIET) && !(dwFlags & DEL_TOTAL))
							ConErrPrintf (_T("Deleting: %s\n"), f.cFileName);

						/* delete the file */
						if (!(dwFlags & DEL_NOTHING))
						{
							if (RemoveFile (f.cFileName, dwFlags))
							{
								dwFiles++;
							}
							else
							{
								if (dwFlags & DEL_ZAP)
								{
									if (SetFileAttributes (arg[i], 0))
									{
										if (RemoveFile (arg[i], dwFlags))
										{
											dwFiles++;
										}
										else
										{
											ErrorMessage (GetLastError(), _T(""));
										}
									}
									else
									{
										ErrorMessage (GetLastError(), _T(""));
									}
								}
								else
								{
									ErrorMessage (GetLastError(), _T(""));
								}
							}
						}


					}
					while (FindNextFile (hFile, &f));
					FindClose (hFile);
				}
				else
				{
					/* no wildcards in filespec */

#ifdef _DEBUG
					ConErrPrintf (_T("No Wildcards!\n"));
#endif

					if (!(dwFlags & DEL_QUIET) && !(dwFlags & DEL_TOTAL))
						ConOutPrintf (_T("Deleting %s\n"), arg[i]);

					if (!(dwFlags & DEL_NOTHING))
					{
						if (RemoveFile (arg[i], dwFlags))
						{
							dwFiles++;
						}
						else
						{
							if (dwFlags & DEL_ZAP)
							{
								if (SetFileAttributes (arg[i], 0))
								{
									if (RemoveFile (arg[i], dwFlags))
									{
										dwFiles++;
									}
									else
									{
										ErrorMessage (GetLastError(), _T(""));
									}
								}
								else
								{
									ErrorMessage (GetLastError(), _T(""));
								}
							}
							else
							{
								ErrorMessage (GetLastError(), _T(""));
							}
						}
					}
				}
			}
		}
	}
	else
	{
		/* only command given */
		error_req_param_missing ();
		freep (arg);
		return 1;
	}

	freep (arg);


	if (!(dwFlags & DEL_QUIET))
	{
		if (dwFiles == 0)
			ConOutPrintf (_T("    0 files deleted\n"));
		else
			ConOutPrintf (_T("    %lu file%s deleted\n"),
						  dwFiles, (dwFiles == 1) ? "s" : "");
	}

	return 0;
}

#endif
