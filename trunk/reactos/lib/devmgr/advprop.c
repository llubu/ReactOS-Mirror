/*
 * ReactOS Device Manager Applet
 * Copyright (C) 2004 - 2005 ReactOS Team
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
/* $Id: hwpage.c 19599 2005-11-26 02:12:58Z weiden $
 *
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/advprop.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */
#include <precomp.h>

#define NDEBUG
#include <debug.h>

typedef INT_PTR (WINAPI *PPROPERTYSHEETW)(LPCPROPSHEETHEADERW);
typedef HPROPSHEETPAGE (WINAPI *PCREATEPROPERTYSHEETPAGEW)(LPCPROPSHEETPAGEW);
typedef BOOL (WINAPI *PDESTROYPROPERTYSHEETPAGE)(HPROPSHEETPAGE);

typedef struct _DEVADVPROP_INFO
{
    HWND hWndGeneralPage;
    HWND hWndParent;
    WNDPROC ParentOldWndProc;

    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    HDEVINFO CurrentDeviceInfoSet;
    SP_DEVINFO_DATA CurrentDeviceInfoData;
    DEVINST ParentDevInst;
    HMACHINE hMachine;
    LPCWSTR lpMachineName;

    HINSTANCE hComCtl32;

    DWORD PropertySheetType;
    DWORD nDevPropSheets;
    HPROPSHEETPAGE *DevPropSheets;

    BOOL FreeDevPropSheets : 1;
    BOOL CanDisable : 1;
    BOOL DeviceEnabled : 1;
    BOOL DeviceUsageChanged : 1;
    BOOL CloseDevInst : 1;
    BOOL IsAdmin : 1;
    BOOL DoDefaultDevAction : 1;

    WCHAR szDevName[255];
    WCHAR szTemp[255];
    WCHAR szDeviceID[1];
    /* struct may be dynamically expanded here! */
} DEVADVPROP_INFO, *PDEVADVPROP_INFO;


