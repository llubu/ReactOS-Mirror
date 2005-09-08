/* COPYRIGHT:       See COPYING in the top level directory
 *                  Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/math.c
 * PURPOSE:         Math functions for i387.
 * PROGRAMMER:      John C. Bowman <bowman@ipp-garching.mpg.de>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

double atan (double __x);
double ceil (double __x);
double cos (double __x);
double fabs (double __x);
double floor (double __x);
double log (double __x);
double __log2 (double __x);
double pow (double __x, double __y);
double sin (double __x);
double sqrt (double __x);
double tan (double __x);

int _fltused = 0x9875;

double atan (double __x)
{
  register double __value;
  __asm __volatile__
    ("fld1\n\t"
     "fpatan"
     : "=t" (__value) : "0" (__x));

  return __value;
}

/*
 * @implemented
 */
double ceil (double __x)
{
  register double __value;
  __volatile unsigned short int __cw, __cwtmp;

  __asm __volatile ("fnstcw %0" : "=m" (__cw));
  __cwtmp = (__cw & 0xf3ff) | 0x0800; /* rounding up */
  __asm __volatile ("fldcw %0" : : "m" (__cwtmp));
  __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));
  __asm __volatile ("fldcw %0" : : "m" (__cw));

  return __value;
}

/*
 * @implemented
 */
double cos (double __x)
{
  register double __value;
  __asm __volatile__
    ("fcos"
     : "=t" (__value): "0" (__x));

  return __value;
}

/*
 * @implemented
 */
double fabs (double __x)
{
  register double __value;
  __asm __volatile__
    ("fabs"
     : "=t" (__value) : "0" (__x));

  return __value;
}

/*
 * @implemented
 */
double floor (double __x)
{
  register double __value;
  __volatile unsigned short int __cw, __cwtmp;

  __asm __volatile ("fnstcw %0" : "=m" (__cw));
  __cwtmp = (__cw & 0xf3ff) | 0x0400; /* rounding down */
  __asm __volatile ("fldcw %0" : : "m" (__cwtmp));
  __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));
  __asm __volatile ("fldcw %0" : : "m" (__cw));

  return __value;
}

/*
 * @implemented
 */
double log (double __x)
{
  register double __value;
  __asm __volatile__
    ("fldln2\n\t"
     "fxch\n\t"
     "fyl2x"
     : "=t" (__value) : "0" (__x));

  return __value;
}

double __log2 (double __x)
{
  register double __value;
  __asm __volatile__
    ("fld1\n\t"
     "fxch\n\t"
     "fyl2x"
     : "=t" (__value) : "0" (__x));

  return __value;
}

/*
 * @implemented
 */
double pow (double __x, double __y)
{
  register double __value, __exponent;
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
     : "=t" (__value), "=u" (__exponent) :  "0" (__log2 (__x)), "1" (__y));
  __value += 1.0;
  __asm __volatile__
    ("fscale"
     : "=t" (__value) : "0" (__value), "u" (__exponent));

  return __value;
}

/*
 * @implemented
 */
double sin (double __x)
{
  register double __value;
  __asm __volatile__
    ("fsin"
     : "=t" (__value) : "0" (__x));

  return __value;
}

/*
 * @implemented
 */
double sqrt (double __x)
{
  register double __value;
  __asm __volatile__
    ("fsqrt"
     : "=t" (__value) : "0" (__x));

  return __value;
}

/*
 * @implemented
 */
double tan (double __x)
{
  register double __value;
  register double __value2 __attribute__ ((unused));
  __asm __volatile__
    ("fptan"
     : "=t" (__value2), "=u" (__value) : "0" (__x));

  return __value;
}
