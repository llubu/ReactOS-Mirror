/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdlib.h>

long double
_atold(const char *ascii)
{
  return _strtold(ascii, 0);
}
