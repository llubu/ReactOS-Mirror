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
 *
 *    06-Nov-1999 (Eric Kohl <ekohl@abo.rhein-zeiung.de>)
 *        Little fix to keep DEL quiet inside batch files.
 *
 *    28-Jan-2004 (Michael Fritscher <michael@fritscher.net>)
 *        Added prompt ("/P"), yes ("/Y") and wipe("/W") option.
 */

#include "config.h"

#ifdef INCLUDE_CMD_DEL

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "cmd.h"
#include "batch.h"


enum
{
	DEL_ATTRIBUTES = 0x001,   /* /A : not implemented */
	DEL_ERROR      = 0x002,   /* /E : not implemented */
	DEL_NOTHING    = 0x004,   /* /N */
	DEL_PROMPT     = 0x008,   /* /P */
	DEL_QUIET      = 0x010,   /* /Q */
	DEL_SUBDIR     = 0x020,   /* /S : not implemented */
	DEL_TOTAL      = 0x040,   /* /T */
	DEL_WIPE       = 0x080,   /* /W */
	DEL_EMPTYDIR   = 0x100,   /* /X : not implemented */
	DEL_YES        = 0x200,   /* /Y */
	DEL_ZAP        = 0x400    /* /Z */
};



static BOOL
RemoveFile (LPTSTR lpFileName, DWORD dwFlags)
{
	if (dwFlags & DEL_WIPE)
	{

		HANDLE file;
		DWORD temp;
		LONG BufferSize = 65536;
		BYTE buffer[BufferSize];
		LONG i;
		HANDLE fh;
		WIN32_FIND_DATA f;
		LONGLONG FileSize;

		fh = FindFirstFile(lpFileName, &f);
		FileSize = ((LONGLONG)f.nFileSizeHigh * ((LONGLONG)MAXDWORD+1)) + (LONGLONG)f.nFileSizeLow;

		for(i = 0; i < BufferSize; i++)
		{
			buffer[i]=rand() % 256;
		}
		file = CreateFile (lpFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,  FILE_FLAG_WRITE_THROUGH, NULL);
		for(i = 0; i < (FileSize - BufferSize); i += BufferSize)
		{
			WriteFile (file, buffer, BufferSize, &temp, NULL);
			ConOutPrintf (_T("%I64d%% wiped\r"),(i * (LONGLONG)100)/FileSize);
		}
		WriteFile (file, buffer, FileSize - i, &temp, NULL);
		ConOutPrintf (_T("100%% wiped\n"));
		CloseHandle (file);
	}

	return DeleteFile (lpFileName);
}


INT CommandDelete (LPTSTR cmd, LPTSTR param)
{
	TCHAR szFullPath[MAX_PATH];
	LPTSTR pFilePart;
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
		               "DEL [/N /P /T /Q /W /Y /Z] file ...\n"
		               "DELETE [/N /P /T /Q /W /Y /Z] file ...\n"
		               "ERASE [/N /P /T /Q /W /Y /Z] file ...\n"
		               "\n"
		               "  file  Specifies the file(s) to delete.\n"
		               "\n"
		               "  /N    Nothing.\n"
		               "  /P    Prompt. Ask before deleting each file.\n"
		               "  /T    Total. Display total number of deleted files and freed disk space.\n"
		               "  /Q    Quiet.\n"
		               "  /W    Wipe. Overwrite the file with random numbers before deleting it.\n"
		               "  /Y    Yes. Kill even *.* without asking.\n"
		               "  /Z    Zap. Delete hidden, read-only and system files).\n"));

		return 0;
	}

	arg = split (param, &args, FALSE);

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
						case _T('Y'):
							dwFlags |= DEL_YES;
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

		/* keep quiet within batch files */
		if (bc != NULL)
			dwFlags |= DEL_QUIET;

		/* check for filenames anywhere in command line */
		for (i = 0; i < args; i++)
		{
			if (!_tcscmp (arg[i], _T("*")) ||
                            !_tcscmp (arg[i], _T("*.*")))
			{
                                INT res;
                                if (!((dwFlags & DEL_YES) || (dwFlags & DEL_QUIET) || (dwFlags & DEL_PROMPT)))
                                {
                                	res = FilePromptYN (_T("All files in the directory will be deleted!\n"
                                             "Are you sure (Y/N)?"));
                                
                                if ((res == PROMPT_NO) ||
                                    (res == PROMPT_BREAK))
					break;
				}
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

					GetFullPathName (arg[i],
					                 MAX_PATH,
					                 szFullPath,
					                 &pFilePart);

#ifdef _DEBUG
					ConErrPrintf (_T("Full path: %s\n"), szFullPath);
					ConErrPrintf (_T("File part: %s\n"), pFilePart);
#endif

					hFile = FindFirstFile (szFullPath, &f);
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

						_tcscpy (pFilePart, f.cFileName);

#ifdef _DEBUG
						ConErrPrintf (_T("Full filename: %s\n"), szFullPath);
#endif
						/*ask for deleting */
						if(dwFlags & DEL_PROMPT) 
						{
							INT res;
							ConErrPrintf (_T("The file %s will be deleted! "), szFullPath);
		                                	res = FilePromptYN (_T("Are you sure (Y/N)?"));
		                                        
		                                        if ((res == PROMPT_NO) || (res == PROMPT_BREAK))
		                                        {
		                                        	continue;  //FIXME: Errorcode?
		                                        }
						}	

						if (!(dwFlags & DEL_QUIET) && !(dwFlags & DEL_TOTAL))
							ConErrPrintf (_T("Deleting: %s\n"), szFullPath);

		                                

						/* delete the file */
						if (!(dwFlags & DEL_NOTHING))
						{
							if (RemoveFile (szFullPath, dwFlags))
							{
								dwFiles++;
							}
							else
							{
								if (dwFlags & DEL_ZAP)
								{
									if (SetFileAttributes (szFullPath, 0))
									{
										if (RemoveFile (szFullPath, dwFlags))
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
					GetFullPathName (arg[i],
					                 MAX_PATH,
					                 szFullPath,
					                 &pFilePart);

						
					/*ask for deleting */
					if((dwFlags & DEL_PROMPT) && (FindFirstFile(szFullPath, &f) != INVALID_HANDLE_VALUE)) //Don't ask if the file doesn't exist, the following code will make the error-msg 
					{
						INT res;
						ConErrPrintf (_T("The file %s will be deleted! "), szFullPath);
	                                	res = FilePromptYN (_T("Are you sure (Y/N)?"));
	                                        
	                                        if ((res == PROMPT_NO) || (res == PROMPT_BREAK))
	                                        {
	                                        	break;   //FIXME: Errorcode?
	                                        }
					}	

#ifdef _DEBUG
					ConErrPrintf (_T("Full path: %s\n"), szFullPath);
#endif
					if (!(dwFlags & DEL_QUIET) && !(dwFlags & DEL_TOTAL))
						ConOutPrintf (_T("Deleting %s\n"), szFullPath);

					if (!(dwFlags & DEL_NOTHING))
					{
						if (RemoveFile (szFullPath, dwFlags))
						{
							dwFiles++;
						}
						else
						{
							if (dwFlags & DEL_ZAP)
							{
								if (SetFileAttributes (szFullPath, 0))
								{
									if (RemoveFile (szFullPath, dwFlags))
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
			              dwFiles,
			              (dwFiles == 1) ? _T("") : _T("s"));
	}

	return 0;
}

#endif
