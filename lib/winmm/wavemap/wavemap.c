/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Wine Wave mapper driver
 *
 * Copyright 	1999,2001 Eric Pouech
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

/* TODOs
 *	+ better protection against evilish dwUser parameters
 *	+ use asynchronous ACM conversion
 *	+ don't use callback functions when none is required in open
 *	+ the buffer sizes may not be accurate, so there may be some
 *	  remaining bytes in src and dst buffers after ACM conversions...
 *		those should be taken care of...
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmddk.h"
#include "mmreg.h"
#include "msacm.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wavemap);

typedef	struct tagWAVEMAPDATA {
    struct tagWAVEMAPDATA*	self;
    union {
        struct {
            HWAVEOUT	hOuterWave;
            HWAVEOUT	hInnerWave;
        } out;
        struct {
            HWAVEIN	hOuterWave;
            HWAVEIN	hInnerWave;
        } in;
    } u;
    HACMSTREAM	hAcmStream;
    /* needed data to filter callbacks. Only needed when hAcmStream is not 0 */
    DWORD	dwCallback;
    DWORD	dwClientInstance;
    DWORD	dwFlags;
    /* ratio to compute position from a PCM playback to any format */
    DWORD       avgSpeedOuter;
    DWORD       avgSpeedInner;
    /* channel size of inner and outer */
    DWORD       nSamplesPerSecOuter;
    DWORD       nSamplesPerSecInner;
} WAVEMAPDATA;

static	BOOL	WAVEMAP_IsData(WAVEMAPDATA* wm)
{
    return (!IsBadReadPtr(wm, sizeof(WAVEMAPDATA)) && wm->self == wm);
}

/*======================================================================*
 *                  WAVE OUT part                                       *
 *======================================================================*/

static void	CALLBACK wodCallback(HWAVEOUT hWave, UINT uMsg, DWORD dwInstance,
				     DWORD dwParam1, DWORD dwParam2)
{
    WAVEMAPDATA*	wom = (WAVEMAPDATA*)dwInstance;

    TRACE("(%p %u %ld %lx %lx);\n", hWave, uMsg, dwInstance, dwParam1, dwParam2);

    if (!WAVEMAP_IsData(wom)) {
	ERR("Bad data\n");
	return;
    }

    if (hWave != wom->u.out.hInnerWave && uMsg != WOM_OPEN)
	ERR("Shouldn't happen (%p %p)\n", hWave, wom->u.out.hInnerWave);

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case WOM_DONE:
	if (wom->hAcmStream) {
	    LPWAVEHDR		lpWaveHdrDst = (LPWAVEHDR)dwParam1;
	    PACMSTREAMHEADER	ash = (PACMSTREAMHEADER)((LPSTR)lpWaveHdrDst - sizeof(ACMSTREAMHEADER));
	    LPWAVEHDR		lpWaveHdrSrc = (LPWAVEHDR)ash->dwUser;

	    lpWaveHdrSrc->dwFlags &= ~WHDR_INQUEUE;
	    lpWaveHdrSrc->dwFlags |= WHDR_DONE;
	    dwParam1 = (DWORD)lpWaveHdrSrc;
	}
	break;
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    DriverCallback(wom->dwCallback, HIWORD(wom->dwFlags), (HDRVR)wom->u.out.hOuterWave,
		   uMsg, wom->dwClientInstance, dwParam1, dwParam2);
}

/******************************************************************
 *		wodOpenHelper
 *
 *
 */
static	DWORD	wodOpenHelper(WAVEMAPDATA* wom, UINT idx,
			      LPWAVEOPENDESC lpDesc, LPWAVEFORMATEX lpwfx,
			      DWORD dwFlags)
{
    DWORD	ret;

    TRACE("(%p %04x %p %p %08lx)\n", wom, idx, lpDesc, lpwfx, dwFlags);

    /* destination is always PCM, so the formulas below apply */
    lpwfx->nBlockAlign = (lpwfx->nChannels * lpwfx->wBitsPerSample) / 8;
    lpwfx->nAvgBytesPerSec = lpwfx->nSamplesPerSec * lpwfx->nBlockAlign;
    if (dwFlags & WAVE_FORMAT_QUERY) {
	ret = acmStreamOpen(NULL, 0, lpDesc->lpFormat, lpwfx, NULL, 0L, 0L, ACM_STREAMOPENF_QUERY);
    } else {
	ret = acmStreamOpen(&wom->hAcmStream, 0, lpDesc->lpFormat, lpwfx, NULL, 0L, 0L, 0L);
    }
    if (ret == MMSYSERR_NOERROR) {
	ret = waveOutOpen(&wom->u.out.hInnerWave, idx, lpwfx, (DWORD)wodCallback,
			  (DWORD)wom, (dwFlags & ~CALLBACK_TYPEMASK) | CALLBACK_FUNCTION);
	if (ret != MMSYSERR_NOERROR && !(dwFlags & WAVE_FORMAT_QUERY)) {
	    acmStreamClose(wom->hAcmStream, 0);
	    wom->hAcmStream = 0;
	}
    }
    TRACE("ret = %08lx\n", ret);
    return ret;
}

