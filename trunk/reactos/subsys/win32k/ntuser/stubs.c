/* $Id: stubs.c,v 1.24 2003/08/06 13:17:44 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native User stubs
 * FILE:             subsys/win32k/ntuser/stubs.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       04-06-2001  CSH  Created
 */
#include <ddk/ntddk.h>
#include <windows.h>

#define NDEBUG
#include <debug.h>


DWORD
STDCALL
NtUserActivateKeyboardLayout(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserAttachThreadInput(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserBitBltSysBmp(
  DWORD Unknown0,
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

DWORD
STDCALL
NtUserBlockInput(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}


DWORD
STDCALL
NtUserBuildNameList(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}


DWORD
STDCALL
NtUserCallHwnd(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCallHwndOpt(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCallHwndParam(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCallHwndParamLock(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCallMsgFilter(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserChangeClipboardChain(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

LONG
STDCALL
NtUserChangeDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  LPDEVMODEW lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCloseClipboard(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserConvertMemHandle(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCopyAcceleratorTable(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCountClipboardFormats(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCreateAcceleratorTable(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCreateCaret(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserCreateLocalMemHandle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDdeGetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDdeInitialize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDdeSetQualityOfService(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDefSetText(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDestroyAcceleratorTable(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}


DWORD
STDCALL
NtUserDragObject(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDrawAnimatedRects(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDrawCaption(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserDrawCaptionTemp(
  DWORD Unknown0,
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

DWORD
STDCALL
NtUserDrawIconEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7,
  DWORD Unknown8,
  DWORD Unknown9,
  DWORD Unknown10)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserEmptyClipboard(VOID)
{
  UNIMPLEMENTED

  return 0;
}

WINBOOL
STDCALL
NtUserEnumDisplayDevices (
  PUNICODE_STRING lpDevice, /* device name */
  DWORD iDevNum, /* display device */
  PDISPLAY_DEVICE lpDisplayDevice, /* device information */
  DWORD dwFlags ) /* reserved */
{
  UNIMPLEMENTED

  return 0;
}

WINBOOL
STDCALL
NtUserEnumDisplayMonitors(
  HDC hdc,
  LPCRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData)
{
  UNIMPLEMENTED

  return 0;
}

WINBOOL
STDCALL
NtUserEnumDisplaySettings(
  PUNICODE_STRING lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODEW lpDevMode, /* FIXME is this correct? */
  DWORD dwFlags )
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserEvent(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserExcludeUpdateRgn(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetAltTabInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetAsyncKeyState(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetCaretBlinkTime(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetCaretPos(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetClipboardData(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetClipboardFormatName(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetClipboardOwner(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetClipboardSequenceNumber(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetClipboardViewer(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetComboBoxInfo(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetControlBrush(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetControlColor(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetCPD(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetDoubleClickTime(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetGuiResources(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetGUIThreadInfo(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetKeyboardLayoutList(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetKeyboardLayoutName(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetKeyNameText(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetListBoxInfo(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetMouseMovePointsEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetPriorityClipboardFormat(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetThreadState(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetTitleBarInfo(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetUpdateRect(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserHideCaret(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserImpersonateDdeClientWindow(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserInitializeClientPfnArrays(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserInitTask(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6,
  DWORD Unknown7,
  DWORD Unknown8,
  DWORD Unknown9,
  DWORD Unknown10)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserIsClipboardFormatAvailable(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserLoadKeyboardLayoutEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserLockWorkStation(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserMapVirtualKeyEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserMinMaximize(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserMNDragLeave(VOID)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserMNDragOver(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserModifyUserStartupInfoFlags(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserNotifyIMEStatus(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserNotifyWinEvent(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserOpenClipboard(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserQueryUserCounters(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserRegisterHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserRegisterTasklist(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}


DWORD
STDCALL
NtUserSBGetParms(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSendInput(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetClipboardData(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetClipboardViewer(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetConsoleReserveKeys(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetDbgTag(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetImeHotKey(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetParent(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetRipFlags(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetSysColors(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetThreadState(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserShowCaret(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSystemParametersInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserToUnicodeEx(
  DWORD Unknown0,
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

DWORD
STDCALL
NtUserTrackMouseEvent(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserTranslateAccelerator(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUnloadKeyboardLayout(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUnregisterHotKey(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUpdateInputContext(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUpdateInstance(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUpdatePerUserSystemParameters(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUserHandleGrantAccess(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserValidateHandleSecure(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserVkKeyScanEx(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserWaitForInputIdle(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserWaitForMsgAndEvent(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserWin32PoolAllocationStats(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserYieldTask(VOID)
{
  UNIMPLEMENTED

  return 0;
}

/* EOF */
