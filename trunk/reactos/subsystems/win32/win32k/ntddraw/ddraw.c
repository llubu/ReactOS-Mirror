/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/ddraw.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       19/1-2006   Magnus Olsen
 */

#include <w32k.h>
#include <debug.h>

PGD_DXDDSTARTUPDXGRAPHICS gpfnStartupDxGraphics = NULL;
PGD_DXDDCLEANUPDXGRAPHICS gpfnCleanupDxGraphics = NULL;

/* export from dxeng.c */
extern DRVFN gaEngFuncs;
extern ULONG gcEngFuncs;

PDRVFN gpDxFuncs;
HANDLE ghDxGraphics;
ULONG gdwDirectDrawContext;


/************************************************************************/
/* DirectX graphic/video driver loading and cleanup start here          */
/************************************************************************/
NTSTATUS
STDCALL
DxDdStartupDxGraphics(  ULONG ulc1,
                        PDRVENABLEDATA DxEngDrvOld,
                        ULONG ulc2,
                        PDRVENABLEDATA DxgDrvOld,
                        PULONG DirectDrawContext,
                        PEPROCESS Proc)
{
    DRVENABLEDATA DxEngDrv;
    DRVENABLEDATA DxgDrv;

    NTSTATUS Status = STATUS_PROCEDURE_NOT_FOUND;

    /* FIXME setup of gaEngFuncs driver export list
     * but not in this api, we can add it here tempary until we figout where 
     * no code have been writen for it yet
     */


    /* FIXME ReactOS does not loading the dxapi.sys or import functions from it yet */
    // DxApiGetVersion()

    /* Loading the kernel interface of directx for win32k */
    ghDxGraphics = EngLoadImage(L"drivers\\dxg.sys");
    if (!ghDxGraphics)
    {
        DPRINT1("Warring no dxg.sys in ReactOS");
        Status = STATUS_DLL_NOT_FOUND;
    }
    else
    {
        /* import DxDdStartupDxGraphics and  DxDdCleanupDxGraphics */
        gpfnStartupDxGraphics = EngFindImageProcAddress(ghDxGraphics,"DxDdStartupDxGraphics");
        gpfnCleanupDxGraphics = EngFindImageProcAddress(ghDxGraphics,"DxDdCleanupDxGraphics");

        if ((gpfnStartupDxGraphics) &&
            (gpfnCleanupDxGraphics))
        {
            /* Setup driver data for activate the dx interface */
            DxEngDrv.iDriverVersion = DDI_DRIVER_VERSION_NT5_01;
            DxEngDrv.pdrvfn = &gaEngFuncs;
            DxEngDrv.c = gcEngFuncs;

            Status = gpfnStartupDxGraphics ( sizeof(DRVENABLEDATA),
                                             &DxEngDrv,
                                             sizeof(DRVENABLEDATA),
                                             &DxgDrv,
                                             &gdwDirectDrawContext,
                                             Proc );
        }

        /* check if we manger loading the data and execute the dxStartupDxGraphics and it susscess */
        if (!NT_SUCCESS(Status))
        {
            gpfnStartupDxGraphics = NULL;
            gpfnCleanupDxGraphics = NULL;
            EngUnloadImage( ghDxGraphics);
            ghDxGraphics = NULL;
            DPRINT1("Warring no init of DirectX graphic interface");
        }
        else
        {
            /* Sort the drv functions list in index order, this allown us doing, smaller optimze
             * in api that are redirect to dx.sys
             */

            PDRVFN lstDrvFN = DxgDrv.pdrvfn;
            INT t;
            for (t=0;t<=DXG_INDEX_DxDdIoctl;t++)
            {
                gpDxFuncs[lstDrvFN[t].iFunc].iFunc =lstDrvFN[t].iFunc;
                gpDxFuncs[lstDrvFN[t].iFunc].pfn =lstDrvFN[t].pfn;
            }
            DPRINT1("DirectX interface is Activated");
        }
        /* return the status */
    }

    return Status;
}

/************************************************************************/
/* DirectX graphic/video driver loading cleanup ends here               */
/************************************************************************/



