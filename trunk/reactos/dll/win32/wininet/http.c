/*
 * Wininet - Http Implementation
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 * Copyright 2002 TransGaming Technologies Inc.
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
 * Copyright 2006 Robert Shearman for CodeWeavers
 *
 * Ulrich Czekalla
 * David Hammerton
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
#include "wine/port.h"

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#endif

#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <time.h>
#include <assert.h>
#ifdef HAVE_ZLIB
#  include <zlib.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STRFCNS
#define NO_SHLWAPI_GDI
#include "shlwapi.h"
#include "sspi.h"
#include "wincrypt.h"

#include "internet.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/unicode.h"

#include "inet_ntop.c"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

static const WCHAR g_szHttp1_0[] = {'H','T','T','P','/','1','.','0',0};
static const WCHAR g_szHttp1_1[] = {'H','T','T','P','/','1','.','1',0};
static const WCHAR szOK[] = {'O','K',0};
static const WCHAR szDefaultHeader[] = {'H','T','T','P','/','1','.','0',' ','2','0','0',' ','O','K',0};
static const WCHAR hostW[] = { 'H','o','s','t',0 };
static const WCHAR szAuthorization[] = { 'A','u','t','h','o','r','i','z','a','t','i','o','n',0 };
static const WCHAR szProxy_Authorization[] = { 'P','r','o','x','y','-','A','u','t','h','o','r','i','z','a','t','i','o','n',0 };
static const WCHAR szStatus[] = { 'S','t','a','t','u','s',0 };
static const WCHAR szKeepAlive[] = {'K','e','e','p','-','A','l','i','v','e',0};
static const WCHAR szGET[] = { 'G','E','T', 0 };
static const WCHAR szHEAD[] = { 'H','E','A','D', 0 };
static const WCHAR szCrLf[] = {'\r','\n', 0};

static const WCHAR szAccept[] = { 'A','c','c','e','p','t',0 };
static const WCHAR szAccept_Charset[] = { 'A','c','c','e','p','t','-','C','h','a','r','s','e','t', 0 };
static const WCHAR szAccept_Encoding[] = { 'A','c','c','e','p','t','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szAccept_Language[] = { 'A','c','c','e','p','t','-','L','a','n','g','u','a','g','e',0 };
static const WCHAR szAccept_Ranges[] = { 'A','c','c','e','p','t','-','R','a','n','g','e','s',0 };
static const WCHAR szAge[] = { 'A','g','e',0 };
static const WCHAR szAllow[] = { 'A','l','l','o','w',0 };
static const WCHAR szCache_Control[] = { 'C','a','c','h','e','-','C','o','n','t','r','o','l',0 };
static const WCHAR szConnection[] = { 'C','o','n','n','e','c','t','i','o','n',0 };
static const WCHAR szContent_Base[] = { 'C','o','n','t','e','n','t','-','B','a','s','e',0 };
static const WCHAR szContent_Encoding[] = { 'C','o','n','t','e','n','t','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szContent_ID[] = { 'C','o','n','t','e','n','t','-','I','D',0 };
static const WCHAR szContent_Language[] = { 'C','o','n','t','e','n','t','-','L','a','n','g','u','a','g','e',0 };
static const WCHAR szContent_Length[] = { 'C','o','n','t','e','n','t','-','L','e','n','g','t','h',0 };
static const WCHAR szContent_Location[] = { 'C','o','n','t','e','n','t','-','L','o','c','a','t','i','o','n',0 };
static const WCHAR szContent_MD5[] = { 'C','o','n','t','e','n','t','-','M','D','5',0 };
static const WCHAR szContent_Range[] = { 'C','o','n','t','e','n','t','-','R','a','n','g','e',0 };
static const WCHAR szContent_Transfer_Encoding[] = { 'C','o','n','t','e','n','t','-','T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szContent_Type[] = { 'C','o','n','t','e','n','t','-','T','y','p','e',0 };
static const WCHAR szCookie[] = { 'C','o','o','k','i','e',0 };
static const WCHAR szDate[] = { 'D','a','t','e',0 };
static const WCHAR szFrom[] = { 'F','r','o','m',0 };
static const WCHAR szETag[] = { 'E','T','a','g',0 };
static const WCHAR szExpect[] = { 'E','x','p','e','c','t',0 };
static const WCHAR szExpires[] = { 'E','x','p','i','r','e','s',0 };
static const WCHAR szIf_Match[] = { 'I','f','-','M','a','t','c','h',0 };
static const WCHAR szIf_Modified_Since[] = { 'I','f','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szIf_None_Match[] = { 'I','f','-','N','o','n','e','-','M','a','t','c','h',0 };
static const WCHAR szIf_Range[] = { 'I','f','-','R','a','n','g','e',0 };
static const WCHAR szIf_Unmodified_Since[] = { 'I','f','-','U','n','m','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szLast_Modified[] = { 'L','a','s','t','-','M','o','d','i','f','i','e','d',0 };
static const WCHAR szLocation[] = { 'L','o','c','a','t','i','o','n',0 };
static const WCHAR szMax_Forwards[] = { 'M','a','x','-','F','o','r','w','a','r','d','s',0 };
static const WCHAR szMime_Version[] = { 'M','i','m','e','-','V','e','r','s','i','o','n',0 };
static const WCHAR szPragma[] = { 'P','r','a','g','m','a',0 };
static const WCHAR szProxy_Authenticate[] = { 'P','r','o','x','y','-','A','u','t','h','e','n','t','i','c','a','t','e',0 };
static const WCHAR szProxy_Connection[] = { 'P','r','o','x','y','-','C','o','n','n','e','c','t','i','o','n',0 };
static const WCHAR szPublic[] = { 'P','u','b','l','i','c',0 };
static const WCHAR szRange[] = { 'R','a','n','g','e',0 };
static const WCHAR szReferer[] = { 'R','e','f','e','r','e','r',0 };
static const WCHAR szRetry_After[] = { 'R','e','t','r','y','-','A','f','t','e','r',0 };
static const WCHAR szServer[] = { 'S','e','r','v','e','r',0 };
static const WCHAR szSet_Cookie[] = { 'S','e','t','-','C','o','o','k','i','e',0 };
static const WCHAR szTransfer_Encoding[] = { 'T','r','a','n','s','f','e','r','-','E','n','c','o','d','i','n','g',0 };
static const WCHAR szUnless_Modified_Since[] = { 'U','n','l','e','s','s','-','M','o','d','i','f','i','e','d','-','S','i','n','c','e',0 };
static const WCHAR szUpgrade[] = { 'U','p','g','r','a','d','e',0 };
static const WCHAR szURI[] = { 'U','R','I',0 };
static const WCHAR szUser_Agent[] = { 'U','s','e','r','-','A','g','e','n','t',0 };
static const WCHAR szVary[] = { 'V','a','r','y',0 };
static const WCHAR szVia[] = { 'V','i','a',0 };
static const WCHAR szWarning[] = { 'W','a','r','n','i','n','g',0 };
static const WCHAR szWWW_Authenticate[] = { 'W','W','W','-','A','u','t','h','e','n','t','i','c','a','t','e',0 };

#define MAXHOSTNAME 100
#define MAX_FIELD_VALUE_LEN 256
#define MAX_FIELD_LEN 256

#define HTTP_REFERER    szReferer
#define HTTP_ACCEPT     szAccept
#define HTTP_USERAGENT  szUser_Agent

#define HTTP_ADDHDR_FLAG_ADD				0x20000000
#define HTTP_ADDHDR_FLAG_ADD_IF_NEW			0x10000000
#define HTTP_ADDHDR_FLAG_COALESCE			0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA		0x40000000
#define HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON	0x01000000
#define HTTP_ADDHDR_FLAG_REPLACE			0x80000000
#define HTTP_ADDHDR_FLAG_REQ				0x02000000

#define ARRAYSIZE(array) (sizeof(array)/sizeof((array)[0]))

struct HttpAuthInfo
{
    LPWSTR scheme;
    CredHandle cred;
    CtxtHandle ctx;
    TimeStamp exp;
    ULONG attr;
    ULONG max_token;
    void *auth_data;
    unsigned int auth_data_len;
    BOOL finished; /* finished authenticating */
};


struct gzip_stream_t {
#ifdef HAVE_ZLIB
    z_stream zstream;
#endif
    BYTE buf[8192];
    DWORD buf_size;
    DWORD buf_pos;
    BOOL end_of_data;
};

typedef struct _authorizationData
{
    struct list entry;

    LPWSTR lpszwHost;
    LPWSTR lpszwRealm;
    LPSTR  lpszAuthorization;
    UINT   AuthorizationLen;
} authorizationData;

static struct list basicAuthorizationCache = LIST_INIT(basicAuthorizationCache);

static CRITICAL_SECTION authcache_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &authcache_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": authcache_cs") }
};
static CRITICAL_SECTION authcache_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static DWORD HTTP_OpenConnection(http_request_t *req);
static BOOL HTTP_GetResponseHeaders(http_request_t *req, BOOL clear);
static DWORD HTTP_ProcessHeader(http_request_t *req, LPCWSTR field, LPCWSTR value, DWORD dwModifier);
static LPWSTR * HTTP_InterpretHttpHeader(LPCWSTR buffer);
static DWORD HTTP_InsertCustomHeader(http_request_t *req, LPHTTPHEADERW lpHdr);
static INT HTTP_GetCustomHeaderIndex(http_request_t *req, LPCWSTR lpszField, INT index, BOOL Request);
static BOOL HTTP_DeleteCustomHeader(http_request_t *req, DWORD index);
static LPWSTR HTTP_build_req( LPCWSTR *list, int len );
static DWORD HTTP_HttpQueryInfoW(http_request_t*, DWORD, LPVOID, LPDWORD, LPDWORD);
static LPWSTR HTTP_GetRedirectURL(http_request_t *req, LPCWSTR lpszUrl);
static UINT HTTP_DecodeBase64(LPCWSTR base64, LPSTR bin);
static BOOL HTTP_VerifyValidHeader(http_request_t *req, LPCWSTR field);
static void HTTP_DrainContent(http_request_t *req);
static BOOL HTTP_FinishedReading(http_request_t *req);

static LPHTTPHEADERW HTTP_GetHeader(http_request_t *req, LPCWSTR head)
{
    int HeaderIndex = 0;
    HeaderIndex = HTTP_GetCustomHeaderIndex(req, head, 0, TRUE);
    if (HeaderIndex == -1)
        return NULL;
    else
        return &req->pCustHeaders[HeaderIndex];
}

#ifdef HAVE_ZLIB

static voidpf wininet_zalloc(voidpf opaque, uInt items, uInt size)
{
    return HeapAlloc(GetProcessHeap(), 0, items*size);
}

static void wininet_zfree(voidpf opaque, voidpf address)
{
    HeapFree(GetProcessHeap(), 0, address);
}

static void init_gzip_stream(http_request_t *req)
{
    gzip_stream_t *gzip_stream;
    int index, zres;

    gzip_stream = HeapAlloc(GetProcessHeap(), 0, sizeof(gzip_stream_t));
    gzip_stream->zstream.zalloc = wininet_zalloc;
    gzip_stream->zstream.zfree = wininet_zfree;
    gzip_stream->zstream.opaque = NULL;
    gzip_stream->zstream.next_in = NULL;
    gzip_stream->zstream.avail_in = 0;
    gzip_stream->zstream.next_out = NULL;
    gzip_stream->zstream.avail_out = 0;
    gzip_stream->buf_pos = 0;
    gzip_stream->buf_size = 0;
    gzip_stream->end_of_data = FALSE;

    zres = inflateInit2(&gzip_stream->zstream, 0x1f);
    if(zres != Z_OK) {
        ERR("inflateInit failed: %d\n", zres);
        HeapFree(GetProcessHeap(), 0, gzip_stream);
        return;
    }

    req->gzip_stream = gzip_stream;

    index = HTTP_GetCustomHeaderIndex(req, szContent_Length, 0, FALSE);
    if(index != -1)
        HTTP_DeleteCustomHeader(req, index);
}

#else

static void init_gzip_stream(http_request_t *req)
{
    ERR("gzip stream not supported, missing zlib.\n");
}

#endif

/* set the request content length based on the headers */
static DWORD set_content_length( http_request_t *lpwhr )
{
    static const WCHAR szChunked[] = {'c','h','u','n','k','e','d',0};
    WCHAR encoding[20];
    DWORD size;

    size = sizeof(lpwhr->dwContentLength);
    if (HTTP_HttpQueryInfoW(lpwhr, HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_CONTENT_LENGTH,
                            &lpwhr->dwContentLength, &size, NULL) != ERROR_SUCCESS)
        lpwhr->dwContentLength = ~0u;

    size = sizeof(encoding);
    if (HTTP_HttpQueryInfoW(lpwhr, HTTP_QUERY_TRANSFER_ENCODING, encoding, &size, NULL) == ERROR_SUCCESS &&
        !strcmpiW(encoding, szChunked))
    {
        lpwhr->dwContentLength = ~0u;
        lpwhr->read_chunked = TRUE;
    }

    if(lpwhr->decoding) {
        int encoding_idx;

        static const WCHAR gzipW[] = {'g','z','i','p',0};

        encoding_idx = HTTP_GetCustomHeaderIndex(lpwhr, szContent_Encoding, 0, FALSE);
        if(encoding_idx != -1 && !strcmpiW(lpwhr->pCustHeaders[encoding_idx].lpszValue, gzipW))
            init_gzip_stream(lpwhr);
    }

    return lpwhr->dwContentLength;
}

/***********************************************************************
 *           HTTP_Tokenize (internal)
 *
 *  Tokenize a string, allocating memory for the tokens.
 */
static LPWSTR * HTTP_Tokenize(LPCWSTR string, LPCWSTR token_string)
{
    LPWSTR * token_array;
    int tokens = 0;
    int i;
    LPCWSTR next_token;

    if (string)
    {
        /* empty string has no tokens */
        if (*string)
            tokens++;
        /* count tokens */
        for (i = 0; string[i]; i++)
        {
            if (!strncmpW(string+i, token_string, strlenW(token_string)))
            {
                DWORD j;
                tokens++;
                /* we want to skip over separators, but not the null terminator */
                for (j = 0; j < strlenW(token_string) - 1; j++)
                    if (!string[i+j])
                        break;
                i += j;
            }
        }
    }

    /* add 1 for terminating NULL */
    token_array = HeapAlloc(GetProcessHeap(), 0, (tokens+1) * sizeof(*token_array));
    token_array[tokens] = NULL;
    if (!tokens)
        return token_array;
    for (i = 0; i < tokens; i++)
    {
        int len;
        next_token = strstrW(string, token_string);
        if (!next_token) next_token = string+strlenW(string);
        len = next_token - string;
        token_array[i] = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
        memcpy(token_array[i], string, len*sizeof(WCHAR));
        token_array[i][len] = '\0';
        string = next_token+strlenW(token_string);
    }
    return token_array;
}

/***********************************************************************
 *           HTTP_FreeTokens (internal)
 *
 *  Frees memory returned from HTTP_Tokenize.
 */
static void HTTP_FreeTokens(LPWSTR * token_array)
{
    int i;
    for (i = 0; token_array[i]; i++)
        HeapFree(GetProcessHeap(), 0, token_array[i]);
    HeapFree(GetProcessHeap(), 0, token_array);
}

static void HTTP_FixURL(http_request_t *lpwhr)
{
    static const WCHAR szSlash[] = { '/',0 };
    static const WCHAR szHttp[] = { 'h','t','t','p',':','/','/', 0 };

    /* If we don't have a path we set it to root */
    if (NULL == lpwhr->lpszPath)
        lpwhr->lpszPath = heap_strdupW(szSlash);
    else /* remove \r and \n*/
    {
        int nLen = strlenW(lpwhr->lpszPath);
        while ((nLen >0 ) && ((lpwhr->lpszPath[nLen-1] == '\r')||(lpwhr->lpszPath[nLen-1] == '\n')))
        {
            nLen--;
            lpwhr->lpszPath[nLen]='\0';
        }
        /* Replace '\' with '/' */
        while (nLen>0) {
            nLen--;
            if (lpwhr->lpszPath[nLen] == '\\') lpwhr->lpszPath[nLen]='/';
        }
    }

    if(CSTR_EQUAL != CompareStringW( LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                       lpwhr->lpszPath, strlenW(lpwhr->lpszPath), szHttp, strlenW(szHttp) )
       && lpwhr->lpszPath[0] != '/') /* not an absolute path ?? --> fix it !! */
    {
        WCHAR *fixurl = HeapAlloc(GetProcessHeap(), 0, 
                             (strlenW(lpwhr->lpszPath) + 2)*sizeof(WCHAR));
        *fixurl = '/';
        strcpyW(fixurl + 1, lpwhr->lpszPath);
        HeapFree( GetProcessHeap(), 0, lpwhr->lpszPath );
        lpwhr->lpszPath = fixurl;
    }
}

static LPWSTR HTTP_BuildHeaderRequestString( http_request_t *lpwhr, LPCWSTR verb, LPCWSTR path, LPCWSTR version )
{
    LPWSTR requestString;
    DWORD len, n;
    LPCWSTR *req;
    UINT i;
    LPWSTR p;

    static const WCHAR szSpace[] = { ' ',0 };
    static const WCHAR szColon[] = { ':',' ',0 };
    static const WCHAR sztwocrlf[] = {'\r','\n','\r','\n', 0};

    /* allocate space for an array of all the string pointers to be added */
    len = (lpwhr->nCustHeaders)*4 + 10;
    req = HeapAlloc( GetProcessHeap(), 0, len*sizeof(LPCWSTR) );

    /* add the verb, path and HTTP version string */
    n = 0;
    req[n++] = verb;
    req[n++] = szSpace;
    req[n++] = path;
    req[n++] = szSpace;
    req[n++] = version;

    /* Append custom request headers */
    for (i = 0; i < lpwhr->nCustHeaders; i++)
    {
        if (lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST)
        {
            req[n++] = szCrLf;
            req[n++] = lpwhr->pCustHeaders[i].lpszField;
            req[n++] = szColon;
            req[n++] = lpwhr->pCustHeaders[i].lpszValue;

            TRACE("Adding custom header %s (%s)\n",
                   debugstr_w(lpwhr->pCustHeaders[i].lpszField),
                   debugstr_w(lpwhr->pCustHeaders[i].lpszValue));
        }
    }

    if( n >= len )
        ERR("oops. buffer overrun\n");

    req[n] = NULL;
    requestString = HTTP_build_req( req, 4 );
    HeapFree( GetProcessHeap(), 0, req );

    /*
     * Set (header) termination string for request
     * Make sure there's exactly two new lines at the end of the request
     */
    p = &requestString[strlenW(requestString)-1];
    while ( (*p == '\n') || (*p == '\r') )
       p--;
    strcpyW( p+1, sztwocrlf );
    
    return requestString;
}

static void HTTP_ProcessCookies( http_request_t *lpwhr )
{
    int HeaderIndex;
    int numCookies = 0;
    LPHTTPHEADERW setCookieHeader;

    while((HeaderIndex = HTTP_GetCustomHeaderIndex(lpwhr, szSet_Cookie, numCookies, FALSE)) != -1)
    {
        setCookieHeader = &lpwhr->pCustHeaders[HeaderIndex];

        if (!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_COOKIES) && setCookieHeader->lpszValue)
        {
            int len;
            static const WCHAR szFmt[] = { 'h','t','t','p',':','/','/','%','s','%','s',0};
            LPWSTR buf_url;
            LPHTTPHEADERW Host;

            Host = HTTP_GetHeader(lpwhr, hostW);
            len = lstrlenW(Host->lpszValue) + 9 + lstrlenW(lpwhr->lpszPath);
            buf_url = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
            sprintfW(buf_url, szFmt, Host->lpszValue, lpwhr->lpszPath);
            InternetSetCookieW(buf_url, NULL, setCookieHeader->lpszValue);

            HeapFree(GetProcessHeap(), 0, buf_url);
        }
        numCookies++;
    }
}

static void strip_spaces(LPWSTR start)
{
    LPWSTR str = start;
    LPWSTR end;

    while (*str == ' ' && *str != '\0')
        str++;

    if (str != start)
        memmove(start, str, sizeof(WCHAR) * (strlenW(str) + 1));

    end = start + strlenW(start) - 1;
    while (end >= start && *end == ' ')
    {
        *end = '\0';
        end--;
    }
}

static inline BOOL is_basic_auth_value( LPCWSTR pszAuthValue, LPWSTR *pszRealm )
{
    static const WCHAR szBasic[] = {'B','a','s','i','c'}; /* Note: not nul-terminated */
    static const WCHAR szRealm[] = {'r','e','a','l','m'}; /* Note: not nul-terminated */
    BOOL is_basic;
    is_basic = !strncmpiW(pszAuthValue, szBasic, ARRAYSIZE(szBasic)) &&
        ((pszAuthValue[ARRAYSIZE(szBasic)] == ' ') || !pszAuthValue[ARRAYSIZE(szBasic)]);
    if (is_basic && pszRealm)
    {
        LPCWSTR token;
        LPCWSTR ptr = &pszAuthValue[ARRAYSIZE(szBasic)];
        LPCWSTR realm;
        ptr++;
        *pszRealm=NULL;
        token = strchrW(ptr,'=');
        if (!token)
            return TRUE;
        realm = ptr;
        while (*realm == ' ' && *realm != '\0')
            realm++;
        if(!strncmpiW(realm, szRealm, ARRAYSIZE(szRealm)) &&
            (realm[ARRAYSIZE(szRealm)] == ' ' || realm[ARRAYSIZE(szRealm)] == '='))
        {
            token++;
            while (*token == ' ' && *token != '\0')
                token++;
            if (*token == '\0')
                return TRUE;
            *pszRealm = heap_strdupW(token);
            strip_spaces(*pszRealm);
        }
    }

    return is_basic;
}

static void destroy_authinfo( struct HttpAuthInfo *authinfo )
{
    if (!authinfo) return;

    if (SecIsValidHandle(&authinfo->ctx))
        DeleteSecurityContext(&authinfo->ctx);
    if (SecIsValidHandle(&authinfo->cred))
        FreeCredentialsHandle(&authinfo->cred);

    HeapFree(GetProcessHeap(), 0, authinfo->auth_data);
    HeapFree(GetProcessHeap(), 0, authinfo->scheme);
    HeapFree(GetProcessHeap(), 0, authinfo);
}

static UINT retrieve_cached_basic_authorization(LPWSTR host, LPWSTR realm, LPSTR *auth_data)
{
    authorizationData *ad;
    UINT rc = 0;

    TRACE("Looking for authorization for %s:%s\n",debugstr_w(host),debugstr_w(realm));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH_ENTRY(ad, &basicAuthorizationCache, authorizationData, entry)
    {
        if (!strcmpiW(host,ad->lpszwHost) && !strcmpW(realm,ad->lpszwRealm))
        {
            TRACE("Authorization found in cache\n");
            *auth_data = HeapAlloc(GetProcessHeap(),0,ad->AuthorizationLen);
            memcpy(*auth_data,ad->lpszAuthorization,ad->AuthorizationLen);
            rc = ad->AuthorizationLen;
            break;
        }
    }
    LeaveCriticalSection(&authcache_cs);
    return rc;
}

