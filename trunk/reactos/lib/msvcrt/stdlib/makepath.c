#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

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

void _wmakepath( wchar_t *path, const wchar_t *drive, const wchar_t *dir, const wchar_t *fname, const wchar_t *ext )
{
	int dir_len;
	if ( drive != NULL ) {
		wcscat(path,drive);
		wcscat(path,L":");
	}

	if ( dir != NULL ) {
		wcscat(path,dir);
		if ( *dir != L'\\' )
			wcscat(path,L"\\");
		dir_len = wcslen(dir);
		if ( *(dir + dir_len - 1) != L'\\' )
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
