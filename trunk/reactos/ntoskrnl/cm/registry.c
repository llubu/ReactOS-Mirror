/* $Id: registry.c,v 1.62 2001/06/16 14:06:00 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 * PROGRAMMERS:     Rex Jolliff
 *                  Matt Pyne
 *                  Jean Michault
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

#include "cm.h"

#define  REG_ROOT_KEY_NAME	L"\\Registry"
#define  REG_MACHINE_KEY_NAME	L"\\Registry\\Machine"
#define  REG_SYSTEM_KEY_NAME	L"\\Registry\\Machine\\System"
#define  REG_SOFTWARE_KEY_NAME	L"\\Registry\\Machine\\Software"
#define  REG_SAM_KEY_NAME	L"\\Registry\\Machine\\Sam"
#define  REG_SEC_KEY_NAME	L"\\Registry\\Machine\\Security"
#define  REG_USERS_KEY_NAME	L"\\Registry\\User"
#define  REG_USER_KEY_NAME	L"\\Registry\\User\\CurrentUser"

#define  SYSTEM_REG_FILE	L"\\SystemRoot\\System32\\Config\\SYSTEM"
#define  SOFTWARE_REG_FILE	L"\\SystemRoot\\System32\\Config\\SOFTWARE"
#define  USER_REG_FILE		L"\\SystemRoot\\System32\\Config\\DEFAULT"
#define  SAM_REG_FILE		L"\\SystemRoot\\System32\\Config\\SAM"
#define  SEC_REG_FILE		L"\\SystemRoot\\System32\\Config\\SECURITY"


/*  -------------------------------------------------  File Statics  */

POBJECT_TYPE  CmiKeyType = NULL;
PREGISTRY_FILE  CmiVolatileFile = NULL;
KSPIN_LOCK  CmiKeyListLock;

static PKEY_OBJECT  CmiRootKey = NULL;
static PKEY_OBJECT  CmiMachineKey = NULL;
static PKEY_OBJECT  CmiUserKey = NULL;

static GENERIC_MAPPING CmiKeyMapping =
	{KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS};


