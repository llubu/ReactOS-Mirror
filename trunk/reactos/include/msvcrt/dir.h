/*
 * dir.h
 *
 * Functions for working with directories and path names.
 * This file OBSOLESCENT and only provided for backward compatibility.
 * Please use io.h instead.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.6 $
 * $Author: robd $
 * $Date: 2002/11/24 18:06:00 $
 *
 */

#ifndef __STRICT_ANSI__

#ifndef _DIR_H_
#define _DIR_H_

#include <msvcrt/stdio.h>   /* To get FILENAME_MAX... ugly. */
#include <msvcrt/sys/types.h>   /* To get time_t. */

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Attributes of files as returned by _findfirst et al.
 */
#define _A_NORMAL   0x00000000
#define _A_RDONLY   0x00000001
#define _A_HIDDEN   0x00000002
#define _A_SYSTEM   0x00000004
#define _A_VOLID    0x00000008
#define _A_SUBDIR   0x00000010
#define _A_ARCH     0x00000020

#ifndef _FSIZE_T_DEFINED
typedef unsigned long   _fsize_t;
#define _FSIZE_T_DEFINED
#endif

/*
 * The following structures are filled in by _findfirst or _findnext when
 * they succeed in finding a match.
 */
struct _finddata_t
{
    unsigned    attrib;     /* Attributes, see constants above. */
    time_t      time_create;
    time_t      time_access;    /* always midnight local time */
    time_t      time_write;
    _fsize_t    size;
    char        name[FILENAME_MAX]; /* may include spaces. */
};

struct _finddatai64_t
{
    unsigned    attrib;     /* Attributes, see constants above. */
    time_t      time_create;
    time_t      time_access;    /* always midnight local time */
    time_t      time_write;
    __int64     size;
    char        name[FILENAME_MAX]; /* may include spaces. */
};

struct _wfinddata_t
{
    unsigned    attrib;     /* Attributes, see constants above. */
    time_t      time_create;
    time_t      time_access;    /* always midnight local time */
    time_t      time_write;
    _fsize_t    size;
    wchar_t     name[FILENAME_MAX]; /* may include spaces. */
};

struct _wfinddatai64_t
{
    unsigned    attrib;     /* Attributes, see constants above. */
    time_t      time_create;
    time_t      time_access;    /* always midnight local time */
    time_t      time_write;
    __int64     size;
    wchar_t     name[FILENAME_MAX]; /* may include spaces. */
};

/*
 * Functions for searching for files. _findfirst returns -1 if no match
 * is found. Otherwise it returns a handle to be used in _findnext and
 * _findclose calls. _findnext also returns -1 if no match could be found,
 * and 0 if a match was found. Call _findclose when you are finished.
 */
int _findclose(int nHandle);
int _findfirst(const char* szFilespec, struct _finddata_t* find);
int _findnext(int nHandle, struct _finddata_t* find);
int _findfirsti64(const char* szFilespec, struct _finddatai64_t* find);
int _findnexti64(int nHandle, struct _finddatai64_t* find);
/* Wide character versions */
int _wfindfirst(const wchar_t *_name, struct _wfinddata_t *result);
int _wfindfirsti64(const wchar_t *_name, struct _wfinddatai64_t *result);
int _wfindnext(int handle, struct _wfinddata_t *result);
int _wfindnexti64(int handle, struct _wfinddatai64_t *result);

int _chdir(const char* szPath);
char* _getcwd(char* caBuffer, int nBufferSize);
int _mkdir(const char* szPath);
char* _mktemp(char* szTemplate);
int _rmdir(const char* szPath);

/* Wide character versions */
int _wchdir(const wchar_t *szPath);
wchar_t* _wgetcwd(wchar_t *buffer, int maxlen);
int _wmkdir(const wchar_t *_path);
wchar_t* _wmktemp(wchar_t *_template);
int _wrmdir(const wchar_t *_path);


#ifndef _NO_OLDNAMES

int chdir(const char* szPath);
char* getcwd(char* caBuffer, int nBufferSize);
int mkdir(const char* szPath);
char* mktemp(char* szTemplate);
int rmdir(const char* szPath);

#endif /* Not _NO_OLDNAMES */


#ifdef  __cplusplus
}
#endif

#endif  /* Not _DIR_H_ */

#endif  /* Not __STRICT_ANSI__ */
