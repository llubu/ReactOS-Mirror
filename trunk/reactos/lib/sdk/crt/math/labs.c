/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */

/*
 * @implemented
 */
long
labs(long j)
{
  return j<0 ? -j : j;
}
