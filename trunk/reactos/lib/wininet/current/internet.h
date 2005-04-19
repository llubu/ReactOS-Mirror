/*
 * Wininet
 *
 * Copyright 1999 Corel Corporation
 *
 * Ulrich Czekalla
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

#ifndef _WINE_INTERNET_H_
#define _WINE_INTERNET_H_

#include "wine/unicode.h"

#include <time.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <sys/types.h>
# include <netinet/in.h>
#endif
#ifdef HAVE_OPENSSL_SSL_H
#define DSA __ssl_DSA  /* avoid conflict with commctrl.h */
#undef FAR
# include <openssl/ssl.h>
#undef FAR
#define FAR do_not_use_this_in_wine
#undef DSA
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#if defined(__MINGW32__) || defined (_MSC_VER)
#include "winsock2.h"
#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif
#else
#define closesocket close
#endif /* __MINGW32__ */

/* used for netconnection.c stuff */
typedef struct
{
    BOOL useSSL;
    int socketFD;
#ifdef HAVE_OPENSSL_SSL_H
    SSL *ssl_s;
    int ssl_sock;
#endif
} WININET_NETCONNECTION;

inline static LPSTR WININET_strdup( LPCSTR str )
{
    LPSTR ret = HeapAlloc( GetProcessHeap(), 0, strlen(str) + 1 );
    if (ret) strcpy( ret, str );
    return ret;
}

inline static LPWSTR WININET_strdupW( LPCWSTR str )
{
    LPWSTR ret = HeapAlloc( GetProcessHeap(), 0, (strlenW(str) + 1)*sizeof(WCHAR) );
    if (ret) strcpyW( ret, str );
    return ret;
}

inline static LPWSTR WININET_strdup_AtoW( LPCSTR str )
{
    int len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (ret)
        MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len);
    return ret;
}

inline static LPSTR WININET_strdup_WtoA( LPCWSTR str )
{
    int len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    LPSTR ret = HeapAlloc( GetProcessHeap(), 0, len );
    if (ret)
        WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL);
    return ret;
}

inline static void WININET_find_data_WtoA(LPWIN32_FIND_DATAW dataW, LPWIN32_FIND_DATAA dataA)
{
    dataA->dwFileAttributes = dataW->dwFileAttributes;
    dataA->ftCreationTime   = dataW->ftCreationTime;
    dataA->ftLastAccessTime = dataW->ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW->ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW->nFileSizeHigh;
    dataA->nFileSizeLow     = dataW->nFileSizeLow;
    dataA->dwReserved0      = dataW->dwReserved0;
    dataA->dwReserved1      = dataW->dwReserved1;
    WideCharToMultiByte(CP_ACP, 0, dataW->cFileName, -1, 
        dataA->cFileName, sizeof(dataA->cFileName),
        NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, dataW->cAlternateFileName, -1, 
        dataA->cAlternateFileName, sizeof(dataA->cAlternateFileName),
        NULL, NULL);
}

typedef enum
{
    WH_HINIT = INTERNET_HANDLE_TYPE_INTERNET,
    WH_HFTPSESSION = INTERNET_HANDLE_TYPE_CONNECT_FTP,
    WH_HGOPHERSESSION = INTERNET_HANDLE_TYPE_CONNECT_GOPHER,
    WH_HHTTPSESSION = INTERNET_HANDLE_TYPE_CONNECT_HTTP,
    WH_HFILE = INTERNET_HANDLE_TYPE_FTP_FILE,
    WH_HFINDNEXT = INTERNET_HANDLE_TYPE_FTP_FIND,
    WH_HHTTPREQ = INTERNET_HANDLE_TYPE_HTTP_REQUEST,
} WH_TYPE;

#define INET_OPENURL 0x0001
#define INET_CALLBACKW 0x0002

struct _WININETHANDLEHEADER;
typedef struct _WININETHANDLEHEADER WININETHANDLEHEADER, *LPWININETHANDLEHEADER;

typedef void (*WININET_object_destructor)( LPWININETHANDLEHEADER );

struct _WININETHANDLEHEADER
{
    WH_TYPE htype;
    DWORD  dwFlags;
    DWORD  dwContext;
    DWORD  dwError;
    DWORD  dwInternalFlags;
    DWORD  dwRefCount;
    WININET_object_destructor destroy;
    INTERNET_STATUS_CALLBACK lpfnStatusCB;
    struct _WININETHANDLEHEADER *lpwhparent;
};


