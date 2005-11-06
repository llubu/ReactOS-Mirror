/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Imagehlp Libary
 * FILE:            lib/imagehlp/integrity.c
 * PURPOSE:         Image Integrity: Security Certificates and Checksums
 * PROGRAMMER:      Patrik Stridvall, Mike McCormack (WINE)
 */

/*
 * These functions are partially documented at:
 *   http://www.cs.auckland.ac.nz/~pgut001/pubs/authenticode.txt
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static
BOOL
IMAGEHLP_GetSecurityDirOffset(HANDLE handle, 
                              DWORD *pdwOfs,
                              DWORD *pdwSize)
{
    IMAGE_DOS_HEADER dos_hdr;
    IMAGE_NT_HEADERS nt_hdr;
    DWORD count;
    BOOL r;
    IMAGE_DATA_DIRECTORY *sd;
    DPRINT("handle %p\n", handle );

    /* read the DOS header */
    count = SetFilePointer( handle, 0, NULL, FILE_BEGIN );
    if( count == INVALID_SET_FILE_POINTER )
        return FALSE;
    count = 0;
    r = ReadFile( handle, &dos_hdr, sizeof dos_hdr, &count, NULL );
    if( !r )
        return FALSE;
    if( count != sizeof dos_hdr )
        return FALSE;

    /* read the PE header */
    count = SetFilePointer( handle, dos_hdr.e_lfanew, NULL, FILE_BEGIN );
    if( count == INVALID_SET_FILE_POINTER )
        return FALSE;
    count = 0;
    r = ReadFile( handle, &nt_hdr, sizeof nt_hdr, &count, NULL );
    if( !r )
        return FALSE;
    if( count != sizeof nt_hdr )
        return FALSE;

    sd = &nt_hdr.OptionalHeader.
                    DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];

    DPRINT("size = %lx addr = %lx\n", sd->Size, sd->VirtualAddress);
    *pdwSize = sd->Size;
    *pdwOfs = sd->VirtualAddress;

    return TRUE;
}

static
BOOL
IMAGEHLP_GetCertificateOffset(HANDLE handle,
                              DWORD num,
                              DWORD *pdwOfs,
                              DWORD *pdwSize)
{
    DWORD size, count, offset, len, sd_VirtualAddr;
    BOOL r;

    r = IMAGEHLP_GetSecurityDirOffset( handle, &sd_VirtualAddr, &size );
    if( !r )
        return FALSE;

    offset = 0;
    /* take the n'th certificate */
    while( 1 )
    {
        /* read the length of the current certificate */
        count = SetFilePointer( handle, sd_VirtualAddr + offset,
                                 NULL, FILE_BEGIN );
        if( count == INVALID_SET_FILE_POINTER )
            return FALSE;
        r = ReadFile( handle, &len, sizeof len, &count, NULL );
        if( !r )
            return FALSE;
        if( count != sizeof len )
            return FALSE;

        /* check the certificate is not too big or too small */
        if( len < sizeof len )
            return FALSE;
        if( len > (size-offset) )
            return FALSE;
        if( !num-- )
            break;

        /* calculate the offset of the next certificate */
        offset += len;
        if( offset >= size )
            return FALSE;
    }

    *pdwOfs = sd_VirtualAddr + offset;
    *pdwSize = len;

    DPRINT("len = %lx addr = %lx\n", len, sd_VirtualAddr + offset);
    return TRUE;
}

static
WORD
CalcCheckSum(DWORD StartValue,
             LPVOID BaseAddress,
             DWORD WordCount)
{
   LPWORD Ptr;
   DWORD Sum;
   DWORD i;

   Sum = StartValue;
   Ptr = (LPWORD)BaseAddress;
   for (i = 0; i < WordCount; i++)
     {
    Sum += *Ptr;
    if (HIWORD(Sum) != 0)
      {
         Sum = LOWORD(Sum) + HIWORD(Sum);
      }
    Ptr++;
     }

   return (WORD)(LOWORD(Sum) + HIWORD(Sum));
}

/*
 * @unimplemented
 */
BOOL
IMAGEAPI
ImageAddCertificate(HANDLE FileHandle,
                    LPWIN_CERTIFICATE Certificate,
                    PDWORD Index)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
