#include <stdio.h>

FILE *fdopen(int handle, char *mode)
{
  FILE *file;
  int rw;

  if ( handle == 0 )
  	return stdin;

  if ( handle == 1 )  
   	return stdout; 
  
  if ( handle == 2 )         
    	return stderr;   
    
  if ( handle == 3 )    
    	return stdaux;  
 
  if ( handle == 4 )    
    	return stdprn;  
 
  file = __alloc_file();
  if (f == NULL)       
	return NULL;       
  file->_file = handle;        
 
  rw = (mode[1] == '+') || (mode[1] && (mode[2] == '+'));     

  if (*mode == 'a')
    lseek(fd, 0, SEEK_END);

  file->_cnt = 0;
  file->_file = handle;
  file->_bufsiz = 0;
  if (rw)
    file->_flag = _IORW;
  else if (*mode == 'r')
    file->_flag = _IOREAD;
  else
    file->_flag = _IOWRT;

  file->_base = f->_ptr = NULL;   
  return file;
}        
                       
                        
                        
                        
                       
                       
                      
                      
                      
                       