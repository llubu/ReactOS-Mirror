/* $Id$
 *
 * reactos/lib/gdi32/misc/stubs.c
 *
 * GDI32.DLL Stubs
 *
 * When you implement one of these functions,
 * remove its stub from this file.
 *
 */

#include "precomp.h"

#define SIZEOF_DEVMODEA_300 124
#define SIZEOF_DEVMODEA_400 148
#define SIZEOF_DEVMODEA_500 156
#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

#define UNIMPLEMENTED DbgPrint("GDI32: %s is unimplemented, please try again later.\n", __FUNCTION__);

/*
 * @unimplemented
 */
BOOL
STDCALL
CancelDC(
	HDC	a0
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}
  

/*
 * @unimplemented
 */
int
STDCALL
DrawEscape(
	HDC		a0,
	int		a1,
	int		a2,
	LPCSTR		a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
EnumObjects(
	HDC		a0,
	int		a1,
	GOBJENUMPROC	a2,
	LPARAM		a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int
STDCALL
Escape(HDC hdc, INT escape, INT in_count, LPCSTR in_data, LPVOID out_data)
{
        UNIMPLEMENTED;
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return 0;
}

/*
 * @unimplemented
 */
HRGN
STDCALL
ExtCreateRegion(
	CONST XFORM *	a0,
	DWORD		a1,
	CONST RGNDATA *	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @implemented
 */
UINT
STDCALL
GetBoundsRect(
	HDC	hdc,
	LPRECT	lprcBounds,
	UINT	flags
	)
{
    return NtGdiGetBoundsRect(hdc,lprcBounds,flags & DCB_RESET);
}


/*
 * @implemented
 */
int
STDCALL
GetMetaRgn(HDC hdc,
           HRGN hrgn)
{
    return NtGdiGetRandomRgn(hdc,hrgn,2);
}


/*
 * @unimplemented
 */
UINT
STDCALL
GetMetaFileBitsEx(
	HMETAFILE	a0,
	UINT		a1,
	LPVOID		a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/*
 * @unimplemented
 */
DWORD
STDCALL
GetFontLanguageInfo(
	HDC 	hDc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
PlayMetaFile(
	HDC		a0,
	HMETAFILE	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ResizePalette(
	HPALETTE	a0,
	UINT		a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
int
STDCALL
SetMetaRgn(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
SetBoundsRect(
	HDC		a0,
	CONST RECT	*a1,
	UINT		a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
SetMapperFlags(
	HDC	a0,
	DWORD	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HMETAFILE
STDCALL
SetMetaFileBitsEx(
	UINT		a0,
	CONST BYTE	*a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
SetSystemPaletteUse(
	HDC	a0,
	UINT	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetTextJustification(
	HDC	a0,
	int	a1,
	int	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
UpdateColors(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
PlayMetaFileRecord(
	HDC		a0,
	LPHANDLETABLE	a1,
	LPMETARECORD	a2,
	UINT		a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumMetaFile(
	HDC			a0,
	HMETAFILE		a1,
	MFENUMPROC		a2,
	LPARAM			a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteEnhMetaFile(
	HENHMETAFILE	a0
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EnumEnhMetaFile(
	HDC		a0,
	HENHMETAFILE	a1,
	ENHMFENUMPROC	a2,
	LPVOID		a3,
	CONST RECT	*a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetEnhMetaFileBits(
	HENHMETAFILE	a0,
	UINT		a1,
	LPBYTE		a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
GetEnhMetaFileHeader(
	HENHMETAFILE	a0,
	UINT		a1,
	LPENHMETAHEADER	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetEnhMetaFilePaletteEntries(
	HENHMETAFILE	a0,
	UINT		a1,
	LPPALETTEENTRY	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetWinMetaFileBits(
	HENHMETAFILE	a0,
	UINT		a1,
	LPBYTE		a2,
	INT		a3,
	HDC		a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
PlayEnhMetaFile(
	HDC		a0,
	HENHMETAFILE	a1,
	CONST RECT	*a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
PlayEnhMetaFileRecord(
	HDC			a0,
	LPHANDLETABLE		a1,
	CONST ENHMETARECORD	*a2,
	UINT			a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HENHMETAFILE
STDCALL
SetEnhMetaFileBits(
	UINT		a0,
	CONST BYTE	*a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HENHMETAFILE
STDCALL
SetWinMetaFileBits(
	UINT			a0,
	CONST BYTE		*a1,
	HDC			a2,
	CONST METAFILEPICT	*a3)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GdiComment(
	HDC		hDC,
	UINT		bytes,
	CONST BYTE	*buffer
	)
{
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_EMF)
  {
     PLDC pLDC = GdiGetLDC(hDC);
     if ( !pLDC )
     {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
     }
     if (pLDC->iType == LDC_EMFLDC)
     {          // Wine port
         return EMFDRV_GdiComment( hDC, bytes, buffer );
     }
  }
#endif
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
AngleArc(
	HDC	hdc,
	int	a1,
	int	a2,
	DWORD	a3,
	FLOAT	a4,
	FLOAT	a5
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented 
 */
BOOL
STDCALL
SetColorAdjustment(
	HDC			hdc,
	CONST COLORADJUSTMENT	*a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @unimplemented
 */
int
STDCALL
EndDoc(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
StartPage(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
EndPage(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
AbortDoc(
	HDC	hdc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
SetAbortProc(
	HDC		hdc,
	ABORTPROC	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ScaleViewportExtEx(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	LPSIZE	a5
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ScaleWindowExtEx(
	HDC	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	LPSIZE	a5
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
UnrealizeObject(
	HGDIOBJ	a0
	)
{
	return NtGdiUnrealizeObject(a0);
}


/*
 * @implemented
 */
BOOL
STDCALL
GdiFlush()
{
    NtGdiFlush();
    return TRUE;
}


/*
 * @unimplemented
 */
int
STDCALL
SetICMMode(
	HDC	a0,
	int	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
CheckColorsInGamut(
	HDC	a0,
	LPVOID	a1,
	LPVOID	a2,
	DWORD	a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HCOLORSPACE
STDCALL
GetColorSpace(
	HDC	hDc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HCOLORSPACE
STDCALL
SetColorSpace(
	HDC		a0,
	HCOLORSPACE	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetDeviceGammaRamp( HDC hdc,
                    LPVOID lpGammaRamp)
{
    BOOL retValue = FALSE;
    if (lpGammaRamp == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        retValue = NtGdiGetDeviceGammaRamp(hdc,lpGammaRamp);
    }

    return retValue;
}


    



/*
 * @unimplemented
 */
BOOL
STDCALL
SetDeviceGammaRamp(
	HDC	a0,
	LPVOID	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ColorMatchToTarget(
	HDC	a0,
	HDC	a1,
	DWORD	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglCopyContext(
	HGLRC	a0,
	HGLRC	a1,
	UINT	a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HGLRC
STDCALL
wglCreateContext(
	HDC	hDc
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HGLRC
STDCALL
wglCreateLayerContext(
	HDC	hDc,
	int	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglDeleteContext(
	HGLRC	a
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
HGLRC
STDCALL
wglGetCurrentContext(VOID)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HDC
STDCALL
wglGetCurrentDC(VOID)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
PROC
STDCALL
wglGetProcAddress(
	LPCSTR		a0
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglMakeCurrent(
	HDC	a0,
	HGLRC	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglShareLists(
	HGLRC	a0,
	HGLRC	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglDescribeLayerPlane(
	HDC			a0,
	int			a1,
	int			a2,
	UINT			a3,
	LPLAYERPLANEDESCRIPTOR	a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
wglSetLayerPaletteEntries(
	HDC		a0,
	int		a1,
	int		a2,
	int		a3,
	CONST COLORREF	*a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
wglGetLayerPaletteEntries(
	HDC		a0,
	int		a1,
	int		a2,
	int		a3,
	COLORREF	*a4
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglRealizeLayerPalette(
	HDC		a0,
	int		a1,
	BOOL		a2
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
wglSwapLayerBuffers(
	HDC		a0,
	UINT		a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* === AFTER THIS POINT I GUESS... =========
 * (based on stack size in Norlander's .def)
 * === WHERE ARE THEY DEFINED? =============
 */








/*
 * @unimplemented
 */
DWORD
STDCALL
IsValidEnhMetaRecord(
	DWORD	a0,
	DWORD	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;

}

/*
 * @unimplemented
 */
DWORD
STDCALL
IsValidEnhMetaRecordOffExt(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;

}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetGlyphOutlineWow(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5,
	DWORD	a6
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}




/*
 * @unimplemented
 */
DWORD
STDCALL
SelectBrushLocal(
	DWORD	a0,
	DWORD	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
SelectFontLocal(
	DWORD	a0,
	DWORD	a1
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
SetFontEnumeration(
	DWORD	a0
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
gdiPlaySpoolStream(
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5
	)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE 
STDCALL 
AddFontMemResourceEx(
	PVOID pbFont,
	DWORD cbFont,
	PVOID pdv,
	DWORD *pcFonts
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL 
AddFontResourceTracking(
	LPCSTR lpString,
	int unknown
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/*
 * @unimplemented
 */
HBITMAP 
STDCALL
ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH 
STDCALL
ClearBrushAttributes(HBRUSH hbm, DWORD dwFlags)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
ColorCorrectPalette(HDC hDC,HPALETTE hPalette,DWORD dwFirstEntry,DWORD dwNumOfEntries)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int
STDCALL
EndFormPage(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/*
 * @unimplemented
 */
DWORD 
STDCALL
GdiAddGlsBounds(HDC hdc,LPRECT prc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GdiArtificialDecrementDriver(LPWSTR pDriverName,BOOL unknown)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiCleanCacheDC(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HDC
STDCALL
GdiConvertAndCheckDC(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HENHMETAFILE 
STDCALL
GdiConvertEnhMetaFile(HENHMETAFILE hmf)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiDrawStream(HDC dc, ULONG l, VOID *v)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GdiGetCodePage(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH 
STDCALL
GdiGetLocalBrush(HBRUSH hbr)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HDC 
STDCALL
GdiGetLocalDC(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HFONT 
STDCALL
GdiGetLocalFont(HFONT hfont)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiIsMetaFileDC(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiIsMetaPrintDC(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiIsPlayMetafileDC(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiValidateHandle(HGDIOBJ hobj)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetBitmapAttributes(HBITMAP hbm)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetBrushAttributes(HBRUSH hbr)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetCharABCWidthsI(
	HDC hdc,
	UINT giFirst,
	UINT cgi,
	LPWORD pgi,
	LPABC lpabc
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GetCharWidthI(
	HDC hdc,
	UINT giFirst,
	UINT cgi,
	LPWORD pgi,
	LPINT lpBuffer
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/*
 * @implemented
 */
ULONG 
STDCALL
GetEUDCTimeStamp(VOID)
{
    return NtGdiGetEudcTimeStampEx(NULL,0,TRUE);
}

/*
 * @implemented
 */
ULONG 
STDCALL
GetFontAssocStatus(HDC hdc)
{
    ULONG retValue = 0;

    if (hdc)
    {
        retValue = NtGdiQueryFontAssocInfo(hdc);
    }

    return retValue;
}

/*
 * @unimplemented
 */
HFONT 
STDCALL
GetHFONT(HDC dc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GetLayout(
	HDC hdc
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetTextExtentExPointWPri(HDC hdc,
                         LPWSTR lpwsz,
                         ULONG cwc,
                         ULONG dxMax,
                         ULONG *pcCh,
                         PULONG pdxOut,
                         LPSIZE psize)
{
    return NtGdiGetTextExtentExW(hdc,lpwsz,cwc,dxMax,pcCh,pdxOut,psize,0);
}

/*
 * @implemented
 */
INT 
STDCALL
GetTextFaceAliasW(HDC hdc,
                  int cChar,
                  LPWSTR pszOut)
{
    INT retValue = 0;
    if ((!pszOut) || (cChar))
    {
        retValue = NtGdiGetTextFaceW(hdc,cChar,pszOut,TRUE);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return retValue;
}



/*
 * @unimplemented
 */
BOOL 
STDCALL
MirrorRgn(HWND hwnd,HRGN hrgn)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/*
 * @unimplemented
 */
DWORD 
STDCALL
QueryFontAssocStatus(VOID)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
RemoveFontMemResourceEx(
	HANDLE fh
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL
RemoveFontResourceTracking(LPCSTR lpString,int unknown)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBITMAP 
STDCALL
SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HBRUSH 
STDCALL
SetBrushAttributes(HBRUSH hbm, DWORD dwFlags)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
SetLayout(
	HDC hdc,
	DWORD dwLayout
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
SetLayoutWidth(HDC hdc,LONG wox,DWORD dwLayout)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
SetMagicColors(HDC hdc,PALETTEENTRY peMagic,ULONG Index)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetVirtualResolution(HDC hdc, int cxVirtualDevicePixel,int cyVirtualDevicePixel,int cxVirtualDeviceMm, int cyVirtualDeviceMm)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
int 
STDCALL
StartFormPage(HDC hdc)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID 
STDCALL
UnloadNetworkFonts(DWORD unknown)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @implemented
 */
BOOL 
STDCALL
GetTextExtentExPointI(HDC hdc,
                      LPWORD pgiIn,
                      int cgi,
                      int nMaxExtent,
                      LPINT lpnFit,
                      LPINT alpDx,
                      LPSIZE lpSize)
{
    return NtGdiGetTextExtentExW(hdc,pgiIn,cgi,nMaxExtent,(ULONG *)lpnFit, (PULONG) alpDx,lpSize,1);
}

/*
 * @implemented
 */
BOOL 
STDCALL
GetTextExtentPointI(HDC hdc,
                    LPWORD pgiIn,
                    int cgi,
                    LPSIZE lpSize)
{
    return NtGdiGetTextExtent(hdc,pgiIn,cgi,lpSize,2);
}





/*
 * @unimplemented
 */
BOOL 
STDCALL
GdiRealizationInfo(HDC hdc, PREALIZATION_INFO pri)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/*
 * @unimplemented
 */
BOOL 
STDCALL
GetETM(HDC hdc,EXTTEXTMETRIC *petm)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiAddGlsRecord(HDC hdc,DWORD unknown1,LPCSTR unknown2,LPRECT unknown3)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
GdiConvertMetaFilePict(HGLOBAL hMem)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @implemented
 */
DEVMODEW *
STDCALL
GdiConvertToDevmodeW(DEVMODEA *dm)
{
  LPDEVMODEW dmw;
  
  dmw = HEAP_alloc(sizeof(DEVMODEW));
#define COPYS(f,len) MultiByteToWideChar ( CP_THREAD_ACP, 0, (LPSTR)dm->f, len, dmw->f, len )
#define COPYN(f) dmw->f = dm->f
  COPYS(dmDeviceName, CCHDEVICENAME );
  COPYN(dmSpecVersion);
  COPYN(dmDriverVersion);
  switch ( dm->dmSize )
    {
    case SIZEOF_DEVMODEA_300:
      dmw->dmSize = SIZEOF_DEVMODEW_300;
      break;
    case SIZEOF_DEVMODEA_400:
      dmw->dmSize = SIZEOF_DEVMODEW_400;
      break;
    case SIZEOF_DEVMODEA_500:
    default: /* FIXME what to do??? */
      dmw->dmSize = SIZEOF_DEVMODEW_500;
      break;
    }
  COPYN(dmDriverExtra);
  COPYN(dmFields);
  COPYN(dmPosition.x);
  COPYN(dmPosition.y);
  COPYN(dmScale);
  COPYN(dmCopies);
  COPYN(dmDefaultSource);
  COPYN(dmPrintQuality);
  COPYN(dmColor);
  COPYN(dmDuplex);
  COPYN(dmYResolution);
  COPYN(dmTTOption);
  COPYN(dmCollate);
  COPYS(dmFormName,CCHFORMNAME);
  COPYN(dmLogPixels);
  COPYN(dmBitsPerPel);
  COPYN(dmPelsWidth);
  COPYN(dmPelsHeight);
  COPYN(dmDisplayFlags); // aka dmNup
  COPYN(dmDisplayFrequency);

  if ( dm->dmSize <= SIZEOF_DEVMODEA_300 )
    return dmw; // we're done with 0x300 fields

  COPYN(dmICMMethod);
  COPYN(dmICMIntent);
  COPYN(dmMediaType);
  COPYN(dmDitherType);
  COPYN(dmReserved1);
  COPYN(dmReserved2);

  if ( dm->dmSize <= SIZEOF_DEVMODEA_400 )
    return dmw; // we're done with 0x400 fields

  COPYN(dmPanningWidth);
  COPYN(dmPanningHeight);

  return dmw;

#undef COPYN
#undef COPYS
}

/*
 * @unimplemented
 */
HENHMETAFILE
STDCALL
GdiCreateLocalEnhMetaFile(HENHMETAFILE hmo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
METAFILEPICT *
STDCALL
GdiCreateLocalMetaFilePict(HENHMETAFILE hmo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
HANDLE 
STDCALL
GdiGetSpoolFileHandle(
	LPWSTR		pwszPrinterName,
	LPDEVMODEW	pDevmode,
	LPWSTR		pwszDocName)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiDeleteSpoolFileHandle(
	HANDLE	SpoolFileHandle)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD 
STDCALL
GdiGetPageCount(
	HANDLE	SpoolFileHandle)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HDC
STDCALL
GdiGetDC(
	HANDLE	SpoolFileHandle)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE 
STDCALL
GdiGetPageHandle(
	HANDLE	SpoolFileHandle,
	DWORD	Page,
	LPDWORD	pdwPageType)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiStartDocEMF(
	HANDLE		SpoolFileHandle,
	DOCINFOW	*pDocInfo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiStartPageEMF(
	HANDLE	SpoolFileHandle)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiPlayPageEMF(
	HANDLE	SpoolFileHandle,
	HANDLE	hemf,
	RECT	*prectDocument,
	RECT	*prectBorder,
	RECT	*prectClip)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiEndPageEMF(
	HANDLE	SpoolFileHandle,
	DWORD	dwOptimization)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiEndDocEMF(
	HANDLE	SpoolFileHandle)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiGetDevmodeForPage(
	HANDLE		SpoolFileHandle,
	DWORD		dwPageNumber,
	PDEVMODEW	*pCurrDM,
	PDEVMODEW	*pLastDM)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiResetDCEMF(
	HANDLE		SpoolFileHandle,
	PDEVMODEW	pCurrDM)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


HBITMAP 
STDCALL 
CreateDIBitmap(HDC
	hDc, const BITMAPINFOHEADER *Header,
	DWORD Init, LPCVOID Bits, const BITMAPINFO *Data,
	UINT ColorUse)
{
	/* FIMXE we need do more thing in user mode */
	return NtGdiCreateDIBitmap(hDc, Header, Init, Bits, Data,  ColorUse);
}

/*
 * @unimplemented
 */
INT
STDCALL
CombineRgn(HRGN  hDest,
                    HRGN  hSrc1,
                    HRGN  hSrc2,
                    INT  CombineMode)
{
    /* FIXME some part should be done in user mode */
    return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode); 
}

/*
 * @unimplemented
 */
HBITMAP STDCALL
CreateBitmap(
    INT  Width,
    INT  Height,
    UINT  Planes,
    UINT  BitsPixel,
    PCVOID pUnsafeBits)
{
    /* FIXME some part should be done in user mode */
    return NtGdiCreateBitmap(Width, Height, Planes, BitsPixel, (LPBYTE) pUnsafeBits);
}




/*
 * @unimplemented
 */
LPWSTR STDCALL
EngGetDriverName(HDEV hdev)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 * wrong info it is not Obsolete GDI Function as http://www.osronline.com/DDKx/graphics/gdioview_20tj.htm say
 */
BOOL STDCALL
EngQueryEMFInfo(HDEV hdev,EMFINFO *pEMFInfo)
{
#if 0
    BOOL retValue = FALSE;
    DHPDEV Dhpdev;

    if ((!hdev) && (!pEMFInfo))
    {
        if ((Dhpdev = NtGdiGetDhpdev(hdev)))
        {
            /* FIXME check if it support or if it is pEMFInfo we got */
            /* FIXME copy the data from Dhpdev to pEMFInfo           */
        }
    }
    return retValue;
#else
    return FALSE;
#endif
}


/*
 * @unimplemented
 */
INT STDCALL 
EngWideCharToMultiByte( UINT CodePage,
                        LPWSTR WideCharString,
                        INT BytesInWideCharString,
                        LPSTR MultiByteString,
                        INT BytesInMultiByteString)
{
  return WideCharToMultiByte(
                         CodePage,
                         0,
                         WideCharString,
                        (BytesInWideCharString/sizeof(WCHAR)), /* Bytes to (in WCHARs) */
                         MultiByteString,
                         BytesInMultiByteString,
                         NULL,
                         NULL);
}



/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bEnum(STROBJ *pstro,ULONG *pc,PGLYPHPOS *ppgpos)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bEnumPositionsOnly(STROBJ *pstro,ULONG *pc,PGLYPHPOS *ppgpos)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL
STROBJ_bGetAdvanceWidths(STROBJ *pso,ULONG iFirst,ULONG c,POINTQF *pptqD)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL
STROBJ_dwGetCodePage(STROBJ  *pstro)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL
STROBJ_vEnumStart(STROBJ *pstro)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL STDCALL
XFORMOBJ_bApplyXform(XFORMOBJ *pxo,ULONG iMode,ULONG cPoints,PVOID pvIn,PVOID pvOut)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
XFORMOBJ_iGetXform(XFORMOBJ *pxo,XFORML *pxform)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
XLATEOBJ_cGetPalette(XLATEOBJ *XlateObj,
		     ULONG PalOutType,
		     ULONG cPal,
		     ULONG *OutPal)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL
XLATEOBJ_hGetColorTransform(XLATEOBJ *pxlo)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG STDCALL
XLATEOBJ_iXlate(XLATEOBJ *XlateObj,
		ULONG Color)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
ULONG * STDCALL
XLATEOBJ_piVector(XLATEOBJ *XlateObj)
{
  return XlateObj->pulXlate;
}

/*
 * @unimplemented
 */
BOOL 
STDCALL
GdiPlayEMF
(
	LPWSTR     pwszPrinterName,
	LPDEVMODEW pDevmode,
	LPWSTR     pwszDocName,
	EMFPLAYPROC pfnEMFPlayFn,
	HANDLE     hPageQuery
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



/*
 * @unimplemented
 */
BOOL
STDCALL
GdiPlayPrivatePageEMF
(
	HANDLE	SpoolFileHandle,
	DWORD	unknown,
	RECT	*prectDocument
)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL GdiInitializeLanguagePack(DWORD InitParam)
{
	UNIMPLEMENTED;
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}


/*
 * @implemented
 */
INT
STDCALL
ExcludeClipRect(IN HDC hdc, IN INT xLeft, IN INT yTop, IN INT xRight, IN INT yBottom)
{
	/* FIXME some part need be done on user mode size */
	return NtGdiExcludeClipRect(hdc, xLeft, yTop, xRight, yBottom);
}

/*
 * @implemented
 */
INT
STDCALL
ExtSelectClipRgn( IN HDC hdc, IN HRGN hrgn, IN INT iMode)
{
	/* FIXME some part need be done on user mode size */
	return NtGdiExtSelectClipRgn(hdc,hrgn, iMode);
}

/*
 * @implemented
 */
BOOL
STDCALL
GdiGradientFill(
    IN HDC hdc,
    IN PTRIVERTEX pVertex,
    IN ULONG nVertex,
    IN PVOID pMesh,
    IN ULONG nMesh,
    IN ULONG ulMode)
{
    /* FIXME some part need be done in user mode */
    return NtGdiGradientFill(hdc, pVertex, nVertex, pMesh, nMesh, ulMode);
}


/*
 * @implemented
 */
BOOL
STDCALL
GdiTransparentBlt(
    IN HDC hdcDst,
    IN INT xDst,
    IN INT yDst,
    IN INT cxDst,
    IN INT cyDst,
    IN HDC hdcSrc,
    IN INT xSrc,
    IN INT ySrc,
    IN INT cxSrc,
    IN INT cySrc,
    IN COLORREF TransColor
)
{
    /* FIXME some part need be done in user mode */
    return NtGdiTransparentBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcSrc, xSrc, ySrc, cxSrc, cySrc, TransColor);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GdiPrinterThunk(
    IN HUMPD humpd,
    DWORD *status,
    DWORD unuse)
{
    /* FIXME figout the protypes, the HUMPD are a STRUCT or COM object */
    /* status contain some form of return value that being save, what it is I do not known */
    /* unsue seam have zero effect, what it is for I do not known */

    // ? return NtGdiSetPUMPDOBJ(humpd->0x10,TRUE, humpd, ?) <- blackbox, OpenRCE info, and api hooks for anylaysing;
    return FALSE;
}

/*
 * @unimplemented
 *
 */
HBITMAP
STDCALL
GdiConvertBitmapV5(
    HBITMAP in_format_BitMap, 
    HBITMAP src_BitMap,
    INT bpp,
    INT unuse)
{
    /* FIXME guessing the prototypes */

    /* 
     * it have create a new bitmap with desired in format, 
     * then convert it src_bitmap to new format
     * and return it as HBITMAP 
     */

    return FALSE;
}


/*
 * @implemented
 *
 */
COLORREF 
STDCALL 
GetBkColor(HDC hdc)
{
    /* FIXME some part are done in user mode */
    return NtGdiGetBkColor(hdc);
}

/*
 * @implemented
 *
 */
int
STDCALL 
GetBkMode(HDC hdc)
{
    /* FIXME some part are done in user mode */
    return NtGdiGetBkMode(hdc);
}

/*
 * @implemented
 *
 */
BOOL 
STDCALL 
GetBrushOrgEx(HDC hdc,LPPOINT pt)
{
    /* FIXME some part are done in user mode */
    return NtGdiGetBrushOrgEx(hdc,pt);
}

/*
 * @implemented
 *
 */
BOOL 
STDCALL 
GetCharABCWidthsFloatW(HDC hdc,UINT FirstChar,UINT LastChar,LPABCFLOAT abcF)
{
    /* FIXME some part are done in user mode */
    return NtGdiGetCharABCWidthsFloat(hdc, FirstChar, LastChar, abcF);
}

/*
 * @implemented
 *
 */
int 
STDCALL 
GetDeviceCaps(HDC hdc,
              int i)
{
    /* FIXME some part need be done in user mode */
    return NtGdiGetDeviceCaps(hdc,i);
}


/*
 * @implemented
 *
 */
BOOL 
STDCALL 
GetCurrentPositionEx(HDC hdc,
                     LPPOINT lpPoint)
{
    /* FIXME some part need be done in user mode */
    return  NtGdiGetCurrentPositionEx(hdc, lpPoint);
}

/*
 * @implemented
 *
 */
int 
STDCALL
GetClipBox(HDC hdc,
           LPRECT lprc)
{
    /* FIXME some part need be done in user mode */
    return  NtGdiGetClipBox(hdc, lprc);
}

/*
 * @implemented
 *
 */
BOOL 
STDCALL
GetCharWidthFloatW(HDC hdc,
                   UINT iFirstChar, 
                   UINT iLastChar, 
                   PFLOAT pxBuffer)
{
    /* FIXME some part need be done in user mode */
    return NtGdiGetCharWidthFloat(hdc, iFirstChar, iLastChar, pxBuffer);
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
GetCharWidth32W(HDC hdc,
               UINT iFirstChar,
               UINT iLastChar,
               LPINT lpBuffer)
{
    /* FIXME some part need be done in user mode */
    return NtGdiGetCharWidth32(hdc, iFirstChar, iLastChar, lpBuffer);
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
GetCharABCWidths(HDC hdc,
                 UINT uFirstChar,
                 UINT uLastChar,
                 LPABC lpabc)
{
    /* FIXME some part need be done in user mode */
    return NtGdiGetCharABCWidths(hdc, uFirstChar, uLastChar, lpabc);
}


/*
 * @implemented
 *
 */
DWORD 
STDCALL
GetFontData(HDC hdc,
            DWORD dwTable,
            DWORD dwOffset,
            LPVOID lpvBuffer,
            DWORD cbData)
{
    if (!lpvBuffer)
    {
       cbData = 0;
    }
    return NtGdiGetFontData(hdc, dwTable, dwOffset, lpvBuffer, cbData);
}


/*
 * @implemented
 *
 */
DWORD 
STDCALL
GetRegionData(HRGN hrgn,
              DWORD nCount,
              LPRGNDATA lpRgnData)
{
    if (!lpRgnData)
    {
        nCount = 0;
    }

    return NtGdiGetRegionData(hrgn,nCount,lpRgnData);
}


/*
 * @implemented
 *
 */
INT
STDCALL
GetRgnBox(HRGN hrgn,
          LPRECT prcOut)
{
    /* FIXME some stuff need be done in user mode */
    return NtGdiGetRgnBox(hrgn, prcOut);
}


/*
 * @implemented
 *
 */
INT
STDCALL
OffsetRgn( HRGN hrgn,
          int nXOffset,
          int nYOffset)
{
    /* FIXME some part are done in user mode */
    return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);
}


INT
STDCALL
GetTextCharsetInfo(HDC hdc,
                   LPFONTSIGNATURE lpSig,
                   DWORD dwFlags)
{
    /* FIXME some part are done in user mode */
    return NtGdiGetTextCharsetInfo(hdc, lpSig, dwFlags); 
}



INT 
STDCALL
IntersectClipRect(HDC hdc,
                  int nLeftRect,
                  int nTopRect,
                  int nRightRect,
                  int nBottomRect)
{
    /* FIXME some part are done in user mode */
    return NtGdiIntersectClipRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
}

INT 
STDCALL
OffsetClipRgn(HDC hdc,
              int nXOffset,
              int nYOffset)
{
    /* FIXME some part are done in user mode */
    return NtGdiOffsetClipRgn( hdc,  nXOffset,  nYOffset);
}


INT
STDCALL
NamedEscape(HDC hdc,
            PWCHAR pDriver,
            INT iEsc, 
            INT cjIn,
            LPSTR pjIn,
            INT cjOut,
            LPSTR pjOut)
{
    /* FIXME metadc, metadc are done most in user mode, and we do not support it 
     * Windows 2000/XP/Vista ignore the current hdc, that are being pass and always set hdc to NULL 
     * when it calls to NtGdiExtEscape from NamedEscape
     */
    return NtGdiExtEscape(NULL,pDriver,wcslen(pDriver),iEsc,cjIn,pjIn,cjOut,pjOut);
}


BOOL
STDCALL
PatBlt(HDC hdc, 
       int nXLeft, 
       int nYLeft, 
       int nWidth, 
       int nHeight, 
       DWORD dwRop)
{
    /* FIXME some part need be done in user mode */
    return PatBlt( hdc,  nXLeft,  nYLeft,  nWidth,  nHeight,  dwRop);
}

BOOL
STDCALL
PolyPatBlt(IN HDC hdc,
           IN DWORD rop4,
           IN PPOLYPATBLT pPoly,
           IN DWORD Count,
           IN DWORD Mode)
{
    /* FIXME some part need be done in user mode */
    return NtGdiPolyPatBlt(hdc, rop4, pPoly,Count,Mode);
}









