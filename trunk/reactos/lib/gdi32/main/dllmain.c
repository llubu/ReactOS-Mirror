/*
 * dllmain.c
 *
 * $Revision: 1.5 $
 * $Author: chorns $
 * $Date: 2002/09/07 15:12:24 $
 *
 */

#include <windows.h>
#include <ntos/gdi.h>
#include <win32k/win32k.h>

/*
 * GDI32.DLL doesn't have an entry point. The initialization is done by a call
 * to GdiDllInitialize(). This call is done from the entry point of USER32.DLL.
 */
BOOL
WINAPI
DllMain (
	HANDLE	hDll,
	DWORD	dwReason,
	LPVOID	lpReserved
	)
{
	return TRUE;
}


BOOL
WINAPI
GdiDllInitialize (
	HANDLE	hDll,
	DWORD	dwReason,
	LPVOID	lpReserved
	)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			GdiProcessSetup ();
			break;

		case DLL_THREAD_ATTACH:
			break;

		default:
			return FALSE;
	}

#if 0
	/* FIXME: working teb handling needed */
	NtCurrentTeb()->GdiTebBatch.Offset = 0;
	NtCurrentTeb()->GdiBatchCount = 0;
#endif

	return TRUE;
}

VOID
WINAPI
GdiProcessSetup (VOID)
{

}

/* EOF */
