/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include "rosdraw.h"
#include "ddrawgdi.h"

DDRAWI_DIRECTDRAW_GBL ddgbl;
DDRAWI_DDRAWSURFACE_GBL ddSurfGbl;

WCHAR classname[128];
WNDCLASSW wnd_class;


HRESULT WINAPI
Create_DirectDraw (LPGUID pGUID, LPDIRECTDRAW* pIface,
                   REFIID id, BOOL ex)
{
    LPDDRAWI_DIRECTDRAW_INT This;

    DX_WINDBG_trace();

    if ((IsBadReadPtr(pIface,sizeof(LPDIRECTDRAW))) ||
       (IsBadWritePtr(pIface,sizeof(LPDIRECTDRAW))))
    {
        return DDERR_INVALIDPARAMS;
    }

    DX_STUB_str("here\n");

    This = (LPDDRAWI_DIRECTDRAW_INT)*pIface;

    /* fixme linking too second link when we shall not doing it */
    if (IsBadReadPtr(This,sizeof(LPDIRECTDRAW)))
    {
        DX_STUB_str("1. no linking\n");
        /* We do not have a DirectDraw interface, we need alloc it*/
        LPDDRAWI_DIRECTDRAW_INT memThis;

        DX_STUB_str("here\n");

        DxHeapMemAlloc(memThis, sizeof(DDRAWI_DIRECTDRAW_INT));

        DX_STUB_str("here\n")

        if (memThis == NULL)
        {
            DX_STUB_str("DDERR_OUTOFMEMORY\n");
            return DDERR_OUTOFMEMORY;
        }

        This = memThis;


        DX_STUB_str("here\n");

        /* Fixme release memory alloc if we fail */
        DxHeapMemAlloc(This->lpLcl, sizeof(DDRAWI_DIRECTDRAW_INT));
        if (This->lpLcl == NULL)
        {
            DX_STUB_str("DDERR_OUTOFMEMORY\n");
            return DDERR_OUTOFMEMORY;
        }

        DX_STUB_str("here\n");
    }
    else
    {
        DX_STUB_str("2.linking\n");
        /* We got the DirectDraw interface alloc and we need create the link */
        LPDDRAWI_DIRECTDRAW_INT  newThis;

        DX_STUB_str("here\n");

        /* step 1.Alloc the new  DDRAWI_DIRECTDRAW_INT for the lnking */
        DxHeapMemAlloc(newThis, sizeof(DDRAWI_DIRECTDRAW_INT));
        if (newThis == NULL)
        {
            DX_STUB_str("DDERR_OUTOFMEMORY\n");
            return DDERR_OUTOFMEMORY;
        }

        DX_STUB_str("here\n");

        /* step 2 check if it not DDCREATE_HARDWAREONLY we got if so we fail */
        if ((pGUID) && (pGUID != (LPGUID)DDCREATE_HARDWAREONLY))
        {
            if (pGUID !=NULL)
            {
                This = newThis;
                DX_STUB_str("DDERR_INVALIDDIRECTDRAWGUID\n");
                return DDERR_INVALIDDIRECTDRAWGUID;
            }
        }

        DX_STUB_str("here\n");

        /* step 3 do the link the old interface are store in the new one */
        newThis->lpLink = This;

        /* step 4 we need create new local directdraw struct for the new linked interface */
        DxHeapMemAlloc(newThis->lpLcl, sizeof(DDRAWI_DIRECTDRAW_LCL));
        if (newThis->lpLcl == NULL)
        {
            This = newThis;
            DX_STUB_str("DDERR_OUTOFMEMORY\n");
            return DDERR_OUTOFMEMORY;
        }

        DX_STUB_str("here\n");

        This = newThis;
    }


    This->lpLcl->lpGbl = &ddgbl;

    *pIface = (LPDIRECTDRAW)This;

    /* Get right interface we whant */
    if (Main_DirectDraw_QueryInterface((LPDIRECTDRAW7)This, id, (void**)&pIface) == DD_OK)
    {
        DX_STUB_str("Got iface\n");

        if (StartDirectDraw((LPDIRECTDRAW)This, pGUID, FALSE) == DD_OK);
        {
            /*
            RtlZeroMemory(&wnd_class, sizeof(wnd_class));
            wnd_class.style = CS_HREDRAW | CS_VREDRAW;
            wnd_class.lpfnWndProc = DefWindowProcW;
            wnd_class.cbClsExtra = 0;
            wnd_class.cbWndExtra = 0;
            wnd_class.hInstance = GetModuleHandleW(0);
            wnd_class.hIcon = 0;
            wnd_class.hCursor = 0;
            wnd_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
            wnd_class.lpszMenuName = NULL;
            wnd_class.lpszClassName = classname;
            if(!RegisterClassW(&wnd_class))
            {
                DX_STUB_str("DDERR_GENERIC");
                return DDERR_GENERIC;
            }
            */

            DX_STUB_str("DD_OK\n");
            return DD_OK;
        }
    }

    DX_STUB_str("DDERR_INVALIDPARAMS\n");
    return DDERR_INVALIDPARAMS;
}


