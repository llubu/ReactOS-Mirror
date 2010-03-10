/*
 * DirectShow MCI Driver
 *
 * Copyright 2009 Christian Costa
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "mciqtz_private.h"
#include "digitalv.h"
#include "wownt32.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciqtz);

static DWORD MCIQTZ_mciClose(UINT, DWORD, LPMCI_GENERIC_PARMS);
static DWORD MCIQTZ_mciStop(UINT, DWORD, LPMCI_GENERIC_PARMS);

/*======================================================================*
 *                          MCI QTZ implementation                      *
 *======================================================================*/

HINSTANCE MCIQTZ_hInstance = 0;

/***********************************************************************
 *              DllMain (MCIQTZ.0)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        MCIQTZ_hInstance = hInstDLL;
        break;
    }
    return TRUE;
}

/**************************************************************************
 *                              MCIQTZ_mciGetOpenDev            [internal]
 */
static WINE_MCIQTZ* MCIQTZ_mciGetOpenDev(UINT wDevID)
{
    WINE_MCIQTZ* wma = (WINE_MCIQTZ*)mciGetDriverData(wDevID);

    if (!wma) {
        WARN("Invalid wDevID=%u\n", wDevID);
        return NULL;
    }
    return wma;
}

/**************************************************************************
 *                              MCIQTZ_drvOpen                  [internal]
 */
static DWORD MCIQTZ_drvOpen(LPCWSTR str, LPMCI_OPEN_DRIVER_PARMSW modp)
{
    WINE_MCIQTZ* wma;

    TRACE("(%s, %p)\n", debugstr_w(str), modp);

    /* session instance */
    if (!modp)
        return 0xFFFFFFFF;

    wma = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCIQTZ));
    if (!wma)
        return 0;

    wma->wDevID = modp->wDeviceID;
    mciSetDriverData(wma->wDevID, (DWORD_PTR)wma);

    return modp->wDeviceID;
}

/**************************************************************************
 *                              MCIQTZ_drvClose         [internal]
 */
static DWORD MCIQTZ_drvClose(DWORD dwDevID)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x)\n", dwDevID);

    wma = MCIQTZ_mciGetOpenDev(dwDevID);

    if (wma) {
        /* finish all outstanding things */
        MCIQTZ_mciClose(dwDevID, MCI_WAIT, NULL);

        mciSetDriverData(dwDevID, 0);
        HeapFree(GetProcessHeap(), 0, wma);
        return 1;
    }

    return (dwDevID == 0xFFFFFFFF) ? 1 : 0;
}

/**************************************************************************
 *                              MCIQTZ_drvConfigure             [internal]
 */
static DWORD MCIQTZ_drvConfigure(DWORD dwDevID)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x)\n", dwDevID);

    wma = MCIQTZ_mciGetOpenDev(dwDevID);
    if (!wma)
        return 0;

    MCIQTZ_mciStop(dwDevID, MCI_WAIT, NULL);

    MessageBoxA(0, "Sample QTZ Wine Driver !", "MM-Wine Driver", MB_OK);

    return 1;
}

/***************************************************************************
 *                              MCIQTZ_mciOpen                  [internal]
 */
static DWORD MCIQTZ_mciOpen(UINT wDevID, DWORD dwFlags,
                            LPMCI_DGV_OPEN_PARMSW lpOpenParms)
{
    WINE_MCIQTZ* wma;
    HRESULT hr;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpOpenParms);

    if (!lpOpenParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    MCIQTZ_mciStop(wDevID, MCI_WAIT, NULL);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (LPVOID*)&wma->pgraph);
    if (FAILED(hr)) {
        TRACE("Cannot create filtergraph (hr = %x)\n", hr);
        goto err;
    }

    hr = IGraphBuilder_QueryInterface(wma->pgraph, &IID_IMediaControl, (LPVOID*)&wma->pmctrl);
    if (FAILED(hr)) {
        TRACE("Cannot get IMediaControl interface (hr = %x)\n", hr);
        goto err;
    }

    if (!(dwFlags & MCI_OPEN_ELEMENT) || (dwFlags & MCI_OPEN_ELEMENT_ID)) {
        TRACE("Wrong dwFlags %x\n", dwFlags);
        goto err;
    }

    if (!lpOpenParms->lpstrElementName || !lpOpenParms->lpstrElementName[0]) {
        TRACE("Invalid filename specified\n");
        goto err;
    }

    TRACE("Open file %s\n", debugstr_w(lpOpenParms->lpstrElementName));

    hr = IGraphBuilder_RenderFile(wma->pgraph, lpOpenParms->lpstrElementName, NULL);
    if (FAILED(hr)) {
        TRACE("Cannot render file (hr = %x)\n", hr);
        goto err;
    }

    wma->opened = TRUE;

    return 0;

