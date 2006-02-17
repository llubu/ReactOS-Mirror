/*
 * Wininet - networking layer. Uses unix sockets or OpenSSL.
 *
 * Copyright 2002 TransGaming Technologies Inc.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "wine/library.h"
#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

/* To avoid conflicts with the Unix socket headers. we only need it for
 * the error codes anyway. */
#define USE_WS_PREFIX
#include "winsock2.h"

#include "wine/debug.h"
#include "internet.h"

#define RESPONSE_TIMEOUT        30            /* FROM internet.c */


WINE_DEFAULT_DEBUG_CHANNEL(wininet);

/* FIXME!!!!!!
 *    This should use winsock - To use winsock the functions will have to change a bit
 *        as they are designed for unix sockets.
 *    SSL stuff should use crypt32.dll
 */

#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H

#include <openssl/err.h>

#ifndef SONAME_LIBSSL
#define SONAME_LIBSSL "libssl.so"
#endif
#ifndef SONAME_LIBCRYPTO
#define SONAME_LIBCRYPTO "libcrypto.so"
#endif

static void *OpenSSL_ssl_handle;
static void *OpenSSL_crypto_handle;

static SSL_METHOD *meth;
static SSL_CTX *ctx;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

/* OpenSSL functions that we use */
MAKE_FUNCPTR(SSL_library_init);
MAKE_FUNCPTR(SSL_load_error_strings);
MAKE_FUNCPTR(SSLv23_method);
MAKE_FUNCPTR(SSL_CTX_new);
MAKE_FUNCPTR(SSL_new);
MAKE_FUNCPTR(SSL_free);
MAKE_FUNCPTR(SSL_set_fd);
MAKE_FUNCPTR(SSL_connect);
MAKE_FUNCPTR(SSL_shutdown);
MAKE_FUNCPTR(SSL_write);
MAKE_FUNCPTR(SSL_read);
MAKE_FUNCPTR(SSL_get_verify_result);
MAKE_FUNCPTR(SSL_get_peer_certificate);
MAKE_FUNCPTR(SSL_CTX_get_timeout);
MAKE_FUNCPTR(SSL_CTX_set_timeout);
MAKE_FUNCPTR(SSL_CTX_set_default_verify_paths);

/* OpenSSL's libcrypto functions that we use */
MAKE_FUNCPTR(BIO_new_fp);
MAKE_FUNCPTR(ERR_get_error);
MAKE_FUNCPTR(ERR_error_string);
#undef MAKE_FUNCPTR

#endif

void NETCON_init(WININET_NETCONNECTION *connection, BOOL useSSL)
{
    connection->useSSL = FALSE;
    connection->socketFD = -1;
    if (useSSL)
    {
#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H
        TRACE("using SSL connection\n");
	if (OpenSSL_ssl_handle) /* already initilzed everything */
            return;
	OpenSSL_ssl_handle = wine_dlopen(SONAME_LIBSSL, RTLD_NOW, NULL, 0);
	if (!OpenSSL_ssl_handle)
	{
	    ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n",
		SONAME_LIBSSL);
            connection->useSSL = FALSE;
            return;
	}
	OpenSSL_crypto_handle = wine_dlopen(SONAME_LIBCRYPTO, RTLD_NOW, NULL, 0);
	if (!OpenSSL_crypto_handle)
	{
	    ERR("trying to use a SSL connection, but couldn't load %s. Expect trouble.\n",
		SONAME_LIBCRYPTO);
            connection->useSSL = FALSE;
            return;
	}

        /* mmm nice ugly macroness */
#define DYNSSL(x) \
    p##x = wine_dlsym(OpenSSL_ssl_handle, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        connection->useSSL = FALSE; \
        return; \
    }

	DYNSSL(SSL_library_init);
	DYNSSL(SSL_load_error_strings);
	DYNSSL(SSLv23_method);
	DYNSSL(SSL_CTX_new);
	DYNSSL(SSL_new);
	DYNSSL(SSL_free);
	DYNSSL(SSL_set_fd);
	DYNSSL(SSL_connect);
	DYNSSL(SSL_shutdown);
	DYNSSL(SSL_write);
	DYNSSL(SSL_read);
	DYNSSL(SSL_get_verify_result);
	DYNSSL(SSL_get_peer_certificate);
	DYNSSL(SSL_CTX_get_timeout);
	DYNSSL(SSL_CTX_set_timeout);
	DYNSSL(SSL_CTX_set_default_verify_paths);
