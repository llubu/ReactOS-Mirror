/*
 * stdio.h
 *
 * Definitions of types and prototypes of functions for standard input and
 * output.
 *
 * NOTE: The file manipulation functions provided by Microsoft seem to
 * work with either slash (/) or backslash (\) as the path separator.
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
 * $Revision: 1.8 $
 * $Author$
 * $Date$
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */
/* implemented clearerr feof ferror perror as macros */
/* added _IOCOMMIT */
/* added filbuf and flsbuf and fwalk   */

#ifndef _STDIO_H_
#define _STDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define __need_size_t
#define __need_NULL
#define __need_wchar_t
#define __need_wint_t
#include <msvcrt/stddef.h>


#define _IOREAD   0x0001
#define _IOWRT    0x0002
#define _IOMYBUF  0x0008  /* stdio malloc()'d buffer */
#define _IOEOF    0x0010  /* EOF reached on read */
#define _IOERR    0x0020  /* I/O error from system */
#define _IOSTRG   0x0040  /* Strange or no file descriptor */

#define _IOBINARY 0x040000
#define _IOTEXT   0x000000

#define _IOCOMMIT 0x100000

#define _IODIRTY  0x010000
#define _IOAHEAD  0x020000




/*
 * I used to include stdarg.h at this point, in order to allow for the
 * functions later on in the file which use va_list. That conflicts with
 * using stdio.h and varargs.h in the same file, so I do the typedef myself.
 */
/*
 *#ifndef _VA_LIST
 *#define _VA_LIST
 *typedef   char* va_list;
 *#endif
 */
#include <msvcrt/stdarg.h>

/*
 * FILE should be used as a pointer to an opaque data type. Do not rely on
 * anything else, especially the size or contents of this structure!
 */
#ifndef _FILE_DEFINED
typedef struct {
    char* _ptr;
    int   _cnt;
    char* _base;
    int   _flag;
    int   _file;
    int   _ungotchar;
    int   _bufsiz;
    char* _name_to_remove;
} FILE;
#define _FILE_DEFINED
#endif

//#define _fillsize _bufsiz

/*
 * The three standard file pointers provided by the run time library.
 * NOTE: These will go to the bit-bucket silently in GUI applications!
 */
extern FILE _iob[]; /* an array of FILE */
#define stdin   (&_iob[0])
#define stdout  (&_iob[1])
#define stderr  (&_iob[2])
#define stdaux  (&_iob[3])
#define stdprn  (&_iob[4])

/* Returned by various functions on end of file condition or error. */
#define EOF (-1)

/*
 * The maximum length of a file name. You should use GetVolumeInformation
 * instead of this constant. But hey, this works.
 *
 * NOTE: This is used in the structure _finddata_t (see dir.h) so changing it
 *       is probably not a good idea.
 */
#define FILENAME_MAX    (260)

/*
 * The maximum number of files that may be open at once. I have set this to
 * a conservative number. The actual value may be higher.
 */
#define FOPEN_MAX   (20)

/*
 * File Operations
 */
FILE* fopen(const char* szFileName, const char* szMode);
FILE* freopen(const char* szNewFileName, const char* szNewMode, FILE* fileChangeAssociation);
int fflush(FILE* fileFlush);
int fclose(FILE* fileClose);
#define fcloseall _fcloseall
int remove(const char* szFileName);
int rename(const char* szOldFileName, const char* szNewFileName);
FILE* tmpfile(void);

FILE* _fdopen(int handle, char *mode);
FILE* _wfdopen(int handle, wchar_t *mode);
FILE* _wfopen(const wchar_t *file, const wchar_t *mode);
FILE* _wfreopen(const wchar_t *file, const wchar_t *mode, FILE *f);
FILE* _fsopen(const char *file, const char *mode, int shflag);
FILE* _wfsopen(const wchar_t *file, const wchar_t *mode, int shflag);
int _wremove(const wchar_t* szFileName);
int _wrename(const wchar_t *oldName, const wchar_t *newName);

int _filbuf(FILE* f);
int _flsbuf(int c, FILE* f);
void _fwalk(void (*func)(FILE*)); // not exported
int _fcloseall(void);