static	DWORD	wodOpen(LPDWORD lpdwUser, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    UINT 		ndlo, ndhi;
    UINT		i;
    WAVEMAPDATA*	wom = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEMAPDATA));
    DWORD               res;

    TRACE("(%p %p %08lx)\n", lpdwUser, lpDesc, dwFlags);

    if (!wom) {
        WARN("no memory\n");
	return MMSYSERR_NOMEM;
    }

    ndhi = waveOutGetNumDevs();
    if (dwFlags & WAVE_MAPPED) {
	if (lpDesc->uMappedDeviceID >= ndhi) {
            WARN("invalid parameter: dwFlags WAVE_MAPPED\n");
            return MMSYSERR_INVALPARAM;
        }
	ndlo = lpDesc->uMappedDeviceID;
	ndhi = ndlo + 1;
	dwFlags &= ~WAVE_MAPPED;
    } else {
	ndlo = 0;
    }
    wom->self = wom;
    wom->dwCallback = lpDesc->dwCallback;
    wom->dwFlags = dwFlags;
    wom->dwClientInstance = lpDesc->dwInstance;
    wom->u.out.hOuterWave = (HWAVEOUT)lpDesc->hWave;
    wom->avgSpeedOuter = wom->avgSpeedInner = lpDesc->lpFormat->nAvgBytesPerSec;
    wom->nSamplesPerSecOuter = wom->nSamplesPerSecInner = lpDesc->lpFormat->nSamplesPerSec;

    for (i = ndlo; i < ndhi; i++) {
	/* if no ACM stuff is involved, no need to handle callbacks at this
	 * level, this will be done transparently
	 */
	if (waveOutOpen(&wom->u.out.hInnerWave, i, lpDesc->lpFormat, (DWORD)wodCallback,
			(DWORD)wom, (dwFlags & ~CALLBACK_TYPEMASK) | CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT) == MMSYSERR_NOERROR) {
	    wom->hAcmStream = 0;
	    goto found;
	}
    }

    if ((dwFlags & WAVE_FORMAT_DIRECT) == 0 && lpDesc->lpFormat->wFormatTag == WAVE_FORMAT_PCM) {
        WAVEFORMATEX	wfx;

        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.cbSize = 0; /* normally, this field is not used for PCM format, just in case */
        /* try some ACM stuff */

#define	TRY(sps,bps)    wfx.nSamplesPerSec = (sps); wfx.wBitsPerSample = (bps); \
                        switch (res=wodOpenHelper(wom, i, lpDesc, &wfx, dwFlags | WAVE_FORMAT_DIRECT)) { \
                            case MMSYSERR_NOERROR: wom->avgSpeedInner = wfx.nAvgBytesPerSec; wom->nSamplesPerSecInner = wfx.nSamplesPerSec; goto found; \
                            case WAVERR_BADFORMAT: break; \
                            default: goto error; \
                        }

        /* Our resampling algorithm is quite primitive so first try
         * to just change the bit depth and number of channels
         */
        for (i = ndlo; i < ndhi; i++) {
            wfx.nSamplesPerSec=lpDesc->lpFormat->nSamplesPerSec;
            wfx.nChannels = lpDesc->lpFormat->nChannels;
            TRY(wfx.nSamplesPerSec, 16);
            TRY(wfx.nSamplesPerSec, 8);
            wfx.nChannels ^= 3;
            TRY(wfx.nSamplesPerSec, 16);
            TRY(wfx.nSamplesPerSec, 8);
        }

        for (i = ndlo; i < ndhi; i++) {
            /* first try with same stereo/mono option as source */
            wfx.nChannels = lpDesc->lpFormat->nChannels;
            TRY(96000, 16);
            TRY(48000, 16);
            TRY(44100, 16);
            TRY(22050, 16);
            TRY(11025, 16);

            /* 2^3 => 1, 1^3 => 2, so if stereo, try mono (and the other way around) */
            wfx.nChannels ^= 3;
            TRY(96000, 16);
            TRY(48000, 16);
            TRY(44100, 16);
            TRY(22050, 16);
            TRY(11025, 16);

            /* first try with same stereo/mono option as source */
            wfx.nChannels = lpDesc->lpFormat->nChannels;
            TRY(96000, 8);
            TRY(48000, 8);
            TRY(44100, 8);
            TRY(22050, 8);
            TRY(11025, 8);

            /* 2^3 => 1, 1^3 => 2, so if stereo, try mono (and the other way around) */
            wfx.nChannels ^= 3;
            TRY(96000, 8);
            TRY(48000, 8);
            TRY(44100, 8);
            TRY(22050, 8);
            TRY(11025, 8);
        }
#undef TRY
    }

    HeapFree(GetProcessHeap(), 0, wom);
    WARN("ret = WAVERR_BADFORMAT\n");
    return WAVERR_BADFORMAT;

found:
    if (dwFlags & WAVE_FORMAT_QUERY) {
	*lpdwUser = 0L;
	HeapFree(GetProcessHeap(), 0, wom);
    } else {
	*lpdwUser = (DWORD)wom;
    }
    return MMSYSERR_NOERROR;
error:
    HeapFree(GetProcessHeap(), 0, wom);
    if (res==ACMERR_NOTPOSSIBLE) {
        WARN("ret = WAVERR_BADFORMAT\n");
        return WAVERR_BADFORMAT;
    }
    WARN("ret = 0x%08lx\n", res);
    return res;
}

static	DWORD	wodClose(WAVEMAPDATA* wom)
{
    DWORD ret;

    TRACE("(%p)\n", wom);

    ret = waveOutClose(wom->u.out.hInnerWave);
    if (ret == MMSYSERR_NOERROR) {
	if (wom->hAcmStream) {
	    ret = acmStreamClose(wom->hAcmStream, 0);
	}
	if (ret == MMSYSERR_NOERROR) {
	    HeapFree(GetProcessHeap(), 0, wom);
	}
    }
    return ret;
}

static	DWORD	wodWrite(WAVEMAPDATA* wom, LPWAVEHDR lpWaveHdrSrc, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    LPWAVEHDR		lpWaveHdrDst;

    TRACE("(%p %p %08lx)\n", wom, lpWaveHdrSrc, dwParam2);

    if (!wom->hAcmStream) {
	return waveOutWrite(wom->u.out.hInnerWave, lpWaveHdrSrc, dwParam2);
    }

    lpWaveHdrSrc->dwFlags |= WHDR_INQUEUE;
    ash = (PACMSTREAMHEADER)lpWaveHdrSrc->reserved;
    /* acmStreamConvert will actually check that the new size is less than initial size */
    ash->cbSrcLength = lpWaveHdrSrc->dwBufferLength;
    if (acmStreamConvert(wom->hAcmStream, ash, 0L) != MMSYSERR_NOERROR) {
        WARN("acmStreamConvert failed\n");
	return MMSYSERR_ERROR;
    }

    lpWaveHdrDst = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    if (ash->cbSrcLength > ash->cbSrcLengthUsed)
        FIXME("Not all src buffer has been written, expect bogus sound\n");
    else if (ash->cbSrcLength < ash->cbSrcLengthUsed)
        ERR("CoDec has read more data than it is allowed to\n");

    if (ash->cbDstLengthUsed == 0) {
        /* something went wrong in decoding */
        FIXME("Got 0 length\n");
        return MMSYSERR_ERROR;
    }
    lpWaveHdrDst->dwBufferLength = ash->cbDstLengthUsed;
    return waveOutWrite(wom->u.out.hInnerWave, lpWaveHdrDst, sizeof(*lpWaveHdrDst));
}

