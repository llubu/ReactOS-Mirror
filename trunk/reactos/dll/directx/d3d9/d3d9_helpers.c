/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_helpers.c
 * PURPOSE:         d3d9.dll helper functions
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */

#include <d3d9.h>
#include "d3d9_helpers.h"
#include <stdio.h>
#include <ddraw.h>
#include <debug.h>

#define MEM_ALIGNMENT 0x20

static LPCSTR D3D9_DebugRegPath = "Software\\Microsoft\\Direct3D";

LPDIRECT3D9_INT impl_from_IDirect3D9(LPDIRECT3D9 iface)
{
    if (IsBadWritePtr(iface, sizeof(LPDIRECT3D9_INT)))
        return NULL;

    return (LPDIRECT3D9_INT)((ULONG_PTR)iface - FIELD_OFFSET(DIRECT3D9_INT, lpVtbl));
}

BOOL ReadRegistryValue(IN DWORD ValueType, IN LPCSTR ValueName, OUT LPBYTE DataBuffer, IN OUT LPDWORD DataBufferSize)
{
    HKEY hKey;
    DWORD Type;
    LONG Ret;

    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE, D3D9_DebugRegPath, 0, KEY_QUERY_VALUE, &hKey))
        return FALSE;

    Ret = RegQueryValueExA(hKey, ValueName, 0, &Type, DataBuffer, DataBufferSize);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != Ret)
        return FALSE;

    if (Type != ValueType)
        return FALSE;

    return TRUE;
}

HRESULT AlignedAlloc(IN OUT LPVOID *ppObject, IN SIZE_T dwSize)
{
    ULONG AddressOffset;
    ULONG AlignedMask = MEM_ALIGNMENT - 1;
    CHAR *AlignedPtr;
    ULONG_PTR *AlignedOffsetPtr;

    if (ppObject == 0)
        return DDERR_INVALIDPARAMS;

    if (dwSize == 0)
    {
        *ppObject = NULL;
        return S_OK;
    }

    dwSize += MEM_ALIGNMENT;

    AlignedPtr = (CHAR *)LocalAlloc(LMEM_ZEROINIT, dwSize);

    if (AlignedPtr == 0)
        return DDERR_OUTOFMEMORY;

    AddressOffset = MEM_ALIGNMENT - ((ULONG)AlignedPtr & AlignedMask);
	
    AlignedPtr += AddressOffset;

    AlignedOffsetPtr = (ULONG_PTR *)(AlignedPtr - sizeof(ULONG));
    *AlignedOffsetPtr = AddressOffset;

    *ppObject = (ULONG_PTR *)AlignedPtr;

    return S_OK;
}

VOID AlignedFree(IN OUT LPVOID pObject)
{
    CHAR *NonAlignedPtr = pObject;
    ULONG_PTR *AlignedPtr = pObject;

    if (pObject == 0)
        return;

    NonAlignedPtr -= *(AlignedPtr - 1);

    LocalFree(NonAlignedPtr);
}
