/*
 *  DATE.C - date internal command.
 *
 *
 *  History:
 *
 *    08 Jul 1998 (John P. Price)
 *        started.
 *
 *    20 Jul 1998 (John P. Price)
 *        corrected number of days for December from 30 to 31.
 *        (Thanx to Steffen Kaiser for bug report)
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    29-Jul-1998 (Rob Lake)
 *        fixed stand-alone mode.
 *        Added Pacific C compatible dos_getdate functions
 *
 *    09-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added locale support
 *
 *    23-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *
 *    04-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed date input bug.
 *
 *    03-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc  
 */

#include "precomp.h"
#include "resource.h"

#ifdef INCLUDE_CMD_DATE


static WORD awMonths[2][13] =
{
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


static VOID
PrintDateString (VOID)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	switch (nDateFormat)
	{
		case 0: /* mmddyy */
		default:
			LoadString( GetModuleHandle(NULL), STRING_DATE_HELP1, szMsg,sizeof(szMsg)/sizeof(TCHAR));    
			ConOutPrintf (szMsg, cDateSeparator, cDateSeparator);
			break;

		case 1: /* ddmmyy */
			LoadString( GetModuleHandle(NULL), STRING_DATE_HELP2, szMsg,sizeof(szMsg)/sizeof(TCHAR));    
			ConOutPrintf (szMsg, cDateSeparator, cDateSeparator);
			break;

		case 2: /* yymmdd */
			LoadString( GetModuleHandle(NULL), STRING_DATE_HELP3, szMsg,sizeof(szMsg)/sizeof(TCHAR));    
			ConOutPrintf (szMsg, cDateSeparator, cDateSeparator);
			break;
	}
}


static BOOL
ReadNumber (LPTSTR *s, LPWORD lpwValue)
{
	if (_istdigit (**s))
	{
		while (_istdigit (**s))
		{
			*lpwValue = *lpwValue * 10 + **s - _T('0');
			(*s)++;
		}
		return TRUE;
	}
	return FALSE;
}


static BOOL
ReadSeparator (LPTSTR *s)
{
	if (**s == _T('/') || **s == _T('-') || **s == cDateSeparator)
	{
		(*s)++;
		return TRUE;
	}
	return FALSE;
}


static BOOL
ParseDate (LPTSTR s)
{
	SYSTEMTIME d;
	unsigned char leap;
	LPTSTR p = s;

	if (!*s)
		return TRUE;

	GetLocalTime (&d);

	d.wYear = 0;
	d.wDay = 0;
	d.wMonth = 0;

	switch (nDateFormat)
	{
		case 0: /* mmddyy */
		default:
			if (!ReadNumber (&p, &d.wMonth))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wDay))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wYear))
				return FALSE;
			break;

		case 1: /* ddmmyy */
			if (!ReadNumber (&p, &d.wDay))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wMonth))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wYear))
				return FALSE;
			break;

		case 2: /* yymmdd */
			if (!ReadNumber (&p, &d.wYear))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wMonth))
				return FALSE;
			if (!ReadSeparator (&p))
				return FALSE;
			if (!ReadNumber (&p, &d.wDay))
				return FALSE;
			break;
	}

    /* if only entered two digits: */
	/*   assume 2000's if value less than 80 */
	/*   assume 1900's if value greater or equal 80 */
	if (d.wYear <= 99)
	{
		if (d.wYear >= 80)
			d.wYear = 1900 + d.wYear;
		else
			d.wYear = 2000 + d.wYear;
	}

	leap = (!(d.wYear % 4) && (d.wYear % 100)) || !(d.wYear % 400);

	if ((d.wMonth >= 1 && d.wMonth <= 12) &&
		(d.wDay >= 1 && d.wDay <= awMonths[leap][d.wMonth]) &&
		(d.wYear >= 1980 && d.wYear <= 2099))
	{
		SetLocalTime (&d);
		return TRUE;
	}

	return FALSE;
}


INT cmd_date (LPTSTR cmd, LPTSTR param)
{
	LPTSTR *arg;
	INT    argc;
	INT    i;
	BOOL   bPrompt = TRUE;
	INT    nDateString = -1;
	TCHAR szMsg[RC_STRING_MAX_SIZE];

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		LoadString( GetModuleHandle(NULL), STRING_DATE_HELP4, szMsg,sizeof(szMsg)/sizeof(TCHAR));    
		ConOutPuts ((LPTSTR)szMsg);
		return 0;
	}

	/* build parameter array */
	arg = split (param, &argc, FALSE);

	/* check for options */
	for (i = 0; i < argc; i++)
	{
		if (_tcsicmp (arg[i], _T("/t")) == 0)
			bPrompt = FALSE;
		if ((*arg[i] != _T('/')) && (nDateString == -1))
			nDateString = i;
	}

	if (nDateString == -1)
		PrintDate ();

	if (!bPrompt)
	{
		freep (arg);
		return 0;
	}

	if (nDateString == -1)
	{
		while (TRUE)  /* forever loop */
		{
			TCHAR s[40];

			PrintDateString ();
			ConInString (s, 40);
#ifdef _DEBUG
			DebugPrintf (_T("\'%s\'\n"), s);
#endif
			while (*s && s[_tcslen (s) - 1] < _T(' '))
				s[_tcslen (s) - 1] = _T('\0');
			if (ParseDate (s))
			{
				freep (arg);
				return 0;
			}
			ConErrPuts (_T("Invalid date."));
		}
	}
	else
	{
		if (ParseDate (arg[nDateString]))
		{
			freep (arg);
			return 0;
		}
		LoadString( GetModuleHandle(NULL), STRING_DATE_ERROR, szMsg,sizeof(szMsg)/sizeof(TCHAR));    
		ConErrPuts (szMsg);
		
	}

	freep (arg);

	return 0;
}
#endif

