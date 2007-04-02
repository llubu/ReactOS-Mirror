/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native DirectDraw implementation
 * FILE:             subsys/win32k/ntddraw/dd.c
 * PROGRAMER:        Magnus Olsen (greatlord@reactos.org)
 * REVISION HISTORY:
 *       19/7-2006  Magnus Olsen
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

#define DdHandleTable GdiHandleTable

/* 
   DdMapMemory, DdDestroyDriver are not exported as NtGdi Call 
   This file is compelete for DD_CALLBACKS setup

   ToDO fix the NtGdiDdCreateSurface, shall we fix it 
   from GdiEntry or gdientry callbacks for DdCreateSurface
   have we miss some thing there 
*/

/************************************************************************/
/* NtGdiDdCreateSurface                                                 */
/* status : Bugs out                                                    */
/************************************************************************/

DWORD STDCALL NtGdiDdCreateSurface(
    HANDLE hDirectDrawLocal,
    HANDLE *hSurface,
    DDSURFACEDESC *puSurfaceDescription,
    DD_SURFACE_GLOBAL *puSurfaceGlobalData,
    DD_SURFACE_LOCAL *puSurfaceLocalData,
    DD_SURFACE_MORE *puSurfaceMoreData,
    PDD_CREATESURFACEDATA puCreateSurfaceData,
    HANDLE *puhSurface
)
{
    INT i = 0;
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    NTSTATUS Status = FALSE;
    PDD_DIRECTDRAW pDirectDraw = NULL;
    PDD_SURFACE phsurface = NULL;

    PDD_SURFACE_LOCAL pLocal;
    PDD_SURFACE_MORE pMore;
    PDD_SURFACE_GLOBAL pGlobal;

    DD_CREATESURFACEDATA CreateSurfaceData;

    /* FIXME alloc so mayne we need */
    PHANDLE *myhSurface = NULL;

    /* GCC4  warnns on value are unisitaed,
       but they are initated in seh 
    */

    DPRINT1("NtGdiDdCreateSurface\n");

    DPRINT1("Copy puCreateSurfaceData to kmode CreateSurfaceData\n");
    _SEH_TRY
    {
        ProbeForRead(puCreateSurfaceData,  sizeof(DD_CREATESURFACEDATA), 1);
        RtlCopyMemory( &CreateSurfaceData, puCreateSurfaceData,
                       sizeof( DD_CREATESURFACEDATA ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return ddRVal;
    }

    /* FIXME we only support one surface at moment 
       this is a hack to prevent more that one surface being create 
     */
    if (CreateSurfaceData.dwSCnt > 1)
    {
        CreateSurfaceData.dwSCnt = 1;
    }


    DPRINT1("Setup surface in put handler\n");
    myhSurface = ExAllocatePoolWithTag( PagedPool, CreateSurfaceData.dwSCnt * sizeof(HANDLE), 0);

    _SEH_TRY
    {
        ProbeForRead(hSurface,  CreateSurfaceData.dwSCnt * sizeof(HANDLE), 1);
        for (i=0;i<CreateSurfaceData.dwSCnt;i++)
        {
            myhSurface[i] = hSurface[i];
        }
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return ddRVal;
    }

    /* see if a surface have been create or not */
    
    for (i=0;i<CreateSurfaceData.dwSCnt;i++)
    {
        if (!myhSurface[i])
        {
            myhSurface[i] = GDIOBJ_AllocObj(DdHandleTable, GDI_OBJECT_TYPE_DD_SURFACE);
            if (!myhSurface[i])
            {
                /* FIXME lock myhSurface*/
                /* FIXME free myhSurface, and the contain */
                /* add to attach list */
                return ddRVal;
            }
            else
            {
                /* FIXME lock myhSurface*/
                /* FIXME add to attach list */
            }
        }
    }

    /* FIXME we need continue fix more that one createsurface */
    /* FIXME we need release  myhSurface before any exits*/

    phsurface = GDIOBJ_LockObj(DdHandleTable, myhSurface[0], GDI_OBJECT_TYPE_DD_SURFACE);
    if (!phsurface)
    {
        return ddRVal;
    }

    DPRINT1("Copy puCreateSurfaceData to kmode CreateSurfaceData\n");
    _SEH_TRY
    {
        ProbeForRead(puCreateSurfaceData,  sizeof(DD_CREATESURFACEDATA), 1);
        RtlCopyMemory( &CreateSurfaceData, puCreateSurfaceData,
                       sizeof( DD_CREATESURFACEDATA ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Copy puSurfaceGlobalData to kmode phsurface->Global\n");
    _SEH_TRY
    {
        ProbeForRead(puSurfaceGlobalData,  sizeof(DD_SURFACE_GLOBAL), 1);
        RtlCopyMemory( &phsurface->Global, puSurfaceGlobalData,
                       sizeof( DD_SURFACE_GLOBAL ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Copy puSurfaceMoreData to kmode phsurface->More\n");
    _SEH_TRY
    {
        ProbeForRead(puSurfaceMoreData,  sizeof(DD_SURFACE_MORE), 1);
        RtlCopyMemory( &phsurface->More, puSurfaceMoreData,
                       sizeof( DD_SURFACE_MORE ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Copy puSurfaceLocalData to kmode phsurface->Local\n");
    _SEH_TRY
    {
        ProbeForRead(puSurfaceLocalData,  sizeof(DD_SURFACE_LOCAL), 1);
        RtlCopyMemory( &phsurface->Local, puSurfaceLocalData,
                       sizeof( DD_SURFACE_LOCAL ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Copy puSurfaceDescription to kmode phsurface->desc\n");
    _SEH_TRY
    {
        ProbeForRead(puSurfaceDescription,  sizeof(DDSURFACEDESC), 1);
        RtlCopyMemory( &phsurface->desc, puSurfaceDescription,
                       sizeof( DDSURFACEDESC ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(!NT_SUCCESS(Status))
    {
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        SetLastNtError(Status);
        return ddRVal;
    }

    DPRINT1("Lock hDirectDrawLocal \n");
    phsurface->hDirectDrawLocal = hDirectDrawLocal;
    pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
    if (!pDirectDraw)
    {
        DPRINT1("fail \n");
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        return ddRVal;
    }


    /* FIXME unlock phsurface free phsurface at fail*/
    /* FIXME unlock hsurface free phsurface at fail*/
    /* FIXME add support for more that one surface create */
    /* FIXME alloc memory if it more that one surface */

    pLocal = &phsurface->Local;
    pMore = &phsurface->More;
    pGlobal = &phsurface->Global;

    for (i = 0; i < CreateSurfaceData.dwSCnt; i++)
    {
        phsurface->lcllist[i] = (PDD_SURFACE_LOCAL)pLocal;
        pLocal->lpGbl = pGlobal;
        pLocal->lpSurfMore = pMore;

        /* FIXME ?
        pLocal->lpAttachList;
        pLocal->lpAttachListFrom;
        */
        
       /* FIXME a countup to next pLocal, pMore, pGlobal */

    }

    /* setup DD_CREATESURFACEDATA CreateSurfaceData for the driver */
    CreateSurfaceData.lplpSList = (PDD_SURFACE_LOCAL *) &phsurface->lcllist;
    CreateSurfaceData.lpDDSurfaceDesc = &phsurface->desc;
    CreateSurfaceData.CreateSurface = NULL;
    CreateSurfaceData.ddRVal = DDERR_GENERIC;
    CreateSurfaceData.lpDD = &pDirectDraw->Global;

    /* the CreateSurface crash with lcl convering */
    if ((pDirectDraw->DD.dwFlags & DDHAL_CB32_CREATESURFACE))
    {
        DPRINT1("0x%04x",pDirectDraw->DD.CreateSurface);

        ddRVal = pDirectDraw->DD.CreateSurface(&CreateSurfaceData);
    }

    DPRINT1("Retun value is %04x and driver return code is %04x\n",ddRVal,CreateSurfaceData.ddRVal);

    /* FIXME support for more that one surface */
    _SEH_TRY
    {
        ProbeForWrite(puSurfaceDescription,  sizeof(DDSURFACEDESC), 1);
        RtlCopyMemory( puSurfaceDescription, &phsurface->desc, sizeof( DDSURFACEDESC ) );
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    _SEH_TRY
    {
        ProbeForWrite(puCreateSurfaceData,  sizeof(DD_CREATESURFACEDATA), 1);
        puCreateSurfaceData->ddRVal =  CreateSurfaceData.ddRVal;
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    for (i = 0; i < CreateSurfaceData.dwSCnt; i++)
    {
        _SEH_TRY
        {
            ProbeForWrite(puSurfaceGlobalData,  sizeof(DD_SURFACE_GLOBAL), 1);
            RtlCopyMemory( puSurfaceGlobalData, &phsurface->Global, sizeof( DD_SURFACE_GLOBAL ) );
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        _SEH_TRY
        {
            ProbeForWrite(puSurfaceLocalData,  sizeof(DD_SURFACE_LOCAL), 1);
            RtlCopyMemory( puSurfaceLocalData, &phsurface->Local, sizeof( DD_SURFACE_LOCAL ) );
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        _SEH_TRY
        {
            ProbeForWrite(puSurfaceMoreData,  sizeof(DD_SURFACE_MORE), 1);
            RtlCopyMemory( puSurfaceMoreData, &phsurface->More, sizeof( DD_SURFACE_MORE ) );

            puSurfaceLocalData->lpGbl = puSurfaceGlobalData;
            puSurfaceLocalData->lpSurfMore = puSurfaceMoreData;
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        DPRINT1("GDIOBJ_UnlockObjByPtr\n");
        GDIOBJ_UnlockObjByPtr(DdHandleTable, phsurface);
        _SEH_TRY
        {
            ProbeForWrite(puhSurface, sizeof(HANDLE), 1);
            puhSurface[i] = myhSurface[i];
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }


    /* FIXME fillin the return handler */
    DPRINT1("GDIOBJ_UnlockObjByPtr\n");
    GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);

    DPRINT1("Retun value is %04x and driver return code is %04x\n",ddRVal,CreateSurfaceData.ddRVal);
    return ddRVal;
}

/************************************************************************/
/* NtGdiDdWaitForVerticalBlank                                          */
/* status : OK working as it should                                     */
/************************************************************************/


DWORD STDCALL NtGdiDdWaitForVerticalBlank(
    HANDLE hDirectDrawLocal,
    PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData)
{
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    PDD_DIRECTDRAW pDirectDraw = NULL;
    NTSTATUS Status = FALSE;
    DD_WAITFORVERTICALBLANKDATA WaitForVerticalBlankData;

    DPRINT1("NtGdiDdWaitForVerticalBlank\n");

    _SEH_TRY
    {
            ProbeForRead(puWaitForVerticalBlankData, sizeof(DD_WAITFORVERTICALBLANKDATA), 1);
            RtlCopyMemory(&WaitForVerticalBlankData,puWaitForVerticalBlankData, sizeof(DD_WAITFORVERTICALBLANKDATA));
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(NT_SUCCESS(Status))
    {
        pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);

        if (pDirectDraw != NULL) 
        {
            if (pDirectDraw->DD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK)
            {
                WaitForVerticalBlankData.ddRVal = DDERR_GENERIC;
                WaitForVerticalBlankData.lpDD =  &pDirectDraw->Global;;
                ddRVal = pDirectDraw->DD.WaitForVerticalBlank(&WaitForVerticalBlankData);
            }
            _SEH_TRY
            {
                ProbeForWrite(puWaitForVerticalBlankData,  sizeof(DD_WAITFORVERTICALBLANKDATA), 1);
                puWaitForVerticalBlankData->ddRVal  = WaitForVerticalBlankData.ddRVal;
                puWaitForVerticalBlankData->bIsInVB = WaitForVerticalBlankData.bIsInVB;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;

            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        }
    }
    return ddRVal;
}


/************************************************************************/
/* CanCreateSurface                                                     */
/* status : OK working as it should                                     */
/************************************************************************/

DWORD STDCALL NtGdiDdCanCreateSurface(
    HANDLE hDirectDrawLocal,
    PDD_CANCREATESURFACEDATA puCanCreateSurfaceData
)
{
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    DD_CANCREATESURFACEDATA CanCreateSurfaceData;
    DDSURFACEDESC           desc;
    NTSTATUS Status = FALSE;
    PDD_DIRECTDRAW pDirectDraw = NULL;

    DPRINT1("NtGdiDdCanCreateSurface\n");

    _SEH_TRY
    {
            ProbeForRead(puCanCreateSurfaceData,  sizeof(DD_CANCREATESURFACEDATA), 1);
            RtlCopyMemory(&CanCreateSurfaceData,puCanCreateSurfaceData, sizeof(DD_CANCREATESURFACEDATA));

            /* FIXME can be version 2 of DDSURFACEDESC */
            ProbeForRead(puCanCreateSurfaceData->lpDDSurfaceDesc, sizeof(DDSURFACEDESC), 1);
            RtlCopyMemory(&desc,puCanCreateSurfaceData->lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(NT_SUCCESS(Status))
    {
        pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);
        if (pDirectDraw != NULL)
        {
            if (pDirectDraw->DD.dwFlags & DDHAL_CB32_CANCREATESURFACE)
            {
                CanCreateSurfaceData.ddRVal = DDERR_GENERIC;
                CanCreateSurfaceData.lpDD = &pDirectDraw->Global;
                CanCreateSurfaceData.lpDDSurfaceDesc = &desc;
                ddRVal = pDirectDraw->DD.CanCreateSurface(&CanCreateSurfaceData);

                _SEH_TRY
                {
                     ProbeForWrite(puCanCreateSurfaceData, sizeof(DD_CANCREATESURFACEDATA), 1);
                     puCanCreateSurfaceData->ddRVal = CanCreateSurfaceData.ddRVal;

                     /* FIXME can be version 2 of DDSURFACEDESC */
                     ProbeForWrite(puCanCreateSurfaceData->lpDDSurfaceDesc, sizeof(DDSURFACEDESC), 1);
                     RtlCopyMemory(puCanCreateSurfaceData->lpDDSurfaceDesc,&desc, sizeof(DDSURFACEDESC));

                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        }
    }

  return ddRVal;
}

/************************************************************************/
/* GetScanLine                                                          */
/* status : not implement, was undoc in msdn now it is doc              */
/************************************************************************/
DWORD STDCALL 
NtGdiDdGetScanLine( HANDLE hDirectDrawLocal, PDD_GETSCANLINEDATA puGetScanLineData)
{
    DWORD  ddRVal = DDHAL_DRIVER_NOTHANDLED;
    DD_GETSCANLINEDATA GetScanLineData;
    PDD_DIRECTDRAW pDirectDraw = NULL;
    NTSTATUS Status = FALSE;

    DPRINT1("NtGdiDdGetScanLine\n");

    _SEH_TRY
    {
            ProbeForRead(puGetScanLineData,  sizeof(DD_GETSCANLINEDATA), 1);
            RtlCopyMemory(&GetScanLineData,puGetScanLineData, sizeof(DD_GETSCANLINEDATA));
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if(NT_SUCCESS(Status))
    {
        pDirectDraw = GDIOBJ_LockObj(DdHandleTable, hDirectDrawLocal, GDI_OBJECT_TYPE_DIRECTDRAW);;
        if (pDirectDraw != NULL)
        {
            if (pDirectDraw->DD.dwFlags & DDHAL_CB32_GETSCANLINE)
            {
                GetScanLineData.ddRVal = DDERR_GENERIC;
                GetScanLineData.lpDD = &pDirectDraw->Global;
                ddRVal = pDirectDraw->DD.GetScanLine(&GetScanLineData);

                _SEH_TRY
                {
                    ProbeForWrite(puGetScanLineData,  sizeof(DD_GETSCANLINEDATA), 1);
                    puGetScanLineData->dwScanLine = GetScanLineData.dwScanLine;
                    puGetScanLineData->ddRVal     = GetScanLineData.ddRVal;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
            }
            GDIOBJ_UnlockObjByPtr(DdHandleTable, pDirectDraw);
        }
    }

  return ddRVal;
}
