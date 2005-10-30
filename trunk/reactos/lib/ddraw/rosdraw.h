#ifndef __DDRAW_PRIVATE
#define __DDRAW_PRIVATE

/********* Includes  *********/

#include <windows.h>
#include <stdio.h>
#include <ddraw.h>
#include <ddk/ddrawi.h>
#include <ddk/d3dhal.h>
#include <ddrawgdi.h>

/******** Main Object ********/

typedef struct 
{
	IDirectDraw7Vtbl* lpVtbl;
	IDirectDraw4Vtbl* lpVtbl_v4;
	IDirectDraw2Vtbl* lpVtbl_v2;
	IDirectDrawVtbl*  lpVtbl_v1;

	DDRAWI_DIRECTDRAW_GBL DirectDrawGlobal;
	DDHAL_DDMISCELLANEOUSCALLBACKS Misc2Callback;
	DDHALINFO HalInfo;	

    HWND window;
    DWORD cooperative_level;
	HDC hdc;
	int Height, Width, Bpp;

	GUID* lpGUID;

} IDirectDrawImpl; 

/******** Surface Object ********/

typedef struct 
{
	IDirectDrawSurface7Vtbl* lpVtbl;
	IDirectDrawSurface3Vtbl* lpVtbl_v3;
   
    IDirectDrawImpl* owner;

	DDRAWI_DDRAWSURFACE_GBL Global; 
	DDRAWI_DDRAWSURFACE_MORE More; 
	DDRAWI_DDRAWSURFACE_LCL Local;

} IDirectDrawSurfaceImpl;

/******** Clipper Object ********/

typedef struct 
{
	IDirectDrawClipperVtbl* lpVtbl;
    LONG ref;

    IDirectDrawImpl* owner;

} IDirectDrawClipperImpl;

/******** Palette Object ********/

typedef struct 
{
	IDirectDrawPaletteVtbl* lpVtbl;
    LONG ref;

    IDirectDrawImpl* owner;

} IDirectDrawPaletteImpl;

/*********** VTables ************/

extern IDirectDraw7Vtbl				DirectDraw7_Vtable;
extern IDirectDrawVtbl				DDRAW_IDirectDraw_VTable;
extern IDirectDraw2Vtbl				DDRAW_IDirectDraw2_VTable;
extern IDirectDraw4Vtbl				DDRAW_IDirectDraw4_VTable;

extern IDirectDrawSurface7Vtbl		DirectDrawSurface7_Vtable;
extern IDirectDrawSurface3Vtbl		DDRAW_IDDS3_Thunk_VTable;

extern IDirectDrawPaletteVtbl		DirectDrawPalette_Vtable;
extern IDirectDrawClipperVtbl		DirectDrawClipper_Vtable;
extern IDirectDrawColorControlVtbl	DirectDrawColorControl_Vtable;
extern IDirectDrawGammaControlVtbl	DirectDrawGammaControl_Vtable;

/********* Prototypes **********/

HRESULT Hal_DirectDraw_Initialize (LPDIRECTDRAW7 );
HRESULT Hal_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 );
VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 );
HRESULT Hal_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7, LPDDSCAPS2, LPDWORD, LPDWORD );	
HRESULT Hal_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7, DWORD, HANDLE ); 
HRESULT Hal_DirectDraw_GetScanLine(LPDIRECTDRAW7 , LPDWORD );
HRESULT Hal_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 ); 
HRESULT Hal_DirectDraw_SetDisplayMode (LPDIRECTDRAW7, DWORD, DWORD, DWORD, DWORD, DWORD );
HRESULT Hal_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX );


HRESULT Hel_DirectDraw_Initialize (LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 );
VOID Hel_DirectDraw_Release (LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 , LPDDSCAPS2 ddsaps, LPDWORD , LPDWORD );	
HRESULT Hel_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7, DWORD, HANDLE ); 
HRESULT Hel_DirectDraw_GetScanLine(LPDIRECTDRAW7 , LPDWORD );
HRESULT Hel_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 );
HRESULT Hel_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 , DWORD , DWORD ,DWORD , DWORD , DWORD );
HRESULT Hel_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX );

/*********** Macros ***********/

#define DX_STUB \
	static BOOL firstcall = TRUE; \
	if (firstcall) \
	{ \
		char buffer[1024]; \
		sprintf ( buffer, "Function %s is not implemented yet (%s:%d)\n", __FUNCTION__,__FILE__,__LINE__ ); \
		OutputDebugStringA(buffer); \
		firstcall = FALSE; \
	} \
	return DDERR_UNSUPPORTED; 

#endif /* __DDRAW_PRIVATE */