static	DWORD	wodPrepare(WAVEMAPDATA* wom, LPWAVEHDR lpWaveHdrSrc, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    DWORD		size;
    DWORD		dwRet;
    LPWAVEHDR		lpWaveHdrDst;

    TRACE("(%p %p %08lx)\n", wom, lpWaveHdrSrc, dwParam2);

    if (!wom->hAcmStream)
	return waveOutPrepareHeader(wom->u.out.hInnerWave, lpWaveHdrSrc, dwParam2);

    if (acmStreamSize(wom->hAcmStream, lpWaveHdrSrc->dwBufferLength, &size, ACM_STREAMSIZEF_SOURCE) != MMSYSERR_NOERROR) {
        WARN("acmStreamSize failed\n");
	return MMSYSERR_ERROR;
    }

    ash = HeapAlloc(GetProcessHeap(), 0, sizeof(ACMSTREAMHEADER) + sizeof(WAVEHDR) + size);
    if (ash == NULL) {
        WARN("no memory\n");
	return MMSYSERR_NOMEM;
    }

    ash->cbStruct = sizeof(*ash);
    ash->fdwStatus = 0L;
    ash->dwUser = (DWORD)lpWaveHdrSrc;
    ash->pbSrc = lpWaveHdrSrc->lpData;
    ash->cbSrcLength = lpWaveHdrSrc->dwBufferLength;
    /* ash->cbSrcLengthUsed */
    ash->dwSrcUser = lpWaveHdrSrc->dwUser; /* FIXME ? */
    ash->pbDst = (LPSTR)ash + sizeof(ACMSTREAMHEADER) + sizeof(WAVEHDR);
    ash->cbDstLength = size;
    /* ash->cbDstLengthUsed */
    ash->dwDstUser = 0; /* FIXME ? */
    dwRet = acmStreamPrepareHeader(wom->hAcmStream, ash, 0L);
    if (dwRet != MMSYSERR_NOERROR) {
        WARN("acmStreamPrepareHeader failed\n");
	goto errCleanUp;
    }

    lpWaveHdrDst = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    lpWaveHdrDst->lpData = ash->pbDst;
    lpWaveHdrDst->dwBufferLength = size; /* conversion is not done yet */
    lpWaveHdrDst->dwFlags = 0;
    lpWaveHdrDst->dwLoops = 0;
    dwRet = waveOutPrepareHeader(wom->u.out.hInnerWave, lpWaveHdrDst, sizeof(*lpWaveHdrDst));
    if (dwRet != MMSYSERR_NOERROR) {
        WARN("waveOutPrepareHeader failed\n");
	goto errCleanUp;
    }

    lpWaveHdrSrc->reserved = (DWORD)ash;
    lpWaveHdrSrc->dwFlags = WHDR_PREPARED;
    TRACE("=> (0)\n");
    return MMSYSERR_NOERROR;
errCleanUp:
    TRACE("=> (%ld)\n", dwRet);
    HeapFree(GetProcessHeap(), 0, ash);
    return dwRet;
}

static	DWORD	wodUnprepare(WAVEMAPDATA* wom, LPWAVEHDR lpWaveHdrSrc, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    LPWAVEHDR		lpWaveHdrDst;
    DWORD		dwRet1, dwRet2;

    TRACE("(%p %p %08lx)\n", wom, lpWaveHdrSrc, dwParam2);

    if (!wom->hAcmStream) {
	return waveOutUnprepareHeader(wom->u.out.hInnerWave, lpWaveHdrSrc, dwParam2);
    }
    ash = (PACMSTREAMHEADER)lpWaveHdrSrc->reserved;
    dwRet1 = acmStreamUnprepareHeader(wom->hAcmStream, ash, 0L);

    lpWaveHdrDst = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    dwRet2 = waveOutUnprepareHeader(wom->u.out.hInnerWave, lpWaveHdrDst, sizeof(*lpWaveHdrDst));

    HeapFree(GetProcessHeap(), 0, ash);

    lpWaveHdrSrc->dwFlags &= ~WHDR_PREPARED;
    return (dwRet1 == MMSYSERR_NOERROR) ? dwRet2 : dwRet1;
}

