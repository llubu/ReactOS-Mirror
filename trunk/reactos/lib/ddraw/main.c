
/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library 
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"


HRESULT WINAPI Create_DirectDraw (LPGUID pGUID, LPDIRECTDRAW* pIface,
										IUnknown* pUnkOuter, BOOL ex)
{   
    IDirectDrawImpl* This = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawImpl));

	if (This == NULL) 
		return E_OUTOFMEMORY;

	ZeroMemory(This,sizeof(IDirectDrawImpl));

	This->lpVtbl = &DirectDraw_VTable;
	This->ref = 1;
	*pIface = (LPDIRECTDRAW)This;

	return This->lpVtbl->Initialize ((LPDIRECTDRAW7)This, pGUID);
}

HRESULT WINAPI DirectDrawCreate (LPGUID lpGUID, LPDIRECTDRAW* lplpDD, LPUNKNOWN pUnkOuter) 
{   
	/* check see if pUnkOuter is null or not */
	if (pUnkOuter)
	{
		/* we do not use same error code as MS, ms use 0x8004110 */
		return DDERR_INVALIDPARAMS; 
	}
	
	
	return Create_DirectDraw (lpGUID, lplpDD, pUnkOuter, FALSE);
}
 
HRESULT WINAPI DirectDrawCreateEx(LPGUID lpGUID, LPVOID* lplpDD, REFIID iid, LPUNKNOWN pUnkOuter)
{    	
	/* check see if pUnkOuter is null or not */
	if (pUnkOuter)
	{
		/* we do not use same error code as MS, ms use 0x8004110 */
		return DDERR_INVALIDPARAMS; 
	}
	
	/* Is it a DirectDraw 7 Request or not */
	if (!IsEqualGUID(iid, &IID_IDirectDraw7)) 
	{
	  return DDERR_INVALIDPARAMS;
	}

    return Create_DirectDraw (lpGUID, (LPDIRECTDRAW*)lplpDD, pUnkOuter, TRUE);
}

HRESULT WINAPI DirectDrawEnumerateA(
  LPDDENUMCALLBACKA lpCallback, 
  LPVOID lpContext
)
{
    return DD_OK;
}


HRESULT WINAPI DirectDrawEnumerateW(
  LPDDENUMCALLBACKW lpCallback, 
  LPVOID lpContext
)
{
    return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExA(
  LPDDENUMCALLBACKEXA lpCallback, 
  LPVOID lpContext, 
  DWORD dwFlags
)
{
    return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExW(
  LPDDENUMCALLBACKEXW lpCallback, 
  LPVOID lpContext, 
  DWORD dwFlags
)
{
    return DD_OK;
}
 
HRESULT WINAPI DirectDrawCreateClipper(
  DWORD dwFlags, 
  LPDIRECTDRAWCLIPPER* lplpDDClipper, 
  LPUNKNOWN pUnkOuter
)
{
    return DD_OK;
}

