#include <crtdll/direct.h>
#include <crtdll/stdlib.h>
#include <crtdll/ctype.h>
#include <windows.h>

int cur_drive = 0;



int _chdrive( int drive )
{
	char d[3];
	if (!( drive >= 1 && drive <= 26 )) 
		return -1;

	if ( cur_drive != drive ) {
		cur_drive = drive;
		d[0] = toupper(cur_drive + '@');
		d[1] = ':';
		d[2] = 0;
		SetCurrentDirectoryA(d);
	}


	return 0;
}
