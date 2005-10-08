/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>	/* Insomnia - pow() function */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winreg.h"
#include "winternl.h"
#include "winnls.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"
#include "initguid.h"
#include "dsconf.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);


/*******************************************************************************
 *              IKsBufferPropertySet
 */

/* IUnknown methods */
static HRESULT WINAPI IKsBufferPropertySetImpl_QueryInterface(
    LPKSPROPERTYSET iface,
    REFIID riid,
    LPVOID *ppobj )
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    return IDirectSoundBuffer_QueryInterface((LPDIRECTSOUNDBUFFER8)This->dsb, riid, ppobj);
}

static ULONG WINAPI IKsBufferPropertySetImpl_AddRef(LPKSPROPERTYSET iface)
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    TRACE("(%p) ref was %ld\n", This, This->ref);
    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IKsBufferPropertySetImpl_Release(LPKSPROPERTYSET iface)
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    ULONG ulReturn;

    TRACE("(%p) ref was %ld\n", This, This->ref);
    ulReturn = InterlockedDecrement(&(This->ref));
    if (!ulReturn) {
	This->dsb->iks = 0;
	IDirectSoundBuffer_Release((LPDIRECTSOUND3DBUFFER)This->dsb);
	HeapFree(GetProcessHeap(),0,This);
	TRACE("(%p) released\n",This);
    }
    return ulReturn;
}