err:
    if (wma->pgraph)
        IGraphBuilder_Release(wma->pgraph);
    wma->pgraph = NULL;
    if (wma->pmctrl)
        IMediaControl_Release(wma->pmctrl);
    wma->pmctrl = NULL;

    CoUninitialize();

    return MCIERR_INTERNAL;
}

/***************************************************************************
 *                              MCIQTZ_mciClose                 [internal]
 */
static DWORD MCIQTZ_mciClose(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    MCIQTZ_mciStop(wDevID, MCI_WAIT, NULL);

    if (wma->opened) {
        IGraphBuilder_Release(wma->pgraph);
        IMediaControl_Release(wma->pmctrl);
        CoUninitialize();
        wma->opened = FALSE;
    }

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciPlay                  [internal]
 */
static DWORD MCIQTZ_mciPlay(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    WINE_MCIQTZ* wma;
    HRESULT hr;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    hr = IMediaControl_Run(wma->pmctrl);
    if (FAILED(hr)) {
        TRACE("Cannot run filtergraph (hr = %x)\n", hr);
        return MCIERR_INTERNAL;
    }

    wma->started = TRUE;

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciSeek                  [internal]
 */
static DWORD MCIQTZ_mciSeek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    WINE_MCIQTZ* wma;
    HRESULT hr;
    IMediaPosition* pmpos;
    LONGLONG newpos;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    MCIQTZ_mciStop(wDevID, MCI_WAIT, NULL);

    if (dwFlags & MCI_SEEK_TO_START) {
        newpos = 0;
    } else if (dwFlags & MCI_SEEK_TO_END) {
        FIXME("MCI_SEEK_TO_END not implemented yet\n");
        return MCIERR_INTERNAL;
    } else if (dwFlags & MCI_TO) {
        FIXME("MCI_TO not implemented yet\n");
        return MCIERR_INTERNAL;
    } else {
        WARN("dwFlag doesn't tell where to seek to...\n");
        return MCIERR_MISSING_PARAMETER;
    }

    hr = IGraphBuilder_QueryInterface(wma->pgraph, &IID_IMediaPosition, (LPVOID*)&pmpos);
    if (FAILED(hr)) {
        FIXME("Cannot get IMediaPostion interface (hr = %x)\n", hr);
        return MCIERR_INTERNAL;
    }

    hr = IMediaPosition_put_CurrentPosition(pmpos, newpos);
    if (FAILED(hr)) {
        FIXME("Cannot set position (hr = %x)\n", hr);
        IMediaPosition_Release(pmpos);
        return MCIERR_INTERNAL;
    }

    IMediaPosition_Release(pmpos);

    if (dwFlags & MCI_NOTIFY)
        mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)), wDevID, MCI_NOTIFY_SUCCESSFUL);

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciStop                  [internal]
 */
static DWORD MCIQTZ_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIQTZ* wma;
    HRESULT hr;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    if (!wma->started)
        return 0;

    hr = IMediaControl_Stop(wma->pmctrl);
    if (FAILED(hr)) {
        TRACE("Cannot stop filtergraph (hr = %x)\n", hr);
        return MCIERR_INTERNAL;
    }

    wma->started = FALSE;

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciGetDevCaps            [internal]
 */
