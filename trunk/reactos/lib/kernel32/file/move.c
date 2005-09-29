/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Gerhard W. Gruber (sparhawk_at_gmx.at)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#include <malloc.h>

#define NDEBUG
#include "../include/debug.h"

/* GLOBALS *****************************************************************/

/* FUNCTIONS ****************************************************************/
static BOOL
RemoveReadOnlyAttributeW(IN LPCWSTR lpFileName)
{
    DWORD Attributes;
    Attributes = GetFileAttributesW(lpFileName);
    if (Attributes != INVALID_FILE_ATTRIBUTES)
    {	
        return SetFileAttributesW(lpFileName,Attributes - 
			                      (Attributes & ~FILE_ATTRIBUTE_READONLY));
    }
 
    return FALSE;
}


/***********************************************************************
 *           add_boot_rename_entry
 *
 * Adds an entry to the registry that is loaded when windows boots and
 * checks if there are some files to be removed or renamed/moved.
 * <fn1> has to be valid and <fn2> may be NULL. If both pointers are
 * non-NULL then the file is moved, otherwise it is deleted.  The
 * entry of the registrykey is always appended with two zero
 * terminated strings. If <fn2> is NULL then the second entry is
 * simply a single 0-byte. Otherwise the second filename goes
 * there. The entries are prepended with \??\ before the path and the
 * second filename gets also a '!' as the first character if
 * MOVEFILE_REPLACE_EXISTING is set. After the final string another
 * 0-byte follows to indicate the end of the strings.
 * i.e.:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * \??\D:\test\file2[0]
 * !\??\D:\test\file2_renamed[0]
 * [0]                        <- indicates end of strings
 *
 * or:
 * \??\D:\test\file1[0]
 * !\??\D:\test\file1_renamed[0]
 * \??\D:\Test|delete[0]
 * [0]                        <- file is to be deleted, second string empty
 * [0]                        <- indicates end of strings
 *
 */
