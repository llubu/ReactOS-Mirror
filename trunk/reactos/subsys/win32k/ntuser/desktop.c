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
 *
 *  $Id: desktop.c,v 1.21 2004/09/15 20:53:43 mf Exp $
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          Desktops
 *  FILE:             subsys/win32k/ntuser/desktop.c
 *  PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 *  REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/
#include <w32k.h>

#if 0
/* not yet defined in w32api... */
NTSTATUS STDCALL
ObFindHandleForObject(IN PEPROCESS Process,
                      IN PVOID Object,
                      IN POBJECT_TYPE ObjectType,
                      IN POBJECT_HANDLE_INFORMATION HandleInformation,
                      OUT PHANDLE Handle);
#else
#define ObFindHandleForObject(Process, Object, ObjectType, HandleInformation, Handle) \
  (STATUS_UNSUCCESSFUL)
#endif

/* GLOBALS *******************************************************************/

/* Currently active desktop */
PDESKTOP_OBJECT InputDesktop = NULL;
HDESK InputDesktopHandle = NULL; 
HDC ScreenDeviceContext = NULL;

/* INITALIZATION FUNCTIONS ****************************************************/

NTSTATUS FASTCALL
InitDesktopImpl(VOID)
{
  return STATUS_SUCCESS;
}

NTSTATUS FASTCALL
CleanupDesktopImpl(VOID)
{
  return STATUS_SUCCESS;
}

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * IntValidateDesktopHandle
 *
 * Validates the desktop handle.
 *
 * Remarks
 *    If the function succeeds, the handle remains referenced. If the
 *    fucntion fails, last error is set.
 */

NTSTATUS FASTCALL
IntValidateDesktopHandle(
   HDESK Desktop,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PDESKTOP_OBJECT *Object)
{
   NTSTATUS Status;

   Status = ObReferenceObjectByHandle(
      Desktop,
      DesiredAccess,
      ExDesktopObjectType,
      AccessMode,
      (PVOID*)Object,
      NULL);

   if (!NT_SUCCESS(Status))
      SetLastNtError(Status);

   return Status;
}

VOID FASTCALL
IntGetDesktopWorkArea(PDESKTOP_OBJECT Desktop, PRECT Rect)
{
  PRECT Ret;
  
  ASSERT(Desktop);
  
  Ret = &Desktop->WorkArea;
  if((Ret->right == -1) && ScreenDeviceContext)
  {
    PDC dc;
    BITMAPOBJ *BitmapObj;
    dc = DC_LockDc(ScreenDeviceContext);
    BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
    if(BitmapObj)
    {
      Ret->right = BitmapObj->SurfObj.sizlBitmap.cx;
      Ret->bottom = BitmapObj->SurfObj.sizlBitmap.cy;
      BITMAPOBJ_UnlockBitmap(dc->w.hBitmap);
    }
    DC_UnlockDc(ScreenDeviceContext);
  }
  
  if(Rect)
  {
    *Rect = *Ret;
  }
}

PDESKTOP_OBJECT FASTCALL
IntGetActiveDesktop(VOID)
{
  return InputDesktop;
}

/*
 * returns or creates a handle to the desktop object
 */
HDESK FASTCALL
IntGetDesktopObjectHandle(PDESKTOP_OBJECT DesktopObject)
{
  NTSTATUS Status;
  HDESK Ret;
  
  ASSERT(DesktopObject);
  
  Status = ObFindHandleForObject(PsGetCurrentProcess(),
                                 DesktopObject,
                                 ExDesktopObjectType,
                                 NULL,
                                 (PHANDLE)&Ret);
   
  if(!NT_SUCCESS(Status))
  {
    Status = ObOpenObjectByPointer(DesktopObject,
                                   0,
                                   NULL,
                                   0,
                                   ExDesktopObjectType,
                                   UserMode,
                                   (PHANDLE)&Ret);
    if(!NT_SUCCESS(Status))
    {
     /* unable to create a handle */
     DPRINT1("Unable to create a desktop handle\n"); 
     return NULL;
    }
  }
  
  return Ret;
}

