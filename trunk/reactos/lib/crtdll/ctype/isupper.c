/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <ctype.h>

#undef isupper
int isupper(int c)
{
  return _isctype(c,_UPPER);
}

int iswupper(int c)
{
  return iswctype(c,_UPPER);
}
