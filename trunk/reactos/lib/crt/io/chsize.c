/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
#include <io.h>

#define NDEBUG
#include <internal/debug.h>

/*
 * @implemented
 */
int _chsize(int _fd, long size)
{
  DPRINT("_chsize(fd %d, size %d)\n", _fd, size);
  if (_lseek(_fd, size, 0) == -1)
    return -1;
  if (_write(_fd, 0, 0) < 0)
    return -1;
  return 0;
}
