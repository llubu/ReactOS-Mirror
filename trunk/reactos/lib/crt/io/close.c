#include "precomp.h"
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

/*
 * @implemented
 */
int _close(int _fd)
{
  DPRINT("_close(fd %d)\n", _fd);
  if (_fd == -1)
    return -1;
  if (CloseHandle(_get_osfhandle(_fd)) == FALSE)
    return -1;
  return __fileno_close(_fd);
}
