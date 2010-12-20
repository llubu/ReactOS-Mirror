/*
 * Copyright 2005-2007 Jacek Caban for CodeWeavers
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

#include "urlmon_main.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

static WCHAR cbinding_contextW[] = {'C','B','i','n','d','i','n','g',' ','C','o','n','t','e','x','t',0};
static WCHAR bscb_holderW[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

typedef struct Binding Binding;

struct _task_header_t;

typedef void (*task_proc_t)(Binding*, struct _task_header_t*);

typedef struct _task_header_t {
    task_proc_t proc;
    struct _task_header_t *next;
} task_header_t;

typedef struct {
    const IUnknownVtbl *lpUnknownVtbl;

    LONG ref;

    IInternetProtocolEx *protocol;

    BYTE buf[1024*8];
    DWORD size;
    BOOL init;
    HANDLE file;
    HRESULT hres;

    LPWSTR cache_file;
} stgmed_buf_t;

typedef struct _stgmed_obj_t stgmed_obj_t;

typedef struct {
    void (*release)(stgmed_obj_t*);
    HRESULT (*fill_stgmed)(stgmed_obj_t*,STGMEDIUM*);
    HRESULT (*get_result)(stgmed_obj_t*,DWORD,void**);
} stgmed_obj_vtbl;

struct _stgmed_obj_t {
    const stgmed_obj_vtbl *vtbl;
};

#define STGMEDUNK(x)  ((IUnknown*) &(x)->lpUnknownVtbl)

typedef enum {
    BEFORE_DOWNLOAD,
    DOWNLOADING,
    END_DOWNLOAD
} download_state_t;

#define BINDING_LOCKED    0x0001
#define BINDING_STOPPED   0x0002
#define BINDING_OBJAVAIL  0x0004
#define BINDING_ABORTED   0x0008

struct Binding {
    const IBindingVtbl               *lpBindingVtbl;
    const IInternetProtocolSinkVtbl  *lpIInternetProtocolSinkVtbl;
    const IInternetBindInfoVtbl      *lpInternetBindInfoVtbl;
    const IWinInetHttpInfoVtbl       *lpWinInetHttpInfoVtbl;
    const IServiceProviderVtbl       *lpServiceProviderVtbl;

    LONG ref;

    IBindStatusCallback *callback;
    IInternetProtocolEx *protocol;
    IServiceProvider *service_provider;

    stgmed_buf_t *stgmed_buf;
    stgmed_obj_t *stgmed_obj;

    BINDINFO bindinfo;
    DWORD bindf;
    BOOL to_object;
    LPWSTR mime;
    UINT clipboard_format;
    LPWSTR url;
    LPWSTR redirect_url;
    IID iid;
    BOOL report_mime;
    BOOL use_cache_file;
    DWORD state;
    HRESULT hres;
    download_state_t download_state;
    IUnknown *obj;
    IMoniker *mon;
    IBindCtx *bctx;
    HWND notif_hwnd;

    CRITICAL_SECTION section;
};

#define BINDING(x)   ((IBinding*)               &(x)->lpBindingVtbl)
#define BINDINF(x)   ((IInternetBindInfo*)      &(x)->lpInternetBindInfoVtbl)
#define INETINFO(x)  ((IWinInetHttpInfo*)       &(x)->lpWinInetHttpInfoVtbl)
#define SERVPROV(x)  ((IServiceProvider*)       &(x)->lpServiceProviderVtbl)

#define STREAM(x) ((IStream*) &(x)->lpStreamVtbl)
#define HTTPNEG2(x) ((IHttpNegotiate2*) &(x)->lpHttpNegotiate2Vtbl)

static void fill_stgmed_buffer(stgmed_buf_t *buf)
{
    DWORD read = 0;

    if(sizeof(buf->buf) == buf->size)
        return;

    buf->hres = IInternetProtocol_Read(buf->protocol, buf->buf+buf->size,
            sizeof(buf->buf)-buf->size, &read);
    buf->size += read;
    if(read > 0)
        buf->init = TRUE;
}

static void read_protocol_data(stgmed_buf_t *stgmed_buf)
{
    BYTE buf[8192];
    DWORD read;
    HRESULT hres;

    fill_stgmed_buffer(stgmed_buf);
    if(stgmed_buf->size < sizeof(stgmed_buf->buf))
        return;

    do hres = IInternetProtocol_Read(stgmed_buf->protocol, buf, sizeof(buf), &read);
    while(hres == S_OK);
}

static void dump_BINDINFO(BINDINFO *bi)
{
    static const char * const BINDINFOF_str[] = {
        "#0",
        "BINDINFOF_URLENCODESTGMEDDATA",
        "BINDINFOF_URLENCODEDEXTRAINFO"
    };

    static const char * const BINDVERB_str[] = {
        "BINDVERB_GET",
        "BINDVERB_POST",
        "BINDVERB_PUT",
        "BINDVERB_CUSTOM"
    };

    TRACE("\n"
            "BINDINFO = {\n"
            "    %d, %s,\n"
            "    {%d, %p, %p},\n"
            "    %s,\n"
            "    %s,\n"
            "    %s,\n"
            "    %d, %08x, %d, %d\n"
            "    {%d %p %x},\n"
            "    %s\n"
            "    %p, %d\n"
            "}\n",

            bi->cbSize, debugstr_w(bi->szExtraInfo),
            bi->stgmedData.tymed, bi->stgmedData.u.hGlobal, bi->stgmedData.pUnkForRelease,
            bi->grfBindInfoF > BINDINFOF_URLENCODEDEXTRAINFO
                ? "unknown" : BINDINFOF_str[bi->grfBindInfoF],
            bi->dwBindVerb > BINDVERB_CUSTOM
                ? "unknown" : BINDVERB_str[bi->dwBindVerb],
            debugstr_w(bi->szCustomVerb),
            bi->cbstgmedData, bi->dwOptions, bi->dwOptionsFlags, bi->dwCodePage,
            bi->securityAttributes.nLength,
            bi->securityAttributes.lpSecurityDescriptor,
            bi->securityAttributes.bInheritHandle,
            debugstr_guid(&bi->iid),
            bi->pUnk, bi->dwReserved
            );
}

static void mime_available(Binding *This, LPCWSTR mime)
{
    heap_free(This->mime);
    This->mime = heap_strdupW(mime);

    if(!This->mime || !This->report_mime)
        return;

    IBindStatusCallback_OnProgress(This->callback, 0, 0, BINDSTATUS_MIMETYPEAVAILABLE, This->mime);

    This->clipboard_format = RegisterClipboardFormatW(This->mime);
}

static void stop_binding(Binding *binding, HRESULT hres, LPCWSTR str)
{
    if(binding->state & BINDING_LOCKED) {
        IInternetProtocol_UnlockRequest(binding->protocol);
        binding->state &= ~BINDING_LOCKED;
    }

    if(!(binding->state & BINDING_STOPPED)) {
        binding->state |= BINDING_STOPPED;

        IBindStatusCallback_OnStopBinding(binding->callback, hres, str);
        binding->hres = hres;
    }
}

static LPWSTR get_mime_clsid(LPCWSTR mime, CLSID *clsid)
{
    LPWSTR key_name, ret;
    DWORD res, type, size;
    HKEY hkey;
    int len;
    HRESULT hres;

    static const WCHAR mime_keyW[] =
        {'M','I','M','E','\\','D','a','t','a','b','a','s','e','\\',
         'C','o','n','t','e','n','t',' ','T','y','p','e','\\'};
    static const WCHAR clsidW[] = {'C','L','S','I','D',0};

    len = strlenW(mime)+1;
    key_name = heap_alloc(sizeof(mime_keyW) + len*sizeof(WCHAR));
    memcpy(key_name, mime_keyW, sizeof(mime_keyW));
    strcpyW(key_name + sizeof(mime_keyW)/sizeof(WCHAR), mime);

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, key_name, &hkey);
    heap_free(key_name);
    if(res != ERROR_SUCCESS) {
        WARN("Could not open MIME key: %x\n", res);
        return NULL;
    }

    size = 50*sizeof(WCHAR);
    ret = heap_alloc(size);
    res = RegQueryValueExW(hkey, clsidW, NULL, &type, (LPBYTE)ret, &size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS) {
        WARN("Could not get CLSID: %08x\n", res);
        heap_free(ret);
        return NULL;
    }

    hres = CLSIDFromString(ret, clsid);
    if(FAILED(hres)) {
        WARN("Could not parse CLSID: %08x\n", hres);
        heap_free(ret);
        return NULL;
    }

    return ret;
}

static void load_doc_mon(Binding *binding, IPersistMoniker *persist)
{
    IBindCtx *bctx;
    HRESULT hres;

    hres = CreateAsyncBindCtxEx(binding->bctx, 0, NULL, NULL, &bctx, 0);
    if(FAILED(hres)) {
        WARN("CreateAsyncBindCtxEx failed: %08x\n", hres);
        return;
    }

    IBindCtx_RevokeObjectParam(bctx, bscb_holderW);
    IBindCtx_RegisterObjectParam(bctx, cbinding_contextW, (IUnknown*)BINDING(binding));

    hres = IPersistMoniker_Load(persist, binding->download_state == END_DOWNLOAD, binding->mon, bctx, 0x12);
    IBindCtx_RevokeObjectParam(bctx, cbinding_contextW);
    IBindCtx_Release(bctx);
    if(FAILED(hres))
        FIXME("Load failed: %08x\n", hres);
}

static HRESULT create_mime_object(Binding *binding, const CLSID *clsid, LPCWSTR clsid_str)
{
    IPersistMoniker *persist;
    HRESULT hres;

    hres = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                            &binding->iid, (void**)&binding->obj);
    if(FAILED(hres)) {
        WARN("CoCreateInstance failed: %08x\n", hres);
        return INET_E_CANNOT_INSTANTIATE_OBJECT;
    }

    binding->state |= BINDING_OBJAVAIL;

    hres = IUnknown_QueryInterface(binding->obj, &IID_IPersistMoniker, (void**)&persist);
    if(SUCCEEDED(hres)) {
        IMonikerProp *prop;

        hres = IPersistMoniker_QueryInterface(persist, &IID_IMonikerProp, (void**)&prop);
        if(SUCCEEDED(hres)) {
            IMonikerProp_PutProperty(prop, MIMETYPEPROP, binding->mime);
            IMonikerProp_PutProperty(prop, CLASSIDPROP, clsid_str);
            IMonikerProp_Release(prop);
        }

        load_doc_mon(binding, persist);

        IPersistMoniker_Release(persist);
    }else {
        FIXME("Could not get IPersistMoniker: %08x\n", hres);
        /* FIXME: Try query IPersistFile */
    }

    IBindStatusCallback_OnObjectAvailable(binding->callback, &binding->iid, binding->obj);

    return S_OK;
}

