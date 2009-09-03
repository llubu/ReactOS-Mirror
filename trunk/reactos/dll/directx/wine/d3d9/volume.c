/*
 * IDirect3DVolume9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 *                     Raphael Junqueira
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

/* IDirect3DVolume9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVolume9Impl_QueryInterface(LPDIRECT3DVOLUME9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVolume9)) {
        IDirect3DVolume9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVolume9Impl_AddRef(LPDIRECT3DVOLUME9 iface) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward refcounting */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_AddRef(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&This->ref);
        TRACE("(%p) : AddRef from %d\n", This, ref - 1);
        return ref;
    }
}

static ULONG WINAPI IDirect3DVolume9Impl_Release(LPDIRECT3DVOLUME9 iface) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward refcounting */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_Release(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);
        TRACE("(%p) : ReleaseRef to %d\n", This, ref);

        if (ref == 0) {
            wined3d_mutex_lock();
            IWineD3DVolume_Release(This->wineD3DVolume);
            wined3d_mutex_unlock();

            HeapFree(GetProcessHeap(), 0, This);
        }

        return ref;
    }
}

/* IDirect3DVolume9 Interface follow: */
static HRESULT WINAPI IDirect3DVolume9Impl_GetDevice(LPDIRECT3DVOLUME9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;
    IWineD3DDevice       *myDevice = NULL;

    TRACE("iface %p, ppDevice %p\n", iface, ppDevice);

    wined3d_mutex_lock();

    IWineD3DVolume_GetDevice(This->wineD3DVolume, &myDevice);
    IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
    IWineD3DDevice_Release(myDevice);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVolume9Impl_SetPrivateData(LPDIRECT3DVOLUME9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();

    hr = IWineD3DVolume_SetPrivateData(This->wineD3DVolume, refguid, pData, SizeOfData, Flags);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_GetPrivateData(LPDIRECT3DVOLUME9 iface, REFGUID  refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();

    hr = IWineD3DVolume_GetPrivateData(This->wineD3DVolume, refguid, pData, pSizeOfData);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_FreePrivateData(LPDIRECT3DVOLUME9 iface, REFGUID refguid) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();

    hr = IWineD3DVolume_FreePrivateData(This->wineD3DVolume, refguid);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_GetContainer(LPDIRECT3DVOLUME9 iface, REFIID riid, void** ppContainer) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;
    HRESULT res;

    TRACE("(This %p, riid %s, ppContainer %p)\n", This, debugstr_guid(riid), ppContainer);

    if (!This->container) return E_NOINTERFACE;

    if (!ppContainer) {
        ERR("Called without a valid ppContainer.\n");
    }

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("Returning ppContainer %p, *ppContainer %p\n", ppContainer, *ppContainer);

    return res;
}

static HRESULT WINAPI IDirect3DVolume9Impl_GetDesc(LPDIRECT3DVOLUME9 iface, D3DVOLUME_DESC* pDesc) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;
    WINED3DVOLUME_DESC     wined3ddesc;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();

    hr = IWineD3DVolume_GetDesc(This->wineD3DVolume, &wined3ddesc);

    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        pDesc->Format = d3dformat_from_wined3dformat(wined3ddesc.Format);
        pDesc->Type = wined3ddesc.Type;
        pDesc->Usage = wined3ddesc.Usage;
        pDesc->Pool = wined3ddesc.Pool;
        pDesc->Width = wined3ddesc.Width;
        pDesc->Height = wined3ddesc.Height;
        pDesc->Depth = wined3ddesc.Depth;
    }

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_LockBox(LPDIRECT3DVOLUME9 iface, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) relay %p %p %p %d\n", This, This->wineD3DVolume, pLockedVolume, pBox, Flags);

    wined3d_mutex_lock();

    hr = IWineD3DVolume_LockBox(This->wineD3DVolume, (WINED3DLOCKED_BOX *)pLockedVolume,
            (const WINED3DBOX *)pBox, Flags);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_UnlockBox(LPDIRECT3DVOLUME9 iface) {
    IDirect3DVolume9Impl *This = (IDirect3DVolume9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) relay %p\n", This, This->wineD3DVolume);

    wined3d_mutex_lock();

    hr = IWineD3DVolume_UnlockBox(This->wineD3DVolume);

    wined3d_mutex_unlock();

    return hr;
}

const IDirect3DVolume9Vtbl Direct3DVolume9_Vtbl =
{
    /* IUnknown */
    IDirect3DVolume9Impl_QueryInterface,
    IDirect3DVolume9Impl_AddRef,
    IDirect3DVolume9Impl_Release,
    /* IDirect3DVolume9 */
    IDirect3DVolume9Impl_GetDevice,
    IDirect3DVolume9Impl_SetPrivateData,
    IDirect3DVolume9Impl_GetPrivateData,
    IDirect3DVolume9Impl_FreePrivateData,
    IDirect3DVolume9Impl_GetContainer,
    IDirect3DVolume9Impl_GetDesc,
    IDirect3DVolume9Impl_LockBox,
    IDirect3DVolume9Impl_UnlockBox
};

ULONG WINAPI D3D9CB_DestroyVolume(IWineD3DVolume *pVolume) {
    IDirect3DVolume9Impl* volumeParent;

    IWineD3DVolume_GetParent(pVolume, (IUnknown **) &volumeParent);
    /* GetParent's AddRef was forwarded to an object in destruction.
     * Releasing it here again would cause an endless recursion. */
    volumeParent->forwardReference = NULL;
    return IDirect3DVolume9_Release((IDirect3DVolume9*) volumeParent);
}