#undef DYNSSL

#define DYNCRYPTO(x) \
    p##x = wine_dlsym(OpenSSL_crypto_handle, #x, NULL, 0); \
    if (!p##x) \
    { \
        ERR("failed to load symbol %s\n", #x); \
        connection->useSSL = FALSE; \
        return; \
    }
	DYNCRYPTO(BIO_new_fp);
	DYNCRYPTO(ERR_get_error);
	DYNCRYPTO(ERR_error_string);
#undef DYNCRYPTO

	pSSL_library_init();
	pSSL_load_error_strings();
	pBIO_new_fp(stderr, BIO_NOCLOSE); /* FIXME: should use winedebug stuff */

	meth = pSSLv23_method();
        connection->peek_msg = NULL;
        connection->peek_msg_mem = NULL;
#else
	FIXME("can't use SSL, not compiled in.\n");
        connection->useSSL = FALSE;
#endif
    }
}

BOOL NETCON_connected(WININET_NETCONNECTION *connection)
{
    if (connection->socketFD == -1)
        return FALSE;
    else
        return TRUE;
}

/******************************************************************************
 * NETCON_create
 * Basically calls 'socket()'
 */
BOOL NETCON_create(WININET_NETCONNECTION *connection, int domain,
	      int type, int protocol)
{
#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H
    if (connection->useSSL)
        return FALSE;
#endif

    connection->socketFD = socket(domain, type, protocol);
    if (INVALID_SOCKET == connection->socketFD)
    {
        INTERNET_SetLastError(WSAGetLastError());
        return FALSE;
    }
    return TRUE;
}

/******************************************************************************
 * NETCON_close
 * Basically calls 'close()' unless we should use SSL
 */
BOOL NETCON_close(WININET_NETCONNECTION *connection)
{
    int result;

    if (!NETCON_connected(connection)) return FALSE;

#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H
    if (connection->useSSL)
    {
        HeapFree(GetProcessHeap(),0,connection->peek_msg_mem);
        connection->peek_msg = NULL;
        connection->peek_msg_mem = NULL;

        pSSL_shutdown(connection->ssl_s);
        pSSL_free(connection->ssl_s);
        connection->ssl_s = NULL;

        connection->useSSL = FALSE;
    }
#endif

    result = closesocket(connection->socketFD);
    connection->socketFD = -1;

    if (0 != result)
    {
        INTERNET_SetLastError(WSAGetLastError());
        return FALSE;
    }
    return TRUE;
}
#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H
static BOOL check_hostname(X509 *cert, char *hostname)
{
    /* FIXME: implement */
    return TRUE;
}
#endif
/******************************************************************************
 * NETCON_secure_connect
 * Initiates a secure connection over an existing plaintext connection.
 */