IMAGEAPI
ImageEnumerateCertificates(HANDLE FileHandle,
                           WORD TypeFilter,
                           PDWORD CertificateCount,
                           PDWORD Indices,
                           DWORD IndexCount)
{
    DWORD size, count, offset, sd_VirtualAddr;
    WIN_CERTIFICATE hdr;
    const size_t cert_hdr_size = sizeof hdr - sizeof hdr.bCertificate;
    BOOL r;

    DPRINT("%p %hd %p %p %ld\n",
           FileHandle, TypeFilter, CertificateCount, Indices, IndexCount);

    if( Indices )
    {
        DPRINT1("Indicies not FileHandled!\n");
        return FALSE;
    }

    r = IMAGEHLP_GetSecurityDirOffset( FileHandle, &sd_VirtualAddr, &size );
    if( !r )
        return FALSE;

    offset = 0;
    *CertificateCount = 0;
    while( offset < size )
    {
        /* read the length of the current certificate */
        count = SetFilePointer( FileHandle, sd_VirtualAddr + offset,
                                 NULL, FILE_BEGIN );
        if( count == INVALID_SET_FILE_POINTER )
            return FALSE;
        r = ReadFile( FileHandle, &hdr, (DWORD)cert_hdr_size, &count, NULL );
        if( !r )
            return FALSE;
        if( count != cert_hdr_size )
            return FALSE;

        DPRINT("Size = %08lx  id = %08hx\n", hdr.dwLength, hdr.wCertificateType );

        /* check the certificate is not too big or too small */
        if( hdr.dwLength < cert_hdr_size )
            return FALSE;
        if( hdr.dwLength > (size-offset) )
            return FALSE;
       
        if( (TypeFilter == CERT_SECTION_TYPE_ANY) ||
            (TypeFilter == hdr.wCertificateType) )
        {
            (*CertificateCount)++;
        }

        /* next certificate */
        offset += hdr.dwLength;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
IMAGEAPI
ImageGetCertificateData(HANDLE handle,
                        DWORD Index,
                        LPWIN_CERTIFICATE Certificate,
                        PDWORD RequiredLength)
{
    DWORD r, offset, ofs, size, count;
    DPRINT("%p %ld %p %p\n", handle, Index, Certificate, RequiredLength);

    if( !IMAGEHLP_GetCertificateOffset( handle, Index, &ofs, &size ) )
        return FALSE;

    if( !Certificate )
    {
        *RequiredLength = size;
        return TRUE;
    }

    if( *RequiredLength < size )
    {
        *RequiredLength = size;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *RequiredLength = size;

    offset = SetFilePointer( handle, ofs, NULL, FILE_BEGIN );
    if( offset == INVALID_SET_FILE_POINTER )
        return FALSE;

    r = ReadFile( handle, Certificate, size, &count, NULL );
    if( !r )
        return FALSE;
    if( count != size )
        return FALSE;

    DPRINT("OK\n");
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
IMAGEAPI
ImageGetCertificateHeader(HANDLE FileHandle,
                          DWORD CertificateIndex,
                          LPWIN_CERTIFICATE Certificateheader)
{
    DWORD r, offset, ofs, size, count;
    const size_t cert_hdr_size = sizeof *Certificateheader -
                                 sizeof Certificateheader->bCertificate;

    DPRINT("%p %ld %p\n", FileHandle, CertificateIndex, Certificateheader);

    if( !IMAGEHLP_GetCertificateOffset( FileHandle, CertificateIndex, &ofs, &size ) )
        return FALSE;

    if( size < cert_hdr_size )
        return FALSE;

    offset = SetFilePointer( FileHandle, ofs, NULL, FILE_BEGIN );
    if( offset == INVALID_SET_FILE_POINTER )
        return FALSE;

    r = ReadFile( FileHandle, Certificateheader, (DWORD)cert_hdr_size, &count, NULL );
    if( !r )
        return FALSE;
    if( count != cert_hdr_size )
        return FALSE;

    DPRINT("OK\n");
    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
IMAGEAPI
ImageGetDigestStream(HANDLE FileHandle,
                     DWORD DigestLevel,
                     DIGEST_FUNCTION DigestFunction,
                     DIGEST_HANDLE DigestHandle)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
IMAGEAPI
ImageRemoveCertificate(HANDLE FileHandle,
                       DWORD Index)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @implemented
 */
PIMAGE_NT_HEADERS
IMAGEAPI
CheckSumMappedFile(LPVOID BaseAddress,
                   DWORD FileLength,
                   LPDWORD HeaderSum,
                   LPDWORD CheckSum)
{
  PIMAGE_NT_HEADERS Header;
  DWORD CalcSum;
  DWORD HdrSum;
  DPRINT("stub\n");

  CalcSum = (DWORD)CalcCheckSum(0,
                BaseAddress,
                (FileLength + 1) / sizeof(WORD));

  Header = ImageNtHeader(BaseAddress);
  HdrSum = Header->OptionalHeader.CheckSum;

  /* Subtract image checksum from calculated checksum. */
  /* fix low word of checksum */
  if (LOWORD(CalcSum) >= LOWORD(HdrSum))
  {
    CalcSum -= LOWORD(HdrSum);
  }
  else
  {
    CalcSum = ((LOWORD(CalcSum) - LOWORD(HdrSum)) & 0xFFFF) - 1;
  }

   /* fix high word of checksum */
  if (LOWORD(CalcSum) >= HIWORD(HdrSum))
  {
    CalcSum -= HIWORD(HdrSum);
  }
  else
  {
    CalcSum = ((LOWORD(CalcSum) - HIWORD(HdrSum)) & 0xFFFF) - 1;
  }

  /* add file length */
  CalcSum += FileLength;

  *CheckSum = CalcSum;
  *HeaderSum = Header->OptionalHeader.CheckSum;

  return Header;
}

/*
 * @implemented
 */
DWORD
IMAGEAPI
MapFileAndCheckSumA(LPSTR Filename,
                    LPDWORD HeaderSum,
                    LPDWORD CheckSum)
{
  HANDLE hFile;
  HANDLE hMapping;
  LPVOID BaseAddress;
  DWORD FileLength;

  DPRINT("(%s, %p, %p): stub\n", Filename, HeaderSum, CheckSum);

  hFile = CreateFileA(Filename,
              GENERIC_READ,
              FILE_SHARE_READ | FILE_SHARE_WRITE,
              NULL,
              OPEN_EXISTING,
              FILE_ATTRIBUTE_NORMAL,
              0);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    return CHECKSUM_OPEN_FAILURE;
  }

  hMapping = CreateFileMappingW(hFile,
                   NULL,
                   PAGE_READONLY,
                   0,
                   0,
                   NULL);
  if (hMapping == 0)
  {
    CloseHandle(hFile);
    return CHECKSUM_MAP_FAILURE;
  }

  BaseAddress = MapViewOfFile(hMapping,
                  FILE_MAP_READ,
                  0,
                  0,
                  0);
  if (hMapping == 0)
  {
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return CHECKSUM_MAPVIEW_FAILURE;
  }

  FileLength = GetFileSize(hFile,
               NULL);

  CheckSumMappedFile(BaseAddress,
             FileLength,
             HeaderSum,
             CheckSum);

  UnmapViewOfFile(BaseAddress);
  CloseHandle(hMapping);
  CloseHandle(hFile);

  return 0;
}

/*
 * @implemented
 */
DWORD
IMAGEAPI
MapFileAndCheckSumW(LPWSTR Filename,
                    LPDWORD HeaderSum,
                    LPDWORD CheckSum)
{
  HANDLE hFile;
  HANDLE hMapping;
  LPVOID BaseAddress;
  DWORD FileLength;

  DPRINT("(%S, %p, %p): stub\n", Filename, HeaderSum, CheckSum);

  hFile = CreateFileW(Filename,
              GENERIC_READ,
              FILE_SHARE_READ | FILE_SHARE_WRITE,
              NULL,
              OPEN_EXISTING,
              FILE_ATTRIBUTE_NORMAL,
              0);
  if (hFile == INVALID_HANDLE_VALUE)
  {
  return CHECKSUM_OPEN_FAILURE;
  }

  hMapping = CreateFileMappingW(hFile,
                   NULL,
                   PAGE_READONLY,
                   0,
                   0,
                   NULL);
  if (hMapping == 0)
  {
    CloseHandle(hFile);
    return CHECKSUM_MAP_FAILURE;
  }

  BaseAddress = MapViewOfFile(hMapping,
                  FILE_MAP_READ,
                  0,
                  0,
                  0);
  if (hMapping == 0)
  {
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return CHECKSUM_MAPVIEW_FAILURE;
  }

  FileLength = GetFileSize(hFile,
               NULL);

  CheckSumMappedFile(BaseAddress,
             FileLength,
             HeaderSum,
             CheckSum);

  UnmapViewOfFile(BaseAddress);
  CloseHandle(hMapping);
  CloseHandle(hFile);

  return 0;
}