static void create_object(Binding *binding)
{
    LPWSTR clsid_str;
    CLSID clsid;
    HRESULT hres;

    if(!binding->mime) {
        FIXME("MIME not available\n");
        return;
    }

    if(!(clsid_str = get_mime_clsid(binding->mime, &clsid))) {
        FIXME("Could not find object for MIME %s\n", debugstr_w(binding->mime));
        return;
    }

    IBindStatusCallback_OnProgress(binding->callback, 0, 0, BINDSTATUS_CLASSIDAVAILABLE, clsid_str);
    IBindStatusCallback_OnProgress(binding->callback, 0, 0, BINDSTATUS_BEGINSYNCOPERATION, NULL);

    hres = create_mime_object(binding, &clsid, clsid_str);
    heap_free(clsid_str);

    IBindStatusCallback_OnProgress(binding->callback, 0, 0, BINDSTATUS_ENDSYNCOPERATION, NULL);

    stop_binding(binding, hres, NULL);
    if(FAILED(hres))
        IInternetProtocol_Terminate(binding->protocol, 0);
}

static void cache_file_available(Binding *This, const WCHAR *file_name)
{
    heap_free(This->stgmed_buf->cache_file);
    This->stgmed_buf->cache_file = heap_strdupW(file_name);

    if(This->use_cache_file) {
        This->stgmed_buf->file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if(This->stgmed_buf->file == INVALID_HANDLE_VALUE)
            WARN("CreateFile failed: %u\n", GetLastError());
    }
}

