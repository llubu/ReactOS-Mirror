/*
 * IDirect3DVertexBuffer9 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
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

/* IDirect3DVertexBuffer9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVertexBuffer9Impl_QueryInterface(LPDIRECT3DVERTEXBUFFER9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DVertexBuffer9)) {
        IDirect3DVertexBuffer9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexBuffer9Impl_AddRef(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DVertexBuffer9Impl_Release(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        wined3d_mutex_lock();
        IWineD3DBuffer_Release(This->wineD3DVertexBuffer);
        wined3d_mutex_unlock();

        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVertexBuffer9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetDevice(LPDIRECT3DVERTEXBUFFER9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    IWineD3DDevice *wined3d_device;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_GetDevice(This->wineD3DVertexBuffer, &wined3d_device);
    if (SUCCEEDED(hr))
    {
        IWineD3DDevice_GetParent(wined3d_device, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(wined3d_device);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_SetPrivateData(This->wineD3DVertexBuffer, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_GetPrivateData(This->wineD3DVertexBuffer, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_FreePrivateData(This->wineD3DVertexBuffer, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DVertexBuffer9Impl_SetPriority(LPDIRECT3DVERTEXBUFFER9 iface, DWORD PriorityNew) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_SetPriority(This->wineD3DVertexBuffer, PriorityNew);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DVertexBuffer9Impl_GetPriority(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_GetPriority(This->wineD3DVertexBuffer);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DVertexBuffer9Impl_PreLoad(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    IWineD3DBuffer_PreLoad(This->wineD3DVertexBuffer);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer9Impl_GetType(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    TRACE("(%p)\n", This);

    return D3DRTYPE_VERTEXBUFFER;
}

/* IDirect3DVertexBuffer9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexBuffer9Impl_Lock(LPDIRECT3DVERTEXBUFFER9 iface, UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_Map(This->wineD3DVertexBuffer, OffsetToLock, SizeToLock, (BYTE **)ppbData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_Unlock(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_Unmap(This->wineD3DVertexBuffer);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetDesc(LPDIRECT3DVERTEXBUFFER9 iface, D3DVERTEXBUFFER_DESC* pDesc) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    WINED3DBUFFER_DESC desc;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_GetDesc(This->wineD3DVertexBuffer, &desc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) {
        pDesc->Format = D3DFMT_VERTEXDATA;
        pDesc->Usage = desc.Usage;
        pDesc->Pool = desc.Pool;
        pDesc->Size = desc.Size;
        pDesc->Type = D3DRTYPE_VERTEXBUFFER;
        pDesc->FVF = This->fvf;
    }


    return hr;
}

static const IDirect3DVertexBuffer9Vtbl Direct3DVertexBuffer9_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexBuffer9Impl_QueryInterface,
    IDirect3DVertexBuffer9Impl_AddRef,
    IDirect3DVertexBuffer9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DVertexBuffer9Impl_GetDevice,
    IDirect3DVertexBuffer9Impl_SetPrivateData,
    IDirect3DVertexBuffer9Impl_GetPrivateData,
    IDirect3DVertexBuffer9Impl_FreePrivateData,
    IDirect3DVertexBuffer9Impl_SetPriority,
    IDirect3DVertexBuffer9Impl_GetPriority,
    IDirect3DVertexBuffer9Impl_PreLoad,
    IDirect3DVertexBuffer9Impl_GetType,
    /* IDirect3DVertexBuffer9 */
    IDirect3DVertexBuffer9Impl_Lock,
    IDirect3DVertexBuffer9Impl_Unlock,
    IDirect3DVertexBuffer9Impl_GetDesc
};


/* IDirect3DDevice9 IDirect3DVertexBuffer9 Methods follow: */
HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexBuffer(IDirect3DDevice9Ex *iface, UINT Size, DWORD Usage,
        DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle)
{
    IDirect3DVertexBuffer9Impl *object;
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hrc = D3D_OK;

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexBuffer9Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVertexBuffer9_Vtbl;
    object->ref = 1;
    object->fvf = FVF;

    wined3d_mutex_lock();
    hrc = IWineD3DDevice_CreateVertexBuffer(This->WineD3DDevice, Size, Usage & WINED3DUSAGE_MASK,
            0 /* fvf for ddraw only */, (WINED3DPOOL) Pool, &(object->wineD3DVertexBuffer), (IUnknown *)object);
    wined3d_mutex_unlock();

    if (hrc != D3D_OK) {

        /* free up object */
        WARN("(%p) call to IWineD3DDevice_CreateVertexBuffer failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        IDirect3DDevice9Ex_AddRef(iface);
        object->parentDevice = iface;
        TRACE("(%p) : Created vertex buffer %p\n", This, object);
        *ppVertexBuffer = (LPDIRECT3DVERTEXBUFFER9) object;
    }
    return hrc;
}