static BOOL add_boot_rename_entry( LPCWSTR source, LPCWSTR dest, DWORD flags )
{
    static const WCHAR ValueName[] = {'P','e','n','d','i','n','g',
                                      'F','i','l','e','R','e','n','a','m','e',
                                      'O','p','e','r','a','t','i','o','n','s',0};
    static const WCHAR SessionW[] = {'M','a','c','h','i','n','e','\\',
                                     'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, source_name, dest_name;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    BOOL rc = FALSE;
    HANDLE Reboot = 0;
    DWORD len1, len2;
    DWORD DataSize = 0;
    BYTE *Buffer = NULL;
    WCHAR *p;

    DPRINT("Add support to smss for keys created by MOVEFILE_DELAY_UNTIL_REBOOT\n");

    if (!RtlDosPathNameToNtPathName_U( (LPWSTR)source, &source_name, NULL, NULL ))
    {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    dest_name.Buffer = NULL;
    if (dest && !RtlDosPathNameToNtPathName_U( (LPWSTR)dest, &dest_name, NULL, NULL ))
    {
        RtlFreeUnicodeString( &source_name );
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, SessionW );

    if (NtCreateKey( &Reboot, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) != STATUS_SUCCESS)
    {
        DPRINT1("Error creating key for reboot managment [%s]\n",
             "SYSTEM\\CurrentControlSet\\Control\\Session Manager");
        RtlFreeUnicodeString( &source_name );
        RtlFreeUnicodeString( &dest_name );
        return FALSE;
    }

    len1 = source_name.Length + sizeof(WCHAR);
    if (dest)
    {
        len2 = dest_name.Length + sizeof(WCHAR);
        if (flags & MOVEFILE_REPLACE_EXISTING)
            len2 += sizeof(WCHAR); /* Plus 1 because of the leading '!' */
    }
    else len2 = sizeof(WCHAR); /* minimum is the 0 characters for the empty second string */

    RtlInitUnicodeString( &nameW, ValueName );

    /* First we check if the key exists and if so how many bytes it already contains. */
    if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                         NULL, 0, &DataSize ) == STATUS_BUFFER_OVERFLOW)
    {
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
        if (NtQueryValueKey( Reboot, &nameW, KeyValuePartialInformation,
                             Buffer, DataSize, &DataSize )) goto Quit;
        info = (KEY_VALUE_PARTIAL_INFORMATION *)Buffer;
        if (info->Type != REG_MULTI_SZ) goto Quit;
        if (DataSize > sizeof(info)) DataSize -= sizeof(WCHAR);  /* remove terminating null (will be added back later) */
    }
    else
    {
        DataSize = info_size;
        if (!(Buffer = HeapAlloc( GetProcessHeap(), 0, DataSize + len1 + len2 + sizeof(WCHAR) )))
            goto Quit;
    }

    memcpy( Buffer + DataSize, source_name.Buffer, len1 );
    DataSize += len1;
    p = (WCHAR *)(Buffer + DataSize);
    if (dest)
    {
        if (flags & MOVEFILE_REPLACE_EXISTING)
            *p++ = '!';
        memcpy( p, dest_name.Buffer, len2 );
        DataSize += len2;
    }
    else
    {
        *p = 0;
        DataSize += sizeof(WCHAR);
    }

    /* add final null */
    p = (WCHAR *)(Buffer + DataSize);
    *p = 0;
    DataSize += sizeof(WCHAR);

    rc = !NtSetValueKey(Reboot, &nameW, 0, REG_MULTI_SZ, Buffer + info_size, DataSize - info_size);

 Quit:
    RtlFreeUnicodeString( &source_name );
    RtlFreeUnicodeString( &dest_name );
    if (Reboot) NtClose(Reboot);
    HeapFree( GetProcessHeap(), 0, Buffer );
    return(rc);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileWithProgressW (
	LPCWSTR			lpExistingFileName,
	LPCWSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	HANDLE hFile = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
	PFILE_RENAME_INFORMATION FileRename;
	NTSTATUS errCode;
	BOOL Result;
	UNICODE_STRING DstPathU;
	BOOL folder = FALSE;

	DPRINT("MoveFileWithProgressW()\n");

	if (dwFlags & MOVEFILE_DELAY_UNTIL_REBOOT)
		return add_boot_rename_entry( lpExistingFileName, lpNewFileName, dwFlags );

	hFile = CreateFileW (lpExistingFileName,
	                     GENERIC_ALL,
	                     FILE_SHARE_WRITE|FILE_SHARE_READ,
	                     NULL,
	                     OPEN_EXISTING,
	                     FILE_FLAG_BACKUP_SEMANTICS,
	                     NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
	   return FALSE;
	}

	
        /* validate & translate the filename */
        if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpNewFileName,
				           &DstPathU,
				           NULL,
				           NULL))
        {
           DPRINT("Invalid destination path\n");
	   CloseHandle(hFile);
           SetLastError(ERROR_PATH_NOT_FOUND);
           return FALSE;
        }

	FileRename = alloca(sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length);
	if ((dwFlags & MOVEFILE_REPLACE_EXISTING) == MOVEFILE_REPLACE_EXISTING)
		FileRename->ReplaceIfExists = TRUE;
	else
		FileRename->ReplaceIfExists = FALSE;

	memcpy(FileRename->FileName, DstPathU.Buffer, DstPathU.Length);
        RtlFreeHeap (RtlGetProcessHeap (),
		     0,
		     DstPathU.Buffer);
	/*
	 * FIXME:
	 *   Is the length the count of characters or the length of the buffer?
	 */
	FileRename->FileNameLength = DstPathU.Length / sizeof(WCHAR);
	errCode = NtSetInformationFile (hFile,
	                                &IoStatusBlock,
	                                FileRename,
	                                sizeof(FILE_RENAME_INFORMATION) + DstPathU.Length,
	                                FileRenameInformation);
	CloseHandle(hFile);

	if (GetFileAttributesW(lpExistingFileName) & FILE_ATTRIBUTE_DIRECTORY)
	{
           folder = TRUE;
	}

	
	/*
	 *  FIXME:
	 *  Fail now move the folder 
	 *  Before we fail at CreateFileW 
	 */
	 
	 
	if (NT_SUCCESS(errCode))
	{
		Result = TRUE;
	}
	else 
	{
 	        if (folder==FALSE)
		{
		    Result = CopyFileExW (lpExistingFileName,
		                      lpNewFileName,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      FileRename->ReplaceIfExists ? 0 : COPY_FILE_FAIL_IF_EXISTS);
 		    if (Result)
		    {
			/* Cleanup the source file */			
	                Result = DeleteFileW (lpExistingFileName);
		    } 
                  }
		 else
		 {
		   /* move folder code start */
		   WIN32_FIND_DATAW findBuffer;
		   LPWSTR lpExistingFileName2 = NULL;
		   LPWSTR lpNewFileName2 = NULL; 
		   LPWSTR lpDeleteFile = NULL;
		   INT size;
		   INT size2;
		   BOOL loop = TRUE;
		   BOOL Result = FALSE;
		   INT max_size = MAX_PATH;


		   		   /* Build the string */
		   size = wcslen(lpExistingFileName); 
		   if (size+6> max_size)
			   max_size = size + 6;

		   lpDeleteFile = (LPWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,max_size * sizeof(WCHAR));
		   if (lpDeleteFile == NULL)		   
		       return FALSE;		  		  

		   lpNewFileName2 = (LPWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,max_size * sizeof(WCHAR));
		   if (lpNewFileName2 == NULL)
		   {		
		     HeapFree(GetProcessHeap(),0,(VOID *)  lpDeleteFile);
			 return FALSE;
		   }

		   lpExistingFileName2 = (LPWSTR) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,max_size * sizeof(WCHAR));
		   if (lpNewFileName2 == NULL)
		   {		
		     HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);		  	  		    		
		     HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		     return FALSE;
		   }		   		   
		
		   wcscpy( (WCHAR *)lpExistingFileName2,lpExistingFileName);
		   wcscpy( (WCHAR *)&lpExistingFileName2[size],L"\\*.*\0");
	  
		   /* Get the file name */
		   memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
		   hFile = FindFirstFileW(lpExistingFileName2,  &findBuffer);
		   if (hFile == NULL) 
		       loop=FALSE;

		   if (findBuffer.cFileName[0] == L'\0')
		       loop=FALSE;
		

		    /* FIXME 
			 * remove readonly flag from source folder and do not set the readonly flag to dest folder 
			 */
		   RemoveReadOnlyAttributeW(lpExistingFileName);
           RemoveReadOnlyAttributeW(lpNewFileName);
		   //CreateDirectoryExW(lpExistingFileName,lpNewFileName,NULL);
		   CreateDirectoryW(lpNewFileName, NULL);
		  		   
		   /* search the files/folders and move them */
		   while (loop==TRUE)
		   {	
		     Result = TRUE;

		     if ((!wcscmp(findBuffer.cFileName,L"..")) || (!wcscmp(findBuffer.cFileName,L".")))
		     {		  
		       loop = FindNextFileW(hFile, &findBuffer);		  
		  		  
		       if (!loop)
		       {					 
		         size = wcslen(lpExistingFileName2)-4;
		         FindClose(hFile);
		         wcscpy( &lpExistingFileName2[size],L"\0");

		         if (wcsncmp(lpExistingFileName,lpExistingFileName2,size))
		         {  
				   DWORD Attributes;

		           FindClose(hFile);			     

				   /* delete folder */					  				 
				   DPRINT("MoveFileWithProgressW : Delete folder : %S\n",lpDeleteFile);

				   /* remove system folder flag other wise we can not delete the folder */
				   Attributes = GetFileAttributesW(lpExistingFileName2);
                   if (Attributes != INVALID_FILE_ATTRIBUTES)
                   {	
                     SetFileAttributesW(lpExistingFileName2,(Attributes & ~FILE_ATTRIBUTE_SYSTEM));
				   }
				   
				   RemoveReadOnlyAttributeW(lpExistingFileName2);
					 				   
				   Result = RemoveDirectoryW(lpExistingFileName2);
				   if (Result == FALSE)
				       break;
				 				 
				   loop=TRUE;				 				 
				   size = wcslen(lpExistingFileName); 
				
				   if (size+6>max_size)
				   {				       
		              if (lpNewFileName2 != NULL)		          		
		                  HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		           
		              if (lpExistingFileName2 != NULL)		          	  
		                  HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		           		   
		              if (lpDeleteFile != NULL)		   		
		                  HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		          
				      return FALSE;
				   }

				   wcscpy( lpExistingFileName2,lpExistingFileName);
				   wcscpy( &lpExistingFileName2[size],L"\\*.*\0");

				   /* Get the file name */
				   memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
				   hFile = FindFirstFileW(lpExistingFileName2, &findBuffer);	                 
		         }
               } 		  
               continue;		  		  
             }
	  	  	  
		     if (findBuffer.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		     {	               

               /* Build the new src string */		  		  		  
               size = wcslen(findBuffer.cFileName); 
               size2= wcslen(lpExistingFileName2);

               if (size2+size+6>max_size)
               {
	              FindClose(hFile);    		 

		         if (lpNewFileName2 != NULL)		          		
		              HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		           
		         if (lpExistingFileName2 != NULL)		          	  
		              HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		           		   
		         if (lpDeleteFile != NULL)		   		
		              HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		          
				 return FALSE;			      				 
	           }

	            wcscpy( &lpExistingFileName2[size2-3],findBuffer.cFileName);			   			  
                wcscpy( &lpExistingFileName2[size2+size-3],L"\0");
			   

			   /* Continue */
			   wcscpy( lpDeleteFile,lpExistingFileName2);
	           wcscpy( &lpExistingFileName2[size2+size-3],L"\\*.*\0");
          
		  
	           /* Build the new dst string */
	           size = wcslen(lpExistingFileName2) + wcslen(lpNewFileName);
	           size2 = wcslen(lpExistingFileName);
		  
	           if (size>max_size)
	           {
	             FindClose(hFile);    		 

		         if (lpNewFileName2 != NULL)		          		
		              HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		           
		         if (lpExistingFileName2 != NULL)		          	  
		              HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		           		   
		         if (lpDeleteFile != NULL)		   		
		              HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		          
				 return FALSE;			     
	           }

	           wcscpy( lpNewFileName2,lpNewFileName);		 		  
	           size = wcslen(lpNewFileName);		 
	           wcscpy( &lpNewFileName2[size], &lpExistingFileName2[size2]);
	           size = wcslen(lpNewFileName2);
	           wcscpy( &lpNewFileName2[size-4],L"\0");
	           
	           /* Create Folder */	          

			   /* FIXME 
			    * remove readonly flag from source folder and do not set the readonly flag to dest folder 
				*/
			   RemoveReadOnlyAttributeW(lpDeleteFile);
			   RemoveReadOnlyAttributeW(lpNewFileName2);

			   CreateDirectoryW(lpNewFileName2,NULL);
	           //CreateDirectoryExW(lpDeleteFile, lpNewFileName2,NULL);
			   

			   /* set new search path  from src string */
	           FindClose(hFile);
	           memset(&findBuffer,0,sizeof(WIN32_FIND_DATAW));
	           hFile = FindFirstFileW(lpExistingFileName2, &findBuffer);
	         }
		     else
		     {
		  
	           /* Build the new string */		  		  		  		  
	           size = wcslen(findBuffer.cFileName); 
	           size2= wcslen(lpExistingFileName2);
	           wcscpy( lpDeleteFile,lpExistingFileName2);	   
	           wcscpy( &lpDeleteFile[size2-3],findBuffer.cFileName);	   
		  
	           /* Build dest string */
	           size = wcslen(lpDeleteFile) + wcslen(lpNewFileName);
	           size2 = wcslen(lpExistingFileName);

	           if (size>max_size)
	           {                 				  
			      FindClose(hFile);    		 

		          if (lpNewFileName2 != NULL)		          		
		              HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		           
		          if (lpExistingFileName2 != NULL)		          	  
		              HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		           		   
		          if (lpDeleteFile != NULL)		   		
		              HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		          
				  return FALSE;
			   }
		  
               wcscpy( lpNewFileName2,lpNewFileName);		 		  
               size = wcslen(lpNewFileName);		 
               wcscpy(&lpNewFileName2[size],&lpDeleteFile[size2]);
		 
              
			   /* overrite existsen file, if the file got the flag have readonly 
			    * we need reomve that flag 
				*/
			   
			    /* copy file */
			   
			   DPRINT("MoveFileWithProgressW : Copy file : %S to %S\n",lpDeleteFile, lpNewFileName2);
			   RemoveReadOnlyAttributeW(lpDeleteFile);
			   RemoveReadOnlyAttributeW(lpNewFileName2);
			  
			   Result = CopyFileExW (lpDeleteFile,
		                      lpNewFileName2,
		                      lpProgressRoutine,
		                      lpData,
		                      NULL,
		                      0);

			   if (Result == FALSE)                                
                   break;
              
               /* delete file */               		            
			   DPRINT("MoveFileWithProgressW : remove readonly flag from file : %S\n",lpNewFileName2);
			   Result = RemoveReadOnlyAttributeW(lpDeleteFile);
			   if (Result == FALSE)
			       break;

               DPRINT("MoveFileWithProgressW : Delete file : %S\n",lpDeleteFile);
			   Result = DeleteFileW(lpDeleteFile);
               if (Result == FALSE)                               
                   break;
              
             }	  	              
             loop = FindNextFileW(hFile, &findBuffer);	  	  
           }

           
		   /* Remove last folder */
           if ((loop == FALSE) && (Result != FALSE))
		   {
		     DWORD Attributes;

		     FindClose(hFile);				 
			 Attributes = GetFileAttributesW(lpDeleteFile);
             if (Attributes != INVALID_FILE_ATTRIBUTES)
             {	
                SetFileAttributesW(lpDeleteFile,(Attributes & ~FILE_ATTRIBUTE_SYSTEM));
			 }
					 				   				 
		     Result = RemoveDirectoryW(lpExistingFileName);		     
		   }
           
		   /* Cleanup */
		   FindClose(hFile);    
		   
		   if (lpNewFileName2 != NULL)
		   {		
		     HeapFree(GetProcessHeap(),0,(VOID *)  lpNewFileName2);
		     lpNewFileName2 = NULL;
		   }

		   if (lpExistingFileName2 != NULL)
		   {	  
		     HeapFree(GetProcessHeap(),0,(VOID *) lpExistingFileName2);		
		     lpExistingFileName2 = NULL;
		   }

		   if (lpDeleteFile != NULL)
		   {		
		     HeapFree(GetProcessHeap(),0,(VOID *) lpDeleteFile);		
		     lpDeleteFile = NULL;
		   }

           return Result;

		   // end move folder code		 
		  }
	}
	
	
	return Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileWithProgressA (
	LPCSTR			lpExistingFileName,
	LPCSTR			lpNewFileName,
	LPPROGRESS_ROUTINE	lpProgressRoutine,
	LPVOID			lpData,
	DWORD			dwFlags
	)
{
	PWCHAR ExistingFileNameW;
   PWCHAR NewFileNameW;
	BOOL ret;

   if (!(ExistingFileNameW = FilenameA2W(lpExistingFileName, FALSE)))
      return FALSE;

   if (!(NewFileNameW= FilenameA2W(lpNewFileName, TRUE)))
      return FALSE;

   ret = MoveFileWithProgressW (ExistingFileNameW ,
                                   NewFileNameW,
	                                lpProgressRoutine,
	                                lpData,
	                                dwFlags);

   RtlFreeHeap (RtlGetProcessHeap (), 0, NewFileNameW);

	return ret;
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName
	)
{
	return MoveFileExW (lpExistingFileName,
	                    lpNewFileName,
	                    MOVEFILE_COPY_ALLOWED);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileExW (
	LPCWSTR	lpExistingFileName,
	LPCWSTR	lpNewFileName,
	DWORD	dwFlags
	)
{
	return MoveFileWithProgressW (lpExistingFileName,
	                              lpNewFileName,
	                              NULL,
	                              NULL,
	                              dwFlags);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName
	)
{
	return MoveFileExA (lpExistingFileName,
	                    lpNewFileName,
	                    MOVEFILE_COPY_ALLOWED);
}


/*
 * @implemented
 */
BOOL
STDCALL
MoveFileExA (
	LPCSTR	lpExistingFileName,
	LPCSTR	lpNewFileName,
	DWORD	dwFlags
	)
{
	return MoveFileWithProgressA (lpExistingFileName,
	                              lpNewFileName,
	                              NULL,
	                              NULL,
	                              dwFlags);
}

/* EOF */
