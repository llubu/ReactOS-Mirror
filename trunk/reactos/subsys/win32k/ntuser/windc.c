/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: windc.c,v 1.26 2003/10/03 18:04:37 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/region.h>
#include <win32k/userobj.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/msgqueue.h>
#include <include/window.h>
#include <include/rect.h>
#include <include/dce.h>
#include <include/vis.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static PDCE FirstDce = NULL;
static HDC defaultDCstate;

#define DCX_CACHECOMPAREMASK (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | \
                              DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)

/* FUNCTIONS *****************************************************************/

HRGN STDCALL
DceGetVisRgn(HWND hWnd, ULONG Flags, HWND hWndChild, ULONG CFlags)
{
  PWINDOW_OBJECT Window;
  PWINDOW_OBJECT Child;
  HRGN VisRgn;
  HRGN VisChild;
  HRGN ChildRect;
  HRGN ParentRect;

  Window = IntGetWindowObject(hWnd);

  if (NULL == Window)
    {
      return NULL;
    }

  VisRgn = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop, Window,
                                    0 == (Flags & DCX_WINDOW),
                                    0 != (Flags & DCX_CLIPCHILDREN),
                                    0 != (Flags & DCX_CLIPSIBLINGS));
  if (NULL != hWndChild && 0 != (CFlags & DCX_CLIPCHILDREN))
    {
      /* We need to filter out the child windows of hWndChild */
      Child = IntGetWindowObject(hWnd);
      if (NULL != Child)
 	{
    if (Child->FirstChild)
	    {
	      /* Compute the visible region of the child */
	      VisChild = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop,
	                                          Child, FALSE, TRUE, FALSE);
	      /* If the child doesn't obscure the whole window, we need to
                 extend it. First compute the difference between window and child */
	      ChildRect = UnsafeIntCreateRectRgnIndirect(&(Child->ClientRect));
	      if (0 == (Flags & DCX_WINDOW))
		{
		  ParentRect = UnsafeIntCreateRectRgnIndirect(&(Window->ClientRect));
		}
	      else
		{
		  ParentRect = UnsafeIntCreateRectRgnIndirect(&(Window->WindowRect));
		}
	      NtGdiCombineRgn(ChildRect, ParentRect, ChildRect, RGN_DIFF);

	      /* Now actually extend the child by adding the difference */
	      NtGdiCombineRgn(VisChild, VisChild, ChildRect, RGN_OR);

	      /* Clip the childs children */
	      NtGdiCombineRgn(VisRgn, VisRgn, VisChild, RGN_AND);
	    }
	  IntReleaseWindowObject(Child);
	}
    }

  IntReleaseWindowObject(Window);

  return VisRgn;
}

HDC STDCALL
NtUserGetDC(HWND hWnd)
{
    return NtUserGetDCEx(hWnd, NULL, NULL == hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE);
}

PDCE FASTCALL
DceAllocDCE(HWND hWnd, DCE_TYPE Type)
{
  HDCE DceHandle;
  DCE* Dce;

  DceHandle = DCEOBJ_AllocDCE();
  if(!DceHandle)
    return NULL;
  
  Dce = DCEOBJ_LockDCE(DceHandle);
  Dce->hDC = NtGdiCreateDC(L"DISPLAY", NULL, NULL, NULL);
  if (NULL == defaultDCstate)
    {
      defaultDCstate = NtGdiGetDCState(Dce->hDC);
    }
  Dce->hwndCurrent = hWnd;
  Dce->hClipRgn = NULL;
  Dce->next = FirstDce;
  FirstDce = Dce;

  if (Type != DCE_CACHE_DC)
    {
      Dce->DCXFlags = DCX_DCEBUSY;
      if (hWnd != NULL)
	{
	  PWINDOW_OBJECT WindowObject;

	  WindowObject = IntGetWindowObject(hWnd);
	  if (WindowObject->Style & WS_CLIPCHILDREN)
	    {
	      Dce->DCXFlags |= DCX_CLIPCHILDREN;
	    }
	  if (WindowObject->Style & WS_CLIPSIBLINGS)
	    {
	      Dce->DCXFlags |= DCX_CLIPSIBLINGS;
	    }
	  IntReleaseWindowObject(WindowObject);
	}
    }
  else
    {
      Dce->DCXFlags = DCX_CACHE | DCX_DCEEMPTY;
    }

  return(Dce);
}

VOID STATIC STDCALL
DceSetDrawable(PWINDOW_OBJECT WindowObject, HDC hDC, ULONG Flags,
	       BOOL SetClipOrigin)
{
  DC *dc = DC_LockDc(hDC);
  if(!dc)
    return;
  
  if (WindowObject == NULL)
    {
      dc->w.DCOrgX = 0;
      dc->w.DCOrgY = 0;
    }
  else
    {
      if (Flags & DCX_WINDOW)
	{
	  dc->w.DCOrgX = WindowObject->WindowRect.left;
	  dc->w.DCOrgY = WindowObject->WindowRect.top;
	}
      else
	{
	  dc->w.DCOrgX = WindowObject->ClientRect.left;
	  dc->w.DCOrgY = WindowObject->ClientRect.top;
	}
    }
  DC_UnlockDc(hDC);
}


