/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/setmode.c
 * PURPOSE:     Sets the file translation mode
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <crtdll/io.h>
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>


int _setmode(int _fd, int _newmode)
{
	return __fileno_setmode(_fd, _newmode);
}
