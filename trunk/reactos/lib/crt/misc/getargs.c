#include "precomp.h"
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>


extern char*_acmdln;
extern char*_pgmptr;
#undef _environ
extern char**_environ;

#undef __argv
#undef __argc

char**__argv = NULL;
#undef __wargv
wchar_t**__wargv = NULL;
int __argc = 0;

extern HANDLE hHeap;

char* strndup(char* name, int len)
{
    char *s = malloc(len + 1);
    if (s != NULL) {
        strncpy(s, name, len);
        name[len] = 0;
    }
    return s;
}

#define SIZE (4096 / sizeof(char*))

int add(char* name)
{
    char** _new;
    if ((__argc % SIZE) == 0) {
        if (__argv == NULL)
            _new = malloc(sizeof(char*) * SIZE);
        else
            _new = realloc(__argv, sizeof(char*) * (__argc + SIZE));
        if (_new == NULL)
            return -1;
        __argv = _new;
    }
    __argv[__argc++] = name;
    return 0;
}

int expand(char* name, int flag)
{
    char* s;
    WIN32_FIND_DATAA fd;
    HANDLE hFile;
    BOOLEAN first = TRUE;
    char buffer[256];
    int pos;

    s = strpbrk(name, "*?");
    if (s && flag) {
        hFile = FindFirstFileA(name, &fd);
        if (hFile != INVALID_HANDLE_VALUE) {
            while(s != name && *s != '/' && *s != '\\')
                s--;
            pos = s - name;
            if (*s == '/' || *s == '\\')
                pos++;
            strncpy(buffer, name, pos);
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    strcpy(&buffer[pos], fd.cFileName);
                    if (add(_strdup(buffer)) < 0) {
                        FindClose(hFile);
                        return -1;
                    }
                    first = FALSE;
                }
            }
            while(FindNextFileA(hFile, &fd));
            FindClose(hFile);
        }
    }
    if (first) {
        if (add(name) < 0)
            return -1;
    }
    else
        free(name);
    return 0;
}

/*
 * @unimplemented
 */
int __getmainargs(int* argc, char*** argv, char*** env, int flag)
{
    int i, afterlastspace, ignorespace, len, doexpand;

    /* missing threading init */

    i = 0;
    afterlastspace = 0;
    ignorespace = 0;
    doexpand = flag;

    len = strlen(_acmdln);

    while (_acmdln[i]) {
	if (_acmdln[i] == '"') {
	    if(ignorespace) {
		ignorespace = 0;
	    } else {
		ignorespace = 1;
		doexpand = 0;
	    }
	    memmove(_acmdln + i, _acmdln + i + 1, len - i);
	    len--;
	    continue;
	}

        if (_acmdln[i] == ' ' && !ignorespace) {
            expand(strndup(_acmdln + afterlastspace, i - afterlastspace), doexpand);
            i++;
            while (_acmdln[i]==' ')
                i++;
            afterlastspace=i;
	    doexpand = flag;
        } else {
            i++;
        }
    }

    if (_acmdln[afterlastspace] != 0) {
        expand(strndup(_acmdln+afterlastspace, i - afterlastspace), doexpand);
    }
    HeapValidate(hHeap, 0, NULL);
    *argc = __argc;
    *argv = __argv;
    *env  = _environ;
    _pgmptr = _strdup((char*)argv[0]);
    return 0;
}

/*
 * @unimplemented
 */
void __wgetmainargs(int* argc, wchar_t*** wargv, wchar_t*** wenv,
                    int expand_wildcards, int* new_mode)
{
    extern wchar_t **__winitenv;
    *argc = 0;
    *wargv = NULL;
    *wenv = __winitenv;
}

/*
 * @implemented
 */
int* __p___argc(void)
{
    return &__argc;
}

/*
 * @implemented
 */
char*** __p___argv(void)
{
    return &__argv;
}

#if 0
int _chkstk(void)
{
    return 0;
}
#endif