#define STGMEDUNK_THIS(iface) DEFINE_THIS(stgmed_buf_t, Unknown, iface)

static HRESULT WINAPI StgMedUnk_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    stgmed_buf_t *This = STGMEDUNK_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);

        *ppv = STGMEDUNK(This);
        IUnknown_AddRef(STGMEDUNK(This));
        return S_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI StgMedUnk_AddRef(IUnknown *iface)
{
    stgmed_buf_t *This = STGMEDUNK_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI StgMedUnk_Release(IUnknown *iface)
{
    stgmed_buf_t *This = STGMEDUNK_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->file != INVALID_HANDLE_VALUE)
            CloseHandle(This->file);
        IInternetProtocol_Release(This->protocol);
        heap_free(This->cache_file);
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

#undef STGMEDUNK_THIS

static const IUnknownVtbl StgMedUnkVtbl = {
    StgMedUnk_QueryInterface,
    StgMedUnk_AddRef,
    StgMedUnk_Release
};

static stgmed_buf_t *create_stgmed_buf(IInternetProtocolEx *protocol)
{
    stgmed_buf_t *ret = heap_alloc(sizeof(*ret));

    ret->lpUnknownVtbl = &StgMedUnkVtbl;
    ret->ref = 1;
    ret->size = 0;
    ret->init = FALSE;
    ret->file = INVALID_HANDLE_VALUE;
    ret->hres = S_OK;
    ret->cache_file = NULL;

    IInternetProtocol_AddRef(protocol);
    ret->protocol = protocol;

    URLMON_LockModule();

    return ret;
}

typedef struct {
    stgmed_obj_t stgmed_obj;
    const IStreamVtbl *lpStreamVtbl;

    LONG ref;

    stgmed_buf_t *buf;
} ProtocolStream;

#define STREAM_THIS(iface) DEFINE_THIS(ProtocolStream, Stream, iface)

static HRESULT WINAPI ProtocolStream_QueryInterface(IStream *iface,
                                                          REFIID riid, void **ppv)
{
    ProtocolStream *This = STREAM_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = STREAM(This);
    }else if(IsEqualGUID(&IID_ISequentialStream, riid)) {
        TRACE("(%p)->(IID_ISequentialStream %p)\n", This, ppv);
        *ppv = STREAM(This);
    }else if(IsEqualGUID(&IID_IStream, riid)) {
        TRACE("(%p)->(IID_IStream %p)\n", This, ppv);
        *ppv = STREAM(This);
    }

    if(*ppv) {
        IStream_AddRef(STREAM(This));
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolStream_AddRef(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ProtocolStream_Release(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IUnknown_Release(STGMEDUNK(This->buf));
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI ProtocolStream_Read(IStream *iface, void *pv,
                                          ULONG cb, ULONG *pcbRead)
{
    ProtocolStream *This = STREAM_THIS(iface);
    DWORD read = 0, pread = 0;
    HRESULT hres;

    TRACE("(%p)->(%p %d %p)\n", This, pv, cb, pcbRead);

    if(This->buf->file != INVALID_HANDLE_VALUE) {
        if (!ReadFile(This->buf->file, pv, cb, &read, NULL))
            return INET_E_DOWNLOAD_FAILURE;

        if(pcbRead)
            *pcbRead = read;
        return read ? S_OK : S_FALSE;
    }

    if(This->buf->size) {
        read = cb;

        if(read > This->buf->size)
            read = This->buf->size;

        memcpy(pv, This->buf->buf, read);

        if(read < This->buf->size)
            memmove(This->buf->buf, This->buf->buf+read, This->buf->size-read);
        This->buf->size -= read;
    }

    if(read == cb) {
        if (pcbRead)
            *pcbRead = read;
        return S_OK;
    }

    hres = This->buf->hres = IInternetProtocol_Read(This->buf->protocol, (PBYTE)pv+read, cb-read, &pread);
    if (pcbRead)
        *pcbRead = read + pread;

    if(hres == E_PENDING)
        return E_PENDING;
    else if(FAILED(hres))
        FIXME("Read failed: %08x\n", hres);

    return read || pread ? S_OK : S_FALSE;
}

static HRESULT WINAPI ProtocolStream_Write(IStream *iface, const void *pv,
                                          ULONG cb, ULONG *pcbWritten)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)->(%p %d %p)\n", This, pv, cb, pcbWritten);

    return STG_E_ACCESSDENIED;
}

static HRESULT WINAPI ProtocolStream_Seek(IStream *iface, LARGE_INTEGER dlibMove,
                                         DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%d %08x %p)\n", This, dlibMove.u.LowPart, dwOrigin, plibNewPosition);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%d)\n", This, libNewSize.u.LowPart);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_CopyTo(IStream *iface, IStream *pstm,
        ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%p %d %p %p)\n", This, pstm, cb.u.LowPart, pcbRead, pcbWritten);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, grfCommitFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Revert(IStream *iface)
{
    ProtocolStream *This = STREAM_THIS(iface);

    TRACE("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                               ULARGE_INTEGER cb, DWORD dwLockType)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%d %d %d)\n", This, libOffset.u.LowPart, cb.u.LowPart, dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_UnlockRegion(IStream *iface,
        ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%d %d %d)\n", This, libOffset.u.LowPart, cb.u.LowPart, dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolStream_Stat(IStream *iface, STATSTG *pstatstg,
                                         DWORD dwStatFlag)
{
    ProtocolStream *This = STREAM_THIS(iface);
    TRACE("(%p)->(%p %08x)\n", This, pstatstg, dwStatFlag);

    if(!pstatstg)
        return E_FAIL;

    memset(pstatstg, 0, sizeof(STATSTG));

    if(!(dwStatFlag&STATFLAG_NONAME) && This->buf->cache_file) {
        pstatstg->pwcsName = CoTaskMemAlloc((lstrlenW(This->buf->cache_file)+1)*sizeof(WCHAR));
        if(!pstatstg->pwcsName)
            return STG_E_INSUFFICIENTMEMORY;

        lstrcpyW(pstatstg->pwcsName, This->buf->cache_file);
    }

    pstatstg->type = STGTY_STREAM;
    if(This->buf->file != INVALID_HANDLE_VALUE) {
        GetFileSizeEx(This->buf->file, (PLARGE_INTEGER)&pstatstg->cbSize);
        GetFileTime(This->buf->file, &pstatstg->ctime, &pstatstg->atime, &pstatstg->mtime);
        if(pstatstg->cbSize.QuadPart)
            pstatstg->grfMode = GENERIC_READ;
    }

    return S_OK;
}

static HRESULT WINAPI ProtocolStream_Clone(IStream *iface, IStream **ppstm)
{
    ProtocolStream *This = STREAM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppstm);
    return E_NOTIMPL;
}

#undef STREAM_THIS

static const IStreamVtbl ProtocolStreamVtbl = {
    ProtocolStream_QueryInterface,
    ProtocolStream_AddRef,
    ProtocolStream_Release,
    ProtocolStream_Read,
    ProtocolStream_Write,
    ProtocolStream_Seek,
    ProtocolStream_SetSize,
    ProtocolStream_CopyTo,
    ProtocolStream_Commit,
    ProtocolStream_Revert,
    ProtocolStream_LockRegion,
    ProtocolStream_UnlockRegion,
    ProtocolStream_Stat,
    ProtocolStream_Clone
};

static void stgmed_stream_release(stgmed_obj_t *obj)
{
    ProtocolStream *stream = (ProtocolStream*)obj;
    IStream_Release(STREAM(stream));
}

static HRESULT stgmed_stream_fill_stgmed(stgmed_obj_t *obj, STGMEDIUM *stgmed)
{
    ProtocolStream *stream = (ProtocolStream*)obj;

    stgmed->tymed = TYMED_ISTREAM;
    stgmed->u.pstm = STREAM(stream);
    stgmed->pUnkForRelease = STGMEDUNK(stream->buf);

    return S_OK;
}

static HRESULT stgmed_stream_get_result(stgmed_obj_t *obj, DWORD bindf, void **result)
{
    ProtocolStream *stream = (ProtocolStream*)obj;

    if(!(bindf & BINDF_ASYNCHRONOUS) && stream->buf->file == INVALID_HANDLE_VALUE
       && (stream->buf->hres != S_FALSE || stream->buf->size))
        return INET_E_DATA_NOT_AVAILABLE;

    IStream_AddRef(STREAM(stream));
    *result = STREAM(stream);
    return S_OK;
}

static const stgmed_obj_vtbl stgmed_stream_vtbl = {
    stgmed_stream_release,
    stgmed_stream_fill_stgmed,
    stgmed_stream_get_result
};

typedef struct {
    stgmed_obj_t stgmed_obj;
    stgmed_buf_t *buf;
} stgmed_file_obj_t;

static stgmed_obj_t *create_stgmed_stream(stgmed_buf_t *buf)
{
    ProtocolStream *ret = heap_alloc(sizeof(ProtocolStream));

    ret->stgmed_obj.vtbl = &stgmed_stream_vtbl;
    ret->lpStreamVtbl = &ProtocolStreamVtbl;
    ret->ref = 1;

    IUnknown_AddRef(STGMEDUNK(buf));
    ret->buf = buf;

    URLMON_LockModule();

    return &ret->stgmed_obj;
}

static void stgmed_file_release(stgmed_obj_t *obj)
{
    stgmed_file_obj_t *file_obj = (stgmed_file_obj_t*)obj;

    IUnknown_Release(STGMEDUNK(file_obj->buf));
    heap_free(file_obj);
}

static HRESULT stgmed_file_fill_stgmed(stgmed_obj_t *obj, STGMEDIUM *stgmed)
{
    stgmed_file_obj_t *file_obj = (stgmed_file_obj_t*)obj;

    if(!file_obj->buf->cache_file) {
        WARN("cache_file not set\n");
        return INET_E_DATA_NOT_AVAILABLE;
    }

    read_protocol_data(file_obj->buf);

    stgmed->tymed = TYMED_FILE;
    stgmed->u.lpszFileName = file_obj->buf->cache_file;
    stgmed->pUnkForRelease = STGMEDUNK(file_obj->buf);

    return S_OK;
}

static HRESULT stgmed_file_get_result(stgmed_obj_t *obj, DWORD bindf, void **result)
{
    return bindf & BINDF_ASYNCHRONOUS ? MK_S_ASYNCHRONOUS : S_OK;
}

static const stgmed_obj_vtbl stgmed_file_vtbl = {
    stgmed_file_release,
    stgmed_file_fill_stgmed,
    stgmed_file_get_result
};

static stgmed_obj_t *create_stgmed_file(stgmed_buf_t *buf)
{
    stgmed_file_obj_t *ret = heap_alloc(sizeof(*ret));

    ret->stgmed_obj.vtbl = &stgmed_file_vtbl;

    IUnknown_AddRef(STGMEDUNK(buf));
    ret->buf = buf;

    return &ret->stgmed_obj;
}

#define BINDING_THIS(iface) DEFINE_THIS(Binding, Binding, iface)

static HRESULT WINAPI Binding_QueryInterface(IBinding *iface, REFIID riid, void **ppv)
{
    Binding *This = BINDING_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = BINDING(This);
    }else if(IsEqualGUID(&IID_IBinding, riid)) {
        TRACE("(%p)->(IID_IBinding %p)\n", This, ppv);
        *ppv = BINDING(This);
    }else if(IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        TRACE("(%p)->(IID_IInternetProtocolSink %p)\n", This, ppv);
        *ppv = PROTSINK(This);
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINF(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }else if(IsEqualGUID(&IID_IWinInetInfo, riid)) {
        TRACE("(%p)->(IID_IWinInetInfo %p)\n", This, ppv);

        /* NOTE: This violidates COM rules, but tests prove that we should do it */
        if(!get_wininet_info(This->protocol))
           return E_NOINTERFACE;

        *ppv = INETINFO(This);
    }else if(IsEqualGUID(&IID_IWinInetHttpInfo, riid)) {
        IWinInetHttpInfo *http_info;
        IWinInetInfo *info;
        HRESULT hres;

        TRACE("(%p)->(IID_IWinInetHttpInfo %p)\n", This, ppv);

        info = get_wininet_info(This->protocol);
        if(!info)
            return E_NOINTERFACE;

        hres = IWinInetInfo_QueryInterface(info, &IID_IWinInetHttpInfo, (void**)&http_info);
        if(FAILED(hres))
            return E_NOINTERFACE;

        IWinInetHttpInfo_Release(http_info);
        *ppv = INETINFO(This);
    }

    if(*ppv) {
        IBinding_AddRef(BINDING(This));
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Binding_AddRef(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI Binding_Release(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->notif_hwnd)
            release_notif_hwnd(This->notif_hwnd);
        if(This->mon)
            IMoniker_Release(This->mon);
        if(This->callback)
            IBindStatusCallback_Release(This->callback);
        if(This->protocol)
            IInternetProtocol_Release(This->protocol);
        if(This->service_provider)
            IServiceProvider_Release(This->service_provider);
        if(This->stgmed_buf)
            IUnknown_Release(STGMEDUNK(This->stgmed_buf));
        if(This->stgmed_obj)
            This->stgmed_obj->vtbl->release(This->stgmed_obj);
        if(This->obj)
            IUnknown_Release(This->obj);
        if(This->bctx)
            IBindCtx_Release(This->bctx);

        ReleaseBindInfo(&This->bindinfo);
        This->section.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->section);
        SysFreeString(This->url);
        heap_free(This->mime);
        heap_free(This->redirect_url);
        heap_free(This);

        URLMON_UnlockModule();
    }

    return ref;
}

static HRESULT WINAPI Binding_Abort(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    HRESULT hres;

    TRACE("(%p)\n", This);

    if(This->state & BINDING_ABORTED)
        return E_FAIL;

    hres = IInternetProtocol_Abort(This->protocol, E_ABORT, ERROR_SUCCESS);
    if(FAILED(hres))
        return hres;

    This->state |= BINDING_ABORTED;
    return S_OK;
}

static HRESULT WINAPI Binding_Suspend(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Resume(IBinding *iface)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_SetPriority(IBinding *iface, LONG nPriority)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%d)\n", This, nPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetPriority(IBinding *iface, LONG *pnPriority)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetBindResult(IBinding *iface, CLSID *pclsidProtocol,
        DWORD *pdwResult, LPOLESTR *pszResult, DWORD *pdwReserved)
{
    Binding *This = BINDING_THIS(iface);
    FIXME("(%p)->(%p %p %p %p)\n", This, pclsidProtocol, pdwResult, pszResult, pdwReserved);
    return E_NOTIMPL;
}

