#include <msvcrt/mbstring.h>
#include <msvcrt/string.h>

/*
 * @implemented
 */
unsigned char * _mbscpy(unsigned char *dst, const unsigned char *str)
{
  return (unsigned char*)strcpy((char*)dst,(const char*)str);
}