PUSER_MESSAGE_QUEUE FASTCALL
IntGetFocusMessageQueue(VOID)
{
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return(NULL);
   }
   return (PUSER_MESSAGE_QUEUE)pdo->ActiveMessageQueue;
}

VOID FASTCALL
IntSetFocusMessageQueue(PUSER_MESSAGE_QUEUE NewQueue)
{
   PUSER_MESSAGE_QUEUE Old;
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return;
   }
   if(NewQueue != NULL)
   {
      if(NewQueue->Desktop != NULL)
      {
        DPRINT("Message Queue already attached to another desktop!\n");
        return;
      }
      IntReferenceMessageQueue(NewQueue);
      InterlockedExchange((LONG*)&NewQueue->Desktop, (LONG)pdo);
   }
   Old = (PUSER_MESSAGE_QUEUE)InterlockedExchange((LONG*)&pdo->ActiveMessageQueue, (LONG)NewQueue);
   if(Old != NULL)
   {
      InterlockedExchange((LONG*)&Old->Desktop, 0);
      IntDereferenceMessageQueue(Old);
   }
}

HWND FASTCALL IntGetDesktopWindow(VOID)
{
   PDESKTOP_OBJECT pdo = IntGetActiveDesktop();
   if (!pdo)
   {
      DPRINT("No active desktop\n");
      return NULL;
   }
  return pdo->DesktopWindow;
}

HWND FASTCALL IntGetCurrentThreadDesktopWindow(VOID)
{
  PDESKTOP_OBJECT pdo = PsGetWin32Thread()->Desktop;
  if (NULL == pdo)
    {
      DPRINT1("Thread doesn't have a desktop\n");
      return NULL;
    }
  return pdo->DesktopWindow;
}

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS FASTCALL
IntShowDesktop(PDESKTOP_OBJECT Desktop, ULONG Width, ULONG Height)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;

  Request.Type = CSRSS_SHOW_DESKTOP;
  Request.Data.ShowDesktopRequest.DesktopWindow = Desktop->DesktopWindow;
  Request.Data.ShowDesktopRequest.Width = Width;
  Request.Data.ShowDesktopRequest.Height = Height;

  return CsrNotify(&Request, &Reply);
}

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP_OBJECT Desktop)
{
#if 0
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;

  Request.Type = CSRSS_HIDE_DESKTOP;
  Request.Data.HideDesktopRequest.DesktopWindow = Desktop->DesktopWindow;

  return NotifyCsrss(&Request, &Reply);
#else
  PWINDOW_OBJECT DesktopWindow;

  DesktopWindow = IntGetWindowObject(Desktop->DesktopWindow);
  if (! DesktopWindow)
    {
      return ERROR_INVALID_WINDOW_HANDLE;
    }
  DesktopWindow->Style &= ~WS_VISIBLE;

  return STATUS_SUCCESS;
#endif
}

/*
 * NtUserCreateDesktop
 *
 * Creates a new desktop.
 *
 * Parameters
 *    lpszDesktopName
 *       Name of the new desktop.
 *
 *    dwFlags
 *       Interaction flags.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 *    lpSecurity
 *       Security descriptor.
 *
 *    hWindowStation
 *       Handle to window station on which to create the desktop.
 *
 * Return Value
 *    If the function succeeds, the return value is a handle to the newly
 *    created desktop. If the specified desktop already exists, the function
 *    succeeds and returns a handle to the existing desktop. When you are
 *    finished using the handle, call the CloseDesktop function to close it.
 *    If the function fails, the return value is NULL.
 *
 * Status
 *    @implemented
 */

