/* $Id: echo.c,v 1.2 2003/08/07 09:27:42 hbirr Exp $
 *
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
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>

#include "cmd.h"
#include "batch.h"


INT CommandEcho (LPTSTR cmd, LPTSTR param)
{
#ifdef _DEBUG
	DebugPrintf (_T("CommandEcho '%s' : '%s'\n"), cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts ("Displays a message or switches command echoing on or off.\n"
			    "\n"
			    "  ECHO [ON | OFF]\n"
			    "  ECHO [message]\n"
			    "  ECHO.             prints an empty line\n"
			    "\n"
			    "Type ECHO without a parameter to display the current ECHO setting.");
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
			ConOutPrintf (_T("ECHO is %s\n"), bEcho ? D_ON : D_OFF);
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
		ConOutPuts (_T("Display a messages without trailing carridge return and line feed.\n"
		               "\n"
		               "  ECHOS message"));
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
		ConOutPuts (_T("Displays a message to the standard error.\n"
		               "\n"
		               "  ECHOERR message\n"
		               "  ECHOERR.           prints an empty line"));
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
		ConOutPuts (_T("Prints a messages to standard error output without trailing carridge return and line feed.\n"
		               "\n"
		               "  ECHOSERR message"));
		return 0;
	}

	if (*param)
		ConOutPrintf (_T("%s"), param);

	return 0;
}

/* EOF */
