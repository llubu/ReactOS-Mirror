/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/display.c
 * PURPOSE:         display settings
 * COPYRIGHT:       Copyright 2007 ReactOS
 *
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define SIZEOF_DEVMODEW_300 188
#define SIZEOF_DEVMODEW_400 212
#define SIZEOF_DEVMODEW_500 220

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
APIENTRY
NtUserEnumDisplaySettings(
   PUNICODE_STRING pusDeviceName,
   DWORD iModeNum,
   LPDEVMODEW lpDevMode, /* FIXME is this correct? */
   DWORD dwFlags )
{
    NTSTATUS Status;
    LPDEVMODEW pSafeDevMode;
    PUNICODE_STRING pusSafeDeviceName = NULL;
    UNICODE_STRING usSafeDeviceName;
    USHORT Size = 0, ExtraSize = 0;

    /* Copy the devmode */
    _SEH2_TRY
    {
        ProbeForRead(lpDevMode, sizeof(DEVMODEW), 1);
        Size = lpDevMode->dmSize;
        ExtraSize = lpDevMode->dmDriverExtra;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT("FIXME ? : Out of range of DEVMODEW size \n");
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    if (Size != sizeof(DEVMODEW))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    pSafeDevMode = ExAllocatePool(PagedPool, Size + ExtraSize);
    if (pSafeDevMode == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    pSafeDevMode->dmSize = Size;
    pSafeDevMode->dmDriverExtra = ExtraSize;

    /* Copy the device name */
    if (pusDeviceName != NULL)
    {
        Status = IntSafeCopyUnicodeString(&usSafeDeviceName, pusDeviceName);
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(pSafeDevMode);
            return Status;
        }
        pusSafeDeviceName = &usSafeDeviceName;
    }

    /* Call internal function */
    Status = IntEnumDisplaySettings(pusSafeDeviceName, iModeNum, pSafeDevMode, dwFlags);

    if (pusSafeDeviceName != NULL)
        RtlFreeUnicodeString(pusSafeDeviceName);

    if (!NT_SUCCESS(Status))
    {
        ExFreePool(pSafeDevMode);
        return Status;
    }

    /* Copy some information back */
    _SEH2_TRY
    {
        ProbeForWrite(lpDevMode,Size + ExtraSize, 1);
        lpDevMode->dmPelsWidth = pSafeDevMode->dmPelsWidth;
        lpDevMode->dmPelsHeight = pSafeDevMode->dmPelsHeight;
        lpDevMode->dmBitsPerPel = pSafeDevMode->dmBitsPerPel;
        lpDevMode->dmDisplayFrequency = pSafeDevMode->dmDisplayFrequency;
        lpDevMode->dmDisplayFlags = pSafeDevMode->dmDisplayFlags;

        /* output private/extra driver data */
        if (ExtraSize > 0)
        {
            memcpy((PCHAR)lpDevMode + Size, (PCHAR)pSafeDevMode + Size, ExtraSize);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ExFreePool(pSafeDevMode);
    return Status;
}


LONG
APIENTRY
NtUserChangeDisplaySettings(
   PUNICODE_STRING lpszDeviceName,
   LPDEVMODEW lpDevMode,
   HWND hwnd,
   DWORD dwflags,
   LPVOID lParam)
{
   NTSTATUS Status = STATUS_SUCCESS;
   LPDEVMODEW lpSafeDevMode = NULL;
   DEVMODEW DevMode;
   PUNICODE_STRING pSafeDeviceName = NULL;
   UNICODE_STRING SafeDeviceName;
   LONG Ret;

   /* Check arguments */
#ifdef CDS_VIDEOPARAMETERS
    if (dwflags != CDS_VIDEOPARAMETERS && lParam != NULL)
#else
    if (lParam != NULL)
#endif
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return DISP_CHANGE_BADPARAM;
    }

    if (hwnd != NULL)
    {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return DISP_CHANGE_BADPARAM;
    }

    /* Copy devmode */
    if (lpDevMode != NULL)
    {
        _SEH2_TRY
        {
            ProbeForRead(lpDevMode, sizeof(DevMode.dmSize), 1);
            DevMode.dmSize = lpDevMode->dmSize;
            DevMode.dmSize = min(sizeof(DevMode), DevMode.dmSize);
            ProbeForRead(lpDevMode, DevMode.dmSize, 1);
            RtlCopyMemory(&DevMode, lpDevMode, DevMode.dmSize);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END

        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            return DISP_CHANGE_BADPARAM;
        }

        if (DevMode.dmDriverExtra > 0)
        {
            DPRINT1("lpDevMode->dmDriverExtra is IGNORED!\n");
            DevMode.dmDriverExtra = 0;
        }
        lpSafeDevMode = &DevMode;
    }

    /* Copy the device name */
    if (lpszDeviceName != NULL)
    {
        Status = IntSafeCopyUnicodeString(&SafeDeviceName, lpszDeviceName);
        if (!NT_SUCCESS(Status))
        {
            SetLastNtError(Status);
            return DISP_CHANGE_BADPARAM;
        }
        pSafeDeviceName = &SafeDeviceName;
    }

    /* Call internal function */
    Ret = IntChangeDisplaySettings(pSafeDeviceName, lpSafeDevMode, dwflags, lParam);

    if (pSafeDeviceName != NULL)
        RtlFreeUnicodeString(pSafeDeviceName);

    return Ret;
}

