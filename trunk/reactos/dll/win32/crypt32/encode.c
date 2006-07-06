/*
 * Copyright 2005 Juan Lang
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
 *
 * This file implements ASN.1 DER encoding of a limited set of types.
 * It isn't a full ASN.1 implementation.  Microsoft implements BER
 * encoding of many of the basic types in msasn1.dll, but that interface is
 * undocumented, so I implement them here.
 *
 * References:
 * "A Layman's Guide to a Subset of ASN.1, BER, and DER", by Burton Kaliski
 * (available online, look for a PDF copy as the HTML versions tend to have
 * translation errors.)
 *
 * RFC3280, http://www.faqs.org/rfcs/rfc3280.html
 *
 * MSDN, especially:
 * http://msdn.microsoft.com/library/en-us/seccrypto/security/constants_for_cryptencodeobject_and_cryptdecodeobject.asp
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "excpt.h"
#include "wincrypt.h"
#include "winreg.h"
#include "snmp.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "crypt32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypt);

typedef BOOL (WINAPI *CryptEncodeObjectFunc)(DWORD, LPCSTR, const void *,
 BYTE *, DWORD *);
typedef BOOL (WINAPI *CryptEncodeObjectExFunc)(DWORD, LPCSTR, const void *,
 DWORD, PCRYPT_ENCODE_PARA, BYTE *, DWORD *);

/* Prototypes for built-in encoders.  They follow the Ex style prototypes.
 * The dwCertEncodingType and lpszStructType are ignored by the built-in
 * functions, but the parameters are retained to simplify CryptEncodeObjectEx,
 * since it must call functions in external DLLs that follow these signatures.
 */
static BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeSequenceOfAny(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeBool(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeBitsSwapBytes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeUnsignedInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);
static BOOL WINAPI CRYPT_AsnEncodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded);

BOOL WINAPI CryptEncodeObject(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret = FALSE;
    HCRYPTOIDFUNCADDR hFunc;
    CryptEncodeObjectFunc pCryptEncodeObject;

    TRACE("(0x%08lx, %s, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, pbEncoded,
     pcbEncoded);

    if (!pbEncoded && !pcbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Try registered DLL first.. */
    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_ENCODE_OBJECT_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
     (void **)&pCryptEncodeObject, &hFunc);
    if (pCryptEncodeObject)
    {
        ret = pCryptEncodeObject(dwCertEncodingType, lpszStructType,
         pvStructInfo, pbEncoded, pcbEncoded);
        CryptFreeOIDFunctionAddress(hFunc, 0);
    }
    else
    {
        /* If not, use CryptEncodeObjectEx */
        ret = CryptEncodeObjectEx(dwCertEncodingType, lpszStructType,
         pvStructInfo, 0, NULL, pbEncoded, pcbEncoded);
    }
    return ret;
}

/* Helper function to check *pcbEncoded, set it to the required size, and
 * optionally to allocate memory.  Assumes pbEncoded is not NULL.
 * If CRYPT_ENCODE_ALLOC_FLAG is set in dwFlags, *pbEncoded will be set to a
 * pointer to the newly allocated memory.
 */
static BOOL CRYPT_EncodeEnsureSpace(DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded,
 DWORD bytesNeeded)
{
    BOOL ret = TRUE;

    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
    {
        if (pEncodePara && pEncodePara->pfnAlloc)
            *(BYTE **)pbEncoded = pEncodePara->pfnAlloc(bytesNeeded);
        else
            *(BYTE **)pbEncoded = LocalAlloc(0, bytesNeeded);
        if (!*(BYTE **)pbEncoded)
            ret = FALSE;
        else
            *pcbEncoded = bytesNeeded;
    }
    else if (bytesNeeded > *pcbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    return ret;
}

static BOOL CRYPT_EncodeLen(DWORD len, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD bytesNeeded, significantBytes = 0;

    if (len <= 0x7f)
        bytesNeeded = 1;
    else
    {
        DWORD temp;

        for (temp = len, significantBytes = sizeof(temp); !(temp & 0xff000000);
         temp <<= 8, significantBytes--)
            ;
        bytesNeeded = significantBytes + 1;
    }
    if (!pbEncoded)
    {
        *pcbEncoded = bytesNeeded;
        return TRUE;
    }
    if (*pcbEncoded < bytesNeeded)
    {
        SetLastError(ERROR_MORE_DATA);
        return FALSE;
    }
    if (len <= 0x7f)
        *pbEncoded = (BYTE)len;
    else
    {
        DWORD i;

        *pbEncoded++ = significantBytes | 0x80;
        for (i = 0; i < significantBytes; i++)
        {
            *(pbEncoded + significantBytes - i - 1) = (BYTE)(len & 0xff);
            len >>= 8;
        }
    }
    *pcbEncoded = bytesNeeded;
    return TRUE;
}

struct AsnEncodeSequenceItem
{
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
    DWORD                   size; /* used during encoding, not for your use */
};

static BOOL WINAPI CRYPT_AsnEncodeSequence(DWORD dwCertEncodingType,
 struct AsnEncodeSequenceItem items[], DWORD cItem, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD i, dataLen = 0;

    TRACE("%p, %ld, %08lx, %p, %p, %ld\n", items, cItem, dwFlags, pEncodePara,
     pbEncoded, *pcbEncoded);
    for (i = 0, ret = TRUE; ret && i < cItem; i++)
    {
        ret = items[i].encodeFunc(dwCertEncodingType, NULL,
         items[i].pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL,
         NULL, &items[i].size);
        /* Some functions propagate their errors through the size */
        if (!ret)
            *pcbEncoded = items[i].size;
        dataLen += items[i].size;
    }
    if (ret)
    {
        DWORD lenBytes, bytesNeeded;

        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_SEQUENCE;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                for (i = 0; ret && i < cItem; i++)
                {
                    ret = items[i].encodeFunc(dwCertEncodingType, NULL,
                     items[i].pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG,
                     NULL, pbEncoded, &items[i].size);
                    /* Some functions propagate their errors through the size */
                    if (!ret)
                        *pcbEncoded = items[i].size;
                    pbEncoded += items[i].size;
                }
            }
        }
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

struct AsnConstructedItem
{
    BYTE                    tag;
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
};

static BOOL WINAPI CRYPT_AsnEncodeConstructed(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    const struct AsnConstructedItem *item =
     (const struct AsnConstructedItem *)pvStructInfo;
    DWORD len;

    if ((ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
     item->pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, NULL, &len)))
    {
        DWORD dataLen, bytesNeeded;

        CRYPT_EncodeLen(len, NULL, &dataLen);
        bytesNeeded = 1 + dataLen + len;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
         pbEncoded, pcbEncoded, bytesNeeded)))
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = ASN_CONTEXT | ASN_CONSTRUCTOR | item->tag;
            CRYPT_EncodeLen(len, pbEncoded, &dataLen);
            pbEncoded += dataLen;
            ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
             item->pvStructInfo, dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL,
             pbEncoded, &len);
            if (!ret)
            {
                /* Some functions propagate their errors through the size */
                *pcbEncoded = len;
            }
        }
    }
    else
    {
        /* Some functions propagate their errors through the size */
        *pcbEncoded = len;
    }
    return ret;
}

struct AsnEncodeTagSwappedItem
{
    BYTE                    tag;
    const void             *pvStructInfo;
    CryptEncodeObjectExFunc encodeFunc;
};

/* Sort of a wacky hack, it encodes something using the struct
 * AsnEncodeTagSwappedItem's encodeFunc, then replaces the tag byte with the tag
 * given in the struct AsnEncodeTagSwappedItem.
 */
