/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/ctype.h>

#undef ispunct
int ispunct(int c)
{
  return _isctype(c,_PUNCT);
}

#undef iswpunct
int iswpunct(wint_t c)
{
  return iswctype(c,_PUNCT);
}
