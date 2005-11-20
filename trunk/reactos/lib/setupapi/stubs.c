/*
 * SetupAPI stubs
 *
 * Copyright 2000 James Hatheway
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "setupapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *		TPWriteProfileString (SETUPX.62)
 */
BOOL WINAPI TPWriteProfileString16( LPCSTR section, LPCSTR entry, LPCSTR string )
{
    FIXME( "%s %s %s: stub\n", debugstr_a(section), debugstr_a(entry), debugstr_a(string) );
    return TRUE;
}


/***********************************************************************
 *		suErrorToIds  (SETUPX.61)
 */
DWORD WINAPI suErrorToIds16( WORD w1, WORD w2 )
{
    FIXME( "%x %x: stub\n", w1, w2 );
    return 0;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListDetailA  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInfoListDetailA(HDEVINFO devinfo, PSP_DEVINFO_LIST_DETAIL_DATA_A devinfo_data )
{
  FIXME("\n");
  return FALSE;
}

/***********************************************************************
 *		SetupDiGetDeviceInfoListDetailW  (SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetDeviceInfoListDetailW(HDEVINFO devinfo, PSP_DEVINFO_LIST_DETAIL_DATA_W devinfo_data )
{
  FIXME("\n");
  return FALSE;
}

/***********************************************************************
 *		SetupCopyOEMInfA  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfA(PCSTR sourceinffile, PCSTR sourcemedialoc,
			    DWORD mediatype, DWORD copystyle, PSTR destinfname,
			    DWORD destnamesize, PDWORD required,
			    PSTR *destinfnamecomponent)
{
  FIXME("stub: source %s location %s ...\n", debugstr_a(sourceinffile),
        debugstr_a(sourcemedialoc));
  return FALSE;
}

/***********************************************************************
 *      SetupCopyOEMInfW  (SETUPAPI.@)
 */
BOOL WINAPI SetupCopyOEMInfW(PCWSTR sourceinffile, PCWSTR sourcemedialoc,
                DWORD mediatype, DWORD copystyle, PWSTR destinfname,
                DWORD destnamesize, PDWORD required,
                PWSTR *destinfnamecomponent)
{
  FIXME("stub: source %s location %s ...\n", debugstr_w(sourceinffile),
        debugstr_w(sourcemedialoc));
  //return FALSE;
  return TRUE;
}

/***********************************************************************
 *		SetupGetInfInformationA    (SETUPAPI.@)
 */
BOOL WINAPI SetupGetInfInformationA( LPCVOID InfSpec, DWORD SearchControl,
                                     PSP_INF_INFORMATION ReturnBuffer,
                                     DWORD ReturnBufferSize, PDWORD RequiredSize)
{
    FIXME("(%p, %ld, %p, %ld, %p) Stub!\n",
          InfSpec, SearchControl, ReturnBuffer, ReturnBufferSize, RequiredSize );
    return TRUE;
}

/***********************************************************************
 *		SetupInitializeFileLogW(SETUPAPI.@)
 */
HANDLE WINAPI SetupInitializeFileLogW(LPCWSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%lx\n",debugstr_w(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupInitializeFileLogA(SETUPAPI.@)
 */
HANDLE WINAPI SetupInitializeFileLogA(LPCSTR LogFileName, DWORD Flags)
{
    FIXME("Stub %s, 0x%lx\n",debugstr_a(LogFileName),Flags);
    return INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupPromptReboot(SETUPAPI.@)
 */
INT WINAPI SetupPromptReboot(HSPFILEQ FileQueue, HWND Owner, BOOL ScanOnly)
{
#if 0
    int ret;
    TCHAR RebootText[RC_STRING_MAX_SIZE];
    TCHAR RebootCaption[RC_STRING_MAX_SIZE];
    INT rc = 0;

    TRACE("%p %p %d\n", FileQueue, Owner, ScanOnly);

    if (ScanOnly && !FileQueue)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    if (FileQueue)
    {
        FIXME("Case 'FileQueue != NULL' not implemented\n");
        /* In some cases, do 'rc |= SPFILEQ_FILE_IN_USE' */
    }

    if (ScanOnly)
        return rc;

    /* We need to ask the question to the user. */
    rc |= SPFILEQ_REBOOT_RECOMMENDED;
    if (0 == LoadString(hInstance, IDS_QUERY_REBOOT_TEXT, RebootText, RC_STRING_MAX_SIZE))
        return -1;
    if (0 == LoadString(hInstance, IDS_QUERY_REBOOT_CAPTION, RebootCaption, RC_STRING_MAX_SIZE))
        return -1;
    ret = MessageBox(Owner, RebootText, RebootCaption, MB_YESNO | MB_DEFBUTTON1);
    if (IDNO == ret)
        return rc;
    else
    {
        if (ExitWindowsEx(EWX_REBOOT, 0))
            return rc | SPFILEQ_REBOOT_IN_PROGRESS;
        else
            return -1;
    }
#endif
    FIXME("Stub %p %p %d\n", FileQueue, Owner, ScanOnly);
    SetLastError(ERROR_GEN_FAILURE);
    return -1;
}


/***********************************************************************
 *		SetupTerminateFileLog(SETUPAPI.@)
 */
BOOL WINAPI SetupTerminateFileLog(HANDLE FileLogHandle)
{
    FIXME ("Stub %p\n",FileLogHandle);
    return TRUE;
}


/***********************************************************************
 *		SetupDiGetClassImageList(SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassImageList(PSP_CLASSIMAGELIST_DATA ClassImageListData)
{
    FIXME ("Stub %p\n", ClassImageListData);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *		SetupDiGetClassImageListExA(SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassImageListExA(PSP_CLASSIMAGELIST_DATA ClassImageListData,
                                        PCSTR MachineName, PVOID Reserved)
{
    FIXME ("Stub %p %s %p\n", ClassImageListData, debugstr_a(MachineName), Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *		SetupDiGetClassImageListExW(SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassImageListExW(PSP_CLASSIMAGELIST_DATA ClassImageListData,
                                        PCWSTR MachineName, PVOID Reserved)
{
    FIXME ("Stub %p %s %p\n", ClassImageListData, debugstr_w(MachineName), Reserved);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *		SetupDiGetClassImageIndex(SETUPAPI.@)
 */
BOOL WINAPI SetupDiGetClassImageIndex(PSP_CLASSIMAGELIST_DATA ClassImageListData,
                                      CONST GUID *ClassGuid, PINT ImageIndex)
{
    FIXME ("Stub %p %p %p\n", ClassImageListData, ClassGuid, ImageIndex);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *		SetupDiDestroyClassImageList(SETUPAPI.@)
 */
BOOL WINAPI SetupDiDestroyClassImageList(PSP_CLASSIMAGELIST_DATA ClassImageListData)
{
    FIXME ("Stub %p\n", ClassImageListData);
    return TRUE;
}