static BOOL WINAPI CRYPT_AsnEncodeSwapTag(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    const struct AsnEncodeTagSwappedItem *item =
     (const struct AsnEncodeTagSwappedItem *)pvStructInfo;

    ret = item->encodeFunc(dwCertEncodingType, lpszStructType,
     item->pvStructInfo, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    if (ret && pbEncoded)
        *pbEncoded = item->tag;
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCertVersion(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const DWORD *ver = (const DWORD *)pvStructInfo;
    BOOL ret;

    /* CERT_V1 is not encoded */
    if (*ver == CERT_V1)
    {
        *pcbEncoded = 0;
        ret = TRUE;
    }
    else
    {
        struct AsnConstructedItem item = { 0, ver, CRYPT_AsnEncodeInt };

        ret = CRYPT_AsnEncodeConstructed(dwCertEncodingType, X509_INTEGER,
         &item, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    return ret;
}

static BOOL WINAPI CRYPT_CopyEncodedBlob(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const CRYPT_DER_BLOB *blob = (const CRYPT_DER_BLOB *)pvStructInfo;
    BOOL ret;

    if (!pbEncoded)
    {
        *pcbEncoded = blob->cbData;
        ret = TRUE;
    }
    else if (*pcbEncoded < blob->cbData)
    {
        *pcbEncoded = blob->cbData;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        if (blob->cbData)
            memcpy(pbEncoded, blob->pbData, blob->cbData);
        *pcbEncoded = blob->cbData;
        ret = TRUE;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeValidity(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    /* This has two filetimes in a row, a NotBefore and a NotAfter */
    const FILETIME *timePtr = (const FILETIME *)pvStructInfo;
    struct AsnEncodeSequenceItem items[] = {
     { timePtr++, CRYPT_AsnEncodeChoiceOfTime, 0 },
     { timePtr,   CRYPT_AsnEncodeChoiceOfTime, 0 },
    };

    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, 
     sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAlgorithmId(
 DWORD dwCertEncodingType, LPCSTR lpszStructType, const void *pvStructInfo,
 DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    const CRYPT_ALGORITHM_IDENTIFIER *algo =
     (const CRYPT_ALGORITHM_IDENTIFIER *)pvStructInfo;
    BOOL ret;
    struct AsnEncodeSequenceItem items[] = {
     { algo->pszObjId,    CRYPT_AsnEncodeOid, 0 },
     { &algo->Parameters, CRYPT_CopyEncodedBlob, 0 },
    };

    ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
     sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
     pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodePubKeyInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_PUBLIC_KEY_INFO *info =
         (const CERT_PUBLIC_KEY_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &info->Algorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->PublicKey, CRYPT_AsnEncodeBits, 0 },
        };

        TRACE("Encoding public key with OID %s\n",
         debugstr_a(info->Algorithm.pszObjId));
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
         sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
         pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCert(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_SIGNED_CONTENT_INFO *info =
         (const CERT_SIGNED_CONTENT_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[] = {
         { &info->ToBeSigned,         CRYPT_CopyEncodedBlob, 0 },
         { &info->SignatureAlgorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Signature,          CRYPT_AsnEncodeBitsSwapBytes, 0 },
        };

        if (dwFlags & CRYPT_ENCODE_NO_SIGNATURE_BYTE_REVERSAL_FLAG)
            items[2].encodeFunc = CRYPT_AsnEncodeBits;
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, 
         sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
         pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

/* Like in Windows, this blithely ignores the validity of the passed-in
 * CERT_INFO, and just encodes it as-is.  The resulting encoded data may not
 * decode properly, see CRYPT_AsnDecodeCertInfo.
 */
static BOOL WINAPI CRYPT_AsnEncodeCertInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_INFO *info = (const CERT_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[10] = {
         { &info->dwVersion,            CRYPT_AsnEncodeCertVersion, 0 },
         { &info->SerialNumber,         CRYPT_AsnEncodeInteger, 0 },
         { &info->SignatureAlgorithm,   CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Issuer,               CRYPT_CopyEncodedBlob, 0 },
         { &info->NotBefore,            CRYPT_AsnEncodeValidity, 0 },
         { &info->Subject,              CRYPT_CopyEncodedBlob, 0 },
         { &info->SubjectPublicKeyInfo, CRYPT_AsnEncodePubKeyInfo, 0 },
         { 0 }
        };
        struct AsnConstructedItem constructed[3] = { { 0 } };
        DWORD cItem = 7, cConstructed = 0;

        if (info->IssuerUniqueId.cbData)
        {
            constructed[cConstructed].tag = 1;
            constructed[cConstructed].pvStructInfo = &info->IssuerUniqueId;
            constructed[cConstructed].encodeFunc = CRYPT_AsnEncodeBits;
            items[cItem].pvStructInfo = &constructed[cConstructed];
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cConstructed++;
            cItem++;
        }
        if (info->SubjectUniqueId.cbData)
        {
            constructed[cConstructed].tag = 2;
            constructed[cConstructed].pvStructInfo = &info->SubjectUniqueId;
            constructed[cConstructed].encodeFunc = CRYPT_AsnEncodeBits;
            items[cItem].pvStructInfo = &constructed[cConstructed];
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cConstructed++;
            cItem++;
        }
        if (info->cExtension)
        {
            constructed[cConstructed].tag = 3;
            constructed[cConstructed].pvStructInfo = &info->cExtension;
            constructed[cConstructed].encodeFunc = CRYPT_AsnEncodeExtensions;
            items[cItem].pvStructInfo = &constructed[cConstructed];
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cConstructed++;
            cItem++;
        }

        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCRLEntry(const CRL_ENTRY *entry,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    struct AsnEncodeSequenceItem items[3] = {
     { &entry->SerialNumber,   CRYPT_AsnEncodeInteger, 0 },
     { &entry->RevocationDate, CRYPT_AsnEncodeChoiceOfTime, 0 },
     { 0 }
    };
    DWORD cItem = 2;
    BOOL ret;

    TRACE("%p, %p, %p\n", entry, pbEncoded, pcbEncoded);

    if (entry->cExtension)
    {
        items[cItem].pvStructInfo = &entry->cExtension;
        items[cItem].encodeFunc = CRYPT_AsnEncodeExtensions;
        cItem++;
    }

    ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem, 0, NULL,
     pbEncoded, pcbEncoded);

    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCRLEntries(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD cCRLEntry = *(const DWORD *)pvStructInfo;
    DWORD bytesNeeded, dataLen, lenBytes, i;
    const CRL_ENTRY *rgCRLEntry = *(const CRL_ENTRY **)
     ((const BYTE *)pvStructInfo + sizeof(DWORD));
    BOOL ret = TRUE;

    for (i = 0, dataLen = 0; ret && i < cCRLEntry; i++)
    {
        DWORD size;

        ret = CRYPT_AsnEncodeCRLEntry(&rgCRLEntry[i], NULL, &size);
        if (ret)
            dataLen += size;
    }
    CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
    bytesNeeded = 1 + lenBytes + dataLen;
    if (!pbEncoded)
        *pcbEncoded = bytesNeeded;
    else
    {
        if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
         pcbEncoded, bytesNeeded)))
        {
            if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                pbEncoded = *(BYTE **)pbEncoded;
            *pbEncoded++ = ASN_SEQUENCEOF;
            CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            for (i = 0; i < cCRLEntry; i++)
            {
                DWORD size = dataLen;

                ret = CRYPT_AsnEncodeCRLEntry(&rgCRLEntry[i], pbEncoded, &size);
                pbEncoded += size;
                dataLen -= size;
            }
        }
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCRLVersion(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    const DWORD *ver = (const DWORD *)pvStructInfo;
    BOOL ret;

    /* CRL_V1 is not encoded */
    if (*ver == CRL_V1)
    {
        *pcbEncoded = 0;
        ret = TRUE;
    }
    else
        ret = CRYPT_AsnEncodeInt(dwCertEncodingType, X509_INTEGER, ver,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    return ret;
}

/* Like in Windows, this blithely ignores the validity of the passed-in
 * CRL_INFO, and just encodes it as-is.  The resulting encoded data may not
 * decode properly, see CRYPT_AsnDecodeCRLInfo.
 */
static BOOL WINAPI CRYPT_AsnEncodeCRLInfo(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRL_INFO *info = (const CRL_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[7] = {
         { &info->dwVersion,          CRYPT_AsnEncodeCRLVersion, 0 },
         { &info->SignatureAlgorithm, CRYPT_AsnEncodeAlgorithmId, 0 },
         { &info->Issuer,             CRYPT_CopyEncodedBlob, 0 },
         { &info->ThisUpdate,         CRYPT_AsnEncodeChoiceOfTime, 0 },
         { 0 }
        };
        struct AsnConstructedItem constructed[1] = { { 0 } };
        DWORD cItem = 4, cConstructed = 0;

        if (info->NextUpdate.dwLowDateTime || info->NextUpdate.dwHighDateTime)
        {
            items[cItem].pvStructInfo = &info->NextUpdate;
            items[cItem].encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            cItem++;
        }
        if (info->cCRLEntry)
        {
            items[cItem].pvStructInfo = &info->cCRLEntry;
            items[cItem].encodeFunc = CRYPT_AsnEncodeCRLEntries;
            cItem++;
        }
        if (info->cExtension)
        {
            constructed[cConstructed].tag = 0;
            constructed[cConstructed].pvStructInfo = &info->cExtension;
            constructed[cConstructed].encodeFunc = CRYPT_AsnEncodeExtensions;
            items[cItem].pvStructInfo = &constructed[cConstructed];
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cConstructed++;
            cItem++;
        }

        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeExtension(CERT_EXTENSION *ext, BYTE *pbEncoded,
 DWORD *pcbEncoded)
{
    BOOL ret;
    struct AsnEncodeSequenceItem items[3] = {
     { ext->pszObjId, CRYPT_AsnEncodeOid, 0 },
     { NULL, NULL, 0 },
     { NULL, NULL, 0 },
    };
    DWORD cItem = 1;

    TRACE("%p, %p, %ld\n", ext, pbEncoded, *pcbEncoded);

    if (ext->fCritical)
    {
        items[cItem].pvStructInfo = &ext->fCritical;
        items[cItem].encodeFunc = CRYPT_AsnEncodeBool;
        cItem++;
    }
    items[cItem].pvStructInfo = &ext->Value;
    items[cItem].encodeFunc = CRYPT_AsnEncodeOctets;
    cItem++;

    ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem, 0, NULL,
     pbEncoded, pcbEncoded);
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeExtensions(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const CERT_EXTENSIONS *exts = (const CERT_EXTENSIONS *)pvStructInfo;

        ret = TRUE;
        for (i = 0, dataLen = 0; ret && i < exts->cExtension; i++)
        {
            DWORD size;

            ret = CRYPT_AsnEncodeExtension(&exts->rgExtension[i], NULL, &size);
            if (ret)
                dataLen += size;
        }
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_SEQUENCEOF;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                for (i = 0; i < exts->cExtension; i++)
                {
                    DWORD size = dataLen;

                    ret = CRYPT_AsnEncodeExtension(&exts->rgExtension[i],
                     pbEncoded, &size);
                    pbEncoded += size;
                    dataLen -= size;
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeOid(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    LPCSTR pszObjId = (LPCSTR)pvStructInfo;
    DWORD bytesNeeded = 0, lenBytes;
    BOOL ret = TRUE;
    int firstPos = 0;
    BYTE firstByte = 0;

    TRACE("%s\n", debugstr_a(pszObjId));

    if (pszObjId)
    {
        const char *ptr;
        int val1, val2;

        if (sscanf(pszObjId, "%d.%d.%n", &val1, &val2, &firstPos) != 2)
        {
            SetLastError(CRYPT_E_ASN1_ERROR);
            return FALSE;
        }
        bytesNeeded++;
        firstByte = val1 * 40 + val2;
        ptr = pszObjId + firstPos;
        while (ret && *ptr)
        {
            int pos;

            /* note I assume each component is at most 32-bits long in base 2 */
            if (sscanf(ptr, "%d%n", &val1, &pos) == 1)
            {
                if (val1 >= 0x10000000)
                    bytesNeeded += 5;
                else if (val1 >= 0x200000)
                    bytesNeeded += 4;
                else if (val1 >= 0x4000)
                    bytesNeeded += 3;
                else if (val1 >= 0x80)
                    bytesNeeded += 2;
                else
                    bytesNeeded += 1;
                ptr += pos;
                if (*ptr == '.')
                    ptr++;
            }
            else
            {
                SetLastError(CRYPT_E_ASN1_ERROR);
                return FALSE;
            }
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
    }
    else
        lenBytes = 1;
    bytesNeeded += 1 + lenBytes;
    if (pbEncoded)
    {
        if (*pcbEncoded < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            ret = FALSE;
        }
        else
        {
            *pbEncoded++ = ASN_OBJECTIDENTIFIER;
            CRYPT_EncodeLen(bytesNeeded - 1 - lenBytes, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            if (pszObjId)
            {
                const char *ptr;
                int val, pos;

                *pbEncoded++ = firstByte;
                ptr = pszObjId + firstPos;
                while (ret && *ptr)
                {
                    sscanf(ptr, "%d%n", &val, &pos);
                    {
                        unsigned char outBytes[5];
                        int numBytes, i;

                        if (val >= 0x10000000)
                            numBytes = 5;
                        else if (val >= 0x200000)
                            numBytes = 4;
                        else if (val >= 0x4000)
                            numBytes = 3;
                        else if (val >= 0x80)
                            numBytes = 2;
                        else
                            numBytes = 1;
                        for (i = numBytes; i > 0; i--)
                        {
                            outBytes[i - 1] = val & 0x7f;
                            val >>= 7;
                        }
                        for (i = 0; i < numBytes - 1; i++)
                            *pbEncoded++ = outBytes[i] | 0x80;
                        *pbEncoded++ = outBytes[i];
                        ptr += pos;
                        if (*ptr == '.')
                            ptr++;
                    }
                }
            }
        }
    }
    *pcbEncoded = bytesNeeded;
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeNameValue(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = TRUE;

    __TRY
    {
        BYTE tag;
        DWORD bytesNeeded, lenBytes, encodedLen;
        const CERT_NAME_VALUE *value = (CERT_NAME_VALUE *)pvStructInfo;

        switch (value->dwValueType)
        {
        case CERT_RDN_NUMERIC_STRING:
            tag = ASN_NUMERICSTRING;
            encodedLen = value->Value.cbData;
            break;
        case CERT_RDN_PRINTABLE_STRING:
            tag = ASN_PRINTABLESTRING;
            encodedLen = value->Value.cbData;
            break;
        case CERT_RDN_T61_STRING:
            tag = ASN_T61STRING;
            encodedLen = value->Value.cbData;
            break;
        case CERT_RDN_IA5_STRING:
            tag = ASN_IA5STRING;
            encodedLen = value->Value.cbData;
            break;
        case CERT_RDN_ANY_TYPE:
            /* explicitly disallowed */
            SetLastError(E_INVALIDARG);
            return FALSE;
        default:
            FIXME("String type %ld unimplemented\n", value->dwValueType);
            return FALSE;
        }
        CRYPT_EncodeLen(encodedLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + encodedLen;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = tag;
                CRYPT_EncodeLen(encodedLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                switch (value->dwValueType)
                {
                case CERT_RDN_NUMERIC_STRING:
                case CERT_RDN_PRINTABLE_STRING:
                case CERT_RDN_T61_STRING:
                case CERT_RDN_IA5_STRING:
                    memcpy(pbEncoded, value->Value.pbData, value->Value.cbData);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeRdnAttr(DWORD dwCertEncodingType,
 CERT_RDN_ATTR *attr, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    DWORD bytesNeeded = 0, lenBytes, size;
    BOOL ret;

    ret = CRYPT_AsnEncodeOid(dwCertEncodingType, NULL, attr->pszObjId,
     0, NULL, NULL, &size);
    if (ret)
    {
        bytesNeeded += size;
        /* hack: a CERT_RDN_ATTR is identical to a CERT_NAME_VALUE beginning
         * with dwValueType, so "cast" it to get its encoded size
         */
        ret = CRYPT_AsnEncodeNameValue(dwCertEncodingType, X509_NAME_VALUE,
         (CERT_NAME_VALUE *)&attr->dwValueType, 0, NULL, NULL, &size);
        if (ret)
        {
            bytesNeeded += size;
            CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
            bytesNeeded += 1 + lenBytes;
            if (pbEncoded)
            {
                if (*pcbEncoded < bytesNeeded)
                {
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    *pbEncoded++ = ASN_SEQUENCE;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                     &lenBytes);
                    pbEncoded += lenBytes;
                    size = bytesNeeded - 1 - lenBytes;
                    ret = CRYPT_AsnEncodeOid(dwCertEncodingType, NULL,
                     attr->pszObjId, 0, NULL, pbEncoded, &size);
                    if (ret)
                    {
                        pbEncoded += size;
                        size = bytesNeeded - 1 - lenBytes - size;
                        ret = CRYPT_AsnEncodeNameValue(dwCertEncodingType,
                         X509_NAME_VALUE, (CERT_NAME_VALUE *)&attr->dwValueType,
                         0, NULL, pbEncoded, &size);
                    }
                }
            }
            *pcbEncoded = bytesNeeded;
        }
    }
    return ret;
}

static int BLOBComp(const void *l, const void *r)
{
    CRYPT_DER_BLOB *a = (CRYPT_DER_BLOB *)l, *b = (CRYPT_DER_BLOB *)r;
    int ret;

    if (!(ret = memcmp(a->pbData, b->pbData, min(a->cbData, b->cbData))))
        ret = a->cbData - b->cbData;
    return ret;
}

/* This encodes as a SET OF, which in DER must be lexicographically sorted.
 */
static BOOL WINAPI CRYPT_AsnEncodeRdn(DWORD dwCertEncodingType, CERT_RDN *rdn,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    CRYPT_DER_BLOB *blobs = NULL;

    __TRY
    {
        DWORD bytesNeeded = 0, lenBytes, i;

        blobs = NULL;
        ret = TRUE;
        if (rdn->cRDNAttr)
        {
            blobs = CryptMemAlloc(rdn->cRDNAttr * sizeof(CRYPT_DER_BLOB));
            if (!blobs)
                ret = FALSE;
            else
                memset(blobs, 0, rdn->cRDNAttr * sizeof(CRYPT_DER_BLOB));
        }
        for (i = 0; ret && i < rdn->cRDNAttr; i++)
        {
            ret = CRYPT_AsnEncodeRdnAttr(dwCertEncodingType, &rdn->rgRDNAttr[i],
             NULL, &blobs[i].cbData);
            if (ret)
                bytesNeeded += blobs[i].cbData;
        }
        if (ret)
        {
            CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
            bytesNeeded += 1 + lenBytes;
            if (pbEncoded)
            {
                if (*pcbEncoded < bytesNeeded)
                {
                    SetLastError(ERROR_MORE_DATA);
                    ret = FALSE;
                }
                else
                {
                    for (i = 0; ret && i < rdn->cRDNAttr; i++)
                    {
                        blobs[i].pbData = CryptMemAlloc(blobs[i].cbData);
                        if (!blobs[i].pbData)
                            ret = FALSE;
                        else
                            ret = CRYPT_AsnEncodeRdnAttr(dwCertEncodingType,
                             &rdn->rgRDNAttr[i], blobs[i].pbData,
                             &blobs[i].cbData);
                    }
                    if (ret)
                    {
                        qsort(blobs, rdn->cRDNAttr, sizeof(CRYPT_DER_BLOB),
                         BLOBComp);
                        *pbEncoded++ = ASN_CONSTRUCTOR | ASN_SETOF;
                        CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                         &lenBytes);
                        pbEncoded += lenBytes;
                        for (i = 0; ret && i < rdn->cRDNAttr; i++)
                        {
                            memcpy(pbEncoded, blobs[i].pbData, blobs[i].cbData);
                            pbEncoded += blobs[i].cbData;
                        }
                    }
                }
            }
            *pcbEncoded = bytesNeeded;
        }
        if (blobs)
        {
            for (i = 0; i < rdn->cRDNAttr; i++)
                CryptMemFree(blobs[i].pbData);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    CryptMemFree(blobs);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_NAME_INFO *info = (const CERT_NAME_INFO *)pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        TRACE("encoding name with %ld RDNs\n", info->cRDN);
        ret = TRUE;
        for (i = 0; ret && i < info->cRDN; i++)
        {
            ret = CRYPT_AsnEncodeRdn(dwCertEncodingType, &info->rgRDN[i], NULL,
             &size);
            if (ret)
                bytesNeeded += size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    *pbEncoded++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                     &lenBytes);
                    pbEncoded += lenBytes;
                    for (i = 0; ret && i < info->cRDN; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeRdn(dwCertEncodingType,
                         &info->rgRDN[i], pbEncoded, &size);
                        if (ret)
                        {
                            pbEncoded += size;
                            bytesNeeded -= size;
                        }
                    }
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBool(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL val = *(const BOOL *)pvStructInfo, ret;

    TRACE("%d\n", val);

    if (!pbEncoded)
    {
        *pcbEncoded = 3;
        ret = TRUE;
    }
    else if (*pcbEncoded < 3)
    {
        *pcbEncoded = 3;
        SetLastError(ERROR_MORE_DATA);
        ret = FALSE;
    }
    else
    {
        *pcbEncoded = 3;
        *pbEncoded++ = ASN_BOOL;
        *pbEncoded++ = 1;
        *pbEncoded++ = val ? 0xff : 0;
        ret = TRUE;
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL CRYPT_AsnEncodeAltNameEntry(const CERT_ALT_NAME_ENTRY *entry,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;
    DWORD dataLen;

    ret = TRUE;
    switch (entry->dwAltNameChoice)
    {
    case CERT_ALT_NAME_RFC822_NAME:
    case CERT_ALT_NAME_DNS_NAME:
    case CERT_ALT_NAME_URL:
        if (entry->u.pwszURL)
        {
            DWORD i;

            /* Not + 1: don't encode the NULL-terminator */
            dataLen = lstrlenW(entry->u.pwszURL);
            for (i = 0; ret && i < dataLen; i++)
            {
                if (entry->u.pwszURL[i] > 0x7f)
                {
                    SetLastError(CRYPT_E_INVALID_IA5_STRING);
                    ret = FALSE;
                    *pcbEncoded = i;
                }
            }
        }
        else
            dataLen = 0;
        break;
    case CERT_ALT_NAME_IP_ADDRESS:
        dataLen = entry->u.IPAddress.cbData;
        break;
    case CERT_ALT_NAME_REGISTERED_ID:
        /* FIXME: encode OID */
    case CERT_ALT_NAME_OTHER_NAME:
    case CERT_ALT_NAME_DIRECTORY_NAME:
        FIXME("name type %ld unimplemented\n", entry->dwAltNameChoice);
        return FALSE;
    default:
        SetLastError(E_INVALIDARG);
        return FALSE;
    }
    if (ret)
    {
        DWORD bytesNeeded, lenBytes;

        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + dataLen + lenBytes;
        if (!pbEncoded)
            *pcbEncoded = bytesNeeded;
        else if (*pcbEncoded < bytesNeeded)
        {
            SetLastError(ERROR_MORE_DATA);
            *pcbEncoded = bytesNeeded;
            ret = FALSE;
        }
        else
        {
            *pbEncoded++ = ASN_CONTEXT | (entry->dwAltNameChoice - 1);
            CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
            pbEncoded += lenBytes;
            switch (entry->dwAltNameChoice)
            {
            case CERT_ALT_NAME_RFC822_NAME:
            case CERT_ALT_NAME_DNS_NAME:
            case CERT_ALT_NAME_URL:
            {
                DWORD i;

                for (i = 0; i < dataLen; i++)
                    *pbEncoded++ = (BYTE)entry->u.pwszURL[i];
                break;
            }
            case CERT_ALT_NAME_IP_ADDRESS:
                memcpy(pbEncoded, entry->u.IPAddress.pbData, dataLen);
                break;
            }
            if (ret)
                *pcbEncoded = bytesNeeded;
        }
    }
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeAltName(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_ALT_NAME_INFO *info =
         (const CERT_ALT_NAME_INFO *)pvStructInfo;
        DWORD bytesNeeded, dataLen, lenBytes, i;

        ret = TRUE;
        /* FIXME: should check that cAltEntry is not bigger than 0xff, since we
         * can't encode an erroneous entry index if it's bigger than this.
         */
        for (i = 0, dataLen = 0; ret && i < info->cAltEntry; i++)
        {
            DWORD len;

            ret = CRYPT_AsnEncodeAltNameEntry(&info->rgAltEntry[i], NULL,
             &len);
            if (ret)
                dataLen += len;
            else if (GetLastError() == CRYPT_E_INVALID_IA5_STRING)
            {
                /* CRYPT_AsnEncodeAltNameEntry encoded the index of
                 * the bad character, now set the index of the bad
                 * entry
                 */
                *pcbEncoded = (BYTE)i <<
                 CERT_ALT_NAME_ENTRY_ERR_INDEX_SHIFT | len;
            }
        }
        if (ret)
        {
            CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
            bytesNeeded = 1 + lenBytes + dataLen;
            if (!pbEncoded)
            {
                *pcbEncoded = bytesNeeded;
                ret = TRUE;
            }
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    *pbEncoded++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    for (i = 0; ret && i < info->cAltEntry; i++)
                    {
                        DWORD len = dataLen;

                        ret = CRYPT_AsnEncodeAltNameEntry(&info->rgAltEntry[i],
                         pbEncoded, &len);
                        if (ret)
                        {
                            pbEncoded += len;
                            dataLen -= len;
                        }
                    }
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBasicConstraints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_BASIC_CONSTRAINTS_INFO *info =
         (const CERT_BASIC_CONSTRAINTS_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[3] = {
         { &info->SubjectType, CRYPT_AsnEncodeBits, 0 },
         { 0 }
        };
        DWORD cItem = 1;

        if (info->fPathLenConstraint)
        {
            items[cItem].pvStructInfo = &info->dwPathLenConstraint;
            items[cItem].encodeFunc = CRYPT_AsnEncodeInt;
            cItem++;
        }
        if (info->cSubtreesConstraint)
        {
            items[cItem].pvStructInfo = &info->cSubtreesConstraint;
            items[cItem].encodeFunc = CRYPT_AsnEncodeSequenceOfAny;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBasicConstraints2(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_BASIC_CONSTRAINTS2_INFO *info =
         (const CERT_BASIC_CONSTRAINTS2_INFO *)pvStructInfo;
        struct AsnEncodeSequenceItem items[2] = { { 0 } };
        DWORD cItem = 0;

        if (info->fCA)
        {
            items[cItem].pvStructInfo = &info->fCA;
            items[cItem].encodeFunc = CRYPT_AsnEncodeBool;
            cItem++;
        }
        if (info->fPathLenConstraint)
        {
            items[cItem].pvStructInfo = &info->dwPathLenConstraint;
            items[cItem].encodeFunc = CRYPT_AsnEncodeInt;
            cItem++;
        }
        ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
         dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeRsaPubKey(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const BLOBHEADER *hdr =
         (const BLOBHEADER *)pvStructInfo;

        if (hdr->bType != PUBLICKEYBLOB)
        {
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        else
        {
            const RSAPUBKEY *rsaPubKey = (const RSAPUBKEY *)
             ((const BYTE *)pvStructInfo + sizeof(BLOBHEADER));
            CRYPT_INTEGER_BLOB blob = { rsaPubKey->bitlen / 8,
             (BYTE *)pvStructInfo + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) };
            struct AsnEncodeSequenceItem items[] = { 
             { &blob, CRYPT_AsnEncodeUnsignedInteger, 0 },
             { &rsaPubKey->pubexp, CRYPT_AsnEncodeInt, 0 },
            };

            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items,
             sizeof(items) / sizeof(items[0]), dwFlags, pEncodePara, pbEncoded,
             pcbEncoded);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeOctets(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_DATA_BLOB *blob = (const CRYPT_DATA_BLOB *)pvStructInfo;
        DWORD bytesNeeded, lenBytes;

        TRACE("(%ld, %p), %08lx, %p, %p, %ld\n", blob->cbData, blob->pbData,
         dwFlags, pEncodePara, pbEncoded, *pcbEncoded);

        CRYPT_EncodeLen(blob->cbData, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + blob->cbData;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_OCTETSTRING;
                CRYPT_EncodeLen(blob->cbData, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                if (blob->cbData)
                    memcpy(pbEncoded, blob->pbData, blob->cbData);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    TRACE("returning %d (%08lx)\n", ret, GetLastError());
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBits(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_BIT_BLOB *blob = (const CRYPT_BIT_BLOB *)pvStructInfo;
        DWORD bytesNeeded, lenBytes, dataBytes;
        BYTE unusedBits;

        /* yep, MS allows cUnusedBits to be >= 8 */
        if (!blob->cUnusedBits)
        {
            dataBytes = blob->cbData;
            unusedBits = 0;
        }
        else if (blob->cbData * 8 > blob->cUnusedBits)
        {
            dataBytes = (blob->cbData * 8 - blob->cUnusedBits) / 8 + 1;
            unusedBits = blob->cUnusedBits >= 8 ? blob->cUnusedBits / 8 :
             blob->cUnusedBits;
        }
        else
        {
            dataBytes = 0;
            unusedBits = 0;
        }
        CRYPT_EncodeLen(dataBytes + 1, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataBytes + 1;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_BITSTRING;
                CRYPT_EncodeLen(dataBytes + 1, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                *pbEncoded++ = unusedBits;
                if (dataBytes)
                {
                    BYTE mask = 0xff << unusedBits;

                    if (dataBytes > 1)
                    {
                        memcpy(pbEncoded, blob->pbData, dataBytes - 1);
                        pbEncoded += dataBytes - 1;
                    }
                    *pbEncoded = *(blob->pbData + dataBytes - 1) & mask;
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeBitsSwapBytes(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRYPT_BIT_BLOB *blob = (const CRYPT_BIT_BLOB *)pvStructInfo;
        CRYPT_BIT_BLOB newBlob = { blob->cbData, NULL, blob->cUnusedBits };

        ret = TRUE;
        if (newBlob.cbData)
        {
            newBlob.pbData = CryptMemAlloc(newBlob.cbData);
            if (newBlob.pbData)
            {
                DWORD i;

                for (i = 0; i < newBlob.cbData; i++)
                    newBlob.pbData[newBlob.cbData - i - 1] = blob->pbData[i];
            }
            else
                ret = FALSE;
        }
        if (ret)
            ret = CRYPT_AsnEncodeBits(dwCertEncodingType, lpszStructType,
             &newBlob, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        CryptMemFree(newBlob.pbData);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeInt(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    CRYPT_INTEGER_BLOB blob = { sizeof(INT), (BYTE *)pvStructInfo };

    return CRYPT_AsnEncodeInteger(dwCertEncodingType, X509_MULTI_BYTE_INTEGER,
     &blob, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
}

static BOOL WINAPI CRYPT_AsnEncodeInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD significantBytes, lenBytes;
        BYTE padByte = 0, bytesNeeded;
        BOOL pad = FALSE;
        const CRYPT_INTEGER_BLOB *blob =
         (const CRYPT_INTEGER_BLOB *)pvStructInfo;

        significantBytes = blob->cbData;
        if (significantBytes)
        {
            if (blob->pbData[significantBytes - 1] & 0x80)
            {
                /* negative, lop off leading (little-endian) 0xffs */
                for (; significantBytes > 0 &&
                 blob->pbData[significantBytes - 1] == 0xff; significantBytes--)
                    ;
                if (blob->pbData[significantBytes - 1] < 0x80)
                {
                    padByte = 0xff;
                    pad = TRUE;
                }
            }
            else
            {
                /* positive, lop off leading (little-endian) zeroes */
                for (; significantBytes > 0 &&
                 !blob->pbData[significantBytes - 1]; significantBytes--)
                    ;
                if (significantBytes == 0)
                    significantBytes = 1;
                if (blob->pbData[significantBytes - 1] > 0x7f)
                {
                    padByte = 0;
                    pad = TRUE;
                }
            }
        }
        if (pad)
            CRYPT_EncodeLen(significantBytes + 1, NULL, &lenBytes);
        else
            CRYPT_EncodeLen(significantBytes, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + significantBytes;
        if (pad)
            bytesNeeded++;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_INTEGER;
                if (pad)
                {
                    CRYPT_EncodeLen(significantBytes + 1, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    *pbEncoded++ = padByte;
                }
                else
                {
                    CRYPT_EncodeLen(significantBytes, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                }
                for (; significantBytes > 0; significantBytes--)
                    *(pbEncoded++) = blob->pbData[significantBytes - 1];
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUnsignedInteger(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD significantBytes, lenBytes;
        BYTE bytesNeeded;
        BOOL pad = FALSE;
        const CRYPT_INTEGER_BLOB *blob =
         (const CRYPT_INTEGER_BLOB *)pvStructInfo;

        significantBytes = blob->cbData;
        if (significantBytes)
        {
            /* positive, lop off leading (little-endian) zeroes */
            for (; significantBytes > 0 && !blob->pbData[significantBytes - 1];
             significantBytes--)
                ;
            if (significantBytes == 0)
                significantBytes = 1;
            if (blob->pbData[significantBytes - 1] > 0x7f)
                pad = TRUE;
        }
        if (pad)
            CRYPT_EncodeLen(significantBytes + 1, NULL, &lenBytes);
        else
            CRYPT_EncodeLen(significantBytes, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + significantBytes;
        if (pad)
            bytesNeeded++;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_INTEGER;
                if (pad)
                {
                    CRYPT_EncodeLen(significantBytes + 1, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                    *pbEncoded++ = 0;
                }
                else
                {
                    CRYPT_EncodeLen(significantBytes, pbEncoded, &lenBytes);
                    pbEncoded += lenBytes;
                }
                for (; significantBytes > 0; significantBytes--)
                    *(pbEncoded++) = blob->pbData[significantBytes - 1];
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeEnumerated(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    CRYPT_INTEGER_BLOB blob;
    BOOL ret;

    /* Encode as an unsigned integer, then change the tag to enumerated */
    blob.cbData = sizeof(DWORD);
    blob.pbData = (BYTE *)pvStructInfo;
    ret = CRYPT_AsnEncodeUnsignedInteger(dwCertEncodingType,
     X509_MULTI_BYTE_UINT, &blob, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    if (ret && pbEncoded)
    {
        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
            pbEncoded = *(BYTE **)pbEncoded;
        pbEncoded[0] = ASN_ENUMERATED;
    }
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeUtcTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;
        /* sorry, magic number: enough for tag, len, YYMMDDHHMMSSZ\0.  I use a
         * temporary buffer because the output buffer is not NULL-terminated.
         */
        char buf[16];
        static const DWORD bytesNeeded = sizeof(buf) - 1;

        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            /* Sanity check the year, this is a two-digit year format */
            ret = FileTimeToSystemTime((const FILETIME *)pvStructInfo,
             &sysTime);
            if (ret && (sysTime.wYear < 1950 || sysTime.wYear > 2050))
            {
                SetLastError(CRYPT_E_BAD_ENCODE);
                ret = FALSE;
            }
            if (ret)
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    buf[0] = ASN_UTCTIME;
                    buf[1] = bytesNeeded - 2;
                    snprintf(buf + 2, sizeof(buf) - 2,
                     "%02d%02d%02d%02d%02d%02dZ", sysTime.wYear >= 2000 ?
                     sysTime.wYear - 2000 : sysTime.wYear - 1900,
                     sysTime.wMonth, sysTime.wDay, sysTime.wHour,
                     sysTime.wMinute, sysTime.wSecond);
                    memcpy(pbEncoded, buf, bytesNeeded);
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeGeneralizedTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;
        /* sorry, magic number: enough for tag, len, YYYYMMDDHHMMSSZ\0.  I use a
         * temporary buffer because the output buffer is not NULL-terminated.
         */
        char buf[18];
        static const DWORD bytesNeeded = sizeof(buf) - 1;

        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            ret = FileTimeToSystemTime((const FILETIME *)pvStructInfo,
             &sysTime);
            if (ret)
                ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
                 pcbEncoded, bytesNeeded);
            if (ret)
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                buf[0] = ASN_GENERALTIME;
                buf[1] = bytesNeeded - 2;
                snprintf(buf + 2, sizeof(buf) - 2, "%04d%02d%02d%02d%02d%02dZ",
                 sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour,
                 sysTime.wMinute, sysTime.wSecond);
                memcpy(pbEncoded, buf, bytesNeeded);
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeChoiceOfTime(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        SYSTEMTIME sysTime;

        /* Check the year, if it's in the UTCTime range call that encode func */
        if (!FileTimeToSystemTime((const FILETIME *)pvStructInfo, &sysTime))
            return FALSE;
        if (sysTime.wYear >= 1950 && sysTime.wYear <= 2050)
            ret = CRYPT_AsnEncodeUtcTime(dwCertEncodingType, lpszStructType,
             pvStructInfo, dwFlags, pEncodePara, pbEncoded, pcbEncoded);
        else
            ret = CRYPT_AsnEncodeGeneralizedTime(dwCertEncodingType,
             lpszStructType, pvStructInfo, dwFlags, pEncodePara, pbEncoded,
             pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeSequenceOfAny(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        DWORD bytesNeeded, dataLen, lenBytes, i;
        const CRYPT_SEQUENCE_OF_ANY *seq =
         (const CRYPT_SEQUENCE_OF_ANY *)pvStructInfo;

        for (i = 0, dataLen = 0; i < seq->cValue; i++)
            dataLen += seq->rgValue[i].cbData;
        CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
        bytesNeeded = 1 + lenBytes + dataLen;
        if (!pbEncoded)
        {
            *pcbEncoded = bytesNeeded;
            ret = TRUE;
        }
        else
        {
            if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara, pbEncoded,
             pcbEncoded, bytesNeeded)))
            {
                if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                    pbEncoded = *(BYTE **)pbEncoded;
                *pbEncoded++ = ASN_SEQUENCEOF;
                CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                pbEncoded += lenBytes;
                for (i = 0; i < seq->cValue; i++)
                {
                    memcpy(pbEncoded, seq->rgValue[i].pbData,
                     seq->rgValue[i].cbData);
                    pbEncoded += seq->rgValue[i].cbData;
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL CRYPT_AsnEncodeDistPoint(const CRL_DIST_POINT *distPoint,
 BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret = TRUE;
    struct AsnEncodeSequenceItem items[3] = { { 0 } };
    struct AsnConstructedItem constructed = { 0 };
    struct AsnEncodeTagSwappedItem swapped[3] = { { 0 } };
    DWORD cItem = 0, cSwapped = 0;

    switch (distPoint->DistPointName.dwDistPointNameChoice)
    {
    case CRL_DIST_POINT_NO_NAME:
        /* do nothing */
        break;
    case CRL_DIST_POINT_FULL_NAME:
        swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 0;
        swapped[cSwapped].pvStructInfo = &distPoint->DistPointName.u.FullName;
        swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeAltName;
        constructed.tag = 0;
        constructed.pvStructInfo = &swapped[cSwapped];
        constructed.encodeFunc = CRYPT_AsnEncodeSwapTag;
        items[cItem].pvStructInfo = &constructed;
        items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
        cSwapped++;
        cItem++;
        break;
    case CRL_DIST_POINT_ISSUER_RDN_NAME:
        FIXME("unimplemented for CRL_DIST_POINT_ISSUER_RDN_NAME\n");
        ret = FALSE;
        break;
    default:
        ret = FALSE;
    }
    if (ret && distPoint->ReasonFlags.cbData)
    {
        swapped[cSwapped].tag = ASN_CONTEXT | 1;
        swapped[cSwapped].pvStructInfo = &distPoint->ReasonFlags;
        swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBits;
        items[cItem].pvStructInfo = &swapped[cSwapped];
        items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
        cSwapped++;
        cItem++;
    }
    if (ret && distPoint->CRLIssuer.cAltEntry)
    {
        swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 2;
        swapped[cSwapped].pvStructInfo = &distPoint->CRLIssuer;
        swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeAltName;
        items[cItem].pvStructInfo = &swapped[cSwapped];
        items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
        cSwapped++;
        cItem++;
    }
    if (ret)
        ret = CRYPT_AsnEncodeSequence(X509_ASN_ENCODING, items, cItem, 0, NULL,
         pbEncoded, pcbEncoded);
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeCRLDistPoints(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRL_DIST_POINTS_INFO *info =
         (const CRL_DIST_POINTS_INFO *)pvStructInfo;

        if (!info->cDistPoint)
        {
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        else
        {
            DWORD bytesNeeded, dataLen, lenBytes, i;

            ret = TRUE;
            for (i = 0, dataLen = 0; ret && i < info->cDistPoint; i++)
            {
                DWORD len;

                ret = CRYPT_AsnEncodeDistPoint(&info->rgDistPoint[i], NULL,
                 &len);
                if (ret)
                    dataLen += len;
                else if (GetLastError() == CRYPT_E_INVALID_IA5_STRING)
                {
                    /* Have to propagate index of failing character */
                    *pcbEncoded = len;
                }
            }
            if (ret)
            {
                CRYPT_EncodeLen(dataLen, NULL, &lenBytes);
                bytesNeeded = 1 + lenBytes + dataLen;
                if (!pbEncoded)
                {
                    *pcbEncoded = bytesNeeded;
                    ret = TRUE;
                }
                else
                {
                    if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                     pbEncoded, pcbEncoded, bytesNeeded)))
                    {
                        if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                            pbEncoded = *(BYTE **)pbEncoded;
                        *pbEncoded++ = ASN_SEQUENCEOF;
                        CRYPT_EncodeLen(dataLen, pbEncoded, &lenBytes);
                        pbEncoded += lenBytes;
                        for (i = 0; ret && i < info->cDistPoint; i++)
                        {
                            DWORD len = dataLen;

                            ret = CRYPT_AsnEncodeDistPoint(
                             &info->rgDistPoint[i], pbEncoded, &len);
                            if (ret)
                            {
                                pbEncoded += len;
                                dataLen -= len;
                            }
                        }
                    }
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeEnhancedKeyUsage(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CERT_ENHKEY_USAGE *usage =
         (const CERT_ENHKEY_USAGE *)pvStructInfo;
        DWORD bytesNeeded = 0, lenBytes, size, i;

        ret = TRUE;
        for (i = 0; ret && i < usage->cUsageIdentifier; i++)
        {
            ret = CRYPT_AsnEncodeOid(dwCertEncodingType, NULL,
             usage->rgpszUsageIdentifier[i],
             dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, NULL, &size);
            if (ret)
                bytesNeeded += size;
        }
        CRYPT_EncodeLen(bytesNeeded, NULL, &lenBytes);
        bytesNeeded += 1 + lenBytes;
        if (ret)
        {
            if (!pbEncoded)
                *pcbEncoded = bytesNeeded;
            else
            {
                if ((ret = CRYPT_EncodeEnsureSpace(dwFlags, pEncodePara,
                 pbEncoded, pcbEncoded, bytesNeeded)))
                {
                    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG)
                        pbEncoded = *(BYTE **)pbEncoded;
                    *pbEncoded++ = ASN_SEQUENCEOF;
                    CRYPT_EncodeLen(bytesNeeded - lenBytes - 1, pbEncoded,
                     &lenBytes);
                    pbEncoded += lenBytes;
                    for (i = 0; ret && i < usage->cUsageIdentifier; i++)
                    {
                        size = bytesNeeded;
                        ret = CRYPT_AsnEncodeOid(dwCertEncodingType, NULL,
                         usage->rgpszUsageIdentifier[i],
                         dwFlags & ~CRYPT_ENCODE_ALLOC_FLAG, NULL, pbEncoded,
                         &size);
                        if (ret)
                        {
                            pbEncoded += size;
                            bytesNeeded -= size;
                        }
                    }
                }
            }
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

static BOOL WINAPI CRYPT_AsnEncodeIssuingDistPoint(DWORD dwCertEncodingType,
 LPCSTR lpszStructType, const void *pvStructInfo, DWORD dwFlags,
 PCRYPT_ENCODE_PARA pEncodePara, BYTE *pbEncoded, DWORD *pcbEncoded)
{
    BOOL ret;

    __TRY
    {
        const CRL_ISSUING_DIST_POINT *point =
         (const CRL_ISSUING_DIST_POINT *)pvStructInfo;
        struct AsnEncodeSequenceItem items[6] = { { 0 } };
        struct AsnConstructedItem constructed = { 0 };
        struct AsnEncodeTagSwappedItem swapped[5] = { { 0 } };
        DWORD cItem = 0, cSwapped = 0;

        ret = TRUE;
        switch (point->DistPointName.dwDistPointNameChoice)
        {
        case CRL_DIST_POINT_NO_NAME:
            /* do nothing */
            break;
        case CRL_DIST_POINT_FULL_NAME:
            swapped[cSwapped].tag = ASN_CONTEXT | ASN_CONSTRUCTOR | 0;
            swapped[cSwapped].pvStructInfo = &point->DistPointName.u.FullName;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeAltName;
            constructed.tag = 0;
            constructed.pvStructInfo = &swapped[cSwapped];
            constructed.encodeFunc = CRYPT_AsnEncodeSwapTag;
            items[cItem].pvStructInfo = &constructed;
            items[cItem].encodeFunc = CRYPT_AsnEncodeConstructed;
            cSwapped++;
            cItem++;
            break;
        default:
            SetLastError(E_INVALIDARG);
            ret = FALSE;
        }
        if (ret && point->fOnlyContainsUserCerts)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 1;
            swapped[cSwapped].pvStructInfo = &point->fOnlyContainsUserCerts;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBool;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (ret && point->fOnlyContainsCACerts)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 2;
            swapped[cSwapped].pvStructInfo = &point->fOnlyContainsCACerts;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBool;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (ret && point->OnlySomeReasonFlags.cbData)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 3;
            swapped[cSwapped].pvStructInfo = &point->OnlySomeReasonFlags;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBits;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (ret && point->fIndirectCRL)
        {
            swapped[cSwapped].tag = ASN_CONTEXT | 4;
            swapped[cSwapped].pvStructInfo = &point->fIndirectCRL;
            swapped[cSwapped].encodeFunc = CRYPT_AsnEncodeBool;
            items[cItem].pvStructInfo = &swapped[cSwapped];
            items[cItem].encodeFunc = CRYPT_AsnEncodeSwapTag;
            cSwapped++;
            cItem++;
        }
        if (ret)
            ret = CRYPT_AsnEncodeSequence(dwCertEncodingType, items, cItem,
             dwFlags, pEncodePara, pbEncoded, pcbEncoded);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(STATUS_ACCESS_VIOLATION);
        ret = FALSE;
    }
    __ENDTRY
    return ret;
}

BOOL WINAPI CryptEncodeObjectEx(DWORD dwCertEncodingType, LPCSTR lpszStructType,
 const void *pvStructInfo, DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara,
 void *pvEncoded, DWORD *pcbEncoded)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret = FALSE;
    CryptEncodeObjectExFunc encodeFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE("(0x%08lx, %s, %p, 0x%08lx, %p, %p, %p)\n", dwCertEncodingType,
     debugstr_a(lpszStructType), pvStructInfo, dwFlags, pEncodePara,
     pvEncoded, pcbEncoded);

    if (!pvEncoded && !pcbEncoded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if ((dwCertEncodingType & CERT_ENCODING_TYPE_MASK) != X509_ASN_ENCODING
     && (dwCertEncodingType & CMSG_ENCODING_TYPE_MASK) != PKCS_7_ASN_ENCODING)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    SetLastError(NOERROR);
    if (dwFlags & CRYPT_ENCODE_ALLOC_FLAG && pvEncoded)
        *(BYTE **)pvEncoded = NULL;
    if (!HIWORD(lpszStructType))
    {
        switch (LOWORD(lpszStructType))
        {
        case (WORD)X509_CERT:
            encodeFunc = CRYPT_AsnEncodeCert;
            break;
        case (WORD)X509_CERT_TO_BE_SIGNED:
            encodeFunc = CRYPT_AsnEncodeCertInfo;
            break;
        case (WORD)X509_CERT_CRL_TO_BE_SIGNED:
            encodeFunc = CRYPT_AsnEncodeCRLInfo;
            break;
        case (WORD)X509_EXTENSIONS:
            encodeFunc = CRYPT_AsnEncodeExtensions;
            break;
        case (WORD)X509_NAME_VALUE:
            encodeFunc = CRYPT_AsnEncodeNameValue;
            break;
        case (WORD)X509_NAME:
            encodeFunc = CRYPT_AsnEncodeName;
            break;
        case (WORD)X509_PUBLIC_KEY_INFO:
            encodeFunc = CRYPT_AsnEncodePubKeyInfo;
            break;
        case (WORD)X509_ALTERNATE_NAME:
            encodeFunc = CRYPT_AsnEncodeAltName;
            break;
        case (WORD)X509_BASIC_CONSTRAINTS:
            encodeFunc = CRYPT_AsnEncodeBasicConstraints;
            break;
        case (WORD)X509_BASIC_CONSTRAINTS2:
            encodeFunc = CRYPT_AsnEncodeBasicConstraints2;
            break;
        case (WORD)RSA_CSP_PUBLICKEYBLOB:
            encodeFunc = CRYPT_AsnEncodeRsaPubKey;
            break;
        case (WORD)X509_OCTET_STRING:
            encodeFunc = CRYPT_AsnEncodeOctets;
            break;
        case (WORD)X509_BITS:
        case (WORD)X509_KEY_USAGE:
            encodeFunc = CRYPT_AsnEncodeBits;
            break;
        case (WORD)X509_INTEGER:
            encodeFunc = CRYPT_AsnEncodeInt;
            break;
        case (WORD)X509_MULTI_BYTE_INTEGER:
            encodeFunc = CRYPT_AsnEncodeInteger;
            break;
        case (WORD)X509_MULTI_BYTE_UINT:
            encodeFunc = CRYPT_AsnEncodeUnsignedInteger;
            break;
        case (WORD)X509_ENUMERATED:
            encodeFunc = CRYPT_AsnEncodeEnumerated;
            break;
        case (WORD)X509_CHOICE_OF_TIME:
            encodeFunc = CRYPT_AsnEncodeChoiceOfTime;
            break;
        case (WORD)X509_SEQUENCE_OF_ANY:
            encodeFunc = CRYPT_AsnEncodeSequenceOfAny;
            break;
        case (WORD)PKCS_UTC_TIME:
            encodeFunc = CRYPT_AsnEncodeUtcTime;
            break;
        case (WORD)X509_CRL_DIST_POINTS:
            encodeFunc = CRYPT_AsnEncodeCRLDistPoints;
            break;
        case (WORD)X509_ENHANCED_KEY_USAGE:
            encodeFunc = CRYPT_AsnEncodeEnhancedKeyUsage;
            break;
        case (WORD)X509_ISSUING_DIST_POINT:
            encodeFunc = CRYPT_AsnEncodeIssuingDistPoint;
            break;
        default:
            FIXME("%d: unimplemented\n", LOWORD(lpszStructType));
        }
    }
    else if (!strcmp(lpszStructType, szOID_CERT_EXTENSIONS))
        encodeFunc = CRYPT_AsnEncodeExtensions;
    else if (!strcmp(lpszStructType, szOID_RSA_signingTime))
        encodeFunc = CRYPT_AsnEncodeUtcTime;
    else if (!strcmp(lpszStructType, szOID_CRL_REASON_CODE))
        encodeFunc = CRYPT_AsnEncodeEnumerated;
    else if (!strcmp(lpszStructType, szOID_KEY_USAGE))
        encodeFunc = CRYPT_AsnEncodeBits;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_KEY_IDENTIFIER))
        encodeFunc = CRYPT_AsnEncodeOctets;
    else if (!strcmp(lpszStructType, szOID_BASIC_CONSTRAINTS))
        encodeFunc = CRYPT_AsnEncodeBasicConstraints;
    else if (!strcmp(lpszStructType, szOID_BASIC_CONSTRAINTS2))
        encodeFunc = CRYPT_AsnEncodeBasicConstraints2;
    else if (!strcmp(lpszStructType, szOID_ISSUER_ALT_NAME))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_ISSUER_ALT_NAME2))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_NEXT_UPDATE_LOCATION))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_ALT_NAME))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_SUBJECT_ALT_NAME2))
        encodeFunc = CRYPT_AsnEncodeAltName;
    else if (!strcmp(lpszStructType, szOID_CRL_DIST_POINTS))
        encodeFunc = CRYPT_AsnEncodeCRLDistPoints;
    else if (!strcmp(lpszStructType, szOID_ENHANCED_KEY_USAGE))
        encodeFunc = CRYPT_AsnEncodeEnhancedKeyUsage;
    else if (!strcmp(lpszStructType, szOID_ISSUING_DIST_POINT))
        encodeFunc = CRYPT_AsnEncodeIssuingDistPoint;
    else
        TRACE("OID %s not found or unimplemented, looking for DLL\n",
         debugstr_a(lpszStructType));
    if (!encodeFunc)
    {
        if (!set)
            set = CryptInitOIDFunctionSet(CRYPT_OID_ENCODE_OBJECT_EX_FUNC, 0);
        CryptGetOIDFunctionAddress(set, dwCertEncodingType, lpszStructType, 0,
         (void **)&encodeFunc, &hFunc);
    }
    if (encodeFunc)
        ret = encodeFunc(dwCertEncodingType, lpszStructType, pvStructInfo,
         dwFlags, pEncodePara, pvEncoded, pcbEncoded);
    else
        SetLastError(ERROR_FILE_NOT_FOUND);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

BOOL WINAPI CryptExportPublicKeyInfo(HCRYPTPROV hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    return CryptExportPublicKeyInfoEx(hCryptProv, dwKeySpec, dwCertEncodingType,
     NULL, 0, NULL, pInfo, pcbInfo);
}

static BOOL WINAPI CRYPT_ExportRsaPublicKeyInfoEx(HCRYPTPROV hCryptProv,
 DWORD dwKeySpec, DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId,
 DWORD dwFlags, void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    BOOL ret;
    HCRYPTKEY key;
    static CHAR oid[] = szOID_RSA_RSA;

    TRACE("(%ld, %ld, %08lx, %s, %08lx, %p, %p, %p)\n", hCryptProv, dwKeySpec,
     dwCertEncodingType, debugstr_a(pszPublicKeyObjId), dwFlags, pvAuxInfo,
     pInfo, pcbInfo);

    if (!pszPublicKeyObjId)
        pszPublicKeyObjId = oid;
    if ((ret = CryptGetUserKey(hCryptProv, dwKeySpec, &key)))
    {
        DWORD keySize = 0;

        ret = CryptExportKey(key, 0, PUBLICKEYBLOB, 0, NULL, &keySize);
        if (ret)
        {
            LPBYTE pubKey = CryptMemAlloc(keySize);

            if (pubKey)
            {
                ret = CryptExportKey(key, 0, PUBLICKEYBLOB, 0, pubKey,
                 &keySize);
                if (ret)
                {
                    DWORD encodedLen = 0;

                    ret = CryptEncodeObject(dwCertEncodingType,
                     RSA_CSP_PUBLICKEYBLOB, pubKey, NULL, &encodedLen);
                    if (ret)
                    {
                        DWORD sizeNeeded = sizeof(CERT_PUBLIC_KEY_INFO) +
                         strlen(pszPublicKeyObjId) + 1 + encodedLen;

                        if (!pInfo)
                            *pcbInfo = sizeNeeded;
                        else if (*pcbInfo < sizeNeeded)
                        {
                            SetLastError(ERROR_MORE_DATA);
                            *pcbInfo = sizeNeeded;
                            ret = FALSE;
                        }
                        else
                        {
                            pInfo->Algorithm.pszObjId = (char *)pInfo +
                             sizeof(CERT_PUBLIC_KEY_INFO);
                            lstrcpyA(pInfo->Algorithm.pszObjId,
                             pszPublicKeyObjId);
                            pInfo->Algorithm.Parameters.cbData = 0;
                            pInfo->Algorithm.Parameters.pbData = NULL;
                            pInfo->PublicKey.pbData =
                             (BYTE *)pInfo->Algorithm.pszObjId
                             + lstrlenA(pInfo->Algorithm.pszObjId) + 1;
                            pInfo->PublicKey.cbData = encodedLen;
                            pInfo->PublicKey.cUnusedBits = 0;
                            ret = CryptEncodeObject(dwCertEncodingType,
                             RSA_CSP_PUBLICKEYBLOB, pubKey,
                             pInfo->PublicKey.pbData, &pInfo->PublicKey.cbData);
                        }
                    }
                }
                CryptMemFree(pubKey);
            }
            else
                ret = FALSE;
        }
        CryptDestroyKey(key);
    }
    return ret;
}

typedef BOOL (WINAPI *ExportPublicKeyInfoExFunc)(HCRYPTPROV hCryptProv,
 DWORD dwKeySpec, DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId,
 DWORD dwFlags, void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo);

BOOL WINAPI CryptExportPublicKeyInfoEx(HCRYPTPROV hCryptProv, DWORD dwKeySpec,
 DWORD dwCertEncodingType, LPSTR pszPublicKeyObjId, DWORD dwFlags,
 void *pvAuxInfo, PCERT_PUBLIC_KEY_INFO pInfo, DWORD *pcbInfo)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret;
    ExportPublicKeyInfoExFunc exportFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE("(%ld, %ld, %08lx, %s, %08lx, %p, %p, %p)\n", hCryptProv, dwKeySpec,
     dwCertEncodingType, debugstr_a(pszPublicKeyObjId), dwFlags, pvAuxInfo,
     pInfo, pcbInfo);

    if (!hCryptProv)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pszPublicKeyObjId)
    {
        if (!set)
            set = CryptInitOIDFunctionSet(CRYPT_OID_EXPORT_PUBLIC_KEY_INFO_FUNC,
             0);
        CryptGetOIDFunctionAddress(set, dwCertEncodingType, pszPublicKeyObjId,
         0, (void **)&exportFunc, &hFunc);
    }
    if (!exportFunc)
        exportFunc = CRYPT_ExportRsaPublicKeyInfoEx;
    ret = exportFunc(hCryptProv, dwKeySpec, dwCertEncodingType,
     pszPublicKeyObjId, dwFlags, pvAuxInfo, pInfo, pcbInfo);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}

BOOL WINAPI CryptImportPublicKeyInfo(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, HCRYPTKEY *phKey)
{
    return CryptImportPublicKeyInfoEx(hCryptProv, dwCertEncodingType, pInfo,
     0, 0, NULL, phKey);
}

static BOOL WINAPI CRYPT_ImportRsaPublicKeyInfoEx(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg,
 DWORD dwFlags, void *pvAuxInfo, HCRYPTKEY *phKey)
{
    BOOL ret;
    DWORD pubKeySize = 0;

    TRACE("(%ld, %ld, %p, %d, %08lx, %p, %p)\n", hCryptProv,
     dwCertEncodingType, pInfo, aiKeyAlg, dwFlags, pvAuxInfo, phKey);

    ret = CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
     pInfo->PublicKey.pbData, pInfo->PublicKey.cbData, 0, NULL, &pubKeySize);
    if (ret)
    {
        LPBYTE pubKey = CryptMemAlloc(pubKeySize);

        if (pubKey)
        {
            ret = CryptDecodeObject(dwCertEncodingType, RSA_CSP_PUBLICKEYBLOB,
             pInfo->PublicKey.pbData, pInfo->PublicKey.cbData, 0, pubKey,
             &pubKeySize);
            if (ret)
                ret = CryptImportKey(hCryptProv, pubKey, pubKeySize, 0, 0,
                 phKey);
            CryptMemFree(pubKey);
        }
        else
            ret = FALSE;
    }
    return ret;
}

typedef BOOL (WINAPI *ImportPublicKeyInfoExFunc)(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg,
 DWORD dwFlags, void *pvAuxInfo, HCRYPTKEY *phKey);

BOOL WINAPI CryptImportPublicKeyInfoEx(HCRYPTPROV hCryptProv,
 DWORD dwCertEncodingType, PCERT_PUBLIC_KEY_INFO pInfo, ALG_ID aiKeyAlg,
 DWORD dwFlags, void *pvAuxInfo, HCRYPTKEY *phKey)
{
    static HCRYPTOIDFUNCSET set = NULL;
    BOOL ret;
    ImportPublicKeyInfoExFunc importFunc = NULL;
    HCRYPTOIDFUNCADDR hFunc = NULL;

    TRACE("(%ld, %ld, %p, %d, %08lx, %p, %p)\n", hCryptProv,
     dwCertEncodingType, pInfo, aiKeyAlg, dwFlags, pvAuxInfo, phKey);

    if (!set)
        set = CryptInitOIDFunctionSet(CRYPT_OID_IMPORT_PUBLIC_KEY_INFO_FUNC, 0);
    CryptGetOIDFunctionAddress(set, dwCertEncodingType,
     pInfo->Algorithm.pszObjId, 0, (void **)&importFunc, &hFunc);
    if (!importFunc)
        importFunc = CRYPT_ImportRsaPublicKeyInfoEx;
    ret = importFunc(hCryptProv, dwCertEncodingType, pInfo, aiKeyAlg, dwFlags,
     pvAuxInfo, phKey);
    if (hFunc)
        CryptFreeOIDFunctionAddress(hFunc, 0);
    return ret;
}
