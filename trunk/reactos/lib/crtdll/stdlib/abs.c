/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdlib.h>
#include <msvcrt/crttypes.h>

int
abs(int j)
{
  return j<0 ? -j : j;
}
