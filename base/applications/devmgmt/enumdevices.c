/*
 * PROJECT:     ReactOS Device Managment
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/devmgmt/enumdevices.c
 * PURPOSE:     Enumerates all devices on the local machine
 * COPYRIGHT:   Copyright 2006 Ged Murphy <gedmurphy@gmail.com>
 *
 */

#include "precomp.h"

SP_CLASSIMAGELIST_DATA ImageListData;


static HTREEITEM
InsertIntoTreeView(HWND hTV,
                   HTREEITEM hRoot,
                   LPTSTR lpLabel,
                   INT DevImage)
{
    TV_ITEM tvi;
    TV_INSERTSTRUCT tvins;

    ZeroMemory(&tvi, sizeof(tvi));
    ZeroMemory(&tvins, sizeof(tvins));

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.pszText = lpLabel;
    tvi.cchTextMax = lstrlen(lpLabel);
    tvi.iImage = DevImage;
    tvi.iSelectedImage = DevImage;

    tvins.item = tvi;
    tvins.hParent = hRoot;

    return TreeView_InsertItem(hTV, &tvins);
}


static INT
EnumDeviceClasses(INT ClassIndex,
                  LPTSTR DevClassName,
                  LPTSTR DevClassDesc,
                  BOOL *DevPresent,
                  INT *ClassImage)
{
    GUID ClassGuid;
    HKEY KeyClass;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD RequiredSize = MAX_CLASS_NAME_LEN;
    UINT Ret;

    *DevPresent = FALSE;

    Ret = CM_Enumerate_Classes(ClassIndex,
                               &ClassGuid,
                               0);

    if (Ret != CR_SUCCESS)
    {
        /* all classes enumerated */
        if(Ret == CR_NO_SUCH_VALUE)
            return -1;

        if (Ret == CR_INVALID_DATA)
            ; /*FIXME: what should we do here? */

        /* handle other errors... */
    }

    if (SetupDiClassNameFromGuid(&ClassGuid,
                                 ClassName,
                                 RequiredSize,
                                 &RequiredSize))
    {
        lstrcpy(DevClassName, ClassName);
    }
    else
    {
        *DevClassName = _T('\0');
    }

    if (!SetupDiGetClassImageIndex(&ImageListData,
                                   &ClassGuid,
                                   ClassImage))
    {
        /* set the blank icon */
        *ClassImage = 41;
    }

    /* FIXME: do we need this?
    hDevInfo = SetupDiGetClassDevs(&ClassGuid,
                                   0,
                                   NULL,
                                   DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        return 0;
    */

    KeyClass = SetupDiOpenClassRegKeyEx(&ClassGuid,
                                        MAXIMUM_ALLOWED,
                                        DIOCR_INSTALLER,
                                        NULL,
                                        0);
    if (KeyClass != INVALID_HANDLE_VALUE)
    {

        LONG dwSize = MAX_DEV_LEN;

        if (RegQueryValue(KeyClass,
                          NULL,
                          DevClassDesc,
                          &dwSize) != ERROR_SUCCESS)
        {
            *DevClassDesc = _T('\0');
        }
    }
    else
    {
        return -3;
    }

    /* FIXME: see above?
    SetupDiDestroyDeviceInfoList(hDevInfo); */

    *DevPresent = TRUE;

    RegCloseKey(KeyClass);

    return 0;
}


static INT
EnumDevices(INT index,
            TCHAR* DeviceClassName,
            TCHAR* DeviceName)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD RequiredSize = 0;
    GUID *guids = NULL;
    BOOL bRet;

    *DeviceName = _T('\0');

    bRet = SetupDiClassGuidsFromName(DeviceClassName,
                                     NULL,
                                     RequiredSize,
                                     &RequiredSize);
    if (RequiredSize == 0)
        return -2;

    if (!bRet)
    {
        guids = HeapAlloc(GetProcessHeap(),
                          0,
                          RequiredSize * sizeof(GUID));
        if (guids == NULL)
            return -1;

        bRet = SetupDiClassGuidsFromName(DeviceClassName,
                                         guids,
                                         RequiredSize,
                                         &RequiredSize);

        if (!bRet || RequiredSize == 0)
        {
            /* incorrect class name */
            HeapFree(GetProcessHeap(), 0, guids);
            return -3;
        }
    }

//TimerInfo(_T("IN"));
    /* get device info set for our device class */
    hDevInfo = SetupDiGetClassDevs(guids,
                                   0,
                                   NULL,
                                   DIGCF_PRESENT);
