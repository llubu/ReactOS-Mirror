/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/mbsrchr.c 
 * PURPOSE:     Searches for a character in reverse
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */

#include <msvcrt/mbstring.h>

char *strrchr(const char* szSearch, int cFor);
extern int __mb_cur_max;

/*
 * @implemented
 */
unsigned char * _mbsrchr(const unsigned char *src, unsigned int val)
{
  if (__mb_cur_max > 1)
  {
    unsigned int c;
    unsigned char *match = NULL;

    if (!src)
      return NULL;

    while (1)
    {
      c = _mbsnextc(src);
      if (c == val)
        match = (unsigned char*)src;
      if (!c)
        return match;
      src += (c > 255) ? 2 : 1;
    }
  }

  return strrchr(src, val);
}
