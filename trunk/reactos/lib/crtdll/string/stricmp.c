/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <string.h>
#include <ctype.h>

int
stricmp(const char *s1, const char *s2)
{
  while (toupper(*s1) == toupper(*s2))
  {
    if (*s1 == 0)
      return 0;
    s1++;
    s2++;
  }
  return toupper(*(unsigned const char *)s1) - toupper(*(unsigned const char *)(s2));
}
