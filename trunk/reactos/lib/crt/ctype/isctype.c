/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             msvcrt/ctype/isctype.c
 * PURPOSE:          C Runtime
 * PROGRAMMER:       Copyright (C) 1995 DJ Delorie
 */
 
#include <msvcrt/ctype.h>


extern unsigned short _ctype[];

unsigned short *_pctype = _ctype + 1;
unsigned short *_pwctype = _ctype + 1;


/*
 * @implemented
 */
unsigned short **__p__pctype(void)
{
   return &_pctype;
}

/*
 * @implemented
 */
unsigned short **__p__pwctype(void)
{
   return &_pwctype;
}

/*
 * @implemented
 */
int _isctype(unsigned int c, int ctypeFlags)
{
   return (_pctype[(unsigned char)(c & 0xFF)] & ctypeFlags);
}

/*
 * @implemented
 */
int iswctype(wint_t wc, wctype_t wctypeFlags)
{
   return (_pwctype[(unsigned char)(wc & 0xFF)] & wctypeFlags);
}

/*
 * obsolete
 *
 * @implemented
 */
int is_wctype(wint_t wc, wctype_t wctypeFlags)
{
   return (_pwctype[(unsigned char)(wc & 0xFF)] & wctypeFlags);
}

/* EOF */
