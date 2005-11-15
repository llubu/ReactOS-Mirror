/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include "precomp.h"

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <errno.h>
#include <internal/file.h>


/*
 * @implemented
 */
int setvbuf(FILE *f, char *buf, int type, size_t len)
{
  int mine=0;
  if (!__validfp (f) ) {
      	__set_errno (EINVAL);
      	return 0;
  }
  if ( f->_base != NULL )
  	fflush(f);
  /* Cannot use _IOLBF as flag value because _IOLBF is equal to _IOSTRG */
  if (type == _IOLBF)
      type = _IO_LBF;

  switch (type)
  {
  case _IOFBF:
  case _IO_LBF:
    if (len <= 0) {
	__set_errno (EINVAL);
	return EOF;
    }
    if (buf == 0)
    {
      buf = (char *)malloc(len+1);
      if (buf == NULL) {
	__set_errno (ENOMEM);
	return -1;
      }
      mine = 1;
    }
    /* FALLTHROUGH */
  case _IONBF:
    if (f->_base != NULL && f->_flag & _IOMYBUF)
      free(f->_base);
    f->_cnt = 0;

    f->_flag &= ~(_IONBF|_IOFBF|_IO_LBF|_IOUNGETC);
    f->_flag |= type;
    if (type != _IONBF)
    {
      if (mine)
	f->_flag |= _IOMYBUF;
      f->_ptr = f->_base = buf;
      f->_bufsiz = len;
    }
    else
    {
      f->_base = 0;
      f->_bufsiz = 0;
    }
    return 0;
  default:
      __set_errno (EINVAL);
      return EOF;
  }
}
