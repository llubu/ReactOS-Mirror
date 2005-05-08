/*
 *  PAUSE.C - pause internal command.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        started.
 *
 *    16 Jul 1998 (John P Price)
 *        Seperated commands into individual files.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    18-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_PAUSE

/*
 * Perform PAUSE command.
 *
 * FREEDOS extension : If parameter is specified use that as the pause
 *   message.
 *
 * ?? Extend to include functionality of CHOICE if switch chars
 *     specified.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */

INT cmd_pause (LPTSTR cmd, LPTSTR param)
{

#ifdef _DEBUG
	DebugPrintf (_T("cmd_pause: \'%s\' : \'%s\')\n"), cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPuts(STRING_PAUSE_HELP1);
		return 0;
	}

	if (*param)
		ConOutPrintf (param);
	else
		msg_pause ();

	cgetchar ();

	return 0;
}

#endif

/* EOF */