static void cache_basic_authorization(LPWSTR host, LPWSTR realm, LPSTR auth_data, UINT auth_data_len)
{
    struct list *cursor;
    authorizationData* ad = NULL;

    TRACE("caching authorization for %s:%s = %s\n",debugstr_w(host),debugstr_w(realm),debugstr_an(auth_data,auth_data_len));

    EnterCriticalSection(&authcache_cs);
    LIST_FOR_EACH(cursor, &basicAuthorizationCache)
    {
        authorizationData *check = LIST_ENTRY(cursor,authorizationData,entry);
        if (!strcmpiW(host,check->lpszwHost) && !strcmpW(realm,check->lpszwRealm))
        {
            ad = check;
            break;
        }
    }

    if (ad)
    {
        TRACE("Found match in cache, replacing\n");
        HeapFree(GetProcessHeap(),0,ad->lpszAuthorization);
        ad->lpszAuthorization = HeapAlloc(GetProcessHeap(),0,auth_data_len);
        memcpy(ad->lpszAuthorization, auth_data, auth_data_len);
        ad->AuthorizationLen = auth_data_len;
    }
    else
    {
        ad = HeapAlloc(GetProcessHeap(),0,sizeof(authorizationData));
        ad->lpszwHost = heap_strdupW(host);
        ad->lpszwRealm = heap_strdupW(realm);
        ad->lpszAuthorization = HeapAlloc(GetProcessHeap(),0,auth_data_len);
        memcpy(ad->lpszAuthorization, auth_data, auth_data_len);
        ad->AuthorizationLen = auth_data_len;
        list_add_head(&basicAuthorizationCache,&ad->entry);
        TRACE("authorization cached\n");
    }
    LeaveCriticalSection(&authcache_cs);
}

static BOOL HTTP_DoAuthorization( http_request_t *lpwhr, LPCWSTR pszAuthValue,
                                  struct HttpAuthInfo **ppAuthInfo,
                                  LPWSTR domain_and_username, LPWSTR password,
                                  LPWSTR host )
{
    SECURITY_STATUS sec_status;
    struct HttpAuthInfo *pAuthInfo = *ppAuthInfo;
    BOOL first = FALSE;
    LPWSTR szRealm = NULL;

    TRACE("%s\n", debugstr_w(pszAuthValue));

    if (!pAuthInfo)
    {
        TimeStamp exp;

        first = TRUE;
        pAuthInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(*pAuthInfo));
        if (!pAuthInfo)
            return FALSE;

        SecInvalidateHandle(&pAuthInfo->cred);
        SecInvalidateHandle(&pAuthInfo->ctx);
        memset(&pAuthInfo->exp, 0, sizeof(pAuthInfo->exp));
        pAuthInfo->attr = 0;
        pAuthInfo->auth_data = NULL;
        pAuthInfo->auth_data_len = 0;
        pAuthInfo->finished = FALSE;

        if (is_basic_auth_value(pszAuthValue,NULL))
        {
            static const WCHAR szBasic[] = {'B','a','s','i','c',0};
            pAuthInfo->scheme = heap_strdupW(szBasic);
            if (!pAuthInfo->scheme)
            {
                HeapFree(GetProcessHeap(), 0, pAuthInfo);
                return FALSE;
            }
        }
        else
        {
            PVOID pAuthData;
            SEC_WINNT_AUTH_IDENTITY_W nt_auth_identity;

            pAuthInfo->scheme = heap_strdupW(pszAuthValue);
            if (!pAuthInfo->scheme)
            {
                HeapFree(GetProcessHeap(), 0, pAuthInfo);
                return FALSE;
            }

            if (domain_and_username)
            {
                WCHAR *user = strchrW(domain_and_username, '\\');
                WCHAR *domain = domain_and_username;

                /* FIXME: make sure scheme accepts SEC_WINNT_AUTH_IDENTITY before calling AcquireCredentialsHandle */

                pAuthData = &nt_auth_identity;

                if (user) user++;
                else
                {
                    user = domain_and_username;
                    domain = NULL;
                }

                nt_auth_identity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
                nt_auth_identity.User = user;
                nt_auth_identity.UserLength = strlenW(nt_auth_identity.User);
                nt_auth_identity.Domain = domain;
                nt_auth_identity.DomainLength = domain ? user - domain - 1 : 0;
                nt_auth_identity.Password = password;
                nt_auth_identity.PasswordLength = strlenW(nt_auth_identity.Password);
            }
            else
                /* use default credentials */
                pAuthData = NULL;

            sec_status = AcquireCredentialsHandleW(NULL, pAuthInfo->scheme,
                                                   SECPKG_CRED_OUTBOUND, NULL,
                                                   pAuthData, NULL,
                                                   NULL, &pAuthInfo->cred,
                                                   &exp);
            if (sec_status == SEC_E_OK)
            {
                PSecPkgInfoW sec_pkg_info;
                sec_status = QuerySecurityPackageInfoW(pAuthInfo->scheme, &sec_pkg_info);
                if (sec_status == SEC_E_OK)
                {
                    pAuthInfo->max_token = sec_pkg_info->cbMaxToken;
                    FreeContextBuffer(sec_pkg_info);
                }
            }
            if (sec_status != SEC_E_OK)
            {
                WARN("AcquireCredentialsHandleW for scheme %s failed with error 0x%08x\n",
                     debugstr_w(pAuthInfo->scheme), sec_status);
                HeapFree(GetProcessHeap(), 0, pAuthInfo->scheme);
                HeapFree(GetProcessHeap(), 0, pAuthInfo);
                return FALSE;
            }
        }
        *ppAuthInfo = pAuthInfo;
    }
    else if (pAuthInfo->finished)
        return FALSE;

    if ((strlenW(pszAuthValue) < strlenW(pAuthInfo->scheme)) ||
        strncmpiW(pszAuthValue, pAuthInfo->scheme, strlenW(pAuthInfo->scheme)))
    {
        ERR("authentication scheme changed from %s to %s\n",
            debugstr_w(pAuthInfo->scheme), debugstr_w(pszAuthValue));
        return FALSE;
    }

    if (is_basic_auth_value(pszAuthValue,&szRealm))
    {
        int userlen;
        int passlen;
        char *auth_data = NULL;
        UINT auth_data_len = 0;

        TRACE("basic authentication realm %s\n",debugstr_w(szRealm));

        if (!domain_and_username)
        {
            if (host && szRealm)
                auth_data_len = retrieve_cached_basic_authorization(host, szRealm,&auth_data);
            if (auth_data_len == 0)
            {
                HeapFree(GetProcessHeap(),0,szRealm);
                return FALSE;
            }
        }
        else
        {
            userlen = WideCharToMultiByte(CP_UTF8, 0, domain_and_username, lstrlenW(domain_and_username), NULL, 0, NULL, NULL);
            passlen = WideCharToMultiByte(CP_UTF8, 0, password, lstrlenW(password), NULL, 0, NULL, NULL);

            /* length includes a nul terminator, which will be re-used for the ':' */
            auth_data = HeapAlloc(GetProcessHeap(), 0, userlen + 1 + passlen);
            if (!auth_data)
            {
                HeapFree(GetProcessHeap(),0,szRealm);
                return FALSE;
            }

            WideCharToMultiByte(CP_UTF8, 0, domain_and_username, -1, auth_data, userlen, NULL, NULL);
            auth_data[userlen] = ':';
            WideCharToMultiByte(CP_UTF8, 0, password, -1, &auth_data[userlen+1], passlen, NULL, NULL);
            auth_data_len = userlen + 1 + passlen;
            if (host && szRealm)
                cache_basic_authorization(host, szRealm, auth_data, auth_data_len);
        }

        pAuthInfo->auth_data = auth_data;
        pAuthInfo->auth_data_len = auth_data_len;
        pAuthInfo->finished = TRUE;
        HeapFree(GetProcessHeap(),0,szRealm);

        return TRUE;
    }
    else
    {
        LPCWSTR pszAuthData;
        SecBufferDesc out_desc, in_desc;
        SecBuffer out, in;
        unsigned char *buffer;
        ULONG context_req = ISC_REQ_CONNECTION | ISC_REQ_USE_DCE_STYLE |
            ISC_REQ_MUTUAL_AUTH | ISC_REQ_DELEGATE;

        in.BufferType = SECBUFFER_TOKEN;
        in.cbBuffer = 0;
        in.pvBuffer = NULL;

        in_desc.ulVersion = 0;
        in_desc.cBuffers = 1;
        in_desc.pBuffers = &in;

        pszAuthData = pszAuthValue + strlenW(pAuthInfo->scheme);
        if (*pszAuthData == ' ')
        {
            pszAuthData++;
            in.cbBuffer = HTTP_DecodeBase64(pszAuthData, NULL);
            in.pvBuffer = HeapAlloc(GetProcessHeap(), 0, in.cbBuffer);
            HTTP_DecodeBase64(pszAuthData, in.pvBuffer);
        }

        buffer = HeapAlloc(GetProcessHeap(), 0, pAuthInfo->max_token);

        out.BufferType = SECBUFFER_TOKEN;
        out.cbBuffer = pAuthInfo->max_token;
        out.pvBuffer = buffer;

        out_desc.ulVersion = 0;
        out_desc.cBuffers = 1;
        out_desc.pBuffers = &out;

        sec_status = InitializeSecurityContextW(first ? &pAuthInfo->cred : NULL,
                                                first ? NULL : &pAuthInfo->ctx,
                                                first ? lpwhr->lpHttpSession->lpszServerName : NULL,
                                                context_req, 0, SECURITY_NETWORK_DREP,
                                                in.pvBuffer ? &in_desc : NULL,
                                                0, &pAuthInfo->ctx, &out_desc,
                                                &pAuthInfo->attr, &pAuthInfo->exp);
        if (sec_status == SEC_E_OK)
        {
            pAuthInfo->finished = TRUE;
            pAuthInfo->auth_data = out.pvBuffer;
            pAuthInfo->auth_data_len = out.cbBuffer;
            TRACE("sending last auth packet\n");
        }
        else if (sec_status == SEC_I_CONTINUE_NEEDED)
        {
            pAuthInfo->auth_data = out.pvBuffer;
            pAuthInfo->auth_data_len = out.cbBuffer;
            TRACE("sending next auth packet\n");
        }
        else
        {
            ERR("InitializeSecurityContextW returned error 0x%08x\n", sec_status);
            HeapFree(GetProcessHeap(), 0, out.pvBuffer);
            destroy_authinfo(pAuthInfo);
            *ppAuthInfo = NULL;
            return FALSE;
        }
    }

    return TRUE;
}

/***********************************************************************
 *           HTTP_HttpAddRequestHeadersW (internal)
 */
static DWORD HTTP_HttpAddRequestHeadersW(http_request_t *lpwhr,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    LPWSTR lpszStart;
    LPWSTR lpszEnd;
    LPWSTR buffer;
    DWORD len, res = ERROR_HTTP_INVALID_HEADER;

    TRACE("copying header: %s\n", debugstr_wn(lpszHeader, dwHeaderLength));

    if( dwHeaderLength == ~0U )
        len = strlenW(lpszHeader);
    else
        len = dwHeaderLength;
    buffer = HeapAlloc( GetProcessHeap(), 0, sizeof(WCHAR)*(len+1) );
    lstrcpynW( buffer, lpszHeader, len + 1);

    lpszStart = buffer;

    do
    {
        LPWSTR * pFieldAndValue;

        lpszEnd = lpszStart;

        while (*lpszEnd != '\0')
        {
            if (*lpszEnd == '\r' || *lpszEnd == '\n')
                 break;
            lpszEnd++;
        }

        if (*lpszStart == '\0')
	    break;

        if (*lpszEnd == '\r' || *lpszEnd == '\n')
        {
            *lpszEnd = '\0';
            lpszEnd++; /* Jump over newline */
        }
        TRACE("interpreting header %s\n", debugstr_w(lpszStart));
        if (*lpszStart == '\0')
        {
            /* Skip 0-length headers */
            lpszStart = lpszEnd;
            res = ERROR_SUCCESS;
            continue;
        }
        pFieldAndValue = HTTP_InterpretHttpHeader(lpszStart);
        if (pFieldAndValue)
        {
            res = HTTP_VerifyValidHeader(lpwhr, pFieldAndValue[0]);
            if (res == ERROR_SUCCESS)
                res = HTTP_ProcessHeader(lpwhr, pFieldAndValue[0],
                    pFieldAndValue[1], dwModifier | HTTP_ADDHDR_FLAG_REQ);
            HTTP_FreeTokens(pFieldAndValue);
        }

        lpszStart = lpszEnd;
    } while (res == ERROR_SUCCESS);

    HeapFree(GetProcessHeap(), 0, buffer);

    return res;
}

/***********************************************************************
 *           HttpAddRequestHeadersW (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * NOTE
 * On Windows if dwHeaderLength includes the trailing '\0', then
 * HttpAddRequestHeadersW() adds it too. However this results in an
 * invalid Http header which is rejected by some servers so we probably
 * don't need to match Windows on that point.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersW(HINTERNET hHttpRequest,
	LPCWSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    http_request_t *lpwhr;
    DWORD res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;

    TRACE("%p, %s, %i, %i\n", hHttpRequest, debugstr_wn(lpszHeader, dwHeaderLength), dwHeaderLength, dwModifier);

    if (!lpszHeader) 
      return TRUE;

    lpwhr = (http_request_t*) WININET_GetObject( hHttpRequest );
    if (lpwhr && lpwhr->hdr.htype == WH_HHTTPREQ)
        res = HTTP_HttpAddRequestHeadersW( lpwhr, lpszHeader, dwHeaderLength, dwModifier );
    if( lpwhr )
        WININET_Release( &lpwhr->hdr );

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpAddRequestHeadersA (WININET.@)
 *
 * Adds one or more HTTP header to the request handler
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpAddRequestHeadersA(HINTERNET hHttpRequest,
	LPCSTR lpszHeader, DWORD dwHeaderLength, DWORD dwModifier)
{
    DWORD len;
    LPWSTR hdr;
    BOOL r;

    TRACE("%p, %s, %i, %i\n", hHttpRequest, debugstr_an(lpszHeader, dwHeaderLength), dwHeaderLength, dwModifier);

    len = MultiByteToWideChar( CP_ACP, 0, lpszHeader, dwHeaderLength, NULL, 0 );
    hdr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, lpszHeader, dwHeaderLength, hdr, len );
    if( dwHeaderLength != ~0U )
        dwHeaderLength = len;

    r = HttpAddRequestHeadersW( hHttpRequest, hdr, dwHeaderLength, dwModifier );

    HeapFree( GetProcessHeap(), 0, hdr );

    return r;
}

/***********************************************************************
 *           HttpOpenRequestA (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestA(HINTERNET hHttpSession,
	LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
	LPCSTR lpszReferrer , LPCSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD_PTR dwContext)
{
    LPWSTR szVerb = NULL, szObjectName = NULL;
    LPWSTR szVersion = NULL, szReferrer = NULL, *szAcceptTypes = NULL;
    INT acceptTypesCount;
    HINTERNET rc = FALSE;
    LPCSTR *types;

    TRACE("(%p, %s, %s, %s, %s, %p, %08x, %08lx)\n", hHttpSession,
          debugstr_a(lpszVerb), debugstr_a(lpszObjectName),
          debugstr_a(lpszVersion), debugstr_a(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);

    if (lpszVerb)
    {
        szVerb = heap_strdupAtoW(lpszVerb);
        if ( !szVerb )
            goto end;
    }

    if (lpszObjectName)
    {
        szObjectName = heap_strdupAtoW(lpszObjectName);
        if ( !szObjectName )
            goto end;
    }

    if (lpszVersion)
    {
        szVersion = heap_strdupAtoW(lpszVersion);
        if ( !szVersion )
            goto end;
    }

    if (lpszReferrer)
    {
        szReferrer = heap_strdupAtoW(lpszReferrer);
        if ( !szReferrer )
            goto end;
    }

    if (lpszAcceptTypes)
    {
        acceptTypesCount = 0;
        types = lpszAcceptTypes;
        while (*types)
        {
            __TRY
            {
                /* find out how many there are */
                if (*types && **types)
                {
                    TRACE("accept type: %s\n", debugstr_a(*types));
                    acceptTypesCount++;
                }
            }
            __EXCEPT_PAGE_FAULT
            {
                WARN("invalid accept type pointer\n");
            }
            __ENDTRY;
            types++;
        }
        szAcceptTypes = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR *) * (acceptTypesCount+1));
        if (!szAcceptTypes) goto end;

        acceptTypesCount = 0;
        types = lpszAcceptTypes;
        while (*types)
        {
            __TRY
            {
                if (*types && **types)
                    szAcceptTypes[acceptTypesCount++] = heap_strdupAtoW(*types);
            }
            __EXCEPT_PAGE_FAULT
            {
                /* ignore invalid pointer */
            }
            __ENDTRY;
            types++;
        }
        szAcceptTypes[acceptTypesCount] = NULL;
    }

    rc = HttpOpenRequestW(hHttpSession, szVerb, szObjectName,
                          szVersion, szReferrer,
                          (LPCWSTR*)szAcceptTypes, dwFlags, dwContext);

end:
    if (szAcceptTypes)
    {
        acceptTypesCount = 0;
        while (szAcceptTypes[acceptTypesCount])
        {
            HeapFree(GetProcessHeap(), 0, szAcceptTypes[acceptTypesCount]);
            acceptTypesCount++;
        }
        HeapFree(GetProcessHeap(), 0, szAcceptTypes);
    }
    HeapFree(GetProcessHeap(), 0, szReferrer);
    HeapFree(GetProcessHeap(), 0, szVersion);
    HeapFree(GetProcessHeap(), 0, szObjectName);
    HeapFree(GetProcessHeap(), 0, szVerb);

    return rc;
}

/***********************************************************************
 *  HTTP_EncodeBase64
 */
static UINT HTTP_EncodeBase64( LPCSTR bin, unsigned int len, LPWSTR base64 )
{
    UINT n = 0, x;
    static const CHAR HTTP_Base64Enc[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while( len > 0 )
    {
        /* first 6 bits, all from bin[0] */
        base64[n++] = HTTP_Base64Enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if( len == 1 )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[1]&0xf0) >> 4 ) ];
        x = ( bin[1] & 0x0f ) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if( len == 2 )
        {
            base64[n++] = HTTP_Base64Enc[x];
            base64[n++] = '=';
            break;
        }
        base64[n++] = HTTP_Base64Enc[ x | ( (bin[2]&0xc0 ) >> 6 ) ];

        /* last 6 bits, all from bin [2] */
        base64[n++] = HTTP_Base64Enc[ bin[2] & 0x3f ];
        bin += 3;
        len -= 3;
    }
    base64[n] = 0;
    return n;
}

#define CH(x) (((x) >= 'A' && (x) <= 'Z') ? (x) - 'A' : \
               ((x) >= 'a' && (x) <= 'z') ? (x) - 'a' + 26 : \
               ((x) >= '0' && (x) <= '9') ? (x) - '0' + 52 : \
               ((x) == '+') ? 62 : ((x) == '/') ? 63 : -1)
static const signed char HTTP_Base64Dec[256] =
{
    CH( 0),CH( 1),CH( 2),CH( 3),CH( 4),CH( 5),CH( 6),CH( 7),CH( 8),CH( 9),
    CH(10),CH(11),CH(12),CH(13),CH(14),CH(15),CH(16),CH(17),CH(18),CH(19),
    CH(20),CH(21),CH(22),CH(23),CH(24),CH(25),CH(26),CH(27),CH(28),CH(29),
    CH(30),CH(31),CH(32),CH(33),CH(34),CH(35),CH(36),CH(37),CH(38),CH(39),
    CH(40),CH(41),CH(42),CH(43),CH(44),CH(45),CH(46),CH(47),CH(48),CH(49),
    CH(50),CH(51),CH(52),CH(53),CH(54),CH(55),CH(56),CH(57),CH(58),CH(59),
    CH(60),CH(61),CH(62),CH(63),CH(64),CH(65),CH(66),CH(67),CH(68),CH(69),
    CH(70),CH(71),CH(72),CH(73),CH(74),CH(75),CH(76),CH(77),CH(78),CH(79),
    CH(80),CH(81),CH(82),CH(83),CH(84),CH(85),CH(86),CH(87),CH(88),CH(89),
    CH(90),CH(91),CH(92),CH(93),CH(94),CH(95),CH(96),CH(97),CH(98),CH(99),
    CH(100),CH(101),CH(102),CH(103),CH(104),CH(105),CH(106),CH(107),CH(108),CH(109),
    CH(110),CH(111),CH(112),CH(113),CH(114),CH(115),CH(116),CH(117),CH(118),CH(119),
    CH(120),CH(121),CH(122),CH(123),CH(124),CH(125),CH(126),CH(127),CH(128),CH(129),
    CH(130),CH(131),CH(132),CH(133),CH(134),CH(135),CH(136),CH(137),CH(138),CH(139),
    CH(140),CH(141),CH(142),CH(143),CH(144),CH(145),CH(146),CH(147),CH(148),CH(149),
    CH(150),CH(151),CH(152),CH(153),CH(154),CH(155),CH(156),CH(157),CH(158),CH(159),
    CH(160),CH(161),CH(162),CH(163),CH(164),CH(165),CH(166),CH(167),CH(168),CH(169),
    CH(170),CH(171),CH(172),CH(173),CH(174),CH(175),CH(176),CH(177),CH(178),CH(179),
    CH(180),CH(181),CH(182),CH(183),CH(184),CH(185),CH(186),CH(187),CH(188),CH(189),
    CH(190),CH(191),CH(192),CH(193),CH(194),CH(195),CH(196),CH(197),CH(198),CH(199),
    CH(200),CH(201),CH(202),CH(203),CH(204),CH(205),CH(206),CH(207),CH(208),CH(209),
    CH(210),CH(211),CH(212),CH(213),CH(214),CH(215),CH(216),CH(217),CH(218),CH(219),
    CH(220),CH(221),CH(222),CH(223),CH(224),CH(225),CH(226),CH(227),CH(228),CH(229),
    CH(230),CH(231),CH(232),CH(233),CH(234),CH(235),CH(236),CH(237),CH(238),CH(239),
    CH(240),CH(241),CH(242),CH(243),CH(244),CH(245),CH(246),CH(247),CH(248), CH(249),
    CH(250),CH(251),CH(252),CH(253),CH(254),CH(255),
};
#undef CH

/***********************************************************************
 *  HTTP_DecodeBase64
 */
