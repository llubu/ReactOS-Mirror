/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/debug.h
 * PURPOSE:         Useful debugging macros
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY: 
 *                28/05/98: Created
 */

/*
 * NOTE: Define NDEBUG before including this header to disable debugging
 * macros
 */

#ifndef __INTERNAL_DEBUG
#define __INTERNAL_DEBUG

//#define UNIMPLEMENTED do {DbgPrint("%s at %s:%d is unimplemented, have a nice day\n",__FUNCTION__,__FILE__,__LINE__); for(;;);  } while(0);
#define UNIMPLEMENTED   DbgPrint("WARNING:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__);

/*  FIXME: should probably remove this later  */
#if !defined(CHECKED) && !defined(NDEBUG)
#define CHECKED
#endif

#ifndef assert
#ifndef NASSERT
#define assert(x) if (!(x)) {RtlAssert("#x",__FILE__,__LINE__, ""); }
#else
#define assert(x)
#endif
#endif

#ifndef ASSERT
#ifndef NASSERT
#define ASSERT(x) if (!(x)) {RtlAssert("#x",__FILE__,__LINE__, ""); }
#else
#define ASSERT(x)
#endif
#endif

#ifndef ASSERTMSG
#ifndef NASSERT
#define ASSERTMSG(x,m) if (!(x)) {RtlAssert("#x",__FILE__,__LINE__, m); }
#else
#define ASSERTMSG(x)
#endif
#endif


/* TODO: Make the output of file/line and the debug message atomic */
#define DPRINT1 DbgPrint("(%s:%d) ",__FILE__,__LINE__), DbgPrint
#define CHECKPOINT1 do { DbgPrint("%s:%d\n",__FILE__,__LINE__); } while(0);


#ifndef NDEBUG
#define DPRINT(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT do { DbgPrint("%s:%d\n",__FILE__,__LINE__); } while(0);
#else
#ifdef __GNUC__
#define DPRINT(args...)
#else
#define DPRINT
#endif	/* __GNUC__ */
#define CHECKPOINT
#endif /* NDEBUG */

/*
 * FUNCTION: Assert a maximum value for the current irql
 * ARGUMENTS:
 *        x = Maximum irql
 */
#define ASSERT_IRQL(x) assert(KeGetCurrentIrql()<=(x))
#define assert_irql(x) assert(KeGetCurrentIrql()<=(x))

/* Macros expanding to the appropriate inline assembly to raise a breakpoint */
#if defined(_M_IX86)
#define ASM_BREAKPOINT "\nint $3\n"
#elif defined(_M_ALPHA)
#define ASM_BREAKPOINT "\ncall_pal bpt\n"
#elif defined(_M_MIPS)
#define ASM_BREAKPOINT "\nbreak\n"
#else
#error Unsupported architecture.
#endif

#endif /* __INTERNAL_DEBUG */
