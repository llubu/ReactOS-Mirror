#include <rpc.h>
#include <rpcndr.h>

#ifndef _WTYPES_H
#define _WTYPES_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IID_NULL GUID_NULL
#define CLSID_NULL GUID_NULL
#define CBPCLIPDATA(d) ((d).cbSize-sizeof((d).ulClipFmt))
#define DECIMAL_NEG ((BYTE)0x80)
#define DECIMAL_SETZERO(d) {DEC_LO64(&d)=DEC_HI32(&d)=DEC_SIGNSCALE(&d)=0;}
#define ROTFLAGS_REGISTRATIONKEEPSALIVE	0x01
#define ROTFLAGS_ALLOWANYCLIENT		0x02

#ifndef __BLOB_T_DEFINED /* also in winsock2.h */
#define __BLOB_T_DEFINED
typedef struct _BLOB {
	ULONG	cbSize;
	BYTE	*pBlobData;
} BLOB,*PBLOB,*LPBLOB;
#endif
typedef enum tagDVASPECT {
	DVASPECT_CONTENT=1,
	DVASPECT_THUMBNAIL=2,
	DVASPECT_ICON=4,
	DVASPECT_DOCPRINT=8
} DVASPECT;
typedef enum tagDVASPECT2 {
	DVASPECT_OPAQUE=16,
	DVASPECT_TRANSPARENT=32
} DVASPECT2;
typedef enum tagSTATFLAG {
	STATFLAG_DEFAULT=0,
	STATFLAG_NONAME=1
} STATFLAG;
typedef enum tagMEMCTX {
	MEMCTX_LOCAL=0,
	MEMCTX_TASK,
	MEMCTX_SHARED,
	MEMCTX_MACSYSTEM,
	MEMCTX_UNKNOWN=-1,
	MEMCTX_SAME=-2
} MEMCTX;
typedef enum tagMSHCTX {
	MSHCTX_LOCAL=0,
	MSHCTX_NOSHAREDMEM,
	MSHCTX_DIFFERENTMACHINE,
	MSHCTX_INPROC,
	MSHCTX_CROSSCTX
} MSHCTX;
typedef enum tagCLSCTX {
	CLSCTX_INPROC_SERVER=1,CLSCTX_INPROC_HANDLER=2,CLSCTX_LOCAL_SERVER=4,
	CLSCTX_INPROC_SERVER16=8,CLSCTX_REMOTE_SERVER=16
} CLSCTX;
typedef enum tagMSHLFLAGS {
	MSHLFLAGS_NORMAL,MSHLFLAGS_TABLESTRONG,MSHLFLAGS_TABLEWEAK,MSHLFLAGS_NOPING
} MSHLFLAGS;
typedef struct _FLAGGED_WORD_BLOB {
	unsigned long fFlags;
	unsigned long clSize;
	unsigned short asData[1];
}FLAGGED_WORD_BLOB;

#ifndef OLE2ANSI
typedef WCHAR OLECHAR;
typedef LPWSTR LPOLESTR;
typedef LPCWSTR LPCOLESTR;
#define OLESTR(s) L##s
#else
typedef char OLECHAR;
typedef LPSTR LPOLESTR;
typedef LPCSTR LPCOLESTR;
#define OLESTR(s) s
#endif
typedef unsigned short VARTYPE;
typedef short VARIANT_BOOL;
typedef VARIANT_BOOL _VARIANT_BOOL;
#define VARIANT_TRUE ((VARIANT_BOOL)0xffff)
#define VARIANT_FALSE ((VARIANT_BOOL)0)
typedef OLECHAR *BSTR;
typedef FLAGGED_WORD_BLOB *wireBSTR;
typedef BSTR *LPBSTR;
typedef LONG SCODE;
typedef void *HCONTEXT;
typedef union tagCY {
	_ANONYMOUS_STRUCT struct {
		unsigned long Lo;
		long Hi;
	}_STRUCT_NAME(s);
	LONGLONG int64;
} CY;
typedef double DATE;
typedef struct  tagBSTRBLOB {
	ULONG cbSize;
	PBYTE pData;
}BSTRBLOB;
typedef struct tagBSTRBLOB *LPBSTRBLOB;
typedef struct tagCLIPDATA {
	ULONG cbSize;
	long ulClipFmt;
	PBYTE pClipData;
}CLIPDATA;
typedef enum tagSTGC {
	STGC_DEFAULT,STGC_OVERWRITE,STGC_ONLYIFCURRENT,
	STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE
}STGC;
typedef enum tagSTGMOVE {
	STGMOVE_MOVE,STGMOVE_COPY,STGMOVE_SHALLOWCOPY
}STGMOVE;
enum VARENUM {
	VT_EMPTY,VT_NULL,VT_I2,VT_I4,VT_R4,VT_R8,VT_CY,VT_DATE,VT_BSTR,VT_DISPATCH,
	VT_ERROR,VT_BOOL,VT_VARIANT,VT_UNKNOWN,VT_DECIMAL,VT_I1=16,VT_UI1,VT_UI2,VT_UI4,VT_I8,
	VT_UI8,VT_INT,VT_UINT,VT_VOID,VT_HRESULT,VT_PTR,VT_SAFEARRAY,VT_CARRAY,VT_USERDEFINED,
	VT_LPSTR,VT_LPWSTR,VT_RECORD=36,VT_INT_PTR=37,VT_UINT_PTR=38,VT_FILETIME=64,VT_BLOB,VT_STREAM,VT_STORAGE,VT_STREAMED_OBJECT,
	VT_STORED_OBJECT,VT_BLOB_OBJECT,VT_CF,VT_CLSID,VT_BSTR_BLOB=0xfff,VT_VECTOR=0x1000,
	VT_ARRAY=0x2000,VT_BYREF=0x4000,VT_RESERVED=0x8000,VT_ILLEGAL= 0xffff,VT_ILLEGALMASKED=0xfff,
	VT_TYPEMASK=0xfff
};