HDESK STDCALL
NtUserCreateDesktop(
   PUNICODE_STRING lpszDesktopName,
   DWORD dwFlags,
   ACCESS_MASK dwDesiredAccess,
   LPSECURITY_ATTRIBUTES lpSecurity,
   HWINSTA hWindowStation)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  PWINSTATION_OBJECT WinStaObject;
  PDESKTOP_OBJECT DesktopObject;
  UNICODE_STRING DesktopName;
  NTSTATUS Status;
  HDESK Desktop;
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;

  Status = IntValidateWindowStationHandle(
    hWindowStation,
    KernelMode,
    0,
    &WinStaObject);

  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed validation of window station handle (0x%X)\n", 
        hWindowStation);
      SetLastNtError(Status);
      return NULL;
    }
  
  if (! IntGetFullWindowStationName(&DesktopName, &WinStaObject->Name,
          lpszDesktopName))
    {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      ObDereferenceObject(WinStaObject);
      return NULL;
    }

  ObDereferenceObject(WinStaObject);

  /*
   * Try to open already existing desktop
   */

  DPRINT("Trying to open desktop (%wZ)\n", &DesktopName);

  /* Initialize ObjectAttributes for the desktop object */
  InitializeObjectAttributes(
    &ObjectAttributes,
    &DesktopName,
    0,
    NULL,
    NULL);

  Status = ObOpenObjectByName(
    &ObjectAttributes,
    ExDesktopObjectType,
    NULL,
    UserMode,
    dwDesiredAccess,
    NULL,
    (HANDLE*)&Desktop);

  if (NT_SUCCESS(Status))
    {
      DPRINT("Successfully opened desktop (%wZ)\n", &DesktopName);
      ExFreePool(DesktopName.Buffer);
      return Desktop;
    }

  /*
   * No existing desktop found, try to create new one
   */

  Status = ObCreateObject(
    ExGetPreviousMode(),
    ExDesktopObjectType,
    &ObjectAttributes,
    ExGetPreviousMode(),
    NULL,
    sizeof(DESKTOP_OBJECT),
    0,
    0,
    (PVOID*)&DesktopObject);

  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed creating desktop (%wZ)\n", &DesktopName);
      ExFreePool(DesktopName.Buffer);
      SetLastNtError(STATUS_UNSUCCESSFUL);
      return NULL;
    }
  
  // init desktop area
  DesktopObject->WorkArea.left = 0;
  DesktopObject->WorkArea.top = 0;
  DesktopObject->WorkArea.right = -1;
  DesktopObject->WorkArea.bottom = -1;
  IntGetDesktopWorkArea(DesktopObject, NULL);

  /* Initialize some local (to win32k) desktop state. */
  DesktopObject->ActiveMessageQueue = NULL;

  Status = ObInsertObject(
    (PVOID)DesktopObject,
    NULL,
    STANDARD_RIGHTS_REQUIRED,
    0,
    NULL,
    (HANDLE*)&Desktop);
  
  ObDereferenceObject(DesktopObject);
  ExFreePool(DesktopName.Buffer);

  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create desktop handle\n");
      SetLastNtError(Status);
      return NULL;
    }

  Request.Type = CSRSS_CREATE_DESKTOP;
  memcpy(Request.Data.CreateDesktopRequest.DesktopName, lpszDesktopName->Buffer,
         lpszDesktopName->Length);
  Request.Data.CreateDesktopRequest.DesktopName[lpszDesktopName->Length / sizeof(WCHAR)] = L'\0';

  Status = CsrNotify(&Request, &Reply);
  if (! NT_SUCCESS(Status))
    {
      DPRINT1("Failed to notify CSRSS about new desktop\n");
      ZwClose(Desktop);
      SetLastNtError(Status);
      return NULL;
    }

  return Desktop;
}

/*
 * NtUserOpenDesktop
 *
 * Opens an existing desktop.
 *
 * Parameters
 *    lpszDesktopName
 *       Name of the existing desktop.
 *
 *    dwFlags
 *       Interaction flags.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 *
 * Return Value
 *    Handle to the desktop or zero on failure.
 *
 * Status
 *    @implemented
 */

