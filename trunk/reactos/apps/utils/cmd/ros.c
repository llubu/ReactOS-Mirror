
#include "config.h"

#ifdef __REACTOS__

#include <windows.h>
#include <stdlib.h>
#include <string.h>


void* malloc(size_t _size)
{
   return(HeapAlloc(GetProcessHeap(),
		    0,
		    _size));
}


void free(void* _ptr)
{
   HeapFree(GetProcessHeap(),
	    0,
	    _ptr);
}


void* realloc(void* _ptr, size_t _size)
{
   return(HeapReAlloc(GetProcessHeap(),
		      0,
		      _ptr,
		      _size));
}


char *_strdup(const char *_s)
{
  char *rv;
  if (_s == 0)
    return 0;
  rv = (char *)malloc(strlen(_s) + 1);
  if (rv == 0)
    return 0;
  strcpy(rv, _s);
  return rv;
}


void _makepath( char *path, const char *drive, const char *dir, const char *fname, const char *ext )
{
	int dir_len;
	if ( drive != NULL ) {
		strcat(path,drive);
		strcat(path,":");
	}

	if ( dir != NULL ) {
		strcat(path,dir);
		if ( *dir != '\\' )
			strcat(path,"\\");
		dir_len = strlen(dir);
		if ( *(dir + dir_len - 1) != '\\' ) 
			strcat(path,"\\"); 
	}
	if ( fname != NULL ) {
		strcat(path,fname);
		if ( ext != NULL ) {
			if ( *ext != '.')
				strcat(path,".");
			strcat(path,ext);
		}
	}
}

#endif
