/* $Id: window.c,v 1.5 2002/01/27 01:11:24 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
InitWindowImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS
CleanupWindowImpl(VOID)
{
  return(STATUS_SUCCESS);
}


DWORD STDCALL
NtUserAlterWindowStyle(DWORD Unknown0,
		       DWORD Unknown1,
		       DWORD Unknown2)
{
  UNIMPLEMENTED

  return(0);
}

DWORD STDCALL
NtUserChildWindowFromPointEx(DWORD Unknown0,
			     DWORD Unknown1,
			     DWORD Unknown2,
			     DWORD Unknown3)
{
  UNIMPLEMENTED

  return(0);
}

HWND STDCALL
NtUserCreateWindowEx(DWORD dwExStyle,
		     PUNICODE_STRING lpClassName,
		     PUNICODE_STRING lpWindowName,
		     DWORD dwStyle,
		     LONG x,
		     LONG y,
		     LONG nWidth,
		     LONG nHeight,
		     HWND hWndParent,
		     HMENU hMenu,
		     HINSTANCE hInstance,
		     LPVOID lpParam,
		     DWORD Unknown12)
{
  PWINSTATION_OBJECT WinStaObject;
  PWNDCLASS_OBJECT ClassObject;
  PWINDOW_OBJECT WindowObject;
  UNICODE_STRING WindowName;
  NTSTATUS Status;
  HANDLE Handle;

  W32kGuiCheck();

  Status = ClassReferenceClassByNameOrAtom(&ClassObject, lpClassName->Buffer);
  if (!NT_SUCCESS(Status))
    {
      return (HWND)0;
    }
  
  if (!RtlCreateUnicodeString(&WindowName, lpWindowName->Buffer))
    {
      ObmDereferenceObject(ClassObject);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return (HWND)0;
    }

  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&WindowName);
      ObmDereferenceObject(ClassObject);
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return (HWND)0;
    }

  WindowObject = (PWINDOW_OBJECT) 
    ObmCreateObject(PsGetWin32Process()->HandleTable, &Handle, otWindow, 
		    sizeof(WINDOW_OBJECT));
  if (!WindowObject) 
    {
      ObDereferenceObject(WinStaObject);
      ObmDereferenceObject(ClassObject);
      RtlFreeUnicodeString(&WindowName);
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return (HWND)0;
    }

  ObDereferenceObject(WinStaObject);
  
  WindowObject->Class = ClassObject;
  WindowObject->ExStyle = dwExStyle;
  WindowObject->Style = dwStyle;
  WindowObject->x = x;
  WindowObject->y = y;
  WindowObject->Width = nWidth;
  WindowObject->Height = nHeight;
  WindowObject->Parent = hWndParent;
  WindowObject->Menu = hMenu;
  WindowObject->Instance = hInstance;
  WindowObject->Parameters = lpParam;

  RtlInitUnicodeString(&WindowObject->WindowName, WindowName.Buffer);

  ExAcquireFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);
  InsertTailList (&PsGetWin32Process()->WindowListHead, 
		  &WindowObject->ListEntry);
  ExReleaseFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);

  return (HWND)Handle;
}

DWORD STDCALL
NtUserDeferWindowPos(DWORD Unknown0,
		     DWORD Unknown1,
		     DWORD Unknown2,
		     DWORD Unknown3,
		     DWORD Unknown4,
		     DWORD Unknown5,
		     DWORD Unknown6,
		     DWORD Unknown7)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserDestroyWindow(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserEndDeferWindowPosEx(DWORD Unknown0,
			  DWORD Unknown1)
{
  UNIMPLEMENTED
    
  return 0;
}

DWORD STDCALL
NtUserFillWindow(DWORD Unknown0,
		 DWORD Unknown1,
		 DWORD Unknown2,
		 DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

HWND STDCALL
NtUserFindWindowEx(HWND hwndParent,
		   HWND hwndChildAfter,
		   PUNICODE_STRING ucClassName,
		   PUNICODE_STRING ucWindowName,
		   DWORD Unknown4)
{
  NTSTATUS status;
  HWND windowHandle;
  PWINDOW_OBJECT windowObject;
  PLIST_ENTRY currentEntry;
  PWNDCLASS_OBJECT classObject;
  
  W32kGuiCheck();
  
  status = ClassReferenceClassByNameOrAtom(&classObject, ucClassName->Buffer);
  if (!NT_SUCCESS(status))
    {
      return (HWND)0;
    }

  ExAcquireFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);
  currentEntry = PsGetWin32Process()->WindowListHead.Flink;
  while (currentEntry != &PsGetWin32Process()->WindowListHead)
    {
      windowObject = CONTAINING_RECORD (currentEntry, WINDOW_OBJECT, 
					ListEntry);

      if (classObject == windowObject->Class &&
	  RtlCompareUnicodeString (ucWindowName, &windowObject->WindowName, 
				   TRUE) == 0)
	{
	  ObmCreateHandle(PsGetWin32Process()->HandleTable,
			  windowObject,
			  &windowHandle);
	  ExReleaseFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);
	  ObmDereferenceObject (classObject);
      
	  return  windowHandle;
	}
      currentEntry = currentEntry->Flink;
  }
  ExReleaseFastMutexUnsafe (&PsGetWin32Process()->WindowListLock);
  
  ObmDereferenceObject (classObject);

  return  (HWND)0;
}

DWORD STDCALL
NtUserFlashWindowEx(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetForegroundWindow(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetInternalWindowPos(DWORD Unknown0,
			   DWORD Unknown1,
			   DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetOpenClipboardWindow(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetWindowDC(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserGetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserInternalGetWindowText(DWORD Unknown0,
			    DWORD Unknown1,
			    DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserLockWindowUpdate(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserMoveWindow(DWORD Unknown0,
		 DWORD Unknown1,
		 DWORD Unknown2,
		 DWORD Unknown3,
		 DWORD Unknown4,
		 DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserQueryWindow(DWORD Unknown0,
		  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserRealChildWindowFromPoint(DWORD Unknown0,
			       DWORD Unknown1,
			       DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserRedrawWindow(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2,
		   DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserRegisterWindowMessage(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserScrollWindowEx(DWORD Unknown0,
		     DWORD Unknown1,
		     DWORD Unknown2,
		     DWORD Unknown3,
		     DWORD Unknown4,
		     DWORD Unknown5,
		     DWORD Unknown6,
		     DWORD Unknown7)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetActiveWindow(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetImeOwnerWindow(DWORD Unknown0,
			DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetInternalWindowPos(DWORD Unknown0,
			   DWORD Unknown1,
			   DWORD Unknown2,
			   DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetLayeredWindowAttributes(DWORD Unknown0,
				 DWORD Unknown1,
				 DWORD Unknown2,
				 DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetLogonNotifyWindow(DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetShellWindowEx(DWORD Unknown0,
		       DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowFNID(DWORD Unknown0,
		    DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowLong(DWORD Unknown0,
		    DWORD Unknown1,
		    DWORD Unknown2,
		    DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowPlacement(DWORD Unknown0,
			 DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowPos(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2,
		   DWORD Unknown3,
		   DWORD Unknown4,
		   DWORD Unknown5,
		   DWORD Unknown6)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowRgn(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserSetWindowWord(DWORD Unknown0,
		    DWORD Unknown1,
		    DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL
NtUserShowWindow(HWND hWnd,
		 LONG nCmdShow)
{
  PWINDOW_OBJECT WindowObject;
  NTSTATUS Status;
  
  W32kGuiCheck();

  Status = ObmReferenceObjectByHandle(PsGetWin32Process()->HandleTable,
				      hWnd,
				      otWindow,
				      (PVOID*)&WindowObject);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }

  return TRUE;
}

DWORD STDCALL
NtUserShowWindowAsync(DWORD Unknown0,
		      DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserUpdateLayeredWindow(DWORD Unknown0,
			  DWORD Unknown1,
			  DWORD Unknown2,
			  DWORD Unknown3,
			  DWORD Unknown4,
			  DWORD Unknown5,
			  DWORD Unknown6,
			  DWORD Unknown7,
			  DWORD Unknown8)
{
  UNIMPLEMENTED

  return 0;
}

DWORD STDCALL
NtUserWindowFromPoint(DWORD Unknown0,
		      DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

/* EOF */