HDESK STDCALL
NtUserOpenDesktop(
   PUNICODE_STRING lpszDesktopName,
   DWORD dwFlags,
   ACCESS_MASK dwDesiredAccess)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   PWINSTATION_OBJECT WinStaObject;
   UNICODE_STRING DesktopName;
   NTSTATUS Status;
   HDESK Desktop;

   /*
    * Validate the window station handle and compose the fully
    * qualified desktop name
    */

   Status = IntValidateWindowStationHandle(
      PROCESS_WINDOW_STATION(),
      KernelMode,
      0,
      &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Failed validation of window station handle (0x%X)\n",
         PROCESS_WINDOW_STATION());
      SetLastNtError(Status);
      return 0;
   }

   if (!IntGetFullWindowStationName(&DesktopName, &WinStaObject->Name,
       lpszDesktopName))
   {
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      ObDereferenceObject(WinStaObject);
      return 0;
   }
 
   ObDereferenceObject(WinStaObject);

   DPRINT("Trying to open desktop station (%wZ)\n", &DesktopName);

   /* Initialize ObjectAttributes for the desktop object */
   InitializeObjectAttributes(
      &ObjectAttributes,
      &DesktopName,
      0,
      NULL,
      NULL);

   Status = ObOpenObjectByName(
      &ObjectAttributes,
      ExDesktopObjectType,
      NULL,
      UserMode,
      dwDesiredAccess,
      NULL,
      (HANDLE*)&Desktop);

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      ExFreePool(DesktopName.Buffer);
      return 0;
   }

   DPRINT("Successfully opened desktop (%wZ)\n", &DesktopName);
   ExFreePool(DesktopName.Buffer);

   return Desktop;
}

/*
 * NtUserOpenInputDesktop
 *
 * Opens the input (interactive) desktop.
 *
 * Parameters
 *    dwFlags
 *       Interaction flags.
 *
 *    fInherit
 *       Inheritance option.
 *
 *    dwDesiredAccess
 *       Requested type of access.
 * 
 * Return Value
 *    Handle to the input desktop or zero on failure.
 *
 * Status
 *    @implemented
 */

HDESK STDCALL
NtUserOpenInputDesktop(
   DWORD dwFlags,
   BOOL fInherit,
   ACCESS_MASK dwDesiredAccess)
{
   PDESKTOP_OBJECT Object;
   NTSTATUS Status;
   HDESK Desktop;

   DPRINT("About to open input desktop\n");

   /* Get a pointer to the desktop object */

   Status = IntValidateDesktopHandle(
      InputDesktopHandle,
      UserMode,
      0,
      &Object);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of input desktop handle (0x%X) failed\n", InputDesktop);
      return (HDESK)0;
   }

   /* Create a new handle to the object */

   Status = ObOpenObjectByPointer(
      Object,
      0,
      NULL,
      dwDesiredAccess,
      ExDesktopObjectType,
      UserMode,
      (HANDLE*)&Desktop);

   ObDereferenceObject(Object);

   if (NT_SUCCESS(Status))
   {
      DPRINT("Successfully opened input desktop\n");
      return (HDESK)Desktop;
   }

   SetLastNtError(Status);
   return (HDESK)0;
}

