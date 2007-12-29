#ifndef _INT_W32k_DDRAW
#define _INT_W32k_DDRAW

#include <ddrawint.h>
#include <ddkernel.h>
#include <reactos/drivers/directx/directxint.h>
#include <reactos/drivers/directx/dxg.h>
#include <reactos/drivers/directx/dxeng.h>

/* From ddraw.c */
extern DRVFN gpDxFuncs[];

typedef BOOL (NTAPI* PGD_DDSETGAMMARAMP)(HANDLE, HDC, LPVOID);
typedef BOOL (NTAPI* PGD_DDRELEASEDC)(HANDLE);
typedef BOOL (NTAPI* PGD_DDRESTVISRGN)(HANDLE, HWND);
typedef HANDLE (NTAPI* PGD_DDGETDXHANDLE)(HANDLE, HANDLE, BOOL);
typedef HDC (NTAPI *PGD_DDGETDC)(HANDLE, PALETTEENTRY *);
typedef DWORD (NTAPI *PGD_DXDDREENABLEDIRECTDRAWOBJECT)(HANDLE, BOOL*);
typedef DWORD (NTAPI *PGD_DXDDGETDRIVERINFO)(HANDLE, PDD_GETDRIVERINFODATA);
typedef DWORD (NTAPI *PGD_DXDDSETEXCLUSIVEMODE)(HANDLE, PDD_SETEXCLUSIVEMODEDATA);
typedef NTSTATUS (NTAPI *PGD_DXDDSTARTUPDXGRAPHICS) (ULONG, PDRVENABLEDATA, ULONG, PDRVENABLEDATA, PULONG, PEPROCESS);
typedef NTSTATUS (NTAPI *PGD_DXDDCLEANUPDXGRAPHICS) (VOID);
typedef HANDLE (NTAPI *PGD_DDCREATEDIRECTDRAWOBJECT) (HDC hdc);
typedef DWORD (NTAPI *PGD_DDGETDRIVERSTATE)(PDD_GETDRIVERSTATEDATA);
typedef DWORD (NTAPI *PGD_DDCOLORCONTROL)(HANDLE hSurface,PDD_COLORCONTROLDATA puColorControlData);
typedef HANDLE (NTAPI *PGD_DXDDCREATESURFACEOBJECT)(HANDLE, HANDLE, PDD_SURFACE_LOCAL, PDD_SURFACE_MORE, PDD_SURFACE_GLOBAL, BOOL);
typedef BOOL (NTAPI *PGD_DXDDDELETEDIRECTDRAWOBJECT)(HANDLE);
typedef BOOL (NTAPI *PGD_DXDDDELETESURFACEOBJECT)(HANDLE);
typedef DWORD (NTAPI *PGD_DXDDFLIPTOGDISURFACE)(HANDLE, PDD_FLIPTOGDISURFACEDATA);
typedef DWORD (NTAPI *PGD_DXDDGETAVAILDRIVERMEMORY)(HANDLE , PDD_GETAVAILDRIVERMEMORYDATA);
typedef BOOL (NTAPI *PGD_DXDDQUERYDIRECTDRAWOBJECT)(HANDLE, DD_HALINFO*, DWORD*,  LPD3DNTHAL_CALLBACKS, LPD3DNTHAL_GLOBALDRIVERDATA,
                                                    PDD_D3DBUFCALLBACKS, LPDDSURFACEDESC, DWORD *, VIDEOMEMORY *, DWORD *, DWORD *);


/* From d3d.c */
typedef DWORD (NTAPI *PGD_DXDDDESTROYD3DBUFFER)(HANDLE);
typedef DWORD (NTAPI *PGD_DDCANCREATED3DBUFFER)(HANDLE, PDD_CANCREATESURFACEDATA);
typedef DWORD (NTAPI *PGD_DXDDUNLOCKD3D)(HANDLE, PDD_UNLOCKDATA);
typedef DWORD (NTAPI *PGD_DXDDLOCKD3D)(HANDLE, PDD_LOCKDATA);
typedef DWORD (NTAPI *PGD_D3DVALIDATETEXTURESTAGESTATE)(LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA);
typedef DWORD (NTAPI *PGD_D3DDRAWPRIMITIVES2)(HANDLE, HANDLE, LPD3DNTHAL_DRAWPRIMITIVES2DATA, FLATPTR *, DWORD *, FLATPTR *, DWORD *);
typedef DWORD (NTAPI *PGD_DDCREATED3DBUFFER)(HANDLE, HANDLE *, DDSURFACEDESC *, DD_SURFACE_GLOBAL *, DD_SURFACE_LOCAL *, DD_SURFACE_MORE *, PDD_CREATESURFACEDATA , HANDLE *);
typedef BOOL (NTAPI *PGD_D3DCONTEXTCREATE)(HANDLE, HANDLE, HANDLE, LPD3DNTHAL_CONTEXTCREATEDATA);
typedef DWORD (NTAPI *PGD_D3DCONTEXTDESTROY)(LPD3DNTHAL_CONTEXTDESTROYDATA);
typedef DWORD (NTAPI *PGD_D3DCONTEXTDESTROYALL)(LPD3DNTHAL_CONTEXTDESTROYALLDATA);

