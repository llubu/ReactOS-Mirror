/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/time/strtime.c
 * PURPOSE:     Fills a buffer with a formatted date representation
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrti.h>


char *_strdate( char *datestr )
{
  time_t t;
  struct tm *d;
  char *dt = (char *)datestr;

  if ( datestr == NULL )
    {
      __set_errno(EINVAL);
      return NULL;
    }
  t =  time(NULL);
  d = localtime(&t);
  sprintf(dt,"%d/%d/%d",d->tm_mday,d->tm_mon+1,d->tm_year);
  return dt;
}

wchar_t *_wstrdate( wchar_t *datestr )
{
  time_t t;
  struct tm *d;
  wchar_t *dt = (wchar_t *)datestr;

  if ( datestr == NULL )
    {
      __set_errno(EINVAL);
      return NULL;
    }
  t =  time(NULL);
  d = localtime(&t);
  swprintf(dt,L"%d/%d/%d",d->tm_mday,d->tm_mon+1,d->tm_year);
  return dt;
}