static Binding *get_bctx_binding(IBindCtx *bctx)
{
    IBinding *binding;
    IUnknown *unk;
    HRESULT hres;

    hres = IBindCtx_GetObjectParam(bctx, cbinding_contextW, &unk);
    if(FAILED(hres))
        return NULL;

    hres = IUnknown_QueryInterface(unk, &IID_IBinding, (void*)&binding);
    IUnknown_Release(unk);
    if(FAILED(hres))
        return NULL;

    /* FIXME!!! */
    return BINDING_THIS(binding);
}

#undef BINDING_THIS

static const IBindingVtbl BindingVtbl = {
    Binding_QueryInterface,
    Binding_AddRef,
    Binding_Release,
    Binding_Abort,
    Binding_Suspend,
    Binding_Resume,
    Binding_SetPriority,
    Binding_GetPriority,
    Binding_GetBindResult
};

#define PROTSINK_THIS(iface) DEFINE_THIS(Binding, IInternetProtocolSink, iface)

static HRESULT WINAPI InternetProtocolSink_QueryInterface(IInternetProtocolSink *iface,
        REFIID riid, void **ppv)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI InternetProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI InternetProtocolSink_Release(IInternetProtocolSink *iface)
{
    Binding *This = PROTSINK_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI InternetProtocolSink_Switch(IInternetProtocolSink *iface,
        PROTOCOLDATA *pProtocolData)
{
    Binding *This = PROTSINK_THIS(iface);

    WARN("(%p)->(%p)\n", This, pProtocolData);

    return E_FAIL;
}

static void on_progress(Binding *This, ULONG progress, ULONG progress_max,
                        ULONG status_code, LPCWSTR status_text)
{
    IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
            status_code, status_text);
}

