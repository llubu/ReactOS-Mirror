/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_impl.c
 * PURPOSE:         IDirect3D9 implementation
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include "d3d9_common.h"
#include <d3d9.h>
#include <debug.h>
#include "d3d9_helpers.h"
#include "adapter.h"
#include "format.h"

/* IDirect3D9: IUnknown implementation */
static HRESULT WINAPI IDirect3D9Impl_QueryInterface(LPDIRECT3D9 iface, REFIID riid, LPVOID* ppvObject)
{
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirect3D9))
    {
        IUnknown_AddRef(iface);
        *ppvObject = &This->lpVtbl;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3D9Impl_AddRef(LPDIRECT3D9 iface)
{
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    ULONG ref = InterlockedIncrement(&This->dwRefCnt);

    return ref;
}

static ULONG WINAPI IDirect3D9Impl_Release(LPDIRECT3D9 iface)
{
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    ULONG ref = InterlockedDecrement(&This->dwRefCnt);

    if (ref == 0)
    {
        EnterCriticalSection(&This->d3d9_cs);
        /* TODO: Free resources here */
        LeaveCriticalSection(&This->d3d9_cs);
        AlignedFree(This);
    }

    return ref;
}

/* IDirect3D9 interface */
static HRESULT WINAPI IDirect3D9Impl_RegisterSoftwareDevice(LPDIRECT3D9 iface, void* pInitializeFunction)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/*++
* @name IDirect3D9::GetAdapterCount
* @implemented
*
* The function IDirect3D9Impl_GetAdapterCount returns the number of adapters
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D object returned from Direct3DCreate9()
*
* @return UINT
* The number of display adapters on the system when Direct3DCreate9() was called.
*
*/
static UINT WINAPI IDirect3D9Impl_GetAdapterCount(LPDIRECT3D9 iface)
{
    UINT NumDisplayAdapters;

    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    LOCK_D3D9();

    NumDisplayAdapters = This->NumDisplayAdapters;

    UNLOCK_D3D9();
    return NumDisplayAdapters;
}

/*++
* @name IDirect3D9::GetAdapterIdentifier
* @implemented
*
* The function IDirect3D9Impl_GetAdapterIdentifier gathers information about
* a specified display adapter and fills the pIdentifier argument with the available information.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D object returned from Direct3DCreate9()
*
* @param UINT Adapter
* Adapter index to get information about. D3DADAPTER_DEFAULT is the primary display.
* The maximum value for this is the value returned by IDirect3D::GetAdapterCount().
*
* @param DWORD Flags
* Ignored at the moment, but the only valid flag is D3DENUM_WHQL_LEVEL
*
* @param D3DADAPTER_IDENTIFIER9* pIdentifier
* Pointer to a D3DADAPTER_IDENTIFIER9 structure to be filled with the available information
* about the display adapter.
*
* @return HRESULT
* If the method successfully fills the pIdentified structure, the return value is D3D_OK.
* If Adapter is out of range, Flags is invalid or pIdentifier is a bad pointer, the return value
* will be D3DERR_INVALIDCALL.
*
*/
HRESULT WINAPI IDirect3D9Impl_GetAdapterIdentifier(LPDIRECT3D9 iface, UINT Adapter, DWORD Flags,
                                                          D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    LOCK_D3D9();

    if (Adapter >= This->NumDisplayAdapters)
    {
        DPRINT1("Invalid Adapter number specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (Flags & ~D3DENUM_WHQL_LEVEL)
    {
        DPRINT1("Invalid Flags specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (IsBadWritePtr(pIdentifier, sizeof(D3DADAPTER_IDENTIFIER9)))
    {
        DPRINT1("Invalid pIdentifier parameter specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    memset(pIdentifier, 0, sizeof(D3DADAPTER_IDENTIFIER9));

    if (FALSE == GetAdapterInfo(This->DisplayAdapters[Adapter].szDeviceName, pIdentifier))
    {
        DPRINT1("Internal error: Couldn't get the adapter info for device (%d): %s", Adapter, This->DisplayAdapters[Adapter].szDeviceName);
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    UNLOCK_D3D9();
    return D3D_OK;
}

/*++
* @name IDirect3D9::GetAdapterModeCount
* @implemented
*
* The function IDirect3D9Impl_GetAdapterModeCount looks if the specified display adapter supports
* a specific pixel format and counts the available display modes for that format.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D object returned from Direct3DCreate9()
*
* @param UINT Adapter
* Adapter index to get information about. D3DADAPTER_DEFAULT is the primary display.
* The maximum value for this is the value returned by IDirect3D::GetAdapterCount().
*
* @param D3DFORMAT Format
* The pixel format to search for
*
* @return HRESULT
* If the method is successful it returns the number of display modes with the specified Format.
* If Adapter is out of range, the return value will be 0.
*
*/
static UINT WINAPI IDirect3D9Impl_GetAdapterModeCount(LPDIRECT3D9 iface, UINT Adapter, D3DFORMAT Format)
{
    UINT AdapterModeCount;

    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    LOCK_D3D9();

    if (Adapter >= This->NumDisplayAdapters)
    {
        DPRINT1("Invalid Adapter number specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (Format != D3DFMT_A2R10G10B10)
    {
        AdapterModeCount = GetDisplayFormatCount(
            Format,
            This->DisplayAdapters[Adapter].pSupportedD3DFormats,
            This->DisplayAdapters[Adapter].NumSupportedD3DFormats);
    }
    else
    {
        AdapterModeCount = GetDisplayFormatCount(
            Format,
            This->DisplayAdapters[Adapter].pSupportedD3DExtendedFormats,
            This->DisplayAdapters[Adapter].NumSupportedD3DExtendedFormats);
    }

    UNLOCK_D3D9();
    return AdapterModeCount;
}

/*++
* @name IDirect3D9::EnumAdapterModes
* @implemented
*
* The function IDirect3D9Impl_EnumAdapterModes looks if the specified display adapter supports
* a specific pixel format and fills the pMode argument with the available display modes for that format.
* This function is often used in a loop to enumerate all the display modes the adapter supports.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D object returned from Direct3DCreate9()
*
* @param UINT Adapter
* Adapter index to get information about. D3DADAPTER_DEFAULT is the primary display.
* The maximum value for this is the value returned by IDirect3D::GetAdapterCount().
*
* @param D3DFORMAT Format
* The pixel format to search for
*
* @param UINT Mode
* Index within the pixel format to be returned.
* The maximym value for this is the value returned by IDirect3D9::GetAdapterModeCount().
*
* @param D3DDISPLAYMODE* pMode
* Pointer to a D3DDISPLAYMODE structure to be filled with the display mode information
* for the specified format.
*
* @return HRESULT
* If the method successfully fills the pMode structure, the return value is D3D_OK.
* If Adapter is out of range, pMode is a bad pointer or, no modes for the specified
* format was found or the mode parameter was invalid - the return value will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI IDirect3D9Impl_EnumAdapterModes(LPDIRECT3D9 iface, UINT Adapter, D3DFORMAT Format,
                                                      UINT Mode, D3DDISPLAYMODE* pMode)
{
    const D3DDISPLAYMODE* pMatchingDisplayFormat;
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    LOCK_D3D9();

    if (Adapter >= This->NumDisplayAdapters)
    {
        DPRINT1("Invalid Adapter number specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (IsBadWritePtr(pMode, sizeof(D3DDISPLAYMODE)))
    {
        DPRINT1("Invalid pMode parameter specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (Format != D3DFMT_A2R10G10B10)
    {
        pMatchingDisplayFormat = FindDisplayFormat(
            Format,
            Mode,
            This->DisplayAdapters[Adapter].pSupportedD3DFormats,
            This->DisplayAdapters[Adapter].NumSupportedD3DFormats);
    }
    else
    {
        pMatchingDisplayFormat = FindDisplayFormat(
            Format,
            Mode,
            This->DisplayAdapters[Adapter].pSupportedD3DExtendedFormats,
            This->DisplayAdapters[Adapter].NumSupportedD3DExtendedFormats);
    }

    if (pMatchingDisplayFormat != NULL)
    {
        *pMode = *pMatchingDisplayFormat;
    }
    UNLOCK_D3D9();


    if (pMatchingDisplayFormat == NULL)
        return D3DERR_INVALIDCALL;

    return D3D_OK;
}

/*++
* @name IDirect3D9::GetAdapterDisplayMode
* @implemented
*
* The function IDirect3D9Impl_GetAdapterDisplayMode fills the pMode argument with the
* currently set display mode.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D object returned from Direct3DCreate9()
*
* @param UINT Adapter
* Adapter index to get information about. D3DADAPTER_DEFAULT is the primary display.
* The maximum value for this is the value returned by IDirect3D::GetAdapterCount().
*
* @param D3DDISPLAYMODE* pMode
* Pointer to a D3DDISPLAYMODE structure to be filled with the current display mode information.
*
* @return HRESULT
* If the method successfully fills the pMode structure, the return value is D3D_OK.
* If Adapter is out of range or pMode is a bad pointer, the return value will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI IDirect3D9Impl_GetAdapterDisplayMode(LPDIRECT3D9 iface, UINT Adapter, D3DDISPLAYMODE* pMode)
{
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    LOCK_D3D9();

    if (Adapter >= This->NumDisplayAdapters)
    {
        DPRINT1("Invalid Adapter number specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (IsBadWritePtr(pMode, sizeof(D3DDISPLAYMODE)))
    {
        DPRINT1("Invalid pMode parameter specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    /* TODO: Handle (This->DisplayAdapters[Adapter].bInUseFlag == FALSE) */
    if (FALSE == GetAdapterMode(This->DisplayAdapters[Adapter].szDeviceName, pMode))
        DPRINT1("Internal error, GetAdapterMode() failed.");

    UNLOCK_D3D9();
    return D3D_OK;
}


/*++
* @name IDirect3D9::CheckDeviceType
* @implemented
*
* The function IDirect3D9Impl_CheckDeviceType checks if a specific D3DFORMAT is hardware accelerated
* on the specified display adapter.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D object returned from Direct3DCreate9()
*
* @param UINT Adapter
* Adapter index to get information about. D3DADAPTER_DEFAULT is the primary display.
* The maximum value for this is the value returned by IDirect3D::GetAdapterCount().
*
* @param D3DDEVTYPE DeviceType
* One of the D3DDEVTYPE enum members.
*
* @param D3DFORMAT DisplayFormat
* One of the D3DFORMAT enum members except D3DFMT_UNKNOWN for the display adapter mode to be checked.
*
* @param D3DFORMAT BackBufferFormat
* One of the D3DFORMAT enum membersfor the render target mode to be checked. D3DFMT_UNKNOWN is only allowed in windowed mode.
*
* @param BOOL Windowed
* If this value is TRUE, the D3DFORMAT check will be done for windowed mode and FALSE equals fullscreen mode.
*
* @return HRESULT
* If the format is hardware accelerated, the method returns D3D_OK.
* If the format isn't hardware accelerated or unsupported - the return value will be D3DERR_NOTAVAILABLE.
* If Adapter is out of range, DeviceType is invalid,
* DisplayFormat or BackBufferFormat is invalid - the return value will be D3DERR_INVALIDCALL.
*
*/
static HRESULT WINAPI IDirect3D9Impl_CheckDeviceType(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                     D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed)
{
    HRESULT hResult;

    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    LOCK_D3D9();

    if (Adapter >= This->NumDisplayAdapters)
    {
        DPRINT1("Invalid Adapter number specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (DeviceType != D3DDEVTYPE_HAL &&
        DeviceType != D3DDEVTYPE_REF &&
        DeviceType != D3DDEVTYPE_SW)
    {
        DPRINT1("Invalid DeviceType specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (BackBufferFormat == D3DFMT_UNKNOWN &&
        Windowed == TRUE)
    {
        BackBufferFormat = DisplayFormat;
    }

    if (DisplayFormat == D3DFMT_UNKNOWN && BackBufferFormat == D3DFMT_UNKNOWN)
    {
        DPRINT1("Invalid D3DFORMAT specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (FALSE == IsBackBufferFormat(BackBufferFormat))
    {
        DPRINT1("Invalid D3DFORMAT specified");
        UNLOCK_D3D9();
        return D3DERR_NOTAVAILABLE;
    }

    if (TRUE == Windowed && TRUE == IsExtendedFormat(DisplayFormat))
    {
        DPRINT1("Extended diplay modes can only be used in fullscreen mode");
        UNLOCK_D3D9();
        return D3DERR_NOTAVAILABLE;
    }

    hResult = CheckDeviceFormat(&This->DisplayAdapters[Adapter].DriverCaps, DisplayFormat, BackBufferFormat, Windowed);

    UNLOCK_D3D9();
    return hResult;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceFormat(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                       D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType,
                                                       D3DFORMAT CheckFormat)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceMultiSampleType(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                                D3DFORMAT SurfaceFormat, BOOL Windowed,
                                                                D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDepthStencilMatch(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                            D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat,
                                                            D3DFORMAT DepthStencilFormat)
{
    UNIMPLEMENTED

    return D3D_OK;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceFormatConversion(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                                 D3DFORMAT SourceFormat, D3DFORMAT TargetFormat)
{
    UNIMPLEMENTED

    return D3D_OK;
}


/*++
* @name IDirect3D9::GetDeviceCaps
* @implemented
*
* The function IDirect3D9Impl_GetDeviceCaps fills the pCaps argument with the
* capabilities of the specified adapter and device type.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D object returned from Direct3DCreate9()
*
* @param UINT Adapter
* Adapter index to get information about. D3DADAPTER_DEFAULT is the primary display.
* The maximum value for this is the value returned by IDirect3D::GetAdapterCount().
*
* @param D3DDEVTYPE DeviceType
* One of the D3DDEVTYPE enum members.
* NOTE: Currently only D3DDEVTYPE_HAL is implemented.
*
* @param D3DCAPS9* pCaps
* Pointer to a D3DCAPS9 structure to be filled with the adapter's device type capabilities.
*
* @return HRESULT
* If the method successfully fills the pCaps structure, the return value is D3D_OK.
* If Adapter is out of range or pCaps is a bad pointer, the return value will be D3DERR_INVALIDCALL.
* If DeviceType is invalid, the return value will be D3DERR_INVALIDDEVICE.
*
*/
static HRESULT WINAPI IDirect3D9Impl_GetDeviceCaps(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps)
{
    HRESULT hResult;
    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    LOCK_D3D9();

    if (Adapter >= This->NumDisplayAdapters)
    {
        DPRINT1("Invalid Adapter number specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    if (IsBadWritePtr(pCaps, sizeof(D3DCAPS9)))
    {
        DPRINT1("Invalid pCaps parameter specified");
        UNLOCK_D3D9();
        return D3DERR_INVALIDCALL;
    }

    hResult = GetAdapterCaps(&This->DisplayAdapters[Adapter], DeviceType, pCaps);

    UNLOCK_D3D9();
    return hResult;
}

/*++
* @name IDirect3D9::GetAdapterMonitor
* @implemented
*
* The function IDirect3D9Impl_GetAdapterMonitor returns the monitor associated
* with the specified display adapter.
*
* @param LPDIRECT3D iface
* Pointer to the IDirect3D object returned from Direct3DCreate9()
*
* @param UINT Adapter
* Adapter index to get information about. D3DADAPTER_DEFAULT is the primary display.
* The maximum value for this is the value returned by IDirect3D::GetAdapterCount().
*
* @return HMONITOR
* If the method successfully it returns the HMONITOR belonging to the specified adapter.
* If the method fails, the return value is NULL.
*
*/
static HMONITOR WINAPI IDirect3D9Impl_GetAdapterMonitor(LPDIRECT3D9 iface, UINT Adapter)
{
    HMONITOR hAdapterMonitor = NULL;

    LPDIRECT3D9_INT This = impl_from_IDirect3D9(iface);
    LOCK_D3D9();

    if (Adapter < This->NumDisplayAdapters)
    {
        hAdapterMonitor = GetAdapterMonitor(This->DisplayAdapters[Adapter].szDeviceName);
    }
    else
    {
        DPRINT1("Invalid Adapter number specified");
    }

    UNLOCK_D3D9();
    return hAdapterMonitor;
}

static HRESULT WINAPI IDirect3D9Impl_CreateDevice(LPDIRECT3D9 iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                  HWND hFocusWindow, DWORD BehaviourFlags,
                                                  D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                  struct IDirect3DDevice9** ppReturnedDeviceInterface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

IDirect3D9Vtbl Direct3D9_Vtbl =
{
    /* IUnknown */
    IDirect3D9Impl_QueryInterface,
    IDirect3D9Impl_AddRef,
    IDirect3D9Impl_Release,

    /* IDirect3D9 */
    IDirect3D9Impl_RegisterSoftwareDevice,
    IDirect3D9Impl_GetAdapterCount,
    IDirect3D9Impl_GetAdapterIdentifier,
    IDirect3D9Impl_GetAdapterModeCount,
    IDirect3D9Impl_EnumAdapterModes,
    IDirect3D9Impl_GetAdapterDisplayMode,
    IDirect3D9Impl_CheckDeviceType,
    IDirect3D9Impl_CheckDeviceFormat,
    IDirect3D9Impl_CheckDeviceMultiSampleType,
    IDirect3D9Impl_CheckDepthStencilMatch,
    IDirect3D9Impl_CheckDeviceFormatConversion,
    IDirect3D9Impl_GetDeviceCaps,
    IDirect3D9Impl_GetAdapterMonitor,
    IDirect3D9Impl_CreateDevice
};
