#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>



HRGN
STDCALL
CreatePolyPolygonRgn(
	CONST POINT	*a0,
	CONST INT	*a1,
	int		a2,
	int		a3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HBRUSH
STDCALL
CreatePatternBrush(
	HBITMAP		a0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



HRGN
STDCALL
CreateRectRgn(
	int		a0,
	int		a1,
	int		a2,
	int		a3
	)
{
	return W32kCreateRectRgn(a0,a1,a2,a3);
}



HRGN
STDCALL
CreateRectRgnIndirect(
	CONST RECT	*a0
	)
{
	return W32kCreateRectRgnIndirect((RECT *)a0);
}



HRGN
STDCALL
CreateRoundRectRgn(
	int		a0,
	int		a1,
	int		a2,
	int		a3,
	int		a4,
	int		a5
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

BOOL
STDCALL
EqualRgn(
	HRGN		a0,
	HRGN		a1
	)
{
	return W32kEqualRgn(a0,a1);
}

int
STDCALL
OffsetRgn(
	HRGN	a0,
	int	a1,
	int	a2
	)
{
	return W32kOffsetRgn(a0,a1,a2);
}

int
STDCALL
GetRgnBox(
	HRGN	a0,
	LPRECT	a1
	)
{
	return W32kGetRgnBox(a0,a1);
}

BOOL
STDCALL
SetRectRgn(
	HRGN	a0,
	int	a1,
	int	a2,
	int	a3,
	int	a4
	)
{
	return W32kSetRectRgn(a0,a1,a2,a3,a4);
}

int
STDCALL
CombineRgn(
	HRGN	a0,
	HRGN	a1,
	HRGN	a2,
	int	a3
	)
{
	return W32kCombineRgn(a0,a1,a2,a3);
}

DWORD
STDCALL
GetRegionData(
	HRGN		a0,
	DWORD		a1,
	LPRGNDATA	a2
	)
{
	return W32kGetRegionData(a0,a1,a2);
}

BOOL
STDCALL
PaintRgn(
	HDC	a0,
	HRGN	a1
	)
{
	return W32kPaintRgn( a0, a1 );
}