VOID
CmInitializeRegistry(VOID)
{
  NTSTATUS  Status;
  HANDLE  RootKeyHandle;
  UNICODE_STRING  RootKeyName;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  PKEY_OBJECT NewKey;
  HANDLE  KeyHandle;
  
  /*  Initialize the Key object type  */
  CmiKeyType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  CmiKeyType->TotalObjects = 0;
  CmiKeyType->TotalHandles = 0;
  CmiKeyType->MaxObjects = LONG_MAX;
  CmiKeyType->MaxHandles = LONG_MAX;
  CmiKeyType->PagedPoolCharge = 0;
  CmiKeyType->NonpagedPoolCharge = sizeof(KEY_OBJECT);
  CmiKeyType->Mapping = &CmiKeyMapping;
  CmiKeyType->Dump = NULL;
  CmiKeyType->Open = NULL;
  CmiKeyType->Close = NULL;
  CmiKeyType->Delete = CmiObjectDelete;
  CmiKeyType->Parse = CmiObjectParse;
  CmiKeyType->Security = NULL;
  CmiKeyType->QueryName = NULL;
  CmiKeyType->OkayToClose = NULL;
  CmiKeyType->Create = CmiObjectCreate;
  RtlInitUnicodeString(&CmiKeyType->TypeName, L"Key");

  /*  Build volitile registry store  */
  CmiVolatileFile = CmiCreateRegistry(NULL);

  /*  Build the Root Key Object  */
  RtlInitUnicodeString(&RootKeyName, REG_ROOT_KEY_NAME);
DPRINT("Creating root\n");
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  Status = ObCreateObject(&RootKeyHandle,
                          STANDARD_RIGHTS_REQUIRED,
                          &ObjectAttributes,
                          CmiKeyType,
                          (PVOID*)&NewKey);
  CmiRootKey = NewKey;
  ObAddEntryDirectory(NameSpaceRoot, CmiRootKey, L"Registry");
  Status = ObReferenceObjectByHandle(RootKeyHandle,
                 STANDARD_RIGHTS_REQUIRED,
		 ObDirectoryType,
		 UserMode,
                 (PVOID*)&CmiRootKey,
                 NULL);
    CmiRootKey->RegistryFile = CmiVolatileFile;
    CmiRootKey->KeyBlock = CmiGetBlock(CmiVolatileFile,CmiVolatileFile->HeaderBlock->RootKeyBlock,NULL);

    CmiRootKey->BlockOffset = CmiVolatileFile->HeaderBlock->RootKeyBlock;
    CmiRootKey->Flags = 0;
    CmiRootKey->NumberOfSubKeys=0;
    CmiRootKey->SubKeys= NULL;
    CmiRootKey->SizeOfSubKeys= 0;
    CmiRootKey->Name=ExAllocatePool(PagedPool,strlen("Registry"));
    CmiRootKey->NameSize=strlen("Registry");
    memcpy(CmiRootKey->Name,"Registry",strlen("Registry"));

  KeInitializeSpinLock(&CmiKeyListLock);

  /*  Create initial predefined symbolic links  */
  /* HKEY_LOCAL_MACHINE  */
  RtlInitUnicodeString(&RootKeyName, REG_MACHINE_KEY_NAME);
DPRINT("Creating HKLM\n");
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  Status=ObCreateObject(&KeyHandle,
                        STANDARD_RIGHTS_REQUIRED,
                        &ObjectAttributes,
                        CmiKeyType,
                        (PVOID*)&NewKey);
  Status = CmiAddSubKey(CmiVolatileFile,
                        CmiRootKey,
                        NewKey,
                        L"Machine",
                        14,
                        0,
                        NULL,
                        0);
    NewKey->RegistryFile = CmiVolatileFile;
    NewKey->Flags = 0;
    NewKey->NumberOfSubKeys=0;
    NewKey->SubKeys= NULL;
    NewKey->SizeOfSubKeys= NewKey->KeyBlock->NumberOfSubKeys;
    NewKey->Name=ExAllocatePool(PagedPool,strlen("Machine"));
    NewKey->NameSize=strlen("Machine");
    memcpy(NewKey->Name,"Machine",strlen("Machine"));
  CmiAddKeyToList(CmiRootKey,NewKey);
    CmiMachineKey=NewKey;

  /* HKEY_USERS  */
  RtlInitUnicodeString(&RootKeyName, REG_USERS_KEY_NAME);
DPRINT("Creating HKU\n");
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
   Status=ObCreateObject(&KeyHandle,
                         STANDARD_RIGHTS_REQUIRED,
                         &ObjectAttributes,
                         CmiKeyType,
                         (PVOID*)&NewKey);
  Status = CmiAddSubKey(CmiVolatileFile,
                        CmiRootKey,
                        NewKey,
                        L"User",
                        8,
                        0,
                        NULL,
                        0);
   NewKey->RegistryFile = CmiVolatileFile;
   NewKey->Flags = 0;
   NewKey->NumberOfSubKeys=0;
   NewKey->SubKeys= NULL;
   NewKey->SizeOfSubKeys= NewKey->KeyBlock->NumberOfSubKeys;
   NewKey->Name=ExAllocatePool(PagedPool,strlen("User"));
   NewKey->NameSize=strlen("User");
   memcpy(NewKey->Name,"User",strlen("User"));
   CmiAddKeyToList(CmiRootKey,NewKey);
   CmiUserKey=NewKey;


  /* FIXME: create remaining structure needed for default handles  */
  /* FIXME: load volatile registry data from ROSDTECT  */

}

