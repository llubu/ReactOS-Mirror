/*
 *  FOR.C - for internal batch command.
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
 *    19 Jul 1998 (Hans B Pufal) [HBP_001]
 *        Implementation of FOR
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"
#include "batch.h"


/*
 * Perform FOR command.
 *
 * First check syntax is correct : FOR %v IN ( <list> ) DO <command>
 *   v must be alphabetic, <command> must not be empty.
 *
 * If all is correct build a new bcontext structure which preserves
 *   the necessary information so that readbatchline can expand
 *   each the command prototype for each list element.
 *
 * You might look on a FOR as being a called batch file with one line
 *   per list element.
 */

INT cmd_for (LPTSTR cmd, LPTSTR param)
{
	LPBATCH_CONTEXT lpNew;
	LPTSTR pp;
	TCHAR  var;

#ifdef _DEBUG
	DebugPrintf ("cmd_for (\'%s\', \'%s\'\n", cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts ("FOR :");
		return 0;
	}

	/* Check that first element is % then an alpha char followed by space */
	if ((*param != _T('%')) || !_istalpha (*(param + 1)) || !_istspace (*(param + 2)))
	{
		error_syntax (_T("bad varable specification."));
		return 1;
	}

	param++;
	var = *param++;               /* Save FOR var name */

	while (_istspace (*param))
		param++;

	/* Check next element is 'IN' */
	if ((_tcsnicmp (param, _T("in"), 2) != 0) || !_istspace (*(param + 2)))
	{
		error_syntax (_T("'in' missing in for statement."));
		return 1;
	}

	param += 2;
	while (_istspace (*param))
		param++;

	/* Folowed by a '(', find also matching ')' */
	if ((*param != _T('(')) || (NULL == (pp = _tcsrchr (param, _T(')')))))
	{
		error_syntax (_T("no brackets found."));
		return 1;
	}

	*pp++ = _T('\0');
	param++;		/* param now points at null terminated list */

	while (_istspace (*pp))
		pp++;

	/* Check DO follows */
	if ((_tcsnicmp (pp, _T("do"), 2) != 0) || !_istspace (*(pp + 2)))
	{
		error_syntax (_T("'do' missing."));
		return 1;
	}

	pp += 2;
	while (_istspace (*pp))
		pp++;

	/* Check that command tail is not empty */
	if (*pp == _T('\0'))
	{
		error_syntax (_T("no command after 'do'."));
		return 1;
	}

	/* OK all is correct, build a bcontext.... */
	lpNew = (LPBATCH_CONTEXT)malloc (sizeof (BATCH_CONTEXT));

	lpNew->prev = bc;
	bc = lpNew;

	bc->hBatchFile = INVALID_HANDLE_VALUE;
	bc->ffind = NULL;
	bc->params = BatchParams (_T(""), param); /* Split out list */
	bc->shiftlevel = 0;
	bc->forvar = var;
	bc->forproto = _tcsdup (pp);

	return 0;
}
