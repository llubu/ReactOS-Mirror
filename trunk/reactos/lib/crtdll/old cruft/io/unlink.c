/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/unlink.c
 * PURPOSE:     Deletes a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <precomp.h>
#include <msvcrt/io.h>


/*
 * @implemented
 */
int _unlink( const char *filename )
{
	if ( !DeleteFileA(filename) )
		return -1;
	return 0;
}
