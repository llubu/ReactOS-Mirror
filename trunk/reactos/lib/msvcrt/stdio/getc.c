#include <msvcrti.h>


//getc can be a macro
#undef getc

int getc(FILE *fp)
{
        int c = -1;
// check for invalid stream

	if ( !__validfp (fp) ) {
		__set_errno(EINVAL);
		return EOF;
	}
// check for read access on stream

	if ( !OPEN4READING(fp) ) {
		__set_errno(EINVAL);
		return -1;
	}

	if(fp->_cnt > 0) {
		fp->_cnt--;
		c =  (int)*fp->_ptr++;
	} 
	else {
		c =  _filbuf(fp);
	}
	return c;
}

wint_t getwc(FILE *fp)
{
  int c = -1;

  // check for invalid stream
  if (!__validfp(fp))
    {
      __set_errno(EINVAL);
      return EOF;
    }

  // check for read access on stream
  if (!OPEN4READING(fp))
    {
      __set_errno(EINVAL);
      return -1;
    }

  // might check on multi bytes if text mode

  if (fp->_cnt > 0)
    {
      fp->_cnt -= sizeof(wchar_t);
      c = (wint_t )*((wchar_t *)(fp->_ptr))++;
    }
  else
    {
      c = _filwbuf(fp);
    }
  return c;
}