HRESULT WINAPI
StartDirectDraw(LPDIRECTDRAW iface, LPGUID lpGuid, BOOL reenable)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DWORD hal_ret = DD_FALSE;
    DWORD hel_ret = DD_FALSE;
    DWORD devicetypes = 0;
    DWORD dwFlags = 0;

    DX_WINDBG_trace();

    /*
     * ddgbl.dwPDevice  is not longer in use in windows 2000 and higher
     * I am using it for device type
     * devicetypes = 1 : both hal and hel are enable
     * devicetypes = 2 : both hal are enable
     * devicetypes = 3 : both hel are enable
     * devicetypes = 4 :loading a guid drv from the register
     */

    if (reenable == FALSE)
    {
        if ((!IsBadReadPtr(This->lpLink,sizeof(LPDIRECTDRAW))) && (This->lpLink == NULL))
        {
            RtlZeroMemory(&ddgbl, sizeof(DDRAWI_DIRECTDRAW_GBL));
            This->lpLcl->lpGbl->dwRefCnt++;
            if (ddgbl.lpDDCBtmp == NULL)
            {
                // LPDDHAL_CALLBACKS
                DxHeapMemAlloc( ddgbl.lpDDCBtmp , sizeof(DDHAL_CALLBACKS));
                if (ddgbl.lpDDCBtmp == NULL)
                {
                    DX_STUB_str("Out of memmory\n");
                    return DD_FALSE;
                }
            }
        }
    }

    DX_STUB_str("here\n");

    if (reenable == FALSE)
    {
        if (lpGuid == NULL)
        {
            DX_STUB_str("lpGuid == NULL\n");
            devicetypes= 1;

            /* Create HDC for default, hal and hel driver */
            This->lpLcl->hWnd = (ULONG_PTR) GetActiveWindow();
            This->lpLcl->hDC = (ULONG_PTR) GetDC((HWND)This->lpLcl->hWnd);

            /* cObsolete is undoc in msdn it being use in CreateDCA */
            RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
            RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
            dwFlags |= DDRAWI_DISPLAYDRV | DDRAWI_GDIDRV;


        }
        else if (lpGuid == (LPGUID) DDCREATE_HARDWAREONLY)
        {
            devicetypes = 2;
            /* Create HDC for default, hal driver */
            This->lpLcl->hWnd =(ULONG_PTR) GetActiveWindow();
            This->lpLcl->hDC = (ULONG_PTR) GetDC((HWND)This->lpLcl->hWnd);

            /* cObsolete is undoc in msdn it being use in CreateDCA */
            RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
            RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
            dwFlags |= DDRAWI_DISPLAYDRV | DDRAWI_GDIDRV;
        }
        else if (lpGuid == (LPGUID) DDCREATE_EMULATIONONLY)
        {
            devicetypes = 3;

            /* Create HDC for default, hal and hel driver */
            This->lpLcl->hWnd = (ULONG_PTR) GetActiveWindow();
            This->lpLcl->hDC = (ULONG_PTR) GetDC((HWND)This->lpLcl->hWnd);

            /* cObsolete is undoc in msdn it being use in CreateDCA */
            RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
            RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);

            dwFlags |= DDRAWI_DISPLAYDRV | DDRAWI_GDIDRV;
        }
        else
        {
            /* FIXME : need getting driver from the GUID that have been pass in from
             * the register. we do not support that yet
             */
             devicetypes = 4;
             This->lpLcl->hDC = (ULONG_PTR) NULL ;
             This->lpLcl->hWnd = (ULONG_PTR) GetActiveWindow();
        }

        if ( (HDC)This->lpLcl->hDC == NULL)
        {
            DX_STUB_str("DDERR_OUTOFMEMORY\n");
            return DDERR_OUTOFMEMORY ;
        }
    }

    This->lpLcl->lpDDCB = ddgbl.lpDDCBtmp;

    /* Startup HEL and HAL */
    This->lpLcl->lpDDCB = This->lpLcl->lpGbl->lpDDCBtmp;
    This->lpLcl->dwProcessId = GetCurrentProcessId();

    switch (devicetypes)
    {
            case 2:
              hal_ret = StartDirectDrawHal(iface, reenable);
              This->lpLcl->lpDDCB->HELDD.dwFlags = 0;
              break;

            case 3:
              hel_ret = StartDirectDrawHel(iface, reenable);
              This->lpLcl->lpDDCB->HALDD.dwFlags = 0;
              break;

            default:
              hal_ret = StartDirectDrawHal(iface, reenable);
              hel_ret = StartDirectDrawHel(iface, reenable);
    }

    DX_STUB_str("return\n");

    if (hal_ret!=DD_OK)
    {
        if (hel_ret!=DD_OK)
        {
            DX_STUB_str("DDERR_NODIRECTDRAWSUPPORT\n");
            return DDERR_NODIRECTDRAWSUPPORT;
        }
        dwFlags |= DDRAWI_NOHARDWARE;
    }

    if (hel_ret!=DD_OK)
    {
        dwFlags |= DDRAWI_NOEMULATION;

    }
    else
    {
        dwFlags |= DDRAWI_EMULATIONINITIALIZED;
    }

    /* Fill some basic info for Surface */
    This->lpLcl->lpGbl->dwFlags = dwFlags | DDRAWI_ATTACHEDTODESKTOP;
    This->lpLcl->lpDDCB = This->lpLcl->lpGbl->lpDDCBtmp;
    This->lpLcl->hDD = This->lpLcl->lpGbl->hDD;
    ddgbl.hDD = This->lpLcl->lpGbl->hDD;

    DX_STUB_str("DD_OK\n");
    return DD_OK;
}