BOOL NETCON_secure_connect(WININET_NETCONNECTION *connection, LPCWSTR hostname)
{
#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H
    long verify_res;
    X509 *cert;
    int len;
    char *hostname_unix;

    /* can't connect if we are already connected */
    if (connection->useSSL)
    {
        ERR("already connected\n");
        return FALSE;
    }

    ctx = pSSL_CTX_new(meth);
    if (!pSSL_CTX_set_default_verify_paths(ctx))
    {
        ERR("SSL_CTX_set_default_verify_paths failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        return FALSE;
    }
    connection->ssl_s = pSSL_new(ctx);
    if (!connection->ssl_s)
    {
        ERR("SSL_new failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        goto fail;
    }

    if (!pSSL_set_fd(connection->ssl_s, connection->socketFD))
    {
        ERR("SSL_set_fd failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        goto fail;
    }

    if (pSSL_connect(connection->ssl_s) <= 0)
    {
        ERR("SSL_connect failed: %s\n",
            pERR_error_string(pERR_get_error(), 0));
        INTERNET_SetLastError(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
        goto fail;
    }
    cert = pSSL_get_peer_certificate(connection->ssl_s);
    if (!cert)
    {
        ERR("no certificate for server %s\n", debugstr_w(hostname));
        /* FIXME: is this the best error? */
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_CA);
        goto fail;
    }
    verify_res = pSSL_get_verify_result(connection->ssl_s);
    if (verify_res != X509_V_OK)
    {
        ERR("couldn't verify the security of the connection, %ld\n", verify_res);
        /* FIXME: we should set an error and return, but we only warn at
         * the moment */
    }

    len = WideCharToMultiByte(CP_THREAD_ACP, 0, hostname, -1, NULL, 0, NULL, NULL);
    hostname_unix = HeapAlloc(GetProcessHeap(), 0, len);
    if (!hostname_unix)
    {
        INTERNET_SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto fail;
    }
    WideCharToMultiByte(CP_THREAD_ACP, 0, hostname, -1, hostname_unix, len, NULL, NULL);

    if (!check_hostname(cert, hostname_unix))
    {
        HeapFree(GetProcessHeap(), 0, hostname_unix);
        INTERNET_SetLastError(ERROR_INTERNET_SEC_CERT_CN_INVALID);
        goto fail;
    }

    HeapFree(GetProcessHeap(), 0, hostname_unix);
    connection->useSSL = TRUE;
    return TRUE;

fail:
    if (connection->ssl_s)
    {
        pSSL_shutdown(connection->ssl_s);
        pSSL_free(connection->ssl_s);
        connection->ssl_s = NULL;
    }
#endif
    return FALSE;
}

/******************************************************************************
 * NETCON_connect
 * Connects to the specified address.
 */
BOOL NETCON_connect(WININET_NETCONNECTION *connection, const struct sockaddr *serv_addr,
		    unsigned int addrlen)
{
    int result;

    if (!NETCON_connected(connection)) return FALSE;

    result = connect(connection->socketFD, serv_addr, addrlen);
    if (SOCKET_ERROR == result)
    {
        WARN("Unable to connect to host (%s)\n", strerror(errno));
        INTERNET_SetLastError(WSAGetLastError());

        closesocket(connection->socketFD);
        connection->socketFD = -1;
        return FALSE;
    }

    return TRUE;
}

/******************************************************************************
 * NETCON_send
 * Basically calls 'send()' unless we should use SSL
 * number of chars send is put in *sent
 */
BOOL NETCON_send(WININET_NETCONNECTION *connection, const void *msg, size_t len, int flags,
		int *sent /* out */)
{
    if (!NETCON_connected(connection)) return FALSE;
    if (!connection->useSSL)
    {
	*sent = send(connection->socketFD, msg, len, flags);
	if (SOCKET_ERROR == *sent)
	{
	    INTERNET_SetLastError(WSAGetLastError());
	    return FALSE;
	}
        return TRUE;
    }
    else
    {
#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H
	if (flags)
            FIXME("SSL_write doesn't support any flags (%08x)\n", flags);
	*sent = pSSL_write(connection->ssl_s, msg, len);
	if (*sent < 1 && len)
	    return FALSE;
        return TRUE;
#else
	return FALSE;
#endif
    }
}

/******************************************************************************
 * NETCON_recv
 * Basically calls 'recv()' unless we should use SSL
 * number of chars received is put in *recvd
 */
BOOL NETCON_recv(WININET_NETCONNECTION *connection, void *buf, size_t len, int flags,
		int *recvd /* out */)
{
    if (!NETCON_connected(connection)) return FALSE;
    if (!connection->useSSL)
    {
	*recvd = recv(connection->socketFD, buf, len, flags);
	if (SOCKET_ERROR == *recvd)
	{
	    INTERNET_SetLastError(WSAGetLastError());
	    return FALSE;
	}
        return TRUE;
    }
    else
    {
#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H
	if (flags & (~MSG_PEEK))
	    FIXME("SSL_read does not support the following flag: %08x\n", flags);

        /* this ugly hack is all for MSG_PEEK. eww gross */
	if (flags & MSG_PEEK && !connection->peek_msg)
	{
	    connection->peek_msg = connection->peek_msg_mem = HeapAlloc(GetProcessHeap(), 0, (sizeof(char) * len) + 1);
	}
	else if (flags & MSG_PEEK && connection->peek_msg)
	{
	    size_t peek_msg_len = strlen(connection->peek_msg);
	    if (len < peek_msg_len)
		FIXME("buffer isn't big enough. Do the expect us to wrap?\n");
	    memcpy(buf, connection->peek_msg, min(len,peek_msg_len+1));
	    *recvd = min(len, peek_msg_len);
            return TRUE;
	}
	else if (connection->peek_msg)
	{
	    size_t peek_msg_len = strlen(connection->peek_msg);
	    memcpy(buf, connection->peek_msg, min(len,peek_msg_len+1));
	    connection->peek_msg += *recvd = min(len, peek_msg_len);
	    if (*connection->peek_msg == '\0' || *(connection->peek_msg - 1) == '\0')
	    {
		HeapFree(GetProcessHeap(), 0, connection->peek_msg_mem);
		connection->peek_msg_mem = NULL;
                connection->peek_msg = NULL;
	    }
            return TRUE;
	}
	*recvd = pSSL_read(connection->ssl_s, buf, len);
	if (flags & MSG_PEEK) /* must copy stuff into buffer */
	{
	    if (!*recvd)
	    {
		HeapFree(GetProcessHeap(), 0, connection->peek_msg_mem);
		connection->peek_msg_mem = NULL;
		connection->peek_msg = NULL;
	    }
	    else
	    {
		memcpy(connection->peek_msg, buf, *recvd);
		connection->peek_msg[*recvd] = '\0';
	    }
	}
	if (*recvd < 1 && len)
            return FALSE;
        return TRUE;
#else
	return FALSE;
#endif
    }
}

/******************************************************************************
 * NETCON_getNextLine
 */
BOOL NETCON_getNextLine(WININET_NETCONNECTION *connection, LPSTR lpszBuffer, LPDWORD dwBuffer)
{

    TRACE("\n");

    if (!NETCON_connected(connection)) return FALSE;

    if (!connection->useSSL)
    {
	struct timeval tv;
	fd_set infd;
	BOOL bSuccess = FALSE;
	DWORD nRecv = 0;
        int r;

	FD_ZERO(&infd);
	FD_SET(connection->socketFD, &infd);
	tv.tv_sec=RESPONSE_TIMEOUT;
	tv.tv_usec=0;

	while (nRecv < *dwBuffer)
	{
	    if (select(connection->socketFD+1,&infd,NULL,NULL,&tv) > 0)
	    {
		r = recv(connection->socketFD, &lpszBuffer[nRecv], 1, 0);
		if (0 == r || SOCKET_ERROR == r)
		{
		    INTERNET_SetLastError(WSAGetLastError());
		    goto lend;
		}

		if (lpszBuffer[nRecv] == '\n')
		{
		    bSuccess = TRUE;
		    break;
		}
		if (lpszBuffer[nRecv] != '\r')
		    nRecv++;
	    }
	    else
	    {
		INTERNET_SetLastError(ERROR_INTERNET_TIMEOUT);
		goto lend;
	    }
	}

    lend:             /* FIXME: don't use labels */
	if (bSuccess)
	{
	    lpszBuffer[nRecv++] = '\0';
	    *dwBuffer = nRecv;
	    TRACE(":%lu %s\n", nRecv, lpszBuffer);
            return TRUE;
	}
	else
	{
	    return FALSE;
	}
    }
    else
    {
#if defined HAVE_OPENSSL_SSL_H && defined HAVE_OPENSSL_ERR_H
	long prev_timeout;
	DWORD nRecv = 0;
        BOOL success = TRUE;

        prev_timeout = pSSL_CTX_get_timeout(ctx);
	pSSL_CTX_set_timeout(ctx, RESPONSE_TIMEOUT);

	while (nRecv < *dwBuffer)
	{
	    int recv = 1;
	    if (!NETCON_recv(connection, &lpszBuffer[nRecv], 1, 0, &recv))
	    {
                INTERNET_SetLastError(ERROR_CONNECTION_ABORTED);
		success = FALSE;
	    }

	    if (lpszBuffer[nRecv] == '\n')
	    {
		success = TRUE;
                break;
	    }
	    if (lpszBuffer[nRecv] != '\r')
		nRecv++;
	}

        pSSL_CTX_set_timeout(ctx, prev_timeout);
	if (success)
	{
	    lpszBuffer[nRecv++] = '\0';
	    *dwBuffer = nRecv;
	    TRACE("_SSL:%lu %s\n", nRecv, lpszBuffer);
            return TRUE;
	}
        return FALSE;
#else
	return FALSE;
#endif
    }
}
