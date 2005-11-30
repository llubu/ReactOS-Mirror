/* Copyright (C) 1991 Free Software Foundation, Inc.
This file is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#include <precomp.h>

/*
 * Read a word (int) from STREAM.
 *
 * @implemented
 */
int _getw(FILE *stream)
{
  int w;

  /* Is there a better way?  */
  if (fread( &w, sizeof(w), 1, stream) != 1) {
    // EOF is a legitimate integer value so users must
    // check feof or ferror to verify an EOF return.
    return(EOF);
  }
  return(w);
}

