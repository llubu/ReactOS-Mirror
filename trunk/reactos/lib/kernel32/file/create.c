/* $Id: create.c,v 1.36 2004/07/01 22:36:16 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  Removed use of SearchPath (not used by Windows)
 *                  18/08/2002: CreateFileW mess cleaned up (KJK::Hyperion)
 *                  24/08/2002: removed superfluous DPRINTs (KJK::Hyperion)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL CreateFileA (LPCSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   UNICODE_STRING FileNameU;
   ANSI_STRING FileName;
   HANDLE FileHandle;
   
   DPRINT("CreateFileA(lpFileName %s)\n",lpFileName);
   
   RtlInitAnsiString (&FileName,
		      (LPSTR)lpFileName);
   
   /* convert ansi (or oem) string to unicode */
   if (bIsFileApiAnsi)
     RtlAnsiStringToUnicodeString (&FileNameU,
				   &FileName,
				   TRUE);
   else
     RtlOemStringToUnicodeString (&FileNameU,
				  &FileName,
				  TRUE);

   FileHandle = CreateFileW (FileNameU.Buffer,
			     dwDesiredAccess,
			     dwShareMode,
			     lpSecurityAttributes,
			     dwCreationDisposition,
			     dwFlagsAndAttributes,
			     hTemplateFile);
   
   RtlFreeHeap (RtlGetProcessHeap (),
		0,
		FileNameU.Buffer);
   
   return FileHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL CreateFileW (LPCWSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING NtPathU;
   HANDLE FileHandle;
   NTSTATUS Status;
   ULONG Flags = 0;
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;

   DPRINT("CreateFileW(lpFileName %S)\n",lpFileName);

   if(hTemplateFile != NULL)
   {
    /* FIXME */
    DPRINT("Template file feature not supported yet\n");
    SetLastError(ERROR_NOT_SUPPORTED);
    return INVALID_HANDLE_VALUE;
   }

   /* validate & translate the creation disposition */
   switch (dwCreationDisposition)
     {
      case CREATE_NEW:
	dwCreationDisposition = FILE_CREATE;
	break;
	
      case CREATE_ALWAYS:
	dwCreationDisposition = FILE_OVERWRITE_IF;
	break;
	
      case OPEN_EXISTING:
	dwCreationDisposition = FILE_OPEN;
	break;
	
      case OPEN_ALWAYS:
	dwCreationDisposition = FILE_OPEN_IF;
	break;

      case TRUNCATE_EXISTING:
	dwCreationDisposition = FILE_OVERWRITE;
        break;
      
      default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return (INVALID_HANDLE_VALUE);
     }

   /* validate & translate the filename */
   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
				      &NtPathU,
				      NULL,
				      NULL))
   {
     DPRINT("Invalid path\n");
     SetLastError(ERROR_BAD_PATHNAME);
     return INVALID_HANDLE_VALUE;
   }
   
   DPRINT("NtPathU \'%S\'\n", NtPathU.Buffer);

  /* validate & translate the flags */

   /* translate the flags that need no validation */
  if (!(dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED)){
    /* yes, nonalert is correct! apc's are not delivered
    while waiting for file io to complete */
    Flags |= FILE_SYNCHRONOUS_IO_NONALERT;
  }
   
   if(dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH)
    Flags |= FILE_WRITE_THROUGH;

   if(dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
    Flags |= FILE_NO_INTERMEDIATE_BUFFERING;

   if(dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS)
    Flags |= FILE_RANDOM_ACCESS;
   
   if(dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN)
    Flags |= FILE_SEQUENTIAL_ONLY;
   
   if(dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE)
    Flags |= FILE_DELETE_ON_CLOSE;
   
   if(dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS)
   {
    if(dwDesiredAccess & GENERIC_ALL)
      Flags |= FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_FOR_RECOVERY;
    else
    {
      if(dwDesiredAccess & GENERIC_READ)
        Flags |= FILE_OPEN_FOR_BACKUP_INTENT;
      
      if(dwDesiredAccess & GENERIC_WRITE)
        Flags |= FILE_OPEN_FOR_RECOVERY;
    }
   }
   else
    Flags |= FILE_NON_DIRECTORY_FILE;
    
    
  /* handle may allways be waited on and querying attributes are allways allowed */
  dwDesiredAccess |= SYNCHRONIZE|FILE_READ_ATTRIBUTES; 

   /* FILE_FLAG_POSIX_SEMANTICS is handled later */

#if 0
   /* FIXME: Win32 constants to be defined */
   if(dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT)
    Flags |= FILE_OPEN_REPARSE_POINT;
   
   if(dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL)
    Flags |= FILE_OPEN_NO_RECALL;
#endif

   /* check for console output */
   if (0 == _wcsicmp(L"CONOUT$", lpFileName))
   {
      /* FIXME: Send required access rights to Csrss */
      Request.Type = CSRSS_GET_OUTPUT_HANDLE;
      Status = CsrClientCallServer(&Request,
			           &Reply,
			           sizeof(CSRSS_API_REQUEST),
			           sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
      {
         SetLastErrorByStatus(Status);
	 return INVALID_HANDLE_VALUE;
      }
      else
      {
         return Reply.Data.GetOutputHandleReply.OutputHandle;
      }
   }

   /* check for console input */
   if (0 == _wcsicmp(L"CONIN$", lpFileName))
   {
      /* FIXME: Send required access rights to Csrss */
      Request.Type = CSRSS_GET_INPUT_HANDLE;
      Status = CsrClientCallServer(&Request,
			           &Reply,
			           sizeof(CSRSS_API_REQUEST),
			           sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
      {
         SetLastErrorByStatus(Status);
	 return INVALID_HANDLE_VALUE;
      }
      else
      {
         return Reply.Data.GetInputHandleReply.InputHandle;
      }
   }

   /* build the object attributes */
   InitializeObjectAttributes(
    &ObjectAttributes,
    &NtPathU,
    0,
    NULL,
    NULL
   );

   if (lpSecurityAttributes)
   {
      if(lpSecurityAttributes->bInheritHandle)
         ObjectAttributes.Attributes |= OBJ_INHERIT;

      ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
   }
   
   if(!(dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS))
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

   /* perform the call */
   Status = NtCreateFile (&FileHandle,
			  dwDesiredAccess,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  NULL,
			  dwFlagsAndAttributes,
			  dwShareMode,
			  dwCreationDisposition,
			  Flags,
			  NULL,
			  0);

   RtlFreeUnicodeString(&NtPathU);

   /* error */
  if (!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus (Status);
    return INVALID_HANDLE_VALUE;
  }
   
  /*
  create with OPEN_ALWAYS (FILE_OPEN_IF) returns info = FILE_OPENED or FILE_CREATED
  create with CREATE_ALWAYS (FILE_OVERWRITE_IF) returns info = FILE_OVERWRITTEN or FILE_CREATED
  */    
  if ((dwCreationDisposition == FILE_OPEN_IF && IoStatusBlock.Information == FILE_OPENED) ||
      (dwCreationDisposition == FILE_OVERWRITE_IF && IoStatusBlock.Information == FILE_OVERWRITTEN))
  {
    SetLastError(ERROR_ALREADY_EXISTS);
  }

  return FileHandle;
}

/* EOF */
