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
/* $Id: class.c,v 1.21 2003/08/04 16:54:54 gdalsnes Exp $
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
#include <napi/win32.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitClassImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupClassImpl(VOID)
{
  return(STATUS_SUCCESS);
}


NTSTATUS FASTCALL
ClassReferenceClassByAtom(PWNDCLASS_OBJECT* Class,
			  RTL_ATOM Atom)
{
  PWNDCLASS_OBJECT Current;
  PLIST_ENTRY CurrentEntry;
  PW32PROCESS Process = PsGetWin32Process();
  
  ExAcquireFastMutexUnsafe (&Process->ClassListLock);
  CurrentEntry = Process->ClassListHead.Flink;
  while (CurrentEntry != &Process->ClassListHead)
    {
      Current = CONTAINING_RECORD(CurrentEntry, WNDCLASS_OBJECT, ListEntry);
      
      if (Current->Class.lpszClassName == (LPWSTR)(ULONG)Atom)
	{
	  *Class = Current;
	  ObmReferenceObject(Current);
	  ExReleaseFastMutexUnsafe (&Process->ClassListLock);
	  return(STATUS_SUCCESS);
	}

      CurrentEntry = CurrentEntry->Flink;
    }
  ExReleaseFastMutexUnsafe (&Process->ClassListLock);
  
  return(STATUS_NOT_FOUND);
}

NTSTATUS STDCALL
ClassReferenceClassByName(PWNDCLASS_OBJECT *Class,
			  PWSTR ClassName)
{
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  RTL_ATOM ClassAtom;

  if (!ClassName)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return(STATUS_UNSUCCESSFUL);
    }

  Status  = RtlLookupAtomInAtomTable(WinStaObject->AtomTable,
				     ClassName,
				     &ClassAtom);

  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(WinStaObject);  
      return(Status);
    }
  Status = ClassReferenceClassByAtom(Class,
				     ClassAtom);
  
  ObDereferenceObject(WinStaObject);  
  return(Status);
}

NTSTATUS FASTCALL
ClassReferenceClassByNameOrAtom(PWNDCLASS_OBJECT *Class,
				LPWSTR ClassNameOrAtom)
{
  NTSTATUS Status;

  if (IS_ATOM(ClassNameOrAtom))
    {
      Status = ClassReferenceClassByAtom(Class, 
				  (RTL_ATOM)((ULONG_PTR)ClassNameOrAtom));
  }
  else
    {
      Status = ClassReferenceClassByName(Class, 
					 ClassNameOrAtom);
    }

  if (!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
    }
  
  return(Status);
}

DWORD STDCALL
NtUserGetClassInfo(IN LPWSTR ClassName,
		   IN ULONG InfoClass,
		   OUT PVOID Info,
		   IN ULONG InfoLength,
		   OUT PULONG ReturnedLength)
{
  UNIMPLEMENTED;
    
  return(0);
}

DWORD STDCALL
NtUserGetClassName(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
  UNIMPLEMENTED;
    
  return(0);
}

DWORD STDCALL
NtUserGetWOWClass(DWORD Unknown0,
		  DWORD Unknown1)
{
  UNIMPLEMENTED;
  
  return(0);
}

PWNDCLASS_OBJECT FASTCALL
W32kCreateClass(LPWNDCLASSEXW lpwcx,
		BOOL bUnicodeClass,
		RTL_ATOM Atom)
{
  PWNDCLASS_OBJECT ClassObject;
  WORD  objectSize;
  LPTSTR  namePtr;

  objectSize = sizeof(WNDCLASS_OBJECT) +
    (lpwcx->lpszMenuName != 0 ? ((wcslen (lpwcx->lpszMenuName) + 1) * 2) : 0);
  ClassObject = ObmCreateObject(NULL, NULL, otClass, objectSize);
  if (ClassObject == 0)
    {          
      return(NULL);
    }

  ClassObject->Class = *lpwcx;
  ClassObject->Unicode = bUnicodeClass;  
  if (lpwcx->lpszMenuName != 0)
    {
      namePtr = (LPTSTR)(((PCHAR)ClassObject) + sizeof (WNDCLASS_OBJECT));
      ClassObject->Class.lpszMenuName = namePtr;
      wcscpy (namePtr, lpwcx->lpszMenuName);
    }
  ClassObject->Class.lpszClassName = (LPWSTR)(ULONG)Atom;
  return(ClassObject);
}

RTL_ATOM STDCALL
NtUserRegisterClassExWOW(LPWNDCLASSEXW lpwcx,
			 BOOL bUnicodeClass,
			 DWORD Unknown2,
			 DWORD Unknown3,
			 DWORD Unknown4,
			 DWORD Unknown5)
/*
 * FUNCTION:
 *   Registers a new class with the window manager
 * ARGUMENTS:
 *   lpcx          = Win32 extended window class structure
 *   bUnicodeClass = Wether to send ANSI or unicode strings
 *                   to window procedures
 * RETURNS:
 *   Atom identifying the new class
 */
{
  PWINSTATION_OBJECT WinStaObject;
  PWNDCLASS_OBJECT ClassObject;
  NTSTATUS Status;
  RTL_ATOM Atom;

  DPRINT("About to open window station handle (0x%X)\n", 
	 PROCESS_WINDOW_STATION());

  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return((RTL_ATOM)0);
    }
  if (!IS_ATOM(lpwcx->lpszClassName))
    {
      Status = RtlAddAtomToAtomTable(WinStaObject->AtomTable,
				     (LPWSTR)lpwcx->lpszClassName,
				     &Atom);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject(WinStaObject);
	  DPRINT("Failed adding class name (%wS) to atom table\n",
		 lpwcx->lpszClassName);
	  SetLastNtError(Status);      
	  return((RTL_ATOM)0);
	}
    }
  else
    {
      Atom = (RTL_ATOM)(ULONG)lpwcx->lpszClassName;
    }
  ClassObject = W32kCreateClass(lpwcx, bUnicodeClass, Atom);
  if (ClassObject == NULL)
    {
      if (!IS_ATOM(lpwcx->lpszClassName))
	{
	  RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
	}
      ObDereferenceObject(WinStaObject);
      DPRINT("Failed creating window class object\n");
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      return((RTL_ATOM)0);
    }
  ExAcquireFastMutex(&PsGetWin32Process()->ClassListLock);
  InsertTailList(&PsGetWin32Process()->ClassListHead, &ClassObject->ListEntry);
  ExReleaseFastMutex(&PsGetWin32Process()->ClassListLock);
  
  ObDereferenceObject(WinStaObject);
  
  return(Atom);
}

