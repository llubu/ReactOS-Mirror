/*
 *  START.C - start internal command.
 *
 *
 *  History:
 *
 *    24-Jul-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>
#include "resource.h"

#ifdef INCLUDE_CMD_START


INT cmd_start (LPTSTR First, LPTSTR Rest)
{
	TCHAR szFullName[CMDLINE_LENGTH];
	TCHAR first[CMDLINE_LENGTH];
	TCHAR *rest = NULL; 
	TCHAR *param = NULL;
	INT size;
	LPTSTR comspec;
	BOOL bWait = FALSE;
	BOOL bBat  = FALSE;
	BOOL bCreate = FALSE;
	TCHAR szFullCmdLine [CMDLINE_LENGTH];
	PROCESS_INFORMATION prci;
	STARTUPINFO stui;

		
	
	if (_tcsncmp (Rest, _T("/?"), 2) == 0)
	{
		ConOutResPaging(TRUE,STRING_START_HELP1);
		return 0;
	}

	/* get comspec */
	comspec = malloc ( MAX_PATH * sizeof(TCHAR));
	if (comspec == NULL)
	{
		error_out_of_memory();
		return 1;
	}
	SetLastError(0);
	size = GetEnvironmentVariable (_T("COMSPEC"), comspec, 512);
	if(GetLastError() == ERROR_ENVVAR_NOT_FOUND)
	{
		Rest = _T("cmd");
	}
	else
	{
		if (size > MAX_PATH)
		{
			comspec = realloc(comspec,size * sizeof(TCHAR) );
			if (comspec==NULL)
			{
				return 1;
			}
			size = GetEnvironmentVariable (_T("COMSPEC"), comspec, size);
		}		
	}

	nErrorLevel = 0;

	if( !*Rest )
	{
		_tcscpy(Rest,_T("\""));
		_tcscat(Rest,comspec);
		_tcscat(Rest,_T("\""));
	}

	rest = malloc ( _tcslen(Rest) + 1 * sizeof(TCHAR)); 
	if (rest == NULL)
	{
	 if(comspec != NULL)
		free(comspec);
	 error_out_of_memory();
	 return 1;
	}

	param =malloc ( _tcslen(Rest) + 1 * sizeof(TCHAR)); 
	if (rest == NULL)
	{
	 if(comspec != NULL)
		free(comspec);
	 free(rest);
	 error_out_of_memory();
	 return 1;
	}

	param[0] = _T('\0');


	_tcscpy(rest,Rest);
	
	/* Parsing the command that gets called by start, and it's parameters */
	if(!_tcschr(rest,_T('\"')))
	{
		INT i = 0;
		INT count = _tcslen(rest);
		
		/* find the end of the command and start of the args */
		for(i = 0; i < count; i++)
		{
			if(rest[i] == _T(' '))
			{	
				
				_tcscpy(param,&rest[i]);
				rest[i] = _T('\0');
				break;
			}
		}
	}
	else
	{
		INT i = 0;	
		INT count = _tcslen(rest);		
		BOOL bInside = FALSE;

		/* find the end of the command and put the arguments in param */
		for(i = 0; i < count; i++)
		{
			if(rest[i] == _T('\"')) 
				bInside = !bInside;
			if((rest[i] == _T(' ')) && !bInside)
			{					
				_tcscpy(param,&rest[i]);
				rest[i] = _T('\0');
				break;
			}
		}
		i = 0;
		/* remove any slashes */
		while(i < count)
		{
			if(rest[i] == _T('\"'))
				memmove(&rest[i],&rest[i + 1], _tcslen(&rest[i]) * sizeof(TCHAR));
			else
				i++;
		}
	}
	
	/* check for a drive change */
	
	if (!_tcscmp (first + 1, _T(":")) && _istalpha (*first))
	{
		TCHAR szPath[CMDLINE_LENGTH];

		_tcscpy (szPath, _T("A:"));
		szPath[0] = _totupper (*first);
		SetCurrentDirectory (szPath);
		GetCurrentDirectory (CMDLINE_LENGTH, szPath);
		if (szPath[0] != (TCHAR)_totupper (*first))
			ConErrResPuts (STRING_FREE_ERROR1);

		if (rest != NULL) 
		    free(rest);

	    if (param != NULL) 
		    free(param);
		 if (comspec != NULL)
			 free(comspec);
		return 0;
	}
	
	/* get the PATH environment variable and parse it */
	/* search the PATH environment variable for the binary */
	if (!SearchForExecutable (rest, szFullName))
	{
		error_bad_command ();

		if (rest != NULL) 
		    free(rest);

	    if (param != NULL) 
		    free(param);

		 if (comspec != NULL)
			 free(comspec);
		return 1;
	}
	

	/* check if this is a .BAT or .CMD file */
	if (!_tcsicmp (_tcsrchr (szFullName, _T('.')), _T(".bat")) ||
	    !_tcsicmp (_tcsrchr (szFullName, _T('.')), _T(".cmd")))
	{
		bBat = TRUE;				
		memset(szFullCmdLine,0,CMDLINE_LENGTH * sizeof(TCHAR));


		_tcscpy(szFullCmdLine,comspec);

		memcpy(&szFullCmdLine[_tcslen(szFullCmdLine)],_T("\" /K \""), 6 * sizeof(TCHAR));				
		memcpy(&szFullCmdLine[_tcslen(szFullCmdLine)], szFullName, _tcslen(szFullName) * sizeof(TCHAR));		
		memcpy(&szFullCmdLine[1], &szFullCmdLine[0], _tcslen(szFullCmdLine) * sizeof(TCHAR)); 
        szFullCmdLine[0] = _T('\"');					
        szFullCmdLine[_tcslen(szFullCmdLine)] = _T('\"');
	}