HRESULT WINAPI
StartDirectDrawHel(LPDIRECTDRAW iface, BOOL reenable)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.CanCreateSurface     = HelDdCanCreateSurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.CreateSurface        = HelDdCreateSurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.CreatePalette        = HelDdCreatePalette;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.DestroyDriver        = HelDdDestroyDriver;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.FlipToGDISurface     = HelDdFlipToGDISurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.GetScanLine          = HelDdGetScanLine;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.SetColorKey          = HelDdSetColorKey;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.SetExclusiveMode     = HelDdSetExclusiveMode;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.SetMode              = HelDdSetMode;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.WaitForVerticalBlank = HelDdWaitForVerticalBlank;

    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.dwFlags =  DDHAL_CB32_CANCREATESURFACE     |
                                          DDHAL_CB32_CREATESURFACE        |
                                          DDHAL_CB32_CREATEPALETTE        |
                                          DDHAL_CB32_DESTROYDRIVER        |
                                          DDHAL_CB32_FLIPTOGDISURFACE     |
                                          DDHAL_CB32_GETSCANLINE          |
                                          DDHAL_CB32_SETCOLORKEY          |
                                          DDHAL_CB32_SETEXCLUSIVEMODE     |
                                          DDHAL_CB32_SETMODE              |
                                          DDHAL_CB32_WAITFORVERTICALBLANK ;

    This->lpLcl->lpGbl->lpDDCBtmp->HELDD.dwSize = sizeof(This->lpLcl->lpDDCB->HELDD);

    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.AddAttachedSurface = HelDdSurfAddAttachedSurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.Blt = HelDdSurfBlt;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.DestroySurface = HelDdSurfDestroySurface;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.Flip = HelDdSurfFlip;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.GetBltStatus = HelDdSurfGetBltStatus;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.GetFlipStatus = HelDdSurfGetFlipStatus;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.Lock = HelDdSurfLock;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.reserved4 = HelDdSurfreserved4;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.SetClipList = HelDdSurfSetClipList;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.SetColorKey = HelDdSurfSetColorKey;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.SetOverlayPosition = HelDdSurfSetOverlayPosition;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.SetPalette = HelDdSurfSetPalette;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.Unlock = HelDdSurfUnlock;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.UpdateOverlay = HelDdSurfUpdateOverlay;
    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.dwFlags = DDHAL_SURFCB32_ADDATTACHEDSURFACE |
                                                DDHAL_SURFCB32_BLT                |
                                                DDHAL_SURFCB32_DESTROYSURFACE     |
                                                DDHAL_SURFCB32_FLIP               |
                                                DDHAL_SURFCB32_GETBLTSTATUS       |
                                                DDHAL_SURFCB32_GETFLIPSTATUS      |
                                                DDHAL_SURFCB32_LOCK               |
                                                DDHAL_SURFCB32_RESERVED4          |
                                                DDHAL_SURFCB32_SETCLIPLIST        |
                                                DDHAL_SURFCB32_SETCOLORKEY        |
                                                DDHAL_SURFCB32_SETOVERLAYPOSITION |
                                                DDHAL_SURFCB32_SETPALETTE         |
                                                DDHAL_SURFCB32_UNLOCK             |
                                                DDHAL_SURFCB32_UPDATEOVERLAY;

    This->lpLcl->lpGbl->lpDDCBtmp->HELDDSurface.dwSize = sizeof(This->lpLcl->lpDDCB->HELDDSurface);

    /*
    This->lpLcl->lpDDCB->HELDDPalette.DestroyPalette  = HelDdPalDestroyPalette;
    This->lpLcl->lpDDCB->HELDDPalette.SetEntries = HelDdPalSetEntries;
    This->lpLcl->lpDDCB->HELDDPalette.dwSize = sizeof(This->lpLcl->lpDDCB->HELDDPalette);
    */

    /*
    This->lpLcl->lpDDCB->HELDDExeBuf.CanCreateExecuteBuffer = HelDdExeCanCreateExecuteBuffer;
    This->lpLcl->lpDDCB->HELDDExeBuf.CreateExecuteBuffer = HelDdExeCreateExecuteBuffer;
    This->lpLcl->lpDDCB->HELDDExeBuf.DestroyExecuteBuffer = HelDdExeDestroyExecuteBuffer;
    This->lpLcl->lpDDCB->HELDDExeBuf.LockExecuteBuffer = HelDdExeLockExecuteBuffer;
    This->lpLcl->lpDDCB->HELDDExeBuf.UnlockExecuteBuffer = HelDdExeUnlockExecuteBuffer;
    */

    return DD_OK;
}