static HRESULT WINAPI IKsBufferPropertySetImpl_Get(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    PIDSDRIVERPROPERTYSET ps;
    TRACE("(iface=%p,guidPropSet=%s,dwPropID=%ld,pInstanceData=%p,cbInstanceData=%ld,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData,pcbReturned);

    if (This->dsb->hwbuf) {
        IDsDriver_QueryInterface(This->dsb->hwbuf, &IID_IDsDriverPropertySet, (void **)&ps);

        if (ps) {
	    DSPROPERTY prop;
	    HRESULT hres;

	    prop.s.Set = *guidPropSet;
	    prop.s.Id = dwPropID;
	    prop.s.Flags = 0;	/* unused */
	    prop.s.InstanceId = (ULONG)This->dsb->dsound;

	    hres = IDsDriverPropertySet_Get(ps, &prop, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned);

	    IDsDriverPropertySet_Release(ps);

	    return hres;
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsBufferPropertySetImpl_Set(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData )
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    PIDSDRIVERPROPERTYSET ps;
    TRACE("(%p,%s,%ld,%p,%ld,%p,%ld)\n",This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData);

    if (This->dsb->hwbuf) {
        IDsDriver_QueryInterface(This->dsb->hwbuf, &IID_IDsDriverPropertySet, (void **)&ps);

        if (ps) {
	    DSPROPERTY prop;
	    HRESULT hres;

	    prop.s.Set = *guidPropSet;
	    prop.s.Id = dwPropID;
	    prop.s.Flags = 0;	/* unused */
	    prop.s.InstanceId = (ULONG)This->dsb->dsound;
	    hres = IDsDriverPropertySet_Set(ps,&prop,pInstanceData,cbInstanceData,pPropData,cbPropData);

	    IDsDriverPropertySet_Release(ps);

	    return hres;
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsBufferPropertySetImpl_QuerySupport(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    PULONG pTypeSupport )
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    PIDSDRIVERPROPERTYSET ps;
    TRACE("(%p,%s,%ld,%p)\n",This,debugstr_guid(guidPropSet),dwPropID,pTypeSupport);

    if (This->dsb->hwbuf) {
        IDsDriver_QueryInterface(This->dsb->hwbuf, &IID_IDsDriverPropertySet, (void **)&ps);

        if (ps) {
            HRESULT hres;

            hres = IDsDriverPropertySet_QuerySupport(ps,guidPropSet, dwPropID,pTypeSupport);

            IDsDriverPropertySet_Release(ps);

            return hres;
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

static IKsPropertySetVtbl iksbvt = {
    IKsBufferPropertySetImpl_QueryInterface,
    IKsBufferPropertySetImpl_AddRef,
    IKsBufferPropertySetImpl_Release,
    IKsBufferPropertySetImpl_Get,
    IKsBufferPropertySetImpl_Set,
    IKsBufferPropertySetImpl_QuerySupport
};

HRESULT WINAPI IKsBufferPropertySetImpl_Create(
    IDirectSoundBufferImpl *dsb,
    IKsBufferPropertySetImpl **piks)
{
    IKsBufferPropertySetImpl *iks;
    TRACE("(%p,%p)\n",dsb,piks);

    iks = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*iks));
    if (iks == 0) {
        WARN("out of memory\n");
        *piks = NULL;
        return DSERR_OUTOFMEMORY;
    }

    iks->ref = 0;
    iks->dsb = dsb;
    dsb->iks = iks;
    iks->lpVtbl = &iksbvt;

    IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)dsb);

    *piks = iks;
    return S_OK;
}

HRESULT WINAPI IKsBufferPropertySetImpl_Destroy(
    IKsBufferPropertySetImpl *piks)
{
    TRACE("(%p)\n",piks);

    while (IKsBufferPropertySetImpl_Release((LPKSPROPERTYSET)piks) > 0);

    return S_OK;
}

/*******************************************************************************
 *              IKsPrivatePropertySet
 */

/* IUnknown methods */
static HRESULT WINAPI IKsPrivatePropertySetImpl_QueryInterface(
    LPKSPROPERTYSET iface,
    REFIID riid,
    LPVOID *ppobj )
{
    IKsPrivatePropertySetImpl *This = (IKsPrivatePropertySetImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    return DSERR_INVALIDPARAM;
}

static ULONG WINAPI IKsPrivatePropertySetImpl_AddRef(LPKSPROPERTYSET iface)
{
    IKsPrivatePropertySetImpl *This = (IKsPrivatePropertySetImpl *)iface;
    TRACE("(%p) ref was %ld\n", This, This->ref);
    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IKsPrivatePropertySetImpl_Release(LPKSPROPERTYSET iface)
{
    IKsPrivatePropertySetImpl *This = (IKsPrivatePropertySetImpl *)iface;
    ULONG ulReturn;

    TRACE("(%p) ref was %ld\n", This, This->ref);
    ulReturn = InterlockedDecrement(&(This->ref));
    if (ulReturn == 0) {
        HeapFree(GetProcessHeap(),0,This);
	TRACE("(%p) released\n",This);
    }
    return ulReturn;
}

static HRESULT WINAPI DSPROPERTY_WaveDeviceMappingA(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA ppd;
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) not implemented!\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA) pPropData;

    if (!ppd) {
	WARN("invalid parameter: pPropData\n");
	return DSERR_INVALIDPARAM;
    }

    FIXME("DeviceName=%s\n",ppd->DeviceName);
    FIXME("DataFlow=%s\n",
	ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "DIRECTSOUNDDEVICE_DATAFLOW_RENDER" :
	ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ? "DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE" : "UNKNOWN");

    /* FIXME: match the name to a wave device somehow. */
    ppd->DeviceId = GUID_NULL;

    if (pcbReturned) {
	*pcbReturned = cbPropData;
	FIXME("*pcbReturned=%ld\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_WaveDeviceMappingW(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA ppd;
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) not implemented!\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA) pPropData;

    if (!ppd) {
	WARN("invalid parameter: pPropData\n");
	return DSERR_INVALIDPARAM;
    }

    FIXME("DeviceName=%s\n",debugstr_w(ppd->DeviceName));
    FIXME("DataFlow=%s\n",
	ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "DIRECTSOUNDDEVICE_DATAFLOW_RENDER" :
	ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ? "DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE" : "UNKNOWN");

    /* FIXME: match the name to a wave device somehow. */
    ppd->DeviceId = GUID_NULL;

    if (pcbReturned) {
	*pcbReturned = cbPropData;
	FIXME("*pcbReturned=%ld\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_Description1(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    HRESULT err;
    GUID guid, dev_guid;
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA ppd;
    TRACE("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA) pPropData;

    if (!ppd) {
	WARN("invalid parameter: pPropData\n");
	return DSERR_INVALIDPARAM;
    }

    TRACE("DeviceId=%s\n",debugstr_guid(&ppd->DeviceId));
    if ( IsEqualGUID( &ppd->DeviceId , &GUID_NULL) ) {
	/* default device of type specified by ppd->DataFlow */
	if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	} else if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	} else {
	    TRACE("DataFlow=Unknown(%d)\n", ppd->DataFlow);
	}
	FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) GUID_NULL not implemented!\n",
	    debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
	return E_PROP_ID_UNSUPPORTED;
    }

    ppd->Type = DIRECTSOUNDDEVICE_TYPE_EMULATED;
    GetDeviceID(&ppd->DeviceId, &dev_guid);

    if ( IsEqualGUID( &ppd->DeviceId, &DSDEVID_DefaultPlayback) ||
	 IsEqualGUID( &ppd->DeviceId, &DSDEVID_DefaultVoicePlayback) ) {
	ULONG wod;
	unsigned int wodn;
	TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
	wodn = waveOutGetNumDevs();
	for (wod = 0; wod < wodn; wod++) {
            if (IsEqualGUID( &dev_guid, &renderer_guids[wod] ) ) {
                DSDRIVERDESC desc;
                ppd->WaveDeviceId = wod;
                ppd->Devnode = wod;
                err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
                if (err == DS_OK) {
                    PIDSDRIVER drv = NULL;
                    strncpy(ppd->DescriptionA, desc.szDesc, sizeof(ppd->DescriptionA) - 1);
                    strncpy(ppd->ModuleA, desc.szDrvname, sizeof(ppd->ModuleA) - 1);
                    MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, ppd->DescriptionW, sizeof(ppd->DescriptionW)/sizeof(WCHAR) );
                    MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, ppd->ModuleW, sizeof(ppd->ModuleW)/sizeof(WCHAR) );
                    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0));
                    if (err == DS_OK && drv)
                        ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
                    break;
                } else {
                    WARN("WineWaveOutMessage failed\n");
                    return E_PROP_ID_UNSUPPORTED;
                }
            }
	}
    } else if ( IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultCapture) ||
	        IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoiceCapture) ) {
	ULONG wid;
	unsigned int widn;
	TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
	widn = waveInGetNumDevs();
	for (wid = 0; wid < widn; wid++) {
            if (IsEqualGUID( &dev_guid, &guid) ) {
                DSDRIVERDESC desc;
                ppd->WaveDeviceId = wid;
                ppd->Devnode = wid;
                err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
                if (err == DS_OK) {
                    PIDSCDRIVER drv;
                    strncpy(ppd->DescriptionA, desc.szDesc, sizeof(ppd->DescriptionA) - 1);
                    strncpy(ppd->ModuleA, desc.szDrvname, sizeof(ppd->ModuleA) - 1);
                    MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, ppd->DescriptionW, sizeof(ppd->DescriptionW)/sizeof(WCHAR) );
                    MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, ppd->ModuleW, sizeof(ppd->ModuleW)/sizeof(WCHAR) );
                    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDIFACE,(DWORD)&drv,0));
                    if (err == DS_OK && drv)
                        ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
                    break;
                } else {
                    WARN("WineWaveInMessage failed\n");
                    return E_PROP_ID_UNSUPPORTED;
                }
            }
	}
    } else {
	BOOL found = FALSE;
	ULONG wod;
	unsigned int wodn;
	/* given specific device so try the render devices first */
	wodn = waveOutGetNumDevs();
	for (wod = 0; wod < wodn; wod++) {
            if (IsEqualGUID( &ppd->DeviceId, &renderer_guids[wod] ) ) {
                DSDRIVERDESC desc;
                TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
                ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
                ppd->WaveDeviceId = wod;
                ppd->Devnode = wod;
                err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
                if (err == DS_OK) {
                    PIDSDRIVER drv = NULL;
                    strncpy(ppd->DescriptionA, desc.szDesc, sizeof(ppd->DescriptionA) - 1);
                    strncpy(ppd->ModuleA, desc.szDrvname, sizeof(ppd->ModuleA) - 1);
                    MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, ppd->DescriptionW, sizeof(ppd->DescriptionW)/sizeof(WCHAR) );
                    MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, ppd->ModuleW, sizeof(ppd->ModuleW)/sizeof(WCHAR) );
                    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0));
                    if (err == DS_OK && drv)
                        ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
                    found = TRUE;
                    break;
                } else {
                    WARN("WineWaveOutMessage failed\n");
                    return E_PROP_ID_UNSUPPORTED;
                }
            }
	}

	if (found == FALSE) {
            ULONG wid;
            unsigned int widn;
            /* given specific device so try the capture devices next */
            widn = waveInGetNumDevs();
            for (wid = 0; wid < widn; wid++) {
                if (IsEqualGUID( &ppd->DeviceId, &capture_guids[wid] ) ) {
                    DSDRIVERDESC desc;
                    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
                    ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
                    ppd->WaveDeviceId = wid;
                    ppd->Devnode = wid;
                    err = mmErr(WineWaveInMessage((HWAVEIN)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
                    if (err == DS_OK) {
                        PIDSDRIVER drv = NULL;
                        strncpy(ppd->DescriptionA, desc.szDesc, sizeof(ppd->DescriptionA) - 1);
                        strncpy(ppd->ModuleA, desc.szDrvname, sizeof(ppd->ModuleA) - 1);
                        MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, ppd->DescriptionW, sizeof(ppd->DescriptionW)/sizeof(WCHAR) );
                        MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, ppd->ModuleW, sizeof(ppd->ModuleW)/sizeof(WCHAR) );
                        err = mmErr(WineWaveInMessage((HWAVEIN)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0));
                        if (err == DS_OK && drv)
                            ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
                        found = TRUE;
                        break;
                    } else {
                        WARN("WineWaveInMessage failed\n");
                        return E_PROP_ID_UNSUPPORTED;
                    }
                }
            }

            if (found == FALSE) {
                WARN("device not found\n");
                return E_PROP_ID_UNSUPPORTED;
            }
	}
    }

    if (pcbReturned) {
	*pcbReturned = cbPropData;
	TRACE("*pcbReturned=%ld\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_DescriptionA(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA) pPropData;
    HRESULT err;
    GUID dev_guid;
    TRACE("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    TRACE("DeviceId=%s\n",debugstr_guid(&ppd->DeviceId));
    if ( IsEqualGUID( &ppd->DeviceId , &GUID_NULL) ) {
	/* default device of type specified by ppd->DataFlow */
	if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	} else if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	} else {
	    TRACE("DataFlow=Unknown(%d)\n", ppd->DataFlow);
	}
	FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) GUID_NULL not implemented!\n",
	    debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
	return E_PROP_ID_UNSUPPORTED;
    }

    ppd->Type = DIRECTSOUNDDEVICE_TYPE_EMULATED;
    GetDeviceID(&ppd->DeviceId, &dev_guid);

    if ( IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultPlayback) ||
	 IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoicePlayback) ) {
	ULONG wod;
	unsigned int wodn;
	TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
	wodn = waveOutGetNumDevs();
	for (wod = 0; wod < wodn; wod++) {
		if (IsEqualGUID( &dev_guid, &renderer_guids[wod] ) ) {
		    DSDRIVERDESC desc;
		    ppd->WaveDeviceId = wod;
		    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
		    if (err == DS_OK) {
			PIDSDRIVER drv = NULL;
			/* FIXME: this is a memory leak */
			CHAR * szDescription = HeapAlloc(GetProcessHeap(),0,strlen(desc.szDesc) + 1);
			CHAR * szModule = HeapAlloc(GetProcessHeap(),0,strlen(desc.szDrvname) + 1);
			CHAR * szInterface = HeapAlloc(GetProcessHeap(),0,strlen("Interface") + 1);

			strcpy(szDescription, desc.szDesc);
			strcpy(szModule, desc.szDrvname);
			strcpy(szInterface, "Interface");

			ppd->Description = szDescription;
			ppd->Module = szModule;
			ppd->Interface = szInterface;
			err = mmErr(WineWaveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0));
			if (err == DS_OK && drv)
			    ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
			break;
		    } else {
			WARN("WineWaveOutMessage failed\n");
			return E_PROP_ID_UNSUPPORTED;
		    }
		}
	}
    } else if (IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultCapture) ||
	       IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoiceCapture) ) {
	ULONG wid;
	unsigned int widn;
	TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
	widn = waveInGetNumDevs();
	for (wid = 0; wid < widn; wid++) {
		if (IsEqualGUID( &dev_guid, &capture_guids[wid] ) ) {
		    DSDRIVERDESC desc;
		    ppd->WaveDeviceId = wid;
		    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
		    if (err == DS_OK) {
			PIDSCDRIVER drv;
			/* FIXME: this is a memory leak */
			CHAR * szDescription = HeapAlloc(GetProcessHeap(),0,strlen(desc.szDesc) + 1);
			CHAR * szModule = HeapAlloc(GetProcessHeap(),0,strlen(desc.szDrvname) + 1);
			CHAR * szInterface = HeapAlloc(GetProcessHeap(),0,strlen("Interface") + 1);

			strcpy(szDescription, desc.szDesc);
			strcpy(szModule, desc.szDrvname);
			strcpy(szInterface, "Interface");

			ppd->Description = szDescription;
			ppd->Module = szModule;
			ppd->Interface = szInterface;
			err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDIFACE,(DWORD)&drv,0));
			if (err == DS_OK && drv)
				ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
			break;
		    } else {
			WARN("WineWaveInMessage failed\n");
			return E_PROP_ID_UNSUPPORTED;
		    }
		    break;
		}
	}
    } else {
	BOOL found = FALSE;
	ULONG wod;
	unsigned int wodn;
	/* given specific device so try the render devices first */
	wodn = waveOutGetNumDevs();
	for (wod = 0; wod < wodn; wod++) {
		if (IsEqualGUID( &ppd->DeviceId, &renderer_guids[wod] ) ) {
		    DSDRIVERDESC desc;
		    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
		    ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
		    ppd->WaveDeviceId = wod;
		    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
		    if (err == DS_OK) {
			PIDSDRIVER drv = NULL;
			/* FIXME: this is a memory leak */
			CHAR * szDescription = HeapAlloc(GetProcessHeap(),0,strlen(desc.szDesc) + 1);
			CHAR * szModule = HeapAlloc(GetProcessHeap(),0,strlen(desc.szDrvname) + 1);
			CHAR * szInterface = HeapAlloc(GetProcessHeap(),0,strlen("Interface") + 1);

			strcpy(szDescription, desc.szDesc);
			strcpy(szModule, desc.szDrvname);
			strcpy(szInterface, "Interface");

			ppd->Description = szDescription;
			ppd->Module = szModule;
			ppd->Interface = szInterface;
			err = mmErr(WineWaveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0));
			if (err == DS_OK && drv)
				ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
			found = TRUE;
			break;
		    } else {
			WARN("WineWaveOutMessage failed\n");
			return E_PROP_ID_UNSUPPORTED;
		    }
		}
	}

	if (found == FALSE) {
	    WARN("device not found\n");
	    return E_PROP_ID_UNSUPPORTED;
	}
    }

    if (pcbReturned) {
	*pcbReturned = cbPropData;
	TRACE("*pcbReturned=%ld\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_DescriptionW(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA) pPropData;
    HRESULT err;
    GUID dev_guid;
    TRACE("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    TRACE("DeviceId=%s\n",debugstr_guid(&ppd->DeviceId));
    if ( IsEqualGUID( &ppd->DeviceId , &GUID_NULL) ) {
	/* default device of type specified by ppd->DataFlow */
	if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	} else if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	} else {
	    TRACE("DataFlow=Unknown(%d)\n", ppd->DataFlow);
	}
	FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) GUID_NULL not implemented!\n",
	    debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
	return E_PROP_ID_UNSUPPORTED;
    }

    ppd->Type = DIRECTSOUNDDEVICE_TYPE_EMULATED;
    GetDeviceID(&ppd->DeviceId, &dev_guid);

    if ( IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultPlayback) ||
	 IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoicePlayback) ) {
	ULONG wod;
	unsigned int wodn;
	TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
	wodn = waveOutGetNumDevs();
	for (wod = 0; wod < wodn; wod++) {
		if (IsEqualGUID( &dev_guid, &renderer_guids[wod] ) ) {
		    DSDRIVERDESC desc;
		    ppd->WaveDeviceId = wod;
		    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
		    if (err == DS_OK) {
			PIDSDRIVER drv = NULL;
			/* FIXME: this is a memory leak */
			WCHAR * wDescription = HeapAlloc(GetProcessHeap(),0,0x200);
			WCHAR * wModule = HeapAlloc(GetProcessHeap(),0,0x200);
			WCHAR * wInterface = HeapAlloc(GetProcessHeap(),0,0x200);

			MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, wDescription, 0x100  );
			MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, wModule, 0x100 );
			MultiByteToWideChar( CP_ACP, 0, "Interface", -1, wInterface, 0x100 );

			ppd->Description = wDescription;
			ppd->Module = wModule;
			ppd->Interface = wInterface;
			err = mmErr(WineWaveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0));
			if (err == DS_OK && drv)
				ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
			    break;
		    } else {
			WARN("WineWaveOutMessage failed\n");
			return E_PROP_ID_UNSUPPORTED;
		    }
		}
	}
    } else if (IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultCapture) ||
	       IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoiceCapture) ) {
	ULONG wid;
	unsigned int widn;
	TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
	widn = waveInGetNumDevs();
	for (wid = 0; wid < widn; wid++) {
		if (IsEqualGUID( &dev_guid, &capture_guids[wid] ) ) {
		    DSDRIVERDESC desc;
		    ppd->WaveDeviceId = wid;
		    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
		    if (err == DS_OK) {
			PIDSCDRIVER drv;
			/* FIXME: this is a memory leak */
			WCHAR * wDescription = HeapAlloc(GetProcessHeap(),0,0x200);
			WCHAR * wModule = HeapAlloc(GetProcessHeap(),0,0x200);
			WCHAR * wInterface = HeapAlloc(GetProcessHeap(),0,0x200);

			MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, wDescription, 0x100  );
			MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, wModule, 0x100 );
			MultiByteToWideChar( CP_ACP, 0, "Interface", -1, wInterface, 0x100 );

			ppd->Description = wDescription;
			ppd->Module = wModule;
			ppd->Interface = wInterface;
			err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDIFACE,(DWORD)&drv,0));
			if (err == DS_OK && drv)
				ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
			break;
		    } else {
			WARN("WineWaveInMessage failed\n");
			return E_PROP_ID_UNSUPPORTED;
		    }
		    break;
		}
	}
    } else {
	BOOL found = FALSE;
	ULONG wod;
	unsigned int wodn;
	/* given specific device so try the render devices first */
	wodn = waveOutGetNumDevs();
	for (wod = 0; wod < wodn; wod++) {
		if (IsEqualGUID( &ppd->DeviceId, &renderer_guids[wod] ) ) {
		    DSDRIVERDESC desc;
		    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
		    ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
		    ppd->WaveDeviceId = wod;
		    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
		    if (err == DS_OK) {
			PIDSDRIVER drv = NULL;
			/* FIXME: this is a memory leak */
			WCHAR * wDescription = HeapAlloc(GetProcessHeap(),0,0x200);
			WCHAR * wModule = HeapAlloc(GetProcessHeap(),0,0x200);
			WCHAR * wInterface = HeapAlloc(GetProcessHeap(),0,0x200);

			MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, wDescription, 0x100  );
			MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, wModule, 0x100 );
			MultiByteToWideChar( CP_ACP, 0, "Interface", -1, wInterface, 0x100 );

			ppd->Description = wDescription;
			ppd->Module = wModule;
			ppd->Interface = wInterface;
			err = mmErr(WineWaveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0));
			if (err == DS_OK && drv)
				ppd->Type = DIRECTSOUNDDEVICE_TYPE_VXD;
			found = TRUE;
			break;
		    } else {
			WARN("WineWaveOutMessage failed\n");
			return E_PROP_ID_UNSUPPORTED;
		    }
		}
	}

	if (found == FALSE) {
	    WARN("device not found\n");
	    return E_PROP_ID_UNSUPPORTED;
	}
    }

    if (pcbReturned) {
	*pcbReturned = cbPropData;
	TRACE("*pcbReturned=%ld\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_Enumerate1(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI DSPROPERTY_EnumerateA(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA) pPropData;
    HRESULT err;
    TRACE("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    if ( IsEqualGUID( &DSPROPSETID_DirectSoundDevice, guidPropSet) ) {
	if (ppd) {
	    if (ppd->Callback) {
		unsigned devs, wod, wid;
		DSDRIVERDESC desc;
		DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA data;

		devs = waveOutGetNumDevs();
		for (wod = 0; wod < devs; ++wod) {
		    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
		    if (err == DS_OK) {
			DWORD size;
			memset(&data, 0, sizeof(data));
			data.DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
			data.WaveDeviceId = wod;
			data.DeviceId = renderer_guids[wod];
			data.Description = desc.szDesc;
			data.Module = desc.szDrvname;
			err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDEVICEINTERFACESIZE,(DWORD_PTR)&size,0));
			if (err == DS_OK) {
			    WCHAR * nameW = HeapAlloc(GetProcessHeap(),0,size);
			    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDEVICEINTERFACE,(DWORD_PTR)nameW,size));
			    if (err == DS_OK) {
				CHAR * szInterface = HeapAlloc(GetProcessHeap(),0,size/sizeof(WCHAR));
				WideCharToMultiByte( CP_ACP, 0, nameW, size/sizeof(WCHAR), szInterface, size/sizeof(WCHAR), NULL, NULL );
				data.Interface = szInterface;

				TRACE("calling Callback(%p,%p)\n", &data, ppd->Context);
				(ppd->Callback)(&data, ppd->Context);

				HeapFree(GetProcessHeap(),0,szInterface);
			    }
			    HeapFree(GetProcessHeap(),0,nameW);
			}
		    }
		}

		devs = waveInGetNumDevs();
		for (wid = 0; wid < devs; ++wid) {
		    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
		    if (err == DS_OK) {
			DWORD size;
			memset(&data, 0, sizeof(data));
			data.DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
			data.WaveDeviceId = wid;
			data.DeviceId = capture_guids[wid];
			data.Description = desc.szDesc;
			data.Module = desc.szDrvname;
			err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDEVICEINTERFACESIZE,(DWORD_PTR)&size,0));
			if (err == DS_OK) {
			    WCHAR * nameW = HeapAlloc(GetProcessHeap(),0,size);
			    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDEVICEINTERFACE,(DWORD_PTR)nameW,size));
			    if (err == DS_OK) {
				CHAR * szInterface = HeapAlloc(GetProcessHeap(),0,size/sizeof(WCHAR));
				WideCharToMultiByte( CP_ACP, 0, nameW, size/sizeof(WCHAR), szInterface, size/sizeof(WCHAR), NULL, NULL );
				data.Interface = szInterface;

				TRACE("calling Callback(%p,%p)\n", &data, ppd->Context);
				(ppd->Callback)(&data, ppd->Context);

				HeapFree(GetProcessHeap(),0,szInterface);
			    }
			    HeapFree(GetProcessHeap(),0,nameW);
			}
		    }
		}

		return S_OK;
	    }
	}
    } else {
	FIXME("unsupported property: %s\n",debugstr_guid(guidPropSet));
    }

    if (pcbReturned) {
	*pcbReturned = 0;
	FIXME("*pcbReturned=%ld\n", *pcbReturned);
    }

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI DSPROPERTY_EnumerateW(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA) pPropData;
    HRESULT err;
    TRACE("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    if ( IsEqualGUID( &DSPROPSETID_DirectSoundDevice, guidPropSet) ) {
	if (ppd) {
	    if (ppd->Callback) {
		unsigned devs, wod, wid;
		DSDRIVERDESC desc;
		DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA data;

		devs = waveOutGetNumDevs();
		for (wod = 0; wod < devs; ++wod) {
		    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
		    if (err == DS_OK) {
			WCHAR * wDescription = HeapAlloc(GetProcessHeap(),0,0x200);
			WCHAR * wModule = HeapAlloc(GetProcessHeap(),0,0x200);
			DWORD size;
			err = mmErr(WineWaveOutMessage((HWAVEOUT)wod,DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&size, 0));
			if (err == DS_OK) {
			    WCHAR * wInterface = HeapAlloc(GetProcessHeap(),0,size);
			    err = mmErr(WineWaveOutMessage((HWAVEOUT)wod, DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)wInterface, size));
			    if (err == DS_OK) {
				memset(&data, 0, sizeof(data));
				data.DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
				data.WaveDeviceId = wod;
				data.DeviceId = renderer_guids[wod];

				MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, wDescription, 0x100 );
				MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, wModule, 0x100 );

				data.Description = wDescription;
				data.Module = wModule;
				data.Interface = wInterface;

				TRACE("calling Callback(%p,%p)\n", &data, ppd->Context);
				(ppd->Callback)(&data, ppd->Context);
			    }
			    HeapFree(GetProcessHeap(),0,wInterface);
			}
			HeapFree(GetProcessHeap(),0,wDescription);
			HeapFree(GetProcessHeap(),0,wModule);
		    }
		}

		devs = waveInGetNumDevs();
		for (wid = 0; wid < devs; ++wid) {
		    err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&desc,0));
		    if (err == DS_OK) {
			WCHAR * wDescription = HeapAlloc(GetProcessHeap(),0,0x200);
			WCHAR * wModule = HeapAlloc(GetProcessHeap(),0,0x200);
			DWORD size;
			err = mmErr(WineWaveInMessage((HWAVEIN)wid,DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&size, 0));
			if (err == DS_OK) {
			    WCHAR * wInterface = HeapAlloc(GetProcessHeap(),0,size);
			    err = mmErr(WineWaveInMessage((HWAVEIN)wod, DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)wInterface, size));
			    if (err == DS_OK) {
				memset(&data, 0, sizeof(data));
				data.DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
				data.WaveDeviceId = wid;
				data.DeviceId = capture_guids[wid];

				MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, wDescription, 0x100 );
				MultiByteToWideChar( CP_ACP, 0, desc.szDrvname, -1, wModule, 0x100 );

				data.Description = wDescription;
				data.Module = wModule;
				data.Interface = wInterface;
				TRACE("calling Callback(%p,%p)\n", &data, ppd->Context);
				(ppd->Callback)(&data, ppd->Context);
			    }
			    HeapFree(GetProcessHeap(),0,wInterface);
			}
			HeapFree(GetProcessHeap(),0,wDescription);
			HeapFree(GetProcessHeap(),0,wModule);
		    }
		}

		return S_OK;
	    }
	}
    } else {
	FIXME("unsupported property: %s\n",debugstr_guid(guidPropSet));
    }

    if (pcbReturned) {
	*pcbReturned = 0;
	FIXME("*pcbReturned=%ld\n", *pcbReturned);
    }

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPrivatePropertySetImpl_Get(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned
) {
    IKsPrivatePropertySetImpl *This = (IKsPrivatePropertySetImpl *)iface;
    TRACE("(iface=%p,guidPropSet=%s,dwPropID=%ld,pInstanceData=%p,cbInstanceData=%ld,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData,pcbReturned);

    if ( IsEqualGUID( &DSPROPSETID_DirectSoundDevice, guidPropSet) ) {
	switch (dwPropID) {
	case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A:
	    return DSPROPERTY_WaveDeviceMappingA(guidPropSet,pPropData,cbPropData,pcbReturned);
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1:
	    return DSPROPERTY_Description1(guidPropSet,pPropData,cbPropData,pcbReturned);
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1:
	    return DSPROPERTY_Enumerate1(guidPropSet,pPropData,cbPropData,pcbReturned);
	case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W:
	    return DSPROPERTY_WaveDeviceMappingW(guidPropSet,pPropData,cbPropData,pcbReturned);
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A:
	    return DSPROPERTY_DescriptionA(guidPropSet,pPropData,cbPropData,pcbReturned);
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W:
	    return DSPROPERTY_DescriptionW(guidPropSet,pPropData,cbPropData,pcbReturned);
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A:
	    return DSPROPERTY_EnumerateA(guidPropSet,pPropData,cbPropData,pcbReturned);
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W:
	    return DSPROPERTY_EnumerateW(guidPropSet,pPropData,cbPropData,pcbReturned);
	default:
	    FIXME("unsupported ID: %ld\n",dwPropID);
	    break;
	}
    } else {
	FIXME("unsupported property: %s\n",debugstr_guid(guidPropSet));
    }

    if (pcbReturned) {
	*pcbReturned = 0;
	FIXME("*pcbReturned=%ld\n", *pcbReturned);
    }

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPrivatePropertySetImpl_Set(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData )
{
    IKsPrivatePropertySetImpl *This = (IKsPrivatePropertySetImpl *)iface;

    FIXME("(%p,%s,%ld,%p,%ld,%p,%ld), stub!\n",This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData);
    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPrivatePropertySetImpl_QuerySupport(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    PULONG pTypeSupport )
{
    IKsPrivatePropertySetImpl *This = (IKsPrivatePropertySetImpl *)iface;
    TRACE("(%p,%s,%ld,%p)\n",This,debugstr_guid(guidPropSet),dwPropID,pTypeSupport);

    if ( IsEqualGUID( &DSPROPSETID_DirectSoundDevice, guidPropSet) ) {
	switch (dwPropID) {
	case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W:
	    *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	    return S_OK;
	default:
	    FIXME("unsupported ID: %ld\n",dwPropID);
	    break;
	}
    } else {
	FIXME("unsupported property: %s\n",debugstr_guid(guidPropSet));
    }

    return E_PROP_ID_UNSUPPORTED;
}

static IKsPropertySetVtbl ikspvt = {
    IKsPrivatePropertySetImpl_QueryInterface,
    IKsPrivatePropertySetImpl_AddRef,
    IKsPrivatePropertySetImpl_Release,
    IKsPrivatePropertySetImpl_Get,
    IKsPrivatePropertySetImpl_Set,
    IKsPrivatePropertySetImpl_QuerySupport
};

HRESULT WINAPI IKsPrivatePropertySetImpl_Create(
    IKsPrivatePropertySetImpl **piks)
{
    IKsPrivatePropertySetImpl *iks;

    iks = HeapAlloc(GetProcessHeap(),0,sizeof(*iks));
    iks->ref = 0;
    iks->lpVtbl = &ikspvt;

    *piks = iks;
    return S_OK;
}
