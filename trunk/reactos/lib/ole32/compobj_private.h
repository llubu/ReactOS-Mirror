/*
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 Justin Bradford
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Marcus Meissner
 * Copyright 2003 Ove K�ven, TransGaming Technologies
 * Copyright 2004 Mike Hearn, CodeWeavers Inc
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

#ifndef __WINE_OLE_COMPOBJ_H
#define __WINE_OLE_COMPOBJ_H

/* All private prototype functions used by OLE will be added to this header file */

#include <stdarg.h>

#include "wine/list.h"

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "dcom.h"
#include "winreg.h"
#include "winternl.h"

struct apartment;


/* exported interface */
typedef struct tagXIF {
  struct tagXIF *next;
  LPVOID iface;            /* interface pointer */
  IID iid;                 /* interface ID */
  IPID ipid;               /* exported interface ID */
  LPRPCSTUBBUFFER stub;    /* interface stub */
  DWORD refs;              /* external reference count */
  HRESULT hres;            /* result of stub creation attempt */
} XIF;

/* exported object */
typedef struct tagXOBJECT {
  IRpcStubBufferVtbl *lpVtbl;
  struct apartment *parent;
  struct tagXOBJECT *next;
  LPUNKNOWN obj;           /* object identity (IUnknown) */
  OID oid;                 /* object ID */
  DWORD ifc;               /* interface ID counter */
  XIF *ifaces;             /* exported interfaces */
  DWORD refs;              /* external reference count */
} XOBJECT;

/* imported interface proxy */
struct ifproxy
{
  struct list entry;
  LPVOID iface;            /* interface pointer */
  IID iid;                 /* interface ID */
  IPID ipid;               /* imported interface ID */
  LPRPCPROXYBUFFER proxy;  /* interface proxy */
  DWORD refs;              /* imported (public) references */
};

/* imported object / proxy manager */
struct proxy_manager
{
  const IInternalUnknownVtbl *lpVtbl;
  struct apartment *parent;
  struct list entry;
  LPRPCCHANNELBUFFER chan; /* channel to object */
  OXID oxid;               /* object exported ID */
  OID oid;                 /* object ID */
  struct list interfaces;  /* imported interfaces */
  DWORD refs;              /* proxy reference count */
  CRITICAL_SECTION cs;     /* thread safety for this object and children */
};

/* this needs to become a COM object that implements IRemUnknown */
struct apartment
{
  struct list entry;       

  DWORD refs;              /* refcount of the apartment */
  DWORD model;             /* threading model */
  DWORD tid;               /* thread id */
  HANDLE thread;           /* thread handle */
  OXID oxid;               /* object exporter ID */
  OID oidc;                /* object ID counter, starts at 1, zero is invalid OID */
  HWND win;                /* message window */
  CRITICAL_SECTION cs;     /* thread safety */
  LPMESSAGEFILTER filter;  /* message filter */
  XOBJECT *objs;           /* exported objects */
  struct list proxies;     /* imported objects */
  DWORD listenertid;       /* id of apartment_listener_thread */
  struct list stubmgrs;    /* stub managers for exported objects */
};

typedef struct apartment APARTMENT;

extern void* StdGlobalInterfaceTable_Construct(void);
extern void  StdGlobalInterfaceTable_Destroy(void* self);
extern HRESULT StdGlobalInterfaceTable_GetFactory(LPVOID *ppv);

extern HRESULT WINE_StringFromCLSID(const CLSID *id,LPSTR idstr);
extern HRESULT create_marshalled_proxy(REFCLSID rclsid, REFIID iid, LPVOID *ppv);

extern void* StdGlobalInterfaceTableInstance;

/* Standard Marshalling definitions */
typedef struct _wine_marshal_id {
    OXID    oxid;       /* id of apartment */
    OID     oid;        /* id of stub manager */
    IPID    ipid;       /* id of interface pointer */
} wine_marshal_id;

inline static BOOL
MARSHAL_Compare_Mids(wine_marshal_id *mid1,wine_marshal_id *mid2) {
    return
	(mid1->oxid == mid2->oxid)	&&
	(mid1->oid == mid2->oid)	&&
	IsEqualGUID(&(mid1->ipid),&(mid2->ipid))
    ;
}

HRESULT MARSHAL_Disconnect_Proxies(APARTMENT *apt);
HRESULT MARSHAL_GetStandardMarshalCF(LPVOID *ppv);