static DWORD MCIQTZ_mciGetDevCaps(UINT wDevID, DWORD dwFlags, LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    if (!(dwFlags & MCI_GETDEVCAPS_ITEM))
        return MCIERR_MISSING_PARAMETER;

    switch (lpParms->dwItem) {
        case MCI_GETDEVCAPS_CAN_RECORD:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            TRACE("MCI_GETDEVCAPS_CAN_RECORD = %08x\n", lpParms->dwReturn);
            break;
        case MCI_GETDEVCAPS_HAS_AUDIO:
            lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            TRACE("MCI_GETDEVCAPS_HAS_AUDIO = %08x\n", lpParms->dwReturn);
            break;
        case MCI_GETDEVCAPS_HAS_VIDEO:
            lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            TRACE("MCI_GETDEVCAPS_HAS_VIDEO = %08x\n", lpParms->dwReturn);
            break;
        case MCI_GETDEVCAPS_DEVICE_TYPE:
            lpParms->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_DIGITAL_VIDEO, MCI_DEVTYPE_DIGITAL_VIDEO);
            TRACE("MCI_GETDEVCAPS_DEVICE_TYPE = %08x\n", lpParms->dwReturn);
            break;
        case MCI_GETDEVCAPS_USES_FILES:
            lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            TRACE("MCI_GETDEVCAPS_USES_FILES = %08x\n", lpParms->dwReturn);
            break;
        case MCI_GETDEVCAPS_COMPOUND_DEVICE:
            lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            TRACE("MCI_GETDEVCAPS_COMPOUND_DEVICE = %08x\n", lpParms->dwReturn);
            break;
        case MCI_GETDEVCAPS_CAN_EJECT:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            TRACE("MCI_GETDEVCAPS_EJECT = %08x\n", lpParms->dwReturn);
            break;
        case MCI_GETDEVCAPS_CAN_PLAY:
            lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            TRACE("MCI_GETDEVCAPS_CAN_PLAY = %08x\n", lpParms->dwReturn);
            break;
        case MCI_GETDEVCAPS_CAN_SAVE:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            TRACE("MCI_GETDEVCAPS_CAN_SAVE = %08x\n", lpParms->dwReturn);
            break;
        case MCI_DGV_GETDEVCAPS_CAN_REVERSE:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            TRACE("MCI_DGV_GETDEVCAPS_CAN_REVERSE = %08x\n", lpParms->dwReturn);
            break;
        case MCI_DGV_GETDEVCAPS_CAN_STRETCH:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE); /* FIXME */
            TRACE("MCI_DGV_GETDEVCAPS_CAN_STRETCH = %08x\n", lpParms->dwReturn);
            break;
        case MCI_DGV_GETDEVCAPS_CAN_LOCK:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            TRACE("MCI_DGV_GETDEVCAPS_CAN_LOCK = %08x\n", lpParms->dwReturn);
            break;
        case MCI_DGV_GETDEVCAPS_CAN_FREEZE:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            TRACE("MCI_DGV_GETDEVCAPS_CAN_FREEZE = %08x\n", lpParms->dwReturn);
            break;
        case MCI_DGV_GETDEVCAPS_CAN_STR_IN:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            TRACE("MCI_DGV_GETDEVCAPS_CAN_STRETCH_INPUT = %08x\n", lpParms->dwReturn);
            break;
        case MCI_DGV_GETDEVCAPS_HAS_STILL:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
            TRACE("MCI_DGV_GETDEVCAPS_HAS_STILL = %08x\n", lpParms->dwReturn);
            break;
        case MCI_DGV_GETDEVCAPS_CAN_TEST:
            lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE); /* FIXME */
            TRACE("MCI_DGV_GETDEVCAPS_CAN_TEST = %08x\n", lpParms->dwReturn);
            break;
        case MCI_DGV_GETDEVCAPS_MAX_WINDOWS:
            lpParms->dwReturn = 1;
            TRACE("MCI_DGV_GETDEVCAPS_MAX_WINDOWS = %u\n", lpParms->dwReturn);
            return 0;
        default:
            WARN("Unknown capability %08x\n", lpParms->dwItem);
            /* Fall through */
        case MCI_DGV_GETDEVCAPS_MAXIMUM_RATE: /* unknown to w2k */
        case MCI_DGV_GETDEVCAPS_MINIMUM_RATE: /* unknown to w2k */
            return MCIERR_UNSUPPORTED_FUNCTION;
    }

    return MCI_RESOURCE_RETURNED;
}