static HRESULT WINAPI InternetProtocolSink_ReportProgress(IInternetProtocolSink *iface,
        ULONG ulStatusCode, LPCWSTR szStatusText)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%u %s)\n", This, ulStatusCode, debugstr_w(szStatusText));

    switch(ulStatusCode) {
    case BINDSTATUS_FINDINGRESOURCE:
        on_progress(This, 0, 0, BINDSTATUS_FINDINGRESOURCE, szStatusText);
        break;
    case BINDSTATUS_CONNECTING:
        on_progress(This, 0, 0, BINDSTATUS_CONNECTING, szStatusText);
        break;
    case BINDSTATUS_REDIRECTING:
        heap_free(This->redirect_url);
        This->redirect_url = heap_strdupW(szStatusText);
        on_progress(This, 0, 0, BINDSTATUS_REDIRECTING, szStatusText);
        break;
    case BINDSTATUS_BEGINDOWNLOADDATA:
        fill_stgmed_buffer(This->stgmed_buf);
        break;
    case BINDSTATUS_SENDINGREQUEST:
        on_progress(This, 0, 0, BINDSTATUS_SENDINGREQUEST, szStatusText);
        break;
    case BINDSTATUS_PROTOCOLCLASSID:
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        mime_available(This, szStatusText);
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        cache_file_available(This, szStatusText);
        break;
    case BINDSTATUS_DECODING:
        IBindStatusCallback_OnProgress(This->callback, 0, 0, BINDSTATUS_DECODING, szStatusText);
        break;
    case BINDSTATUS_LOADINGMIMEHANDLER:
        on_progress(This, 0, 0, BINDSTATUS_LOADINGMIMEHANDLER, szStatusText);
        break;
    case BINDSTATUS_DIRECTBIND: /* FIXME: Handle BINDSTATUS_DIRECTBIND in BindProtocol */
        This->report_mime = FALSE;
        break;
    case BINDSTATUS_ACCEPTRANGES:
        break;
    default:
        FIXME("Unhandled status code %d\n", ulStatusCode);
        return E_NOTIMPL;
    };

    return S_OK;
}

