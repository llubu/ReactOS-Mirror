/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

typedef struct {
  unsigned mantissal:32;
  unsigned mantissah:20;
  unsigned exponent:11;
  unsigned sign:1;
} double_t;


#undef _HUGE
double_t _HUGE = { 0x00000, 0x00000, 0x7ff, 0x0 };
double *_HUGE_dll = (double *)&_HUGE; 