ULONG FASTCALL
W32kGetClassLong(PWINDOW_OBJECT WindowObject, ULONG Offset)
{
  LONG Ret;
  switch (Offset)
    {
    case GCL_STYLE:
      Ret = WindowObject->Class->Class.style;
      break;
    case GCL_CBWNDEXTRA:
      Ret = WindowObject->Class->Class.cbWndExtra;
      break;
    case GCL_CBCLSEXTRA:
      Ret = WindowObject->Class->Class.cbClsExtra;
      break;
    case GCL_HMODULE:
      Ret = (ULONG)WindowObject->Class->Class.hInstance;
      break;
    case GCL_HBRBACKGROUND:
      Ret = (ULONG)WindowObject->Class->Class.hbrBackground;
      break;
    default:
      Ret = 0;
      break;
    }
  return(Ret);
}

DWORD STDCALL
NtUserGetClassLong(HWND hWnd, DWORD Offset)
{
  PWINDOW_OBJECT WindowObject;
  LONG Ret;

  WindowObject = W32kGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return(0);
    }
  Ret = W32kGetClassLong(WindowObject, Offset);
  W32kReleaseWindowObject(WindowObject);
  return(Ret);
}

DWORD STDCALL
NtUserSetClassLong(HWND hWnd, int Index, LONG NewValue, DWORD unk)
{
  UNIMPLEMENTED;

  return(0);
}

DWORD STDCALL
NtUserSetClassWord(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
  UNIMPLEMENTED;

  return(0);
}

DWORD STDCALL
NtUserUnregisterClass(DWORD Unknown0,
		      DWORD Unknown1,
		      DWORD Unknown2)
{
  UNIMPLEMENTED;

  return(0);
}

/* EOF */
