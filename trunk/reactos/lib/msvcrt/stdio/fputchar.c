#include <msvcrt/stdio.h>
#include <msvcrt/conio.h>


int _fputchar(int c)
{
  return _putch(c);
}

int _fputwchar(wchar_t c)
{
  //return _putch(c);
  return 0;
}