static VOID
InitDevUsageActions(IN HWND hwndDlg,
                    IN HWND hComboBox,
                    IN PDEVADVPROP_INFO dap)
{
    INT Index;
    UINT i;
    UINT Actions[] =
    {
        IDS_ENABLEDEVICE,
        IDS_DISABLEDEVICE,
    };

    for (i = 0;
         i != sizeof(Actions) / sizeof(Actions[0]);
         i++)
    {
        /* fill in the device usage combo box */
        if (LoadString(hDllInstance,
                       Actions[i],
                       dap->szTemp,
                       sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
        {
            Index = (INT)SendMessage(hComboBox,
                                     CB_ADDSTRING,
                                     0,
                                     (LPARAM)dap->szTemp);
            if (Index != CB_ERR)
            {
                SendMessage(hComboBox,
                            CB_SETITEMDATA,
                            (WPARAM)Index,
                            (LPARAM)Actions[i]);

                switch (Actions[i])
                {
                    case IDS_ENABLEDEVICE:
                        if (dap->DeviceEnabled)
                        {
                            SendMessage(hComboBox,
                                        CB_SETCURSEL,
                                        (WPARAM)Index,
                                        0);
                        }
                        break;

                    case IDS_DISABLEDEVICE:
                        if (!dap->DeviceEnabled)
                        {
                            SendMessage(hComboBox,
                                        CB_SETCURSEL,
                                        (WPARAM)Index,
                                        0);
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    }
}


static UINT
GetSelectedUsageAction(IN HWND hComboBox)
{
    INT Index;
    UINT Ret = 0;

    Index = (INT)SendMessage(hComboBox,
                             CB_GETCURSEL,
                             0,
                             0);
    if (Index != CB_ERR)
    {
        INT iRet = SendMessage(hComboBox,
                               CB_GETITEMDATA,
                               (WPARAM)Index,
                               0);
        if (iRet != CB_ERR)
        {
            Ret = (UINT)iRet;
        }
    }

    return Ret;
}


static BOOL
ApplyGeneralSettings(IN HWND hwndDlg,
                     IN PDEVADVPROP_INFO dap)
{
    BOOL Ret = FALSE;

    if (dap->DeviceUsageChanged && dap->IsAdmin)
    {
        UINT SelectedUsageAction;
        BOOL NeedReboot = FALSE;

        SelectedUsageAction = GetSelectedUsageAction(GetDlgItem(hwndDlg,
                                                                IDC_DEVUSAGE));
        switch (SelectedUsageAction)
        {
            case IDS_ENABLEDEVICE:
                if (!dap->DeviceEnabled)
                {
                    Ret = EnableDevice(dap->DeviceInfoSet,
                                       &dap->DeviceInfoData,
                                       TRUE,
                                       0,
                                       &NeedReboot);
                }
                break;

            case IDS_DISABLEDEVICE:
                if (dap->DeviceEnabled)
                {
                    Ret = EnableDevice(dap->DeviceInfoSet,
                                       &dap->DeviceInfoData,
                                       FALSE,
                                       0,
                                       &NeedReboot);
                }
                break;

            default:
                break;
        }

        if (Ret)
        {
            if (NeedReboot)
            {
                /* make PropertySheet() return PSM_REBOOTSYSTEM */
                PropSheet_RebootSystem(hwndDlg);
            }
        }
        else
        {
            /* FIXME - display an error message */
            DPRINT1("Failed to enable/disable device! LastError: %d\n",
                    GetLastError());
        }
    }
    else
        Ret = !dap->DeviceUsageChanged;

    /* disable the apply button */
    PropSheet_UnChanged(GetParent(hwndDlg),
                        hwndDlg);
    dap->DeviceUsageChanged = FALSE;
    return Ret;
}


static VOID
UpdateDevInfo(IN HWND hwndDlg,
              IN PDEVADVPROP_INFO dap,
              IN BOOL ReOpen)
{
    HICON hIcon;
    HWND hDevUsage, hPropSheetDlg, hDevProbBtn;
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    UINT TroubleShootStrId = IDS_TROUBLESHOOTDEV;
    BOOL bFlag;
    DWORD i;
    HDEVINFO DeviceInfoSet = NULL;
    PSP_DEVINFO_DATA DeviceInfoData = NULL;

    hPropSheetDlg = GetParent(hwndDlg);

    if (ReOpen)
    {
        PROPSHEETHEADER psh;

        /* switch to the General page */
        PropSheet_SetCurSelByID(hPropSheetDlg,
                                IDD_DEVICEGENERAL);

        /* remove and destroy the existing device property sheet pages */
        for (i = 0;
             i != dap->nDevPropSheets;
             i++)
        {
            PropSheet_RemovePage(hPropSheetDlg,
                                 -1,
                                 dap->DevPropSheets[i]);
        }

        if (dap->FreeDevPropSheets)
        {
            /* don't free the array if it's the one allocated in
               DisplayDeviceAdvancedProperties */
            HeapFree(GetProcessHeap(),
                     0,
                     dap->DevPropSheets);

            dap->FreeDevPropSheets = FALSE;
        }

        dap->DevPropSheets = NULL;
        dap->nDevPropSheets = 0;

        /* create a new device info set and re-open the device */
        if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            SetupDiDestroyDeviceInfoList(dap->CurrentDeviceInfoSet);
        }

        dap->ParentDevInst = 0;
        dap->CurrentDeviceInfoSet = SetupDiCreateDeviceInfoListEx(NULL,
                                                                  hwndDlg,
                                                                  dap->lpMachineName,
                                                                  NULL);
        if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            if (SetupDiOpenDeviceInfo(dap->CurrentDeviceInfoSet,
                                      dap->szDeviceID,
                                      hwndDlg,
                                      0,
                                      &dap->CurrentDeviceInfoData))
            {
                if (dap->CloseDevInst)
                {
                    SetupDiDestroyDeviceInfoList(dap->DeviceInfoSet);
                }

                dap->CloseDevInst = TRUE;
                dap->DeviceInfoSet = dap->CurrentDeviceInfoSet;
                dap->DeviceInfoData = dap->CurrentDeviceInfoData;
                dap->CurrentDeviceInfoSet = INVALID_HANDLE_VALUE;
            }
            else
                goto GetParentNode;
        }
        else
        {
GetParentNode:
            /* get the parent node from the initial devinst */
            CM_Get_Parent_Ex(&dap->ParentDevInst,
                             dap->DeviceInfoData.DevInst,
                             0,
                             dap->hMachine);
        }

        if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            DeviceInfoSet = dap->CurrentDeviceInfoSet;
            DeviceInfoData = &dap->CurrentDeviceInfoData;
        }
        else
        {
            DeviceInfoSet = dap->DeviceInfoSet;
            DeviceInfoData = &dap->DeviceInfoData;
        }

        /* find out how many new device property sheets to add.
           fake a PROPSHEETHEADER structure, we don't plan to
           call PropertySheet again!*/
        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = 0;
        psh.nPages = 0;

        if (!SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                              DeviceInfoData,
                                              &psh,
                                              0,
                                              &dap->nDevPropSheets,
                                              dap->PropertySheetType) &&
            dap->nDevPropSheets != 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            dap->DevPropSheets = HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           dap->nDevPropSheets * sizeof(HPROPSHEETPAGE));
            if (dap->DevPropSheets != NULL)
            {
                psh.phpage = dap->DevPropSheets;

                /* query the new property sheet pages to add */
                if (SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                                     DeviceInfoData,
                                                     &psh,
                                                     dap->nDevPropSheets,
                                                     NULL,
                                                     dap->PropertySheetType))
                {
                    /* add the property sheets */

                    for (i = 0;
                         i != dap->nDevPropSheets;
                         i++)
                    {
                        PropSheet_AddPage(hPropSheetDlg,
                                          dap->DevPropSheets[i]);
                    }

                    dap->FreeDevPropSheets = TRUE;
                }
                else
                {
                    /* cleanup, we were unable to get the device property sheets */
                    HeapFree(GetProcessHeap(),
                             0,
                             dap->DevPropSheets);

                    dap->nDevPropSheets = 0;
                    dap->DevPropSheets = NULL;
                }
            }
            else
                dap->nDevPropSheets = 0;
        }
    }
    else
    {
        if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            DeviceInfoSet = dap->CurrentDeviceInfoSet;
            DeviceInfoData = &dap->CurrentDeviceInfoData;
        }
        else
        {
            DeviceInfoSet = dap->DeviceInfoSet;
            DeviceInfoData = &dap->DeviceInfoData;
        }
    }

    /* get the device name */
    if (GetDeviceDescriptionString(DeviceInfoSet,
                                   DeviceInfoData,
                                   dap->szDevName,
                                   sizeof(dap->szDevName) / sizeof(dap->szDevName[0])))
    {
        PropSheet_SetTitle(GetParent(hwndDlg),
                           PSH_PROPTITLE,
                           dap->szDevName);
    }

    /* set the device image */
    if (SetupDiLoadClassIcon(&DeviceInfoData->ClassGuid,
                             &hIcon,
                             NULL))
    {
        HICON hOldIcon = (HICON)SendDlgItemMessage(hwndDlg,
                                                   IDC_DEVICON,
                                                   STM_SETICON,
                                                   (WPARAM)hIcon,
                                                   0);
        if (hOldIcon != NULL)
        {
            DestroyIcon(hOldIcon);
        }
    }

    /* set the device name edit control text */
    SetDlgItemText(hwndDlg,
                   IDC_DEVNAME,
                   dap->szDevName);

    /* set the device type edit control text */
    if (GetDeviceTypeString(DeviceInfoData,
                            dap->szTemp,
                            sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVTYPE,
                       dap->szTemp);
    }

    /* set the device manufacturer edit control text */
    if (GetDeviceManufacturerString(DeviceInfoSet,
                                    DeviceInfoData,
                                    dap->szTemp,
                                    sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVMANUFACTURER,
                       dap->szTemp);
    }

    /* set the device location edit control text */
    if (GetDeviceLocationString(DeviceInfoData->DevInst,
                                dap->ParentDevInst,
                                dap->szTemp,
                                sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVLOCATION,
                       dap->szTemp);
    }

    /* set the device status edit control text */
    if (GetDeviceStatusString(DeviceInfoData->DevInst,
                              dap->hMachine,
                              dap->szTemp,
                              sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVSTATUS,
                       dap->szTemp);
    }

    /* set the device troubleshoot button text and disable it if necessary */
    hDevProbBtn = GetDlgItem(hwndDlg,
                             IDC_DEVPROBLEM);
    EnableWindow(hDevProbBtn,
                 dap->IsAdmin);
    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DeviceInfoData->DevInst,
                                  0,
                                  dap->hMachine);
    if (cr == CR_SUCCESS && (Status & DN_HAS_PROBLEM))
    {
        switch (ProblemNumber)
        {
            case CM_PROB_DISABLED:
                TroubleShootStrId = IDS_ENABLEDEV;
                break;

            case CM_PROB_FAILED_INSTALL:
                TroubleShootStrId = IDS_REINSTALLDRV;
                break;
        }
    }

    if (LoadString(hDllInstance,
                   TroubleShootStrId,
                   dap->szTemp,
                   sizeof(dap->szTemp) / sizeof(dap->szTemp[0])) != 0)
    {
        SetWindowText(hDevProbBtn,
                      dap->szTemp);
    }

    /* check if the device can be enabled/disabled */
    hDevUsage = GetDlgItem(hwndDlg,
                           IDC_DEVUSAGE);

    dap->CanDisable = FALSE;
    dap->DeviceEnabled = FALSE;

    if (CanDisableDevice(DeviceInfoData->DevInst,
                         dap->hMachine,
                         &bFlag))
    {
        dap->CanDisable = bFlag;
    }

    if (IsDeviceEnabled(DeviceInfoData->DevInst,
                        dap->hMachine,
                        &bFlag))
    {
        dap->DeviceEnabled = bFlag;
    }

    /* enable/disable the device usage controls */
    EnableWindow(GetDlgItem(hwndDlg,
                            IDC_DEVUSAGELABEL),
                 dap->CanDisable && dap->IsAdmin);
    EnableWindow(hDevUsage,
                 dap->CanDisable && dap->IsAdmin);

    /* clear the combobox */
    SendMessage(hDevUsage,
                CB_RESETCONTENT,
                0,
                0);
    if (dap->CanDisable)
    {
        InitDevUsageActions(hwndDlg,
                            hDevUsage,
                            dap);
    }

    /* finally, disable the apply button */
    PropSheet_UnChanged(hPropSheetDlg,
                        hwndDlg);
    dap->DeviceUsageChanged = FALSE;
}