/* Thread-safety Annotation Legend:
 *
 * RO   - The value is read only. It never changes after creation, so no
 *        locking is required.
 * LOCK - The value is written to only using Interlocked* functions.
 * CS   - The value is read or written to with a critical section held.
 *        The identifier following "CS" is the specific critical section that
 *        must be used.
 */

/* an interface stub */
struct ifstub   
{
    struct list       entry;      /* entry in stub_manager->ifstubs list (CS stub_manager->lock) */
    IRpcStubBuffer   *stubbuffer; /* RO */
    IID               iid;        /* RO */
    IPID              ipid;       /* RO */
    IUnknown         *iface;      /* RO */
    BOOL              table;      /* CS stub_manager->lock */
};


/* stub managers hold refs on the object and each interface stub */
struct stub_manager
{
    struct list       entry;      /* entry in apartment stubmgr list (CS apt->cs) */
    struct list       ifstubs;    /* list of active ifstubs for the object (CS lock) */
    CRITICAL_SECTION  lock;
    APARTMENT        *apt;        /* owning apt (RO) */

    ULONG             extrefs;    /* number of 'external' references (LOCK) */
    ULONG             refs;       /* internal reference count (CS apt->cs) */
    OID               oid;        /* apartment-scoped unique identifier (RO) */
    IUnknown         *object;     /* the object we are managing the stub for (RO) */
    ULONG             next_ipid;  /* currently unused (LOCK) */
};

ULONG stub_manager_int_addref(struct stub_manager *This);
ULONG stub_manager_int_release(struct stub_manager *This);
struct stub_manager *new_stub_manager(APARTMENT *apt, IUnknown *object);
ULONG stub_manager_ext_addref(struct stub_manager *m, ULONG refs);
ULONG stub_manager_ext_release(struct stub_manager *m, ULONG refs);
IRpcStubBuffer *stub_manager_ipid_to_stubbuffer(struct stub_manager *m, const IPID *iid);
struct ifstub *stub_manager_new_ifstub(struct stub_manager *m, IRpcStubBuffer *sb, IUnknown *iptr, REFIID iid, BOOL tablemarshal);
struct stub_manager *get_stub_manager(OXID oxid, OID oid);
struct stub_manager *get_stub_manager_from_object(OXID oxid, void *object);

IRpcStubBuffer *mid_to_stubbuffer(wine_marshal_id *mid);

void start_apartment_listener_thread(void);

extern HRESULT PIPE_GetNewPipeBuf(wine_marshal_id *mid, IRpcChannelBuffer **pipebuf);
void RPC_StartLocalServer(REFCLSID clsid, IStream *stream);

/* This function initialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void);

/* This function uninitialize the Running Object Table */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize(void);

/* This function decomposes a String path to a String Table containing all the elements ("\" or "subDirectory" or "Directory" or "FileName") of the path */
int WINAPI FileMonikerImpl_DecomposePath(LPCOLESTR str, LPOLESTR** stringTable);

HRESULT WINAPI __CLSIDFromStringA(LPCSTR idstr, CLSID *id);

/* compobj.c */
APARTMENT *COM_CreateApartment(DWORD model);
APARTMENT *COM_ApartmentFromOXID(OXID oxid, BOOL ref);
DWORD COM_ApartmentAddRef(struct apartment *apt);
DWORD COM_ApartmentRelease(struct apartment *apt);

/* this is what is stored in TEB->ReservedForOle */
struct oletls
{
    struct apartment *apt;
    IErrorInfo       *errorinfo;   /* see errorinfo.c */
    IUnknown         *state;       /* see CoSetState */
    DWORD            inits;        /* number of times CoInitializeEx called */
};

/*
 * Per-thread values are stored in the TEB on offset 0xF80,
 * see http://www.microsoft.com/msj/1099/bugslayer/bugslayer1099.htm
 */

/* will create if necessary */
static inline struct oletls *COM_CurrentInfo(void)
{
    if (!NtCurrentTeb()->ReservedForOle)     
        NtCurrentTeb()->ReservedForOle = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct oletls));
    
    return NtCurrentTeb()->ReservedForOle;
}

static inline APARTMENT* COM_CurrentApt(void)
{  
    return COM_CurrentInfo()->apt;
}

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

#endif /* __WINE_OLE_COMPOBJ_H */