/***************************************************************************
 *                              MCIQTZ_mciSet                   [internal]
 */
static DWORD MCIQTZ_mciSet(UINT wDevID, DWORD dwFlags, LPMCI_DGV_SET_PARMS lpParms)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_SET_TIME_FORMAT) {
        switch (lpParms->dwTimeFormat) {
            case MCI_FORMAT_MILLISECONDS:
                TRACE("MCI_SET_TIME_FORMAT = MCI_FORMAT_MILLISECONDS\n");
                wma->time_format = MCI_FORMAT_MILLISECONDS;
                break;
            case MCI_FORMAT_FRAMES:
                TRACE("MCI_SET_TIME_FORMAT = MCI_FORMAT_FRAMES\n");
                wma->time_format = MCI_FORMAT_FRAMES;
                break;
            default:
                WARN("Bad time format %u\n", lpParms->dwTimeFormat);
                return MCIERR_BAD_TIME_FORMAT;
        }
    }

    if (dwFlags & MCI_SET_DOOR_OPEN)
        FIXME("MCI_SET_DOOR_OPEN not implemented yet\n");
    if (dwFlags & MCI_SET_DOOR_CLOSED)
        FIXME("MCI_SET_DOOR_CLOSED not implemented yet\n");
    if (dwFlags & MCI_SET_AUDIO)
        FIXME("MCI_SET_AUDIO not implemented yet\n");
    if (dwFlags & MCI_SET_VIDEO)
        FIXME("MCI_SET_VIDEO not implemented yet\n");
    if (dwFlags & MCI_SET_ON)
        FIXME("MCI_SET_ON not implemented yet\n");
    if (dwFlags & MCI_SET_OFF)
        FIXME("MCI_SET_OFF not implemented yet\n");
    if (dwFlags & MCI_SET_AUDIO_LEFT)
        FIXME("MCI_SET_AUDIO_LEFT not implemented yet\n");
    if (dwFlags & MCI_SET_AUDIO_RIGHT)
        FIXME("MCI_SET_AUDIO_RIGHT not implemented yet\n");

    if (dwFlags & ~0x7f03 /* All MCI_SET flags mask */)
        ERR("Unknown flags %08x\n", dwFlags & ~0x7f03);

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciStatus                [internal]
 */