NTSTATUS CmConnectHive(PWSTR FileName,PWSTR FullName,CHAR *KeyName,PKEY_OBJECT Parent)
{
 OBJECT_ATTRIBUTES  ObjectAttributes;
 PKEY_OBJECT  NewKey;
 HANDLE  KeyHandle;
 UNICODE_STRING  uKeyName;
 PREGISTRY_FILE  RegistryFile = NULL;
 NTSTATUS Status;
  RegistryFile = CmiCreateRegistry(FileName);
  if( RegistryFile )
  {
    RtlInitUnicodeString(&uKeyName, FullName);
DPRINT("CCH %S ;",FullName);
    InitializeObjectAttributes(&ObjectAttributes, &uKeyName, 0, NULL, NULL);
    Status=ObCreateObject(&KeyHandle,
                          STANDARD_RIGHTS_REQUIRED,
                          &ObjectAttributes,
                          CmiKeyType,
                          (PVOID*)&NewKey);
    if (!NT_SUCCESS(Status))
      return (Status);
    NewKey->RegistryFile = RegistryFile;
    NewKey->BlockOffset = RegistryFile->HeaderBlock->RootKeyBlock;
    NewKey->KeyBlock = CmiGetBlock(RegistryFile,NewKey->BlockOffset,NULL);
    NewKey->Flags = 0;
    NewKey->NumberOfSubKeys=0;
    NewKey->SubKeys= ExAllocatePool(PagedPool
		, NewKey->KeyBlock->NumberOfSubKeys * sizeof(DWORD));
    NewKey->SizeOfSubKeys= NewKey->KeyBlock->NumberOfSubKeys;
    NewKey->Name=ExAllocatePool(PagedPool,strlen(KeyName));
    NewKey->NameSize=strlen(KeyName);
    memcpy(NewKey->Name,KeyName,strlen(KeyName));
    CmiAddKeyToList(Parent,NewKey);
  }
  else
    return STATUS_UNSUCCESSFUL;
  return STATUS_SUCCESS;
}

VOID
CmInitializeRegistry2(VOID)
{
 NTSTATUS Status;
  /* FIXME : delete temporary \Registry\Machine\System */
  /* connect the SYSTEM Hive */
  Status=CmConnectHive(SYSTEM_REG_FILE,REG_SYSTEM_KEY_NAME
			,"System",CmiMachineKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search SYSTEM.alt, or create new */
    CPRINT(" warning : registry file %S not found\n",SYSTEM_REG_FILE);
  }
  /* connect the SOFTWARE Hive */
  Status=CmConnectHive(SOFTWARE_REG_FILE,REG_SOFTWARE_KEY_NAME
			,"Software",CmiMachineKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search SOFTWARE.alt, or create new */
    CPRINT(" warning : registry file %S not found\n",SOFTWARE_REG_FILE);
  }
  /* connect the SAM Hive */
  Status=CmConnectHive(SAM_REG_FILE,REG_SAM_KEY_NAME
			,"Sam",CmiMachineKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search SAM.alt, or create new */
    CPRINT(" warning : registry file %S not found\n",SAM_REG_FILE);
  }
  /* connect the SECURITY Hive */
  Status=CmConnectHive(SEC_REG_FILE,REG_SEC_KEY_NAME
			,"Security",CmiMachineKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search SECURITY.alt, or create new */
    CPRINT(" warning : registry file %S not found\n",SEC_REG_FILE);
  }
  /* connect the DEFAULT Hive */
  Status=CmConnectHive(USER_REG_FILE,REG_USER_KEY_NAME
			,".Default",CmiUserKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search DEFAULT.alt, or create new */
    CPRINT(" warning : registry file %S not found\n",USER_REG_FILE);
  }
  /* FIXME : initialize standards symbolic links */
/*
for(;;)
{
__asm__ ("hlt\n\t");
}
*/
}

VOID 
CmImportHive(PCHAR  Chunk)
{
  /*  FIXME: implemement this  */
  return; 
}

VOID
CmShutdownRegistry(VOID)
{
  DPRINT("CmShutdownRegistry()...\n");
}





/* EOF */
