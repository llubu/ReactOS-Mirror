#include <crtdll/stdlib.h>


typedef int (* new_handler_t)( size_t );

new_handler_t new_handler;

#undef _set_new_handler
new_handler_t _set_new_handler(new_handler_t hnd)
{
        new_handler_t old = new_handler;
	
	new_handler = hnd;
        
        return old;
}

#undef delete
void operator delete(void* m)
{
	if ( m != NULL )
    		free( m );
}

#undef new
void * operator new( unsigned int s )
{
        return malloc( s );
}
