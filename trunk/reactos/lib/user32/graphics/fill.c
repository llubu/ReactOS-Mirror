#include <windows.h>
#include <user32/win.h>

/***********************************************************************
 *           FillWindow    (USER.324)
 */
void  FillWindow( HWND hwndParent, HWND hwnd, HDC hdc, HBRUSH hbrush )
{
    RECT rect;
    GetClientRect( hwnd, &rect );
   // DPtoLP16( hdc, (LPPOINT16)&rect, 2 );
    PaintRect( hwndParent, hwnd, hdc, hbrush, &rect );
}





/***********************************************************************
 *           PaintRect    (USER.325)
 */
void  PaintRect( HWND hwndParent, HWND hwnd, HDC hdc,
                       HBRUSH hbrush, const RECT *rect)
{
    if( hbrush <= CTLCOLOR_MAX ) 
    {
	if( hwndParent )
	    hbrush = PAINT_GetControlBrush( hwndParent, hwnd, hdc, (UINT)hbrush );
	else 
	    return;
    }
    if( hbrush ) 
	FillRect( hdc, rect, hbrush );
}


/***********************************************************************
 *           GetControlBrush    (USER.326)
 */
HBRUSH STDCALL GetControlBrush( HWND hwnd, HDC hdc, UINT ctlType )
{
    WND* wndPtr = WIN_FindWndPtr( hwnd );

    if((ctlType <= CTLCOLOR_MAX) && wndPtr )
    {
	WND* parent;
	if( wndPtr->dwStyle & WS_POPUP ) parent = wndPtr->owner;
	else parent = wndPtr->parent;
	if( !parent ) parent = wndPtr;
	return (HBRUSH)PAINT_GetControlBrush( parent->hwndSelf, hwnd, hdc, ctlType );
    }
    return (HBRUSH)0;
}

INT STDCALL FrameRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;
    //int left, top, right, bottom;

    if ( hdc == NULL )
	return 0;

    //left   = XLPTODP( dc, rect->left );
    //top    = YLPTODP( dc, rect->top );
    //right  = XLPTODP( dc, rect->right );
    //bottom = YLPTODP( dc, rect->bottom );

    //if ( (right <= left) || (bottom <= top) ) return 0;
    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;
    
    PatBlt( hdc, rect->left, rect->top, 1,
	      rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->right - 1, rect->top, 1,
	      rect->bottom - rect->top, PATCOPY );
    PatBlt( hdc, rect->left, rect->top,
	      rect->right - rect->left, 1, PATCOPY );
    PatBlt( hdc, rect->left, rect->bottom - 1,
	      rect->right - rect->left, 1, PATCOPY );

    SelectObject( hdc, prevBrush );
    return 1;
}


INT STDCALL FillRect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prevBrush;

    if (!(prevBrush = SelectObject( hdc, hbrush ))) return 0;
    PatBlt( hdc, rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject( hdc, prevBrush );
    return 1;
}


WINBOOL STDCALL InvertRect( HDC hDC, CONST RECT *lprc)
{
    return PatBlt( hDC, lprc->left, lprc->top,
              lprc->right - lprc->left, lprc->bottom - lprc->top, DSTINVERT );
}
