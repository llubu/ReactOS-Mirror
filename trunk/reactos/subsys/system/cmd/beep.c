/*
 *  BEEP.C - beep internal command.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        started.
 *
 *    16 Jul 1998 (John P Price)
 *        Separated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    14-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("beep /?").
 *        Unicode ready!
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Redirection ready!
 *
 *    02-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_BEEP


INT cmd_beep (LPTSTR cmd, LPTSTR param)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	if (_tcsncmp (param, _T("/?"), 2) == 0)
	{
                LoadString(GetModuleHandle(NULL), STRING_BEEP_HELP, szMsg, RC_STRING_MAX_SIZE);
		ConOutPuts(szMsg);
		return 0;
	}

#if 0
	/* check if run in batch mode */
	if (bc == NULL)
		return 1;
#endif
#ifdef __REACTOS__
    Beep (440, 50);
#else
	MessageBeep (-1);
#endif

	return 0;
}
#endif