/*
 * NtUserCloseDesktop
 *
 * Closes a desktop handle.
 *
 * Parameters
 *    hDesktop
 *       Handle to the desktop.
 *
 * Return Value
 *   Status
 *
 * Remarks
 *   The desktop handle can be created with NtUserCreateDesktop or
 *   NtUserOpenDesktop. This function will fail if any thread in the calling
 *   process is using the specified desktop handle or if the handle refers
 *   to the initial desktop of the calling process.
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserCloseDesktop(HDESK hDesktop)
{
   PDESKTOP_OBJECT Object;
   NTSTATUS Status;

   DPRINT("About to close desktop handle (0x%X)\n", hDesktop);

   Status = IntValidateDesktopHandle(
      hDesktop,
      UserMode,
      0,
      &Object);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      return FALSE;
   }

   ObDereferenceObject(Object);

   DPRINT("Closing desktop handle (0x%X)\n", hDesktop);

   Status = ZwClose(hDesktop);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return FALSE;
   }

   return TRUE;
}


static int GetSystemVersionString(LPWSTR buffer)
{
  RTL_OSVERSIONINFOEXW versionInfo;
  int len;

  versionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);

  if (!NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&versionInfo)))
	return 0;

  if (versionInfo.dwMajorVersion <= 4)
	  len = swprintf(buffer,
			  L"ReactOS Version %d.%d %s Build %d",
			  versionInfo.dwMajorVersion, versionInfo.dwMinorVersion,
			  versionInfo.szCSDVersion, versionInfo.dwBuildNumber&0xFFFF);
  else
	  len = swprintf(buffer,
			  L"ReactOS %s (Build %d)",
			  versionInfo.szCSDVersion, versionInfo.dwBuildNumber&0xFFFF);

  return len;
}

static NTSTATUS STDCALL PaintDesktopVersionCallback(
	IN PWSTR ValueName, IN ULONG ValueType,
	IN PVOID ValueData, IN ULONG ValueLength,
	IN PVOID Context, IN PVOID EntryContext
)
{
  DPRINT("PaintDesktopVersionCallback ValueType=%d ValueLength=%d\n", ValueType, ValueLength);

  if (ValueType==REG_DWORD && ValueLength==sizeof(DWORD))
	*((DWORD*)Context) = *(DWORD*)ValueData;

  return STATUS_SUCCESS;
}

/*
 * NtUserPaintDesktop
 *
 * The NtUserPaintDesktop function fills the clipping region in the
 * specified device context with the desktop pattern or wallpaper. The
 * function is provided primarily for shell desktops.
 *
 * Parameters
 *    hDC 
 *       Handle to the device context. 
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserPaintDesktop(HDC hDC)
{
  RECT Rect;
  HBRUSH DesktopBrush, PreviousBrush;
  HWND hWndDesktop;
  BOOL doPatBlt = TRUE;

  RTL_QUERY_REGISTRY_TABLE queryTable[2];
  DWORD displayVersion;
  NTSTATUS status;
  int len;

  PWINSTATION_OBJECT WinSta = PsGetWin32Thread()->Desktop->WindowStation;

  IntGdiGetClipBox(hDC, &Rect);

  hWndDesktop = IntGetDesktopWindow();
  DesktopBrush = (HBRUSH)NtUserGetClassLong(hWndDesktop, GCL_HBRBACKGROUND, FALSE);

  /*
   * Paint desktop background
   */

  if(WinSta->hbmWallpaper != NULL)
  {
    PWINDOW_OBJECT DeskWin;

    if((DeskWin = IntGetWindowObject(hWndDesktop)))
    {
      SIZE sz;
      int x, y;
      HDC hWallpaperDC;
      
      sz.cx = DeskWin->WindowRect.right - DeskWin->WindowRect.left;
      sz.cy = DeskWin->WindowRect.bottom - DeskWin->WindowRect.top;
      IntReleaseWindowObject(DeskWin);

      x = (sz.cx / 2) - (WinSta->cxWallpaper / 2);
      y = (sz.cy / 2) - (WinSta->cyWallpaper / 2);
      
      hWallpaperDC = NtGdiCreateCompatableDC(hDC);
      if(hWallpaperDC != NULL)
      {
        HBITMAP hOldBitmap;

        if(x > 0 || y > 0)
        {
          /* FIXME - clip out the bitmap */
          PreviousBrush = NtGdiSelectObject(hDC, DesktopBrush);
          NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
          NtGdiSelectObject(hDC, PreviousBrush);
        }
		else
		  doPatBlt = FALSE;

        hOldBitmap = NtGdiSelectObject(hWallpaperDC, WinSta->hbmWallpaper);
        NtGdiBitBlt(hDC, x, y, WinSta->cxWallpaper, WinSta->cyWallpaper, hWallpaperDC, 0, 0, SRCCOPY);
        NtGdiSelectObject(hWallpaperDC, hOldBitmap);
        
        NtGdiDeleteDC(hWallpaperDC);
      }
    }
  }

  if (doPatBlt) {
	PreviousBrush = NtGdiSelectObject(hDC, DesktopBrush);
	NtGdiPatBlt(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, PATCOPY);
	NtGdiSelectObject(hDC, PreviousBrush);
  }

  /*
   * Display system version on the desktop background
   */

  RtlZeroMemory(queryTable, sizeof(queryTable));

  queryTable[0].QueryRoutine = PaintDesktopVersionCallback;
  queryTable[0].Name = L"PaintDesktopVersion";
  queryTable[0].EntryContext = &displayVersion;

  /* query the "PaintDesktopVersion" flag in the "Control Panel\Desktop" key */
  status = RtlQueryRegistryValues(RTL_REGISTRY_USER, L"Control Panel\\Desktop", queryTable, NULL, NULL);

  if (!NT_SUCCESS(status)) {
	DPRINT1("RtlQueryRegistryValues failed for PaintDesktopVersion: status=%x\n", status);
	displayVersion = 0;
  }
  DPRINT("PaintDesktopVersion=%d\n", displayVersion);

  if (displayVersion) {
	static WCHAR s_wszVersion[256] = {0};
	RECT rect;

	if (*s_wszVersion)
	  len = wcslen(s_wszVersion);
	else
	  len = GetSystemVersionString(s_wszVersion);

	if (len) {
	  if (!NtUserSystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0)) {
		rect.right = NtUserGetSystemMetrics(SM_CXSCREEN);
		rect.bottom = NtUserGetSystemMetrics(SM_CYSCREEN);
	  }

	  COLORREF color_old = NtGdiSetTextColor(hDC, RGB(255,255,255));
	  UINT align_old = NtGdiSetTextAlign(hDC, TA_RIGHT);
	  int mode_old = NtGdiSetBkMode(hDC, TRANSPARENT);

	  NtGdiTextOut(hDC, rect.right-16, rect.bottom-48, s_wszVersion, len);

	  NtGdiSetBkMode(hDC, mode_old);
	  NtGdiSetTextAlign(hDC, align_old);
	  NtGdiSetTextColor(hDC, color_old);
	}
  }

  return TRUE;
}


