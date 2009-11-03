/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/primary.c
 * PURPOSE:         Primary IDirectSoundBuffer8 implementation
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */


#include "precomp.h"

typedef struct
{
    const IDirectSoundBuffer8Vtbl *lpVtbl;
    LONG ref;

    LPFILTERINFO Filter;
    DWORD dwLevel;
    WAVEFORMATEX Format;
    HANDLE hPin;
    CRITICAL_SECTION Lock;
    KSSTATE State;
}CDirectSoundBuffer, *LPCDirectSoundBuffer;

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnQueryInterface(
    LPDIRECTSOUNDBUFFER8 iface,
    IN REFIID riid,
    LPVOID* ppobj)
{
    LPOLESTR pStr;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectSoundBuffer) ||
        IsEqualIID(riid, &IID_IDirectSoundBuffer8))
    {
        *ppobj = (LPVOID)&This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }

    if (SUCCEEDED(StringFromIID(riid, &pStr)))
    {
        DPRINT("No Interface for class %s\n", pStr);
        CoTaskMemFree(pStr);
    }
    return E_NOINTERFACE;
}

ULONG
WINAPI
PrimaryDirectSoundBuffer8Impl_fnAddRef(
    LPDIRECTSOUNDBUFFER8 iface)
{
    ULONG ref;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    ref = InterlockedIncrement(&This->ref);

    return ref;

}

ULONG
WINAPI
PrimaryDirectSoundBuffer8Impl_fnRelease(
    LPDIRECTSOUNDBUFFER8 iface)
{
    ULONG ref;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    ref = InterlockedDecrement(&(This->ref));

    if (!ref)
    {
        if (This->hPin)
        {
            /* close pin handle */
            CloseHandle(This->hPin);
        }
        /* free primary buffer */
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnGetCaps(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDSBCAPS pDSBufferCaps)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnGetCurrentPosition(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDWORD pdwCurrentPlayCursor,
    LPDWORD pdwCurrentWriteCursor)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnGetFormat(
    LPDIRECTSOUNDBUFFER8 iface,
    LPWAVEFORMATEX pwfxFormat, 
    DWORD dwSizeAllocated, 
    LPDWORD pdwSizeWritten)
{
    DWORD FormatSize;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    FormatSize = sizeof(WAVEFORMATEX) + This->Format.cbSize;

    if (!pwfxFormat && !pdwSizeWritten)
    {
        /* invalid parameter */
        return DSERR_INVALIDPARAM;
    }

    if (!pwfxFormat)
    {
        /* return required format size */
        *pdwSizeWritten = FormatSize;
        return DS_OK;
    }
    else
    {
        if (dwSizeAllocated >= FormatSize)
        {
            /* copy format */
            CopyMemory(pwfxFormat, &This->Format, FormatSize);

            if (pdwSizeWritten)
                *pdwSizeWritten = FormatSize;

            return DS_OK;
        }
        /* buffer too small */
        if (pdwSizeWritten)
            *pdwSizeWritten = 0;

        return DSERR_INVALIDPARAM;
    }
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnGetVolume(
    LPDIRECTSOUNDBUFFER8 iface,
    LPLONG plVolume)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnGetPan(
    LPDIRECTSOUNDBUFFER8 iface,
    LPLONG plPan)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnGetFrequency(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDWORD pdwFrequency)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnGetStatus(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDWORD pdwStatus)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnInitialize(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDIRECTSOUND pDirectSound,
    LPCDSBUFFERDESC pcDSBufferDesc)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnLock(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwOffset,
    DWORD dwBytes,
    LPVOID *ppvAudioPtr1,
    LPDWORD pdwAudioBytes1,
    LPVOID *ppvAudioPtr2, 
    LPDWORD pdwAudioBytes2,
    DWORD dwFlags)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnPlay(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwReserved1,
    DWORD dwPriority,
    DWORD dwFlags)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnSetCurrentPosition(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwNewPosition)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnSetFormat(
    LPDIRECTSOUNDBUFFER8 iface,
    LPCWAVEFORMATEX pcfxFormat)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (This->dwLevel == DSSCL_NORMAL)
    {
        /* can't change format with this level */
        return DSERR_PRIOLEVELNEEDED;
    }

    ASSERT(pcfxFormat->cbSize == 0);


    DPRINT("This %p Format: Tag %x nChannels %u nSamplesPerSec %u nAvgBytesPerSec %u nBlockAlign %u wBitsPerSample %u cbSize %u\n", This, 
          pcfxFormat->wFormatTag, pcfxFormat->nChannels, pcfxFormat->nSamplesPerSec, pcfxFormat->nAvgBytesPerSec, pcfxFormat->nBlockAlign, pcfxFormat->wBitsPerSample, pcfxFormat->cbSize);

    CopyMemory(&This->Format, pcfxFormat, sizeof(WAVEFORMATEX));

    return DS_OK;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnSetVolume(
    LPDIRECTSOUNDBUFFER8 iface,
    LONG lVolume)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnSetPan(
    LPDIRECTSOUNDBUFFER8 iface,
    LONG lPan)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnSetFrequency(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwFrequency)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnStop(
    LPDIRECTSOUNDBUFFER8 iface)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}


HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnUnlock(
    LPDIRECTSOUNDBUFFER8 iface,
    LPVOID pvAudioPtr1,
    DWORD dwAudioBytes1,
    LPVOID pvAudioPtr2,
    DWORD dwAudioBytes2)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}




HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnRestore(
    LPDIRECTSOUNDBUFFER8 iface)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}


HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnSetFX(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwEffectsCount, 
    LPDSEFFECTDESC pDSFXDesc,
    LPDWORD pdwResultCodes)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnAcquireResources(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwFlags,
    DWORD dwEffectsCount, 
    LPDWORD pdwResultCodes)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
PrimaryDirectSoundBuffer8Impl_fnGetObjectInPath(
    LPDIRECTSOUNDBUFFER8 iface,
    REFGUID rguidObject,
    DWORD dwIndex,
    REFGUID rguidInterface,
    LPVOID *ppObject)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

static IDirectSoundBuffer8Vtbl vt_DirectSoundBuffer8 =
{
    /* IUnknown methods */
    PrimaryDirectSoundBuffer8Impl_fnQueryInterface,
    PrimaryDirectSoundBuffer8Impl_fnAddRef,
    PrimaryDirectSoundBuffer8Impl_fnRelease,
    /* IDirectSoundBuffer methods */
    PrimaryDirectSoundBuffer8Impl_fnGetCaps,
    PrimaryDirectSoundBuffer8Impl_fnGetCurrentPosition,
    PrimaryDirectSoundBuffer8Impl_fnGetFormat,
    PrimaryDirectSoundBuffer8Impl_fnGetVolume,
    PrimaryDirectSoundBuffer8Impl_fnGetPan,
    PrimaryDirectSoundBuffer8Impl_fnGetFrequency,
    PrimaryDirectSoundBuffer8Impl_fnGetStatus,
    PrimaryDirectSoundBuffer8Impl_fnInitialize,
    PrimaryDirectSoundBuffer8Impl_fnLock,
    PrimaryDirectSoundBuffer8Impl_fnPlay,
    PrimaryDirectSoundBuffer8Impl_fnSetCurrentPosition,
    PrimaryDirectSoundBuffer8Impl_fnSetFormat,
    PrimaryDirectSoundBuffer8Impl_fnSetVolume,
    PrimaryDirectSoundBuffer8Impl_fnSetPan,
    PrimaryDirectSoundBuffer8Impl_fnSetFrequency,
    PrimaryDirectSoundBuffer8Impl_fnStop,
    PrimaryDirectSoundBuffer8Impl_fnUnlock,
    PrimaryDirectSoundBuffer8Impl_fnRestore,
    /* IDirectSoundBuffer8 methods */
    PrimaryDirectSoundBuffer8Impl_fnSetFX,
    PrimaryDirectSoundBuffer8Impl_fnAcquireResources,
    PrimaryDirectSoundBuffer8Impl_fnGetObjectInPath
};

DWORD
PrimaryDirectSoundBuffer_Write(
    LPDIRECTSOUNDBUFFER8 iface,
    LPVOID Buffer,
    DWORD  BufferSize)
{
    KSSTREAM_HEADER Header;
    DWORD Result, BytesTransferred;
    OVERLAPPED Overlapped;

    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


    ASSERT(This->hPin);
    ZeroMemory(&Header, sizeof(KSSTREAM_HEADER));

    Header.FrameExtent = BufferSize;
    Header.DataUsed = BufferSize;
    Header.Data = Buffer;
    Header.Size = sizeof(KSSTREAM_HEADER);
    Header.PresentationTime.Numerator = 1;
    Header.PresentationTime.Denominator = 1;

    Result = DeviceIoControl(This->hPin, IOCTL_KS_WRITE_STREAM, NULL, 0, &Header, sizeof(KSSTREAM_HEADER), &BytesTransferred, &Overlapped);

    if (Result != ERROR_SUCCESS)
        return 0;

    return BytesTransferred;
}