static	DWORD	wodGetPosition(WAVEMAPDATA* wom, LPMMTIME lpTime, DWORD dwParam2)
{
    DWORD       val;
    MMTIME      timepos;
    TRACE("(%p %p %08lx)\n", wom, lpTime, dwParam2);

    memcpy(&timepos, lpTime, sizeof(timepos));

    /* For TIME_MS, we're going to recalculate using TIME_BYTES */
    if (lpTime->wType == TIME_MS)
        timepos.wType = TIME_BYTES;

    val = waveOutGetPosition(wom->u.out.hInnerWave, &timepos, dwParam2);

    if (lpTime->wType == TIME_BYTES || lpTime->wType == TIME_MS)
    {
        DWORD dwInnerSamplesPerOuter = wom->nSamplesPerSecInner / wom->nSamplesPerSecOuter;
        if (dwInnerSamplesPerOuter > 0)
        {
            DWORD dwInnerBytesPerSample = wom->avgSpeedInner / wom->nSamplesPerSecInner;
            DWORD dwInnerBytesPerOuterSample = dwInnerBytesPerSample * dwInnerSamplesPerOuter;
            DWORD remainder = 0;

            /* If we are up sampling (going from lower sample rate to higher),
            **   we need to make a special accomodation for times when we've
            **   written a partial output sample.  This happens frequently
            **   to us because we use msacm to do our up sampling, and it
            **   will up sample on an unaligned basis.
            ** For example, if you convert a 2 byte wide 8,000 'outer'
            **   buffer to a 2 byte wide 48,000 inner device, you would
            **   expect 2 bytes of input to produce 12 bytes of output.
            **   Instead, msacm will produce 8 bytes of output.
            **   But reporting our position as 1 byte of output is
            **   nonsensical; the output buffer position needs to be
            **   aligned on outer sample size, and aggressively rounded up.
            */
            remainder = timepos.u.cb % dwInnerBytesPerOuterSample;
            if (remainder > 0)
            {
                timepos.u.cb -= remainder;
                timepos.u.cb += dwInnerBytesPerOuterSample;
            }
        }

        lpTime->u.cb = MulDiv(timepos.u.cb, wom->avgSpeedOuter, wom->avgSpeedInner);

        /* Once we have the TIME_BYTES right, we can easily convert to TIME_MS */
        if (lpTime->wType == TIME_MS)
            lpTime->u.cb = MulDiv(lpTime->u.cb, 1000, wom->avgSpeedOuter);
    }
    else if (lpTime->wType == TIME_SAMPLES)
        lpTime->u.cb = MulDiv(timepos.u.cb, wom->nSamplesPerSecOuter, wom->nSamplesPerSecInner);
    else
        /* other time types don't require conversion */
        lpTime->u = timepos.u;

    return val;
}

static	DWORD	wodGetDevCaps(UINT wDevID, WAVEMAPDATA* wom, LPWAVEOUTCAPSW lpWaveCaps, DWORD dwParam2)
{
    static const WCHAR name[] = {'W','i','n','e',' ','w','a','v','e',' ','o','u','t',' ','m','a','p','p','e','r',0};

    TRACE("(%04x %p %p %08lx)\n",wDevID, wom, lpWaveCaps, dwParam2);

    /* if opened low driver, forward message */
    if (WAVEMAP_IsData(wom))
	return waveOutGetDevCapsW((UINT)wom->u.out.hInnerWave, lpWaveCaps, dwParam2);
    /* else if no drivers, nothing to map so return bad device */
    if (waveOutGetNumDevs() == 0) {
        WARN("bad device id\n");
        return MMSYSERR_BADDEVICEID;
    }
    /* otherwise, return caps of mapper itself */
    if (wDevID == (UINT)-1 || wDevID == (UINT16)-1) {
	WAVEOUTCAPSW woc;
	woc.wMid = 0x00FF;
	woc.wPid = 0x0001;
	woc.vDriverVersion = 0x0100;
	lstrcpyW(woc.szPname, name);
	woc.dwFormats =
            WAVE_FORMAT_96M08 | WAVE_FORMAT_96S08 | WAVE_FORMAT_96M16 | WAVE_FORMAT_96S16 |
            WAVE_FORMAT_48M08 | WAVE_FORMAT_48S08 | WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16 |
	    WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 | WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16 |
	    WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 |
	    WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 | WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16;
	woc.wChannels = 2;
	woc.dwSupport = WAVECAPS_VOLUME | WAVECAPS_LRVOLUME;
        memcpy(lpWaveCaps, &woc, min(dwParam2, sizeof(woc)));

	return MMSYSERR_NOERROR;
    }
    ERR("This shouldn't happen\n");
    return MMSYSERR_ERROR;
}

static	DWORD	wodGetVolume(UINT wDevID, WAVEMAPDATA* wom, LPDWORD lpVol)
{
    TRACE("(%04x %p %p)\n",wDevID, wom, lpVol);

    if (WAVEMAP_IsData(wom))
	return waveOutGetVolume(wom->u.out.hInnerWave, lpVol);
    return MMSYSERR_NOERROR;
}

static	DWORD	wodSetVolume(UINT wDevID, WAVEMAPDATA* wom, DWORD vol)
{
    TRACE("(%04x %p %08lx)\n",wDevID, wom, vol);

    if (WAVEMAP_IsData(wom))
	return waveOutSetVolume(wom->u.out.hInnerWave, vol);
    return MMSYSERR_NOERROR;
}

static	DWORD	wodPause(WAVEMAPDATA* wom)
{
    TRACE("(%p)\n",wom);

    return waveOutPause(wom->u.out.hInnerWave);
}

static	DWORD	wodRestart(WAVEMAPDATA* wom)
{
    TRACE("(%p)\n",wom);

    return waveOutRestart(wom->u.out.hInnerWave);
}

static	DWORD	wodReset(WAVEMAPDATA* wom)
{
    TRACE("(%p)\n",wom);

    return waveOutReset(wom->u.out.hInnerWave);
}

static	DWORD	wodBreakLoop(WAVEMAPDATA* wom)
{
    TRACE("(%p)\n",wom);

    return waveOutBreakLoop(wom->u.out.hInnerWave);
}

static  DWORD	wodMapperStatus(WAVEMAPDATA* wom, DWORD flags, LPVOID ptr)
{
    UINT	id;
    DWORD	ret = MMSYSERR_NOTSUPPORTED;

    TRACE("(%p %08lx %p)\n",wom, flags, ptr);

    switch (flags) {
    case WAVEOUT_MAPPER_STATUS_DEVICE:
	ret = waveOutGetID(wom->u.out.hInnerWave, &id);
	*(LPDWORD)ptr = id;
	break;
    case WAVEOUT_MAPPER_STATUS_MAPPED:
	FIXME("Unsupported flag=%ld\n", flags);
	*(LPDWORD)ptr = 0; /* FIXME ?? */
	break;
    case WAVEOUT_MAPPER_STATUS_FORMAT:
	FIXME("Unsupported flag=%ld\n", flags);
	/* ptr points to a WAVEFORMATEX struct - before or after streaming ? */
	*(LPDWORD)ptr = 0;
	break;
    default:
	FIXME("Unsupported flag=%ld\n", flags);
	*(LPDWORD)ptr = 0;
	break;
    }
    return ret;
}

