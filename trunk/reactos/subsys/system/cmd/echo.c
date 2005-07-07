/*
 *  ECHO.C - internal echo commands.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        Started.
 *
 *    16 Jul 1998 (John P Price)
 *        Separated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added config.h include
 *
 *    08-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *
 *    19-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection ready!
 *
 *    13-Jul-2000 (Eric Kohl <ekohl@rz-online.de>)
 *        Implemented 'echo.' and 'echoerr.'.
 *
 *    28-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

#include <precomp.h>
#include "resource.h"


INT CommandEcho (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

#ifdef _DEBUG
	DebugPrintf (_T("CommandEcho '%s' : '%s'\n"), cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_ECHO_HELP4);
		return 0;
	}

	if (_tcsicmp (cmd, _T("echo.")) == 0)
	{
		if (param[0] == 0)
			ConOutChar (_T('\n'));
		else
			ConOutPuts (param);
	}
	else
	{
		if (_tcsicmp (param, D_OFF) == 0)
			bEcho = FALSE;
		else if (_tcsicmp (param, D_ON) == 0)
			bEcho = TRUE;
		else if (*param)
			ConOutPuts (param);
		else
		{
			LoadString(CMD_ModuleHandle, STRING_ECHO_HELP5, szMsg, RC_STRING_MAX_SIZE);
			ConOutPrintf(szMsg, bEcho ? D_ON : D_OFF);
		}
	}

	return 0;
}


INT CommandEchos (LPTSTR cmd, LPTSTR param)
{

#ifdef _DEBUG
	DebugPrintf (_T("CommandEchos '%s' : '%s'\n"), cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPuts(STRING_ECHO_HELP1);
		return 0;
	}

	if (*param)
		ConOutPrintf (_T("%s"), param);

	return 0;
}


INT CommandEchoerr (LPTSTR cmd, LPTSTR param)
{

#ifdef _DEBUG
	DebugPrintf (_T("CommandEchoerr '%s' : '%s'\n"), cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPuts(STRING_ECHO_HELP2);
		return 0;
	}

	if (_tcsicmp (cmd, _T("echoerr.")) == 0)
	{
		if (param[0] == 0)
			ConErrChar (_T('\n'));
		else
			ConErrPuts (param);
	}
	else if (*param)
	{
		ConErrPuts (param);
	}

	return 0;
}


INT CommandEchoserr (LPTSTR cmd, LPTSTR param)
{

#ifdef _DEBUG
	DebugPrintf (_T("CommandEchoserr '%s' : '%s'\n"), cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPuts(STRING_ECHO_HELP3);
		return 0;
	}

	if (*param)
		ConOutPrintf (_T("%s"), param);

	return 0;
}

/* EOF */
