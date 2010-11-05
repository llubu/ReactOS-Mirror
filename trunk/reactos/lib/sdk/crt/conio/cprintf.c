/*
 * COPYRIGHT:        Winehq
 * PROJECT:          wine
 * FILE:             msvcrt/conio/cprintf.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Magnus Olsen (Imported from wine cvs 2006-05-23)
 */
#ifndef USE_NEW_SPRINTF

#include <precomp.h>

/*
 * @implemented
 */
int
_cprintf(const char *fmt, ...)
{
  char buf[2048], *mem = buf;
  int written, resize = sizeof(buf), retval;
  va_list valist;

  va_start( valist, fmt );

  while ((written = _vsnprintf( mem, resize, fmt, valist )) == -1 ||
          written > resize)
  {
    resize = (written == -1 ? resize * 2 : written + 1);
    if (mem != buf)
       free (mem);
    if (!(mem = (char *)malloc(resize)))
      return  EOF;

    va_end ( valist );
    va_start( valist, fmt );
  }
  va_end ( valist );
  retval = _cputs( mem );
  if (mem != buf)
      free (mem);
  return retval;
}

#endif