/**************************************************************************
 * 				wodMessage (MSACM.@)
 */
DWORD WINAPI WAVEMAP_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser,
				DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case WODM_OPEN:	 	return wodOpen		((LPDWORD)dwUser,      (LPWAVEOPENDESC)dwParam1,dwParam2);
    case WODM_CLOSE:	 	return wodClose		((WAVEMAPDATA*)dwUser);
    case WODM_WRITE:	 	return wodWrite		((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1,	dwParam2);
    case WODM_PAUSE:	 	return wodPause		((WAVEMAPDATA*)dwUser);
    case WODM_GETPOS:	 	return wodGetPosition	((WAVEMAPDATA*)dwUser, (LPMMTIME)dwParam1, 	dwParam2);
    case WODM_BREAKLOOP: 	return wodBreakLoop	((WAVEMAPDATA*)dwUser);
    case WODM_PREPARE:	 	return wodPrepare	((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WODM_UNPREPARE: 	return wodUnprepare	((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (WAVEMAPDATA*)dwUser, (LPWAVEOUTCAPSW)dwParam1,dwParam2);
    case WODM_GETNUMDEVS:	return 1;
    case WODM_GETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETVOLUME:	return wodGetVolume	(wDevID, (WAVEMAPDATA*)dwUser, (LPDWORD)dwParam1);
    case WODM_SETVOLUME:	return wodSetVolume	(wDevID, (WAVEMAPDATA*)dwUser, dwParam1);
    case WODM_RESTART:		return wodRestart	((WAVEMAPDATA*)dwUser);
    case WODM_RESET:		return wodReset		((WAVEMAPDATA*)dwUser);
    case WODM_MAPPER_STATUS:	return wodMapperStatus  ((WAVEMAPDATA*)dwUser, dwParam1, (LPVOID)dwParam2);
    /* known but not supported */
    case DRV_QUERYDEVICEINTERFACESIZE:
    case DRV_QUERYDEVICEINTERFACE:
        return MMSYSERR_NOTSUPPORTED;
    default:
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  WAVE IN part                                        *
 *======================================================================*/

static void	CALLBACK widCallback(HWAVEIN hWave, UINT uMsg, DWORD dwInstance,
				     DWORD dwParam1, DWORD dwParam2)
{
    WAVEMAPDATA*	wim = (WAVEMAPDATA*)dwInstance;

    TRACE("(%p %u %ld %lx %lx);\n", hWave, uMsg, dwInstance, dwParam1, dwParam2);

    if (!WAVEMAP_IsData(wim)) {
	ERR("Bad data\n");
	return;
    }

    if (hWave != wim->u.in.hInnerWave && uMsg != WIM_OPEN)
	ERR("Shouldn't happen (%p %p)\n", hWave, wim->u.in.hInnerWave);

    switch (uMsg) {
    case WIM_OPEN:
    case WIM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case WIM_DATA:
	if (wim->hAcmStream) {
	    LPWAVEHDR		lpWaveHdrSrc = (LPWAVEHDR)dwParam1;
	    PACMSTREAMHEADER	ash = (PACMSTREAMHEADER)((LPSTR)lpWaveHdrSrc - sizeof(ACMSTREAMHEADER));
	    LPWAVEHDR		lpWaveHdrDst = (LPWAVEHDR)ash->dwUser;

	    /* convert data just gotten from waveIn into requested format */
	    if (acmStreamConvert(wim->hAcmStream, ash, 0L) != MMSYSERR_NOERROR) {
		ERR("ACM conversion failed\n");
		return;
	    } else {
		TRACE("Converted %ld bytes into %ld\n", ash->cbSrcLengthUsed, ash->cbDstLengthUsed);
	    }
	    /* and setup the wavehdr to return accordingly */
	    lpWaveHdrDst->dwFlags &= ~WHDR_INQUEUE;
	    lpWaveHdrDst->dwFlags |= WHDR_DONE;
	    lpWaveHdrDst->dwBytesRecorded = ash->cbDstLengthUsed;
	    dwParam1 = (DWORD)lpWaveHdrDst;
	}
	break;
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    DriverCallback(wim->dwCallback, HIWORD(wim->dwFlags), (HDRVR)wim->u.in.hOuterWave,
		   uMsg, wim->dwClientInstance, dwParam1, dwParam2);
}

static	DWORD	widOpenHelper(WAVEMAPDATA* wim, UINT idx,
			      LPWAVEOPENDESC lpDesc, LPWAVEFORMATEX lpwfx,
			      DWORD dwFlags)
{
    DWORD	ret;

    TRACE("(%p %04x %p %p %08lx)\n", wim, idx, lpDesc, lpwfx, dwFlags);

    /* source is always PCM, so the formulas below apply */
    lpwfx->nBlockAlign = (lpwfx->nChannels * lpwfx->wBitsPerSample) / 8;
    lpwfx->nAvgBytesPerSec = lpwfx->nSamplesPerSec * lpwfx->nBlockAlign;
    if (dwFlags & WAVE_FORMAT_QUERY) {
	ret = acmStreamOpen(NULL, 0, lpwfx, lpDesc->lpFormat, NULL, 0L, 0L, ACM_STREAMOPENF_QUERY);
    } else {
	ret = acmStreamOpen(&wim->hAcmStream, 0, lpwfx, lpDesc->lpFormat, NULL, 0L, 0L, 0L);
    }
    if (ret == MMSYSERR_NOERROR) {
	ret = waveInOpen(&wim->u.in.hInnerWave, idx, lpwfx, (DWORD)widCallback,
			 (DWORD)wim, (dwFlags & ~CALLBACK_TYPEMASK) | CALLBACK_FUNCTION);
	if (ret != MMSYSERR_NOERROR && !(dwFlags & WAVE_FORMAT_QUERY)) {
	    acmStreamClose(wim->hAcmStream, 0);
	    wim->hAcmStream = 0;
	}
    }
    TRACE("ret = %08lx\n", ret);
    return ret;
}

static	DWORD	widOpen(LPDWORD lpdwUser, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    UINT 		ndlo, ndhi;
    UINT		i;
    WAVEMAPDATA*	wim = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEMAPDATA));
    DWORD               res;

    TRACE("(%p %p %08lx)\n", lpdwUser, lpDesc, dwFlags);

    if (!wim) {
        WARN("no memory\n");
	return MMSYSERR_NOMEM;
    }

    wim->self = wim;
    wim->dwCallback = lpDesc->dwCallback;
    wim->dwFlags = dwFlags;
    wim->dwClientInstance = lpDesc->dwInstance;
    wim->u.in.hOuterWave = (HWAVEIN)lpDesc->hWave;

    ndhi = waveInGetNumDevs();
    if (dwFlags & WAVE_MAPPED) {
	if (lpDesc->uMappedDeviceID >= ndhi) return MMSYSERR_INVALPARAM;
	ndlo = lpDesc->uMappedDeviceID;
	ndhi = ndlo + 1;
	dwFlags &= ~WAVE_MAPPED;
    } else {
	ndlo = 0;
    }

    wim->avgSpeedOuter = wim->avgSpeedInner = lpDesc->lpFormat->nAvgBytesPerSec;
    wim->nSamplesPerSecOuter = wim->nSamplesPerSecInner = lpDesc->lpFormat->nSamplesPerSec;

    for (i = ndlo; i < ndhi; i++) {
	if (waveInOpen(&wim->u.in.hInnerWave, i, lpDesc->lpFormat, (DWORD)widCallback,
                       (DWORD)wim, (dwFlags & ~CALLBACK_TYPEMASK) | CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT) == MMSYSERR_NOERROR) {
	    wim->hAcmStream = 0;
	    goto found;
	}
    }

    if ((dwFlags & WAVE_FORMAT_DIRECT) == 0)
    {
        WAVEFORMATEX	wfx;

        wfx.wFormatTag = WAVE_FORMAT_PCM;
        wfx.cbSize = 0; /* normally, this field is not used for PCM format, just in case */
        /* try some ACM stuff */

#define	TRY(sps,bps)    wfx.nSamplesPerSec = (sps); wfx.wBitsPerSample = (bps); \
                        switch (res=widOpenHelper(wim, i, lpDesc, &wfx, dwFlags | WAVE_FORMAT_DIRECT)) { \
                        case MMSYSERR_NOERROR: wim->avgSpeedInner = wfx.nAvgBytesPerSec; wim->nSamplesPerSecInner = wfx.nSamplesPerSec; goto found; \
                        case WAVERR_BADFORMAT: break; \
                        default: goto error; \
                        }

        for (i = ndlo; i < ndhi; i++) {
	    wfx.nSamplesPerSec=lpDesc->lpFormat->nSamplesPerSec;
            /* first try with same stereo/mono option as source */
            wfx.nChannels = lpDesc->lpFormat->nChannels;
			TRY(wfx.nSamplesPerSec, 16);
			TRY(wfx.nSamplesPerSec, 8);
			wfx.nChannels ^= 3;
			TRY(wfx.nSamplesPerSec, 16);
			TRY(wfx.nSamplesPerSec, 8);
	}

        for (i = ndlo; i < ndhi; i++) {
	    wfx.nSamplesPerSec=lpDesc->lpFormat->nSamplesPerSec;
            /* first try with same stereo/mono option as source */
            wfx.nChannels = lpDesc->lpFormat->nChannels;
            TRY(96000, 16);
            TRY(48000, 16);
            TRY(44100, 16);
            TRY(22050, 16);
            TRY(11025, 16);

            /* 2^3 => 1, 1^3 => 2, so if stereo, try mono (and the other way around) */
            wfx.nChannels ^= 3;
            TRY(96000, 16);
            TRY(48000, 16);
            TRY(44100, 16);
            TRY(22050, 16);
            TRY(11025, 16);

            /* first try with same stereo/mono option as source */
            wfx.nChannels = lpDesc->lpFormat->nChannels;
            TRY(96000, 8);
            TRY(48000, 8);
            TRY(44100, 8);
            TRY(22050, 8);
            TRY(11025, 8);

            /* 2^3 => 1, 1^3 => 2, so if stereo, try mono (and the other way around) */
            wfx.nChannels ^= 3;
            TRY(96000, 8);
            TRY(48000, 8);
            TRY(44100, 8);
            TRY(22050, 8);
            TRY(11025, 8);
        }
#undef TRY
    }

    HeapFree(GetProcessHeap(), 0, wim);
    WARN("ret = WAVERR_BADFORMAT\n");
    return WAVERR_BADFORMAT;
found:
    if (dwFlags & WAVE_FORMAT_QUERY) {
	*lpdwUser = 0L;
	HeapFree(GetProcessHeap(), 0, wim);
    } else {
	*lpdwUser = (DWORD)wim;
    }
    TRACE("Ok (stream=%08lx)\n", (DWORD)wim->hAcmStream);
    return MMSYSERR_NOERROR;
error:
    HeapFree(GetProcessHeap(), 0, wim);
    if (res==ACMERR_NOTPOSSIBLE) {
        WARN("ret = WAVERR_BADFORMAT\n");
        return WAVERR_BADFORMAT;
    }
    WARN("ret = 0x%08lx\n", res);
    return res;
}

static	DWORD	widClose(WAVEMAPDATA* wim)
{
    DWORD ret;

    TRACE("(%p)\n", wim);

    ret = waveInClose(wim->u.in.hInnerWave);
    if (ret == MMSYSERR_NOERROR) {
	if (wim->hAcmStream) {
	    ret = acmStreamClose(wim->hAcmStream, 0);
	}
	if (ret == MMSYSERR_NOERROR) {
	    HeapFree(GetProcessHeap(), 0, wim);
	}
    }
    return ret;
}

static	DWORD	widAddBuffer(WAVEMAPDATA* wim, LPWAVEHDR lpWaveHdrDst, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    LPWAVEHDR		lpWaveHdrSrc;

    TRACE("(%p %p %08lx)\n", wim, lpWaveHdrDst, dwParam2);

    if (!wim->hAcmStream) {
	return waveInAddBuffer(wim->u.in.hInnerWave, lpWaveHdrDst, dwParam2);
    }

    lpWaveHdrDst->dwFlags |= WHDR_INQUEUE;
    ash = (PACMSTREAMHEADER)lpWaveHdrDst->reserved;

    lpWaveHdrSrc = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    return waveInAddBuffer(wim->u.in.hInnerWave, lpWaveHdrSrc, sizeof(*lpWaveHdrSrc));
}

static	DWORD	widPrepare(WAVEMAPDATA* wim, LPWAVEHDR lpWaveHdrDst, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    DWORD		size;
    DWORD		dwRet;
    LPWAVEHDR		lpWaveHdrSrc;

    TRACE("(%p %p %08lx)\n", wim, lpWaveHdrDst, dwParam2);

    if (!wim->hAcmStream) {
	return waveInPrepareHeader(wim->u.in.hInnerWave, lpWaveHdrDst, dwParam2);
    }
    if (acmStreamSize(wim->hAcmStream, lpWaveHdrDst->dwBufferLength, &size,
		      ACM_STREAMSIZEF_DESTINATION) != MMSYSERR_NOERROR) {
        WARN("acmStreamSize failed\n");
	return MMSYSERR_ERROR;
    }

    ash = HeapAlloc(GetProcessHeap(), 0, sizeof(ACMSTREAMHEADER) + sizeof(WAVEHDR) + size);
    if (ash == NULL) {
        WARN("no memory\n");
	return MMSYSERR_NOMEM;
    }

    ash->cbStruct = sizeof(*ash);
    ash->fdwStatus = 0L;
    ash->dwUser = (DWORD)lpWaveHdrDst;
    ash->pbSrc = (LPSTR)ash + sizeof(ACMSTREAMHEADER) + sizeof(WAVEHDR);
    ash->cbSrcLength = size;
    /* ash->cbSrcLengthUsed */
    ash->dwSrcUser = 0L; /* FIXME ? */
    ash->pbDst = lpWaveHdrDst->lpData;
    ash->cbDstLength = lpWaveHdrDst->dwBufferLength;
    /* ash->cbDstLengthUsed */
    ash->dwDstUser = lpWaveHdrDst->dwUser; /* FIXME ? */
    dwRet = acmStreamPrepareHeader(wim->hAcmStream, ash, 0L);
    if (dwRet != MMSYSERR_NOERROR) {
        WARN("acmStreamPrepareHeader failed\n");
	goto errCleanUp;
    }

    lpWaveHdrSrc = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    lpWaveHdrSrc->lpData = ash->pbSrc;
    lpWaveHdrSrc->dwBufferLength = size; /* conversion is not done yet */
    lpWaveHdrSrc->dwFlags = 0;
    lpWaveHdrSrc->dwLoops = 0;
    dwRet = waveInPrepareHeader(wim->u.in.hInnerWave, lpWaveHdrSrc, sizeof(*lpWaveHdrSrc));
    if (dwRet != MMSYSERR_NOERROR) {
        WARN("waveInPrepareHeader failed\n");
	goto errCleanUp;
    }

    lpWaveHdrDst->reserved = (DWORD)ash;
    lpWaveHdrDst->dwFlags = WHDR_PREPARED;
    TRACE("=> (0)\n");
    return MMSYSERR_NOERROR;
errCleanUp:
    TRACE("=> (%ld)\n", dwRet);
    HeapFree(GetProcessHeap(), 0, ash);
    return dwRet;
}

static	DWORD	widUnprepare(WAVEMAPDATA* wim, LPWAVEHDR lpWaveHdrDst, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    LPWAVEHDR		lpWaveHdrSrc;
    DWORD		dwRet1, dwRet2;

    TRACE("(%p %p %08lx)\n", wim, lpWaveHdrDst, dwParam2);

    if (!wim->hAcmStream) {
	return waveInUnprepareHeader(wim->u.in.hInnerWave, lpWaveHdrDst, dwParam2);
    }
    ash = (PACMSTREAMHEADER)lpWaveHdrDst->reserved;
    dwRet1 = acmStreamUnprepareHeader(wim->hAcmStream, ash, 0L);

    lpWaveHdrSrc = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    dwRet2 = waveInUnprepareHeader(wim->u.in.hInnerWave, lpWaveHdrSrc, sizeof(*lpWaveHdrSrc));

    HeapFree(GetProcessHeap(), 0, ash);

    lpWaveHdrDst->dwFlags &= ~WHDR_PREPARED;
    return (dwRet1 == MMSYSERR_NOERROR) ? dwRet2 : dwRet1;
}

static	DWORD	widGetPosition(WAVEMAPDATA* wim, LPMMTIME lpTime, DWORD dwParam2)
{
    DWORD       val;

    TRACE("(%p %p %08lx)\n", wim, lpTime, dwParam2);

    val = waveInGetPosition(wim->u.in.hInnerWave, lpTime, dwParam2);
    if (lpTime->wType == TIME_BYTES)
        lpTime->u.cb = MulDiv(lpTime->u.cb, wim->avgSpeedOuter, wim->avgSpeedInner);
    if (lpTime->wType == TIME_SAMPLES)
        lpTime->u.cb = MulDiv(lpTime->u.cb, wim->nSamplesPerSecOuter, wim->nSamplesPerSecInner);
    /* other time types don't require conversion */
    return val;
}

static	DWORD	widGetDevCaps(UINT wDevID, WAVEMAPDATA* wim, LPWAVEINCAPSW lpWaveCaps, DWORD dwParam2)
{
    TRACE("(%04x, %p %p %08lx)\n", wDevID, wim, lpWaveCaps, dwParam2);

    /* if opened low driver, forward message */
    if (WAVEMAP_IsData(wim))
	return waveInGetDevCapsW((UINT)wim->u.in.hInnerWave, lpWaveCaps, dwParam2);
    /* else if no drivers, nothing to map so return bad device */
    if (waveInGetNumDevs() == 0) {
        WARN("bad device id\n");
        return MMSYSERR_BADDEVICEID;
    }
    /* otherwise, return caps of mapper itself */
    if (wDevID == (UINT)-1 || wDevID == (UINT16)-1) {
        WAVEINCAPSW wic;
        static const WCHAR init[] = {'W','i','n','e',' ','w','a','v','e',' ','i','n',' ','m','a','p','p','e','r',0};
	wic.wMid = 0x00FF;
	wic.wPid = 0x0001;
	wic.vDriverVersion = 0x0001;
	strcpyW(wic.szPname, init);
	wic.dwFormats =
            WAVE_FORMAT_96M08 | WAVE_FORMAT_96S08 | WAVE_FORMAT_96M16 | WAVE_FORMAT_96S16 |
            WAVE_FORMAT_48M08 | WAVE_FORMAT_48S08 | WAVE_FORMAT_48M16 | WAVE_FORMAT_48S16 |
	    WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 | WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16 |
	    WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 |
	    WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 | WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16;
	wic.wChannels = 2;
        memcpy(lpWaveCaps, &wic, min(dwParam2, sizeof(wic)));

	return MMSYSERR_NOERROR;
    }
    ERR("This shouldn't happen\n");
    return MMSYSERR_ERROR;
}

static	DWORD	widStop(WAVEMAPDATA* wim)
{
    TRACE("(%p)\n", wim);

    return waveInStop(wim->u.in.hInnerWave);
}

static	DWORD	widStart(WAVEMAPDATA* wim)
{
    TRACE("(%p)\n", wim);

    return waveInStart(wim->u.in.hInnerWave);
}

static	DWORD	widReset(WAVEMAPDATA* wim)
{
    TRACE("(%p)\n", wim);

    return waveInReset(wim->u.in.hInnerWave);
}

static  DWORD	widMapperStatus(WAVEMAPDATA* wim, DWORD flags, LPVOID ptr)
{
    UINT	id;
    DWORD	ret = MMSYSERR_NOTSUPPORTED;

    TRACE("(%p %08lx %p)\n", wim, flags, ptr);

    switch (flags) {
    case WAVEIN_MAPPER_STATUS_DEVICE:
	ret = waveInGetID(wim->u.in.hInnerWave, &id);
	*(LPDWORD)ptr = id;
	break;
    case WAVEIN_MAPPER_STATUS_MAPPED:
	FIXME("Unsupported yet flag=%ld\n", flags);
	*(LPDWORD)ptr = 0; /* FIXME ?? */
	break;
    case WAVEIN_MAPPER_STATUS_FORMAT:
	FIXME("Unsupported flag=%ld\n", flags);
	/* ptr points to a WAVEFORMATEX struct  - before or after streaming ? */
	*(LPDWORD)ptr = 0; /* FIXME ?? */
	break;
    default:
	FIXME("Unsupported flag=%ld\n", flags);
	*(LPDWORD)ptr = 0;
	break;
    }
    return ret;
}

/**************************************************************************
 * 				widMessage (MSACM.@)
 */
DWORD WINAPI WAVEMAP_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
				DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;

    case WIDM_OPEN:		return widOpen          ((LPDWORD)dwUser,     (LPWAVEOPENDESC)dwParam1, dwParam2);
    case WIDM_CLOSE:		return widClose         ((WAVEMAPDATA*)dwUser);

    case WIDM_ADDBUFFER:	return widAddBuffer     ((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WIDM_PREPARE:		return widPrepare       ((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WIDM_UNPREPARE:	return widUnprepare     ((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WIDM_GETDEVCAPS:	return widGetDevCaps    (wDevID, (WAVEMAPDATA*)dwUser, (LPWAVEINCAPSW)dwParam1, dwParam2);
    case WIDM_GETNUMDEVS:	return 1;
    case WIDM_GETPOS:		return widGetPosition   ((WAVEMAPDATA*)dwUser, (LPMMTIME)dwParam1, 	dwParam2);
    case WIDM_RESET:		return widReset         ((WAVEMAPDATA*)dwUser);
    case WIDM_START:		return widStart         ((WAVEMAPDATA*)dwUser);
    case WIDM_STOP:		return widStop          ((WAVEMAPDATA*)dwUser);
    case WIDM_MAPPER_STATUS:	return widMapperStatus  ((WAVEMAPDATA*)dwUser, dwParam1, (LPVOID)dwParam2);
    /* known but not supported */
    case DRV_QUERYDEVICEINTERFACESIZE:
    case DRV_QUERYDEVICEINTERFACE:
        return MMSYSERR_NOTSUPPORTED;
    default:
	FIXME("unknown message %u!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Driver part                                         *
 *======================================================================*/

static	struct WINE_WAVEMAP* oss = NULL;

/**************************************************************************
 * 				WAVEMAP_drvOpen			[internal]
 */
static	DWORD	WAVEMAP_drvOpen(LPSTR str)
{
    TRACE("(%p)\n", str);

    if (oss)
	return 0;

    /* I know, this is ugly, but who cares... */
    oss = (struct WINE_WAVEMAP*)1;
    return 1;
}

/**************************************************************************
 * 				WAVEMAP_drvClose		[internal]
 */
static	DWORD	WAVEMAP_drvClose(DWORD dwDevID)
{
    TRACE("(%08lx)\n", dwDevID);

    if (oss) {
	oss = NULL;
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				DriverProc (MSACM.@)
 */
LONG CALLBACK	WAVEMAP_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg,
				   DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%08lX, %p, %08lX, %08lX, %08lX)\n",
	  dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return WAVEMAP_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return WAVEMAP_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "WAVEMAP MultiMedia Driver !", "Wave mapper Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