HRESULT WINAPI
StartDirectDrawHal(LPDIRECTDRAW iface, BOOL reenable)
{
    LPDWORD mpFourCC = NULL;
    DDHALINFO mHALInfo;
    BOOL newmode = FALSE;
    LPDDSURFACEDESC mpTextures;
    D3DHAL_CALLBACKS mD3dCallbacks;
    D3DHAL_GLOBALDRIVERDATA mD3dDriverData;
    DDHAL_DDEXEBUFCALLBACKS mD3dBufferCallbacks;
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    DX_WINDBG_trace();

    RtlZeroMemory(&mHALInfo, sizeof(DDHALINFO));
    RtlZeroMemory(&mD3dCallbacks, sizeof(D3DHAL_CALLBACKS));
    RtlZeroMemory(&mD3dDriverData, sizeof(D3DHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&mD3dBufferCallbacks, sizeof(DDHAL_DDEXEBUFCALLBACKS));

    if (reenable == FALSE)
    {
        if (ddgbl.lpDDCBtmp == NULL)
        {
            DxHeapMemAlloc(ddgbl.lpDDCBtmp, sizeof(DDHAL_CALLBACKS));
            if ( ddgbl.lpDDCBtmp == NULL)
            {
                return DD_FALSE;
            }
        }
    }
    else
    {
        RtlZeroMemory(ddgbl.lpDDCBtmp,sizeof(DDHAL_CALLBACKS));
    }

    /*
     *  Startup DX HAL step one of three
     */
    if (!DdCreateDirectDrawObject(This->lpLcl->lpGbl, (HDC)This->lpLcl->hDC))
    {
       DxHeapMemFree(ddgbl.lpDDCBtmp);
       return DD_FALSE;
    }

    /* Some card disable the dx after it have been created so
     * we are force reanble it
     */
    if (!DdReenableDirectDrawObject(This->lpLcl->lpGbl, &newmode))
    {
      DxHeapMemFree(ddgbl.lpDDCBtmp);
      return DD_FALSE;
    }

    if (!DdQueryDirectDrawObject(This->lpLcl->lpGbl,
                                 &mHALInfo,
                                 &ddgbl.lpDDCBtmp->HALDD,
                                 &ddgbl.lpDDCBtmp->HALDDSurface,
                                 &ddgbl.lpDDCBtmp->HALDDPalette,
                                 &mD3dCallbacks,
                                 &mD3dDriverData,
                                 &mD3dBufferCallbacks,
                                 NULL,
                                 mpFourCC,
                                 NULL))
    {
      DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
      DxHeapMemFree(ddgbl.lpDDCBtmp);
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

#if 0
     DX_STUB_str("Trying alloc FourCCC \n");

    /* Alloc mpFourCC */
    if (This->lpLcl->lpGbl->lpdwFourCC != NULL)
    {
        DxHeapMemFree(This->lpLcl->lpGbl->lpdwFourCC);
    }

    if (mHALInfo.ddCaps.dwNumFourCCCodes > 0 )
    {
        DxHeapMemAlloc(mpFourCC, sizeof(DWORD) * 21);

        mpFourCC = (DWORD *) DxHeapMemAlloc(sizeof(DWORD) * (mHALInfo.ddCaps.dwNumFourCCCodes + 2));

        if (mpFourCC == NULL)
        {
            DxHeapMemFree(ddgbl.lpDDCBtmp);
            // FIXME Close DX fristcall and second call
            return DD_FALSE;
        }
    }

    DX_STUB_str("End Trying alloc FourCCC\n");
#endif




    /* Alloc mpTextures */
#if 0
    DX_STUB_str("1 Here\n");

    if (This->lpLcl->lpGbl->texture != NULL)
    {
        DxHeapMemFree(This->lpLcl->lpGbl->texture;
    }

    mpTextures = NULL;
    if (mD3dDriverData.dwNumTextureFormats > 0)
    {
        mpTextures = (DDSURFACEDESC*) DxHeapMemAlloc(sizeof(DDSURFACEDESC) * mD3dDriverData.dwNumTextureFormats);
        if (mpTextures == NULL)
        {
            DxHeapMemFree(mpFourCC);
            DxHeapMemFree(ddgbl.lpDDCBtmp);
            // FIXME Close DX fristcall and second call
        }
    }

    DX_STUB_str("2 Here\n");

#else
      mpTextures = NULL;
#endif


    /* Get all basic data from the driver */
    if (!DdQueryDirectDrawObject(
                                 This->lpLcl->lpGbl,
                                 &mHALInfo,
                                 &ddgbl.lpDDCBtmp->HALDD,
                                 &ddgbl.lpDDCBtmp->HALDDSurface,
                                 &ddgbl.lpDDCBtmp->HALDDPalette,
                                 &mD3dCallbacks,
                                 &mD3dDriverData,
                                 &ddgbl.lpDDCBtmp->HALDDExeBuf,
                                 (DDSURFACEDESC*)mpTextures,
                                 mpFourCC,
                                 NULL))
    {
        DxHeapMemFree(mpFourCC);
        DxHeapMemFree(mpTextures);
        DxHeapMemFree(ddgbl.lpDDCBtmp);
        // FIXME Close DX fristcall and second call
        return DD_FALSE;
    }

    memcpy(&ddgbl.vmiData, &mHALInfo.vmiData,sizeof(VIDMEMINFO));
    

    memcpy(&ddgbl.ddCaps,  &mHALInfo.ddCaps,sizeof(DDCORECAPS));

    This->lpLcl->lpGbl->dwNumFourCC        = mHALInfo.ddCaps.dwNumFourCCCodes;
    This->lpLcl->lpGbl->lpdwFourCC         = mpFourCC;
    This->lpLcl->lpGbl->dwMonitorFrequency = mHALInfo.dwMonitorFrequency;
    This->lpLcl->lpGbl->dwModeIndex        = mHALInfo.dwModeIndex;
    This->lpLcl->lpGbl->dwNumModes         = mHALInfo.dwNumModes;
    This->lpLcl->lpGbl->lpModeInfo         = mHALInfo.lpModeInfo;

    DX_STUB_str("Here\n");

    /* FIXME convert mpTextures to DDHALMODEINFO */
    // DxHeapMemFree( mpTextures);

    /* FIXME D3D setup mD3dCallbacks and mD3dDriverData */
    DDHAL_GETDRIVERINFODATA DdGetDriverInfo = { 0 };
    DdGetDriverInfo.dwSize = sizeof (DDHAL_GETDRIVERINFODATA);
    DdGetDriverInfo.guidInfo = GUID_MiscellaneousCallbacks;

    DdGetDriverInfo.lpvData = (PVOID)&ddgbl.lpDDCBtmp->HALDDMiscellaneous;

    DdGetDriverInfo.dwExpectedSize = sizeof (DDHAL_DDMISCELLANEOUSCALLBACKS);

    if(mHALInfo.GetDriverInfo (&DdGetDriverInfo) == DDHAL_DRIVER_NOTHANDLED || DdGetDriverInfo.ddRVal != DD_OK)
    {
        DxHeapMemFree(mpFourCC);
        DxHeapMemFree(mpTextures);
        DxHeapMemFree(ddgbl.lpDDCBtmp);
        // FIXME Close DX fristcall and second call
        return DD_FALSE;
    }

    return DD_OK;
}
