/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004 ReactOS Team
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
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Monitor support
 *  FILE:             subsys/win32k/ntuser/monitor.c
 *  PROGRAMER:        Anich Gregor (blight@blight.eu.org)
 *  REVISION HISTORY:
 *       26-02-2004  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

/* FIXME: find include file for these */
#define MONITORINFOF_PRIMARY      1
#define MONITOR_DEFAULTTONULL     0
#define MONITOR_DEFAULTTOPRIMARY  1
#define MONITOR_DEFAULTTONEAREST  2

//#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* list of monitors */
static PMONITOR_OBJECT MonitorList = NULL;
static FAST_MUTEX MonitorListLock; /* R/W lock for monitor list */

/* INITALIZATION FUNCTIONS ****************************************************/

NTSTATUS
InitMonitorImpl()
{
  DPRINT("Initializing monitor implementation...\n");
  ExInitializeFastMutex(&MonitorListLock);

  return STATUS_SUCCESS;
}

NTSTATUS
CleanupMonitorImpl()
{
  DPRINT("Cleaning up monitor implementation...\n");
  /* FIXME: Destroy monitor objects? */

  return STATUS_SUCCESS;
}

/* PRIVATE FUNCTIONS **********************************************************/

#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef ABS
# define ABS(a) ((a) < (0) ? (-(a)) : (a))
#endif

/* IntCreateMonitorObject
 *
 * Creates a MONITOR_OBJECT
 *
 * Return value
 *   If the function succeeds a pointer to a MONITOR_OBJECT is returned. On failure
 *   NULL is returned.
 */
static
PMONITOR_OBJECT
IntCreateMonitorObject()
{
  HANDLE Handle;
  PMONITOR_OBJECT Monitor;

  Monitor = ObmCreateObject(PsGetWin32Thread()->Desktop->WindowStation->HandleTable, &Handle, otMonitor, sizeof (MONITOR_OBJECT));
  if (Monitor == NULL)
    {
      return NULL;
    }

  Monitor->Handle = Handle;
  ExInitializeFastMutex(&Monitor->Lock);

  return Monitor;
}

/* IntDestroyMonitorObject
 *
 * Destroys a MONITOR_OBJECT
 * You have to be the owner of the monitors lock to safely destroy it.
 *
 * Arguments
 *
 *   pMonitor
 *      Pointer to the MONITOR_OBJECT which shall be deleted
 */
static
void
IntDestroyMonitorObject(IN PMONITOR_OBJECT pMonitor)
{
  RtlFreeUnicodeString(&pMonitor->DeviceName);
  ObmDereferenceObject(pMonitor);
}

/* IntGetMonitorObject
 *
 * Returns the MONITOR_OBJECT for the given monitor handle.
 *
 * Arguments
 *
 *   hMonitor
 *     Monitor handle for which to get the MONITOR_OBJECT
 *
 * Return value
 *   Returns a pointer to a MONITOR_OBJECT on success and increase the
 *   refcount of the monitor object; NULL on failure
 */
static
PMONITOR_OBJECT
IntGetMonitorObject(IN HMONITOR hMonitor)
{
  PMONITOR_OBJECT Monitor;
  NTSTATUS Status;

  Status = ObmReferenceObjectByHandle(PsGetWin32Thread()->Desktop->WindowStation->HandleTable, hMonitor, otMonitor, (PVOID *)&Monitor);
  if (!NT_SUCCESS(Status) || Monitor == NULL)
    {
      /* FIXME: SetLastNtError( status ); ? */
      return NULL;
    }

  return Monitor;
}

/* IntReleaseMonitorObject
 *
 * Releases the given MONITOR_OBJECT.
 *
 * Arguments
 *
 *   pMonitor
 *     MONITOR_OBJECT to be released
 */
#define IntReleaseMonitorObject(MonitorObj) \
			ObmDereferenceObject(MonitorObj);

