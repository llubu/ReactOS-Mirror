/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/class.c
 * PURPOSE:         Window classes
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

#include <user32.h>

extern BOOL ControlsInitialized;

/*
 * @implemented
 */
BOOL
STDCALL
GetClassInfoExA(
  HINSTANCE hinst,
  LPCSTR lpszClass,
  LPWNDCLASSEXA lpwcx)
{
    UNICODE_STRING ClassName = {0};
    BOOL Ret;

    if (lpszClass == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IS_ATOM(lpszClass))
    {
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpszClass);
    }
    else
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&ClassName,
                                              lpszClass))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }

    /* Register built-in controls if not already done */
    if (! ControlsInitialized)
    {
        ControlsInitialized = ControlsInit(ClassName.Buffer);
    }

    Ret = NtUserGetClassInfo(hinst,
                             &ClassName,
                             (LPWNDCLASSEXW)lpwcx,
                             TRUE);

    if (!IS_ATOM(lpszClass))
    {
        RtlFreeUnicodeString(&ClassName);
    }

    return Ret;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetClassInfoExW(
  HINSTANCE hinst,
  LPCWSTR lpszClass,
  LPWNDCLASSEXW lpwcx)
{
    UNICODE_STRING ClassName = {0};

    if (lpszClass == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (IS_ATOM(lpszClass))
    {
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpszClass);
    }
    else
    {
        RtlInitUnicodeString(&ClassName,
                             lpszClass);
    }

    /* Register built-in controls if not already done */
    if (! ControlsInitialized)
    {
        ControlsInitialized = ControlsInit(ClassName.Buffer);
    }

    return NtUserGetClassInfo(hinst,
                              &ClassName,
                              lpwcx,
                              FALSE);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetClassInfoA(
  HINSTANCE hInstance,
  LPCSTR lpClassName,
  LPWNDCLASSA lpWndClass)
{
  WNDCLASSEXA w;
  BOOL retval;

  if ( !lpClassName || !lpWndClass )
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  w.cbSize = sizeof(w);
  retval = GetClassInfoExA(hInstance,lpClassName,&w);
  if (retval)
  {
    RtlCopyMemory ( lpWndClass, &w.style, sizeof(WNDCLASSA) );
  }
  return retval;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetClassInfoW(
  HINSTANCE hInstance,
  LPCWSTR lpClassName,
  LPWNDCLASSW lpWndClass)
{
  WNDCLASSEXW w;
  BOOL retval;

  if(!lpClassName || !lpWndClass)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  w.cbSize = sizeof(w);
  retval = GetClassInfoExW(hInstance,lpClassName,&w);
  if (retval)
  {
    RtlCopyMemory (lpWndClass,&w.style,sizeof(WNDCLASSW));
  }
  return retval;
}


/*
 * @implemented
 */
DWORD STDCALL
GetClassLongA(HWND hWnd, int nIndex)
{
   switch (nIndex)
   {
      case GCL_HBRBACKGROUND:
         {
            DWORD hBrush = NtUserGetClassLong(hWnd, GCL_HBRBACKGROUND, TRUE);
            if (hBrush != 0 && hBrush < 0x4000)
               hBrush = (DWORD)GetSysColorBrush((ULONG)hBrush - 1);
            return hBrush;
         }

      case GCL_MENUNAME:
         {
            PUNICODE_STRING Name;
            Name = (PUNICODE_STRING)NtUserGetClassLong(hWnd, nIndex, TRUE);
            if (IS_INTRESOURCE(Name))
               return (DWORD)Name;
            else
               return (DWORD)heap_string_poolA(Name->Buffer, Name->Length);
         }

      default:
         return NtUserGetClassLong(hWnd, nIndex, TRUE);
   }
}

/*
 * @implemented
 */
DWORD STDCALL
GetClassLongW ( HWND hWnd, int nIndex )
{
   switch (nIndex)
   {
      case GCL_HBRBACKGROUND:
         {
            DWORD hBrush = NtUserGetClassLong(hWnd, GCL_HBRBACKGROUND, TRUE);
            if (hBrush != 0 && hBrush < 0x4000)
               hBrush = (DWORD)GetSysColorBrush((ULONG)hBrush - 1);
            return hBrush;
         }

      case GCL_MENUNAME:
         {
            PUNICODE_STRING Name;
            Name = (PUNICODE_STRING)NtUserGetClassLong(hWnd, nIndex, FALSE);
            if (IS_INTRESOURCE(Name))
               return (DWORD)Name;
            else
               return (DWORD)heap_string_poolW(Name->Buffer, Name->Length);
         }

      default:
         return NtUserGetClassLong(hWnd, nIndex, FALSE);
   }
}


/*
 * @implemented
 */
int STDCALL
GetClassNameA(
  HWND hWnd,
  LPSTR lpClassName,
  int nMaxCount)
{
    ANSI_STRING ClassName;

    ClassName.MaximumLength = nMaxCount;
    ClassName.Buffer = lpClassName;

    return NtUserGetClassName(hWnd,
                              (PUNICODE_STRING)&ClassName,
                              TRUE);
}


/*
 * @implemented
 */
int
STDCALL
GetClassNameW(
  HWND hWnd,
  LPWSTR lpClassName,
  int nMaxCount)
{
    UNICODE_STRING ClassName;

    ClassName.MaximumLength = nMaxCount;
    ClassName.Buffer = lpClassName;

    return NtUserGetClassName(hWnd,
                              &ClassName,
                              FALSE);
}


/*
 * @implemented
 */
WORD
STDCALL
GetClassWord(
  HWND hWnd,
  int nIndex)
/*
 * NOTE: Obsoleted in 32-bit windows
 */
{
    if ((nIndex < 0) && (nIndex != GCW_ATOM))
        return 0;

    return (WORD) NtUserGetClassLong ( hWnd, nIndex, TRUE );
}


/*
 * @implemented
 */
LONG
STDCALL
GetWindowLongA ( HWND hWnd, int nIndex )
{
  return NtUserGetWindowLong(hWnd, nIndex, TRUE);
}


/*
 * @implemented
 */
LONG
STDCALL
GetWindowLongW(HWND hWnd, int nIndex)
{
  return NtUserGetWindowLong(hWnd, nIndex, FALSE);
}

/*
 * @implemented
 */
WORD
STDCALL
GetWindowWord(HWND hWnd, int nIndex)
{
  return (WORD)NtUserGetWindowLong(hWnd, nIndex,  TRUE);
}

/*
 * @implemented
 */
WORD
STDCALL
SetWindowWord ( HWND hWnd,int nIndex,WORD wNewWord )
{
  return (WORD)NtUserSetWindowLong ( hWnd, nIndex, (LONG)wNewWord, TRUE );
}

/*
 * @implemented
 */
UINT
STDCALL
RealGetWindowClassW(
  HWND  hwnd,
  LPWSTR pszType,
  UINT  cchType)
{
	/* FIXME: Implement correct functionality of RealGetWindowClass */
	return GetClassNameW(hwnd,pszType,cchType);
}


/*
 * @implemented
 */
UINT
STDCALL
RealGetWindowClassA(
  HWND  hwnd,
  LPSTR pszType,
  UINT  cchType)
{
	/* FIXME: Implement correct functionality of RealGetWindowClass */
	return GetClassNameA(hwnd,pszType,cchType);
}

/*
 * Create a small icon based on a standard icon
 */
static HICON
CreateSmallIcon(HICON StdIcon)
{
   HICON SmallIcon = NULL;
   ICONINFO StdInfo;
   int SmallIconWidth;
   int SmallIconHeight;
   BITMAP StdBitmapInfo;
   HDC hInfoDc = NULL;
   HDC hSourceDc = NULL;
   HDC hDestDc = NULL;
   ICONINFO SmallInfo;
   HBITMAP OldSourceBitmap = NULL;
   HBITMAP OldDestBitmap = NULL;

   SmallInfo.hbmColor = NULL;
   SmallInfo.hbmMask = NULL;

   /* We need something to work with... */
   if (NULL == StdIcon)
   {
      goto cleanup;
   }

   SmallIconWidth = GetSystemMetrics(SM_CXSMICON);
   SmallIconHeight = GetSystemMetrics(SM_CYSMICON);
   if (! GetIconInfo(StdIcon, &StdInfo))
   {
      DPRINT1("Failed to get icon info for icon 0x%x\n", StdIcon);
      goto cleanup;
   }
   if (! GetObjectW(StdInfo.hbmMask, sizeof(BITMAP), &StdBitmapInfo))
   {
      DPRINT1("Failed to get bitmap info for icon 0x%x bitmap 0x%x\n",
              StdIcon, StdInfo.hbmColor);
      goto cleanup;
   }
   if (StdBitmapInfo.bmWidth == SmallIconWidth &&
       StdBitmapInfo.bmHeight == SmallIconHeight)
   {
      /* Icon already has the correct dimensions */
      return StdIcon;
   }

   /* Get a handle to a info DC and handles to DCs which can be used to
      select a bitmap into. This is done to avoid triggering a switch to
      graphics mode (if we're currently in text/blue screen mode) */
   hInfoDc = CreateICW(NULL, NULL, NULL, NULL);
   if (NULL == hInfoDc)
   {
      DPRINT1("Failed to create info DC\n");
      goto cleanup;
   }
   hSourceDc = CreateCompatibleDC(NULL);
   if (NULL == hSourceDc)
   {
      DPRINT1("Failed to create source DC\n");
      goto cleanup;
   }
   hDestDc = CreateCompatibleDC(NULL);
   if (NULL == hDestDc)
   {
      DPRINT1("Failed to create dest DC\n");
      goto cleanup;
   }

   OldSourceBitmap = SelectObject(hSourceDc, StdInfo.hbmColor);
   if (NULL == OldSourceBitmap)
   {
      DPRINT1("Failed to select source color bitmap\n");
      goto cleanup;
   }
   SmallInfo.hbmColor = CreateCompatibleBitmap(hInfoDc, SmallIconWidth,
                                              SmallIconHeight);
   if (NULL == SmallInfo.hbmColor)
   {
      DPRINT1("Failed to create color bitmap\n");
      goto cleanup;
   }
   OldDestBitmap = SelectObject(hDestDc, SmallInfo.hbmColor);
   if (NULL == OldDestBitmap)
   {
      DPRINT1("Failed to select dest color bitmap\n");
      goto cleanup;
   }
   if (! StretchBlt(hDestDc, 0, 0, SmallIconWidth, SmallIconHeight,
                    hSourceDc, 0, 0, StdBitmapInfo.bmWidth,
                    StdBitmapInfo.bmHeight, SRCCOPY))
   {
     DPRINT1("Failed to stretch color bitmap\n");
     goto cleanup;
   }

   if (NULL == SelectObject(hSourceDc, StdInfo.hbmMask))
   {
      DPRINT1("Failed to select source mask bitmap\n");
      goto cleanup;
   }
   SmallInfo.hbmMask = CreateBitmap(SmallIconWidth, SmallIconHeight, 1, 1,
                                    NULL);
   if (NULL == SmallInfo.hbmMask)
   {
      DPRINT1("Failed to create mask bitmap\n");
      goto cleanup;
   }
   if (NULL == SelectObject(hDestDc, SmallInfo.hbmMask))
   {
      DPRINT1("Failed to select dest mask bitmap\n");
      goto cleanup;
   }
   if (! StretchBlt(hDestDc, 0, 0, SmallIconWidth, SmallIconHeight,
                    hSourceDc, 0, 0, StdBitmapInfo.bmWidth,
                    StdBitmapInfo.bmHeight, SRCCOPY))
   {
      DPRINT1("Failed to stretch mask bitmap\n");
      goto cleanup;
   }

   SmallInfo.fIcon = TRUE;
   SmallInfo.xHotspot = SmallIconWidth / 2;
   SmallInfo.yHotspot = SmallIconHeight / 2;
   SmallIcon = CreateIconIndirect(&SmallInfo);
   if (NULL == SmallIcon)
   {
      DPRINT1("Failed to create icon\n");
      goto cleanup;
   }

cleanup:
   if (NULL != SmallInfo.hbmMask)
   {
      DeleteObject(SmallInfo.hbmMask);
   }
   if (NULL != OldDestBitmap)
   {
      SelectObject(hDestDc, OldDestBitmap);
   }
   if (NULL != SmallInfo.hbmColor)
   {
      DeleteObject(SmallInfo.hbmColor);
   }
   if (NULL != hDestDc)
   {
      DeleteDC(hDestDc);
   }
   if (NULL != OldSourceBitmap)
   {
      SelectObject(hSourceDc, OldSourceBitmap);
   }
   if (NULL != hSourceDc)
   {
      DeleteDC(hSourceDc);
   }
   if (NULL != hInfoDc)
   {
      DeleteDC(hInfoDc);
   }

   return SmallIcon;
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExA(CONST WNDCLASSEXA *lpwcx)
{
   RTL_ATOM Atom;
   WNDCLASSEXA WndClass;
   UNICODE_STRING ClassName;
   UNICODE_STRING MenuName = {0};
   HMENU hMenu = NULL;

   if (lpwcx == NULL || lpwcx->cbSize != sizeof(WNDCLASSEXA) ||
       lpwcx->cbClsExtra < 0 || lpwcx->cbWndExtra < 0 ||
       lpwcx->lpszClassName == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /*
    * On real Windows this looks more like:
    *    if (lpwcx->hInstance == User32Instance &&
    *        *(PULONG)((ULONG_PTR)NtCurrentTeb() + 0x6D4) & 0x400)
    * But since I have no idea what the magic field in the
    * TEB structure means, I rather decided to omit that.
    * -- Filip Navara
    */
   if (lpwcx->hInstance == User32Instance)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /* Yes, this is correct. We should modify the passed structure. */
   if (lpwcx->hInstance == NULL)
      ((WNDCLASSEXA*)lpwcx)->hInstance = GetModuleHandleW(NULL);

   RtlCopyMemory(&WndClass, lpwcx, sizeof(WNDCLASSEXA));

   if (NULL == WndClass.hIconSm)
   {
      WndClass.hIconSm = CreateSmallIcon(WndClass.hIcon);
   }

   if (WndClass.lpszMenuName != NULL)
   {
      if (!IS_INTRESOURCE(WndClass.lpszMenuName))
      {
         if (WndClass.lpszMenuName[0])
         {
             RtlCreateUnicodeStringFromAsciiz(&MenuName, WndClass.lpszMenuName);
         }
      }
      else
      {
         MenuName.Buffer = (LPWSTR)WndClass.lpszMenuName;
      }

      if (MenuName.Buffer != NULL)
         hMenu = LoadMenuA(WndClass.hInstance, WndClass.lpszMenuName);
   }
 
   if (IS_ATOM(WndClass.lpszClassName))
   {
      ClassName.Length =
      ClassName.MaximumLength = 0;
      ClassName.Buffer = (LPWSTR)WndClass.lpszClassName;
   }
   else
   {
      RtlCreateUnicodeStringFromAsciiz(&ClassName, WndClass.lpszClassName);
   }

   Atom = NtUserRegisterClassEx((WNDCLASSEXW*)&WndClass,
                                &ClassName,
                                &MenuName,
                                NULL,
                                REGISTERCLASS_ANSI,
                                hMenu);

   if (!IS_INTRESOURCE(WndClass.lpszMenuName))
      RtlFreeUnicodeString(&MenuName);
   if (!IS_ATOM(WndClass.lpszClassName))
      RtlFreeUnicodeString(&ClassName);

   return (ATOM)Atom;
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExW(CONST WNDCLASSEXW *lpwcx)
{
   WNDCLASSEXW WndClass;
   UNICODE_STRING ClassName;
   UNICODE_STRING MenuName = {0};
   HMENU hMenu = NULL;

   if (lpwcx == NULL || lpwcx->cbSize != sizeof(WNDCLASSEXW) ||
       lpwcx->cbClsExtra < 0 || lpwcx->cbWndExtra < 0 ||
       lpwcx->lpszClassName == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /*
    * On real Windows this looks more like:
    *    if (lpwcx->hInstance == User32Instance &&
    *        *(PULONG)((ULONG_PTR)NtCurrentTeb() + 0x6D4) & 0x400)
    * But since I have no idea what the magic field in the
    * TEB structure means, I rather decided to omit that.
    * -- Filip Navara
    */
   if (lpwcx->hInstance == User32Instance)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /* Yes, this is correct. We should modify the passed structure. */
   if (lpwcx->hInstance == NULL)
      ((WNDCLASSEXW*)lpwcx)->hInstance = GetModuleHandleW(NULL);

   RtlCopyMemory(&WndClass, lpwcx, sizeof(WNDCLASSEXW));

   if (NULL == WndClass.hIconSm)
   {
      WndClass.hIconSm = CreateSmallIcon(WndClass.hIcon);
   }

   if (WndClass.lpszMenuName != NULL)
   {
      if (!IS_INTRESOURCE(WndClass.lpszMenuName))
      {
         if (WndClass.lpszMenuName[0])
         {
            RtlInitUnicodeString(&MenuName, WndClass.lpszMenuName);
         }
      }
      else
      {
         MenuName.Buffer = (LPWSTR)WndClass.lpszMenuName;
      }

      if (MenuName.Buffer != NULL)
         hMenu = LoadMenuW(WndClass.hInstance, WndClass.lpszMenuName);
   }

   if (IS_ATOM(WndClass.lpszClassName))
   {
      ClassName.Length =
      ClassName.MaximumLength = 0;
      ClassName.Buffer = (LPWSTR)WndClass.lpszClassName;
   }
   else
   {
      RtlInitUnicodeString(&ClassName, WndClass.lpszClassName);
   }

   return (ATOM)NtUserRegisterClassEx(&WndClass,
                                      &ClassName,
                                      &MenuName,
                                      NULL,
                                      0,
                                      hMenu);
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassA(CONST WNDCLASSA *lpWndClass)
{
   WNDCLASSEXA Class;

   if (lpWndClass == NULL)
      return 0;

   RtlCopyMemory(&Class.style, lpWndClass, sizeof(WNDCLASSA));
   Class.cbSize = sizeof(WNDCLASSEXA);
   Class.hIconSm = NULL;

   return RegisterClassExA(&Class);
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassW(CONST WNDCLASSW *lpWndClass)
{
   WNDCLASSEXW Class;

   if (lpWndClass == NULL)
      return 0;

   RtlCopyMemory(&Class.style, lpWndClass, sizeof(WNDCLASSW));
   Class.cbSize = sizeof(WNDCLASSEXW);
   Class.hIconSm = NULL;

   return RegisterClassExW(&Class);
}

/*
 * @implemented
 */
DWORD
STDCALL
SetClassLongA (HWND hWnd,
               int nIndex,
               LONG dwNewLong)
{
    PSTR lpStr = (PSTR)dwNewLong;
    UNICODE_STRING Value = {0};
    BOOL Allocated = FALSE;
    DWORD Ret;

    /* FIXME - portability!!!! */

    if (nIndex == GCL_MENUNAME && lpStr != NULL)
    {
        if (!IS_INTRESOURCE(lpStr))
        {
            if (!RtlCreateUnicodeStringFromAsciiz(&Value,
                                                  lpStr))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

            Allocated = TRUE;
        }
        else
            Value.Buffer = (PWSTR)lpStr;

        dwNewLong = (LONG)&Value;
    }
    else if (nIndex == GCW_ATOM && lpStr != NULL)
    {
        if (!IS_ATOM(lpStr))
        {
            if (!RtlCreateUnicodeStringFromAsciiz(&Value,
                                                  lpStr))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return 0;
            }

            Allocated = TRUE;
        }
        else
            Value.Buffer = (PWSTR)lpStr;

        dwNewLong = (LONG)&Value;
    }

    Ret = (DWORD)NtUserSetClassLong(hWnd,
                                    nIndex,
                                    dwNewLong,
                                    TRUE);

    if (Allocated)
    {
        RtlFreeUnicodeString(&Value);
    }

    return Ret;
}


/*
 * @implemented
 */
DWORD
STDCALL
SetClassLongW(HWND hWnd,
              int nIndex,
              LONG dwNewLong)
{
    PWSTR lpStr = (PWSTR)dwNewLong;
    UNICODE_STRING Value = {0};

    /* FIXME - portability!!!! */

    if (nIndex == GCL_MENUNAME && lpStr != NULL)
    {
        if (!IS_INTRESOURCE(lpStr))
        {
            RtlInitUnicodeString(&Value,
                                 lpStr);
        }
        else
            Value.Buffer = lpStr;

        dwNewLong = (LONG)&Value;
    }
    else if (nIndex == GCW_ATOM && lpStr != NULL)
    {
        if (!IS_ATOM(lpStr))
        {
            RtlInitUnicodeString(&Value,
                                 lpStr);
        }
        else
            Value.Buffer = lpStr;

        dwNewLong = (LONG)&Value;
    }

    return (DWORD)NtUserSetClassLong(hWnd,
                                     nIndex,
                                     dwNewLong,
                                     FALSE);
}


/*
 * @implemented
 */
WORD
STDCALL
SetClassWord(
  HWND hWnd,
  int nIndex,
  WORD wNewWord)
/*
 * NOTE: Obsoleted in 32-bit windows
 */
{
    if ((nIndex < 0) && (nIndex != GCW_ATOM))
        return 0;

    return (WORD) SetClassLongW ( hWnd, nIndex, wNewWord );
}


/*
 * @implemented
 */
LONG
STDCALL
SetWindowLongA(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return NtUserSetWindowLong(hWnd, nIndex, dwNewLong, TRUE);
}


/*
 * @implemented
 */
LONG
STDCALL
SetWindowLongW(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return NtUserSetWindowLong(hWnd, nIndex, dwNewLong, FALSE);
}


/*
 * @implemented
 */
BOOL
STDCALL
UnregisterClassA(
  LPCSTR lpClassName,
  HINSTANCE hInstance)
{
    UNICODE_STRING ClassName = {0};
    NTSTATUS Status;
    BOOL Ret;

    if (!IS_ATOM(lpClassName))
    {
        Status = HEAP_strdupAtoW(&ClassName.Buffer, lpClassName, NULL);
        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }

        RtlInitUnicodeString(&ClassName,
                             ClassName.Buffer);
    }
    else
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpClassName);

    Ret = NtUserUnregisterClass(&ClassName,
                                hInstance);

    if(!IS_ATOM(lpClassName) && ClassName.Buffer != NULL)
        HEAP_free(ClassName.Buffer);

    return Ret;
}


/*
 * @implemented
 */
BOOL
STDCALL
UnregisterClassW(
  LPCWSTR lpClassName,
  HINSTANCE hInstance)
{
    UNICODE_STRING ClassName = {0};

    if (!IS_ATOM(lpClassName))
    {
        RtlInitUnicodeString(&ClassName,
                             lpClassName);
    }
    else
        ClassName.Buffer = (PWSTR)((ULONG_PTR)lpClassName);

    return NtUserUnregisterClass(&ClassName,
                                 hInstance);
}

/* EOF */
