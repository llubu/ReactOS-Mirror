#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

void _makepath( char *path, const char *drive, const char *dir, const char *fname, const char *ext )
{
	int dir_len;
	if ( drive != NULL ) {
		strcpy(path,drive);
		strcat(path,":");
	}
	   else
	   (*path)=0;

	if ( dir != NULL ) {
		strcat(path,dir);
		if ( *dir != '\\' )
			strcat(path,"\\");
		dir_len = strlen(dir);
		if (dir_len && *(dir + dir_len - 1) != '\\' )
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

void _wmakepath( wchar_t *path, const wchar_t *drive, const wchar_t *dir, const wchar_t *fname, const wchar_t *ext )
{
	int dir_len;
	if ( drive != NULL ) {
		wcscpy(path,drive);
		wcscat(path,L":");
	}
	   else
	   (*path)=0;

	if ( dir != NULL ) {
		wcscat(path,dir);
		if ( *dir != L'\\' )
			wcscat(path,L"\\");
		dir_len = wcslen(dir);
		if ( dir_len && *(dir + dir_len - 1) != L'\\' )
			wcscat(path,L"\\");
	}
	if ( fname != NULL ) {
		wcscat(path,fname);
		if ( ext != NULL ) {
			if ( *ext != L'.')
				wcscat(path,L".");
			wcscat(path,ext);
		}
	}
}
