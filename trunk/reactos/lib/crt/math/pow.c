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

double pow (double __x, double __y);

double __log2 (double __x);

double __log2 (double __x)
{
  register double __val;
#ifdef __GNUC__
  __asm __volatile__
    ("fld1\n\t"
     "fxch\n\t"
     "fyl2x"
     : "=t" (__val) : "0" (__x));
#else
  //__value = linkme_log2(__x);
  __val = 0;
#endif /*__GNUC__*/
  return __val;
}

/*
 * @implemented
 */
double pow (double __x, double __y)
{
  register double __val;
#ifdef __GNUC__
  register double __exponent;
  long __p = (long) __y;

  if (__x == 0.0 && __y > 0.0)
    return 0.0;
  if (__y == (double) __p)
    {
      double __r = 1.0;
      if (__p == 0)
        return 1.0;
      if (__p < 0)
        {
          __p = -__p;
          __x = 1.0 / __x;
        }
      while (1)
        {
          if (__p & 1)
            __r *= __x;
          __p >>= 1;
          if (__p == 0)
            return __r;
          __x *= __x;
        }
      /* NOTREACHED */
    }
  __asm __volatile__
    ("fmul      %%st(1)         # y * log2(x)\n\t"
     "fst       %%st(1)\n\t"
     "frndint                   # int(y * log2(x))\n\t"
     "fxch\n\t"
     "fsub      %%st(1)         # fract(y * log2(x))\n\t"
     "f2xm1                     # 2^(fract(y * log2(x))) - 1\n\t"
     : "=t" (__val), "=u" (__exponent) :  "0" (__log2 (__x)), "1" (__y));
  __val += 1.0;
  __asm __volatile__
    ("fscale"
     : "=t" (__val) : "0" (__val), "u" (__exponent));
#else
  __val = linkme_pow(__x, __y);
#endif /*__GNUC__*/
  return __val;
}

long double powl (long double __x,long double __y)
{
	return pow(__x,__y/2)*pow(__x,__y/2);
}
