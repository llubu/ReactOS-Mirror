
#include <ntos/ntdef.h>

#define UNIMPLEMENTED DbgPrint("%s in %s:%d is unimplemented\n",__FUNCTION__,__FILE__,__LINE__);

#ifndef NASSERT
#define assert(x) if (!(x)) {DbgPrint("Assertion "#x" failed at %s:%d\n", __FILE__,__LINE__); for(;;);}
#else
#define assert(x)
#endif

#ifdef NDEBUG
#define DPRINT(args...)
#define CHECKPOINT
#else
#define DPRINT(args...) do { DbgPrint("(NTDLL:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT do { DbgPrint("(NTDLL:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);
#endif

#define DPRINT1(args...) do { DbgPrint("(NTDLL:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT1 do { DbgPrint("(NTDLL:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

