/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/mbstring/ismbkaln.c
 * PURPOSE:     
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              12/04/99: Created
 */
#include <msvcrt/mbctype.h>
#include <msvcrt/ctype.h>

int _ismbbkalnum( unsigned int c )
{
  return ((_jctype+1)[(unsigned char)(c)] & (_KNJ_P));
}
