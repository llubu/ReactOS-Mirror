/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library 
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"
#include "d3dhal.h"

VOID 
Cleanup(LPDIRECTDRAW7 iface) 
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

    if (ddgbl.lpDDCBtmp != NULL)
    {
        DxHeapMemFree(ddgbl.lpDDCBtmp);
    }

    if (ddgbl.lpModeInfo != NULL)
    {
        DxHeapMemFree(ddgbl.lpModeInfo);
    }

    DdDeleteDirectDrawObject(&ddgbl);

    /* 
       are it any more I forget to release ?
    */

    /* release the linked interface */
    while (This->lpVtbl != NULL)
    {
        LPDDRAWI_DIRECTDRAW_INT newThis = This->lpVtbl;
        if (This->lpLcl != NULL)
        {
            DeleteDC(This->lpLcl->hDC);
            DxHeapMemFree(This->lpLcl);
        }
        
        This = newThis;
    }

    /* release unlinked interface */
    if (This->lpLcl != NULL)
    {
        DeleteDC(This->lpLcl->hDC);
        DxHeapMemFree(This->lpLcl);
    }

}