/*
 * NtUserSwitchDesktop
 *
 * Sets the current input (interactive) desktop.
 *
 * Parameters
 *    hDesktop
 *       Handle to desktop.
 *
 * Return Value
 *    Status
 *
 * Status
 *    @unimplemented
 */

BOOL STDCALL
NtUserSwitchDesktop(HDESK hDesktop)
{
   PDESKTOP_OBJECT DesktopObject;
   NTSTATUS Status;

   DPRINT("About to switch desktop (0x%X)\n", hDesktop);

   Status = IntValidateDesktopHandle(
      hDesktop,
      UserMode,
      0,
      &DesktopObject);

   if (!NT_SUCCESS(Status)) 
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      return FALSE;
   }
   
   /*
    * Don't allow applications switch the desktop if it's locked, unless the caller
    * is the logon application itself
    */
   if((DesktopObject->WindowStation->Flags & WSS_LOCKED) &&
      LogonProcess != NULL && LogonProcess != PsGetWin32Process())
   {
      ObDereferenceObject(DesktopObject);
      DPRINT1("Switching desktop 0x%x denied because the work station is locked!\n", hDesktop);
      return FALSE;
   }
   
   /* FIXME: Fail if the desktop belong to an invisible window station */
   /* FIXME: Fail if the process is associated with a secured
             desktop such as Winlogon or Screen-Saver */
   /* FIXME: Connect to input device */

   /* Set the active desktop in the desktop's window station. */
   DesktopObject->WindowStation->ActiveDesktop = DesktopObject;

   /* Set the global state. */
   InputDesktop = DesktopObject;
   InputDesktopHandle = hDesktop;
   InputWindowStation = DesktopObject->WindowStation;

   ObDereferenceObject(DesktopObject);

   return TRUE;
}

