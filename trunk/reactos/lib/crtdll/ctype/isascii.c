/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/ctype/isascii.c
 * PURPOSE:     Checks if a character is ascii
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <msvcrt/ctype.h>

/*
 * @implemented
 */
int __isascii(int c)
{
    return (!((c)&(~0x7f)));
}

/*
 * @implemented
 */
int iswascii(wint_t c)
{
    return __isascii(c);
}
