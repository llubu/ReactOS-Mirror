/* $Id: windc.c,v 1.2 2002/07/17 21:04:57 dwelch Exp $
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
#include <win32k/userobj.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/msgqueue.h>
#include <include/window.h>
#include <include/rect.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL STATIC
DceGetVisRect(PWINDOW_OBJECT Window, BOOL ClientArea, RECT* Rect)
{
  if (ClientArea)
    {
      *Rect = Window->ClientRect;
    }
  else
    {
      *Rect = Window->WindowRect;
    }

  if (Window->Style & WS_VISIBLE)
    {
      INT XOffset = Rect->left;
      INT YOffset = Rect->top;

      while ((Window = Window->Parent) != NULL)
	{
	  if ((Window->Style & (WS_ICONIC | WS_VISIBLE)) != WS_VISIBLE)
	    {
	      W32kSetEmptyRect(Rect);
	      return(FALSE);
	    }
	  XOffset += Window->ClientRect.left;
	  YOffset += Window->ClientRect.top;
	  W32kOffsetRect(Rect, Window->ClientRect.left, 
			 Window->ClientRect.top);
	  if (Window->ClientRect.left >= Window->ClientRect.right ||
	      Window->ClientRect.top >= Window->ClientRect.bottom ||
	      Rect->left >= Window->ClientRect.right ||
	      Rect->right <= Window->ClientRect.left ||
	      Rect->top >= Window->ClientRect.bottom ||
	      Rect->bottom <= Window->ClientRect.top)
	    {
	      W32kSetEmptyRect(Rect);
	      return(FALSE);
	    }
	  Rect->left = max(Rect->left, Window->ClientRect.left);
	  Rect->right = min(Rect->right, Window->ClientRect.right);
	  Rect->top = max(Rect->top, Window->ClientRect.top);
	  Rect->bottom = min(Rect->bottom, Window->ClientRect.bottom);
	}
      W32kOffsetRect(Rect, -XOffset, -YOffset);
      return(TRUE);
    }
  W32kSetEmptyRect(Rect);
  return(FALSE);
}

BOOL
DceAddClipRects(PWINDOW_OBJECT Parent, PWINDOW_OBJECT End, 
		HRGN ClipRgn, PRECT Rect, INT XOffset, INT YOffset)
{
  PLIST_ENTRY ChildListEntry;
  PWINDOW_OBJECT Child;
  RECT Rect1;

  ChildListEntry = Parent->ChildrenListHead.Flink;
  while (ChildListEntry != &Parent->ChildrenListHead)
    {
      Child = CONTAINING_RECORD(ChildListEntry, WINDOW_OBJECT, 
				SiblingListEntry);
      if (Child == End)
	{
	  return(TRUE);
	}
      if (Child->Style & WS_VISIBLE)
	{
	  Rect1.left = Child->WindowRect.left + XOffset;
	  Rect1.top = Child->WindowRect.top + YOffset;
	  Rect1.right = Child->WindowRect.right + XOffset;
	  Rect1.bottom = Child->WindowRect.bottom + YOffset;

	  if (W32kIntersectRect(&Rect1, &Rect1, Rect))
	    {
	      W32kUnionRectWithRgn(ClipRgn, &Rect1);
	    }
	}
      ChildListEntry = ChildListEntry->Flink;
    }
  return(FALSE);
}

HRGN 
DceGetVisRgn(HWND hWnd, ULONG Flags, HWND hWndChild, ULONG CFlags)
{
  PWINDOW_OBJECT Window;
  PWINDOW_OBJECT Child;
  HRGN VisRgn;
  RECT Rect;

  Window = W32kGetWindowObject(hWnd);
  Child = W32kGetWindowObject(hWndChild);

  if (Window != NULL && DceGetVisRect(Window, !(Flags & DCX_WINDOW), &Rect))
    {
      if ((VisRgn = W32kCreateRectRgnIndirect(&Rect)) != NULL)
	{
	  HRGN ClipRgn = W32kCreateRectRgn(0, 0, 0, 0);
	  INT XOffset, YOffset;

	  if (ClipRgn != NULL)
	    {
	      if (Flags & DCX_CLIPCHILDREN && 
		  !IsListEmpty(&Window->ChildrenListHead))
		{
		  if (Flags & DCX_WINDOW)
		    {
		      XOffset = Window->ClientRect.left -
			Window->WindowRect.left;
		      YOffset = Window->ClientRect.top -
			Window->WindowRect.top;
		    }		
		  else
		    {
		      XOffset = YOffset = 0;
		    }
		  DceAddClipRects(Window, NULL, ClipRgn, &Rect,
				  XOffset, YOffset);
		}

	      if (CFlags & DCX_CLIPCHILDREN && Child &&
		  !IsListEmpty(&Child->ChildrenListHead))
		{
		  if (Flags & DCX_WINDOW)
		    {
		      XOffset = Window->ClientRect.left -
			Window->WindowRect.left;
		      YOffset = Window->ClientRect.top -
			Window->WindowRect.top;
		    }
		  else
		    {
		      XOffset = YOffset = 0;
		    }

		  XOffset += Child->ClientRect.left;
		  YOffset += Child->ClientRect.top;

		  DceAddClipRects(Child, NULL, ClipRgn, &Rect,
				  XOffset, YOffset);
		}

	      if (Flags & DCX_WINDOW)
		{
		  XOffset = -Window->WindowRect.left;
		  YOffset = -Window->WindowRect.top;
		}
	      else
		{
		  XOffset = -Window->ClientRect.left;
		  YOffset = -Window->ClientRect.top;
		}

	      if (Flags & DCX_CLIPSIBLINGS && Window->Parent != NULL)
		{
		  DceAddClipRects(Window->Parent, Window, ClipRgn,
				  &Rect, XOffset, YOffset);
		}
	      
	      while (Window->Style & WS_CHILD)
		{
		  Window = Window->Parent;
		  XOffset -= Window->ClientRect.left;
		  YOffset -= Window->ClientRect.top;
		  if (Window->Style & WS_CLIPSIBLINGS && 
		      Window->Parent != NULL)
		    {
		      DceAddClipRects(Window->Parent, Window, ClipRgn,
				      &Rect, XOffset, YOffset);
		    }
		}

	      W32kCombineRgn(VisRgn, VisRgn, ClipRgn, RGN_DIFF);
	      W32kDeleteObject(ClipRgn);
	    }
	  else
	    {
	      W32kDeleteObject(VisRgn);
	      VisRgn = 0;
	    }
	}
    }
  else
    {
      VisRgn = W32kCreateRectRgn(0, 0, 0, 0);
    }
  W32kReleaseWindowObject(Window);
  W32kReleaseWindowObject(Child);
  return(VisRgn);
}

INT STDCALL
NtUserReleaseDC(HWND hWnd, HDC hDc)
{
  
}

HDC STDCALL
NtUserGetDCEx(HWND hWnd, HANDLE hRegion, ULONG Flags)
{
  
}

/* EOF */
