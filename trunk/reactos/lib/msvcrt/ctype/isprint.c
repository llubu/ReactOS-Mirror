/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>

#undef isprint
/*
 * @implemented
 */
int isprint(int c)
{
  return _isctype(c,_BLANK | _PUNCT | _ALPHA | _DIGIT);
}

/*
 * @implemented
 */
int iswprint(wint_t c)
{
  return iswctype((unsigned short)c,_BLANK | _PUNCT | _ALPHA | _DIGIT);
}