static LRESULT
CALLBACK
DlgParentSubWndProc(IN HWND hwnd,
                    IN UINT uMsg,
                    IN WPARAM wParam,
                    IN LPARAM lParam)
{
    PDEVADVPROP_INFO dap;

    dap = (PDEVADVPROP_INFO)GetProp(hwnd,
                                    L"DevMgrDevChangeSub");
    if (dap != NULL)
    {
        if (uMsg == WM_DEVICECHANGE && !IsWindowVisible(dap->hWndGeneralPage))
        {
            SendMessage(dap->hWndGeneralPage,
                        WM_DEVICECHANGE,
                        wParam,
                        lParam);
        }

        /* pass the message the the old window proc */
        return CallWindowProc(dap->ParentOldWndProc,
                              hwnd,
                              uMsg,
                              wParam,
                              lParam);
    }
    else
    {
        /* this is not a good idea if the subclassed window was an ansi
           window, but we failed finding out the previous window proc
           so we can't use CallWindowProc. This should rarely - if ever -
           happen. */

        return DefWindowProc(hwnd,
                             uMsg,
                             wParam,
                             lParam);
    }
}


static INT_PTR
CALLBACK
AdvPropGeneralDlgProc(IN HWND hwndDlg,
                      IN UINT uMsg,
                      IN WPARAM wParam,
                      IN LPARAM lParam)
{
    PDEVADVPROP_INFO dap;
    INT_PTR Ret = FALSE;

    dap = (PDEVADVPROP_INFO)GetWindowLongPtr(hwndDlg,
                                             DWL_USER);

    if (dap != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDC_DEVUSAGE:
                    {
                        if (HIWORD(wParam) == CBN_SELCHANGE)
                        {
                            PropSheet_Changed(GetParent(hwndDlg),
                                              hwndDlg);
                            dap->DeviceUsageChanged = TRUE;
                        }
                        break;
                    }

                    case IDC_DEVPROBLEM:
                    {
                        if (dap->IsAdmin)
                        {
                            /* display the device problem wizard */
                            ShowDeviceProblemWizard(hwndDlg,
                                                    dap->DeviceInfoSet,
                                                    &dap->DeviceInfoData,
                                                    dap->hMachine);
                        }
                        break;
                    }
                }
                break;
            }

            case WM_NOTIFY:
            {
                NMHDR *hdr = (NMHDR*)lParam;
                switch (hdr->code)
                {
                    case PSN_APPLY:
                        ApplyGeneralSettings(hwndDlg,
                                             dap);
                        break;
                }
                break;
            }

            case WM_INITDIALOG:
            {
                dap = (PDEVADVPROP_INFO)((LPPROPSHEETPAGE)lParam)->lParam;
                if (dap != NULL)
                {
                    HWND hWndParent;

                    dap->hWndGeneralPage = hwndDlg;

                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)dap);

                    /* subclass the parent window to always receive
                       WM_DEVICECHANGE messages */
                    hWndParent = GetParent(hwndDlg);
                    if (hWndParent != NULL)
                    {
                        /* subclass the parent window. This is not safe
                           if the parent window belongs to another thread! */
                        dap->ParentOldWndProc = (WNDPROC)SetWindowLongPtr(hWndParent,
                                                                          GWLP_WNDPROC,
                                                                          (LONG_PTR)DlgParentSubWndProc);

                        if (dap->ParentOldWndProc != NULL &&
                            SetProp(hWndParent,
                                    L"DevMgrDevChangeSub",
                                    (HANDLE)dap))
                        {
                            dap->hWndParent = hWndParent;
                        }
                    }

                    UpdateDevInfo(hwndDlg,
                                  dap,
                                  FALSE);
                }
                Ret = TRUE;
                break;
            }

            case WM_DEVICECHANGE:
            {
                /* FIXME - don't call UpdateDevInfo for all events */
                UpdateDevInfo(hwndDlg,
                              dap,
                              TRUE);
                Ret = TRUE;
                break;
            }

            case WM_DESTROY:
            {
                HICON hDevIcon;

                /* restore the old window proc of the subclassed parent window */
                if (dap->hWndParent != NULL && dap->ParentOldWndProc != NULL)
                {
                    SetWindowLongPtr(dap->hWndParent,
                                     GWLP_WNDPROC,
                                     (LONG_PTR)dap->ParentOldWndProc);
                }

                /* destroy the device icon */
                hDevIcon = (HICON)SendDlgItemMessage(hwndDlg,
                                                     IDC_DEVICON,
                                                     STM_GETICON,
                                                     0,
                                                     0);
                if (hDevIcon != NULL)
                {
                    DestroyIcon(hDevIcon);
                }
                break;
            }
        }
    }

    return Ret;
}