/*
 * The maximum size of name (including NUL) that will be put in the user
 * supplied buffer caName.
 * NOTE: This has not been determined by experiment, but based on the
 * maximum file name length above it is probably reasonable. I could be
 * wrong...
 */
#define L_tmpnam    (260)

char* tmpnam(char caName[]);
char* _tempnam(const char *szDir, const char *szPfx);

wchar_t* _wtmpnam(wchar_t *s);
wchar_t *_wtempnam(const wchar_t *dir,const wchar_t *prefix);

#ifndef _NO_OLDNAMES
#define tempnam _tempnam
#endif  /* Not _NO_OLDNAMES */

/*
 * The three possible buffering mode (nMode) values for setvbuf.
 * NOTE: _IOFBF works, but _IOLBF seems to work like unbuffered...
 * maybe I'm testing it wrong?
 */
#define _IOFBF    0x0000     /* full buffered */
#define _IOLBF    0x0040     /* line buffered */
#define _IONBF    0x0004     /* not buffered */

#define _IO_LBF   0x80000    /* this value is used insteat of _IOLBF within the 
                                structure FILE as value for _flags, 
                                because _IOLBF has the same value as _IOSTRG */

int setvbuf(FILE* fileSetBuffer, char* caBuffer, int nMode, size_t sizeBuffer);

/*
 * The buffer size as used by setbuf such that it is equivalent to
 * (void) setvbuf(fileSetBuffer, caBuffer, _IOFBF, BUFSIZ).
 */
#define BUFSIZ  512

void setbuf(FILE* fileSetBuffer, char* caBuffer);

/*
 * Pipe Operations
 */
  
int _pclose(FILE* pipeClose);
FILE* _popen(const char* szPipeName, const char* szMode);
FILE* _wpopen(const wchar_t *cm, const wchar_t *md);

#define popen  _popen
#define pclose _pclose

/* Wide character version */
FILE*   _wpopen(const wchar_t* szPipeName, const wchar_t* szMode);

/*
 * Formatted Output
 */

int fprintf(FILE* filePrintTo, const char* szFormat, ...);
int printf(const char* szFormat, ...);
int sprintf(char* caBuffer, const char* szFormat, ...);
int _snprintf(char*, size_t, const char*, ...);
int vfprintf(FILE* filePrintTo, const char* szFormat, va_list varg);
int vprintf(const char* szFormat, va_list varg);
int vsprintf(char* caBuffer, const char* szFormat, va_list varg);
int _vsnprintf(char* caBuffer, size_t cnt, const char* szFormat, va_list varg);
int _vsnwprintf (wchar_t*, size_t, const wchar_t*, va_list);

/* Wide character versions */
int fwprintf(FILE* filePrintTo, const wchar_t* wsFormat, ...);
int wprintf(const wchar_t* wsFormat, ...);
int swprintf(wchar_t* wcaBuffer, const wchar_t* wsFormat, ...);
int vfwprintf(FILE* filePrintTo, const wchar_t* wsFormat, va_list varg);
int vwprintf(const wchar_t* wsFormat, va_list varg);
int vswprintf(wchar_t* wcaBuffer, const wchar_t* wsFormat, va_list varg);

/*
 * Formatted Input
 */

int fscanf(FILE* fileReadFrom, const char* szFormat, ...);
int scanf(const char* szFormat, ...);
int sscanf(const char* szReadFrom, const char* szFormat, ...);

/* Wide character versions */
int fwscanf(FILE* fileReadFrom, const wchar_t* wsFormat, ...);
int wscanf(const wchar_t* wsFormat, ...);
int swscanf(const wchar_t* wsReadFrom, const wchar_t* wsFormat, ...);

/*
 * Character Input and Output Functions
 */

int fgetc(FILE* fileRead);
char* fgets(char* caBuffer, int nBufferSize, FILE* fileRead);
int fputc(int c, FILE* fileWrite);
int fputs(const char* szOutput, FILE* fileWrite);
int getc(FILE* fileRead);
int getchar(void);
char* gets(char* caBuffer);  /* Unsafe: how does gets know how long the buffer is? */
int putc(int c, FILE* fileWrite);
int putchar(int c);
int puts(const char* szOutput);
int ungetc(int c, FILE* fileWasRead);

