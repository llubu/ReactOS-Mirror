/*
 *  IF.C - if internal batch command.
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
 *    07-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("if /?") and cleaned up.
 *
 *    21-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection ready!
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"
#include "batch.h"


#define X_EXEC 1
#define X_EMPTY 0x80


INT cmd_if (LPTSTR cmd, LPTSTR param)
{
	INT x_flag = 0; /* when set cause 'then' clause to be executed */
	LPTSTR pp;

#ifdef _DEBUG
	DebugPrintf ("cmd_if: (\'%s\', \'%s\'\n", cmd, param);
#endif

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("IF :"));
		return 0;
	}

	/* First check if param string begins with word 'not' */
	if (!_tcsnicmp (param, _T("not"), 3) && _istspace (*(param + 3)))
	{
		x_flag = X_EXEC;            /* Remember 'NOT' */
		param += 3;                 /* Step over 'NOT' */
		while (_istspace (*param))  /* And subsequent spaces */
			param++;
	}

	/* Check for 'exist' form */
	if (!_tcsnicmp (param, _T("exist"), 5) && _istspace (*(param + 5)))
	{
		param += 5;
		while (_istspace (*param))
			param++;

		pp = param;
		while (*pp && !_istspace (*pp))
			pp++;

		if (*pp)
		{
			WIN32_FIND_DATA f;
			HANDLE hFind;

			*pp++ = _T('\0');
			hFind = FindFirstFile (param, &f);
			x_flag ^= (hFind != INVALID_HANDLE_VALUE) ? 0 : X_EXEC;
			if (hFind != INVALID_HANDLE_VALUE)
				FindClose (hFind);
		}
		else
			return 0;
	}

	/* Check for 'errorlevel' form */
	else if (!_tcsnicmp (param, _T("errorlevel"), 10) && _istspace (*(param + 10)))
	{
		INT n = 0;

		pp = param + 10;
		while (_istspace (*pp))
			pp++;

		while (_istdigit (*pp))
			n = n * 10 + (*pp++ - _T('0'));

		x_flag ^= (nErrorLevel < n) ? 0 : X_EXEC;

		x_flag |= X_EMPTY;          /* Syntax error if comd empty */
	}

	/* Check that '==' is present, syntax error if not */
	else if (NULL == (pp = _tcsstr (param, _T("=="))))
	{
		error_syntax (NULL);
		return 1;
	}

	else
	{
		/* Change first '='to space to terminate comparison loop */

		*pp = _T(' ');   /* Need a space to terminate comparison loop */
		pp += 2;                /* over '==' */
		while (_istspace (*pp)) /* Skip subsequent spaces */
			pp++;

		_tcscat (pp, _T(" "));  /* Add one space to ensure comparison ends */

		while (*param == *pp)       /* Comparison loop */
		{
			if (_istspace (*param))      /* Terminates on space */
				break;

			param++, pp++;
		}

		if (x_flag ^= (*param != *pp) ? 0 : X_EXEC)
		{
			while (*pp && !_istspace (*pp))  /* Find first space, */
				pp++;

			x_flag |= X_EMPTY;
		}
	}

	if (x_flag & X_EMPTY)
	{
		while (_istspace (*pp)) /* Then skip spaces */
			pp++;

		if (*pp == _T('\0'))    /* If nothing left then syntax err */
		{
			error_syntax (NULL);
			return 1;
		}
	}

	if (x_flag & X_EXEC)
		ParseCommandLine (pp);

	return 0;
}