/*
 * NtUserResolveDesktopForWOW
 *
 * Status
 *    @unimplemented
 */

DWORD STDCALL
NtUserResolveDesktopForWOW(DWORD Unknown0)
{
   UNIMPLEMENTED
   return 0;
}

/*
 * NtUserGetThreadDesktop
 *
 * Status
 *    @implemented
 */

HDESK STDCALL
NtUserGetThreadDesktop(DWORD dwThreadId, DWORD Unknown1)
{
  NTSTATUS Status;
  PETHREAD Thread;
  PDESKTOP_OBJECT DesktopObject;
  HDESK Ret, hThreadDesktop;
  OBJECT_HANDLE_INFORMATION HandleInformation;
  
  if(!dwThreadId)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  Status = PsLookupThreadByThreadId((PVOID)dwThreadId, &Thread);
  if(!NT_SUCCESS(Status))
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  if(Thread->ThreadsProcess == PsGetCurrentProcess())
  {
    /* just return the handle, we queried the desktop handle of a thread running
       in the same context */
    Ret = Thread->Win32Thread->hDesktop;
    ObDereferenceObject(Thread);
    return Ret;
  }
  
  /* get the desktop handle and the desktop of the thread */
  if(!(hThreadDesktop = Thread->Win32Thread->hDesktop) ||
     !(DesktopObject = Thread->Win32Thread->Desktop))
  {
    ObDereferenceObject(Thread);
    DPRINT1("Desktop information of thread 0x%x broken!?\n", dwThreadId);
    return NULL;
  }
  
  /* we could just use DesktopObject instead of looking up the handle, but latter
     may be a bit safer (e.g. when the desktop is being destroyed */
  /* switch into the context of the thread we're trying to get the desktop from,
     so we can use the handle */
  KeAttachProcess(Thread->ThreadsProcess);
  Status = ObReferenceObjectByHandle(hThreadDesktop,
                                     GENERIC_ALL,
				     ExDesktopObjectType,
				     UserMode,
				     (PVOID*)&DesktopObject,
				     &HandleInformation);
  KeDetachProcess();
  
  /* the handle couldn't be found, there's nothing to get... */
  if(!NT_SUCCESS(Status))
  {
    ObDereferenceObject(Thread);
    return NULL;                                               
  }
  
  /* lookup our handle table if we can find a handle to the desktop object,
     if not, create one */
  Ret = IntGetDesktopObjectHandle(DesktopObject);
  
  /* all done, we got a valid handle to the desktop */
  ObDereferenceObject(DesktopObject);
  ObDereferenceObject(Thread);
  return Ret;
}

/*
 * NtUserSetThreadDesktop
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserSetThreadDesktop(HDESK hDesktop)
{
   PW32THREAD W32Thread;
   PDESKTOP_OBJECT DesktopObject;
   NTSTATUS Status;

   /* Validate the new desktop. */
   Status = IntValidateDesktopHandle(
      hDesktop,
      UserMode,
      0,
      &DesktopObject);

   if (!NT_SUCCESS(Status)) 
   {
      DPRINT("Validation of desktop handle (0x%X) failed\n", hDesktop);
      return FALSE;
   }

   W32Thread = PsGetWin32Thread();
   /* Check for setting the same desktop as before. */
   if (DesktopObject == W32Thread->Desktop)
   {
      W32Thread->hDesktop = hDesktop;
      ObDereferenceObject(DesktopObject);
      return TRUE;
   }

   /* FIXME: Should check here to see if the thread has any windows. */

   if (W32Thread->Desktop != NULL)
   {
      ObDereferenceObject(W32Thread->Desktop);
   }

   W32Thread->Desktop = DesktopObject;
   W32Thread->hDesktop = hDesktop;

   return TRUE;
}

/* EOF */
