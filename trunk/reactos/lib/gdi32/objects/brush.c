#include "precomp.h"


/*
 * @implemented
 */
HBRUSH
STDCALL
CreateSolidBrush(
	COLORREF	crColor
	)
{
  return NtGdiCreateSolidBrush(crColor);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateBrushIndirect(
	CONST LOGBRUSH	*lplb
	)
{
  return NtGdiCreateBrushIndirect(lplb);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateDIBPatternBrushPt(
	CONST VOID		*lpPackedDIB,
	UINT			iUsage
	)
{
  return NtGdiCreateDIBPatternBrushPt(lpPackedDIB, iUsage);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreateHatchBrush(
	int		fnStyle,
	COLORREF	clrref
	)
{
  return NtGdiCreateHatchBrush(fnStyle, clrref);
}

/*
 * @implemented
 */
HBRUSH
STDCALL
CreatePatternBrush(
	HBITMAP		hbmp
	)
{
  return NtGdiCreatePatternBrush ( hbmp );
}
