/*
 *  CHCP.C - chcp internal command.
 *
 *
 *  History:
 *
 *    23-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Started.
 *
 */

#include "config.h"

#ifdef INCLUDE_CMD_CHCP

#include "precomp.h"
#include <tchar.h>
#include <stdlib.h>
#include <string.h>


INT CommandChcp (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *arg;
	INT    args;
	UINT uOldCodePage;
	UINT uNewCodePage;

	/* print help */
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Displays or sets the active code page number.\n\n"
		               "CHCP [nnn]\n\n"
		               "  nnn   Specifies the active code page number.\n\n"
		               "Type CHCP without a parameter to display the active code page number."));
		return 0;
	}

	/* get parameters */
	arg = split (param, &args, FALSE);

	if (args == 0)
	{
		/* display active code page number */
		ConOutPrintf (_T("Active code page: %u\n"), GetConsoleCP ());
		return 0;
	}

	if (args >= 2)
	{
		/* too many parameters */
		ConErrPrintf (_T("Invalid parameter format - %s\n"), param);
		return 1;
	}


	/* save old code page */
	uOldCodePage = GetConsoleCP ();

	uNewCodePage = (UINT)_ttoi (arg[0]);

	if (uNewCodePage == 0)
	{
		ConErrPrintf (_T("Parameter format incorrect - %s\n"), arg[0]);
		freep (arg);
		return 1;
	}

	if (!SetConsoleCP (uNewCodePage))
	{
		ConErrPrintf (_T("Invalid code page\n"));
	}
	else
	{
		SetConsoleOutputCP (uNewCodePage);
		InitLocale ();
	}

	freep (arg);

	return 0;
}

#endif /* INCLUDE_CMD_CHCP */
