/*
 *  TYPE.C - type internal command.
 *
 *  History:
 *
 *    07/08/1998 (John P. Price)
 *        started.
 *
 *    07/12/98 (Rob Lake)
 *        Changed error messages
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added support for quoted arguments (type "test file.dat").
 *        Cleaned up.
 *
 *    19-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection ready!
 *
 *    19-Jan-1999 (Paolo Pantaleo <paolopan@freemail.it>)
 *        Added multiple file support (copied from y.c)
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_TYPE


INT cmd_type (LPTSTR cmd, LPTSTR param)
{
	TCHAR  buff[256];
	HANDLE hFile, hConsoleOut;
	DWORD  dwRead;
	DWORD  dwWritten;
	BOOL   bRet;
	INT    argc,i;
	LPTSTR *argv;
	LPTSTR errmsg;
	WCHAR szMsg[RC_STRING_MAX_SIZE];
	
	hConsoleOut=GetStdHandle (STD_OUTPUT_HANDLE);

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		LoadString( GetModuleHandle(NULL), STRING_TYPE_HELP1, (LPTSTR) szMsg,sizeof(szMsg));
        ConOutPuts ((LPTSTR)szMsg);	
		return 0;
	}

	if (!*param)
	{
		error_req_param_missing ();
		return 1;
	}

	argv = split (param, &argc, TRUE);
	
	for (i = 0; i < argc; i++)
	{
		if (_T('/') == argv[i][0])
		{
			LoadString( GetModuleHandle(NULL), STRING_TYPE_ERROR1, (LPTSTR) szMsg,sizeof(szMsg));
            ConErrPrintf ((LPTSTR)szMsg, argv[i] + 1);
			continue;
		}
		hFile = CreateFile(argv[i],
			GENERIC_READ,
			FILE_SHARE_READ,NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,NULL);

		if(hFile == INVALID_HANDLE_VALUE)
		{
			FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
			               FORMAT_MESSAGE_IGNORE_INSERTS |
			               FORMAT_MESSAGE_FROM_SYSTEM,
			               NULL,
			               GetLastError(),
			               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			               (LPTSTR) &errmsg,
			               0,
			               NULL);
			ConErrPrintf (_T("%s - %s"), argv[i], errmsg);
			LocalFree (errmsg);
			continue;
		}

		do
		{
			bRet = ReadFile(hFile,buff,sizeof(buff),&dwRead,NULL);

			if (dwRead>0 && bRet)
				WriteFile(hConsoleOut,buff,dwRead,&dwWritten,NULL);
			
		} while(dwRead>0 && bRet);

		CloseHandle(hFile);
	}	
	
	freep (argv);

	return 0;
}

#endif