static UINT HTTP_DecodeBase64( LPCWSTR base64, LPSTR bin )
{
    unsigned int n = 0;

    while(*base64)
    {
        signed char in[4];

        if (base64[0] >= ARRAYSIZE(HTTP_Base64Dec) ||
            ((in[0] = HTTP_Base64Dec[base64[0]]) == -1) ||
            base64[1] >= ARRAYSIZE(HTTP_Base64Dec) ||
            ((in[1] = HTTP_Base64Dec[base64[1]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(base64));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (in[0] << 2 | in[1] >> 4);
        n++;

        if ((base64[2] == '=') && (base64[3] == '='))
            break;
        if (base64[2] > ARRAYSIZE(HTTP_Base64Dec) ||
            ((in[2] = HTTP_Base64Dec[base64[2]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(&base64[2]));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (in[1] << 4 | in[2] >> 2);
        n++;

        if (base64[3] == '=')
            break;
        if (base64[3] > ARRAYSIZE(HTTP_Base64Dec) ||
            ((in[3] = HTTP_Base64Dec[base64[3]]) == -1))
        {
            WARN("invalid base64: %s\n", debugstr_w(&base64[3]));
            return 0;
        }
        if (bin)
            bin[n] = (unsigned char) (((in[2] << 6) & 0xc0) | in[3]);
        n++;

        base64 += 4;
    }

    return n;
}

/***********************************************************************
 *  HTTP_InsertAuthorization
 *
 *   Insert or delete the authorization field in the request header.
 */
static BOOL HTTP_InsertAuthorization( http_request_t *lpwhr, struct HttpAuthInfo *pAuthInfo, LPCWSTR header )
{
    if (pAuthInfo)
    {
        static const WCHAR wszSpace[] = {' ',0};
        static const WCHAR wszBasic[] = {'B','a','s','i','c',0};
        unsigned int len;
        WCHAR *authorization = NULL;

        if (pAuthInfo->auth_data_len)
        {
            /* scheme + space + base64 encoded data (3/2/1 bytes data -> 4 bytes of characters) */
            len = strlenW(pAuthInfo->scheme)+1+((pAuthInfo->auth_data_len+2)*4)/3;
            authorization = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
            if (!authorization)
                return FALSE;

            strcpyW(authorization, pAuthInfo->scheme);
            strcatW(authorization, wszSpace);
            HTTP_EncodeBase64(pAuthInfo->auth_data,
                              pAuthInfo->auth_data_len,
                              authorization+strlenW(authorization));

            /* clear the data as it isn't valid now that it has been sent to the
             * server, unless it's Basic authentication which doesn't do
             * connection tracking */
            if (strcmpiW(pAuthInfo->scheme, wszBasic))
            {
                HeapFree(GetProcessHeap(), 0, pAuthInfo->auth_data);
                pAuthInfo->auth_data = NULL;
                pAuthInfo->auth_data_len = 0;
            }
        }

        TRACE("Inserting authorization: %s\n", debugstr_w(authorization));

        HTTP_ProcessHeader(lpwhr, header, authorization, HTTP_ADDHDR_FLAG_REQ | HTTP_ADDHDR_FLAG_REPLACE);

        HeapFree(GetProcessHeap(), 0, authorization);
    }
    return TRUE;
}

static WCHAR *HTTP_BuildProxyRequestUrl(http_request_t *req)
{
    WCHAR new_location[INTERNET_MAX_URL_LENGTH], *url;
    DWORD size;

    size = sizeof(new_location);
    if (HTTP_HttpQueryInfoW(req, HTTP_QUERY_LOCATION, new_location, &size, NULL) == ERROR_SUCCESS)
    {
        if (!(url = HeapAlloc( GetProcessHeap(), 0, size + sizeof(WCHAR) ))) return NULL;
        strcpyW( url, new_location );
    }
    else
    {
        static const WCHAR slash[] = { '/',0 };
        static const WCHAR format[] = { 'h','t','t','p',':','/','/','%','s',':','%','d',0 };
        static const WCHAR formatSSL[] = { 'h','t','t','p','s',':','/','/','%','s',':','%','d',0 };
        http_session_t *session = req->lpHttpSession;

        size = 16; /* "https://" + sizeof(port#) + ":/\0" */
        size += strlenW( session->lpszHostName ) + strlenW( req->lpszPath );

        if (!(url = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) ))) return NULL;

        if (req->hdr.dwFlags & INTERNET_FLAG_SECURE)
            sprintfW( url, formatSSL, session->lpszHostName, session->nHostPort );
        else
            sprintfW( url, format, session->lpszHostName, session->nHostPort );
        if (req->lpszPath[0] != '/') strcatW( url, slash );
        strcatW( url, req->lpszPath );
    }
    TRACE("url=%s\n", debugstr_w(url));
    return url;
}

/***********************************************************************
 *           HTTP_DealWithProxy
 */
static BOOL HTTP_DealWithProxy(appinfo_t *hIC, http_session_t *lpwhs, http_request_t *lpwhr)
{
    WCHAR buf[MAXHOSTNAME];
    WCHAR proxy[MAXHOSTNAME + 15]; /* 15 == "http://" + sizeof(port#) + ":/\0" */
    static WCHAR szNul[] = { 0 };
    URL_COMPONENTSW UrlComponents;
    static const WCHAR szHttp[] = { 'h','t','t','p',':','/','/',0 };
    static const WCHAR szFormat[] = { 'h','t','t','p',':','/','/','%','s',0 };

    memset( &UrlComponents, 0, sizeof UrlComponents );
    UrlComponents.dwStructSize = sizeof UrlComponents;
    UrlComponents.lpszHostName = buf;
    UrlComponents.dwHostNameLength = MAXHOSTNAME;

    if( CSTR_EQUAL != CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                 hIC->lpszProxy,strlenW(szHttp),szHttp,strlenW(szHttp)) )
        sprintfW(proxy, szFormat, hIC->lpszProxy);
    else
	strcpyW(proxy, hIC->lpszProxy);
    if( !InternetCrackUrlW(proxy, 0, 0, &UrlComponents) )
        return FALSE;
    if( UrlComponents.dwHostNameLength == 0 )
        return FALSE;

    if( !lpwhr->lpszPath )
        lpwhr->lpszPath = szNul;

    if(UrlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
        UrlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;

    HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
    lpwhs->lpszServerName = heap_strdupW(UrlComponents.lpszHostName);
    lpwhs->nServerPort = UrlComponents.nPort;

    TRACE("proxy server=%s port=%d\n", debugstr_w(lpwhs->lpszServerName), lpwhs->nServerPort);
    return TRUE;
}

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

static DWORD HTTP_ResolveName(http_request_t *lpwhr)
{
    char szaddr[INET6_ADDRSTRLEN];
    http_session_t *lpwhs = lpwhr->lpHttpSession;
    const void *addr;

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_RESOLVING_NAME,
                          lpwhs->lpszServerName,
                          (strlenW(lpwhs->lpszServerName)+1) * sizeof(WCHAR));

    lpwhs->sa_len = sizeof(lpwhs->socketAddress);
    if (!GetAddress(lpwhs->lpszServerName, lpwhs->nServerPort,
                    (struct sockaddr *)&lpwhs->socketAddress, &lpwhs->sa_len))
        return ERROR_INTERNET_NAME_NOT_RESOLVED;

    switch (lpwhs->socketAddress.ss_family)
    {
    case AF_INET:
        addr = &((struct sockaddr_in *)&lpwhs->socketAddress)->sin_addr;
        break;
    case AF_INET6:
        addr = &((struct sockaddr_in6 *)&lpwhs->socketAddress)->sin6_addr;
        break;
    default:
        WARN("unsupported family %d\n", lpwhs->socketAddress.ss_family);
        return ERROR_INTERNET_NAME_NOT_RESOLVED;
    }
    inet_ntop(lpwhs->socketAddress.ss_family, addr, szaddr, sizeof(szaddr));
    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_NAME_RESOLVED,
                          szaddr, strlen(szaddr)+1);

    TRACE("resolved %s to %s\n", debugstr_w(lpwhs->lpszServerName), szaddr);
    return ERROR_SUCCESS;
}


/***********************************************************************
 *           HTTPREQ_Destroy (internal)
 *
 * Deallocate request handle
 *
 */
static void HTTPREQ_Destroy(object_header_t *hdr)
{
    http_request_t *lpwhr = (http_request_t*) hdr;
    DWORD i;

    TRACE("\n");

    if(lpwhr->hCacheFile)
        CloseHandle(lpwhr->hCacheFile);

    HeapFree(GetProcessHeap(), 0, lpwhr->lpszCacheFile);

    DeleteCriticalSection( &lpwhr->read_section );
    WININET_Release(&lpwhr->lpHttpSession->hdr);

    destroy_authinfo(lpwhr->pAuthInfo);
    destroy_authinfo(lpwhr->pProxyAuthInfo);

    HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    HeapFree(GetProcessHeap(), 0, lpwhr->lpszVerb);
    HeapFree(GetProcessHeap(), 0, lpwhr->lpszRawHeaders);
    HeapFree(GetProcessHeap(), 0, lpwhr->lpszVersion);
    HeapFree(GetProcessHeap(), 0, lpwhr->lpszStatusText);

    for (i = 0; i < lpwhr->nCustHeaders; i++)
    {
        HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders[i].lpszField);
        HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders[i].lpszValue);
    }

#ifdef HAVE_ZLIB
    if(lpwhr->gzip_stream) {
        if(!lpwhr->gzip_stream->end_of_data)
            inflateEnd(&lpwhr->gzip_stream->zstream);
        HeapFree(GetProcessHeap(), 0, lpwhr->gzip_stream);
    }
#endif

    HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders);
    HeapFree(GetProcessHeap(), 0, lpwhr);
}

static void HTTPREQ_CloseConnection(object_header_t *hdr)
{
    http_request_t *lpwhr = (http_request_t*) hdr;

    TRACE("%p\n",lpwhr);

    if (!NETCON_connected(&lpwhr->netConnection))
        return;

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_CLOSING_CONNECTION, 0, 0);

    NETCON_close(&lpwhr->netConnection);

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_CONNECTION_CLOSED, 0, 0);
}

static BOOL HTTP_GetRequestURL(http_request_t *req, LPWSTR buf)
{
    LPHTTPHEADERW host_header;

    static const WCHAR formatW[] = {'h','t','t','p',':','/','/','%','s','%','s',0};

    host_header = HTTP_GetHeader(req, hostW);
    if(!host_header)
        return FALSE;

    sprintfW(buf, formatW, host_header->lpszValue, req->lpszPath); /* FIXME */
    return TRUE;
}

static BOOL HTTP_KeepAlive(http_request_t *lpwhr)
{
    WCHAR szVersion[10];
    WCHAR szConnectionResponse[20];
    DWORD dwBufferSize = sizeof(szVersion);
    BOOL keepalive = FALSE;

    /* as per RFC 2068, S8.1.2.1, if the client is HTTP/1.1 then assume that
     * the connection is keep-alive by default */
    if (HTTP_HttpQueryInfoW(lpwhr, HTTP_QUERY_VERSION, szVersion, &dwBufferSize, NULL) == ERROR_SUCCESS
        && !strcmpiW(szVersion, g_szHttp1_1))
    {
        keepalive = TRUE;
    }

    dwBufferSize = sizeof(szConnectionResponse);
    if (HTTP_HttpQueryInfoW(lpwhr, HTTP_QUERY_PROXY_CONNECTION, szConnectionResponse, &dwBufferSize, NULL) == ERROR_SUCCESS
        || HTTP_HttpQueryInfoW(lpwhr, HTTP_QUERY_CONNECTION, szConnectionResponse, &dwBufferSize, NULL) == ERROR_SUCCESS)
    {
        keepalive = !strcmpiW(szConnectionResponse, szKeepAlive);
    }

    return keepalive;
}

static DWORD HTTPREQ_QueryOption(object_header_t *hdr, DWORD option, void *buffer, DWORD *size, BOOL unicode)
{
    http_request_t *req = (http_request_t*)hdr;

    switch(option) {
    case INTERNET_OPTION_DIAGNOSTIC_SOCKET_INFO:
    {
        http_session_t *lpwhs = req->lpHttpSession;
        INTERNET_DIAGNOSTIC_SOCKET_INFO *info = buffer;

        FIXME("INTERNET_DIAGNOSTIC_SOCKET_INFO stub\n");

        if (*size < sizeof(INTERNET_DIAGNOSTIC_SOCKET_INFO))
            return ERROR_INSUFFICIENT_BUFFER;
        *size = sizeof(INTERNET_DIAGNOSTIC_SOCKET_INFO);
        /* FIXME: can't get a SOCKET from our connection since we don't use
         * winsock
         */
        info->Socket = 0;
        /* FIXME: get source port from req->netConnection */
        info->SourcePort = 0;
        info->DestPort = lpwhs->nHostPort;
        info->Flags = 0;
        if (HTTP_KeepAlive(req))
            info->Flags |= IDSI_FLAG_KEEP_ALIVE;
        if (lpwhs->lpAppInfo->lpszProxy && lpwhs->lpAppInfo->lpszProxy[0] != 0)
            info->Flags |= IDSI_FLAG_PROXY;
        if (req->netConnection.useSSL)
            info->Flags |= IDSI_FLAG_SECURE;

        return ERROR_SUCCESS;
    }

    case INTERNET_OPTION_SECURITY_FLAGS:
    {
        http_session_t *lpwhs;
        lpwhs = req->lpHttpSession;

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        if (lpwhs->hdr.dwFlags & INTERNET_FLAG_SECURE)
            *(DWORD*)buffer = SECURITY_FLAG_SECURE;
        else
            *(DWORD*)buffer = 0;
        FIXME("Semi-STUB INTERNET_OPTION_SECURITY_FLAGS: %x\n",*(DWORD*)buffer);
        return ERROR_SUCCESS;
    }

    case INTERNET_OPTION_HANDLE_TYPE:
        TRACE("INTERNET_OPTION_HANDLE_TYPE\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD*)buffer = INTERNET_HANDLE_TYPE_HTTP_REQUEST;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_URL: {
        WCHAR url[INTERNET_MAX_URL_LENGTH];
        HTTPHEADERW *host;
        DWORD len;
        WCHAR *pch;

        static const WCHAR httpW[] = {'h','t','t','p',':','/','/',0};

        TRACE("INTERNET_OPTION_URL\n");

        host = HTTP_GetHeader(req, hostW);
        strcpyW(url, httpW);
        strcatW(url, host->lpszValue);
        if (NULL != (pch = strchrW(url + strlenW(httpW), ':')))
            *pch = 0;
        strcatW(url, req->lpszPath);

        TRACE("INTERNET_OPTION_URL: %s\n",debugstr_w(url));

        if(unicode) {
            len = (strlenW(url)+1) * sizeof(WCHAR);
            if(*size < len)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = len;
            strcpyW(buffer, url);
            return ERROR_SUCCESS;
        }else {
            len = WideCharToMultiByte(CP_ACP, 0, url, -1, buffer, *size, NULL, NULL);
            if(len > *size)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = len;
            return ERROR_SUCCESS;
        }
    }

    case INTERNET_OPTION_CACHE_TIMESTAMPS: {
        INTERNET_CACHE_ENTRY_INFOW *info;
        INTERNET_CACHE_TIMESTAMPS *ts = buffer;
        WCHAR url[INTERNET_MAX_URL_LENGTH];
        DWORD nbytes, error;
        BOOL ret;

        TRACE("INTERNET_OPTION_CACHE_TIMESTAMPS\n");

        if (*size < sizeof(*ts))
        {
            *size = sizeof(*ts);
            return ERROR_INSUFFICIENT_BUFFER;
        }
        nbytes = 0;
        HTTP_GetRequestURL(req, url);
        ret = GetUrlCacheEntryInfoW(url, NULL, &nbytes);
        error = GetLastError();
        if (!ret && error == ERROR_INSUFFICIENT_BUFFER)
        {
            if (!(info = HeapAlloc(GetProcessHeap(), 0, nbytes)))
                return ERROR_OUTOFMEMORY;

            GetUrlCacheEntryInfoW(url, info, &nbytes);

            ts->ftExpires = info->ExpireTime;
            ts->ftLastModified = info->LastModifiedTime;

            HeapFree(GetProcessHeap(), 0, info);
            *size = sizeof(*ts);
            return ERROR_SUCCESS;
        }
        return error;
    }

    case INTERNET_OPTION_DATAFILE_NAME: {
        DWORD req_size;

        TRACE("INTERNET_OPTION_DATAFILE_NAME\n");

        if(!req->lpszCacheFile) {
            *size = 0;
            return ERROR_INTERNET_ITEM_NOT_FOUND;
        }

        if(unicode) {
            req_size = (lstrlenW(req->lpszCacheFile)+1) * sizeof(WCHAR);
            if(*size < req_size)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = req_size;
            memcpy(buffer, req->lpszCacheFile, *size);
            return ERROR_SUCCESS;
        }else {
            req_size = WideCharToMultiByte(CP_ACP, 0, req->lpszCacheFile, -1, NULL, 0, NULL, NULL);
            if (req_size > *size)
                return ERROR_INSUFFICIENT_BUFFER;

            *size = WideCharToMultiByte(CP_ACP, 0, req->lpszCacheFile,
                    -1, buffer, *size, NULL, NULL);
            return ERROR_SUCCESS;
        }
    }

    case INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT: {
        PCCERT_CONTEXT context;

        if(*size < sizeof(INTERNET_CERTIFICATE_INFOW)) {
            *size = sizeof(INTERNET_CERTIFICATE_INFOW);
            return ERROR_INSUFFICIENT_BUFFER;
        }

        context = (PCCERT_CONTEXT)NETCON_GetCert(&(req->netConnection));
        if(context) {
            INTERNET_CERTIFICATE_INFOW *info = (INTERNET_CERTIFICATE_INFOW*)buffer;
            DWORD len;

            memset(info, 0, sizeof(INTERNET_CERTIFICATE_INFOW));
            info->ftExpiry = context->pCertInfo->NotAfter;
            info->ftStart = context->pCertInfo->NotBefore;
            if(unicode) {
                len = CertNameToStrW(context->dwCertEncodingType,
                        &context->pCertInfo->Subject, CERT_SIMPLE_NAME_STR, NULL, 0);
                info->lpszSubjectInfo = LocalAlloc(0, len*sizeof(WCHAR));
                if(info->lpszSubjectInfo)
                    CertNameToStrW(context->dwCertEncodingType,
                             &context->pCertInfo->Subject, CERT_SIMPLE_NAME_STR,
                             info->lpszSubjectInfo, len);
                len = CertNameToStrW(context->dwCertEncodingType,
                         &context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR, NULL, 0);
                info->lpszIssuerInfo = LocalAlloc(0, len*sizeof(WCHAR));
                if (info->lpszIssuerInfo)
                    CertNameToStrW(context->dwCertEncodingType,
                             &context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR,
                             info->lpszIssuerInfo, len);
            }else {
                INTERNET_CERTIFICATE_INFOA *infoA = (INTERNET_CERTIFICATE_INFOA*)info;

                len = CertNameToStrA(context->dwCertEncodingType,
                         &context->pCertInfo->Subject, CERT_SIMPLE_NAME_STR, NULL, 0);
                infoA->lpszSubjectInfo = LocalAlloc(0, len);
                if(infoA->lpszSubjectInfo)
                    CertNameToStrA(context->dwCertEncodingType,
                             &context->pCertInfo->Subject, CERT_SIMPLE_NAME_STR,
                             infoA->lpszSubjectInfo, len);
                len = CertNameToStrA(context->dwCertEncodingType,
                         &context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR, NULL, 0);
                infoA->lpszIssuerInfo = LocalAlloc(0, len);
                if(infoA->lpszIssuerInfo)
                    CertNameToStrA(context->dwCertEncodingType,
                             &context->pCertInfo->Issuer, CERT_SIMPLE_NAME_STR,
                             infoA->lpszIssuerInfo, len);
            }

            /*
             * Contrary to MSDN, these do not appear to be set.
             * lpszProtocolName
             * lpszSignatureAlgName
             * lpszEncryptionAlgName
             * dwKeySize
             */
            CertFreeCertificateContext(context);
            return ERROR_SUCCESS;
        }
    }
    }

    return INET_QueryOption(option, buffer, size, unicode);
}

