/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/unlink.c
 * PURPOSE:     Deletes a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <precomp.h>

#define NDEBUG
#include <internal/debug.h>

/*
 * @implemented
 */
int _wunlink(const wchar_t* filename)
{
    DPRINT("_wunlink('%S')\n", filename);
    if (!DeleteFileW(filename)) {
    	_dosmaperr(GetLastError());
        return -1;
	}
    return 0;
}
