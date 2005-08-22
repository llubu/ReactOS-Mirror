/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>
#include <msvcrt/stdio.h>
#include <msvcrt/wchar.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

// putc can be a macro
#undef putc
#undef putwc

/*
 * @implemented
 */
int putc(int c, FILE* fp)
{
// valid stream macro should check that fp is dword aligned
	if (!__validfp (fp)) {
		__set_errno(EINVAL);
		return -1;
	}
// check for write access on fp

	if ( !OPEN4WRITING(fp)  ) {
		__set_errno(EINVAL);
		return -1;
	}
	
	fp->_flag |= _IODIRTY;
	if (fp->_cnt > 0 ) {
		fp->_cnt--;
       		*(fp)->_ptr++ = (unsigned char)c;
		return (int)(unsigned char)c; 
	}
	else {
		return _flsbuf((unsigned char)c,fp);
	}
	return EOF;
}

//wint_t putwc(wint_t c, FILE* fp)
//int putwc(wchar_t c, FILE* fp)
int putwc(wint_t c, FILE* fp)
{
	// might check on multi bytes if text mode
 
        if (fp->_cnt > 0 ) {
                fp->_cnt-= sizeof(wchar_t);
 		*((wchar_t *)(fp->_ptr))  = c;
                fp->_ptr += sizeof(wchar_t);
		return (wint_t)c;
        }
        else
                return  _flswbuf(c,fp);

        return -1;


}