static DWORD HTTPREQ_SetOption(object_header_t *hdr, DWORD option, void *buffer, DWORD size)
{
    http_request_t *req = (http_request_t*)hdr;

    switch(option) {
    case INTERNET_OPTION_SEND_TIMEOUT:
    case INTERNET_OPTION_RECEIVE_TIMEOUT:
        TRACE("INTERNET_OPTION_SEND/RECEIVE_TIMEOUT\n");

        if (size != sizeof(DWORD))
            return ERROR_INVALID_PARAMETER;

        return NETCON_set_timeout(&req->netConnection, option == INTERNET_OPTION_SEND_TIMEOUT,
                    *(DWORD*)buffer);

    case INTERNET_OPTION_USERNAME:
        HeapFree(GetProcessHeap(), 0, req->lpHttpSession->lpszUserName);
        if (!(req->lpHttpSession->lpszUserName = heap_strdupW(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_PASSWORD:
        HeapFree(GetProcessHeap(), 0, req->lpHttpSession->lpszPassword);
        if (!(req->lpHttpSession->lpszPassword = heap_strdupW(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    case INTERNET_OPTION_HTTP_DECODING:
        if(size != sizeof(BOOL))
            return ERROR_INVALID_PARAMETER;
        req->decoding = *(BOOL*)buffer;
        return ERROR_SUCCESS;
    }

    return ERROR_INTERNET_INVALID_OPTION;
}

/* read some more data into the read buffer (the read section must be held) */
static DWORD read_more_data( http_request_t *req, int maxlen )
{
    DWORD res;
    int len;

    if (req->read_pos)
    {
        /* move existing data to the start of the buffer */
        if(req->read_size)
            memmove( req->read_buf, req->read_buf + req->read_pos, req->read_size );
        req->read_pos = 0;
    }

    if (maxlen == -1) maxlen = sizeof(req->read_buf);

    res = NETCON_recv( &req->netConnection, req->read_buf + req->read_size,
                       maxlen - req->read_size, 0, &len );
    if(res == ERROR_SUCCESS)
        req->read_size += len;

    return res;
}

/* remove some amount of data from the read buffer (the read section must be held) */
static void remove_data( http_request_t *req, int count )
{
    if (!(req->read_size -= count)) req->read_pos = 0;
    else req->read_pos += count;
}

static BOOL read_line( http_request_t *req, LPSTR buffer, DWORD *len )
{
    int count, bytes_read, pos = 0;
    DWORD res;

    EnterCriticalSection( &req->read_section );
    for (;;)
    {
        BYTE *eol = memchr( req->read_buf + req->read_pos, '\n', req->read_size );

        if (eol)
        {
            count = eol - (req->read_buf + req->read_pos);
            bytes_read = count + 1;
        }
        else count = bytes_read = req->read_size;

        count = min( count, *len - pos );
        memcpy( buffer + pos, req->read_buf + req->read_pos, count );
        pos += count;
        remove_data( req, bytes_read );
        if (eol) break;

        if ((res = read_more_data( req, -1 )) != ERROR_SUCCESS || !req->read_size)
        {
            *len = 0;
            TRACE( "returning empty string\n" );
            LeaveCriticalSection( &req->read_section );
            INTERNET_SetLastError(res);
            return FALSE;
        }
    }
    LeaveCriticalSection( &req->read_section );

    if (pos < *len)
    {
        if (pos && buffer[pos - 1] == '\r') pos--;
        *len = pos + 1;
    }
    buffer[*len - 1] = 0;
    TRACE( "returning %s\n", debugstr_a(buffer));
    return TRUE;
}

/* discard data contents until we reach end of line (the read section must be held) */
static DWORD discard_eol( http_request_t *req )
{
    DWORD res;

    do
    {
        BYTE *eol = memchr( req->read_buf + req->read_pos, '\n', req->read_size );
        if (eol)
        {
            remove_data( req, (eol + 1) - (req->read_buf + req->read_pos) );
            break;
        }
        req->read_pos = req->read_size = 0;  /* discard everything */
        if ((res = read_more_data( req, -1 )) != ERROR_SUCCESS) return res;
    } while (req->read_size);
    return ERROR_SUCCESS;
}

/* read the size of the next chunk (the read section must be held) */
static DWORD start_next_chunk( http_request_t *req )
{
    DWORD chunk_size = 0, res;

    if (!req->dwContentLength) return ERROR_SUCCESS;
    if (req->dwContentLength == req->dwContentRead)
    {
        /* read terminator for the previous chunk */
        if ((res = discard_eol( req )) != ERROR_SUCCESS) return res;
        req->dwContentLength = ~0u;
        req->dwContentRead = 0;
    }
    for (;;)
    {
        while (req->read_size)
        {
            char ch = req->read_buf[req->read_pos];
            if (ch >= '0' && ch <= '9') chunk_size = chunk_size * 16 + ch - '0';
            else if (ch >= 'a' && ch <= 'f') chunk_size = chunk_size * 16 + ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F') chunk_size = chunk_size * 16 + ch - 'A' + 10;
            else if (ch == ';' || ch == '\r' || ch == '\n')
            {
                TRACE( "reading %u byte chunk\n", chunk_size );
                req->dwContentLength = chunk_size;
                req->dwContentRead = 0;
                return discard_eol( req );
            }
            remove_data( req, 1 );
        }
        if ((res = read_more_data( req, -1 )) != ERROR_SUCCESS) return res;
        if (!req->read_size)
        {
            req->dwContentLength = req->dwContentRead = 0;
            return ERROR_SUCCESS;
        }
    }
}

/* check if we have reached the end of the data to read (the read section must be held) */
static BOOL end_of_read_data( http_request_t *req )
{
    if (req->gzip_stream) return req->gzip_stream->end_of_data && !req->gzip_stream->buf_size;
    if (req->read_chunked) return (req->dwContentLength == 0);
    if (req->dwContentLength == ~0u) return FALSE;
    return (req->dwContentLength == req->dwContentRead);
}

/* fetch some more data into the read buffer (the read section must be held) */
static DWORD refill_buffer( http_request_t *req )
{
    int len = sizeof(req->read_buf);
    DWORD res;

    if (req->read_chunked && (req->dwContentLength == ~0u || req->dwContentLength == req->dwContentRead))
    {
        if ((res = start_next_chunk( req )) != ERROR_SUCCESS) return res;
    }

    if (req->dwContentLength != ~0u) len = min( len, req->dwContentLength - req->dwContentRead );
    if (len <= req->read_size) return ERROR_SUCCESS;

    if ((res = read_more_data( req, len )) != ERROR_SUCCESS) return res;
    if (!req->read_size) req->dwContentLength = req->dwContentRead = 0;
    return ERROR_SUCCESS;
}

static DWORD read_gzip_data(http_request_t *req, BYTE *buf, int size, BOOL sync, int *read_ret)
{
    DWORD ret = ERROR_SUCCESS;
    int read = 0;

#ifdef HAVE_ZLIB
    z_stream *zstream = &req->gzip_stream->zstream;
    DWORD buf_avail;
    int zres;

    while(read < size && !req->gzip_stream->end_of_data) {
        if(!req->read_size) {
            if(!sync || refill_buffer(req) != ERROR_SUCCESS)
                break;
        }

        buf_avail = req->dwContentLength == ~0 ? req->read_size : min(req->read_size, req->dwContentLength-req->dwContentRead);

        zstream->next_in = req->read_buf+req->read_pos;
        zstream->avail_in = buf_avail;
        zstream->next_out = buf+read;
        zstream->avail_out = size-read;
        zres = inflate(zstream, Z_FULL_FLUSH);
        read = size - zstream->avail_out;
        req->dwContentRead += buf_avail-zstream->avail_in;
        remove_data(req, buf_avail-zstream->avail_in);
        if(zres == Z_STREAM_END) {
            TRACE("end of data\n");
            req->gzip_stream->end_of_data = TRUE;
            inflateEnd(&req->gzip_stream->zstream);
        }else if(zres != Z_OK) {
            WARN("inflate failed %d\n", zres);
            if(!read)
                ret = ERROR_INTERNET_DECODING_FAILED;
            break;
        }
    }
#endif

    *read_ret = read;
    return ret;
}

static void refill_gzip_buffer(http_request_t *req)
{
    DWORD res;
    int len;

    if(!req->gzip_stream || !req->read_size || req->gzip_stream->buf_size == sizeof(req->gzip_stream->buf))
        return;

    if(req->gzip_stream->buf_pos) {
        if(req->gzip_stream->buf_size)
            memmove(req->gzip_stream->buf, req->gzip_stream->buf + req->gzip_stream->buf_pos, req->gzip_stream->buf_size);
        req->gzip_stream->buf_pos = 0;
    }

    res = read_gzip_data(req, req->gzip_stream->buf + req->gzip_stream->buf_size,
            sizeof(req->gzip_stream->buf) - req->gzip_stream->buf_size, FALSE, &len);
    if(res == ERROR_SUCCESS)
        req->gzip_stream->buf_size += len;
}

/* return the size of data available to be read immediately (the read section must be held) */
static DWORD get_avail_data( http_request_t *req )
{
    if (req->gzip_stream) {
        refill_gzip_buffer(req);
        return req->gzip_stream->buf_size;
    }
    if (req->read_chunked && (req->dwContentLength == ~0u || req->dwContentLength == req->dwContentRead))
        return 0;
    return min( req->read_size, req->dwContentLength - req->dwContentRead );
}

static void HTTP_ReceiveRequestData(http_request_t *req, BOOL first_notif)
{
    INTERNET_ASYNC_RESULT iar;
    DWORD res;

    TRACE("%p\n", req);

    EnterCriticalSection( &req->read_section );
    if ((res = refill_buffer( req )) == ERROR_SUCCESS) {
        iar.dwResult = (DWORD_PTR)req->hdr.hInternet;
        iar.dwError = first_notif ? 0 : get_avail_data(req);
    }else {
        iar.dwResult = 0;
        iar.dwError = res;
    }
    LeaveCriticalSection( &req->read_section );

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_REQUEST_COMPLETE, &iar,
                          sizeof(INTERNET_ASYNC_RESULT));
}

/* read data from the http connection (the read section must be held) */
static DWORD HTTPREQ_Read(http_request_t *req, void *buffer, DWORD size, DWORD *read, BOOL sync)
{
    BOOL finished_reading = FALSE;
    int len, bytes_read = 0;
    DWORD ret = ERROR_SUCCESS;

    EnterCriticalSection( &req->read_section );

    if (req->read_chunked && (req->dwContentLength == ~0u || req->dwContentLength == req->dwContentRead))
    {
        if (start_next_chunk( req ) != ERROR_SUCCESS) goto done;
    }

    if(req->gzip_stream) {
        if(req->gzip_stream->buf_size) {
            bytes_read = min(req->gzip_stream->buf_size, size);
            memcpy(buffer, req->gzip_stream->buf + req->gzip_stream->buf_pos, bytes_read);
            req->gzip_stream->buf_pos += bytes_read;
            req->gzip_stream->buf_size -= bytes_read;
        }else if(!req->read_size && !req->gzip_stream->end_of_data) {
            refill_buffer(req);
        }

        if(size > bytes_read) {
            ret = read_gzip_data(req, (BYTE*)buffer+bytes_read, size-bytes_read, sync, &len);
            if(ret == ERROR_SUCCESS)
                bytes_read += len;
        }

        finished_reading = req->gzip_stream->end_of_data && !req->gzip_stream->buf_size;
    }else {
        if (req->dwContentLength != ~0u) size = min( size, req->dwContentLength - req->dwContentRead );

        if (req->read_size) {
            bytes_read = min( req->read_size, size );
            memcpy( buffer, req->read_buf + req->read_pos, bytes_read );
            remove_data( req, bytes_read );
        }

        if (size > bytes_read && (!bytes_read || sync)) {
            if (NETCON_recv( &req->netConnection, (char *)buffer + bytes_read, size - bytes_read,
                             sync ? MSG_WAITALL : 0, &len) == ERROR_SUCCESS)
                bytes_read += len;
            /* always return success, even if the network layer returns an error */
        }

        finished_reading = !bytes_read && req->dwContentRead == req->dwContentLength;
        req->dwContentRead += bytes_read;
    }
done:
    *read = bytes_read;

    TRACE( "retrieved %u bytes (%u/%u)\n", bytes_read, req->dwContentRead, req->dwContentLength );
    LeaveCriticalSection( &req->read_section );

    if(ret == ERROR_SUCCESS && req->lpszCacheFile) {
        BOOL res;
        DWORD dwBytesWritten;

        res = WriteFile(req->hCacheFile, buffer, bytes_read, &dwBytesWritten, NULL);
        if(!res)
            WARN("WriteFile failed: %u\n", GetLastError());
    }

    if(finished_reading)
        HTTP_FinishedReading(req);

    return ret;
}


static DWORD HTTPREQ_ReadFile(object_header_t *hdr, void *buffer, DWORD size, DWORD *read)
{
    http_request_t *req = (http_request_t*)hdr;
    return HTTPREQ_Read(req, buffer, size, read, TRUE);
}

static void HTTPREQ_AsyncReadFileExAProc(WORKREQUEST *workRequest)
{
    struct WORKREQ_INTERNETREADFILEEXA const *data = &workRequest->u.InternetReadFileExA;
    http_request_t *req = (http_request_t*)workRequest->hdr;
    INTERNET_ASYNC_RESULT iar;
    DWORD res;

    TRACE("INTERNETREADFILEEXA %p\n", workRequest->hdr);

    res = HTTPREQ_Read(req, data->lpBuffersOut->lpvBuffer,
            data->lpBuffersOut->dwBufferLength, &data->lpBuffersOut->dwBufferLength, TRUE);

    iar.dwResult = res == ERROR_SUCCESS;
    iar.dwError = res;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext,
                          INTERNET_STATUS_REQUEST_COMPLETE, &iar,
                          sizeof(INTERNET_ASYNC_RESULT));
}

static DWORD HTTPREQ_ReadFileExA(object_header_t *hdr, INTERNET_BUFFERSA *buffers,
        DWORD flags, DWORD_PTR context)
{
    http_request_t *req = (http_request_t*)hdr;
    DWORD res;

    if (flags & ~(IRF_ASYNC|IRF_NO_WAIT))
        FIXME("these dwFlags aren't implemented: 0x%x\n", flags & ~(IRF_ASYNC|IRF_NO_WAIT));

    if (buffers->dwStructSize != sizeof(*buffers))
        return ERROR_INVALID_PARAMETER;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

    if ((hdr->dwFlags & INTERNET_FLAG_ASYNC) && !get_avail_data(req))
    {
        WORKREQUEST workRequest;

        if (TryEnterCriticalSection( &req->read_section ))
        {
            if (get_avail_data(req))
            {
                res = HTTPREQ_Read(req, buffers->lpvBuffer, buffers->dwBufferLength,
                                   &buffers->dwBufferLength, FALSE);
                LeaveCriticalSection( &req->read_section );
                goto done;
            }
            LeaveCriticalSection( &req->read_section );
        }

        workRequest.asyncproc = HTTPREQ_AsyncReadFileExAProc;
        workRequest.hdr = WININET_AddRef(&req->hdr);
        workRequest.u.InternetReadFileExA.lpBuffersOut = buffers;

        INTERNET_AsyncCall(&workRequest);

        return ERROR_IO_PENDING;
    }

    res = HTTPREQ_Read(req, buffers->lpvBuffer, buffers->dwBufferLength, &buffers->dwBufferLength,
            !(flags & IRF_NO_WAIT));

done:
    if (res == ERROR_SUCCESS) {
        DWORD size = buffers->dwBufferLength;
        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED,
                &size, sizeof(size));
    }

    return res;
}

static void HTTPREQ_AsyncReadFileExWProc(WORKREQUEST *workRequest)
{
    struct WORKREQ_INTERNETREADFILEEXW const *data = &workRequest->u.InternetReadFileExW;
    http_request_t *req = (http_request_t*)workRequest->hdr;
    INTERNET_ASYNC_RESULT iar;
    DWORD res;

    TRACE("INTERNETREADFILEEXW %p\n", workRequest->hdr);

    res = HTTPREQ_Read(req, data->lpBuffersOut->lpvBuffer,
            data->lpBuffersOut->dwBufferLength, &data->lpBuffersOut->dwBufferLength, TRUE);

    iar.dwResult = res == ERROR_SUCCESS;
    iar.dwError = res;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext,
                          INTERNET_STATUS_REQUEST_COMPLETE, &iar,
                          sizeof(INTERNET_ASYNC_RESULT));
}

static DWORD HTTPREQ_ReadFileExW(object_header_t *hdr, INTERNET_BUFFERSW *buffers,
        DWORD flags, DWORD_PTR context)
{

    http_request_t *req = (http_request_t*)hdr;
    DWORD res;

    if (flags & ~(IRF_ASYNC|IRF_NO_WAIT))
        FIXME("these dwFlags aren't implemented: 0x%x\n", flags & ~(IRF_ASYNC|IRF_NO_WAIT));

    if (buffers->dwStructSize != sizeof(*buffers))
        return ERROR_INVALID_PARAMETER;

    INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

    if (hdr->dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;

        if (TryEnterCriticalSection( &req->read_section ))
        {
            if (get_avail_data(req))
            {
                res = HTTPREQ_Read(req, buffers->lpvBuffer, buffers->dwBufferLength,
                                   &buffers->dwBufferLength, FALSE);
                LeaveCriticalSection( &req->read_section );
                goto done;
            }
            LeaveCriticalSection( &req->read_section );
        }

        workRequest.asyncproc = HTTPREQ_AsyncReadFileExWProc;
        workRequest.hdr = WININET_AddRef(&req->hdr);
        workRequest.u.InternetReadFileExW.lpBuffersOut = buffers;

        INTERNET_AsyncCall(&workRequest);

        return ERROR_IO_PENDING;
    }

    res = HTTPREQ_Read(req, buffers->lpvBuffer, buffers->dwBufferLength, &buffers->dwBufferLength,
            !(flags & IRF_NO_WAIT));

done:
    if (res == ERROR_SUCCESS) {
        DWORD size = buffers->dwBufferLength;
        INTERNET_SendCallback(&req->hdr, req->hdr.dwContext, INTERNET_STATUS_RESPONSE_RECEIVED,
                &size, sizeof(size));
    }

    return res;
}

static DWORD HTTPREQ_WriteFile(object_header_t *hdr, const void *buffer, DWORD size, DWORD *written)
{
    DWORD res;
    http_request_t *lpwhr = (http_request_t*)hdr;

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext, INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

    *written = 0;
    res = NETCON_send(&lpwhr->netConnection, buffer, size, 0, (LPINT)written);
    if (res == ERROR_SUCCESS)
        lpwhr->dwBytesWritten += *written;

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext, INTERNET_STATUS_REQUEST_SENT, written, sizeof(DWORD));
    return res;
}

static void HTTPREQ_AsyncQueryDataAvailableProc(WORKREQUEST *workRequest)
{
    http_request_t *req = (http_request_t*)workRequest->hdr;

    HTTP_ReceiveRequestData(req, FALSE);
}

static DWORD HTTPREQ_QueryDataAvailable(object_header_t *hdr, DWORD *available, DWORD flags, DWORD_PTR ctx)
{
    http_request_t *req = (http_request_t*)hdr;

    TRACE("(%p %p %x %lx)\n", req, available, flags, ctx);

    if (req->lpHttpSession->lpAppInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;

        /* never wait, if we can't enter the section we queue an async request right away */
        if (TryEnterCriticalSection( &req->read_section ))
        {
            if ((*available = get_avail_data( req ))) goto done;
            if (end_of_read_data( req )) goto done;
            LeaveCriticalSection( &req->read_section );
        }

        workRequest.asyncproc = HTTPREQ_AsyncQueryDataAvailableProc;
        workRequest.hdr = WININET_AddRef( &req->hdr );

        INTERNET_AsyncCall(&workRequest);

        return ERROR_IO_PENDING;
    }

    EnterCriticalSection( &req->read_section );

    if (!(*available = get_avail_data( req )) && !end_of_read_data( req ))
    {
        refill_buffer( req );
        *available = get_avail_data( req );
    }

done:
    if (*available == sizeof(req->read_buf) && !req->gzip_stream)  /* check if we have even more pending in the socket */
    {
        DWORD extra;
        if (NETCON_query_data_available(&req->netConnection, &extra))
            *available = min( *available + extra, req->dwContentLength - req->dwContentRead );
    }
    LeaveCriticalSection( &req->read_section );

    TRACE( "returning %u\n", *available );
    return ERROR_SUCCESS;
}

static const object_vtbl_t HTTPREQVtbl = {
    HTTPREQ_Destroy,
    HTTPREQ_CloseConnection,
    HTTPREQ_QueryOption,
    HTTPREQ_SetOption,
    HTTPREQ_ReadFile,
    HTTPREQ_ReadFileExA,
    HTTPREQ_ReadFileExW,
    HTTPREQ_WriteFile,
    HTTPREQ_QueryDataAvailable,
    NULL
};

/***********************************************************************
 *           HTTP_HttpOpenRequestW (internal)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
static DWORD HTTP_HttpOpenRequestW(http_session_t *lpwhs,
        LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
        LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
        DWORD dwFlags, DWORD_PTR dwContext, HINTERNET *ret)
{
    appinfo_t *hIC = NULL;
    http_request_t *lpwhr;
    LPWSTR lpszHostName = NULL;
    HINTERNET handle = NULL;
    static const WCHAR szHostForm[] = {'%','s',':','%','u',0};
    DWORD len, res;

    TRACE("-->\n");

    assert( lpwhs->hdr.htype == WH_HHTTPSESSION );
    hIC = lpwhs->lpAppInfo;

    lpwhr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(http_request_t));
    if (NULL == lpwhr)
    {
        res = ERROR_OUTOFMEMORY;
        goto lend;
    }
    lpwhr->hdr.htype = WH_HHTTPREQ;
    lpwhr->hdr.vtbl = &HTTPREQVtbl;
    lpwhr->hdr.dwFlags = dwFlags;
    lpwhr->hdr.dwContext = dwContext;
    lpwhr->hdr.refs = 1;
    lpwhr->hdr.lpfnStatusCB = lpwhs->hdr.lpfnStatusCB;
    lpwhr->hdr.dwInternalFlags = lpwhs->hdr.dwInternalFlags & INET_CALLBACKW;
    lpwhr->dwContentLength = ~0u;
    InitializeCriticalSection( &lpwhr->read_section );

    WININET_AddRef( &lpwhs->hdr );
    lpwhr->lpHttpSession = lpwhs;
    list_add_head( &lpwhs->hdr.children, &lpwhr->hdr.entry );

    lpszHostName = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR) *
            (strlenW(lpwhs->lpszHostName) + 7 /* length of ":65535" + 1 */));
    if (NULL == lpszHostName)
    {
        res = ERROR_OUTOFMEMORY;
        goto lend;
    }

    handle = WININET_AllocHandle( &lpwhr->hdr );
    if (NULL == handle)
    {
        res = ERROR_OUTOFMEMORY;
        goto lend;
    }

    if ((res = NETCON_init(&lpwhr->netConnection, dwFlags & INTERNET_FLAG_SECURE)) != ERROR_SUCCESS)
    {
        InternetCloseHandle( handle );
        handle = NULL;
        goto lend;
    }

    if (lpszObjectName && *lpszObjectName) {
        HRESULT rc;

        len = 0;
        rc = UrlEscapeW(lpszObjectName, NULL, &len, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            len = strlenW(lpszObjectName)+1;
        lpwhr->lpszPath = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        rc = UrlEscapeW(lpszObjectName, lpwhr->lpszPath, &len,
                   URL_ESCAPE_SPACES_ONLY);
        if (rc != S_OK)
        {
            ERR("Unable to escape string!(%s) (%d)\n",debugstr_w(lpszObjectName),rc);
            strcpyW(lpwhr->lpszPath,lpszObjectName);
        }
    }else {
        static const WCHAR slashW[] = {'/',0};

        lpwhr->lpszPath = heap_strdupW(slashW);
    }

    if (lpszReferrer && *lpszReferrer)
        HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpszReferrer, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REQ);

    if (lpszAcceptTypes)
    {
        int i;
        for (i = 0; lpszAcceptTypes[i]; i++)
        {
            if (!*lpszAcceptTypes[i]) continue;
            HTTP_ProcessHeader(lpwhr, HTTP_ACCEPT, lpszAcceptTypes[i],
                               HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA |
                               HTTP_ADDHDR_FLAG_REQ |
                               (i == 0 ? HTTP_ADDHDR_FLAG_REPLACE : 0));
        }
    }

    lpwhr->lpszVerb = heap_strdupW(lpszVerb && *lpszVerb ? lpszVerb : szGET);
    lpwhr->lpszVersion = heap_strdupW(lpszVersion ? lpszVersion : g_szHttp1_1);

    if (lpwhs->nHostPort != INTERNET_INVALID_PORT_NUMBER &&
        lpwhs->nHostPort != INTERNET_DEFAULT_HTTP_PORT &&
        lpwhs->nHostPort != INTERNET_DEFAULT_HTTPS_PORT)
    {
        sprintfW(lpszHostName, szHostForm, lpwhs->lpszHostName, lpwhs->nHostPort);
        HTTP_ProcessHeader(lpwhr, hostW, lpszHostName,
                HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REQ);
    }
    else
        HTTP_ProcessHeader(lpwhr, hostW, lpwhs->lpszHostName,
                HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REQ);

    if (lpwhs->nServerPort == INTERNET_INVALID_PORT_NUMBER)
        lpwhs->nServerPort = (dwFlags & INTERNET_FLAG_SECURE ?
                        INTERNET_DEFAULT_HTTPS_PORT :
                        INTERNET_DEFAULT_HTTP_PORT);

    if (lpwhs->nHostPort == INTERNET_INVALID_PORT_NUMBER)
        lpwhs->nHostPort = (dwFlags & INTERNET_FLAG_SECURE ?
                        INTERNET_DEFAULT_HTTPS_PORT :
                        INTERNET_DEFAULT_HTTP_PORT);

    if (NULL != hIC->lpszProxy && hIC->lpszProxy[0] != 0)
        HTTP_DealWithProxy( hIC, lpwhs, lpwhr );

    INTERNET_SendCallback(&lpwhs->hdr, dwContext,
                          INTERNET_STATUS_HANDLE_CREATED, &handle,
                          sizeof(handle));