#ifdef _DEBUG
		DebugPrintf (_T("[BATCH: %s %s]\n"), szFullName, rest);
#endif
		

#ifdef _DEBUG
		DebugPrintf (_T("[EXEC: %s %s]\n"), szFullName, rest);
#endif
		/* build command line for CreateProcess() */
		if (bBat == FALSE)
		{
		  _tcscpy (szFullCmdLine, first);
		  if( param != NULL )
		  {
			 
		    _tcscat(szFullCmdLine, _T(" ") );
		    _tcscat (szFullCmdLine, param);
		  }
		}

		/* fill startup info */
		memset (&stui, 0, sizeof (STARTUPINFO));
		stui.cb = sizeof (STARTUPINFO);
		stui.dwFlags = STARTF_USESHOWWINDOW;
		stui.wShowWindow = SW_SHOWDEFAULT;
		        
		if (bBat == TRUE)
		{
		 bCreate = CreateProcess (NULL, szFullCmdLine, NULL, NULL, FALSE,
			 CREATE_NEW_CONSOLE, NULL, NULL, &stui, &prci);
		}
		else
		{
			if(!_tcsicmp(szFullName,comspec))
			{
				bCreate = CreateProcess (szFullName, szFullCmdLine, NULL, NULL, FALSE,
					CREATE_NEW_CONSOLE, NULL, NULL, &stui, &prci);
				
			}
			else
			{
				bCreate = CreateProcess (szFullName, szFullCmdLine, NULL, NULL, FALSE,
					DETACHED_PROCESS, NULL, NULL, &stui, &prci);
			}

		}
		
		if (bCreate)
		{
			if (bWait)
			{
				DWORD dwExitCode;
				WaitForSingleObject (prci.hProcess, INFINITE);
				GetExitCodeProcess (prci.hProcess, &dwExitCode);
				nErrorLevel = (INT)dwExitCode;
			}
			CloseHandle (prci.hThread);
			CloseHandle (prci.hProcess);
		/* Get New code page if it has change */
		InputCodePage= GetConsoleCP();
        OutputCodePage = GetConsoleOutputCP();
		}
		else
		{
			ErrorMessage(GetLastError (),
			              _T("Error executing CreateProcess()!!\n"));
		}


	if (rest != NULL) 
	    free(rest);

    if (param != NULL) 
	    free(param);

	 if (comspec != NULL)
		 free(comspec);
	return 0;
}

#endif

/* EOF */
