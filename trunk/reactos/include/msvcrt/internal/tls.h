/* tls.h */

#ifndef __MSVCRT_INTERNAL_TLS_H
#define __MSVCRT_INTERNAL_TLS_H

#include <msvcrt/stddef.h>

typedef struct _ThreadData
{
  int terrno;			/* *nix error code */
  unsigned long tdoserrno;	/* Win32 error code (for I/O only) */
  unsigned long long tnext;	/* used by rand/srand */

  char *lasttoken;		/* used by strtok */
  wchar_t *wlasttoken;		/* used by wcstok */


  int fpecode;			/* fp exception code */


} THREADDATA, *PTHREADDATA;


int CreateThreadData(void);
void DestroyThreadData(void);

void FreeThreadData(PTHREADDATA ptd);
PTHREADDATA GetThreadData(void);

#endif /* __MSVCRT_INTERNAL_TLS_H */

/* EOF */