lend:
    HeapFree(GetProcessHeap(), 0, lpszHostName);
    if( lpwhr )
        WININET_Release( &lpwhr->hdr );

    TRACE("<-- %p (%p)\n", handle, lpwhr);
    *ret = handle;
    return res;
}

/***********************************************************************
 *           HttpOpenRequestW (WININET.@)
 *
 * Open a HTTP request handle
 *
 * RETURNS
 *    HINTERNET  a HTTP request handle on success
 *    NULL 	 on failure
 *
 */
HINTERNET WINAPI HttpOpenRequestW(HINTERNET hHttpSession,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD_PTR dwContext)
{
    http_session_t *lpwhs;
    HINTERNET handle = NULL;
    DWORD res;

    TRACE("(%p, %s, %s, %s, %s, %p, %08x, %08lx)\n", hHttpSession,
          debugstr_w(lpszVerb), debugstr_w(lpszObjectName),
          debugstr_w(lpszVersion), debugstr_w(lpszReferrer), lpszAcceptTypes,
          dwFlags, dwContext);
    if(lpszAcceptTypes!=NULL)
    {
        int i;
        for(i=0;lpszAcceptTypes[i]!=NULL;i++)
            TRACE("\taccept type: %s\n",debugstr_w(lpszAcceptTypes[i]));
    }

    lpwhs = (http_session_t*) WININET_GetObject( hHttpSession );
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    /*
     * My tests seem to show that the windows version does not
     * become asynchronous until after this point. And anyhow
     * if this call was asynchronous then how would you get the
     * necessary HINTERNET pointer returned by this function.
     *
     */
    res = HTTP_HttpOpenRequestW(lpwhs, lpszVerb, lpszObjectName,
                                lpszVersion, lpszReferrer, lpszAcceptTypes,
                                dwFlags, dwContext, &handle);
lend:
    if( lpwhs )
        WININET_Release( &lpwhs->hdr );
    TRACE("returning %p\n", handle);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return handle;
}

/* read any content returned by the server so that the connection can be
 * reused */
static void HTTP_DrainContent(http_request_t *req)
{
    DWORD bytes_read;

    if (!NETCON_connected(&req->netConnection)) return;

    if (req->dwContentLength == -1)
    {
        NETCON_close(&req->netConnection);
        return;
    }
    if (!strcmpW(req->lpszVerb, szHEAD)) return;

    do
    {
        char buffer[2048];
        if (HTTPREQ_Read(req, buffer, sizeof(buffer), &bytes_read, TRUE) != ERROR_SUCCESS)
            return;
    } while (bytes_read);
}

static const LPCWSTR header_lookup[] = {
    szMime_Version,		/* HTTP_QUERY_MIME_VERSION = 0 */
    szContent_Type,		/* HTTP_QUERY_CONTENT_TYPE = 1 */
    szContent_Transfer_Encoding,/* HTTP_QUERY_CONTENT_TRANSFER_ENCODING = 2 */
    szContent_ID,		/* HTTP_QUERY_CONTENT_ID = 3 */
    NULL,			/* HTTP_QUERY_CONTENT_DESCRIPTION = 4 */
    szContent_Length,		/* HTTP_QUERY_CONTENT_LENGTH =  5 */
    szContent_Language,		/* HTTP_QUERY_CONTENT_LANGUAGE =  6 */
    szAllow,			/* HTTP_QUERY_ALLOW = 7 */
    szPublic,			/* HTTP_QUERY_PUBLIC = 8 */
    szDate,			/* HTTP_QUERY_DATE = 9 */
    szExpires,			/* HTTP_QUERY_EXPIRES = 10 */
    szLast_Modified,		/* HTTP_QUERY_LAST_MODIFIED = 11 */
    NULL,			/* HTTP_QUERY_MESSAGE_ID = 12 */
    szURI,			/* HTTP_QUERY_URI = 13 */
    szFrom,			/* HTTP_QUERY_DERIVED_FROM = 14 */
    NULL,			/* HTTP_QUERY_COST = 15 */
    NULL,			/* HTTP_QUERY_LINK = 16 */
    szPragma,			/* HTTP_QUERY_PRAGMA = 17 */
    NULL,			/* HTTP_QUERY_VERSION = 18 */
    szStatus,			/* HTTP_QUERY_STATUS_CODE = 19 */
    NULL,			/* HTTP_QUERY_STATUS_TEXT = 20 */
    NULL,			/* HTTP_QUERY_RAW_HEADERS = 21 */
    NULL,			/* HTTP_QUERY_RAW_HEADERS_CRLF = 22 */
    szConnection,		/* HTTP_QUERY_CONNECTION = 23 */
    szAccept,			/* HTTP_QUERY_ACCEPT = 24 */
    szAccept_Charset,		/* HTTP_QUERY_ACCEPT_CHARSET = 25 */
    szAccept_Encoding,		/* HTTP_QUERY_ACCEPT_ENCODING = 26 */
    szAccept_Language,		/* HTTP_QUERY_ACCEPT_LANGUAGE = 27 */
    szAuthorization,		/* HTTP_QUERY_AUTHORIZATION = 28 */
    szContent_Encoding,		/* HTTP_QUERY_CONTENT_ENCODING = 29 */
    NULL,			/* HTTP_QUERY_FORWARDED = 30 */
    NULL,			/* HTTP_QUERY_FROM = 31 */
    szIf_Modified_Since,	/* HTTP_QUERY_IF_MODIFIED_SINCE = 32 */
    szLocation,			/* HTTP_QUERY_LOCATION = 33 */
    NULL,			/* HTTP_QUERY_ORIG_URI = 34 */
    szReferer,			/* HTTP_QUERY_REFERER = 35 */
    szRetry_After,		/* HTTP_QUERY_RETRY_AFTER = 36 */
    szServer,			/* HTTP_QUERY_SERVER = 37 */
    NULL,			/* HTTP_TITLE = 38 */
    szUser_Agent,		/* HTTP_QUERY_USER_AGENT = 39 */
    szWWW_Authenticate,		/* HTTP_QUERY_WWW_AUTHENTICATE = 40 */
    szProxy_Authenticate,	/* HTTP_QUERY_PROXY_AUTHENTICATE = 41 */
    szAccept_Ranges,		/* HTTP_QUERY_ACCEPT_RANGES = 42 */
    szSet_Cookie,		/* HTTP_QUERY_SET_COOKIE = 43 */
    szCookie,			/* HTTP_QUERY_COOKIE = 44 */
    NULL,			/* HTTP_QUERY_REQUEST_METHOD = 45 */
    NULL,			/* HTTP_QUERY_REFRESH = 46 */
    NULL,			/* HTTP_QUERY_CONTENT_DISPOSITION = 47 */
    szAge,			/* HTTP_QUERY_AGE = 48 */
    szCache_Control,		/* HTTP_QUERY_CACHE_CONTROL = 49 */
    szContent_Base,		/* HTTP_QUERY_CONTENT_BASE = 50 */
    szContent_Location,		/* HTTP_QUERY_CONTENT_LOCATION = 51 */
    szContent_MD5,		/* HTTP_QUERY_CONTENT_MD5 = 52 */
    szContent_Range,		/* HTTP_QUERY_CONTENT_RANGE = 53 */
    szETag,			/* HTTP_QUERY_ETAG = 54 */
    hostW,			/* HTTP_QUERY_HOST = 55 */
    szIf_Match,			/* HTTP_QUERY_IF_MATCH = 56 */
    szIf_None_Match,		/* HTTP_QUERY_IF_NONE_MATCH = 57 */
    szIf_Range,			/* HTTP_QUERY_IF_RANGE = 58 */
    szIf_Unmodified_Since,	/* HTTP_QUERY_IF_UNMODIFIED_SINCE = 59 */
    szMax_Forwards,		/* HTTP_QUERY_MAX_FORWARDS = 60 */
    szProxy_Authorization,	/* HTTP_QUERY_PROXY_AUTHORIZATION = 61 */
    szRange,			/* HTTP_QUERY_RANGE = 62 */
    szTransfer_Encoding,	/* HTTP_QUERY_TRANSFER_ENCODING = 63 */
    szUpgrade,			/* HTTP_QUERY_UPGRADE = 64 */
    szVary,			/* HTTP_QUERY_VARY = 65 */
    szVia,			/* HTTP_QUERY_VIA = 66 */
    szWarning,			/* HTTP_QUERY_WARNING = 67 */
    szExpect,			/* HTTP_QUERY_EXPECT = 68 */
    szProxy_Connection,		/* HTTP_QUERY_PROXY_CONNECTION = 69 */
    szUnless_Modified_Since,	/* HTTP_QUERY_UNLESS_MODIFIED_SINCE = 70 */
};

#define LAST_TABLE_HEADER (sizeof(header_lookup)/sizeof(header_lookup[0]))

/***********************************************************************
 *           HTTP_HttpQueryInfoW (internal)
 */