typedef struct
{
    WININETHANDLEHEADER hdr;
    LPWSTR  lpszAgent;
    LPWSTR  lpszProxy;
    LPWSTR  lpszProxyBypass;
    LPWSTR  lpszProxyUsername;
    LPWSTR  lpszProxyPassword;
    DWORD   dwAccessType;
} WININETAPPINFOW, *LPWININETAPPINFOW;


typedef struct
{
    WININETHANDLEHEADER hdr;
    LPWSTR  lpszServerName;
    LPWSTR  lpszUserName;
    INTERNET_PORT nServerPort;
    struct sockaddr_in socketAddress;
    struct hostent *phostent;
} WININETHTTPSESSIONW, *LPWININETHTTPSESSIONW;

#define HDR_ISREQUEST		0x0001
#define HDR_COMMADELIMITED	0x0002
#define HDR_SEMIDELIMITED	0x0004

typedef struct
{
    LPWSTR lpszField;
    LPWSTR lpszValue;
    WORD wFlags;
    WORD wCount;
} HTTPHEADERW, *LPHTTPHEADERW;


typedef struct
{
    WININETHANDLEHEADER hdr;
    LPWSTR lpszPath;
    LPWSTR lpszVerb;
    LPWSTR lpszRawHeaders;
    WININET_NETCONNECTION netConnection;
    HTTPHEADERW StdHeaders[HTTP_QUERY_MAX+1];
    HTTPHEADERW *pCustHeaders;
    DWORD nCustHeaders;
} WININETHTTPREQW, *LPWININETHTTPREQW;


typedef struct
{
    WININETHANDLEHEADER hdr;
    BOOL session_deleted;
    int nDataSocket;
} WININETFILE, *LPWININETFILE;


typedef struct
{
    WININETHANDLEHEADER hdr;
    int sndSocket;
    int lstnSocket;
    int pasvSocket; /* data socket connected by us in case of passive FTP */
    LPWININETFILE download_in_progress;
    struct sockaddr_in socketAddress;
    struct sockaddr_in lstnSocketAddress;
    struct hostent *phostent;
    LPWSTR  lpszPassword;
    LPWSTR  lpszUserName;
} WININETFTPSESSIONW, *LPWININETFTPSESSIONW;


typedef struct
{
    BOOL bIsDirectory;
    LPWSTR lpszName;
    DWORD nSize;
    struct tm tmLastModified;
    unsigned short permissions;
} FILEPROPERTIESW, *LPFILEPROPERTIESW;


typedef struct
{
    WININETHANDLEHEADER hdr;
    DWORD index;
    DWORD size;
    LPFILEPROPERTIESW lpafp;
} WININETFINDNEXTW, *LPWININETFINDNEXTW;

typedef enum
{
    FTPPUTFILEW,
    FTPSETCURRENTDIRECTORYW,
    FTPCREATEDIRECTORYW,
    FTPFINDFIRSTFILEW,
    FTPGETCURRENTDIRECTORYW,
    FTPOPENFILEW,
    FTPGETFILEW,
    FTPDELETEFILEW,
    FTPREMOVEDIRECTORYW,
    FTPRENAMEFILEW,
    INTERNETFINDNEXTW,
    HTTPSENDREQUESTW,
    HTTPOPENREQUESTW,
    SENDCALLBACK,
    INTERNETOPENURLW,
} ASYNC_FUNC;

struct WORKREQ_FTPPUTFILEW
{
    LPWSTR lpszLocalFile;
    LPWSTR lpszNewRemoteFile;
    DWORD  dwFlags;
    DWORD  dwContext;
};

struct WORKREQ_FTPSETCURRENTDIRECTORYW
{
    LPWSTR lpszDirectory;
};

struct WORKREQ_FTPCREATEDIRECTORYW
{
    LPWSTR lpszDirectory;
};

struct WORKREQ_FTPFINDFIRSTFILEW
{
    LPWSTR lpszSearchFile;
    LPWIN32_FIND_DATAW lpFindFileData;
    DWORD  dwFlags;
    DWORD  dwContext;
};

struct WORKREQ_FTPGETCURRENTDIRECTORYW
{
    LPWSTR lpszDirectory;
    DWORD *lpdwDirectory;
};

struct WORKREQ_FTPOPENFILEW
{
    LPWSTR lpszFilename;
    DWORD  dwAccess;
    DWORD  dwFlags;
    DWORD  dwContext;
};

struct WORKREQ_FTPGETFILEW
{
    LPWSTR lpszRemoteFile;
    LPWSTR lpszNewFile;
    BOOL   fFailIfExists;
    DWORD  dwLocalFlagsAttribute;
    DWORD  dwFlags;
    DWORD  dwContext;
};