VOID
PrimaryDirectSoundBuffer_SetState(
    LPDIRECTSOUNDBUFFER8 iface,
    KSSTATE State)
{
    KSPROPERTY Property;
    DWORD Result, BytesTransferred;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (This->State == State)
        return;

    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    Result = SyncOverlappedDeviceIoControl(This->hPin, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesTransferred);
    if (Result == ERROR_SUCCESS)
    {
        This->State = State;
    }
}

HRESULT
PrimaryDirectSoundBuffer_GetPosition(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDWORD pdwCurrentPlayCursor,
    LPDWORD pdwCurrentWriteCursor)
{
    KSAUDIO_POSITION Position;
    KSPROPERTY Request;
    DWORD Result;

    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (!This->hPin)
    {
        if (pdwCurrentPlayCursor)
            *pdwCurrentPlayCursor = 0;

        if (pdwCurrentWriteCursor)
            *pdwCurrentWriteCursor = 0;

        DPRINT("No Audio Pin\n");
        return DS_OK;
    }

    /* setup audio position property request */
    Request.Id = KSPROPERTY_AUDIO_POSITION;
    Request.Set = KSPROPSETID_Audio;
    Request.Flags = KSPROPERTY_TYPE_GET;


    Result = SyncOverlappedDeviceIoControl(This->hPin, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSPROPERTY), (PVOID)&Position, sizeof(KSAUDIO_POSITION), NULL);

    if (Result != ERROR_SUCCESS)
    {
        DPRINT("GetPosition failed with %x\n", Result);
        return DSERR_UNSUPPORTED;
    }

    //DPRINT("Play %I64u Write %I64u \n", Position.PlayOffset, Position.WriteOffset);

    if (pdwCurrentPlayCursor)
        *pdwCurrentPlayCursor = (DWORD)Position.PlayOffset;

    if (pdwCurrentWriteCursor)
        *pdwCurrentWriteCursor = (DWORD)Position.WriteOffset;

    return DS_OK;
}

HRESULT
PrimaryDirectSoundBuffer_SetFormat(
    LPDIRECTSOUNDBUFFER8 iface,
    LPWAVEFORMATEX pcfxFormat,
    BOOL bLooped)
{
    ULONG PinId, DeviceId = 0, Result;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (This->hPin)
    {
        /* fixme change format */
        ASSERT(0);
    }

    do
    {
        /* try all available recording pins on that filter */
        PinId = GetPinIdFromFilter(This->Filter, FALSE, DeviceId);
        DPRINT("PinId %u DeviceId %u\n", PinId, DeviceId);

        if (PinId == ULONG_MAX)
            break;

        Result = OpenPin(This->Filter->hFilter, PinId, (LPWAVEFORMATEX)pcfxFormat, &This->hPin, bLooped);
        DPRINT("PinId %u Result %u\n", PinId, Result);
        if (Result == ERROR_SUCCESS)
            break;

        This->hPin = NULL;
        DeviceId++;
    }while(TRUE);

    if (!This->hPin)
    {
        DPRINT("PrimaryDirectSoundBuffer8Impl_fnSetFormat failed\n");
        return DSERR_INVALIDPARAM;
    }

    DPRINT("PrimaryDirectSoundBuffer8Impl_fnSetFormat success\n");
    return DS_OK;
}

VOID
PrimaryDirectSoundBuffer_AcquireLock(
    LPDIRECTSOUNDBUFFER8 iface)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    EnterCriticalSection(&This->Lock);


}

VOID
PrimaryDirectSoundBuffer_ReleaseLock(
    LPDIRECTSOUNDBUFFER8 iface)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    LeaveCriticalSection(&This->Lock);

}


HRESULT
NewPrimarySoundBuffer(
    LPDIRECTSOUNDBUFFER8 *OutBuffer,
    LPFILTERINFO Filter,
    DWORD dwLevel)
{
    LPCDirectSoundBuffer This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CDirectSoundBuffer));

    if (!This)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }

    This->ref = 1;
    This->lpVtbl = &vt_DirectSoundBuffer8;
    This->Filter = Filter;
    This->dwLevel = dwLevel;
    This->hPin = NULL;

    InitializeCriticalSection(&This->Lock);

    *OutBuffer = (LPDIRECTSOUNDBUFFER8)&This->lpVtbl;
    return DS_OK;
}