//TimerInfo(_T("OUT"));
    HeapFree(GetProcessHeap(), 0, guids);
    if(hDevInfo == INVALID_HANDLE_VALUE)
    {
        if(!bRet)
        {
            /* device info is unavailable */
            return -4;
        }
    }

    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    bRet = SetupDiEnumDeviceInfo(hDevInfo,
                                 index,
                                 &DeviceInfoData);

    if (!bRet)
    {
        //no such device:
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return -1;
    }

    /* get the device's friendly name */
    if (!SetupDiGetDeviceRegistryProperty(hDevInfo,
                                          &DeviceInfoData,
                                          SPDRP_FRIENDLYNAME,
                                          0,
                                          (BYTE*)DeviceName,
                                          MAX_DEV_LEN,
                                          NULL))
    {
        /* if the friendly name fails, try the description instead */
        bRet = SetupDiGetDeviceRegistryProperty(hDevInfo,
                                               &DeviceInfoData,
                                               SPDRP_DEVICEDESC,
                                               0,
                                               (BYTE*)DeviceName,
                                               MAX_DEV_LEN,
                                               NULL);
        if (!bRet)
        {
            /* if the description fails, just give up! */
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return -5;
        }
    }

    return 0;
}


VOID
ListDevicesByType(PMAIN_WND_INFO Info,
                  HTREEITEM hRoot)
{
    HTREEITEM hDevItem;
    TCHAR DevName[MAX_DEV_LEN];
    TCHAR DevDesc[MAX_DEV_LEN];
    BOOL DevExist = FALSE;
    INT ClassRet;
    INT index = 0;
    INT DevImage;

    do
    {
        ClassRet = EnumDeviceClasses(index,
                                     DevName,
                                     DevDesc,
                                     &DevExist,
                                     &DevImage);

        if ((ClassRet != -1) && (DevExist))
        {
            TCHAR DeviceName[MAX_DEV_LEN];
            INT Ret, DevIndex = 0;

            if (DevDesc[0] != _T('\0'))
            {
                hDevItem = InsertIntoTreeView(Info->hTreeView,
                                              hRoot,
                                              DevDesc,
                                              DevImage);
            }
            else
            {
                hDevItem = InsertIntoTreeView(Info->hTreeView,
                                              hRoot,
                                              DevName,
                                              DevImage);
            }

            do
            {
                Ret = EnumDevices(DevIndex,
                                  DevName,
                                  DeviceName);
                if (Ret == 0)
                {
                    InsertIntoTreeView(Info->hTreeView,
                                       hDevItem,
                                       DeviceName,
                                       DevImage);
                }

                DevIndex++;

            } while (Ret != -1);

            if (!TreeView_GetChild(Info->hTreeView,
                                   hDevItem))
            {
                (void)TreeView_DeleteItem(Info->hTreeView,
                                          hDevItem);
            }
            else
            {
                (void)TreeView_SortChildren(Info->hTreeView,
                                            hDevItem,
                                            0);
            }
        }

        index++;

    } while (ClassRet != -1);

    (void)TreeView_Expand(Info->hTreeView,
                          hRoot,
                          TVE_EXPAND);

    (void)TreeView_SortChildren(Info->hTreeView,
                                hRoot,
                                0);
}


HTREEITEM
InitTreeView(PMAIN_WND_INFO Info)
{
    HTREEITEM hRoot;
    HBITMAP hComp;
    TCHAR ComputerName[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    INT RootImage;

    (void)TreeView_DeleteAllItems(Info->hTreeView);

    /* get the device image List */
    ImageListData.cbSize = sizeof(ImageListData);
    SetupDiGetClassImageList(&ImageListData);

    hComp = LoadBitmap(hInstance,
                       MAKEINTRESOURCE(IDB_ROOT_IMAGE));

    ImageList_Add(ImageListData.ImageList,
                  hComp,
                  NULL);

    DeleteObject(hComp);

    (void)TreeView_SetImageList(Info->hTreeView,
                                ImageListData.ImageList,
                                TVSIL_NORMAL);

    if (!GetComputerName(ComputerName,
                         &dwSize))
    {
        ComputerName[0] = _T('\0');
    }

    RootImage = ImageList_GetImageCount(ImageListData.ImageList) - 1;

    /* insert the root item into the tree */
    hRoot = InsertIntoTreeView(Info->hTreeView,
                               NULL,
                               ComputerName,
                               RootImage);

    return hRoot;
}
