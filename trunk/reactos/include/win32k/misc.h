#ifndef __WIN32K_MISC_H
#define __WIN32K_MISC_H

/* Process context in which miniport driver is opened/used */
extern PEPROCESS W32kDeviceProcess;

BOOLEAN
STDCALL
W32kInitialize (VOID);

VOID
FASTCALL
DestroyThreadWindows(struct _ETHREAD *Thread);

#endif /* __WIN32K_MISC_H */