/* IntAttachMonitor
 *
 * Creates a new MONITOR_OBJECT and appends it to the list of monitors.
 *
 * Arguments
 *
 *   pGdiDevice     Pointer to the GDIDEVICE onto which the monitor was attached
 *   DisplayNumber  Display Number (starting with 0)
 *
 * Return value
 *   Returns a NTSTATUS
 */
NTSTATUS
IntAttachMonitor(IN GDIDEVICE *pGdiDevice,
                 IN ULONG DisplayNumber)
{
  PMONITOR_OBJECT Monitor;
  WCHAR Buffer[CCHDEVICENAME];

  DPRINT("Attaching monitor...\n");

  /* create new monitor object */
  Monitor = IntCreateMonitorObject();
  if (Monitor == NULL)
    {
      DPRINT("Couldnt create monitor object\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  _snwprintf(Buffer, CCHDEVICENAME, L"\\??\\DISPLAY%d", DisplayNumber + 1);
  if (!RtlCreateUnicodeString(&Monitor->DeviceName, Buffer))
    {
      DPRINT("Couldn't duplicate monitor name!\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Monitor->GdiDevice = pGdiDevice;
  ExAcquireFastMutex(&MonitorListLock);
  if (MonitorList == NULL)
    {
      DPRINT("Primary monitor is beeing attached\n");
      Monitor->IsPrimary = TRUE;
      MonitorList = Monitor;
    }
  else
    {
      PMONITOR_OBJECT p;
      DPRINT("Additional monitor is beeing attached\n");
      for (p = MonitorList; p->Next != NULL; p = p->Next);
        {
          p->Next = Monitor;
        }
      Monitor->Prev = p;
    }
  ExReleaseFastMutex(&MonitorListLock);

  return STATUS_SUCCESS;
}

/* IntDetachMonitor
 *
 * Deletes a MONITOR_OBJECT and removes it from the list of monitors.
 *
 * Arguments
 *
 *   pGdiDevice  Pointer to the GDIDEVICE from which the monitor was detached
 *
 * Return value
 *   Returns a NTSTATUS
 */
NTSTATUS
IntDetachMonitor(IN GDIDEVICE *pGdiDevice)
{
  PMONITOR_OBJECT Monitor;

  ExAcquireFastMutex(&MonitorListLock);
  for (Monitor = MonitorList; Monitor != NULL; Monitor = Monitor->Next)
    {
      if (Monitor->GdiDevice == pGdiDevice)
        break;
    }

  if (Monitor == NULL)
    {
      /* no monitor for given device found */
      ExReleaseFastMutex(&MonitorListLock);
      return STATUS_INVALID_PARAMETER;
    }

  if (Monitor->IsPrimary && (Monitor->Next != NULL || Monitor->Prev != NULL))
    {
      PMONITOR_OBJECT NewPrimaryMonitor = (Monitor->Prev != NULL) ? (Monitor->Prev) : (Monitor->Next);

      ExAcquireFastMutex(&NewPrimaryMonitor->Lock);
      NewPrimaryMonitor->IsPrimary = TRUE;
      ExReleaseFastMutex(&NewPrimaryMonitor->Lock);
    }

  if (MonitorList == Monitor)
    {
      MonitorList = Monitor->Next;
      if (Monitor->Next != NULL)
        Monitor->Next->Prev = NULL;
    }
  else
    {
      Monitor->Prev->Next = Monitor->Next;
      if (Monitor->Next != NULL)
        Monitor->Next->Prev = Monitor->Prev;
    }
  ExReleaseFastMutex(&MonitorListLock);

  IntDestroyMonitorObject(Monitor);

  return STATUS_SUCCESS;
}

/* IntGetPrimaryMonitor
 *
 * Returns a PMONITOR_OBJECT for the primary monitor
 *
 * Return value
 *   PMONITOR_OBJECT
 */
static
PMONITOR_OBJECT
IntGetPrimaryMonitor()
{
  PMONITOR_OBJECT Monitor;

  ExAcquireFastMutex(&MonitorListLock);
  for (Monitor = MonitorList; Monitor != NULL; Monitor = Monitor->Next)
    {
      /* FIXME: I guess locking the monitor is not neccessary to read 1 int */
      if (Monitor->IsPrimary)
        break;
    }
  ExReleaseFastMutex(&MonitorListLock);

  return Monitor;
}

/* IntGetMonitorsFromRect
 *
 * Returns a list of monitor handles/rectangles. The rectangles in the list are
 * the areas of intersection with the monitors.
 *
 * Arguments
 *
 *   pRect
 *      Rectangle in desktop coordinates. If this is NULL all monitors are
 *      returned and the rect list is filled with the sizes of the monitors.
 *
 *   hMonitorList
 *      Pointer to an array of HMONITOR which is filled with monitor handles.
 *      Can be NULL
 *
 *   monitorRectList
 *      Pointer to an array of RECT which is filled with intersection rects in
 *      desktop coordinates.
 *      Can be NULL, will be ignored if no intersecting monitor is found and
 *      flags is MONITOR_DEFAULTTONEAREST
 *
 *   listSize
 *      Size of the hMonitorList and monitorRectList arguments. If this is zero
 *      hMonitorList and monitorRectList are ignored.
 *
 *   flags
 *      Either 0 or MONITOR_DEFAULTTONEAREST (ignored if rect is NULL)
 *
 * Returns
 *   The number of monitors which intersect the specified region.
 */
static
UINT
IntGetMonitorsFromRect(OPTIONAL IN LPCRECT pRect,
                       OPTIONAL OUT HMONITOR *hMonitorList,
                       OPTIONAL OUT LPRECT monitorRectList,
                       OPTIONAL IN DWORD listSize,
                       OPTIONAL IN DWORD flags)
{
  PMONITOR_OBJECT Monitor, NearestMonitor = NULL;
  UINT iCount = 0;
  LONG iNearestDistanceX = 0x7fffffff, iNearestDistanceY = 0x7fffffff;

  /* find monitors which intersect the rectangle */
  ExAcquireFastMutex(&MonitorListLock);
  for (Monitor = MonitorList; Monitor != NULL; Monitor = Monitor->Next)
    {
      RECT MonitorRect, IntersectionRect;

      ExAcquireFastMutex(&Monitor->Lock);
      MonitorRect.left = 0; /* FIXME: get origin */
      MonitorRect.top = 0; /* FIXME: get origin */
      MonitorRect.right = MonitorRect.left + Monitor->GdiDevice->GDIInfo.ulHorzRes;
      MonitorRect.bottom = MonitorRect.top + Monitor->GdiDevice->GDIInfo.ulVertRes;
      ExReleaseFastMutex(&Monitor->Lock);

      DPRINT("MonitorRect: left = %d, top = %d, right = %d, bottom = %d\n",
             MonitorRect.left, MonitorRect.top, MonitorRect.right, MonitorRect.bottom);

      if (pRect != NULL)
        {
          BOOL intersects = TRUE;

          /* check if the rect intersects the monitor */
          if ((pRect->right < MonitorRect.left) || (pRect->left > MonitorRect.right) ||
              (pRect->bottom < MonitorRect.top) || (pRect->top > MonitorRect.bottom))
            {
              intersects = FALSE;
            }

          if (flags == MONITOR_DEFAULTTONEAREST && !intersects)
            {
              INT distanceX, distanceY;

              distanceX = MIN(ABS(MonitorRect.left - pRect->right),
                              ABS(pRect->left - MonitorRect.right));
              distanceY = MIN(ABS(MonitorRect.top - pRect->bottom),
                              ABS(pRect->top - MonitorRect.bottom));

              if (((distanceX <  iNearestDistanceX) && (distanceY <= iNearestDistanceY)) ||
                  ((distanceX <= iNearestDistanceX) && (distanceY <  iNearestDistanceY)))
                {
                  iNearestDistanceX = distanceX;
                  iNearestDistanceY = distanceY;
                  NearestMonitor = Monitor;
                }
            }

          if (!intersects)
            continue;

          /* calculate intersection */
          IntersectionRect.left   = MAX(MonitorRect.left,   pRect->left);
          IntersectionRect.top    = MAX(MonitorRect.top,    pRect->top);
          IntersectionRect.right  = MIN(MonitorRect.right,  pRect->right);
          IntersectionRect.bottom = MIN(MonitorRect.bottom, pRect->bottom);
        }
      else
        {
          IntersectionRect = MonitorRect;
        }

      if (iCount < listSize)
        {
          if (hMonitorList != NULL)
            hMonitorList[iCount] = Monitor->Handle;
          if (monitorRectList != NULL)
            monitorRectList[iCount] = IntersectionRect;
        }
      iCount++;
    }
  ExReleaseFastMutex(&MonitorListLock);

  if (iCount == 0 && flags == MONITOR_DEFAULTTONEAREST)
    {
      if (iCount < listSize)
        {
          if (hMonitorList != NULL)
            hMonitorList[iCount] = NearestMonitor->Handle;
        }
      iCount++;
    }

  return iCount;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/* NtUserEnumDisplayMonitors
 *
 * Enumerates display monitors which intersect the given HDC/cliprect
 *
 * Arguments
 *
 *   hDC
 *      Handle to a DC for which to enum intersecting monitors. If this is NULL
 *      it returns all monitors which are part of the current virtual screen.
 *
 *   pRect
 *      Clipping rectangle with coordinate system origin at the DCs origin if the
 *      given HDC is not NULL or in virtual screen coordinated if it is NULL.
 *      Can be NULL
 *
 *   hMonitorList
 *      Pointer to an array of HMONITOR which is filled with monitor handles.
 *      Can be NULL
 *
 *   monitorRectList
 *      Pointer to an array of RECT which is filled with intersection rectangles.
 *      Can be NULL
 *
 *   listSize
 *      Size of the hMonitorList and monitorRectList arguments. If this is zero
 *      hMonitorList and monitorRectList are ignored.
 *
 * Returns
 *   The number of monitors which intersect the specified region or -1 on failure.
 */
INT
STDCALL
NtUserEnumDisplayMonitors(
	OPTIONAL IN HDC hDC,
	OPTIONAL IN LPCRECT pRect,
	OPTIONAL OUT HMONITOR *hMonitorList,
	OPTIONAL OUT LPRECT monitorRectList,
	OPTIONAL IN DWORD listSize)
{
	INT numMonitors, i;
	HMONITOR *safeHMonitorList = NULL;
	LPRECT safeRectList = NULL;
	RECT rect, *myRect;
	RECT dcRect;
	NTSTATUS status;

	/* get rect */
	if (pRect != NULL)
	{
		status = MmCopyFromCaller(&rect, pRect, sizeof (RECT));
		if (!NT_SUCCESS(status))
		{
			DPRINT("MmCopyFromCaller() failed!\n");
			SetLastNtError(status);
			return -1;
		}
	}

	if (hDC != NULL)
	{
		PDC dc;
		HRGN dcVisRgn;
		INT regionType;

		/* get visible region bounding rect */
		dc = DC_LockDc(hDC);
		if (dc == NULL)
		{
			DPRINT("DC_LockDc() failed!\n");
			/* FIXME: setlasterror? */
			return -1;
		}
		dcVisRgn = dc->w.hVisRgn;
		DC_UnlockDc(hDC);

		regionType = NtGdiGetRgnBox(dcVisRgn, &dcRect);
		if (regionType == 0)
		{
			DPRINT("NtGdiGetRgnBox() failed!\n");
			return -1;
		}
		if (regionType == NULLREGION)
			return 0;
		if (regionType == COMPLEXREGION)
		{	/* TODO: warning */ }

		/* if hDC and pRect are given the area of interest is pRect with
		   coordinate origin at the DC position */
		if (pRect != NULL)
		{
			rect.left += dcRect.left;
			rect.right += dcRect.left;
			rect.top += dcRect.top;
			rect.bottom += dcRect.top;
		}
		/* if hDC is given and pRect is not the area of interest is the
		   bounding rect of hDC */
		else
		{
			rect = dcRect;
		}
	}

	if (hDC == NULL && pRect == NULL)
		myRect = NULL;
	else
		myRect = &rect;

	/* find intersecting monitors */
	numMonitors = IntGetMonitorsFromRect(myRect, NULL, NULL, 0, 0);
	if (numMonitors == 0 || listSize == 0 ||
	    (hMonitorList == NULL && monitorRectList == NULL))
	{
		DPRINT("numMonitors = %d\n", numMonitors);
		return numMonitors;
	}

	if (hMonitorList != NULL && listSize != 0)
	{
		safeHMonitorList = ExAllocatePool(PagedPool, sizeof (HMONITOR) * listSize);
		if (safeHMonitorList == NULL)
		{
			/* FIXME: SetLastWin32Error? */
			return -1;
		}
	}
	if (monitorRectList != NULL && listSize != 0)
	{
		safeRectList = ExAllocatePool(PagedPool, sizeof (RECT) * listSize);
		if (safeRectList == NULL)
		{
			ExFreePool(safeHMonitorList);
			/* FIXME: SetLastWin32Error? */
			return -1;
		}
	}

	/* get intersecting monitors */
	numMonitors = IntGetMonitorsFromRect(myRect, safeHMonitorList, safeRectList,
	                                     listSize, 0 );

	if (hDC != NULL && pRect != NULL && safeRectList != NULL)
		for (i = 0; i < numMonitors; i++)
		{
			safeRectList[i].left -= dcRect.left;
			safeRectList[i].right -= dcRect.left;
			safeRectList[i].top -= dcRect.top;
			safeRectList[i].bottom -= dcRect.top;
		}

	/* output result */
	if (hMonitorList != NULL && listSize != 0)
	{
		status = MmCopyToCaller(hMonitorList, safeHMonitorList, sizeof (HMONITOR) * listSize);
		ExFreePool(safeHMonitorList);
		if (!NT_SUCCESS(status))
		{
			ExFreePool(safeRectList);
			SetLastNtError(status);
			return -1;
		}
	}
	if (monitorRectList != NULL && listSize != 0)
	{
		status = MmCopyToCaller(monitorRectList, safeRectList, sizeof (RECT) * listSize);
		ExFreePool(safeRectList);
		if (!NT_SUCCESS(status))
		{
			SetLastNtError(status);
			return -1;
		}
	}

	return numMonitors;
}

/* NtUserGetMonitorInfo
 *
 * Retrieves information about a given monitor
 *
 * Arguments
 *
 *   hMonitor
 *      Handle to a monitor for which to get information
 *
 *   pMonitorInfo
 *      Pointer to a MONITORINFO struct which is filled with the information.
 *      The cbSize member must be set to sizeof(MONITORINFO) or
 *      sizeof(MONITORINFOEX). Even if set to sizeof(MONITORINFOEX) only parts
 *      from MONITORINFO will be filled.
 *
 *   pDevice
 *      Pointer to a UNICODE_STRING which will recieve the device's name. The
 *      length should be CCHDEVICENAME
 *      Can be NULL
 *
 * Return value
 *   TRUE on success; FALSE on failure (calls SetLastNtError())
 *
 */
BOOL
STDCALL
NtUserGetMonitorInfo(
	IN HMONITOR hMonitor,
	OUT LPMONITORINFO pMonitorInfo)
{
	PMONITOR_OBJECT Monitor;
	MONITORINFOEXW MonitorInfo;
	NTSTATUS Status;

	/* get monitor object */
	if ((Monitor = IntGetMonitorObject(hMonitor)) == NULL)
	{
		DPRINT("Couldnt find monitor 0x%lx\n", hMonitor);
		SetLastNtError(STATUS_INVALID_HANDLE);
		return FALSE;
	}

	/* get size of pMonitorInfo */
	Status = MmCopyFromCaller(&MonitorInfo.cbSize, &pMonitorInfo->cbSize, sizeof (MonitorInfo.cbSize));
	if (!NT_SUCCESS(Status))
	{
		IntReleaseMonitorObject(Monitor);
		SetLastNtError(Status);
		return FALSE;
	}
	if ((MonitorInfo.cbSize != sizeof (MONITORINFO)) &&
	    (MonitorInfo.cbSize != sizeof (MONITORINFOEXW)))
	{
		SetLastNtError(STATUS_INVALID_PARAMETER);
		return FALSE;
	}

	ExAcquireFastMutex(&Monitor->Lock);
	/* fill monitor info */
	MonitorInfo.rcMonitor.left = 0; /* FIXME: get origin */
	MonitorInfo.rcMonitor.top = 0; /* FIXME: get origin */
	MonitorInfo.rcMonitor.right = MonitorInfo.rcMonitor.left + Monitor->GdiDevice->GDIInfo.ulHorzRes;
	MonitorInfo.rcMonitor.bottom = MonitorInfo.rcMonitor.top + Monitor->GdiDevice->GDIInfo.ulVertRes;
	MonitorInfo.rcWork = MonitorInfo.rcMonitor; /* FIXME: use DEVMODE panning to calculate work area? */
	MonitorInfo.dwFlags = 0;
	if (Monitor->IsPrimary)
		MonitorInfo.dwFlags |= MONITORINFOF_PRIMARY;

	/* fill device name */
	if (MonitorInfo.cbSize == sizeof (MONITORINFOEXW))
	{
		WCHAR nul = L'\0';
		INT len = Monitor->DeviceName.Length;
		if (len >= CCHDEVICENAME * sizeof (WCHAR))
			len = (CCHDEVICENAME - 1) * sizeof (WCHAR);
		
		memcpy(MonitorInfo.szDevice, Monitor->DeviceName.Buffer, len);
		memcpy(MonitorInfo.szDevice + (len / sizeof (WCHAR)), &nul, sizeof (WCHAR));
	}
	ExReleaseFastMutex(&Monitor->Lock);
	IntReleaseMonitorObject(Monitor);

	/* output data */
	Status = MmCopyToCaller(pMonitorInfo, &MonitorInfo, MonitorInfo.cbSize);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("GetMonitorInfo: MmCopyToCaller failed\n");
		SetLastNtError(Status);
		return FALSE;
	}

	DPRINT("GetMonitorInfo: success\n");

	return TRUE;
}

/* NtUserMonitorFromPoint
 *
 * Returns a handle to the monitor containing the given point.
 *
 * Arguments
 *
 *   point
 *     Point for which to find monitor
 *
 *   dwFlags
 *     Specifies the behaviour if the point isn't on any of the monitors.
 *
 * Return value
 *   If the point is found a handle to the monitor is returned; if not the
 *   return value depends on dwFlags
 */
HMONITOR
STDCALL
NtUserMonitorFromPoint(
	IN POINT point,
	IN DWORD dwFlags)
{
	INT NumMonitors;
	RECT InRect;
	HMONITOR hMonitor = NULL;

	/* fill inRect */
	InRect.left = InRect.right = point.x;
	InRect.top = InRect.bottom = point.y;

	/* find intersecting monitor */
	NumMonitors = IntGetMonitorsFromRect(&InRect, &hMonitor, NULL, 1, 0);
	if (NumMonitors < 0)
	{
		return (HMONITOR)NULL;
	}

	if (hMonitor == NULL)
	{
		if (dwFlags == MONITOR_DEFAULTTOPRIMARY)
		{
			PMONITOR_OBJECT MonitorObj = IntGetPrimaryMonitor();
			if (MonitorObj)
				hMonitor = MonitorObj->Handle;
		}
		else if (dwFlags == MONITOR_DEFAULTTONEAREST)
		{
			NumMonitors = IntGetMonitorsFromRect(&InRect, &hMonitor, NULL,
			                                     1, MONITOR_DEFAULTTONEAREST);
			/*ASSERT( (numMonitors > 0) && (hMonitor != NULL) );*/
		}
		/* else flag is DEFAULTTONULL */
	}

	return hMonitor;
}

/* NtUserMonitorFromRect
 *
 * Returns a handle to the monitor having the largest intersection with a
 * given rectangle
 *
 * Arguments
 *
 *   pRect
 *     Pointer to a RECT for which to find monitor
 *
 *   dwFlags
 *     Specifies the behaviour if no monitor intersects the given rect
 *
 * Return value
 *   If a monitor intersects the rect a handle to it is returned; if not the
 *   return value depends on dwFlags
 */
HMONITOR
STDCALL
NtUserMonitorFromRect(
	IN LPCRECT pRect,
	IN DWORD dwFlags)
{
	INT numMonitors, iLargestArea = -1, i;
	LPRECT rectList;
	HMONITOR *hMonitorList;
	HMONITOR hMonitor = NULL;
	RECT rect;
	NTSTATUS status;

	/* get rect */
	status = MmCopyFromCaller(&rect, pRect, sizeof (RECT));
	if (!NT_SUCCESS(status))
	{
		SetLastNtError(status);
		return (HMONITOR)NULL;
	}

	/* find intersecting monitors */
	numMonitors = IntGetMonitorsFromRect(&rect, NULL, NULL, 0, 0);
	if (numMonitors < 0)
	{
		return (HMONITOR)NULL;
	}

	if (numMonitors == 0)
	{
		if (dwFlags == MONITOR_DEFAULTTOPRIMARY)
		{
			PMONITOR_OBJECT monitorObj = IntGetPrimaryMonitor();
			if (monitorObj)
				return monitorObj->Handle;
		}
		else if (dwFlags == MONITOR_DEFAULTTONEAREST)
		{
			numMonitors = IntGetMonitorsFromRect(&rect, &hMonitor, NULL,
			                                     1, MONITOR_DEFAULTTONEAREST);
			if (numMonitors <= 0)
			{
				/* error? */
				return (HMONITOR)NULL;
			}

			if (numMonitors > 0)
				return hMonitor;
		}
		/* else flag is DEFAULTTONULL */
		return (HMONITOR)NULL;
	}

	hMonitorList = ExAllocatePool(PagedPool, sizeof (HMONITOR) * numMonitors);
	if (hMonitorList == NULL)
	{
		/* FIXME: SetLastWin32Error? */
		return (HMONITOR)NULL;
	}
	rectList = ExAllocatePool(PagedPool, sizeof (RECT) * numMonitors);
	if (rectList == NULL)
	{
		ExFreePool(hMonitorList);
		/* FIXME: SetLastWin32Error? */
		return (HMONITOR)NULL;
	}

	/* get intersecting monitors */
	numMonitors = IntGetMonitorsFromRect(&rect, hMonitorList, rectList,
	                                     numMonitors, 0);
	if (numMonitors <= 0)
	{
		return (HMONITOR)NULL;
	}

	/* find largest intersection */
	for (i = 0; i < numMonitors; i++)
	{
		INT area = (rectList[i].right - rectList[i].left) *
		           (rectList[i].bottom - rectList[i].top);
		if (area > iLargestArea)
		{
			hMonitor = hMonitorList[i];
		}
	}

	ExFreePool(hMonitorList);
	ExFreePool(rectList);

	return hMonitor;
}


HMONITOR
STDCALL
NtUserMonitorFromWindow(
	IN HWND hWnd,
	IN DWORD dwFlags)
{
	PWINDOW_OBJECT Window;
	HMONITOR hMonitor = NULL;
	RECT Rect;
  
        Window = IntGetWindowObject(hWnd);
	if (Window == NULL)
	{
		SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
		return (HMONITOR)NULL;
	}

	Rect.left = Rect.right = Window->WindowRect.left;
	Rect.top = Rect.bottom = Window->WindowRect.bottom;

	IntGetMonitorsFromRect(&Rect, &hMonitor, NULL, 1, dwFlags);

	IntReleaseWindowObject(Window);

	return hMonitor;
}
