#ifndef __INCLUDE_NAPI_TYPES_H
#define __INCLUDE_NAPI_TYPES_H

/* these should be moved to a file like ntdef.h */

enum
{
   DIRECTORY_QUERY,
   DIRECTORY_TRAVERSE,
   DIRECTORY_CREATE_OBJECT,
   DIRECTORY_CREATE_SUBDIRECTORY,
   DIRECTORY_ALL_ACCESS,
};

/*
 * General type for status information
 */

typedef enum _NT_PRODUCT_TYPE
{
   NtProductWinNt = 1,
   NtProductLanManNt,
   NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

typedef ULARGE_INTEGER TIME, *PTIME;

#ifndef __USE_W32API

typedef const int CINT;
typedef LONG NTSTATUS, *PNTSTATUS;
typedef ULONG DEVICE_TYPE;

/*  File information for IRP_MJ_QUERY_INFORMATION (and SET)  */
typedef enum _FILE_INFORMATION_CLASS
{
  FileDirectoryInformation = 1,
  FileFullDirectoryInformation,
  FileBothDirectoryInformation,
  FileBasicInformation,
  FileStandardInformation,
  FileInternalInformation,
  FileEaInformation,
  FileAccessInformation,
  FileNameInformation,
  FileRenameInformation,
  FileLinkInformation,
  FileNamesInformation,
  FileDispositionInformation,
  FilePositionInformation,
  FileFullEaInformation,
  FileModeInformation,
  FileAlignmentInformation,
  FileAllInformation,
  FileAllocationInformation,
  FileEndOfFileInformation,
  FileAlternateNameInformation,
  FileStreamInformation,
  FilePipeInformation,
  FilePipeLocalInformation,
  FilePipeRemoteInformation,
  FileMailslotQueryInformation,
  FileMailslotSetInformation,
  FileCompressionInformation,
  FileCopyOnWriteInformation,
  FileCompletionInformation,
  FileMoveClusterInformation,
  FileOleClassIdInformation,
  FileOleStateBitsInformation,
  FileNetworkOpenInformation,
  FileObjectIdInformation,
  FileOleAllInformation,
  FileOleDirectoryInformation,
  FileContentIndexInformation,
  FileInheritContentIndexInformation,
  FileOleInformation,
  FileMaximumInformation,
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

#endif /* !__USE_W32API */

#endif /* __INCLUDE_NAPI_TYPES_H */