static DWORD HTTP_HttpQueryInfoW(http_request_t *lpwhr, DWORD dwInfoLevel,
        LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    BOOL request_only = dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS;
    INT requested_index = lpdwIndex ? *lpdwIndex : 0;
    DWORD level = (dwInfoLevel & ~HTTP_QUERY_MODIFIER_FLAGS_MASK);
    INT index = -1;

    /* Find requested header structure */
    switch (level)
    {
    case HTTP_QUERY_CUSTOM:
        if (!lpBuffer) return ERROR_INVALID_PARAMETER;
        index = HTTP_GetCustomHeaderIndex(lpwhr, lpBuffer, requested_index, request_only);
        break;
    case HTTP_QUERY_RAW_HEADERS_CRLF:
        {
            LPWSTR headers;
            DWORD len = 0;
            DWORD res = ERROR_INVALID_PARAMETER;

            if (request_only)
                headers = HTTP_BuildHeaderRequestString(lpwhr, lpwhr->lpszVerb, lpwhr->lpszPath, lpwhr->lpszVersion);
            else
                headers = lpwhr->lpszRawHeaders;

            if (headers)
                len = strlenW(headers) * sizeof(WCHAR);

            if (len + sizeof(WCHAR) > *lpdwBufferLength)
            {
                len += sizeof(WCHAR);
                res = ERROR_INSUFFICIENT_BUFFER;
            }
            else if (lpBuffer)
            {
                if (headers)
                    memcpy(lpBuffer, headers, len + sizeof(WCHAR));
                else
                {
                    len = strlenW(szCrLf) * sizeof(WCHAR);
                    memcpy(lpBuffer, szCrLf, sizeof(szCrLf));
                }
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len / sizeof(WCHAR)));
                res = ERROR_SUCCESS;
            }
            *lpdwBufferLength = len;

            if (request_only)
                HeapFree(GetProcessHeap(), 0, headers);
            return res;
        }
    case HTTP_QUERY_RAW_HEADERS:
        {
            LPWSTR * ppszRawHeaderLines = HTTP_Tokenize(lpwhr->lpszRawHeaders, szCrLf);
            DWORD i, size = 0;
            LPWSTR pszString = lpBuffer;

            for (i = 0; ppszRawHeaderLines[i]; i++)
                size += strlenW(ppszRawHeaderLines[i]) + 1;

            if (size + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                HTTP_FreeTokens(ppszRawHeaderLines);
                *lpdwBufferLength = (size + 1) * sizeof(WCHAR);
                return ERROR_INSUFFICIENT_BUFFER;
            }
            if (pszString)
            {
                for (i = 0; ppszRawHeaderLines[i]; i++)
                {
                    DWORD len = strlenW(ppszRawHeaderLines[i]);
                    memcpy(pszString, ppszRawHeaderLines[i], (len+1)*sizeof(WCHAR));
                    pszString += len+1;
                }
                *pszString = '\0';
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, size));
            }
            *lpdwBufferLength = size * sizeof(WCHAR);
            HTTP_FreeTokens(ppszRawHeaderLines);

            return ERROR_SUCCESS;
        }
    case HTTP_QUERY_STATUS_TEXT:
        if (lpwhr->lpszStatusText)
        {
            DWORD len = strlenW(lpwhr->lpszStatusText);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                return ERROR_INSUFFICIENT_BUFFER;
            }
            if (lpBuffer)
            {
                memcpy(lpBuffer, lpwhr->lpszStatusText, (len + 1) * sizeof(WCHAR));
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len));
            }
            *lpdwBufferLength = len * sizeof(WCHAR);
            return ERROR_SUCCESS;
        }
        break;
    case HTTP_QUERY_VERSION:
        if (lpwhr->lpszVersion)
        {
            DWORD len = strlenW(lpwhr->lpszVersion);
            if (len + 1 > *lpdwBufferLength/sizeof(WCHAR))
            {
                *lpdwBufferLength = (len + 1) * sizeof(WCHAR);
                return ERROR_INSUFFICIENT_BUFFER;
            }
            if (lpBuffer)
            {
                memcpy(lpBuffer, lpwhr->lpszVersion, (len + 1) * sizeof(WCHAR));
                TRACE("returning data: %s\n", debugstr_wn(lpBuffer, len));
            }
            *lpdwBufferLength = len * sizeof(WCHAR);
            return ERROR_SUCCESS;
        }
        break;
    case HTTP_QUERY_CONTENT_ENCODING:
        index = HTTP_GetCustomHeaderIndex(lpwhr, header_lookup[lpwhr->gzip_stream ? HTTP_QUERY_CONTENT_TYPE : level],
                requested_index,request_only);
        break;
    default:
        assert (LAST_TABLE_HEADER == (HTTP_QUERY_UNLESS_MODIFIED_SINCE + 1));

        if (level < LAST_TABLE_HEADER && header_lookup[level])
            index = HTTP_GetCustomHeaderIndex(lpwhr, header_lookup[level],
                                              requested_index,request_only);
    }

    if (index >= 0)
        lphttpHdr = &lpwhr->pCustHeaders[index];

    /* Ensure header satisfies requested attributes */
    if (!lphttpHdr ||
        ((dwInfoLevel & HTTP_QUERY_FLAG_REQUEST_HEADERS) &&
         (~lphttpHdr->wFlags & HDR_ISREQUEST)))
    {
        return ERROR_HTTP_HEADER_NOT_FOUND;
    }

    if (lpdwIndex && level != HTTP_QUERY_STATUS_CODE) (*lpdwIndex)++;

    /* coalesce value to requested type */
    if (dwInfoLevel & HTTP_QUERY_FLAG_NUMBER && lpBuffer)
    {
        *(int *)lpBuffer = atoiW(lphttpHdr->lpszValue);
        TRACE(" returning number: %d\n", *(int *)lpBuffer);
     }
    else if (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME && lpBuffer)
    {
        time_t tmpTime;
        struct tm tmpTM;
        SYSTEMTIME *STHook;

        tmpTime = ConvertTimeString(lphttpHdr->lpszValue);

        tmpTM = *gmtime(&tmpTime);
        STHook = (SYSTEMTIME *)lpBuffer;
        STHook->wDay = tmpTM.tm_mday;
        STHook->wHour = tmpTM.tm_hour;
        STHook->wMilliseconds = 0;
        STHook->wMinute = tmpTM.tm_min;
        STHook->wDayOfWeek = tmpTM.tm_wday;
        STHook->wMonth = tmpTM.tm_mon + 1;
        STHook->wSecond = tmpTM.tm_sec;
        STHook->wYear = tmpTM.tm_year;

        TRACE(" returning time: %04d/%02d/%02d - %d - %02d:%02d:%02d.%02d\n",
              STHook->wYear, STHook->wMonth, STHook->wDay, STHook->wDayOfWeek,
              STHook->wHour, STHook->wMinute, STHook->wSecond, STHook->wMilliseconds);
    }
    else if (lphttpHdr->lpszValue)
    {
        DWORD len = (strlenW(lphttpHdr->lpszValue) + 1) * sizeof(WCHAR);

        if (len > *lpdwBufferLength)
        {
            *lpdwBufferLength = len;
            return ERROR_INSUFFICIENT_BUFFER;
        }
        if (lpBuffer)
        {
            memcpy(lpBuffer, lphttpHdr->lpszValue, len);
            TRACE("! returning string: %s\n", debugstr_w(lpBuffer));
        }
        *lpdwBufferLength = len - sizeof(WCHAR);
    }
    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpQueryInfoW (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoW(HINTERNET hHttpRequest, DWORD dwInfoLevel,
        LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    http_request_t *lpwhr;
    DWORD res;

    if (TRACE_ON(wininet)) {
#define FE(x) { x, #x }
	static const wininet_flag_info query_flags[] = {
	    FE(HTTP_QUERY_MIME_VERSION),
	    FE(HTTP_QUERY_CONTENT_TYPE),
	    FE(HTTP_QUERY_CONTENT_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_CONTENT_ID),
	    FE(HTTP_QUERY_CONTENT_DESCRIPTION),
	    FE(HTTP_QUERY_CONTENT_LENGTH),
	    FE(HTTP_QUERY_CONTENT_LANGUAGE),
	    FE(HTTP_QUERY_ALLOW),
	    FE(HTTP_QUERY_PUBLIC),
	    FE(HTTP_QUERY_DATE),
	    FE(HTTP_QUERY_EXPIRES),
	    FE(HTTP_QUERY_LAST_MODIFIED),
	    FE(HTTP_QUERY_MESSAGE_ID),
	    FE(HTTP_QUERY_URI),
	    FE(HTTP_QUERY_DERIVED_FROM),
	    FE(HTTP_QUERY_COST),
	    FE(HTTP_QUERY_LINK),
	    FE(HTTP_QUERY_PRAGMA),
	    FE(HTTP_QUERY_VERSION),
	    FE(HTTP_QUERY_STATUS_CODE),
	    FE(HTTP_QUERY_STATUS_TEXT),
	    FE(HTTP_QUERY_RAW_HEADERS),
	    FE(HTTP_QUERY_RAW_HEADERS_CRLF),
	    FE(HTTP_QUERY_CONNECTION),
	    FE(HTTP_QUERY_ACCEPT),
	    FE(HTTP_QUERY_ACCEPT_CHARSET),
	    FE(HTTP_QUERY_ACCEPT_ENCODING),
	    FE(HTTP_QUERY_ACCEPT_LANGUAGE),
	    FE(HTTP_QUERY_AUTHORIZATION),
	    FE(HTTP_QUERY_CONTENT_ENCODING),
	    FE(HTTP_QUERY_FORWARDED),
	    FE(HTTP_QUERY_FROM),
	    FE(HTTP_QUERY_IF_MODIFIED_SINCE),
	    FE(HTTP_QUERY_LOCATION),
	    FE(HTTP_QUERY_ORIG_URI),
	    FE(HTTP_QUERY_REFERER),
	    FE(HTTP_QUERY_RETRY_AFTER),
	    FE(HTTP_QUERY_SERVER),
	    FE(HTTP_QUERY_TITLE),
	    FE(HTTP_QUERY_USER_AGENT),
	    FE(HTTP_QUERY_WWW_AUTHENTICATE),
	    FE(HTTP_QUERY_PROXY_AUTHENTICATE),
	    FE(HTTP_QUERY_ACCEPT_RANGES),
        FE(HTTP_QUERY_SET_COOKIE),
        FE(HTTP_QUERY_COOKIE),
	    FE(HTTP_QUERY_REQUEST_METHOD),
	    FE(HTTP_QUERY_REFRESH),
	    FE(HTTP_QUERY_CONTENT_DISPOSITION),
	    FE(HTTP_QUERY_AGE),
	    FE(HTTP_QUERY_CACHE_CONTROL),
	    FE(HTTP_QUERY_CONTENT_BASE),
	    FE(HTTP_QUERY_CONTENT_LOCATION),
	    FE(HTTP_QUERY_CONTENT_MD5),
	    FE(HTTP_QUERY_CONTENT_RANGE),
	    FE(HTTP_QUERY_ETAG),
	    FE(HTTP_QUERY_HOST),
	    FE(HTTP_QUERY_IF_MATCH),
	    FE(HTTP_QUERY_IF_NONE_MATCH),
	    FE(HTTP_QUERY_IF_RANGE),
	    FE(HTTP_QUERY_IF_UNMODIFIED_SINCE),
	    FE(HTTP_QUERY_MAX_FORWARDS),
	    FE(HTTP_QUERY_PROXY_AUTHORIZATION),
	    FE(HTTP_QUERY_RANGE),
	    FE(HTTP_QUERY_TRANSFER_ENCODING),
	    FE(HTTP_QUERY_UPGRADE),
	    FE(HTTP_QUERY_VARY),
	    FE(HTTP_QUERY_VIA),
	    FE(HTTP_QUERY_WARNING),
	    FE(HTTP_QUERY_CUSTOM)
	};
	static const wininet_flag_info modifier_flags[] = {
	    FE(HTTP_QUERY_FLAG_REQUEST_HEADERS),
	    FE(HTTP_QUERY_FLAG_SYSTEMTIME),
	    FE(HTTP_QUERY_FLAG_NUMBER),
	    FE(HTTP_QUERY_FLAG_COALESCE)
	};
#undef FE
	DWORD info_mod = dwInfoLevel & HTTP_QUERY_MODIFIER_FLAGS_MASK;
	DWORD info = dwInfoLevel & HTTP_QUERY_HEADER_MASK;
	DWORD i;

	TRACE("(%p, 0x%08x)--> %d\n", hHttpRequest, dwInfoLevel, info);
	TRACE("  Attribute:");
	for (i = 0; i < (sizeof(query_flags) / sizeof(query_flags[0])); i++) {
	    if (query_flags[i].val == info) {
		TRACE(" %s", query_flags[i].name);
		break;
	    }
	}
	if (i == (sizeof(query_flags) / sizeof(query_flags[0]))) {
	    TRACE(" Unknown (%08x)", info);
	}

	TRACE(" Modifier:");
	for (i = 0; i < (sizeof(modifier_flags) / sizeof(modifier_flags[0])); i++) {
	    if (modifier_flags[i].val & info_mod) {
		TRACE(" %s", modifier_flags[i].name);
		info_mod &= ~ modifier_flags[i].val;
	    }
	}
	
	if (info_mod) {
	    TRACE(" Unknown (%08x)", info_mod);
	}
	TRACE("\n");
    }
    
    lpwhr = (http_request_t*) WININET_GetObject( hHttpRequest );
    if (NULL == lpwhr ||  lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    if (lpBuffer == NULL)
        *lpdwBufferLength = 0;
    res = HTTP_HttpQueryInfoW( lpwhr, dwInfoLevel,
                               lpBuffer, lpdwBufferLength, lpdwIndex);

lend:
    if( lpwhr )
         WININET_Release( &lpwhr->hdr );

    TRACE("%u <--\n", res);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpQueryInfoA (WININET.@)
 *
 * Queries for information about an HTTP request
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpQueryInfoA(HINTERNET hHttpRequest, DWORD dwInfoLevel,
	LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex)
{
    BOOL result;
    DWORD len;
    WCHAR* bufferW;

    if((dwInfoLevel & HTTP_QUERY_FLAG_NUMBER) ||
       (dwInfoLevel & HTTP_QUERY_FLAG_SYSTEMTIME))
    {
        return HttpQueryInfoW( hHttpRequest, dwInfoLevel, lpBuffer,
                               lpdwBufferLength, lpdwIndex );
    }

    if (lpBuffer)
    {
        DWORD alloclen;
        len = (*lpdwBufferLength)*sizeof(WCHAR);
        if ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
        {
            alloclen = MultiByteToWideChar( CP_ACP, 0, lpBuffer, -1, NULL, 0 ) * sizeof(WCHAR);
            if (alloclen < len)
                alloclen = len;
        }
        else
            alloclen = len;
        bufferW = HeapAlloc( GetProcessHeap(), 0, alloclen );
        /* buffer is in/out because of HTTP_QUERY_CUSTOM */
        if ((dwInfoLevel & HTTP_QUERY_HEADER_MASK) == HTTP_QUERY_CUSTOM)
            MultiByteToWideChar( CP_ACP, 0, lpBuffer, -1, bufferW, alloclen / sizeof(WCHAR) );
    } else
    {
        bufferW = NULL;
        len = 0;
    }

    result = HttpQueryInfoW( hHttpRequest, dwInfoLevel, bufferW,
                           &len, lpdwIndex );
    if( result )
    {
        len = WideCharToMultiByte( CP_ACP,0, bufferW, len / sizeof(WCHAR) + 1,
                                     lpBuffer, *lpdwBufferLength, NULL, NULL );
        *lpdwBufferLength = len - 1;

        TRACE("lpBuffer: %s\n", debugstr_a(lpBuffer));
    }
    else
        /* since the strings being returned from HttpQueryInfoW should be
         * only ASCII characters, it is reasonable to assume that all of
         * the Unicode characters can be reduced to a single byte */
        *lpdwBufferLength = len / sizeof(WCHAR);

    HeapFree(GetProcessHeap(), 0, bufferW );

    return result;
}

/***********************************************************************
 *           HTTP_GetRedirectURL (internal)
 */
static LPWSTR HTTP_GetRedirectURL(http_request_t *lpwhr, LPCWSTR lpszUrl)
{
    static WCHAR szHttp[] = {'h','t','t','p',0};
    static WCHAR szHttps[] = {'h','t','t','p','s',0};
    http_session_t *lpwhs = lpwhr->lpHttpSession;
    URL_COMPONENTSW urlComponents;
    DWORD url_length = 0;
    LPWSTR orig_url;
    LPWSTR combined_url;

    urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
    urlComponents.lpszScheme = (lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE) ? szHttps : szHttp;
    urlComponents.dwSchemeLength = 0;
    urlComponents.lpszHostName = lpwhs->lpszHostName;
    urlComponents.dwHostNameLength = 0;
    urlComponents.nPort = lpwhs->nHostPort;
    urlComponents.lpszUserName = lpwhs->lpszUserName;
    urlComponents.dwUserNameLength = 0;
    urlComponents.lpszPassword = NULL;
    urlComponents.dwPasswordLength = 0;
    urlComponents.lpszUrlPath = lpwhr->lpszPath;
    urlComponents.dwUrlPathLength = 0;
    urlComponents.lpszExtraInfo = NULL;
    urlComponents.dwExtraInfoLength = 0;

    if (!InternetCreateUrlW(&urlComponents, 0, NULL, &url_length) &&
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        return NULL;

    orig_url = HeapAlloc(GetProcessHeap(), 0, url_length);

    /* convert from bytes to characters */
    url_length = url_length / sizeof(WCHAR) - 1;
    if (!InternetCreateUrlW(&urlComponents, 0, orig_url, &url_length))
    {
        HeapFree(GetProcessHeap(), 0, orig_url);
        return NULL;
    }

    url_length = 0;
    if (!InternetCombineUrlW(orig_url, lpszUrl, NULL, &url_length, ICU_ENCODE_SPACES_ONLY) &&
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        HeapFree(GetProcessHeap(), 0, orig_url);
        return NULL;
    }
    combined_url = HeapAlloc(GetProcessHeap(), 0, url_length * sizeof(WCHAR));

    if (!InternetCombineUrlW(orig_url, lpszUrl, combined_url, &url_length, ICU_ENCODE_SPACES_ONLY))
    {
        HeapFree(GetProcessHeap(), 0, orig_url);
        HeapFree(GetProcessHeap(), 0, combined_url);
        return NULL;
    }
    HeapFree(GetProcessHeap(), 0, orig_url);
    return combined_url;
}


/***********************************************************************
 *           HTTP_HandleRedirect (internal)
 */
static DWORD HTTP_HandleRedirect(http_request_t *lpwhr, LPCWSTR lpszUrl)
{
    http_session_t *lpwhs = lpwhr->lpHttpSession;
    appinfo_t *hIC = lpwhs->lpAppInfo;
    BOOL using_proxy = hIC->lpszProxy && hIC->lpszProxy[0];
    WCHAR path[INTERNET_MAX_URL_LENGTH];
    int index;

    if(lpszUrl[0]=='/')
    {
        /* if it's an absolute path, keep the same session info */
        lstrcpynW(path, lpszUrl, INTERNET_MAX_URL_LENGTH);
    }
    else
    {
        URL_COMPONENTSW urlComponents;
        WCHAR protocol[32], hostName[MAXHOSTNAME], userName[1024];
        static WCHAR szHttp[] = {'h','t','t','p',0};
        static WCHAR szHttps[] = {'h','t','t','p','s',0};

        userName[0] = 0;
        hostName[0] = 0;
        protocol[0] = 0;

        urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
        urlComponents.lpszScheme = protocol;
        urlComponents.dwSchemeLength = 32;
        urlComponents.lpszHostName = hostName;
        urlComponents.dwHostNameLength = MAXHOSTNAME;
        urlComponents.lpszUserName = userName;
        urlComponents.dwUserNameLength = 1024;
        urlComponents.lpszPassword = NULL;
        urlComponents.dwPasswordLength = 0;
        urlComponents.lpszUrlPath = path;
        urlComponents.dwUrlPathLength = 2048;
        urlComponents.lpszExtraInfo = NULL;
        urlComponents.dwExtraInfoLength = 0;
        if(!InternetCrackUrlW(lpszUrl, strlenW(lpszUrl), 0, &urlComponents))
            return INTERNET_GetLastError();

        if (!strncmpW(szHttp, urlComponents.lpszScheme, strlenW(szHttp)) &&
            (lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE))
        {
            TRACE("redirect from secure page to non-secure page\n");
            /* FIXME: warn about from secure redirect to non-secure page */
            lpwhr->hdr.dwFlags &= ~INTERNET_FLAG_SECURE;
        }
        if (!strncmpW(szHttps, urlComponents.lpszScheme, strlenW(szHttps)) &&
            !(lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE))
        {
            TRACE("redirect from non-secure page to secure page\n");
            /* FIXME: notify about redirect to secure page */
            lpwhr->hdr.dwFlags |= INTERNET_FLAG_SECURE;
        }

        if (urlComponents.nPort == INTERNET_INVALID_PORT_NUMBER)
        {
            if (lstrlenW(protocol)>4) /*https*/
                urlComponents.nPort = INTERNET_DEFAULT_HTTPS_PORT;
            else /*http*/
                urlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;
        }

#if 0
        /*
         * This upsets redirects to binary files on sourceforge.net 
         * and gives an html page instead of the target file
         * Examination of the HTTP request sent by native wininet.dll
         * reveals that it doesn't send a referrer in that case.
         * Maybe there's a flag that enables this, or maybe a referrer
         * shouldn't be added in case of a redirect.
         */

        /* consider the current host as the referrer */
        if (lpwhs->lpszServerName && *lpwhs->lpszServerName)
            HTTP_ProcessHeader(lpwhr, HTTP_REFERER, lpwhs->lpszServerName,
                           HTTP_ADDHDR_FLAG_REQ|HTTP_ADDREQ_FLAG_REPLACE|
                           HTTP_ADDHDR_FLAG_ADD_IF_NEW);
#endif
        
        HeapFree(GetProcessHeap(), 0, lpwhs->lpszHostName);
        if (urlComponents.nPort != INTERNET_DEFAULT_HTTP_PORT &&
            urlComponents.nPort != INTERNET_DEFAULT_HTTPS_PORT)
        {
            int len;
            static const WCHAR fmt[] = {'%','s',':','%','i',0};
            len = lstrlenW(hostName);
            len += 7; /* 5 for strlen("65535") + 1 for ":" + 1 for '\0' */
            lpwhs->lpszHostName = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
            sprintfW(lpwhs->lpszHostName, fmt, hostName, urlComponents.nPort);
        }
        else
            lpwhs->lpszHostName = heap_strdupW(hostName);

        HTTP_ProcessHeader(lpwhr, hostW, lpwhs->lpszHostName, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDHDR_FLAG_REQ);

        HeapFree(GetProcessHeap(), 0, lpwhs->lpszUserName);
        lpwhs->lpszUserName = NULL;
        if (userName[0])
            lpwhs->lpszUserName = heap_strdupW(userName);

        if (!using_proxy)
        {
            if (strcmpiW(lpwhs->lpszServerName, hostName) || lpwhs->nServerPort != urlComponents.nPort)
            {
                DWORD res;

                HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
                lpwhs->lpszServerName = heap_strdupW(hostName);
                lpwhs->nServerPort = urlComponents.nPort;

                NETCON_close(&lpwhr->netConnection);
                if ((res = HTTP_ResolveName(lpwhr)) != ERROR_SUCCESS)
                    return res;

                res = NETCON_init(&lpwhr->netConnection, lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE);
                if (res != ERROR_SUCCESS)
                    return res;

                lpwhr->read_pos = lpwhr->read_size = 0;
                lpwhr->read_chunked = FALSE;
            }
        }
        else
            TRACE("Redirect through proxy\n");
    }

    HeapFree(GetProcessHeap(), 0, lpwhr->lpszPath);
    lpwhr->lpszPath=NULL;
    if (*path)
    {
        DWORD needed = 0;
        HRESULT rc;

        rc = UrlEscapeW(path, NULL, &needed, URL_ESCAPE_SPACES_ONLY);
        if (rc != E_POINTER)
            needed = strlenW(path)+1;
        lpwhr->lpszPath = HeapAlloc(GetProcessHeap(), 0, needed*sizeof(WCHAR));
        rc = UrlEscapeW(path, lpwhr->lpszPath, &needed,
                        URL_ESCAPE_SPACES_ONLY);
        if (rc != S_OK)
        {
            ERR("Unable to escape string!(%s) (%d)\n",debugstr_w(path),rc);
            strcpyW(lpwhr->lpszPath,path);
        }
    }

    /* Remove custom content-type/length headers on redirects.  */
    index = HTTP_GetCustomHeaderIndex(lpwhr, szContent_Type, 0, TRUE);
    if (0 <= index)
        HTTP_DeleteCustomHeader(lpwhr, index);
    index = HTTP_GetCustomHeaderIndex(lpwhr, szContent_Length, 0, TRUE);
    if (0 <= index)
        HTTP_DeleteCustomHeader(lpwhr, index);

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           HTTP_build_req (internal)
 *
 *  concatenate all the strings in the request together
 */
static LPWSTR HTTP_build_req( LPCWSTR *list, int len )
{
    LPCWSTR *t;
    LPWSTR str;

    for( t = list; *t ; t++  )
        len += strlenW( *t );
    len++;

    str = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    *str = 0;

    for( t = list; *t ; t++ )
        strcatW( str, *t );

    return str;
}

static DWORD HTTP_SecureProxyConnect(http_request_t *lpwhr)
{
    LPWSTR lpszPath;
    LPWSTR requestString;
    INT len;
    INT cnt;
    INT responseLen;
    char *ascii_req;
    DWORD res;
    static const WCHAR szConnect[] = {'C','O','N','N','E','C','T',0};
    static const WCHAR szFormat[] = {'%','s',':','%','d',0};
    http_session_t *lpwhs = lpwhr->lpHttpSession;

    TRACE("\n");

    lpszPath = HeapAlloc( GetProcessHeap(), 0, (lstrlenW( lpwhs->lpszHostName ) + 13)*sizeof(WCHAR) );
    sprintfW( lpszPath, szFormat, lpwhs->lpszHostName, lpwhs->nHostPort );
    requestString = HTTP_BuildHeaderRequestString( lpwhr, szConnect, lpszPath, g_szHttp1_1 );
    HeapFree( GetProcessHeap(), 0, lpszPath );

    len = WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                                NULL, 0, NULL, NULL );
    len--; /* the nul terminator isn't needed */
    ascii_req = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                            ascii_req, len, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, requestString );

    TRACE("full request -> %s\n", debugstr_an( ascii_req, len ) );

    res = NETCON_send( &lpwhr->netConnection, ascii_req, len, 0, &cnt );
    HeapFree( GetProcessHeap(), 0, ascii_req );
    if (res != ERROR_SUCCESS)
        return res;

    responseLen = HTTP_GetResponseHeaders( lpwhr, TRUE );
    if (!responseLen)
        return ERROR_HTTP_INVALID_HEADER;

    return ERROR_SUCCESS;
}

static void HTTP_InsertCookies(http_request_t *lpwhr)
{
    static const WCHAR szUrlForm[] = {'h','t','t','p',':','/','/','%','s','%','s',0};
    LPWSTR lpszCookies, lpszUrl = NULL;
    DWORD nCookieSize, size;
    LPHTTPHEADERW Host = HTTP_GetHeader(lpwhr, hostW);

    size = (strlenW(Host->lpszValue) + strlenW(szUrlForm) + strlenW(lpwhr->lpszPath)) * sizeof(WCHAR);
    if (!(lpszUrl = HeapAlloc(GetProcessHeap(), 0, size))) return;
    sprintfW( lpszUrl, szUrlForm, Host->lpszValue, lpwhr->lpszPath);

    if (InternetGetCookieW(lpszUrl, NULL, NULL, &nCookieSize))
    {
        int cnt = 0;
        static const WCHAR szCookie[] = {'C','o','o','k','i','e',':',' ',0};

        size = sizeof(szCookie) + nCookieSize * sizeof(WCHAR) + sizeof(szCrLf);
        if ((lpszCookies = HeapAlloc(GetProcessHeap(), 0, size)))
        {
            cnt += sprintfW(lpszCookies, szCookie);
            InternetGetCookieW(lpszUrl, NULL, lpszCookies + cnt, &nCookieSize);
            strcatW(lpszCookies, szCrLf);

            HTTP_HttpAddRequestHeadersW(lpwhr, lpszCookies, strlenW(lpszCookies), HTTP_ADDREQ_FLAG_REPLACE);
            HeapFree(GetProcessHeap(), 0, lpszCookies);
        }
    }
    HeapFree(GetProcessHeap(), 0, lpszUrl);
}

/***********************************************************************
 *           HTTP_HttpSendRequestW (internal)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
static DWORD HTTP_HttpSendRequestW(http_request_t *lpwhr, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional, DWORD dwOptionalLength,
	DWORD dwContentLength, BOOL bEndRequest)
{
    INT cnt;
    BOOL redirected = FALSE;
    LPWSTR requestString = NULL;
    INT responseLen;
    BOOL loop_next;
    INTERNET_ASYNC_RESULT iar;
    static const WCHAR szPost[] = { 'P','O','S','T',0 };
    static const WCHAR szContentLength[] =
        { 'C','o','n','t','e','n','t','-','L','e','n','g','t','h',':',' ','%','l','i','\r','\n',0 };
    WCHAR contentLengthStr[sizeof szContentLength/2 /* includes \r\n */ + 20 /* int */ ];
    DWORD res;

    TRACE("--> %p\n", lpwhr);

    assert(lpwhr->hdr.htype == WH_HHTTPREQ);

    /* if the verb is NULL default to GET */
    if (!lpwhr->lpszVerb)
        lpwhr->lpszVerb = heap_strdupW(szGET);

    if (dwContentLength || strcmpW(lpwhr->lpszVerb, szGET))
    {
        sprintfW(contentLengthStr, szContentLength, dwContentLength);
        HTTP_HttpAddRequestHeadersW(lpwhr, contentLengthStr, -1L, HTTP_ADDREQ_FLAG_REPLACE);
        lpwhr->dwBytesToWrite = dwContentLength;
    }
    if (lpwhr->lpHttpSession->lpAppInfo->lpszAgent)
    {
        WCHAR *agent_header;
        static const WCHAR user_agent[] = {'U','s','e','r','-','A','g','e','n','t',':',' ','%','s','\r','\n',0};
        int len;

        len = strlenW(lpwhr->lpHttpSession->lpAppInfo->lpszAgent) + strlenW(user_agent);
        agent_header = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        sprintfW(agent_header, user_agent, lpwhr->lpHttpSession->lpAppInfo->lpszAgent);

        HTTP_HttpAddRequestHeadersW(lpwhr, agent_header, strlenW(agent_header), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
        HeapFree(GetProcessHeap(), 0, agent_header);
    }
    if (lpwhr->hdr.dwFlags & INTERNET_FLAG_PRAGMA_NOCACHE)
    {
        static const WCHAR pragma_nocache[] = {'P','r','a','g','m','a',':',' ','n','o','-','c','a','c','h','e','\r','\n',0};
        HTTP_HttpAddRequestHeadersW(lpwhr, pragma_nocache, strlenW(pragma_nocache), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    }
    if ((lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_CACHE_WRITE) && !strcmpW(lpwhr->lpszVerb, szPost))
    {
        static const WCHAR cache_control[] = {'C','a','c','h','e','-','C','o','n','t','r','o','l',':',
                                              ' ','n','o','-','c','a','c','h','e','\r','\n',0};
        HTTP_HttpAddRequestHeadersW(lpwhr, cache_control, strlenW(cache_control), HTTP_ADDREQ_FLAG_ADD_IF_NEW);
    }

    do
    {
        DWORD len;
        char *ascii_req;

        loop_next = FALSE;

        /* like native, just in case the caller forgot to call InternetReadFile
         * for all the data */
        HTTP_DrainContent(lpwhr);
        lpwhr->dwContentRead = 0;

        if (TRACE_ON(wininet))
        {
            LPHTTPHEADERW Host = HTTP_GetHeader(lpwhr, hostW);
            TRACE("Going to url %s %s\n", debugstr_w(Host->lpszValue), debugstr_w(lpwhr->lpszPath));
        }

        HTTP_FixURL(lpwhr);
        if (lpwhr->hdr.dwFlags & INTERNET_FLAG_KEEP_CONNECTION)
        {
            HTTP_ProcessHeader(lpwhr, szConnection, szKeepAlive, HTTP_ADDHDR_FLAG_REQ | HTTP_ADDHDR_FLAG_REPLACE);
        }
        HTTP_InsertAuthorization(lpwhr, lpwhr->pAuthInfo, szAuthorization);
        HTTP_InsertAuthorization(lpwhr, lpwhr->pProxyAuthInfo, szProxy_Authorization);

        if (!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_COOKIES))
            HTTP_InsertCookies(lpwhr);

        /* add the headers the caller supplied */
        if( lpszHeaders && dwHeaderLength )
        {
            HTTP_HttpAddRequestHeadersW(lpwhr, lpszHeaders, dwHeaderLength,
                        HTTP_ADDREQ_FLAG_ADD | HTTP_ADDHDR_FLAG_REPLACE);
        }

        if (lpwhr->lpHttpSession->lpAppInfo->lpszProxy && lpwhr->lpHttpSession->lpAppInfo->lpszProxy[0])
        {
            WCHAR *url = HTTP_BuildProxyRequestUrl(lpwhr);
            requestString = HTTP_BuildHeaderRequestString(lpwhr, lpwhr->lpszVerb, url, lpwhr->lpszVersion);
            HeapFree(GetProcessHeap(), 0, url);
        }
        else
            requestString = HTTP_BuildHeaderRequestString(lpwhr, lpwhr->lpszVerb, lpwhr->lpszPath, lpwhr->lpszVersion);

 
        TRACE("Request header -> %s\n", debugstr_w(requestString) );

        /* Send the request and store the results */
        if ((res = HTTP_OpenConnection(lpwhr)) != ERROR_SUCCESS)
            goto lend;

        /* send the request as ASCII, tack on the optional data */
        if (!lpOptional || redirected)
            dwOptionalLength = 0;
        len = WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                                   NULL, 0, NULL, NULL );
        ascii_req = HeapAlloc( GetProcessHeap(), 0, len + dwOptionalLength );
        WideCharToMultiByte( CP_ACP, 0, requestString, -1,
                             ascii_req, len, NULL, NULL );
        if( lpOptional )
            memcpy( &ascii_req[len-1], lpOptional, dwOptionalLength );
        len = (len + dwOptionalLength - 1);
        ascii_req[len] = 0;
        TRACE("full request -> %s\n", debugstr_a(ascii_req) );

        INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                              INTERNET_STATUS_SENDING_REQUEST, NULL, 0);

        res = NETCON_send(&lpwhr->netConnection, ascii_req, len, 0, &cnt);
        HeapFree( GetProcessHeap(), 0, ascii_req );

        lpwhr->dwBytesWritten = dwOptionalLength;

        INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                              INTERNET_STATUS_REQUEST_SENT,
                              &len, sizeof(DWORD));

        if (bEndRequest)
        {
            DWORD dwBufferSize;
            DWORD dwStatusCode;

            INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                                INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);
    
            if (res != ERROR_SUCCESS)
                goto lend;
    
            responseLen = HTTP_GetResponseHeaders(lpwhr, TRUE);
    
            INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                                INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen,
                                sizeof(DWORD));

            HTTP_ProcessCookies(lpwhr);

            if (!set_content_length( lpwhr )) HTTP_FinishedReading(lpwhr);

            dwBufferSize = sizeof(dwStatusCode);
            if (HTTP_HttpQueryInfoW(lpwhr,HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_STATUS_CODE,
                                    &dwStatusCode,&dwBufferSize,NULL) != ERROR_SUCCESS)
                dwStatusCode = 0;

            if (!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT) && responseLen)
            {
                WCHAR *new_url, szNewLocation[INTERNET_MAX_URL_LENGTH];
                dwBufferSize=sizeof(szNewLocation);
                if ((dwStatusCode==HTTP_STATUS_REDIRECT || dwStatusCode==HTTP_STATUS_MOVED) &&
                    HTTP_HttpQueryInfoW(lpwhr,HTTP_QUERY_LOCATION,szNewLocation,&dwBufferSize,NULL) == ERROR_SUCCESS)
                {
                    if (strcmpW(lpwhr->lpszVerb, szGET) && strcmpW(lpwhr->lpszVerb, szHEAD))
                    {
                        HeapFree(GetProcessHeap(), 0, lpwhr->lpszVerb);
                        lpwhr->lpszVerb = heap_strdupW(szGET);
                    }
                    HTTP_DrainContent(lpwhr);
                    if ((new_url = HTTP_GetRedirectURL( lpwhr, szNewLocation )))
                    {
                        INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext, INTERNET_STATUS_REDIRECT,
                                              new_url, (strlenW(new_url) + 1) * sizeof(WCHAR));
                        res = HTTP_HandleRedirect(lpwhr, new_url);
                        if (res == ERROR_SUCCESS)
                        {
                            HeapFree(GetProcessHeap(), 0, requestString);
                            loop_next = TRUE;
                        }
                        HeapFree( GetProcessHeap(), 0, new_url );
                    }
                    redirected = TRUE;
                }
            }
            if (!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_AUTH) && res == ERROR_SUCCESS)
            {
                WCHAR szAuthValue[2048];
                dwBufferSize=2048;
                if (dwStatusCode == HTTP_STATUS_DENIED)
                {
                    LPHTTPHEADERW Host = HTTP_GetHeader(lpwhr, hostW);
                    DWORD dwIndex = 0;
                    while (HTTP_HttpQueryInfoW(lpwhr,HTTP_QUERY_WWW_AUTHENTICATE,szAuthValue,&dwBufferSize,&dwIndex) == ERROR_SUCCESS)
                    {
                        if (HTTP_DoAuthorization(lpwhr, szAuthValue,
                                                 &lpwhr->pAuthInfo,
                                                 lpwhr->lpHttpSession->lpszUserName,
                                                 lpwhr->lpHttpSession->lpszPassword,
                                                 Host->lpszValue))
                        {
                            HeapFree(GetProcessHeap(), 0, requestString);
                            loop_next = TRUE;
                            break;
                        }
                    }
                }
                if (dwStatusCode == HTTP_STATUS_PROXY_AUTH_REQ)
                {
                    DWORD dwIndex = 0;
                    while (HTTP_HttpQueryInfoW(lpwhr,HTTP_QUERY_PROXY_AUTHENTICATE,szAuthValue,&dwBufferSize,&dwIndex) == ERROR_SUCCESS)
                    {
                        if (HTTP_DoAuthorization(lpwhr, szAuthValue,
                                                 &lpwhr->pProxyAuthInfo,
                                                 lpwhr->lpHttpSession->lpAppInfo->lpszProxyUsername,
                                                 lpwhr->lpHttpSession->lpAppInfo->lpszProxyPassword,
                                                 NULL))
                        {
                            loop_next = TRUE;
                            break;
                        }
                    }
                }
            }
        }
        else
            res = ERROR_SUCCESS;
    }
    while (loop_next);

    if(res == ERROR_SUCCESS) {
        WCHAR url[INTERNET_MAX_URL_LENGTH];
        WCHAR cacheFileName[MAX_PATH+1];
        BOOL b;

        b = HTTP_GetRequestURL(lpwhr, url);
        if(!b) {
            WARN("Could not get URL\n");
            goto lend;
        }

        b = CreateUrlCacheEntryW(url, lpwhr->dwContentLength > 0 ? lpwhr->dwContentLength : 0, NULL, cacheFileName, 0);
        if(b) {
            HeapFree(GetProcessHeap(), 0, lpwhr->lpszCacheFile);
            CloseHandle(lpwhr->hCacheFile);

            lpwhr->lpszCacheFile = heap_strdupW(cacheFileName);
            lpwhr->hCacheFile = CreateFileW(lpwhr->lpszCacheFile, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                      NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if(lpwhr->hCacheFile == INVALID_HANDLE_VALUE) {
                WARN("Could not create file: %u\n", GetLastError());
                lpwhr->hCacheFile = NULL;
            }
        }else {
            WARN("Could not create cache entry: %08x\n", GetLastError());
        }
    }

lend:

    HeapFree(GetProcessHeap(), 0, requestString);

    /* TODO: send notification for P3P header */

    if (lpwhr->lpHttpSession->lpAppInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        if (res == ERROR_SUCCESS && lpwhr->dwBytesWritten == lpwhr->dwBytesToWrite)
            HTTP_ReceiveRequestData(lpwhr, TRUE);
        else
        {
            iar.dwResult = (DWORD_PTR)lpwhr->hdr.hInternet;
            iar.dwError = res;

            INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                                  INTERNET_STATUS_REQUEST_COMPLETE, &iar,
                                  sizeof(INTERNET_ASYNC_RESULT));
        }
    }

    TRACE("<--\n");
    return res;
}

