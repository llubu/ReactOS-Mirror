#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>

/*
 * @implemented
 */
BOOL
STDCALL
Polygon(HDC	hDC,
	CONST POINT	*lpPoints,
	int	nCount)
{
	return NtGdiPolygon(hDC, (CONST PPOINT)lpPoints, nCount);
}


/*
 * @implemented
 */
BOOL
STDCALL
Rectangle(HDC  hDC,
	int  LeftRect,
	int  TopRect,
	int  RightRect,
	int  BottomRect)
{
   return NtGdiRectangle(hDC, LeftRect, TopRect, RightRect, BottomRect);
}

