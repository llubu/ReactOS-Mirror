/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS msgina.dll
 * FILE:            lib/msgina/stubs.c
 * PURPOSE:         msgina.dll stubs
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      24-11-2003  Created
 */
#include <windows.h>
#include <winwlx.h>

#include <wine/debug.h>

/*
 * @unimplemented
 */
DWORD WINAPI
ShellShutdownDialog(
    HWND  hParent,
    DWORD Unknown,
    BOOL  bHideLogoff)
{
  /* Return values:
   * 0x00: Cancelled/Help
   * 0x01: Log off user
   * 0x02: Shutdown
   * 0x04: Reboot
   * 0x10: Standby
   * 0x40: Hibernate
   */
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxDisplayLockedNotice(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxIsLockOk(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return TRUE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxIsLogoffOk(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return TRUE;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxLogoff(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxShutdown(
	PVOID pWlxContext,
	DWORD ShutdownType)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
int WINAPI
WlxWkstaLockedSAS(
	PVOID pWlxContext,
	DWORD dwSasType)
{
  UNIMPLEMENTED;
  return WLX_SAS_ACTION_UNLOCK_WKSTA;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxScreenSaverNotify(
	PVOID pWlxContext,
	BOOL  *pSecure)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxGetStatusMessage(
	PVOID pWlxContext,
	DWORD *pdwOptions,
	PWSTR pMessage,
	DWORD dwBufferSize)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxNetworkProviderLoad(
	PVOID                pWlxContext,
	PWLX_MPR_NOTIFY_INFO pNprNotifyInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
VOID WINAPI
WlxDisconnectNotify(
	PVOID pWlxContext)
{
  UNIMPLEMENTED;
  return;
}


/*
 * @unimplemented
 */
BOOL WINAPI
WlxGetConsoleSwitchCredentials(
	PVOID pWlxContext,
	PVOID pCredInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