INT_PTR
DisplayDeviceAdvancedProperties(IN HWND hWndParent,
                                IN LPCWSTR lpDeviceID  OPTIONAL,
                                IN HDEVINFO DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData,
                                IN HINSTANCE hComCtl32,
                                IN LPCWSTR lpMachineName,
                                IN DWORD dwFlags)
{
    PROPSHEETHEADER psh = {0};
    PROPSHEETPAGE pspGeneral = {0};
    DWORD nPropSheets = 0;
    PPROPERTYSHEETW pPropertySheetW;
    PCREATEPROPERTYSHEETPAGEW pCreatePropertySheetPageW;
    PDESTROYPROPERTYSHEETPAGE pDestroyPropertySheetPage;
    PDEVADVPROP_INFO DevAdvPropInfo;
    HMACHINE hMachine = NULL;
    DWORD DevIdSize = 0;
    INT_PTR Ret = -1;

    /* we don't want to statically link against comctl32, so find the
       functions we need dynamically */
    pPropertySheetW =
        (PPROPERTYSHEETW)GetProcAddress(hComCtl32,
                                        "PropertySheetW");
    pCreatePropertySheetPageW =
        (PCREATEPROPERTYSHEETPAGEW)GetProcAddress(hComCtl32,
                                                  "CreatePropertySheetPageW");
    pDestroyPropertySheetPage =
        (PDESTROYPROPERTYSHEETPAGE)GetProcAddress(hComCtl32,
                                                  "DestroyPropertySheetPage");
    if (pPropertySheetW == NULL ||
        pCreatePropertySheetPageW == NULL ||
        pDestroyPropertySheetPage == NULL)
    {
        return -1;
    }

    if (lpDeviceID == NULL)
    {
        /* find out how much size is needed for the device id */
        if (SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                       DeviceInfoData,
                                       NULL,
                                       0,
                                       &DevIdSize))
        {
            DPRINT1("SetupDiGetDeviceInstanceId unexpectedly returned TRUE!\n");
            return -1;
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return -1;
        }
    }
    else
    {
        DevIdSize = (DWORD)wcslen(lpDeviceID) + 1;
    }

    if (lpMachineName != NULL)
    {
        CONFIGRET cr = CM_Connect_Machine(lpMachineName,
                                          &hMachine);
        if (cr != CR_SUCCESS)
        {
            return -1;
        }
    }

    /* create the internal structure associated with the "General",
       "Driver", ... pages */
    DevAdvPropInfo = HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               FIELD_OFFSET(DEVADVPROP_INFO,
                                            szDeviceID) +
                                   (DevIdSize * sizeof(WCHAR)));
    if (DevAdvPropInfo == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    if (lpDeviceID == NULL)
    {
        /* read the device instance id */
        if (!SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                        DeviceInfoData,
                                        DevAdvPropInfo->szDeviceID,
                                        DevIdSize,
                                        NULL))
        {
            goto Cleanup;
        }
    }
    else
    {
        /* copy the device instance id supplied by the caller */
        wcscpy(DevAdvPropInfo->szDeviceID,
               lpDeviceID);
    }

    DevAdvPropInfo->DeviceInfoSet = DeviceInfoSet;
    DevAdvPropInfo->DeviceInfoData = *DeviceInfoData;
    DevAdvPropInfo->CurrentDeviceInfoSet = INVALID_HANDLE_VALUE;
    DevAdvPropInfo->CurrentDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    DevAdvPropInfo->hMachine = hMachine;
    DevAdvPropInfo->lpMachineName = lpMachineName;
    DevAdvPropInfo->szDevName[0] = L'\0';
    DevAdvPropInfo->hComCtl32 = hComCtl32;

    DevAdvPropInfo->IsAdmin = IsUserAdmin();
    DevAdvPropInfo->DoDefaultDevAction = ((dwFlags & DPF_DEVICE_STATUS_ACTION) != 0);

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    psh.hwndParent = hWndParent;
    psh.pszCaption = DevAdvPropInfo->szDevName;

    DevAdvPropInfo->PropertySheetType = lpMachineName != NULL ?
                                            DIGCDP_FLAG_REMOTE_ADVANCED :
                                            DIGCDP_FLAG_ADVANCED;

    /* find out how many property sheets we need */
    if (SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                         &DevAdvPropInfo->DeviceInfoData,
                                         &psh,
                                         0,
                                         &nPropSheets,
                                         DevAdvPropInfo->PropertySheetType) &&
        nPropSheets != 0)
    {
        DPRINT1("SetupDiGetClassDevPropertySheets unexpectedly returned TRUE!\n");
        goto Cleanup;
    }

    if (nPropSheets != 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        goto Cleanup;
    }

    psh.phpage = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           (nPropSheets + 1) * sizeof(HPROPSHEETPAGE));
    if (psh.phpage == NULL)
    {
        goto Cleanup;
    }

    /* add the "General" property sheet */
    pspGeneral.dwSize = sizeof(PROPSHEETPAGE);
    pspGeneral.dwFlags = PSP_DEFAULT;
    pspGeneral.hInstance = hDllInstance;
    pspGeneral.pszTemplate = (LPCWSTR)MAKEINTRESOURCE(IDD_DEVICEGENERAL);
    pspGeneral.pfnDlgProc = AdvPropGeneralDlgProc;
    pspGeneral.lParam = (LPARAM)DevAdvPropInfo;
    psh.phpage[0] = pCreatePropertySheetPageW(&pspGeneral);
    if (psh.phpage[0] != NULL)
    {
        psh.nPages++;
    }

    DevAdvPropInfo->nDevPropSheets = nPropSheets;

    if (nPropSheets != 0)
    {
        DevAdvPropInfo->DevPropSheets = psh.phpage + psh.nPages;

        /* create the device property sheets */
        if (!SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                              &DevAdvPropInfo->DeviceInfoData,
                                              &psh,
                                              nPropSheets + psh.nPages,
                                              NULL,
                                              DevAdvPropInfo->PropertySheetType))
        {
            goto Cleanup;
        }
    }

    /* FIXME - add the "Driver" property sheet if necessary */

    if (psh.nPages != 0)
    {
        Ret = pPropertySheetW(&psh);

        /* NOTE: no need to destroy the property sheets anymore! */
    }
    else
    {
        UINT i;

Cleanup:
        /* in case of failure the property sheets must be destroyed */
        if (psh.phpage != NULL)
        {
            for (i = 0;
                 i < psh.nPages;
                 i++)
            {
                if (psh.phpage[i] != NULL)
                {
                    pDestroyPropertySheetPage(psh.phpage[i]);
                }
            }
        }
    }

    if (DevAdvPropInfo != NULL)
    {
        if (DevAdvPropInfo->FreeDevPropSheets)
        {
            /* don't free the array if it's the one allocated in
               DisplayDeviceAdvancedProperties */
            HeapFree(GetProcessHeap(),
                     0,
                     DevAdvPropInfo->DevPropSheets);
        }

        if (DevAdvPropInfo->CloseDevInst)
        {
            /* close the device info set in case a new one was created */
            SetupDiDestroyDeviceInfoList(DevAdvPropInfo->DeviceInfoSet);
        }

        if (DevAdvPropInfo->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            SetupDiDestroyDeviceInfoList(DevAdvPropInfo->CurrentDeviceInfoSet);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 DevAdvPropInfo);
    }

    if (psh.phpage != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 psh.phpage);
    }

    if (hMachine != NULL)
    {
        CM_Disconnect_Machine(hMachine);
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceAdvancedPropertiesW
 *
 * DESCRIPTION
 *   Invokes the device properties dialog, this version may add some property pages
 *   for some devices
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *
 * RETURN VALUE
 *   Always returns -1, a call to GetLastError returns 0 if successful
 *
 * @implemented
 */
INT_PTR
WINAPI
DeviceAdvancedPropertiesW(IN HWND hWndParent  OPTIONAL,
                          IN LPCWSTR lpMachineName  OPTIONAL,
                          IN LPCWSTR lpDeviceID)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    HINSTANCE hComCtl32;
    INT_PTR Ret = -1;

    if (lpDeviceID == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* dynamically load comctl32 */
    hComCtl32 = LoadAndInitComctl32();
    if (hComCtl32 != NULL)
    {
        hDevInfo = SetupDiCreateDeviceInfoListEx(NULL,
                                                 hWndParent,
                                                 lpMachineName,
                                                 NULL);
        if (hDevInfo != INVALID_HANDLE_VALUE)
        {
            DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (SetupDiOpenDeviceInfo(hDevInfo,
                                      lpDeviceID,
                                      hWndParent,
                                      0,
                                      &DevInfoData))
            {
                Ret = DisplayDeviceAdvancedProperties(hWndParent,
                                                      lpDeviceID,
                                                      hDevInfo,
                                                      &DevInfoData,
                                                      hComCtl32,
                                                      lpMachineName,
                                                      0);
            }

            SetupDiDestroyDeviceInfoList(hDevInfo);
        }

        FreeLibrary(hComCtl32);
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceAdvancedPropertiesA
 *
 * DESCRIPTION
 *   Invokes the device properties dialog, this version may add some property pages
 *   for some devices
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *
 * RETURN VALUE
 *   Always returns -1, a call to GetLastError returns 0 if successful
 *
 * @implemented
 */
INT_PTR
WINAPI
DeviceAdvancedPropertiesA(IN HWND hWndParent  OPTIONAL,
                          IN LPCSTR lpMachineName  OPTIONAL,
                          IN LPCSTR lpDeviceID)
{
    LPWSTR lpMachineNameW = NULL;
    LPWSTR lpDeviceIDW = NULL;
    INT_PTR Ret = -1;

    if (lpMachineName != NULL)
    {
        if (!(lpMachineNameW = ConvertMultiByteToUnicode(lpMachineName,
                                                         CP_ACP)))
        {
            goto Cleanup;
        }
    }
    if (lpDeviceID != NULL)
    {
        if (!(lpDeviceIDW = ConvertMultiByteToUnicode(lpDeviceID,
                                                      CP_ACP)))
        {
            goto Cleanup;
        }
    }

    Ret = DeviceAdvancedPropertiesW(hWndParent,
                                    lpMachineNameW,
                                    lpDeviceIDW);

Cleanup:
    if (lpMachineNameW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpMachineNameW);
    }
    if (lpDeviceIDW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceIDW);
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DevicePropertiesExA
 *
 * DESCRIPTION
 *   Invokes the extended device properties dialog
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown, optional if
 *                  bShowDevMgr is nonzero
 *   dwFlags:       This parameter can be a combination of the following flags:
 *                  * DPF_DEVICE_STATUS_ACTION: Only valid if bShowDevMgr, causes
 *                                              the default device status action button
 *                                              to be clicked (Troubleshoot, Enable
 *                                              Device, etc)
 *   bShowDevMgr:   If non-zero it displays the device manager instead of
 *                  the advanced device property dialog
 *
 * RETURN VALUE
 *   1:  if bShowDevMgr is non-zero and no error occured
 *   -1: a call to GetLastError returns 0 if successful
 *
 * @implemented
 */
INT_PTR
WINAPI
DevicePropertiesExA(IN HWND hWndParent  OPTIONAL,
                    IN LPCSTR lpMachineName  OPTIONAL,
                    IN LPCSTR lpDeviceID  OPTIONAL,
                    IN DWORD dwFlags  OPTIONAL,
                    IN BOOL bShowDevMgr)
{
    LPWSTR lpMachineNameW = NULL;
    LPWSTR lpDeviceIDW = NULL;
    INT_PTR Ret = -1;

    if (lpMachineName != NULL)
    {
        if (!(lpMachineNameW = ConvertMultiByteToUnicode(lpMachineName,
                                                         CP_ACP)))
        {
            goto Cleanup;
        }
    }
    if (lpDeviceID != NULL)
    {
        if (!(lpDeviceIDW = ConvertMultiByteToUnicode(lpDeviceID,
                                                      CP_ACP)))
        {
            goto Cleanup;
        }
    }

    Ret = DevicePropertiesExW(hWndParent,
                              lpMachineNameW,
                              lpDeviceIDW,
                              dwFlags,
                              bShowDevMgr);

Cleanup:
    if (lpMachineNameW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpMachineNameW);
    }
    if (lpDeviceIDW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceIDW);
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DevicePropertiesExW
 *
 * DESCRIPTION
 *   Invokes the extended device properties dialog
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown, optional if
 *                  bShowDevMgr is nonzero
 *   dwFlags:       This parameter can be a combination of the following flags:
 *                  * DPF_DEVICE_STATUS_ACTION: Only valid if bShowDevMgr, causes
 *                                              the default device status action button
 *                                              to be clicked (Troubleshoot, Enable
 *                                              Device, etc)
 *   bShowDevMgr:   If non-zero it displays the device manager instead of
 *                  the advanced device property dialog
 *
 * RETURN VALUE
 *   1:  if bShowDevMgr is non-zero and no error occured
 *   -1: a call to GetLastError returns 0 if successful
 *
 * @unimplemented
 */
INT_PTR
WINAPI
DevicePropertiesExW(IN HWND hWndParent  OPTIONAL,
                    IN LPCWSTR lpMachineName  OPTIONAL,
                    IN LPCWSTR lpDeviceID  OPTIONAL,
                    IN DWORD dwFlags  OPTIONAL,
                    IN BOOL bShowDevMgr)
{
    INT_PTR Ret = -1;

    if (dwFlags & ~(DPF_UNKNOWN | DPF_DEVICE_STATUS_ACTION))
    {
        DPRINT1("DevPropertiesExW: Invalid flags: 0x%x\n",
                dwFlags & ~(DPF_UNKNOWN | DPF_DEVICE_STATUS_ACTION));
        SetLastError(ERROR_INVALID_FLAGS);
        return -1;
    }

    if (bShowDevMgr)
    {
        DPRINT("DevPropertiesExW doesn't support bShowDevMgr!\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
    else
    {
        HDEVINFO hDevInfo;
        SP_DEVINFO_DATA DevInfoData;
        HINSTANCE hComCtl32;

        if (lpDeviceID == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return -1;
        }

        /* dynamically load comctl32 */
        hComCtl32 = LoadAndInitComctl32();
        if (hComCtl32 != NULL)
        {
            hDevInfo = SetupDiCreateDeviceInfoListEx(NULL,
                                                     hWndParent,
                                                     lpMachineName,
                                                     NULL);
            if (hDevInfo != INVALID_HANDLE_VALUE)
            {
                DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                if (SetupDiOpenDeviceInfo(hDevInfo,
                                          lpDeviceID,
                                          hWndParent,
                                          0,
                                          &DevInfoData))
                {
                    Ret = DisplayDeviceAdvancedProperties(hWndParent,
                                                          lpDeviceID,
                                                          hDevInfo,
                                                          &DevInfoData,
                                                          hComCtl32,
                                                          lpMachineName,
                                                          dwFlags);
                }

                SetupDiDestroyDeviceInfoList(hDevInfo);
            }

            FreeLibrary(hComCtl32);
        }
    }

    return Ret;
}
