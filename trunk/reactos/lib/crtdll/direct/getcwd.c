#include <windows.h>
#include <direct.h>
#include <stdlib.h>



#undef getcwd
char *getcwd( char *buffer, int maxlen )
{
	return _getcwd(buffer,maxlen);
}

char *_getcwd( char *buffer, int maxlen )
{
	char *cwd;
	int len;
	if ( buffer == NULL ) {
		cwd = malloc(MAX_PATH);
		len = MAX_PATH;
	}
	else {
		cwd = buffer;
		len = maxlen;
	}
	

	if ( GetCurrentDirectory(len,cwd) == 0 )
		return NULL;


	return cwd;
}