/* From dvp.c */
typedef DWORD (NTAPI* PGD_DVPCANCREATEVIDEOPORT)(HANDLE, PDD_CANCREATEVPORTDATA);
typedef DWORD (NTAPI* PGD_DVPCOLORCONTROL)(HANDLE, PDD_VPORTCOLORDATA);
typedef HANDLE (NTAPI* PGD_DVPCREATEVIDEOPORT)(HANDLE, PDD_CREATEVPORTDATA);
typedef DWORD (NTAPI* PGD_DVPDESTROYVIDEOPORT)(HANDLE, PDD_DESTROYVPORTDATA);
typedef DWORD (NTAPI* PGD_DVPFLIPVIDEOPORT)(HANDLE,HANDLE,HANDLE,PDD_FLIPVPORTDATA);
typedef DWORD (NTAPI* PGD_DVPGETVIDEOPORTBANDWITH)(HANDLE, PDD_GETVPORTBANDWIDTHDATA);
typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOPORTFLIPSTATUS)(HANDLE, PDD_GETVPORTFLIPSTATUSDATA);
typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOPORTINPUTFORMATS)(HANDLE, PDD_GETVPORTINPUTFORMATDATA);
typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOPORTLINE)(HANDLE, PDD_GETVPORTLINEDATA);
typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOPORTOUTPUTFORMATS)(HANDLE, PDD_GETVPORTOUTPUTFORMATDATA);
typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOPORTCONNECTINFO)(HANDLE, PDD_GETVPORTCONNECTDATA);
typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOSIGNALSTATUS)(HANDLE, PDD_GETVPORTSIGNALDATA);
typedef DWORD (NTAPI *PGD_DXDVPUPDATEVIDEOPORT)(HANDLE, HANDLE*, HANDLE*, PDD_UPDATEVPORTDATA);
typedef DWORD (NTAPI *PGD_DXDVPWAITFORVIDEOPORTSYNC)(HANDLE, PDD_WAITFORVPORTSYNCDATA);
typedef DWORD (NTAPI *PGD_DXDVPACQUIRENOTIFICATION)(HANDLE, HANDLE*, LPDDVIDEOPORTNOTIFY);
typedef DWORD (NTAPI *PGD_DXDVPRELEASENOTIFICATION)(HANDLE, HANDLE);
typedef DWORD (NTAPI *PGD_DXDVPGETVIDEOPORTFIELD)(HANDLE, PDD_GETVPORTFIELDDATA);

/* From mocomp.c */
typedef DWORD (NTAPI *PGD_DDBEGINMOCOMPFRAME)(HANDLE, PDD_BEGINMOCOMPFRAMEDATA);
typedef HANDLE (NTAPI *PGD_DXDDCREATEMOCOMP)(HANDLE, PDD_CREATEMOCOMPDATA );
typedef DWORD (NTAPI *PGD_DXDDDESTROYMOCOMP)(HANDLE, PDD_DESTROYMOCOMPDATA);
typedef DWORD (NTAPI *PGD_DXDDENDMOCOMPFRAME)(HANDLE, PDD_ENDMOCOMPFRAMEDATA);
typedef DWORD (NTAPI *PGD_DXDDGETINTERNALMOCOMPINFO)(HANDLE, PDD_GETINTERNALMOCOMPDATA);
typedef DWORD (NTAPI *PGD_DXDDGETMOCOMPBUFFINFO)(HANDLE, PDD_GETMOCOMPCOMPBUFFDATA);
typedef DWORD (NTAPI *PGD_DXDDGETMOCOMPGUIDS)(HANDLE, PDD_GETMOCOMPGUIDSDATA);
typedef DWORD (NTAPI *PGD_DXDDGETMOCOMPFORMATS)(HANDLE, PDD_GETMOCOMPFORMATSDATA);
typedef DWORD (NTAPI *PGD_DXDDQUERYMOCOMPSTATUS)(HANDLE, PDD_QUERYMOCOMPSTATUSDATA);
typedef DWORD (NTAPI *PGD_DXDDRENDERMOCOMP)(HANDLE, PDD_RENDERMOCOMPDATA);

