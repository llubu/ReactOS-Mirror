/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/string/wstring.c
 * PURPOSE:         Wide string functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   1998/12/04  RJJ  Cleaned up and added i386 def checks
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

/* GLOBALS *******************************************************************/

// static wchar_t * ___wcstok = NULL;

/* FUNCTIONS *****************************************************************/

int _wcsicmp(const wchar_t* cs,const wchar_t * ct)  
{
	while (towlower(*cs) == towlower(*ct))
	{
    		if (*cs == 0)
      			return 0;
    		cs++;
    		ct++;
	}
	return towlower(*cs) - towlower(*ct);
}


wchar_t* _wcslwr(wchar_t *x)
{
	wchar_t  *y=x;

	while (*y) {
		*y=towlower(*y);
		y++;
	}
	return x;
}

int _wcsnicmp(const wchar_t * cs,const wchar_t * ct,size_t count)
{
  wchar_t *save = (wchar_t *)cs;
  while (towlower(*cs) == towlower(*ct) && (int)(cs - save) < count)
  {
    if (*cs == 0)
      return 0;
    cs++;
    ct++;
  }
  return towlower(*cs) - towlower(*ct);
}


wchar_t *_wcsupr(wchar_t *x)
{
	wchar_t  *y=x;

	while (*y) {
		*y=towupper(*y);
		y++;
	}
	return x;
}


wchar_t* wcscat(wchar_t *dest, const wchar_t *src)
{
  int i, j;
   
  for (j = 0; dest[j] != 0; j++)
    ;
  for (i = 0; src[i] != 0; i++)
    {
      dest[j + i] = src[i];
    }
  dest[j + i] = 0;

  return dest;
}

wchar_t* wcschr(const wchar_t *str, wchar_t ch)
{
  while ((*str) != ((wchar_t) 0))
    {
      if ((*str) == ch)
        {
          return (wchar_t *)str;
        }
      str++;
    }

  return NULL;
}

int wcscmp(const wchar_t *cs, const wchar_t *ct)
{
  while (*cs != '\0' && *ct != '\0' && *cs == *ct)
    {
      cs++;
      ct++;
    }
  return *cs - *ct;
}

wchar_t* wcscpy(wchar_t* str1, const wchar_t* str2)
{
   wchar_t* s = str1;
   while ((*str2)!=0)
     {
	*s = *str2;
	s++;
	str2++;
     }
   *s = 0;
   return(str1);
}


size_t wcscspn(const wchar_t *str,const wchar_t *reject)
{
	wchar_t *s;
	wchar_t *t;
	s=(wchar_t *)str;
	do {
		t=(wchar_t *)reject;
		while (*t) { 
			if (*t==*s) 
				break;
			t++;
		}
		if (*t) 
			break;
		s++;
	} while (*s);
	return s-str; /* nr of wchars */
}


size_t wcslen(const wchar_t *s)
{
  unsigned int len = 0;

  while (s[len] != 0) 
    {
      len++;
    }

  return len;
}

wchar_t * 
wcsncat(wchar_t *dest, const wchar_t *src, size_t count)
{
  int i, j;
   
  for (j = 0; dest[j] != 0; j++)
    ;
  for (i = 0; i < count; i++)
    {
      dest[j + i] = src[i];
      if (src[i] == 0)
        {
          return dest;
        }
    }
  dest[j + i] = 0;

  return dest;
}


int wcsncmp(const wchar_t * cs,const wchar_t * ct,size_t count)
{
  while ((*cs) == (*ct) && count > 0)
  {
    if (*cs == 0)
      return 0;
    cs++;
    ct++;
    count--;
  }
  return (*cs) - (*ct);
}


wchar_t* wcsncpy(wchar_t *dest, const wchar_t *src, size_t count)
{
  int i;
   
  for (i = 0; i < count; i++)
    {
      dest[i] = src[i];
      if (src[i] == 0)
        {
          return dest;
        }
    }
  dest[i] = 0;

  return dest;
}


wchar_t *wcspbrk(const wchar_t *s1, const wchar_t *s2)
{
  const wchar_t *scanp;
  int c, sc;

  while ((c = *s1++) != 0)
  {
    for (scanp = s2; (sc = *scanp++) != 0;)
      if (sc == c)
      {
        return (wchar_t *)(--s1);
      }
  }
  return 0;
}


wchar_t * wcsrchr(const wchar_t *str, wchar_t ch)
{
  unsigned int len = 0;
  while (str[len] != ((wchar_t)0))
    {
      len++;
    }
   
  for (; len > 0; len--)
    {
      if (str[len-1]==ch)
        {
          return (wchar_t *) &str[len - 1];
        }
    }

  return NULL;
}


size_t wcsspn(const wchar_t *str,const wchar_t *accept)
{
	wchar_t  *s;
	wchar_t  *t;
	s=(wchar_t *)str;
	do {
		t=(wchar_t *)accept;
		while (*t) { 
			if (*t==*s) 
				break;
			t++;
		}
		if (!*t) 
			break;
		s++;
	} while (*s);
	return s-str; /* nr of wchars */
}


wchar_t *wcsstr(const wchar_t *s,const wchar_t *b)
{
	wchar_t *x;
	wchar_t *y;
	wchar_t *c;
	x=(wchar_t *)s;
	while (*x) {
		if (*x==*b) {
			y=x;
			c=(wchar_t *)b;
			while (*y && *c && *y==*c) { 
				c++;
				y++; 
			}
			if (!*c)
				return x;
		}
		x++;
	}
	return NULL;
}
