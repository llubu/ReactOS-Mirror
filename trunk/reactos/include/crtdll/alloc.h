/*
 * alloc.h
 *
 * Memory management functions. Because most of these functions are
 * actually declared in stdlib.h I have decided to simply include that
 * header file. This file is included by malloc.h. My head hurts...
 *
 * NOTE: In the version of the Standard C++ Library from Cygnus there
 * is also an alloc.h which needs to be on your include path. Most of
 * the time I think the most sensible option would be to get rid of
 * this file.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.1 $
 * $Author: ariadne $
 * $Date: 1999/04/02 21:42:06 $
 *
 */

#ifndef __STRICT_ANSI__

#ifndef	_ALLOC_H_
#define	_ALLOC_H_

#include <crtdll/stdlib.h>

#ifndef RC_INVOKED

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * The structure used to walk through the heap with _heapwalk.
 * TODO: This is a guess at the internals of this structure.
 */
typedef	struct _heapinfo
{
	void*		ptr;
	unsigned int	size;
	int		in_use;
} _HEAPINFO;

int	_heapwalk (_HEAPINFO* pHeapinfo);


#ifndef	_NO_OLDNAMES
int	heapwalk (_HEAPINFO* pHeapinfo);
#endif	/* Not _NO_OLDNAMES */

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _ALLOC_H_ */

#endif	/* Not __STRICT_ANSI__ */