static DWORD MCIQTZ_mciStatus(UINT wDevID, DWORD dwFlags, LPMCI_DGV_STATUS_PARMSW lpParms)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    if (!(dwFlags & MCI_STATUS_ITEM)) {
        WARN("No status item specified\n");
        return MCIERR_UNRECOGNIZED_COMMAND;
    }

    switch (lpParms->dwItem) {
        case MCI_STATUS_LENGTH:
            FIXME("MCI_STATUS_LENGTH not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_POSITION:
        {
            HRESULT hr;
            REFTIME curpos;
            IMediaPosition* pmpos;

            hr = IGraphBuilder_QueryInterface(wma->pgraph, &IID_IMediaPosition, (LPVOID*)&pmpos);
            if (FAILED(hr)) {
                FIXME("Cannot get IMediaPostion interface (hr = %x)\n", hr);
                return MCIERR_INTERNAL;
            }

            hr = IMediaPosition_get_CurrentPosition(pmpos, &curpos);
            if (FAILED(hr)) {
                FIXME("Cannot get position (hr = %x)\n", hr);
                IMediaPosition_Release(pmpos);
                return MCIERR_INTERNAL;
            }

            IMediaPosition_Release(pmpos);
            lpParms->dwReturn = curpos / 10000;

            break;
        }
        case MCI_STATUS_NUMBER_OF_TRACKS:
            FIXME("MCI_STATUS_NUMBER_OF_TRACKS not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_MODE:
            FIXME("MCI_STATUS_MODE not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_MEDIA_PRESENT:
            FIXME("MCI_STATUS_MEDIA_PRESENT not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_TIME_FORMAT:
            FIXME("MCI_STATUS_TIME_FORMAT not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_READY:
            FIXME("MCI_STATUS_READY not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        case MCI_STATUS_CURRENT_TRACK:
            FIXME("MCI_STATUS_CURRENT_TRACK not implemented yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        default:
            FIXME("Unknown command %08X\n", lpParms->dwItem);
            return MCIERR_UNRECOGNIZED_COMMAND;
    }

    if (dwFlags & MCI_NOTIFY)
        mciDriverNotify(HWND_32(LOWORD(lpParms->dwCallback)), wDevID, MCI_NOTIFY_SUCCESSFUL);

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciWhere                 [internal]
 */
static DWORD MCIQTZ_mciWhere(UINT wDevID, DWORD dwFlags, LPMCI_DGV_RECT_PARMS lpParms)
{
    WINE_MCIQTZ* wma;
    IVideoWindow* pVideoWindow;
    HRESULT hr;
    HWND hWnd;
    RECT rc;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    /* Find if there is a video stream and get the display window */
    hr = IGraphBuilder_QueryInterface(wma->pgraph, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);
    if (FAILED(hr)) {
        ERR("Cannot get IVideoWindow interface (hr = %x)\n", hr);
        return MCIERR_INTERNAL;
    }

    hr = IVideoWindow_get_Owner(pVideoWindow, (OAHWND*)&hWnd);
    IVideoWindow_Release(pVideoWindow);
    if (FAILED(hr)) {
        TRACE("No video stream, returning no window error\n");
        return MCIERR_NO_WINDOW;
    }

    if (dwFlags & MCI_DGV_WHERE_SOURCE) {
        if (dwFlags & MCI_DGV_WHERE_MAX)
            FIXME("MCI_DGV_WHERE_SOURCE_MAX not supported yet\n");
        else
            FIXME("MCI_DGV_WHERE_SOURCE not supported yet\n");
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_DESTINATION) {
        if (dwFlags & MCI_DGV_WHERE_MAX) {
            GetClientRect(hWnd, &rc);
            TRACE("MCI_DGV_WHERE_DESTINATION_MAX %s\n", wine_dbgstr_rect(&rc));
        } else {
            FIXME("MCI_DGV_WHERE_DESTINATION not supported yet\n");
            return MCIERR_UNRECOGNIZED_COMMAND;
        }
    }
    if (dwFlags & MCI_DGV_WHERE_FRAME) {
        if (dwFlags & MCI_DGV_WHERE_MAX)
            FIXME("MCI_DGV_WHERE_FRAME_MAX not supported yet\n");
        else
            FIXME("MCI_DGV_WHERE_FRAME not supported yet\n");
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_VIDEO) {
        if (dwFlags & MCI_DGV_WHERE_MAX)
            FIXME("MCI_DGV_WHERE_VIDEO_MAX not supported yet\n");
        else
            FIXME("MCI_DGV_WHERE_VIDEO not supported yet\n");
        return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (dwFlags & MCI_DGV_WHERE_WINDOW) {
        if (dwFlags & MCI_DGV_WHERE_MAX) {
            GetWindowRect(GetDesktopWindow(), &rc);
            TRACE("MCI_DGV_WHERE_WINDOW_MAX %s\n", wine_dbgstr_rect(&rc));
        } else {
            GetWindowRect(hWnd, &rc);
            TRACE("MCI_DGV_WHERE_WINDOW %s\n", wine_dbgstr_rect(&rc));
        }
    }

    /* In MCI, RECT structure is used differently: rc.right = width & rc.bottom = height
     * So convert the normal RECT into a MCI RECT before returning */
    lpParms->rc.left = rc.left;
    lpParms->rc.top = rc.right;
    lpParms->rc.right = rc.right - rc.left;
    lpParms->rc.bottom = rc.bottom - rc.top;

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciSetAudio              [internal]
 */
static DWORD MCIQTZ_mciSetAudio(UINT wDevID, DWORD dwFlags, LPMCI_DGV_SETAUDIO_PARMSW lpParms)
{
    WINE_MCIQTZ *wma;

    FIXME("(%04x, %08x, %p) : stub\n", wDevID, dwFlags, lpParms);

    if (!lpParms)
        return MCIERR_NULL_PARAMETER_BLOCK;

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    MCIQTZ_mciStop(wDevID, MCI_WAIT, NULL);

    return 0;
}

/*======================================================================*
 *                          MCI QTZ entry points                        *
 *======================================================================*/

/**************************************************************************
 *                              DriverProc (MCIQTZ.@)
 */
LRESULT CALLBACK MCIQTZ_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                   LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lX, %p, %08X, %08lX, %08lX)\n",
          dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
        case DRV_LOAD:                  return 1;
        case DRV_FREE:                  return 1;
        case DRV_OPEN:                  return MCIQTZ_drvOpen((LPCWSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSW)dwParam2);
        case DRV_CLOSE:                 return MCIQTZ_drvClose(dwDevID);
        case DRV_ENABLE:                return 1;
        case DRV_DISABLE:               return 1;
        case DRV_QUERYCONFIGURE:        return 1;
        case DRV_CONFIGURE:             return MCIQTZ_drvConfigure(dwDevID);
        case DRV_INSTALL:               return DRVCNF_RESTART;
        case DRV_REMOVE:                return DRVCNF_RESTART;
    }

    /* session instance */
    if (dwDevID == 0xFFFFFFFF)
        return 1;

    switch (wMsg) {
        case MCI_OPEN_DRIVER:   return MCIQTZ_mciOpen      (dwDevID, dwParam1, (LPMCI_DGV_OPEN_PARMSW)     dwParam2);
        case MCI_CLOSE_DRIVER:  return MCIQTZ_mciClose     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
        case MCI_PLAY:          return MCIQTZ_mciPlay      (dwDevID, dwParam1, (LPMCI_PLAY_PARMS)          dwParam2);
        case MCI_SEEK:          return MCIQTZ_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)          dwParam2);
        case MCI_STOP:          return MCIQTZ_mciStop      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)       dwParam2);
        case MCI_GETDEVCAPS:    return MCIQTZ_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)    dwParam2);
        case MCI_SET:           return MCIQTZ_mciSet       (dwDevID, dwParam1, (LPMCI_DGV_SET_PARMS)       dwParam2);
        case MCI_STATUS:        return MCIQTZ_mciStatus    (dwDevID, dwParam1, (LPMCI_DGV_STATUS_PARMSW)   dwParam2);
        case MCI_WHERE:         return MCIQTZ_mciWhere     (dwDevID, dwParam1, (LPMCI_DGV_RECT_PARMS)      dwParam2);
        /* Digital Video specific */
        case MCI_SETAUDIO:      return MCIQTZ_mciSetAudio  (dwDevID, dwParam1, (LPMCI_DGV_SETAUDIO_PARMSW) dwParam2);
        case MCI_RECORD:
        case MCI_PAUSE:
        case MCI_RESUME:
        case MCI_INFO:
        case MCI_PUT:
        case MCI_WINDOW:
        case MCI_LOAD:
        case MCI_SAVE:
        case MCI_FREEZE:
        case MCI_REALIZE:
        case MCI_UNFREEZE:
        case MCI_UPDATE:
        case MCI_STEP:
        case MCI_COPY:
        case MCI_CUT:
        case MCI_DELETE:
        case MCI_PASTE:
        case MCI_CUE:
        /* Digital Video specific */
        case MCI_CAPTURE:
        case MCI_MONITOR:
        case MCI_RESERVE:
        case MCI_SIGNAL:
        case MCI_SETVIDEO:
        case MCI_QUALITY:
        case MCI_LIST:
        case MCI_UNDO:
        case MCI_CONFIGURE:
        case MCI_RESTORE:
            FIXME("Unimplemented command [%08X]\n", wMsg);
            break;
        case MCI_SPIN:
        case MCI_ESCAPE:
            WARN("Unsupported command [%08X]\n", wMsg);
            break;
        case MCI_OPEN:
        case MCI_CLOSE:
            FIXME("Shouldn't receive a MCI_OPEN or CLOSE message\n");
            break;
        default:
            TRACE("Sending msg [%08X] to default driver proc\n", wMsg);
            return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }

    return MCIERR_UNRECOGNIZED_COMMAND;
}
