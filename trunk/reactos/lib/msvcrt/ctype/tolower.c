/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <msvcrt/ctype.h>

#undef tolower
int tolower(int c)
{
   if (_isctype (c, _UPPER))
       return (c - ('A' - 'a'));
   return(c);
}

#undef towlower
int towlower(wint_t c)
//wchar_t towlower(wchar_t c)
{
   if (iswctype (c, _UPPER))
       return (c - (L'A' - L'a'));
   return(c);
}

int _tolower(int c)
{
   if (_isctype (c, _UPPER))
       return (c - ('A' - 'a'));
   return(c);
}

/*
int towlower(wint_t);
int towupper(wint_t);

wchar_t _towlower(wchar_t c)
{
   if (iswctype (c, _UPPER))
       return (c - (L'A' - L'a'));
   return(c);
}
*/