STATIC VOID FASTCALL
DceDeleteClipRgn(DCE* Dce)
{
  Dce->DCXFlags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN | DCX_WINDOWPAINT);

  if (Dce->DCXFlags & DCX_KEEPCLIPRGN )
    {
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
    }
  else if (Dce->hClipRgn > (HRGN) 1)
    {
      NtGdiDeleteObject(Dce->hClipRgn);
    }

  Dce->hClipRgn = NULL;

  /* make it dirty so that the vis rgn gets recomputed next time */
  Dce->DCXFlags |= DCX_DCEDIRTY;
}

STATIC INT FASTCALL
DceReleaseDC(DCE* dce)
{
  if (DCX_DCEBUSY != (dce->DCXFlags & (DCX_DCEEMPTY | DCX_DCEBUSY)))
    {
      return 0;
    }

  /* restore previous visible region */

  if ((dce->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
      (dce->DCXFlags & (DCX_CACHE | DCX_WINDOWPAINT)) )
    {
      DceDeleteClipRgn( dce );
    }

  if (dce->DCXFlags & DCX_CACHE)
    {
      /* make the DC clean so that SetDCState doesn't try to update the vis rgn */
      NtGdiSetHookFlags(dce->hDC, DCHF_VALIDATEVISRGN);
      NtGdiSetDCState(dce->hDC, defaultDCstate);
      dce->DCXFlags &= ~DCX_DCEBUSY;
      if (dce->DCXFlags & DCX_DCEDIRTY)
	{
	  /* don't keep around invalidated entries
	   * because SetDCState() disables hVisRgn updates
	   * by removing dirty bit. */
	  dce->hwndCurrent = 0;
	  dce->DCXFlags &= DCX_CACHE;
	  dce->DCXFlags |= DCX_DCEEMPTY;
	}
    }

  return 1;
}

HDC STDCALL
NtUserGetDCEx(HWND hWnd, HANDLE ClipRegion, ULONG Flags)
{
  PWINDOW_OBJECT Window;
  ULONG DcxFlags;
  DCE* Dce;
  BOOL UpdateVisRgn = TRUE;
  BOOL UpdateClipOrigin = FALSE;
  HANDLE hRgnVisible = NULL;

  if (NULL == hWnd)
    {
      Flags &= ~DCX_USESTYLE;
      Window = NULL;
    }
  else if (NULL == (Window = IntGetWindowObject(hWnd)))
    {
      return(0);
    }

  if (NULL == Window || NULL == Window->Dce)
    {
      Flags |= DCX_CACHE;
    }


  if (Flags & DCX_USESTYLE)
    {
      Flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

      if (Window->Style & WS_CLIPSIBLINGS)
	{
	  Flags |= DCX_CLIPSIBLINGS;
	}

      if (!(Flags & DCX_WINDOW))
	{
	  if (Window->Class->style & CS_PARENTDC)
	    {
	      Flags |= DCX_PARENTCLIP;
	    }

	  if (Window->Style & WS_CLIPCHILDREN &&
	      !(Window->Style & WS_MINIMIZE))
	    {
	      Flags |= DCX_CLIPCHILDREN;
	    }
	}
      else
	{
	  Flags |= DCX_CACHE;
	}
    }

  if (Flags & DCX_NOCLIPCHILDREN)
    {
      Flags |= DCX_CACHE;
      Flags |= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
    }

  if (Flags & DCX_WINDOW)
    {
      Flags = (Flags & ~DCX_CLIPCHILDREN) | DCX_CACHE;
    }

  if (NULL == Window || !(Window->Style & WS_CHILD) || NULL == Window->Parent)
    {
      Flags &= ~DCX_PARENTCLIP;
    }
  else if (Flags & DCX_PARENTCLIP)
    {
      Flags |= DCX_CACHE;
      if (!(Flags & (DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS)))
	{
	  if ((Window->Style & WS_VISIBLE) && 
	      (Window->Parent->Style & WS_VISIBLE))
	    {
	      Flags &= ~DCX_CLIPCHILDREN;
	      if (Window->Parent->Style & WS_CLIPSIBLINGS)
		{
		  Flags |= DCX_CLIPSIBLINGS;
		}
	    }
	}
    }

  DcxFlags = Flags & DCX_CACHECOMPAREMASK;

  if (Flags & DCX_CACHE)
    {
      DCE* DceEmpty = NULL;
      DCE* DceUnused = NULL;

      for (Dce = FirstDce; Dce != NULL; Dce = Dce->next)
	{
	  if ((Dce->DCXFlags & (DCX_CACHE | DCX_DCEBUSY)) == DCX_CACHE)
	    {
	      DceUnused = Dce;
	      if (Dce->DCXFlags & DCX_DCEEMPTY)
		{
		  DceEmpty = Dce;
		}
	      else if (Dce->hwndCurrent == hWnd &&
		       ((Dce->DCXFlags & DCX_CACHECOMPAREMASK) == DcxFlags))
		{
#if 0 /* FIXME */
		  UpdateVisRgn = FALSE;
#endif
		  UpdateClipOrigin = TRUE;
		  break;
		}
	    }
	}

      if (Dce == NULL)
	{
	  Dce = (DceEmpty == NULL) ? DceEmpty : DceUnused;
	}

      if (Dce == NULL)
	{
	  Dce = DceAllocDCE(NULL, DCE_CACHE_DC);
	}
    }
  else
    {
      Dce = Window->Dce;
      /* FIXME: Implement this. */
      DbgBreakPoint();
    }

  if (NULL == Dce)
    {
      if(NULL != Window)
        IntReleaseWindowObject(Window);
      return(NULL);
    }

  Dce->hwndCurrent = hWnd;
  Dce->hClipRgn = NULL;
  Dce->DCXFlags = DcxFlags | (Flags & DCX_WINDOWPAINT) | DCX_DCEBUSY;

  if (0 == (Flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) && NULL != ClipRegion)
    {
      NtGdiDeleteObject(ClipRegion);
      ClipRegion = NULL;
    }

  if (NULL != Dce->hClipRgn)
    {
      DceDeleteClipRgn(Dce);
    }

  if (NULL != ClipRegion)
    {
      Dce->hClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      if(Dce->hClipRgn)
        NtGdiCombineRgn(Dce->hClipRgn, ClipRegion, NULL, RGN_COPY);
      NtGdiDeleteObject(ClipRegion);
    }

  DceSetDrawable(Window, Dce->hDC, Flags, UpdateClipOrigin);

  if (UpdateVisRgn)
    {
      if (Flags & DCX_PARENTCLIP)
	{
	  PWINDOW_OBJECT Parent;

	  Parent = Window->Parent;

	  if (Window->Style & WS_VISIBLE &&
	      !(Parent->Style & WS_MINIMIZE))
	    {
	      if (Parent->Style & WS_CLIPSIBLINGS)
		{
		  DcxFlags = DCX_CLIPSIBLINGS | 
		    (Flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
		}
	      else
		{
		  DcxFlags = Flags & 
		    ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);
		}
	      hRgnVisible = DceGetVisRgn(Parent->Self, DcxFlags, 
					 Window->Self, Flags);
	    }
	  else
	    {
	      hRgnVisible = NtGdiCreateRectRgn(0, 0, 0, 0);
	    }
	}
      else
	{
	  if (hWnd == IntGetDesktopWindow())
	    {
	      hRgnVisible = 
		NtGdiCreateRectRgn(0, 0, 
				  NtUserGetSystemMetrics(SM_CXSCREEN),
				  NtUserGetSystemMetrics(SM_CYSCREEN));
	    }
	  else
	    {
	      hRgnVisible = DceGetVisRgn(hWnd, Flags, 0, 0);
	    }
	}

      if (0 != (Flags & DCX_INTERSECTRGN))
	{
	  NtGdiCombineRgn(hRgnVisible, hRgnVisible, Dce->hClipRgn, RGN_AND);
	}

      if (0 != (Flags & DCX_EXCLUDERGN))
	{
	  NtGdiCombineRgn(hRgnVisible, hRgnVisible, Dce->hClipRgn, RGN_DIFF);
	}

      Dce->DCXFlags &= ~DCX_DCEDIRTY;
      NtGdiSelectVisRgn(Dce->hDC, hRgnVisible);
    }

  if (hRgnVisible != NULL)
    {
      NtGdiDeleteObject(hRgnVisible);
    }
  if (NULL != Window)
    {
      IntReleaseWindowObject(Window);
    }

  return(Dce->hDC);
}

BOOL FASTCALL
DCE_InternalDelete(PDCE Dce)
{
  PDCE PrevInList;

  if (Dce == FirstDce)
    {
      FirstDce = Dce->next;
      PrevInList = Dce;
    }
  else
    {
      for (PrevInList = FirstDce; NULL != PrevInList; PrevInList = PrevInList->next)
	{
	  if (Dce == PrevInList->next)
	    {
	      PrevInList->next = Dce->next;
	      break;
	    }
	}
      assert(NULL != PrevInList);
    }

  return NULL != PrevInList;
}

HWND FASTCALL
IntWindowFromDC(HDC hDc)
{
  DCE *Dce;
  for (Dce = FirstDce; Dce != NULL; Dce = Dce->next)
  {
    if(Dce->hDC == hDc)
    {
      return Dce->hwndCurrent;
    }
  }
  return 0;
}

INT STDCALL
NtUserReleaseDC(HWND hWnd, HDC hDc)
{
  DCE *dce;
  INT nRet = 0;

  /* FIXME USER_Lock(); */
  dce = FirstDce;

  DPRINT("%p %p\n", hWnd, hDc);

  while (dce && (dce->hDC != hDc))
    {
      dce = dce->next;
    }

  if (dce && (dce->DCXFlags & DCX_DCEBUSY))
    {
      nRet = DceReleaseDC(dce);
    }

  /* FIXME USER_Unlock(); */

  return nRet;
}
  

/* EOF */