typedef struct _BYTE_SIZEDARR {
	unsigned long clSize;
	byte *pData;
}BYTE_SIZEDARR;
typedef struct _SHORT_SIZEDARR {
	unsigned long clSize;
	unsigned short *pData;
}WORD_SIZEDARR;
typedef struct _LONG_SIZEDARR {
	unsigned long clSize;
	unsigned long *pData;
}DWORD_SIZEDARR;
typedef struct _HYPER_SIZEDARR {
	unsigned long clSize;
	hyper *pData;
}HYPER_SIZEDARR;
typedef double DOUBLE;
typedef struct tagDEC {
	USHORT wReserved;
	union {
		struct {
			BYTE scale;
			BYTE sign;
		};
		USHORT signscale;
	};
	ULONG Hi32;
	union {
		struct {
			ULONG Lo32;
			ULONG Mid32;
		};
		ULONGLONG Lo64;
	};
} DECIMAL;
typedef void *HMETAFILEPICT;
typedef struct tagCSPLATFORM {
    DWORD dwPlatformId;
    DWORD dwVersionHi;
    DWORD dwVersionLo;
    DWORD dwProcessorArch;
} CSPLATFORM;
typedef struct tagQUERYCONTEXT {
    DWORD dwContext;
    CSPLATFORM Platform;
    LCID Locale;
    DWORD dwVersionHi;
    DWORD dwVersionLo;
} QUERYCONTEXT;
typedef struct {
    DWORD tyspec;
    union {
        CLSID clsid;
        LPOLESTR pFileExt;
        LPOLESTR pMimeType;
        LPOLESTR pProgId;
        LPOLESTR pFileName;
        struct {
            LPOLESTR pPackageName;
            GUID PolicyId;
        } ByName;
        struct {
            GUID ObjectId;
            GUID PolicyId;
        } ByObjectId;
    } tagged_union;
} uCLSSPEC;

#define WDT_INPROC_CALL (0x48746457)

#define WDT_REMOTE_CALL (0x52746457)

#define WDT_INPROC64_CALL (0x50746457)

typedef struct _userCLIPFORMAT {
    long fContext;
    union {
        DWORD dwValue;
        LPWSTR pwszName;
    } u;
} userCLIPFORMAT;
typedef userCLIPFORMAT *wireCLIPFORMAT;
typedef WORD CLIPFORMAT;

typedef struct _RemotableHandle {
    long fContext;
    union {
        long hInproc;
        long hRemote;
    } u;
} RemotableHandle;
typedef RemotableHandle *wireHACCEL;
typedef RemotableHandle *wireHBRUSH;
typedef RemotableHandle *wireHDC;
typedef RemotableHandle *wireHFONT;
typedef RemotableHandle *wireHICON;
typedef RemotableHandle *wireHMENU;
typedef RemotableHandle *wireHWND;

typedef struct _BYTE_BLOB {
    unsigned long clSize;
    byte abData[1];
} BYTE_BLOB;
typedef BYTE_BLOB *UP_BYTE_BLOB;
typedef struct _FLAGGED_BYTE_BLOB {
    unsigned long fFlags;
    unsigned long clSize;
    byte abData[1];
} FLAGGED_BYTE_BLOB;
typedef FLAGGED_BYTE_BLOB *UP_FLAGGED_BYTE_BLOB;

typedef struct _userHENHMETAFILE {
    long fContext;
    union {
        long hInproc;
        BYTE_BLOB *hRemote;
        long hGlobal;
    } u;
} userHENHMETAFILE;
typedef userHENHMETAFILE *wireHENHMETAFILE;
typedef struct tagRemHMETAFILEPICT {
    long mm;
    long xExt;
    long yExt;
    unsigned long cbData;
    byte data[1];
} RemHMETAFILEPICT;
typedef struct _userHMETAFILE {
    long fContext;
    union {
        long hInproc;
        BYTE_BLOB *hRemote;
        long hGlobal;
    } u;
} userHMETAFILE;
typedef userHMETAFILE *wireHMETAFILE;
typedef struct _remoteMETAFILEPICT {
    long mm;
    long xExt;
    long yExt;
    userHMETAFILE *hMF;
} remoteMETAFILEPICT;
typedef struct _userHMETAFILEPICT {
    long fContext;
    union {
        long hInproc;
        remoteMETAFILEPICT *hRemote;
        long hGlobal;
    } u;
} userHMETAFILEPICT;
typedef userHMETAFILEPICT *wireHMETAFILEPICT;
typedef struct _userBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    ULONG cbSize;
    byte pBuffer[1];
} userBITMAP;
typedef struct _userHBITMAP {
    long fContext;
    union {
        long hInproc;
        userBITMAP *hRemote;
        long hGlobal;
    } u;
} userHBITMAP;
typedef userHBITMAP *wireHBITMAP;
typedef struct tagrpcLOGPALETTE {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[1];
} rpcLOGPALETTE;
typedef struct _userHPALETTE {
    long fContext;
    union {
        long hInproc;
        rpcLOGPALETTE *hRemote;
        long hGlobal;
    } u;
} userHPALETTE;
typedef userHPALETTE *wireHPALETTE;
typedef struct _userHGLOBAL {
    long fContext;
    union {
        long hInproc;
        FLAGGED_BYTE_BLOB *hRemote;
        long hGlobal;
    } u;
} userHGLOBAL;
typedef userHGLOBAL *wireHGLOBAL;

#ifdef __cplusplus
}
#endif
#endif