struct WORKREQ_FTPDELETEFILEW
{
    LPWSTR lpszFilename;
};

struct WORKREQ_FTPREMOVEDIRECTORYW
{
    LPWSTR lpszDirectory;
};

struct WORKREQ_FTPRENAMEFILEW
{
    LPWSTR lpszSrcFile;
    LPWSTR lpszDestFile;
};

struct WORKREQ_INTERNETFINDNEXTW
{
    LPWIN32_FIND_DATAW lpFindFileData;
};

struct WORKREQ_HTTPOPENREQUESTW
{
    LPWSTR lpszVerb;
    LPWSTR lpszObjectName;
    LPWSTR lpszVersion;
    LPWSTR lpszReferrer;
    LPCWSTR *lpszAcceptTypes;
    DWORD  dwFlags;
    DWORD  dwContext;
};

struct WORKREQ_HTTPSENDREQUESTW
{
    LPWSTR lpszHeader;
    DWORD  dwHeaderLength;
    LPVOID lpOptional;
    DWORD  dwOptionalLength;
};

struct WORKREQ_SENDCALLBACK
{
    DWORD     dwContext;
    DWORD     dwInternetStatus;
    LPVOID    lpvStatusInfo;
    DWORD     dwStatusInfoLength;
};

struct WORKREQ_INTERNETOPENURLW
{
    HINTERNET hInternet;
    LPWSTR     lpszUrl;
    LPWSTR     lpszHeaders;
    DWORD     dwHeadersLength;
    DWORD     dwFlags;
    DWORD     dwContext;
};

typedef struct WORKREQ
{
    ASYNC_FUNC asyncall;
    WININETHANDLEHEADER *hdr;

    union {
        struct WORKREQ_FTPPUTFILEW              FtpPutFileW;
        struct WORKREQ_FTPSETCURRENTDIRECTORYW  FtpSetCurrentDirectoryW;
        struct WORKREQ_FTPCREATEDIRECTORYW      FtpCreateDirectoryW;
        struct WORKREQ_FTPFINDFIRSTFILEW        FtpFindFirstFileW;
        struct WORKREQ_FTPGETCURRENTDIRECTORYW  FtpGetCurrentDirectoryW;
        struct WORKREQ_FTPOPENFILEW             FtpOpenFileW;
        struct WORKREQ_FTPGETFILEW              FtpGetFileW;
        struct WORKREQ_FTPDELETEFILEW           FtpDeleteFileW;
        struct WORKREQ_FTPREMOVEDIRECTORYW      FtpRemoveDirectoryW;
        struct WORKREQ_FTPRENAMEFILEW           FtpRenameFileW;
        struct WORKREQ_INTERNETFINDNEXTW        InternetFindNextW;
        struct WORKREQ_HTTPOPENREQUESTW         HttpOpenRequestW;
        struct WORKREQ_HTTPSENDREQUESTW         HttpSendRequestW;
        struct WORKREQ_SENDCALLBACK             SendCallback;
	struct WORKREQ_INTERNETOPENURLW         InternetOpenUrlW;
    } u;

    struct WORKREQ *next;
    struct WORKREQ *prev;

} WORKREQUEST, *LPWORKREQUEST;

HINTERNET WININET_AllocHandle( LPWININETHANDLEHEADER info );
LPWININETHANDLEHEADER WININET_GetObject( HINTERNET hinternet );
LPWININETHANDLEHEADER WININET_AddRef( LPWININETHANDLEHEADER info );
BOOL WININET_Release( LPWININETHANDLEHEADER info );
BOOL WININET_FreeHandle( HINTERNET hinternet );
HINTERNET WININET_FindHandle( LPWININETHANDLEHEADER info );

time_t ConvertTimeString(LPCWSTR asctime);

HINTERNET FTP_Connect(LPWININETAPPINFOW hIC, LPCWSTR lpszServerName,
	INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
	LPCWSTR lpszPassword, DWORD dwFlags, DWORD dwContext,
	DWORD dwInternalFlags);

HINTERNET HTTP_Connect(LPWININETAPPINFOW hIC, LPCWSTR lpszServerName,
	INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
	LPCWSTR lpszPassword, DWORD dwFlags, DWORD dwContext,
	DWORD dwInternalFlags);

BOOL GetAddress(LPCWSTR lpszServerName, INTERNET_PORT nServerPort,
	struct hostent **phe, struct sockaddr_in *psa);