static void report_data(Binding *This, DWORD bscf, ULONG progress, ULONG progress_max)
{
    FORMATETC formatetc = {0, NULL, 1, -1, TYMED_ISTREAM};
    BOOL sent_begindownloaddata = FALSE;

    TRACE("(%p)->(%d %u %u)\n", This, bscf, progress, progress_max);

    if(This->download_state == END_DOWNLOAD || (This->state & (BINDING_STOPPED|BINDING_ABORTED)))
        return;

    if(This->stgmed_buf->file != INVALID_HANDLE_VALUE)
        read_protocol_data(This->stgmed_buf);
    else if(This->download_state == BEFORE_DOWNLOAD)
        fill_stgmed_buffer(This->stgmed_buf);

    if(This->download_state == BEFORE_DOWNLOAD) {
        This->download_state = DOWNLOADING;
        sent_begindownloaddata = TRUE;
        IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                BINDSTATUS_BEGINDOWNLOADDATA, This->url);

        if(This->stgmed_buf->cache_file)
            IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                    BINDSTATUS_CACHEFILENAMEAVAILABLE, This->stgmed_buf->cache_file);
    }

    if(This->stgmed_buf->hres == S_FALSE || (bscf & BSCF_LASTDATANOTIFICATION)) {
        This->download_state = END_DOWNLOAD;
        IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                BINDSTATUS_ENDDOWNLOADDATA, This->url);
    }else if(!sent_begindownloaddata) {
        IBindStatusCallback_OnProgress(This->callback, progress, progress_max,
                BINDSTATUS_DOWNLOADINGDATA, This->url);
    }

    if(This->state & (BINDING_STOPPED|BINDING_ABORTED))
        return;

    if(This->to_object) {
        if(!(This->state & BINDING_OBJAVAIL)) {
            IBinding_AddRef(BINDING(This));
            create_object(This);
            IBinding_Release(BINDING(This));
        }
    }else {
        STGMEDIUM stgmed;
        HRESULT hres;

        if(!(This->state & BINDING_LOCKED)) {
            HRESULT hres = IInternetProtocol_LockRequest(This->protocol, 0);
            if(SUCCEEDED(hres))
                This->state |= BINDING_LOCKED;
        }

        hres = This->stgmed_obj->vtbl->fill_stgmed(This->stgmed_obj, &stgmed);
        if(FAILED(hres)) {
            stop_binding(This, hres, NULL);
            return;
        }

        formatetc.tymed = stgmed.tymed;
        formatetc.cfFormat = This->clipboard_format;

        IBindStatusCallback_OnDataAvailable(This->callback, bscf, progress,
                &formatetc, &stgmed);

        if(This->download_state == END_DOWNLOAD)
            stop_binding(This, S_OK, NULL);
    }
}

static HRESULT WINAPI InternetProtocolSink_ReportData(IInternetProtocolSink *iface,
        DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%d %u %u)\n", This, grfBSCF, ulProgress, ulProgressMax);

    report_data(This, grfBSCF, ulProgress, ulProgressMax);
    return S_OK;
}

static HRESULT WINAPI InternetProtocolSink_ReportResult(IInternetProtocolSink *iface,
        HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    Binding *This = PROTSINK_THIS(iface);

    TRACE("(%p)->(%08x %d %s)\n", This, hrResult, dwError, debugstr_w(szResult));

    stop_binding(This, hrResult, szResult);
    IInternetProtocol_Terminate(This->protocol, 0);
    return S_OK;
}

#undef PROTSINK_THIS

static const IInternetProtocolSinkVtbl InternetProtocolSinkVtbl = {
    InternetProtocolSink_QueryInterface,
    InternetProtocolSink_AddRef,
    InternetProtocolSink_Release,
    InternetProtocolSink_Switch,
    InternetProtocolSink_ReportProgress,
    InternetProtocolSink_ReportData,
    InternetProtocolSink_ReportResult
};

#define BINDINF_THIS(iface) DEFINE_THIS(Binding, InternetBindInfo, iface)

static HRESULT WINAPI InternetBindInfo_QueryInterface(IInternetBindInfo *iface,
        REFIID riid, void **ppv)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI InternetBindInfo_AddRef(IInternetBindInfo *iface)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI InternetBindInfo_Release(IInternetBindInfo *iface)
{
    Binding *This = BINDINF_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI InternetBindInfo_GetBindInfo(IInternetBindInfo *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    Binding *This = BINDINF_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = This->bindf;

    *pbindinfo = This->bindinfo;

    if(pbindinfo->szExtraInfo || pbindinfo->szCustomVerb)
        FIXME("copy strings\n");

    if(pbindinfo->stgmedData.pUnkForRelease)
        IUnknown_AddRef(pbindinfo->stgmedData.pUnkForRelease);

    if(pbindinfo->pUnk)
        IUnknown_AddRef(pbindinfo->pUnk);

    return S_OK;
}

static HRESULT WINAPI InternetBindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    Binding *This = BINDINF_THIS(iface);

    TRACE("(%p)->(%d %p %d %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);

    switch(ulStringType) {
    case BINDSTRING_ACCEPT_MIMES: {
        static const WCHAR wszMimes[] = {'*','/','*',0};

        if(!ppwzStr || !pcElFetched)
            return E_INVALIDARG;

        ppwzStr[0] = CoTaskMemAlloc(sizeof(wszMimes));
        memcpy(ppwzStr[0], wszMimes, sizeof(wszMimes));
        *pcElFetched = 1;
        return S_OK;
    }
    case BINDSTRING_USER_AGENT: {
        IInternetBindInfo *bindinfo = NULL;
        HRESULT hres;

        hres = IBindStatusCallback_QueryInterface(This->callback, &IID_IInternetBindInfo,
                                                  (void**)&bindinfo);
        if(FAILED(hres))
            return hres;

        hres = IInternetBindInfo_GetBindString(bindinfo, ulStringType, ppwzStr,
                                               cEl, pcElFetched);
        IInternetBindInfo_Release(bindinfo);

        return hres;
    }
    case BINDSTRING_URL: {
        DWORD size = (SysStringLen(This->url)+1) * sizeof(WCHAR);

        if(!ppwzStr || !pcElFetched)
            return E_INVALIDARG;

        *ppwzStr = CoTaskMemAlloc(size);
        memcpy(*ppwzStr, This->url, size);
        *pcElFetched = 1;
        return S_OK;
    }
    }

    FIXME("not supported string type %d\n", ulStringType);
    return E_NOTIMPL;
}

#undef BINDF_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    InternetBindInfo_QueryInterface,
    InternetBindInfo_AddRef,
    InternetBindInfo_Release,
    InternetBindInfo_GetBindInfo,
    InternetBindInfo_GetBindString
};

