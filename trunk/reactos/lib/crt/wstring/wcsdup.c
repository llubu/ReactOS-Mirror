#include <msvcrt/errno.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
wchar_t* _wcsdup(const wchar_t* ptr)
{
    wchar_t* dup;

    dup = malloc((wcslen(ptr) + 1) * sizeof(wchar_t));
    if (dup == NULL) {
        __set_errno(ENOMEM);
        return NULL;
    }
    wcscpy(dup, ptr);
    return dup;
}