/************************************************************************/
/* NtGdiDdCreateDirectDrawObject                                        */
/************************************************************************/
HANDLE
STDCALL 
NtGdiDdCreateDirectDrawObject(HDC hdc)
{

    PGD_DDCREATEDIRECTDRAWOBJECT pfnDdCreateDirectDrawObject = NULL;
    NTSTATUS Status;
    PEPROCESS Proc = NULL;
    INT i=0;

    /* FIXME get the process data */
    /* FIXME this code should be add where the driver being load */
    Status = DxDdStartupDxGraphics(0,NULL,0,NULL,NULL, Proc);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Warring : Fail to statup the directx interface");
        return 0;
    }

    /* This is in correct place */
    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateDirectDrawObject, pfnDdCreateDirectDrawObject);

    if (pfnDdCreateDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdCreateDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdCreateDirectDrawObject");
    return pfnDdCreateDirectDrawObject(hdc);

}

/*++
* @name NtGdiDxgGenericThunk
* @implemented
*
* The function NtGdiDxgGenericThunk redirect dx call to other thing.
* from dxg.sys
*
* @param ULONG_PTR ulIndex
* The functions we want redirct
*
* @param ULONG_PTR ulHandle
* Unknown
*
* @param SIZE_T *pdwSizeOfPtr1
* Unknown
*
* @param PVOID pvPtr1
* Unknown
*
* @param SIZE_T *pdwSizeOfPtr2
* Unknown
*
* @param PVOID pvPtr2
* Unknown
*
* @return 
* always return DDHAL_DRIVER_NOTHANDLED
*
* @remarks.
* dxg.sys NtGdiDxgGenericThunk call are redirect to dxg.sys
* This api are not longer use in Windows NT 2000/XP/2003
*
*--*/
DWORD
STDCALL
NtGdiDxgGenericThunk(ULONG_PTR ulIndex,
                     ULONG_PTR ulHandle,
                     SIZE_T *pdwSizeOfPtr1,
                     PVOID pvPtr1,
                     SIZE_T *pdwSizeOfPtr2,
                     PVOID pvPtr2)
{
    PGD_DXGENERICTRUNK pfnDxgGenericThunk = (PGD_DXGENERICTRUNK)gpDxFuncs[DXG_INDEX_DxDxgGenericThunk].pfn;

    if (pfnDxgGenericThunk == NULL)
    {
        DPRINT1("Warring no pfnDxgGenericThunk");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDxgGenericThunk");
    return pfnDxgGenericThunk(ulIndex, ulHandle, pdwSizeOfPtr1, pvPtr1, pdwSizeOfPtr2, pvPtr2);
}

/************************************************************************/
/* NtGdiDdGetDriverState                                                */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetDriverState(PDD_GETDRIVERSTATEDATA pdata)
{
    PGD_DDGETDRIVERSTATE pfnDdGetDriverState = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetDriverState, pfnDdGetDriverState);

    if (pfnDdGetDriverState == NULL)
    {
        DPRINT1("Warring no pfnDdGetDriverState");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdGetDriverState");
    return pfnDdGetDriverState(pdata);
}

/************************************************************************/
/* NtGdiDdColorControl                                                  */
/************************************************************************/
DWORD
STDCALL
NtGdiDdColorControl(HANDLE hSurface,
                    PDD_COLORCONTROLDATA puColorControlData)
{
    PGD_DDCOLORCONTROL pfnDdColorControl = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdColorControl, pfnDdColorControl);

    if (pfnDdColorControl == NULL)
    {
        DPRINT1("Warring no pfnDdColorControl");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdColorControl");
    return pfnDdColorControl(hSurface,puColorControlData);
}

/************************************************************************/
/* NtGdiDdCreateSurfaceObject                                           */
/************************************************************************/
HANDLE
STDCALL
NtGdiDdCreateSurfaceObject(HANDLE hDirectDrawLocal,
                           HANDLE hSurface,
                           PDD_SURFACE_LOCAL puSurfaceLocal,
                           PDD_SURFACE_MORE puSurfaceMore,
                           PDD_SURFACE_GLOBAL puSurfaceGlobal,
                           BOOL bComplete
)
{
    PGD_DXDDCREATESURFACEOBJECT pfnDdCreateSurfaceObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdCreateSurfaceObject, pfnDdCreateSurfaceObject);

    if (pfnDdCreateSurfaceObject == NULL)
    {
        DPRINT1("Warring no pfnDdCreateSurfaceObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdCreateSurfaceObject");
    return pfnDdCreateSurfaceObject(hDirectDrawLocal, hSurface, puSurfaceLocal, puSurfaceMore, puSurfaceGlobal, bComplete);
}

/************************************************************************/
/* NtGdiDdDeleteDirectDrawObject                                        */
/************************************************************************/
BOOL
STDCALL 
NtGdiDdDeleteDirectDrawObject(HANDLE hDirectDrawLocal)
{
    PGD_DXDDDELETEDIRECTDRAWOBJECT pfnDdDeleteDirectDrawObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDeleteDirectDrawObject, pfnDdDeleteDirectDrawObject);

    if (pfnDdDeleteDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdDeleteDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdDeleteDirectDrawObject");
    return pfnDdDeleteDirectDrawObject(hDirectDrawLocal);
}

/************************************************************************/
/* NtGdiDdDeleteSurfaceObject                                           */
/************************************************************************/
BOOL
STDCALL
NtGdiDdDeleteSurfaceObject(HANDLE hSurface)
{
    PGD_DXDDDELETESURFACEOBJECT pfnDdDeleteSurfaceObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdDeleteSurfaceObject, pfnDdDeleteSurfaceObject);

    if (pfnDdDeleteSurfaceObject == NULL)
    {
        DPRINT1("Warring no pfnDdDeleteSurfaceObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys DdDeleteSurfaceObject");
    return pfnDdDeleteSurfaceObject(hSurface);
}

/************************************************************************/
/* NtGdiDdDeleteSurfaceObject                                           */
/************************************************************************/
BOOL
STDCALL 
NtGdiDdQueryDirectDrawObject(HANDLE hDirectDrawLocal,
                             DD_HALINFO  *pHalInfo,
                             DWORD *pCallBackFlags,
                             LPD3DNTHAL_CALLBACKS puD3dCallbacks,
                             LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
                             PDD_D3DBUFCALLBACKS puD3dBufferCallbacks,
                             LPDDSURFACEDESC puD3dTextureFormats,
                             DWORD *puNumHeaps,
                             VIDEOMEMORY *puvmList,
                             DWORD *puNumFourCC,
                             DWORD *puFourCC)
{
    PGD_DXDDQUERYDIRECTDRAWOBJECT pfnDdQueryDirectDrawObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdQueryDirectDrawObject, pfnDdQueryDirectDrawObject);

    if (pfnDdQueryDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdQueryDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdQueryDirectDrawObject");
    return pfnDdQueryDirectDrawObject(hDirectDrawLocal, pHalInfo, pCallBackFlags, puD3dCallbacks, puD3dDriverData, 
                                      puD3dBufferCallbacks, puD3dTextureFormats, puNumHeaps, puvmList, puNumFourCC, puFourCC);
}


/************************************************************************/
/* NtGdiDdReenableDirectDrawObject                                      */
/************************************************************************/
BOOL
STDCALL
NtGdiDdReenableDirectDrawObject(HANDLE hDirectDrawLocal,
                                BOOL *pubNewMode)
{
    PGD_DXDDREENABLEDIRECTDRAWOBJECT pfnDdReenableDirectDrawObject = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdReenableDirectDrawObject, pfnDdReenableDirectDrawObject);

    if (pfnDdReenableDirectDrawObject == NULL)
    {
        DPRINT1("Warring no pfnDdReenableDirectDrawObject");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdReenableDirectDrawObject");
    return pfnDdReenableDirectDrawObject(hDirectDrawLocal, pubNewMode);
}


/************************************************************************/
/* NtGdiDdGetDriverInfo                                                 */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetDriverInfo(HANDLE hDirectDrawLocal,
                     PDD_GETDRIVERINFODATA puGetDriverInfoData)

{
    PGD_DXDDGETDRIVERINFO pfnDdGetDriverInfo = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetDriverInfo, pfnDdGetDriverInfo);

    if (pfnDdGetDriverInfo == NULL)
    {
        DPRINT1("Warring no pfnDdGetDriverInfo");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetDriverInfo");
    return pfnDdGetDriverInfo(hDirectDrawLocal, puGetDriverInfoData);
}


/************************************************************************/
/* NtGdiDdGetAvailDriverMemory                                          */
/************************************************************************/
DWORD
STDCALL
NtGdiDdGetAvailDriverMemory(HANDLE hDirectDrawLocal,
                            PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData)
{
    PGD_DXDDGETAVAILDRIVERMEMORY pfnDdGetAvailDriverMemory = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetAvailDriverMemory, pfnDdGetAvailDriverMemory);

    if (pfnDdGetAvailDriverMemory == NULL)
    {
        DPRINT1("Warring no pfnDdGetAvailDriverMemory");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetAvailDriverMemory");
    return pfnDdGetAvailDriverMemory(hDirectDrawLocal, puGetAvailDriverMemoryData);
}


/************************************************************************/
/* NtGdiDdSetExclusiveMode                                              */
/************************************************************************/

DWORD
STDCALL
NtGdiDdSetExclusiveMode(HANDLE hDirectDraw,
                        PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData)
{
    PGD_DXDDSETEXCLUSIVEMODE pfnDdSetExclusiveMode = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdSetExclusiveMode, pfnDdSetExclusiveMode);

    if (pfnDdSetExclusiveMode == NULL)
    {
        DPRINT1("Warring no pfnDdSetExclusiveMode");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdSetExclusiveMode");
    return pfnDdSetExclusiveMode(hDirectDraw, puSetExclusiveModeData);

}


/************************************************************************/
/* NtGdiDdFlipToGDISurface                                              */
/************************************************************************/
DWORD
STDCALL
NtGdiDdFlipToGDISurface(HANDLE hDirectDraw,
                        PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData)
{
    PGD_DXDDFLIPTOGDISURFACE pfnDdFlipToGDISurface = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdFlipToGDISurface, pfnDdFlipToGDISurface);

    if (pfnDdFlipToGDISurface == NULL)
    {
        DPRINT1("Warring no pfnDdFlipToGDISurface");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdFlipToGDISurface");
    return pfnDdFlipToGDISurface(hDirectDraw, puFlipToGDISurfaceData);

}

/************************************************************************/
/* NtGdiDdGetDC                                                         */
/************************************************************************/
HDC
STDCALL
NtGdiDdGetDC(HANDLE hSurface,
             PALETTEENTRY *puColorTable)
{
    PGD_DDGETDC pfnDdGetDC  = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetDC, pfnDdGetDC);

    if (pfnDdGetDC == NULL)
    {
        DPRINT1("Warring no pfnDdGetDC");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetDC");
    return pfnDdGetDC(hSurface, puColorTable);
}

/************************************************************************/
/* NtGdiDdGetDxHandle                                                   */
/************************************************************************/
HANDLE
STDCALL
NtGdiDdGetDxHandle(HANDLE hDirectDraw,
                   HANDLE hSurface,
                   BOOL bRelease)
{
    
    PGD_DDGETDXHANDLE pfnDdGetDxHandle = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdGetDxHandle, pfnDdGetDxHandle);

    if (pfnDdGetDxHandle == NULL)
    {
        DPRINT1("Warring no pfnDdGetDxHandle");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdGetDxHandle");
    return pfnDdGetDxHandle(hDirectDraw, hSurface, bRelease);
}


/************************************************************************/
/* NtGdiDdReleaseDC                                                     */
/************************************************************************/
BOOL
STDCALL
NtGdiDdReleaseDC(HANDLE hSurface)
{
    PGD_DDRELEASEDC pfnDdReleaseDC = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdReleaseDC, pfnDdReleaseDC);

    if (pfnDdReleaseDC == NULL)
    {
        DPRINT1("Warring no pfnDdReleaseDC");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdReleaseDC");
    return pfnDdReleaseDC(hSurface);
}

/************************************************************************/
/* NtGdiDdResetVisrgn                                                   */
/************************************************************************/
BOOL
STDCALL
NtGdiDdResetVisrgn(HANDLE hSurface,
                   HWND hwnd)
{

    PGD_DDRESTVISRGN pfnDdResetVisrgn = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdResetVisrgn, pfnDdResetVisrgn);

    if (pfnDdResetVisrgn == NULL)
    {
        DPRINT1("Warring no pfnDdResetVisrgn");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdResetVisrgn");
    return pfnDdResetVisrgn(hSurface, hwnd);
}

/************************************************************************/
/* NtGdiDdSetGammaRamp                                                  */
/************************************************************************/
BOOL
STDCALL
NtGdiDdSetGammaRamp(HANDLE hDirectDraw,
                    HDC hdc,
                    LPVOID lpGammaRamp)
{
    PGD_DDSETGAMMARAMP pfnDdSetGammaRamp = NULL;
    INT i;

    DXG_GET_INDEX_FUNCTION(DXG_INDEX_DxDdSetGammaRamp, pfnDdSetGammaRamp);

    if (pfnDdSetGammaRamp == NULL)
    {
        DPRINT1("Warring no pfnDdSetGammaRamp");
        return DDHAL_DRIVER_NOTHANDLED;
    }

    DPRINT1("Calling on dxg.sys pfnDdSetGammaRamp");
    return pfnDdSetGammaRamp(hDirectDraw, hdc, lpGammaRamp);
}




/* EOF */
