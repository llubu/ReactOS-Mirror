/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <crtdll/fcntl.h>
#include <crtdll/io.h>

#undef _fmode
unsigned int _fmode = O_TEXT;

unsigned  int *_fmode_dll = &_fmode;

