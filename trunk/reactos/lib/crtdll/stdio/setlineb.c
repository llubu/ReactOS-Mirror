/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/stdio.h>

// not exported

void setlinebuf(FILE *f)
{
  setvbuf(f, 0, _IOLBF, BUFSIZ);
}
