/* $Id: fstat.c,v 1.11 2002/09/07 15:12:37 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/sys/fstat.c
 * PURPOSE:     Gather file information
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrti.h>


int _fstat(int fd, struct _stat *statbuf)
{
  BY_HANDLE_FILE_INFORMATION  FileInformation;
  DWORD dwFileType;
  void* handle;

  if (!statbuf)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if ((void*)-1 == (handle = (HANDLE)_get_osfhandle(fd)))
  {
    __set_errno(EBADF);
    return -1;
  }

  fflush(NULL);

  memset (statbuf, 0, sizeof(struct stat));

  dwFileType = GetFileType(handle);

  if (dwFileType == FILE_TYPE_DISK)
  {
    if (!GetFileInformationByHandle(handle,&FileInformation))
    {
      __set_errno(EBADF);
      return -1;
    }
    statbuf->st_ctime = FileTimeToUnixTime(&FileInformation.ftCreationTime,NULL);
    statbuf->st_atime = FileTimeToUnixTime(&FileInformation.ftLastAccessTime,NULL);
    statbuf->st_mtime = FileTimeToUnixTime(&FileInformation.ftLastWriteTime,NULL);

    statbuf->st_dev = fd;
    statbuf->st_size = FileInformation.nFileSizeLow;
    statbuf->st_mode = S_IREAD;
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
      statbuf->st_mode |= S_IFDIR;
    else
      statbuf->st_mode |= S_IFREG;
    if (!(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) statbuf->st_mode |= S_IWRITE;
  }
  else if (dwFileType == FILE_TYPE_CHAR)
  {
    statbuf->st_dev = fd;
    statbuf->st_mode = S_IFCHR;
  }
  else if (dwFileType == FILE_TYPE_PIPE)
  {
    statbuf->st_dev = fd;
    statbuf->st_mode = S_IFIFO;
  }
  else
  {
    // dwFileType is FILE_TYPE_UNKNOWN or has a bad value
    __set_errno(EBADF);
    return -1;
  }
  return 0;
}

int _fstati64 (int fd, struct _stati64* statbuf)
{
  BY_HANDLE_FILE_INFORMATION FileInformation;
  DWORD dwFileType;
  HANDLE handle;

  if (!statbuf)
  {
    __set_errno(EINVAL);
    return -1;
  }

  if ((HANDLE)-1 == (handle = (HANDLE)_get_osfhandle(fd)))
  {
    __set_errno(EBADF);
    return -1;
  }
  
  fflush(NULL);

  memset(statbuf, 0, sizeof(struct _stati64));

  dwFileType = GetFileType(handle);

  if (dwFileType == FILE_TYPE_DISK)
  {
    if (!GetFileInformationByHandle(handle,&FileInformation))
    {
       __set_errno(EBADF);
       return -1;
    }
    statbuf->st_ctime = FileTimeToUnixTime(&FileInformation.ftCreationTime,NULL);
    statbuf->st_atime = FileTimeToUnixTime(&FileInformation.ftLastAccessTime,NULL);
    statbuf->st_mtime = FileTimeToUnixTime(&FileInformation.ftLastWriteTime,NULL);

    statbuf->st_dev = fd;
    statbuf->st_size = (((__int64)FileInformation.nFileSizeHigh) << 32) +
		     FileInformation.nFileSizeLow;
    statbuf->st_mode = S_IREAD;
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
      statbuf->st_mode |= S_IFDIR;
    else
      statbuf->st_mode |= S_IFREG;
    if (!(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) statbuf->st_mode |= S_IWRITE;
  }
  else if (dwFileType == FILE_TYPE_CHAR)
  {
    statbuf->st_dev = fd;
    statbuf->st_mode = S_IFCHR;
  }
  else if (dwFileType == FILE_TYPE_PIPE)
  {
    statbuf->st_dev = fd;
    statbuf->st_mode = S_IFIFO;
  }
  else
  {
    // dwFileType is FILE_TYPE_UNKNOWN or has a bad value
    __set_errno(EBADF);
    return -1;
  }
  return 0;
}


