#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/io.h>


int rename(const char* old_, const char* new_)
{
    if (old_ == NULL || new_ == NULL)
        return -1;

    if (!MoveFileA(old_, new_))
        return -1;

    return 0;
}
