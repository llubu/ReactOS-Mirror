#include <crtdll/stdio.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>
#include <crtdll/errno.h>
#include <crtdll/internal/file.h>


// carriage return line feed conversion is done in filbuf and  flsbuf
#if 0
size_t
fread(void *p, size_t size, size_t count, FILE *iop)
{
  char *ptr = (char *)p;
  int to_read;
  
  to_read = size * count;
 

	
  while ( to_read > 0 ) {
	*ptr = getc(iop) ;
	if ( *ptr == EOF )
		break;
	to_read--;
	ptr++;
  }

	

  return count- (to_read/size);
}


#else
size_t fread(void *vptr, size_t size, size_t count, FILE *iop)
{
	char *ptr = (char *)vptr;
	size_t  to_read ,n_read;

	to_read = size * count;
  	
	//if (!READ_STREAM(iop))
  	//{
      	//	__set_errno (EINVAL);
      	//	return 0;
    	//}

	if (iop == NULL )
  	{
      		__set_errno (EINVAL);
      		return 0;
    	}
	if (feof (iop) || ferror (iop))
    		return 0;

	if (vptr == NULL || to_read == 0)
		return 0;


	while(iop->_cnt > 0) {
                to_read--;
                *ptr++ = getc(iop);
        } 
  
  	 // check to see if this will work with in combination with ungetc
  	 
  	n_read =  _read(fileno(iop), ptr, to_read);
  	if ( n_read != -1 )
  		to_read -= n_read;
  	
         return count- (to_read/size);
} 
#endif