/***********************************************************************
 *
 * Helper functions for the HttpSendRequest(Ex) functions
 *
 */
static void AsyncHttpSendRequestProc(WORKREQUEST *workRequest)
{
    struct WORKREQ_HTTPSENDREQUESTW const *req = &workRequest->u.HttpSendRequestW;
    http_request_t *lpwhr = (http_request_t*) workRequest->hdr;

    TRACE("%p\n", lpwhr);

    HTTP_HttpSendRequestW(lpwhr, req->lpszHeader,
            req->dwHeaderLength, req->lpOptional, req->dwOptionalLength,
            req->dwContentLength, req->bEndRequest);

    HeapFree(GetProcessHeap(), 0, req->lpszHeader);
}


static DWORD HTTP_HttpEndRequestW(http_request_t *lpwhr, DWORD dwFlags, DWORD_PTR dwContext)
{
    INT responseLen;
    DWORD dwBufferSize;
    INTERNET_ASYNC_RESULT iar;
    DWORD res = ERROR_SUCCESS;

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                  INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);

    responseLen = HTTP_GetResponseHeaders(lpwhr, TRUE);
    if (!responseLen)
        res = ERROR_HTTP_HEADER_NOT_FOUND;

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                  INTERNET_STATUS_RESPONSE_RECEIVED, &responseLen, sizeof(DWORD));

    /* process cookies here. Is this right? */
    HTTP_ProcessCookies(lpwhr);

    if (!set_content_length( lpwhr )) HTTP_FinishedReading(lpwhr);

    if (!(lpwhr->hdr.dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT))
    {
        DWORD dwCode,dwCodeLength = sizeof(DWORD);
        if (HTTP_HttpQueryInfoW(lpwhr, HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_STATUS_CODE, &dwCode, &dwCodeLength, NULL) == ERROR_SUCCESS
            && (dwCode == 302 || dwCode == 301 || dwCode == 303))
        {
            WCHAR *new_url, szNewLocation[INTERNET_MAX_URL_LENGTH];
            dwBufferSize=sizeof(szNewLocation);
            if (HTTP_HttpQueryInfoW(lpwhr, HTTP_QUERY_LOCATION, szNewLocation, &dwBufferSize, NULL) == ERROR_SUCCESS)
            {
                if (strcmpW(lpwhr->lpszVerb, szGET) && strcmpW(lpwhr->lpszVerb, szHEAD))
                {
                    HeapFree(GetProcessHeap(), 0, lpwhr->lpszVerb);
                    lpwhr->lpszVerb = heap_strdupW(szGET);
                }
                HTTP_DrainContent(lpwhr);
                if ((new_url = HTTP_GetRedirectURL( lpwhr, szNewLocation )))
                {
                    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext, INTERNET_STATUS_REDIRECT,
                                          new_url, (strlenW(new_url) + 1) * sizeof(WCHAR));
                    res = HTTP_HandleRedirect(lpwhr, new_url);
                    if (res == ERROR_SUCCESS)
                        res = HTTP_HttpSendRequestW(lpwhr, NULL, 0, NULL, 0, 0, TRUE);
                    HeapFree( GetProcessHeap(), 0, new_url );
                }
            }
        }
    }

    iar.dwResult = (DWORD_PTR)lpwhr->hdr.hInternet;
    iar.dwError = res;

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_REQUEST_COMPLETE, &iar,
                          sizeof(INTERNET_ASYNC_RESULT));
    return res;
}

/***********************************************************************
 *           HttpEndRequestA (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestA(HINTERNET hRequest,
        LPINTERNET_BUFFERSA lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext)
{
    TRACE("(%p, %p, %08x, %08lx)\n", hRequest, lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersOut)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return HttpEndRequestW(hRequest, NULL, dwFlags, dwContext);
}

static void AsyncHttpEndRequestProc(WORKREQUEST *work)
{
    struct WORKREQ_HTTPENDREQUESTW const *req = &work->u.HttpEndRequestW;
    http_request_t *lpwhr = (http_request_t*)work->hdr;

    TRACE("%p\n", lpwhr);

    HTTP_HttpEndRequestW(lpwhr, req->dwFlags, req->dwContext);
}

/***********************************************************************
 *           HttpEndRequestW (WININET.@)
 *
 * Ends an HTTP request that was started by HttpSendRequestEx
 *
 * RETURNS
 *    TRUE	if successful
 *    FALSE	on failure
 *
 */
