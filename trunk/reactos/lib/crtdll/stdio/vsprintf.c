/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <crtdll/internal/file.h>

#if 1
int
vsprintf(char *str, const char *fmt, va_list ap)
{
   abort();
}
#else
int
vsprintf(char *str, const char *fmt, va_list ap)
{
  FILE f;
  int len;

  f._flag = _IOWRT|_IOSTRG;
  f._ptr = str;
  f._cnt = INT_MAX;
  len = _doprnt(fmt, ap, &f);
  *f._ptr = 0;
  return len;
}
#endif
