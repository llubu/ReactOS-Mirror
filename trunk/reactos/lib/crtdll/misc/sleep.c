#include <precomp.h>


void sleep(unsigned long timeout) 
{
	Sleep((timeout)?timeout:1);
}