BOOL WINAPI HttpEndRequestW(HINTERNET hRequest,
        LPINTERNET_BUFFERSW lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext)
{
    http_request_t *lpwhr;
    DWORD res;

    TRACE("-->\n");

    if (lpBuffersOut)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    lpwhr = (http_request_t*) WININET_GetObject( hRequest );

    if (NULL == lpwhr || lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
        if (lpwhr)
            WININET_Release( &lpwhr->hdr );
        return FALSE;
    }
    lpwhr->hdr.dwFlags |= dwFlags;

    if (lpwhr->lpHttpSession->lpAppInfo->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST work;
        struct WORKREQ_HTTPENDREQUESTW *request;

        work.asyncproc = AsyncHttpEndRequestProc;
        work.hdr = WININET_AddRef( &lpwhr->hdr );

        request = &work.u.HttpEndRequestW;
        request->dwFlags = dwFlags;
        request->dwContext = dwContext;

        INTERNET_AsyncCall(&work);
        res = ERROR_IO_PENDING;
    }
    else
        res = HTTP_HttpEndRequestW(lpwhr, dwFlags, dwContext);

    WININET_Release( &lpwhr->hdr );
    TRACE("%u <--\n", res);
    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestExA (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, call GetLastError() for more information.
 */
BOOL WINAPI HttpSendRequestExA(HINTERNET hRequest,
			       LPINTERNET_BUFFERSA lpBuffersIn,
			       LPINTERNET_BUFFERSA lpBuffersOut,
			       DWORD dwFlags, DWORD_PTR dwContext)
{
    INTERNET_BUFFERSW BuffersInW;
    BOOL rc = FALSE;
    DWORD headerlen;
    LPWSTR header = NULL;

    TRACE("(%p, %p, %p, %08x, %08lx)\n", hRequest, lpBuffersIn,
	    lpBuffersOut, dwFlags, dwContext);

    if (lpBuffersIn)
    {
        BuffersInW.dwStructSize = sizeof(LPINTERNET_BUFFERSW);
        if (lpBuffersIn->lpcszHeader)
        {
            headerlen = MultiByteToWideChar(CP_ACP,0,lpBuffersIn->lpcszHeader,
                    lpBuffersIn->dwHeadersLength,0,0);
            header = HeapAlloc(GetProcessHeap(),0,headerlen*sizeof(WCHAR));
            if (!(BuffersInW.lpcszHeader = header))
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
            BuffersInW.dwHeadersLength = MultiByteToWideChar(CP_ACP, 0,
                    lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength,
                    header, headerlen);
        }
        else
            BuffersInW.lpcszHeader = NULL;
        BuffersInW.dwHeadersTotal = lpBuffersIn->dwHeadersTotal;
        BuffersInW.lpvBuffer = lpBuffersIn->lpvBuffer;
        BuffersInW.dwBufferLength = lpBuffersIn->dwBufferLength;
        BuffersInW.dwBufferTotal = lpBuffersIn->dwBufferTotal;
        BuffersInW.Next = NULL;
    }

    rc = HttpSendRequestExW(hRequest, lpBuffersIn ? &BuffersInW : NULL, NULL, dwFlags, dwContext);

    HeapFree(GetProcessHeap(),0,header);

    return rc;
}

/***********************************************************************
 *           HttpSendRequestExW (WININET.@)
 *
 * Sends the specified request to the HTTP server and allows chunked
 * transfers
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE, call GetLastError() for more information.
 */
BOOL WINAPI HttpSendRequestExW(HINTERNET hRequest,
                   LPINTERNET_BUFFERSW lpBuffersIn,
                   LPINTERNET_BUFFERSW lpBuffersOut,
                   DWORD dwFlags, DWORD_PTR dwContext)
{
    http_request_t *lpwhr;
    http_session_t *lpwhs;
    appinfo_t *hIC;
    DWORD res;

    TRACE("(%p, %p, %p, %08x, %08lx)\n", hRequest, lpBuffersIn,
            lpBuffersOut, dwFlags, dwContext);

    lpwhr = (http_request_t*) WININET_GetObject( hRequest );

    if (NULL == lpwhr || lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    lpwhs = lpwhr->lpHttpSession;
    assert(lpwhs->hdr.htype == WH_HHTTPSESSION);
    hIC = lpwhs->lpAppInfo;
    assert(hIC->hdr.htype == WH_HINIT);

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;
        struct WORKREQ_HTTPSENDREQUESTW *req;

        workRequest.asyncproc = AsyncHttpSendRequestProc;
        workRequest.hdr = WININET_AddRef( &lpwhr->hdr );
        req = &workRequest.u.HttpSendRequestW;
        if (lpBuffersIn)
        {
            DWORD size = 0;

            if (lpBuffersIn->lpcszHeader)
            {
                if (lpBuffersIn->dwHeadersLength == ~0u)
                    size = (strlenW( lpBuffersIn->lpcszHeader ) + 1) * sizeof(WCHAR);
                else
                    size = lpBuffersIn->dwHeadersLength * sizeof(WCHAR);

                req->lpszHeader = HeapAlloc( GetProcessHeap(), 0, size );
                memcpy( req->lpszHeader, lpBuffersIn->lpcszHeader, size );
            }
            else req->lpszHeader = NULL;

            req->dwHeaderLength = size / sizeof(WCHAR);
            req->lpOptional = lpBuffersIn->lpvBuffer;
            req->dwOptionalLength = lpBuffersIn->dwBufferLength;
            req->dwContentLength = lpBuffersIn->dwBufferTotal;
        }
        else
        {
            req->lpszHeader = NULL;
            req->dwHeaderLength = 0;
            req->lpOptional = NULL;
            req->dwOptionalLength = 0;
            req->dwContentLength = 0;
        }

        req->bEndRequest = FALSE;

        INTERNET_AsyncCall(&workRequest);
        /*
         * This is from windows.
         */
        res = ERROR_IO_PENDING;
    }
    else
    {
        if (lpBuffersIn)
            res = HTTP_HttpSendRequestW(lpwhr, lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength,
                                        lpBuffersIn->lpvBuffer, lpBuffersIn->dwBufferLength,
                                        lpBuffersIn->dwBufferTotal, FALSE);
        else
            res = HTTP_HttpSendRequestW(lpwhr, NULL, 0, NULL, 0, 0, FALSE);
    }

lend:
    if ( lpwhr )
        WININET_Release( &lpwhr->hdr );

    TRACE("<---\n");
    SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestW (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestW(HINTERNET hHttpRequest, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    http_request_t *lpwhr;
    http_session_t *lpwhs = NULL;
    appinfo_t *hIC = NULL;
    DWORD res = ERROR_SUCCESS;

    TRACE("%p, %s, %i, %p, %i)\n", hHttpRequest,
            debugstr_wn(lpszHeaders, dwHeaderLength), dwHeaderLength, lpOptional, dwOptionalLength);

    lpwhr = (http_request_t*) WININET_GetObject( hHttpRequest );
    if (NULL == lpwhr || lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    lpwhs = lpwhr->lpHttpSession;
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    hIC = lpwhs->lpAppInfo;
    if (NULL == hIC ||  hIC->hdr.htype != WH_HINIT)
    {
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto lend;
    }

    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC)
    {
        WORKREQUEST workRequest;
        struct WORKREQ_HTTPSENDREQUESTW *req;

        workRequest.asyncproc = AsyncHttpSendRequestProc;
        workRequest.hdr = WININET_AddRef( &lpwhr->hdr );
        req = &workRequest.u.HttpSendRequestW;
        if (lpszHeaders)
        {
            DWORD size;

            if (dwHeaderLength == ~0u) size = (strlenW(lpszHeaders) + 1) * sizeof(WCHAR);
            else size = dwHeaderLength * sizeof(WCHAR);

            req->lpszHeader = HeapAlloc(GetProcessHeap(), 0, size);
            memcpy(req->lpszHeader, lpszHeaders, size);
        }
        else
            req->lpszHeader = 0;
        req->dwHeaderLength = dwHeaderLength;
        req->lpOptional = lpOptional;
        req->dwOptionalLength = dwOptionalLength;
        req->dwContentLength = dwOptionalLength;
        req->bEndRequest = TRUE;

        INTERNET_AsyncCall(&workRequest);
        /*
         * This is from windows.
         */
        res = ERROR_IO_PENDING;
    }
    else
    {
	res = HTTP_HttpSendRequestW(lpwhr, lpszHeaders,
		dwHeaderLength, lpOptional, dwOptionalLength,
		dwOptionalLength, TRUE);
    }
lend:
    if( lpwhr )
        WININET_Release( &lpwhr->hdr );

    SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           HttpSendRequestA (WININET.@)
 *
 * Sends the specified request to the HTTP server
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI HttpSendRequestA(HINTERNET hHttpRequest, LPCSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional ,DWORD dwOptionalLength)
{
    BOOL result;
    LPWSTR szHeaders=NULL;
    DWORD nLen=dwHeaderLength;
    if(lpszHeaders!=NULL)
    {
        nLen=MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,NULL,0);
        szHeaders=HeapAlloc(GetProcessHeap(),0,nLen*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP,0,lpszHeaders,dwHeaderLength,szHeaders,nLen);
    }
    result=HttpSendRequestW(hHttpRequest, szHeaders, nLen, lpOptional, dwOptionalLength);
    HeapFree(GetProcessHeap(),0,szHeaders);
    return result;
}

/***********************************************************************
 *           HTTPSESSION_Destroy (internal)
 *
 * Deallocate session handle
 *
 */
static void HTTPSESSION_Destroy(object_header_t *hdr)
{
    http_session_t *lpwhs = (http_session_t*) hdr;

    TRACE("%p\n", lpwhs);

    WININET_Release(&lpwhs->lpAppInfo->hdr);

    HeapFree(GetProcessHeap(), 0, lpwhs->lpszHostName);
    HeapFree(GetProcessHeap(), 0, lpwhs->lpszServerName);
    HeapFree(GetProcessHeap(), 0, lpwhs->lpszPassword);
    HeapFree(GetProcessHeap(), 0, lpwhs->lpszUserName);
    HeapFree(GetProcessHeap(), 0, lpwhs);
}

static DWORD HTTPSESSION_QueryOption(object_header_t *hdr, DWORD option, void *buffer, DWORD *size, BOOL unicode)
{
    switch(option) {
    case INTERNET_OPTION_HANDLE_TYPE:
        TRACE("INTERNET_OPTION_HANDLE_TYPE\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD*)buffer = INTERNET_HANDLE_TYPE_CONNECT_HTTP;
        return ERROR_SUCCESS;
    }

    return INET_QueryOption(option, buffer, size, unicode);
}

static DWORD HTTPSESSION_SetOption(object_header_t *hdr, DWORD option, void *buffer, DWORD size)
{
    http_session_t *ses = (http_session_t*)hdr;

    switch(option) {
    case INTERNET_OPTION_USERNAME:
    {
        HeapFree(GetProcessHeap(), 0, ses->lpszUserName);
        if (!(ses->lpszUserName = heap_strdupW(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    }
    case INTERNET_OPTION_PASSWORD:
    {
        HeapFree(GetProcessHeap(), 0, ses->lpszPassword);
        if (!(ses->lpszPassword = heap_strdupW(buffer))) return ERROR_OUTOFMEMORY;
        return ERROR_SUCCESS;
    }
    default: break;
    }

    return ERROR_INTERNET_INVALID_OPTION;
}

static const object_vtbl_t HTTPSESSIONVtbl = {
    HTTPSESSION_Destroy,
    NULL,
    HTTPSESSION_QueryOption,
    HTTPSESSION_SetOption,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


/***********************************************************************
 *           HTTP_Connect  (internal)
 *
 * Create http session handle
 *
 * RETURNS
 *   HINTERNET a session handle on success
 *   NULL on failure
 *
 */
DWORD HTTP_Connect(appinfo_t *hIC, LPCWSTR lpszServerName,
        INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
        LPCWSTR lpszPassword, DWORD dwFlags, DWORD_PTR dwContext,
        DWORD dwInternalFlags, HINTERNET *ret)
{
    http_session_t *lpwhs = NULL;
    HINTERNET handle = NULL;
    DWORD res = ERROR_SUCCESS;

    TRACE("-->\n");

    if (!lpszServerName || !lpszServerName[0])
        return ERROR_INVALID_PARAMETER;

    assert( hIC->hdr.htype == WH_HINIT );

    lpwhs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(http_session_t));
    if (!lpwhs)
        return ERROR_OUTOFMEMORY;

   /*
    * According to my tests. The name is not resolved until a request is sent
    */

    lpwhs->hdr.htype = WH_HHTTPSESSION;
    lpwhs->hdr.vtbl = &HTTPSESSIONVtbl;
    lpwhs->hdr.dwFlags = dwFlags;
    lpwhs->hdr.dwContext = dwContext;
    lpwhs->hdr.dwInternalFlags = dwInternalFlags | (hIC->hdr.dwInternalFlags & INET_CALLBACKW);
    lpwhs->hdr.refs = 1;
    lpwhs->hdr.lpfnStatusCB = hIC->hdr.lpfnStatusCB;

    WININET_AddRef( &hIC->hdr );
    lpwhs->lpAppInfo = hIC;
    list_add_head( &hIC->hdr.children, &lpwhs->hdr.entry );

    handle = WININET_AllocHandle( &lpwhs->hdr );
    if (NULL == handle)
    {
        ERR("Failed to alloc handle\n");
        res = ERROR_OUTOFMEMORY;
        goto lerror;
    }

    if(hIC->lpszProxy && hIC->dwAccessType == INTERNET_OPEN_TYPE_PROXY) {
        if(strchrW(hIC->lpszProxy, ' '))
            FIXME("Several proxies not implemented.\n");
        if(hIC->lpszProxyBypass)
            FIXME("Proxy bypass is ignored.\n");
    }
    if (lpszServerName && lpszServerName[0])
    {
        lpwhs->lpszServerName = heap_strdupW(lpszServerName);
        lpwhs->lpszHostName = heap_strdupW(lpszServerName);
    }
    if (lpszUserName && lpszUserName[0])
        lpwhs->lpszUserName = heap_strdupW(lpszUserName);
    if (lpszPassword && lpszPassword[0])
        lpwhs->lpszPassword = heap_strdupW(lpszPassword);
    lpwhs->nServerPort = nServerPort;
    lpwhs->nHostPort = nServerPort;

    /* Don't send a handle created callback if this handle was created with InternetOpenUrl */
    if (!(lpwhs->hdr.dwInternalFlags & INET_OPENURL))
    {
        INTERNET_SendCallback(&hIC->hdr, dwContext,
                              INTERNET_STATUS_HANDLE_CREATED, &handle,
                              sizeof(handle));
    }

lerror:
    if( lpwhs )
        WININET_Release( &lpwhs->hdr );

/*
 * an INTERNET_STATUS_REQUEST_COMPLETE is NOT sent here as per my tests on
 * windows
 */

    TRACE("%p --> %p (%p)\n", hIC, handle, lpwhs);

    if(res == ERROR_SUCCESS)
        *ret = handle;
    return res;
}


/***********************************************************************
 *           HTTP_OpenConnection (internal)
 *
 * Connect to a web server
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on failure
 */
static DWORD HTTP_OpenConnection(http_request_t *lpwhr)
{
    http_session_t *lpwhs;
    appinfo_t *hIC = NULL;
    char szaddr[INET6_ADDRSTRLEN];
    const void *addr;
    DWORD res = ERROR_SUCCESS;

    TRACE("-->\n");


    if (lpwhr->hdr.htype != WH_HHTTPREQ)
    {
        res = ERROR_INVALID_PARAMETER;
        goto lend;
    }

    if (NETCON_connected(&lpwhr->netConnection))
        goto lend;
    if ((res = HTTP_ResolveName(lpwhr)) != ERROR_SUCCESS) goto lend;

    lpwhs = lpwhr->lpHttpSession;

    hIC = lpwhs->lpAppInfo;
    switch (lpwhs->socketAddress.ss_family)
    {
    case AF_INET:
        addr = &((struct sockaddr_in *)&lpwhs->socketAddress)->sin_addr;
        break;
    case AF_INET6:
        addr = &((struct sockaddr_in6 *)&lpwhs->socketAddress)->sin6_addr;
        break;
    default:
        WARN("unsupported family %d\n", lpwhs->socketAddress.ss_family);
        return ERROR_INTERNET_NAME_NOT_RESOLVED;
    }
    inet_ntop(lpwhs->socketAddress.ss_family, addr, szaddr, sizeof(szaddr));
    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_CONNECTING_TO_SERVER,
                          szaddr,
                          strlen(szaddr)+1);

    res = NETCON_create(&lpwhr->netConnection, lpwhs->socketAddress.ss_family, SOCK_STREAM, 0);
    if (res != ERROR_SUCCESS)
    {
        WARN("Socket creation failed: %u\n", res);
        goto lend;
    }

    res = NETCON_connect(&lpwhr->netConnection, (struct sockaddr *)&lpwhs->socketAddress,
                         lpwhs->sa_len);
    if(res != ERROR_SUCCESS)
       goto lend;

    if (lpwhr->hdr.dwFlags & INTERNET_FLAG_SECURE)
    {
        /* Note: we differ from Microsoft's WinINet here. they seem to have
         * a bug that causes no status callbacks to be sent when starting
         * a tunnel to a proxy server using the CONNECT verb. i believe our
         * behaviour to be more correct and to not cause any incompatibilities
         * because using a secure connection through a proxy server is a rare
         * case that would be hard for anyone to depend on */
        if (hIC->lpszProxy && (res = HTTP_SecureProxyConnect(lpwhr)) != ERROR_SUCCESS)
            goto lend;

        res = NETCON_secure_connect(&lpwhr->netConnection, lpwhs->lpszHostName);
        if(res != ERROR_SUCCESS)
        {
            WARN("Couldn't connect securely to host\n");
            goto lend;
        }
    }

    INTERNET_SendCallback(&lpwhr->hdr, lpwhr->hdr.dwContext,
                          INTERNET_STATUS_CONNECTED_TO_SERVER,
                          szaddr, strlen(szaddr)+1);

lend:
    lpwhr->read_pos = lpwhr->read_size = 0;
    lpwhr->read_chunked = FALSE;

    TRACE("%d <--\n", res);
    return res;
}


/***********************************************************************
 *           HTTP_clear_response_headers (internal)
 *
 * clear out any old response headers
 */
static void HTTP_clear_response_headers( http_request_t *lpwhr )
{
    DWORD i;

    for( i=0; i<lpwhr->nCustHeaders; i++)
    {
        if( !lpwhr->pCustHeaders[i].lpszField )
            continue;
        if( !lpwhr->pCustHeaders[i].lpszValue )
            continue;
        if ( lpwhr->pCustHeaders[i].wFlags & HDR_ISREQUEST )
            continue;
        HTTP_DeleteCustomHeader( lpwhr, i );
        i--;
    }
}

/***********************************************************************
 *           HTTP_GetResponseHeaders (internal)
 *
 * Read server response
 *
 * RETURNS
 *
 *   TRUE  on success
 *   FALSE on error
 */
static INT HTTP_GetResponseHeaders(http_request_t *lpwhr, BOOL clear)
{
    INT cbreaks = 0;
    WCHAR buffer[MAX_REPLY_LEN];
    DWORD buflen = MAX_REPLY_LEN;
    BOOL bSuccess = FALSE;
    INT  rc = 0;
    char bufferA[MAX_REPLY_LEN];
    LPWSTR status_code = NULL, status_text = NULL;
    DWORD cchMaxRawHeaders = 1024;
    LPWSTR lpszRawHeaders = NULL;
    LPWSTR temp;
    DWORD cchRawHeaders = 0;
    BOOL codeHundred = FALSE;

    TRACE("-->\n");

    /* clear old response headers (eg. from a redirect response) */
    if (clear) HTTP_clear_response_headers( lpwhr );

    if (!NETCON_connected(&lpwhr->netConnection))
        goto lend;

    do {
        static const WCHAR szHundred[] = {'1','0','0',0};
        /*
         * We should first receive 'HTTP/1.x nnn OK' where nnn is the status code.
         */
        buflen = MAX_REPLY_LEN;
        if (!read_line(lpwhr, bufferA, &buflen))
            goto lend;
        rc += buflen;
        MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );
        /* check is this a status code line? */
        if (!strncmpW(buffer, g_szHttp1_0, 4))
        {
            /* split the version from the status code */
            status_code = strchrW( buffer, ' ' );
            if( !status_code )
                goto lend;
            *status_code++=0;

            /* split the status code from the status text */
            status_text = strchrW( status_code, ' ' );
            if( !status_text )
                goto lend;
            *status_text++=0;

            TRACE("version [%s] status code [%s] status text [%s]\n",
               debugstr_w(buffer), debugstr_w(status_code), debugstr_w(status_text) );

            codeHundred = (!strcmpW(status_code, szHundred));
        }
        else if (!codeHundred)
        {
            WARN("No status line at head of response (%s)\n", debugstr_w(buffer));

            HeapFree(GetProcessHeap(), 0, lpwhr->lpszVersion);
            HeapFree(GetProcessHeap(), 0, lpwhr->lpszStatusText);

            lpwhr->lpszVersion = heap_strdupW(g_szHttp1_0);
            lpwhr->lpszStatusText = heap_strdupW(szOK);

            HeapFree(GetProcessHeap(), 0, lpwhr->lpszRawHeaders);
            lpwhr->lpszRawHeaders = heap_strdupW(szDefaultHeader);

            bSuccess = TRUE;
            goto lend;
        }
    } while (codeHundred);

    /* Add status code */
    HTTP_ProcessHeader(lpwhr, szStatus, status_code,
            HTTP_ADDHDR_FLAG_REPLACE);

    HeapFree(GetProcessHeap(),0,lpwhr->lpszVersion);
    HeapFree(GetProcessHeap(),0,lpwhr->lpszStatusText);

    lpwhr->lpszVersion = heap_strdupW(buffer);
    lpwhr->lpszStatusText = heap_strdupW(status_text);

    /* Restore the spaces */
    *(status_code-1) = ' ';
    *(status_text-1) = ' ';

    /* regenerate raw headers */
    lpszRawHeaders = HeapAlloc(GetProcessHeap(), 0, (cchMaxRawHeaders + 1) * sizeof(WCHAR));
    if (!lpszRawHeaders) goto lend;

    while (cchRawHeaders + buflen + strlenW(szCrLf) > cchMaxRawHeaders)
        cchMaxRawHeaders *= 2;
    temp = HeapReAlloc(GetProcessHeap(), 0, lpszRawHeaders, (cchMaxRawHeaders+1)*sizeof(WCHAR));
    if (temp == NULL) goto lend;
    lpszRawHeaders = temp;
    memcpy(lpszRawHeaders+cchRawHeaders, buffer, (buflen-1)*sizeof(WCHAR));
    cchRawHeaders += (buflen-1);
    memcpy(lpszRawHeaders+cchRawHeaders, szCrLf, sizeof(szCrLf));
    cchRawHeaders += sizeof(szCrLf)/sizeof(szCrLf[0])-1;
    lpszRawHeaders[cchRawHeaders] = '\0';

    /* Parse each response line */
    do
    {
	buflen = MAX_REPLY_LEN;
        if (read_line(lpwhr, bufferA, &buflen))
        {
            LPWSTR * pFieldAndValue;

            TRACE("got line %s, now interpreting\n", debugstr_a(bufferA));

            if (!bufferA[0]) break;
            MultiByteToWideChar( CP_ACP, 0, bufferA, buflen, buffer, MAX_REPLY_LEN );

            pFieldAndValue = HTTP_InterpretHttpHeader(buffer);
            if (pFieldAndValue)
            {
                while (cchRawHeaders + buflen + strlenW(szCrLf) > cchMaxRawHeaders)
                    cchMaxRawHeaders *= 2;
                temp = HeapReAlloc(GetProcessHeap(), 0, lpszRawHeaders, (cchMaxRawHeaders+1)*sizeof(WCHAR));
                if (temp == NULL) goto lend;
                lpszRawHeaders = temp;
                memcpy(lpszRawHeaders+cchRawHeaders, buffer, (buflen-1)*sizeof(WCHAR));
                cchRawHeaders += (buflen-1);
                memcpy(lpszRawHeaders+cchRawHeaders, szCrLf, sizeof(szCrLf));
                cchRawHeaders += sizeof(szCrLf)/sizeof(szCrLf[0])-1;
                lpszRawHeaders[cchRawHeaders] = '\0';

                HTTP_ProcessHeader(lpwhr, pFieldAndValue[0], pFieldAndValue[1],
                                   HTTP_ADDREQ_FLAG_ADD );

                HTTP_FreeTokens(pFieldAndValue);
            }
        }
	else
	{
	    cbreaks++;
	    if (cbreaks >= 2)
	       break;
	}
    }while(1);

    /* make sure the response header is terminated with an empty line.  Some apps really
       truly care about that empty line being there for some reason.  Just add it to the
       header. */
    if (cchRawHeaders + strlenW(szCrLf) > cchMaxRawHeaders)
    {
        cchMaxRawHeaders = cchRawHeaders + strlenW(szCrLf);
        temp = HeapReAlloc(GetProcessHeap(), 0, lpszRawHeaders, (cchMaxRawHeaders + 1) * sizeof(WCHAR));
        if (temp == NULL) goto lend;
        lpszRawHeaders = temp;
    }

    memcpy(&lpszRawHeaders[cchRawHeaders], szCrLf, sizeof(szCrLf));

    HeapFree(GetProcessHeap(), 0, lpwhr->lpszRawHeaders);
    lpwhr->lpszRawHeaders = lpszRawHeaders;
    TRACE("raw headers: %s\n", debugstr_w(lpszRawHeaders));
    bSuccess = TRUE;

lend:

    TRACE("<--\n");
    if (bSuccess)
        return rc;
    else
    {
        HeapFree(GetProcessHeap(), 0, lpszRawHeaders);
        return 0;
    }
}

/***********************************************************************
 *           HTTP_InterpretHttpHeader (internal)
 *
 * Parse server response
 *
 * RETURNS
 *
 *   Pointer to array of field, value, NULL on success.
 *   NULL on error.
 */
static LPWSTR * HTTP_InterpretHttpHeader(LPCWSTR buffer)
{
    LPWSTR * pTokenPair;
    LPWSTR pszColon;
    INT len;

    pTokenPair = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pTokenPair)*3);

    pszColon = strchrW(buffer, ':');
    /* must have two tokens */
    if (!pszColon)
    {
        HTTP_FreeTokens(pTokenPair);
        if (buffer[0])
            TRACE("No ':' in line: %s\n", debugstr_w(buffer));
        return NULL;
    }

    pTokenPair[0] = HeapAlloc(GetProcessHeap(), 0, (pszColon - buffer + 1) * sizeof(WCHAR));
    if (!pTokenPair[0])
    {
        HTTP_FreeTokens(pTokenPair);
        return NULL;
    }
    memcpy(pTokenPair[0], buffer, (pszColon - buffer) * sizeof(WCHAR));
    pTokenPair[0][pszColon - buffer] = '\0';

    /* skip colon */
    pszColon++;
    len = strlenW(pszColon);
    pTokenPair[1] = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!pTokenPair[1])
    {
        HTTP_FreeTokens(pTokenPair);
        return NULL;
    }
    memcpy(pTokenPair[1], pszColon, (len + 1) * sizeof(WCHAR));

    strip_spaces(pTokenPair[0]);
    strip_spaces(pTokenPair[1]);

    TRACE("field(%s) Value(%s)\n", debugstr_w(pTokenPair[0]), debugstr_w(pTokenPair[1]));
    return pTokenPair;
}

/***********************************************************************
 *           HTTP_ProcessHeader (internal)
 *
 * Stuff header into header tables according to <dwModifier>
 *
 */

#define COALESCEFLAGS (HTTP_ADDHDR_FLAG_COALESCE|HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA|HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON)

static DWORD HTTP_ProcessHeader(http_request_t *lpwhr, LPCWSTR field, LPCWSTR value, DWORD dwModifier)
{
    LPHTTPHEADERW lphttpHdr = NULL;
    INT index = -1;
    BOOL request_only = dwModifier & HTTP_ADDHDR_FLAG_REQ;
    DWORD res = ERROR_HTTP_INVALID_HEADER;

    TRACE("--> %s: %s - 0x%08x\n", debugstr_w(field), debugstr_w(value), dwModifier);

    /* REPLACE wins out over ADD */
    if (dwModifier & HTTP_ADDHDR_FLAG_REPLACE)
        dwModifier &= ~HTTP_ADDHDR_FLAG_ADD;
    
    if (dwModifier & HTTP_ADDHDR_FLAG_ADD)
        index = -1;
    else
        index = HTTP_GetCustomHeaderIndex(lpwhr, field, 0, request_only);

    if (index >= 0)
    {
        if (dwModifier & HTTP_ADDHDR_FLAG_ADD_IF_NEW)
            return ERROR_HTTP_INVALID_HEADER;
        lphttpHdr = &lpwhr->pCustHeaders[index];
    }
    else if (value)
    {
        HTTPHEADERW hdr;

        hdr.lpszField = (LPWSTR)field;
        hdr.lpszValue = (LPWSTR)value;
        hdr.wFlags = hdr.wCount = 0;

        if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
            hdr.wFlags |= HDR_ISREQUEST;

        return HTTP_InsertCustomHeader(lpwhr, &hdr);
    }
    /* no value to delete */
    else return ERROR_SUCCESS;

    if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
	    lphttpHdr->wFlags |= HDR_ISREQUEST;
    else
        lphttpHdr->wFlags &= ~HDR_ISREQUEST;

    if (dwModifier & HTTP_ADDHDR_FLAG_REPLACE)
    {
        HTTP_DeleteCustomHeader( lpwhr, index );

        if (value)
        {
            HTTPHEADERW hdr;

            hdr.lpszField = (LPWSTR)field;
            hdr.lpszValue = (LPWSTR)value;
            hdr.wFlags = hdr.wCount = 0;

            if (dwModifier & HTTP_ADDHDR_FLAG_REQ)
                hdr.wFlags |= HDR_ISREQUEST;

            return HTTP_InsertCustomHeader(lpwhr, &hdr);
        }

        return ERROR_SUCCESS;
    }
    else if (dwModifier & COALESCEFLAGS)
    {
        LPWSTR lpsztmp;
        WCHAR ch = 0;
        INT len = 0;
        INT origlen = strlenW(lphttpHdr->lpszValue);
        INT valuelen = strlenW(value);

        if (dwModifier & HTTP_ADDHDR_FLAG_COALESCE_WITH_COMMA)
        {
            ch = ',';
            lphttpHdr->wFlags |= HDR_COMMADELIMITED;
        }
        else if (dwModifier & HTTP_ADDHDR_FLAG_COALESCE_WITH_SEMICOLON)
        {
            ch = ';';
            lphttpHdr->wFlags |= HDR_COMMADELIMITED;
        }

        len = origlen + valuelen + ((ch > 0) ? 2 : 0);

        lpsztmp = HeapReAlloc(GetProcessHeap(), 0, lphttpHdr->lpszValue, (len+1)*sizeof(WCHAR));
        if (lpsztmp)
        {
            lphttpHdr->lpszValue = lpsztmp;
    /* FIXME: Increment lphttpHdr->wCount. Perhaps lpszValue should be an array */
            if (ch > 0)
            {
                lphttpHdr->lpszValue[origlen] = ch;
                origlen++;
                lphttpHdr->lpszValue[origlen] = ' ';
                origlen++;
            }

            memcpy(&lphttpHdr->lpszValue[origlen], value, valuelen*sizeof(WCHAR));
            lphttpHdr->lpszValue[len] = '\0';
            res = ERROR_SUCCESS;
        }
        else
        {
            WARN("HeapReAlloc (%d bytes) failed\n",len+1);
            res = ERROR_OUTOFMEMORY;
        }
    }
    TRACE("<-- %d\n", res);
    return res;
}


/***********************************************************************
 *           HTTP_FinishedReading (internal)
 *
 * Called when all content from server has been read by client.
 *
 */
static BOOL HTTP_FinishedReading(http_request_t *lpwhr)
{
    BOOL keepalive = HTTP_KeepAlive(lpwhr);

    TRACE("\n");


    if (!keepalive)
    {
        HTTPREQ_CloseConnection(&lpwhr->hdr);
    }

    /* FIXME: store data in the URL cache here */

    return TRUE;
}


/***********************************************************************
 *           HTTP_GetCustomHeaderIndex (internal)
 *
 * Return index of custom header from header array
 *
 */
static INT HTTP_GetCustomHeaderIndex(http_request_t *lpwhr, LPCWSTR lpszField,
                                     int requested_index, BOOL request_only)
{
    DWORD index;

    TRACE("%s, %d, %d\n", debugstr_w(lpszField), requested_index, request_only);

    for (index = 0; index < lpwhr->nCustHeaders; index++)
    {
        if (strcmpiW(lpwhr->pCustHeaders[index].lpszField, lpszField))
            continue;

        if (request_only && !(lpwhr->pCustHeaders[index].wFlags & HDR_ISREQUEST))
            continue;

        if (!request_only && (lpwhr->pCustHeaders[index].wFlags & HDR_ISREQUEST))
            continue;

        if (requested_index == 0)
            break;
        requested_index --;
    }

    if (index >= lpwhr->nCustHeaders)
	index = -1;

    TRACE("Return: %d\n", index);
    return index;
}


/***********************************************************************
 *           HTTP_InsertCustomHeader (internal)
 *
 * Insert header into array
 *
 */
static DWORD HTTP_InsertCustomHeader(http_request_t *lpwhr, LPHTTPHEADERW lpHdr)
{
    INT count;
    LPHTTPHEADERW lph = NULL;

    TRACE("--> %s: %s\n", debugstr_w(lpHdr->lpszField), debugstr_w(lpHdr->lpszValue));
    count = lpwhr->nCustHeaders + 1;
    if (count > 1)
	lph = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, lpwhr->pCustHeaders, sizeof(HTTPHEADERW) * count);
    else
	lph = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HTTPHEADERW) * count);

    if (!lph)
        return ERROR_OUTOFMEMORY;

    lpwhr->pCustHeaders = lph;
    lpwhr->pCustHeaders[count-1].lpszField = heap_strdupW(lpHdr->lpszField);
    lpwhr->pCustHeaders[count-1].lpszValue = heap_strdupW(lpHdr->lpszValue);
    lpwhr->pCustHeaders[count-1].wFlags = lpHdr->wFlags;
    lpwhr->pCustHeaders[count-1].wCount= lpHdr->wCount;
    lpwhr->nCustHeaders++;

    return ERROR_SUCCESS;
}


/***********************************************************************
 *           HTTP_DeleteCustomHeader (internal)
 *
 * Delete header from array
 *  If this function is called, the indexs may change.
 */
static BOOL HTTP_DeleteCustomHeader(http_request_t *lpwhr, DWORD index)
{
    if( lpwhr->nCustHeaders <= 0 )
        return FALSE;
    if( index >= lpwhr->nCustHeaders )
        return FALSE;
    lpwhr->nCustHeaders--;

    HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders[index].lpszField);
    HeapFree(GetProcessHeap(), 0, lpwhr->pCustHeaders[index].lpszValue);

    memmove( &lpwhr->pCustHeaders[index], &lpwhr->pCustHeaders[index+1],
             (lpwhr->nCustHeaders - index)* sizeof(HTTPHEADERW) );
    memset( &lpwhr->pCustHeaders[lpwhr->nCustHeaders], 0, sizeof(HTTPHEADERW) );

    return TRUE;
}


/***********************************************************************
 *           HTTP_VerifyValidHeader (internal)
 *
 * Verify the given header is not invalid for the given http request
 *
 */
static BOOL HTTP_VerifyValidHeader(http_request_t *lpwhr, LPCWSTR field)
{
    /* Accept-Encoding is stripped from HTTP/1.0 requests. It is invalid */
    if (!strcmpW(lpwhr->lpszVersion, g_szHttp1_0) && !strcmpiW(field, szAccept_Encoding))
        return ERROR_HTTP_INVALID_HEADER;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *          IsHostInProxyBypassList (@)
 *
 * Undocumented
 *
 */
BOOL WINAPI IsHostInProxyBypassList(DWORD flags, LPCSTR szHost, DWORD length)
{
   FIXME("STUB: flags=%d host=%s length=%d\n",flags,szHost,length);
   return FALSE;
}
