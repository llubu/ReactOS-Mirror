/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrti.h>


char* strcpy(char *to, const char *from)
{
  char *save = to;

  for (; (*to = *from); ++from, ++to);
  return save;
}