#define INETINFO_THIS(iface) DEFINE_THIS(Binding, WinInetHttpInfo, iface)

static HRESULT WINAPI WinInetHttpInfo_QueryInterface(IWinInetHttpInfo *iface, REFIID riid, void **ppv)
{
    Binding *This = INETINFO_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI WinInetHttpInfo_AddRef(IWinInetHttpInfo *iface)
{
    Binding *This = INETINFO_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI WinInetHttpInfo_Release(IWinInetHttpInfo *iface)
{
    Binding *This = INETINFO_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI WinInetHttpInfo_QueryOption(IWinInetHttpInfo *iface, DWORD dwOption,
        void *pBuffer, DWORD *pcbBuffer)
{
    Binding *This = INETINFO_THIS(iface);
    FIXME("(%p)->(%x %p %p)\n", This, dwOption, pBuffer, pcbBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI WinInetHttpInfo_QueryInfo(IWinInetHttpInfo *iface, DWORD dwOption,
        void *pBuffer, DWORD *pcbBuffer, DWORD *pdwFlags, DWORD *pdwReserved)
{
    Binding *This = INETINFO_THIS(iface);
    FIXME("(%p)->(%x %p %p %p %p)\n", This, dwOption, pBuffer, pcbBuffer, pdwFlags, pdwReserved);
    return E_NOTIMPL;
}

#undef INETINFO_THIS

static const IWinInetHttpInfoVtbl WinInetHttpInfoVtbl = {
    WinInetHttpInfo_QueryInterface,
    WinInetHttpInfo_AddRef,
    WinInetHttpInfo_Release,
    WinInetHttpInfo_QueryOption,
    WinInetHttpInfo_QueryInfo
};

#define SERVPROV_THIS(iface) DEFINE_THIS(Binding, ServiceProvider, iface)

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface,
        REFIID riid, void **ppv)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_QueryInterface(BINDING(This), riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_AddRef(BINDING(This));
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    Binding *This = SERVPROV_THIS(iface);
    return IBinding_Release(BINDING(This));
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    Binding *This = SERVPROV_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(This->service_provider) {
        hres = IServiceProvider_QueryService(This->service_provider, guidService,
                                             riid, ppv);
        if(SUCCEEDED(hres))
            return hres;
    }

    WARN("unknown service %s\n", debugstr_guid(guidService));
    return E_NOTIMPL;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static HRESULT get_callback(IBindCtx *pbc, IBindStatusCallback **callback)
{
    IUnknown *unk;
    HRESULT hres;

    hres = IBindCtx_GetObjectParam(pbc, bscb_holderW, &unk);
    if(FAILED(hres))
        return create_default_callback(callback);

    hres = IUnknown_QueryInterface(unk, &IID_IBindStatusCallback, (void**)callback);
    IUnknown_Release(unk);
    return hres;
}

static BOOL is_urlmon_protocol(IUri *uri)
{
    DWORD scheme;
    HRESULT hres;

    hres = IUri_GetScheme(uri, &scheme);
    if(FAILED(hres))
        return FALSE;

    switch(scheme) {
    case URL_SCHEME_FILE:
    case URL_SCHEME_FTP:
    case URL_SCHEME_GOPHER:
    case URL_SCHEME_HTTP:
    case URL_SCHEME_HTTPS:
    case URL_SCHEME_MK:
        return TRUE;
    }

    return FALSE;
}

static HRESULT Binding_Create(IMoniker *mon, Binding *binding_ctx, IUri *uri, IBindCtx *pbc,
        BOOL to_obj, REFIID riid, Binding **binding)
{
    Binding *ret;
    HRESULT hres;

    URLMON_LockModule();

    ret = heap_alloc_zero(sizeof(Binding));

    ret->lpBindingVtbl              = &BindingVtbl;
    ret->lpIInternetProtocolSinkVtbl = &InternetProtocolSinkVtbl;
    ret->lpInternetBindInfoVtbl     = &InternetBindInfoVtbl;
    ret->lpWinInetHttpInfoVtbl      = &WinInetHttpInfoVtbl;
    ret->lpServiceProviderVtbl      = &ServiceProviderVtbl;

    ret->ref = 1;

    ret->to_object = to_obj;
    ret->iid = *riid;
    ret->notif_hwnd = get_notif_hwnd();
    ret->report_mime = !binding_ctx;
    ret->download_state = BEFORE_DOWNLOAD;

    if(to_obj) {
        IBindCtx_AddRef(pbc);
        ret->bctx = pbc;
    }

    if(mon) {
        IMoniker_AddRef(mon);
        ret->mon = mon;
    }

    ret->bindinfo.cbSize = sizeof(BINDINFO);

    InitializeCriticalSection(&ret->section);
    ret->section.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": Binding.section");

    hres = get_callback(pbc, &ret->callback);
    if(FAILED(hres)) {
        WARN("Could not get IBindStatusCallback\n");
        IBinding_Release(BINDING(ret));
        return hres;
    }

    IBindStatusCallback_QueryInterface(ret->callback, &IID_IServiceProvider,
                                       (void**)&ret->service_provider);

    if(binding_ctx) {
        ret->protocol = binding_ctx->protocol;
        IInternetProtocol_AddRef(ret->protocol);
    }else {
        hres = create_binding_protocol(TRUE, &ret->protocol);
        if(FAILED(hres)) {
            WARN("Could not get protocol handler\n");
            IBinding_Release(BINDING(ret));
            return hres;
        }
    }

    hres = IBindStatusCallback_GetBindInfo(ret->callback, &ret->bindf, &ret->bindinfo);
    if(FAILED(hres)) {
        WARN("GetBindInfo failed: %08x\n", hres);
        IBinding_Release(BINDING(ret));
        return hres;
    }

    TRACE("bindf %08x\n", ret->bindf);
    dump_BINDINFO(&ret->bindinfo);

    ret->bindf |= BINDF_FROMURLMON;
    if(to_obj)
        ret->bindinfo.dwOptions |= 0x100000;

    if(!(ret->bindf & BINDF_ASYNCHRONOUS)) {
        ret->bindf |= BINDF_NEEDFILE;
        ret->use_cache_file = TRUE;
    }else if(!is_urlmon_protocol(uri)) {
        ret->bindf |= BINDF_NEEDFILE;
    }

    hres = IUri_GetDisplayUri(uri, &ret->url);
    if(FAILED(hres)) {
        IBinding_Release(BINDING(ret));
        return hres;
    }

    if(binding_ctx) {
        ret->stgmed_buf = binding_ctx->stgmed_buf;
        IUnknown_AddRef(STGMEDUNK(ret->stgmed_buf));
        ret->clipboard_format = binding_ctx->clipboard_format;
    }else {
        ret->stgmed_buf = create_stgmed_buf(ret->protocol);
    }

    if(to_obj) {
        ret->stgmed_obj = NULL;
    }else if(IsEqualGUID(&IID_IStream, riid)) {
        ret->stgmed_obj = create_stgmed_stream(ret->stgmed_buf);
    }else if(IsEqualGUID(&IID_IUnknown, riid)) {
        ret->bindf |= BINDF_NEEDFILE;
        ret->stgmed_obj = create_stgmed_file(ret->stgmed_buf);
    }else {
        FIXME("Unsupported riid %s\n", debugstr_guid(riid));
        IBinding_Release(BINDING(ret));
        return E_NOTIMPL;
    }

    *binding = ret;
    return S_OK;
}

static HRESULT start_binding(IMoniker *mon, Binding *binding_ctx, IUri *uri, IBindCtx *pbc,
                             BOOL to_obj, REFIID riid, Binding **ret)
{
    Binding *binding = NULL;
    HRESULT hres;
    MSG msg;

    hres = Binding_Create(mon, binding_ctx, uri, pbc, to_obj, riid, &binding);
    if(FAILED(hres))
        return hres;

    hres = IBindStatusCallback_OnStartBinding(binding->callback, 0, BINDING(binding));
    if(FAILED(hres)) {
        WARN("OnStartBinding failed: %08x\n", hres);
        stop_binding(binding, INET_E_DOWNLOAD_FAILURE, NULL);
        IBinding_Release(BINDING(binding));
        return hres;
    }

    if(binding_ctx) {
        set_binding_sink(binding->protocol, PROTSINK(binding), BINDINF(binding));
        if(binding_ctx->redirect_url)
            IBindStatusCallback_OnProgress(binding->callback, 0, 0, BINDSTATUS_REDIRECTING, binding_ctx->redirect_url);
        report_data(binding, BSCF_FIRSTDATANOTIFICATION | (binding_ctx->download_state == END_DOWNLOAD ? BSCF_LASTDATANOTIFICATION : 0),
                0, 0);
    }else {
        hres = IInternetProtocolEx_StartEx(binding->protocol, uri, PROTSINK(binding),
                 BINDINF(binding), PI_APARTMENTTHREADED|PI_MIMEVERIFICATION, 0);

        TRACE("start ret %08x\n", hres);

        if(FAILED(hres) && hres != E_PENDING) {
            stop_binding(binding, hres, NULL);
            IBinding_Release(BINDING(binding));

            return hres;
        }
    }

    while(!(binding->bindf & BINDF_ASYNCHRONOUS) &&
          !(binding->state & BINDING_STOPPED)) {
        MsgWaitForMultipleObjects(0, NULL, FALSE, 5000, QS_POSTMESSAGE);
        while (PeekMessageW(&msg, binding->notif_hwnd, WM_USER, WM_USER+117, PM_REMOVE|PM_NOYIELD)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    *ret = binding;
    return S_OK;
}

HRESULT bind_to_storage(IUri *uri, IBindCtx *pbc, REFIID riid, void **ppv)
{
    Binding *binding = NULL, *binding_ctx;
    HRESULT hres;

    *ppv = NULL;

    binding_ctx = get_bctx_binding(pbc);

    hres = start_binding(NULL, binding_ctx, uri, pbc, FALSE, riid, &binding);
    if(binding_ctx)
        IBinding_Release(BINDING(binding_ctx));
    if(FAILED(hres))
        return hres;

    if(binding->hres == S_OK && binding->stgmed_buf->init) {
        if((binding->state & BINDING_STOPPED) && (binding->state & BINDING_LOCKED))
            IInternetProtocol_UnlockRequest(binding->protocol);

        hres = binding->stgmed_obj->vtbl->get_result(binding->stgmed_obj, binding->bindf, ppv);
    }else if(binding->bindf & BINDF_ASYNCHRONOUS) {
        hres = MK_S_ASYNCHRONOUS;
    }else {
        hres = FAILED(binding->hres) ? binding->hres : S_OK;
    }

    IBinding_Release(BINDING(binding));

    return hres;
}

HRESULT bind_to_object(IMoniker *mon, IUri *uri, IBindCtx *pbc, REFIID riid, void **ppv)
{
    Binding *binding;
    HRESULT hres;

    *ppv = NULL;

    hres = start_binding(mon, NULL, uri, pbc, TRUE, riid, &binding);
    if(FAILED(hres))
        return hres;

    if(binding->hres != S_OK) {
        hres = SUCCEEDED(binding->hres) ? S_OK : binding->hres;
    }else if(binding->bindf & BINDF_ASYNCHRONOUS) {
        hres = MK_S_ASYNCHRONOUS;
    }else {
        *ppv = binding->obj;
        IUnknown_AddRef(binding->obj);
        hres = S_OK;
    }

    IBinding_Release(BINDING(binding));

    return hres;
}