/* From dd.c */
typedef DWORD (NTAPI *PGD_DDCREATESURFACE)(HANDLE, HANDLE *, DDSURFACEDESC *, DD_SURFACE_GLOBAL *, DD_SURFACE_LOCAL *, DD_SURFACE_MORE *, PDD_CREATESURFACEDATA , HANDLE *);
typedef DWORD (NTAPI *PGD_DXDDWAITFORVERTICALBLANK)(HANDLE, PDD_WAITFORVERTICALBLANKDATA);
typedef DWORD (NTAPI *PGD_DDCANCREATESURFACE)(HANDLE hDirectDrawLocal, PDD_CANCREATESURFACEDATA puCanCreateSurfaceData);
typedef DWORD (NTAPI *PGD_DXDDGETSCANLINE)(HANDLE, PDD_GETSCANLINEDATA);
typedef DWORD (NTAPI *PGD_DXDDCREATESURFACEEX)(HANDLE,HANDLE,DWORD);

/* From ddsurf.c */
typedef DWORD (NTAPI *PGD_DDALPHABLT)(HANDLE, HANDLE, PDD_BLTDATA);
typedef BOOL (NTAPI *PGD_DDATTACHSURFACE)(HANDLE, HANDLE);
typedef DWORD (NTAPI *PGD_DXDDUNATTACHSURFACE)(HANDLE, HANDLE);
typedef DWORD (NTAPI *PGD_DXDDDESTROYSURFACE)(HANDLE, BOOL);
typedef DWORD (NTAPI *PGD_DXDDFLIP)(HANDLE, HANDLE, HANDLE, HANDLE, PDD_FLIPDATA);
typedef DWORD (NTAPI *PGD_DXDDLOCK)(HANDLE, PDD_LOCKDATA, HDC);
typedef DWORD (NTAPI *PGD_DXDDUNLOCK)(HANDLE, PDD_UNLOCKDATA );
typedef DWORD (NTAPI *PGD_DDBLT)(HANDLE, HANDLE, PDD_BLTDATA);
typedef DWORD (NTAPI *PGD_DXDDSETCOLORKEY)(HANDLE, PDD_SETCOLORKEYDATA);
typedef DWORD (NTAPI *PGD_DDADDATTACHEDSURFACE)(HANDLE, HANDLE,PDD_ADDATTACHEDSURFACEDATA);
typedef DWORD (NTAPI *PGD_DXDDGETBLTSTATUS)(HANDLE, PDD_GETBLTSTATUSDATA);
typedef DWORD (NTAPI *PGD_DXDDGETFLIPSTATUS)(HANDLE, PDD_GETFLIPSTATUSDATA);
typedef DWORD (NTAPI *PGD_DXDDUPDATEOVERLAY)(HANDLE, HANDLE, PDD_UPDATEOVERLAYDATA);
typedef DWORD (NTAPI *PGD_DXDDSETOVERLAYPOSITION)(HANDLE, HANDLE, PDD_SETOVERLAYPOSITIONDATA);

/* From eng.c */
typedef FLATPTR (NTAPI *PGD_HEAPVIDMEMALLOCALIGNED)(LPVIDMEM, DWORD, DWORD, LPSURFACEALIGNMENT, LPLONG);
typedef VOID (NTAPI *PGD_VIDMEMFREE)(LPVMEMHEAP, FLATPTR);
typedef PVOID (NTAPI *PGD_ENGALLOCPRIVATEUSERMEM)(PDD_SURFACE_LOCAL, SIZE_T, ULONG) ;
typedef VOID (NTAPI *PGD_ENGFREEPRIVATEUSERMEM)(PDD_SURFACE_LOCAL, PVOID);
typedef PDD_SURFACE_LOCAL (NTAPI *PGD_ENGLOCKDIRECTDRAWSURFACE)(HANDLE);
typedef BOOL (NTAPI *PGD_ENGUNLOCKDIRECTDRAWSURFACE)(PDD_SURFACE_LOCAL);


/* Standard macro */
#define DXG_GET_INDEX_FUNCTION(INDEX, FUNCTION) \
    if (gpDxFuncs) \
    { \
        for (i = 0; i <= DXG_INDEX_DxDdIoctl; i++) \
        { \
            if (gpDxFuncs[i].iFunc == INDEX)  \
            { \
                FUNCTION = (VOID *)gpDxFuncs[i].pfn;  \
                break;  \
            }  \
        } \
    }

/* Gammaramp internal prototype */
BOOL FASTCALL IntGetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp);
BOOL FASTCALL IntSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp, BOOL);

#endif /* _INT_W32k_DDRAW */
