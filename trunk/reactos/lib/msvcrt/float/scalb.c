#include <msvcrt/float.h>
#include <msvcrt/internal/ieee.h>

double _scalb( double __x, long e )
{
	double_t *x = (double_t *)&__x;
	
	x->exponent += e;

	return __x;
}