void INTERNET_SetLastError(DWORD dwError);
DWORD INTERNET_GetLastError(void);
BOOL INTERNET_AsyncCall(LPWORKREQUEST lpWorkRequest);
LPSTR INTERNET_GetResponseBuffer(void);
LPSTR INTERNET_GetNextLine(INT nSocket, LPDWORD dwLen);

BOOLAPI FTP_FtpPutFileW(LPWININETFTPSESSIONW lpwfs, LPCWSTR lpszLocalFile,
    LPCWSTR lpszNewRemoteFile, DWORD dwFlags, DWORD dwContext);
BOOLAPI FTP_FtpSetCurrentDirectoryW(LPWININETFTPSESSIONW lpwfs, LPCWSTR lpszDirectory);
BOOLAPI FTP_FtpCreateDirectoryW(LPWININETFTPSESSIONW lpwfs, LPCWSTR lpszDirectory);
INTERNETAPI HINTERNET WINAPI FTP_FtpFindFirstFileW(LPWININETFTPSESSIONW lpwfs,
    LPCWSTR lpszSearchFile, LPWIN32_FIND_DATAW lpFindFileData, DWORD dwFlags, DWORD dwContext);
BOOLAPI FTP_FtpGetCurrentDirectoryW(LPWININETFTPSESSIONW lpwfs, LPWSTR lpszCurrentDirectory,
	LPDWORD lpdwCurrentDirectory);
BOOL FTP_ConvertFileProp(LPFILEPROPERTIESW lpafp, LPWIN32_FIND_DATAW lpFindFileData);
BOOL FTP_FtpRenameFileW(LPWININETFTPSESSIONW lpwfs, LPCWSTR lpszSrc, LPCWSTR lpszDest);
BOOL FTP_FtpRemoveDirectoryW(LPWININETFTPSESSIONW lpwfs, LPCWSTR lpszDirectory);
BOOL FTP_FtpDeleteFileW(LPWININETFTPSESSIONW lpwfs, LPCWSTR lpszFileName);
HINTERNET FTP_FtpOpenFileW(LPWININETFTPSESSIONW lpwfs, LPCWSTR lpszFileName,
	DWORD fdwAccess, DWORD dwFlags, DWORD dwContext);
BOOLAPI FTP_FtpGetFileW(LPWININETFTPSESSIONW lpwfs, LPCWSTR lpszRemoteFile, LPCWSTR lpszNewFile,
	BOOL fFailIfExists, DWORD dwLocalFlagsAttribute, DWORD dwInternetFlags,
	DWORD dwContext);

BOOLAPI HTTP_HttpSendRequestW(LPWININETHTTPREQW lpwhr, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength);
INTERNETAPI HINTERNET WINAPI HTTP_HttpOpenRequestW(LPWININETHTTPSESSIONW lpwhs,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD dwContext);

VOID SendAsyncCallback(LPWININETHANDLEHEADER hdr, DWORD dwContext,
                       DWORD dwInternetStatus, LPVOID lpvStatusInfo,
                       DWORD dwStatusInfoLength);

VOID SendSyncCallback(LPWININETHANDLEHEADER hdr, DWORD dwContext,
                      DWORD dwInternetStatus, LPVOID lpvStatusInfo,
                      DWORD dwStatusInfoLength);

BOOL HTTP_InsertProxyAuthorization( LPWININETHTTPREQW lpwhr,
                       LPCWSTR username, LPCWSTR password );

BOOL NETCON_connected(WININET_NETCONNECTION *connection);
void NETCON_init(WININET_NETCONNECTION *connnection, BOOL useSSL);
BOOL NETCON_create(WININET_NETCONNECTION *connection, int domain,
	      int type, int protocol);
BOOL NETCON_close(WININET_NETCONNECTION *connection);
BOOL NETCON_connect(WININET_NETCONNECTION *connection, const struct sockaddr *serv_addr,
		    unsigned int addrlen);
BOOL NETCON_send(WININET_NETCONNECTION *connection, const void *msg, size_t len, int flags,
		int *sent /* out */);
BOOL NETCON_recv(WININET_NETCONNECTION *connection, void *buf, size_t len, int flags,
		int *recvd /* out */);
BOOL NETCON_getNextLine(WININET_NETCONNECTION *connection, LPSTR lpszBuffer, LPDWORD dwBuffer);

#define MAX_REPLY_LEN	 	0x5B4

/* Used for debugging - maybe need to be shared in the Wine debugging code ? */
typedef struct
{
    DWORD val;
    const char* name;
} wininet_flag_info;

extern void dump_INTERNET_FLAGS(DWORD dwFlags) ;

#endif /* _WINE_INTERNET_H_ */