/* Wide character versions */
wint_t fgetwc(FILE* fileRead);
wint_t fputwc(wchar_t wc, FILE* fileWrite);
wint_t getwc(FILE *fileRead); // not exported from crtdll

/* TODO: check type wint_t, why doesn't compare to WEOF correctly ??? */
/* wint_t putwc(wint_t wc, FILE* fileWrite); */
int putwc(wint_t wc, FILE* fileWrite);

wint_t putwchar(wint_t c);
int _putws(const wchar_t* ws);

wint_t ungetwc(wchar_t wc, FILE* fileWasRead);

wint_t _filwbuf(FILE *f);
wint_t _flswbuf(wchar_t c, FILE *f);

/*
 * Not exported by CRTDLL.DLL included for reference purposes.
 */
#if 0
wchar_t* fgetws(wchar_t* wcaBuffer, int nBufferSize, FILE* fileRead);
int fputws(const wchar_t* wsOutput, FILE* fileWrite);
int getwc(FILE* fileRead);
int getwchar();
wchar_t* getws(wchar_t* wcaBuffer);
#endif  /* 0 */

/* NOTE: putchar has no wide char equivalent even in tchar.h */

/*
 * Direct Input and Output Functions
 */

size_t fread(void* pBuffer, size_t sizeObject, size_t sizeObjCount, FILE* fileRead);
size_t fwrite(const void* pObjArray, size_t sizeObject, size_t sizeObjCount, FILE* fileWrite);

/*
 * File Positioning Functions
 */

/* Constants for nOrigin indicating the position relative to which fseek
 * sets the file position. Enclosed in ifdefs because io.h could also
 * define them. (Though not anymore since io.h includes this file now.) */
#ifndef SEEK_SET
#define SEEK_SET    (0)
#endif

#ifndef SEEK_CUR
#define SEEK_CUR    (1)
#endif

#ifndef SEEK_END
#define SEEK_END    (2)
#endif

int fseek(FILE* fileSetPosition, long lnOffset, int nOrigin);
long ftell(FILE* fileGetPosition);
void rewind(FILE* fileRewind);

/*
 * An opaque data type used for storing file positions... The contents of
 * this type are unknown, but we (the compiler) need to know the size
 * because the programmer using fgetpos and fsetpos will be setting aside
 * storage for fpos_t structres. Actually I tested using a byte array and
 * it is fairly evident that the fpos_t type is a long (in CRTDLL.DLL).
 * Perhaps an unsigned long? TODO?
 */
typedef long fpos_t;

int fgetpos(FILE* fileGetPosition, fpos_t* pfpos);
int fsetpos(FILE* fileSetPosition, const fpos_t* pfpos);

/*
 * Error Functions
 */
#if 0
void clearerr(FILE* fileClearErrors);
int feof(FILE* fileIsAtEnd);
int ferror(FILE* fileIsError);
void perror(const char* szErrorMessage);
#endif

void _wperror(const wchar_t *s);

#define clearerr(f)  (((f)->_flag) &= ~(_IOERR|_IOEOF))
#define feof(f)      (((f)->_flag&_IOEOF)!=0)
/* #define ferror(f)    (((f)->_flag&_IOERR)!=0) */
int ferror(FILE* fileIsError);
#define perror(s)    (fprintf(stderr, "%s: %s\n", (s), _strerror(NULL)))

/*
 * Non ANSI functions
 */

#ifndef __STRICT_ANSI__
int _fgetchar(void);
int _fputchar(int c);
FILE* _fdopen(int nHandle, char* szMode);

#ifndef _NO_OLDNAMES
#define fgetchar    _fgetchar
#define fputchar    _fputchar
#define fdopen      _fdopen
#endif  /* Not _NO_OLDNAMES */

#endif  /* Not __STRICT_ANSI__ */

#ifdef __cplusplus
}
#endif

#endif /* _STDIO_H_ */
