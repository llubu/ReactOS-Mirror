/*
 * IDirect3DTexture9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/* IDirect3DTexture9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DTexture9Impl_QueryInterface(LPDIRECT3DTEXTURE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
        || IsEqualGUID(riid, &IID_IDirect3DTexture9)) {
        IDirect3DTexture9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p) not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DTexture9Impl_AddRef(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DTexture9Impl_Release(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        wined3d_mutex_lock();
        IWineD3DTexture_Destroy(This->wineD3DTexture, D3D9CB_DestroySurface);
        wined3d_mutex_unlock();

        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DTexture9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DTexture9Impl_GetDevice(LPDIRECT3DTEXTURE9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    IWineD3DDevice *wined3d_device;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_GetDevice(This->wineD3DTexture, &wined3d_device);
    if (SUCCEEDED(hr))
    {
        IWineD3DDevice_GetParent(wined3d_device, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(wined3d_device);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_SetPrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_SetPrivateData(This->wineD3DTexture, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_GetPrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_GetPrivateData(This->wineD3DTexture, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_FreePrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_FreePrivateData(This->wineD3DTexture, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DTexture9Impl_SetPriority(LPDIRECT3DTEXTURE9 iface, DWORD PriorityNew) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_SetPriority(This->wineD3DTexture, PriorityNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture9Impl_GetPriority(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_GetPriority(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI IDirect3DTexture9Impl_PreLoad(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    IWineD3DTexture_PreLoad(This->wineD3DTexture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DTexture9Impl_GetType(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT ret;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_GetType(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return ret;
}

/* IDirect3DTexture9 IDirect3DBaseTexture9 Interface follow: */
static DWORD WINAPI IDirect3DTexture9Impl_SetLOD(LPDIRECT3DTEXTURE9 iface, DWORD LODNew) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_SetLOD(This->wineD3DTexture, LODNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture9Impl_GetLOD(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_GetLOD(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture9Impl_GetLevelCount(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_GetLevelCount(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DTexture9Impl_SetAutoGenFilterType(LPDIRECT3DTEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_SetAutoGenFilterType(This->wineD3DTexture, (WINED3DTEXTUREFILTERTYPE) FilterType);
    wined3d_mutex_unlock();

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI IDirect3DTexture9Impl_GetAutoGenFilterType(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    D3DTEXTUREFILTERTYPE ret;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    ret = (D3DTEXTUREFILTERTYPE) IWineD3DTexture_GetAutoGenFilterType(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI IDirect3DTexture9Impl_GenerateMipSubLevels(LPDIRECT3DTEXTURE9 iface) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    IWineD3DTexture_GenerateMipSubLevels(This->wineD3DTexture);
    wined3d_mutex_unlock();
}

/* IDirect3DTexture9 Interface follow: */
static HRESULT WINAPI IDirect3DTexture9Impl_GetLevelDesc(LPDIRECT3DTEXTURE9 iface, UINT Level, D3DSURFACE_DESC* pDesc) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    WINED3DSURFACE_DESC wined3ddesc;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_GetLevelDesc(This->wineD3DTexture, Level, &wined3ddesc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        pDesc->Format = d3dformat_from_wined3dformat(wined3ddesc.format);
        pDesc->Type = wined3ddesc.resource_type;
        pDesc->Usage = wined3ddesc.usage;
        pDesc->Pool = wined3ddesc.pool;
        pDesc->MultiSampleType = wined3ddesc.multisample_type;
        pDesc->MultiSampleQuality = wined3ddesc.multisample_quality;
        pDesc->Width = wined3ddesc.width;
        pDesc->Height = wined3ddesc.height;
    }

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_GetSurfaceLevel(LPDIRECT3DTEXTURE9 iface, UINT Level, IDirect3DSurface9** ppSurfaceLevel) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DSurface *mySurface = NULL;

    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hrc = IWineD3DTexture_GetSurfaceLevel(This->wineD3DTexture, Level, &mySurface);
    if (hrc == D3D_OK && NULL != ppSurfaceLevel) {
       IWineD3DSurface_GetParent(mySurface, (IUnknown **)ppSurfaceLevel);
       IWineD3DSurface_Release(mySurface);
    }
    wined3d_mutex_unlock();

    return hrc;
}

static HRESULT WINAPI IDirect3DTexture9Impl_LockRect(LPDIRECT3DTEXTURE9 iface, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_LockRect(This->wineD3DTexture, Level, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_UnlockRect(LPDIRECT3DTEXTURE9 iface, UINT Level) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_UnlockRect(This->wineD3DTexture, Level);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_AddDirtyRect(LPDIRECT3DTEXTURE9 iface, CONST RECT* pDirtyRect) {
    IDirect3DTexture9Impl *This = (IDirect3DTexture9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_AddDirtyRect(This->wineD3DTexture, pDirtyRect);
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DTexture9Vtbl Direct3DTexture9_Vtbl =
{
    /* IUnknown */
    IDirect3DTexture9Impl_QueryInterface,
    IDirect3DTexture9Impl_AddRef,
    IDirect3DTexture9Impl_Release,
     /* IDirect3DResource9 */
    IDirect3DTexture9Impl_GetDevice,
    IDirect3DTexture9Impl_SetPrivateData,
    IDirect3DTexture9Impl_GetPrivateData,
    IDirect3DTexture9Impl_FreePrivateData,
    IDirect3DTexture9Impl_SetPriority,
    IDirect3DTexture9Impl_GetPriority,
    IDirect3DTexture9Impl_PreLoad,
    IDirect3DTexture9Impl_GetType,
    /* IDirect3dBaseTexture9 */
    IDirect3DTexture9Impl_SetLOD,
    IDirect3DTexture9Impl_GetLOD,
    IDirect3DTexture9Impl_GetLevelCount,
    IDirect3DTexture9Impl_SetAutoGenFilterType,
    IDirect3DTexture9Impl_GetAutoGenFilterType,
    IDirect3DTexture9Impl_GenerateMipSubLevels,
    /* IDirect3DTexture9 */
    IDirect3DTexture9Impl_GetLevelDesc,
    IDirect3DTexture9Impl_GetSurfaceLevel,
    IDirect3DTexture9Impl_LockRect,
    IDirect3DTexture9Impl_UnlockRect,
    IDirect3DTexture9Impl_AddDirtyRect
};


/* IDirect3DDevice9 IDirect3DTexture9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateTexture(LPDIRECT3DDEVICE9EX iface, UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                                    D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
    IDirect3DTexture9Impl *object;
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : W(%d) H(%d), Lvl(%d) d(%d), Fmt(%#x), Pool(%d)\n", This, Width, Height, Levels, Usage, Format,  Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DTexture9Impl));

    if (NULL == object) {
        ERR("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DTexture9_Vtbl;
    object->ref = 1;

    wined3d_mutex_lock();
    hrc = IWineD3DDevice_CreateTexture(This->WineD3DDevice, Width, Height, Levels, Usage & WINED3DUSAGE_MASK,
            wined3dformat_from_d3dformat(Format), Pool, &object->wineD3DTexture, (IUnknown *)object);
    wined3d_mutex_unlock();

    if (FAILED(hrc))
    {
        /* free up object */
        WARN("(%p) call to IWineD3DDevice_CreateTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
   } else {
        IDirect3DDevice9Ex_AddRef(iface);
        object->parentDevice = iface;
        *ppTexture= (LPDIRECT3DTEXTURE9) object;
        TRACE("(%p) Created Texture %p, %p\n", This, object, object->wineD3DTexture);
   }

   return hrc;
}
