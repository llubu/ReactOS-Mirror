/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/getch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <crtdll/conio.h>
#include <crtdll/stdio.h>
#include <crtdll/io.h>

extern int char_avail;
extern int ungot_char;


int
_getch(void)
{
  
  DWORD  NumberOfCharsRead = 0;
  char c;
  if (char_avail)
  {
    c = ungot_char;
    char_avail = 0;
  }
  else
  {	
	ReadConsoleA(_get_osfhandle(stdin->_file), &c,1,&NumberOfCharsRead ,NULL);
	
  }
  if ( c == 10 )
	c = 13;
  putchar(c);
  return c;
}
