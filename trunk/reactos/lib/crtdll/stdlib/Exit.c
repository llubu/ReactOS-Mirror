#include <windows.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <libc/atexit.h>


struct __atexit *__atexit_ptr = 0;

void
exit(int status)
{
  //int i;
  struct __atexit *a = __atexit_ptr;
  __atexit_ptr = 0; /* to prevent infinite loops */
  while (a)
  {
    (a->__function)();
    a = a->__next;
  }
/*
  if (__stdio_cleanup_hook)
    __stdio_cleanup_hook();
  for (i=0; i<djgpp_last_dtor-djgpp_first_dtor; i++)
    djgpp_first_dtor[i]();
*/
  /* in case the program set it this way */
  setmode(0, O_TEXT);
  _exit(status);
}



void _exit(int _status)
{
	ExitProcess(_status);
}

void _cexit( void )
{
	// flush
}

void _c_exit( void )
{
	// reset interup vectors
}
