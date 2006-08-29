/* Math functions for i387.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by John C. Bowman <bowman@ipp-garching.mpg.de>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <math.h>

double fmod (double __x, double __y);

double fmod (double __x, double __y)
{
  register double __val;
#ifdef __GNUC__
  __asm __volatile__
    ("1:        fprem\n\t"
     "fstsw     %%ax\n\t"
     "sahf\n\t"
     "jp        1b"
     : "=t" (__val) : "0" (__x), "u" (__y) : "ax", "cc");
#else
  __val = linkme_fmod(__x, __y);
#endif /*__GNUC__*/
  return __val;
}
