#include <windows.h>
#include <io.h>

long
_filelength(int _fd)
{
	return GetFileSize(_get_osfhandle(_fd),NULL);
}
