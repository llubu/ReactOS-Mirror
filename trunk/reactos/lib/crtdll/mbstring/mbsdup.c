/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/mbstring/hanzen.c
 * PURPOSE:     Duplicates a multi byte string
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
		Modified from DJGPP strdup
 *              12/04/99: Created
 */

#include <crtdll/mbstring.h>
#include <crtdll/stdlib.h>

unsigned char * _mbsdup(const unsigned char *_s)
{
	char *rv;
	if (_s == 0)
		return 0;
	rv = (char *)malloc(_mbslen(_s) + 1);
	if (rv == 0)
		return 0;
	_mbscpy(rv, _s);
	return rv;
}
