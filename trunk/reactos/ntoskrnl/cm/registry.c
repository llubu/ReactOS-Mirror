/* $Id: registry.c,v 1.71 2002/05/05 14:57:43 chorns Exp $
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

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <limits.h>
#include <string.h>
#include <internal/pool.h>
#include <internal/registry.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

/*  -------------------------------------------------  File Statics  */

POBJECT_TYPE  CmiKeyType = NULL;
PREGISTRY_HIVE  CmiVolatileHive = NULL;
KSPIN_LOCK  CmiKeyListLock;

static PKEY_OBJECT  CmiRootKey = NULL;
static PKEY_OBJECT  CmiMachineKey = NULL;
static PKEY_OBJECT  CmiUserKey = NULL;
static PKEY_OBJECT  CmiHardwareKey = NULL;

static GENERIC_MAPPING CmiKeyMapping =
	{KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS};


VOID
CmiCheckKey(BOOLEAN Verbose,
  HANDLE Key);


VOID
CmiCheckSubKeys(BOOLEAN Verbose,
  HANDLE Key)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  PKEY_NODE_INFORMATION KeyInfo;
  WCHAR KeyBuffer[MAX_PATH];
  UNICODE_STRING KeyPath;
  WCHAR Name[MAX_PATH];
  ULONG BufferSize;
  ULONG ResultSize;
  NTSTATUS Status;
  HANDLE SubKey;
  ULONG Index;

  Index = 0;
  while (TRUE)
    {
  	  BufferSize = sizeof(KEY_NODE_INFORMATION) + 4096;
	    KeyInfo = ExAllocatePool(PagedPool, BufferSize);

	    Status = NtEnumerateKey(Key,
			  Index,
				KeyNodeInformation,
				KeyInfo,
				BufferSize,
				&ResultSize);
      if (!NT_SUCCESS(Status))
		    {
          ExFreePool(KeyInfo);
		      if (Status == STATUS_NO_MORE_ENTRIES)
			      Status = STATUS_SUCCESS;
		      break;
		    }

      wcsncpy(Name,
        KeyInfo->Name,
        KeyInfo->NameLength / sizeof(WCHAR));

      if (Verbose)
				{
          DbgPrint("Key: %S\n", Name);
				}

      /* FIXME: Check info. */

      ExFreePool(KeyInfo);

    	wcscpy(KeyBuffer, L"\\Registry\\");
    	wcscat(KeyBuffer, Name);

    	RtlInitUnicodeString(&KeyPath, KeyBuffer);

		  InitializeObjectAttributes(&ObjectAttributes,
				&KeyPath,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

		  Status = NtOpenKey(&SubKey,
				KEY_ALL_ACCESS,
				&ObjectAttributes);

		  assert(NT_SUCCESS(Status));
		
		  CmiCheckKey(Verbose, SubKey);
		
		  NtClose(SubKey);

		  Index++;
    }

  assert(NT_SUCCESS(Status));
}


VOID
CmiCheckValues(BOOLEAN Verbose,
  HANDLE Key)
{
  PKEY_NODE_INFORMATION ValueInfo;
  WCHAR Name[MAX_PATH];
  ULONG BufferSize;
  ULONG ResultSize;
  NTSTATUS Status;
  ULONG Index;

  Index = 0;
  while (TRUE)
    {
  	  BufferSize = sizeof(KEY_NODE_INFORMATION) + 4096;
	    ValueInfo = ExAllocatePool(PagedPool, BufferSize);

	    Status = NtEnumerateValueKey(Key,
			  Index,
				KeyNodeInformation,
				ValueInfo,
				BufferSize,
				&ResultSize);
      if (!NT_SUCCESS(Status))
		    {
          ExFreePool(ValueInfo);
		      if (Status == STATUS_NO_MORE_ENTRIES)
			      Status = STATUS_SUCCESS;
		      break;
		    }

      wcsncpy(Name,
        ValueInfo->Name,
        ValueInfo->NameLength / sizeof(WCHAR));

      if (Verbose)
				{
          DbgPrint("Value: %S\n", Name);
				}

      /* FIXME: Check info. */

      ExFreePool(ValueInfo);

		  Index++;
    }

  assert(NT_SUCCESS(Status));
}


VOID
CmiCheckKey(BOOLEAN Verbose,
  HANDLE Key)
{
  CmiCheckValues(Verbose, Key);
  CmiCheckSubKeys(Verbose, Key);
}


VOID
CmiCheckByName(BOOLEAN Verbose,
  PWSTR KeyName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR KeyPathBuffer[MAX_PATH];
  UNICODE_STRING KeyPath;
  NTSTATUS Status;
  HANDLE Key;

  wcscpy(KeyPathBuffer, L"\\Registry\\");
  wcscat(KeyPathBuffer, KeyName);

	RtlInitUnicodeString(&KeyPath, KeyPathBuffer);

  InitializeObjectAttributes(&ObjectAttributes,
		&KeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

  Status = NtOpenKey(&Key,
		KEY_ALL_ACCESS,
		&ObjectAttributes);

  if (CHECKED)
    {
      if (!NT_SUCCESS(Status))
				{
          DbgPrint("KeyPath %wZ  Status: %.08x", KeyPath, Status);
          DbgPrint("KeyPath %S  Status: %.08x", KeyPath.Buffer, Status);
          assert(NT_SUCCESS(Status));
				}
    }

  CmiCheckKey(Verbose, Key);

  NtClose(Key);
}


VOID
CmiCheckRegistry(BOOLEAN Verbose)
{
  if (Verbose)
    DbgPrint("Checking registry internals\n");

  CmiCheckByName(Verbose, L"Machine");
  CmiCheckByName(Verbose, L"User");
}


VOID
CmInitializeRegistry(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING RootKeyName;
  HANDLE RootKeyHandle;
  PKEY_OBJECT NewKey;
  HANDLE KeyHandle;
  NTSTATUS Status;
  
  /*  Initialize the Key object type  */
  CmiKeyType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  assert(CmiKeyType);
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
  CmiKeyType->DuplicationNotify = NULL;
  RtlInitUnicodeString(&CmiKeyType->TypeName, L"Key");

  /*  Build volatile registry store  */
  Status = CmiCreateRegistryHive(NULL, &CmiVolatileHive, FALSE);
  assert(NT_SUCCESS(Status));

  /* Build the Root Key Object */
  RtlInitUnicodeString(&RootKeyName, REG_ROOT_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  Status = ObCreateObject(&RootKeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		&ObjectAttributes,
		CmiKeyType,
		(PVOID *) &NewKey);
  assert(NT_SUCCESS(Status));
  CmiRootKey = NewKey;
  Status = ObReferenceObjectByHandle(RootKeyHandle,
    STANDARD_RIGHTS_REQUIRED,
		CmiKeyType,
		KernelMode,
		(PVOID *) &CmiRootKey,
		NULL);
  assert(NT_SUCCESS(Status));
  CmiRootKey->RegistryHive = CmiVolatileHive;
  NewKey->BlockOffset = CmiVolatileHive->HiveHeader->RootKeyCell;
  NewKey->KeyCell = CmiGetBlock(CmiVolatileHive, NewKey->BlockOffset, NULL);
  CmiRootKey->Flags = 0;
  CmiRootKey->NumberOfSubKeys = 0;
  CmiRootKey->SubKeys = NULL;
  CmiRootKey->SizeOfSubKeys = 0;
  CmiRootKey->Name = ExAllocatePool(PagedPool, strlen("Registry"));
  CmiRootKey->NameSize = strlen("Registry");
  memcpy(CmiRootKey->Name, "Registry", strlen("Registry"));

  KeInitializeSpinLock(&CmiKeyListLock);

  /* Create initial predefined symbolic links */

  /* HKEY_LOCAL_MACHINE */
  Status = ObCreateObject(&KeyHandle,
    STANDARD_RIGHTS_REQUIRED,
    NULL,
    CmiKeyType,
    (PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
		CmiRootKey,
		NewKey,
		L"Machine",
		wcslen(L"Machine") * sizeof(WCHAR),
		0,
		NULL,
		0);
  assert(NT_SUCCESS(Status));
	NewKey->RegistryHive = CmiVolatileHive;
	NewKey->Flags = 0;
	NewKey->NumberOfSubKeys = 0;
	NewKey->SubKeys = NULL;
	NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
	NewKey->Name = ExAllocatePool(PagedPool, strlen("Machine"));
	NewKey->NameSize = strlen("Machine");
	memcpy(NewKey->Name, "Machine", strlen("Machine"));
  CmiAddKeyToList(CmiRootKey, NewKey);
  CmiMachineKey = NewKey;

  /* HKEY_USERS */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
    CmiRootKey,
    NewKey,
    L"User",
    wcslen(L"User") * sizeof(WCHAR),
    0,
    NULL,
    0);
  assert(NT_SUCCESS(Status));
	NewKey->RegistryHive = CmiVolatileHive;
	NewKey->Flags = 0;
	NewKey->NumberOfSubKeys = 0;
	NewKey->SubKeys = NULL;
	NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
	NewKey->Name = ExAllocatePool(PagedPool, strlen("User"));
	NewKey->NameSize = strlen("User");
	memcpy(NewKey->Name, "User", strlen("User"));
	CmiAddKeyToList(CmiRootKey, NewKey);
	CmiUserKey = NewKey;

  /* Create '\\Registry\\Machine\\HARDWARE' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*)&NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
    CmiMachineKey,
    NewKey,
    L"HARDWARE",
    wcslen(L"HARDWARE") * sizeof(WCHAR),
    0,
    NULL,
    0);
  assert(NT_SUCCESS(Status));
  NewKey->RegistryHive = CmiVolatileHive;
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen("HARDWARE"));
  NewKey->NameSize = strlen("HARDWARE");
  memcpy(NewKey->Name, "HARDWARE", strlen("HARDWARE"));
  CmiAddKeyToList(CmiMachineKey, NewKey);
  CmiHardwareKey = NewKey;

  /* Create '\\Registry\\Machine\\HARDWARE\\DESCRIPTION' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
		CmiHardwareKey,
		NewKey,
		L"DESCRIPTION",
		wcslen(L"DESCRIPTION") * sizeof(WCHAR),
		0,
		NULL,
		0);
  assert(NT_SUCCESS(Status));
  NewKey->RegistryHive = CmiVolatileHive;
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen("DESCRIPTION"));
  NewKey->NameSize = strlen("DESCRIPTION");
  memcpy(NewKey->Name, "DESCRIPTION", strlen("DESCRIPTION"));
  CmiAddKeyToList(CmiHardwareKey, NewKey);

  /* Create '\\Registry\\Machine\\HARDWARE\\DEVICEMAP' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
    CmiHardwareKey,
    NewKey,
    L"DEVICEMAP",
		wcslen(L"DEVICEMAP") * sizeof(WCHAR),
    0,
    NULL,
    0);
  assert(NT_SUCCESS(Status));
  NewKey->RegistryHive = CmiVolatileHive;
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen("DEVICEMAP"));
  NewKey->NameSize = strlen("DEVICEMAP");
  memcpy(NewKey->Name, "DEVICEMAP", strlen("DEVICEMAP"));
  CmiAddKeyToList(CmiHardwareKey,NewKey);

  /* Create '\\Registry\\Machine\\HARDWARE\\RESOURCEMAP' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
		CmiHardwareKey,
		NewKey,
		L"RESOURCEMAP",
		wcslen(L"RESOURCEMAP") * sizeof(WCHAR),
		0,
		NULL,
		0);
  assert(NT_SUCCESS(Status));
  NewKey->RegistryHive = CmiVolatileHive;
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen("RESOURCEMAP"));
  NewKey->NameSize = strlen("RESOURCEMAP");
  memcpy(NewKey->Name, "RESOURCEMAP", strlen("RESOURCEMAP"));
  CmiAddKeyToList(CmiHardwareKey, NewKey);

  /* FIXME: create remaining structure needed for default handles  */
  /* FIXME: load volatile registry data from ROSDTECT  */
}


NTSTATUS
CmiConnectHive(PWSTR FileName,
  PWSTR FullName,
  PCHAR KeyName,
  PKEY_OBJECT Parent,
  BOOLEAN CreateNew)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  PREGISTRY_HIVE RegistryHive = NULL;
  UNICODE_STRING uKeyName;
  PKEY_OBJECT NewKey;
  HANDLE KeyHandle;
  NTSTATUS Status;

  DPRINT("Called. FileName %S\n", FullName);

  Status = CmiCreateRegistryHive(FileName, &RegistryHive, CreateNew);
  if (!NT_SUCCESS(Status))
    return(Status);

  RtlInitUnicodeString(&uKeyName, FullName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &uKeyName,
			     0,
			     NULL,
			     NULL);

  Status = ObCreateObject(&KeyHandle,
			  STANDARD_RIGHTS_REQUIRED,
			  &ObjectAttributes,
			  CmiKeyType,
			  (PVOID*)&NewKey);
  if (!NT_SUCCESS(Status))
    return(Status);

  NewKey->RegistryHive = RegistryHive;
  NewKey->BlockOffset = RegistryHive->HiveHeader->RootKeyCell;
  NewKey->KeyCell = CmiGetBlock(RegistryHive, NewKey->BlockOffset, NULL);
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = ExAllocatePool(PagedPool,
  NewKey->KeyCell->NumberOfSubKeys * sizeof(DWORD));

  if ((NewKey->SubKeys == NULL) && (NewKey->KeyCell->NumberOfSubKeys != 0))
    {
      /* FIXME: Cleanup from CmiCreateRegistryHive() */
      DPRINT("NumberOfSubKeys %d\n", NewKey->KeyCell->NumberOfSubKeys);
      ZwClose(NewKey);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen(KeyName));

  if ((NewKey->Name == NULL) && (strlen(KeyName) != 0))
    {
      /* FIXME: Cleanup from CmiCreateRegistryHive() */
      DPRINT("strlen(KeyName) %d\n", strlen(KeyName));
      if (NewKey->SubKeys != NULL)
	ExFreePool(NewKey->SubKeys);
      ZwClose(NewKey);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  NewKey->NameSize = strlen(KeyName);
  memcpy(NewKey->Name, KeyName, strlen(KeyName));
  CmiAddKeyToList(Parent, NewKey);

  VERIFY_KEY_OBJECT(NewKey);

  return(STATUS_SUCCESS);
}


NTSTATUS
CmiInitializeHive(PWSTR FileName,
  PWSTR FullName,
  PCHAR KeyName,
  PKEY_OBJECT Parent,
  BOOLEAN CreateNew)
{
  NTSTATUS Status;

  DPRINT("CmiInitializeHive(%s) called\n", KeyName);

  /* Try to connect the hive */
  //Status = CmiConnectHive(FileName, FullName, KeyName, Parent, FALSE);
  Status = CmiConnectHive(FileName, FullName, KeyName, Parent, CreateNew);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("Status %.08x\n", Status);
#if 0
      WCHAR AltFileName[MAX_PATH];

      CPRINT("WARNING! Registry file %S not found\n", FileName);

      wcscpy(AltFileName, FileName);
      wcscat(AltFileName, L".alt");

      /* Try to connect the alternative hive */
      Status = CmiConnectHive(AltFileName, FullName, KeyName, Parent, TRUE);

      if (!NT_SUCCESS(Status))
	{
	  CPRINT("WARNING! Alternative registry file %S not found\n", AltFileName);
	  DPRINT("Status %.08x\n", Status);
	}
#endif
    }

  DPRINT("CmiInitializeHive() done\n");

  return(Status);
}


NTSTATUS
CmiInitHives(BOOLEAN SetUpBoot)
{
  NTSTATUS Status;

  DPRINT("CmiInitHives() called\n");

  CmiDoVerify = TRUE;

  /* FIXME: Delete temporary \Registry\Machine\System */

  /* Connect the SYSTEM hive */
  /* FIXME: Don't overwrite the existing 'System' hive yet */
//  Status = CmiInitializeHive(SYSTEM_REG_FILE, REG_SYSTEM_KEY_NAME, "System", CmiMachineKey);
//  assert(NT_SUCCESS(Status));

  /* Connect the SOFTWARE hive */
  Status = CmiInitializeHive(SOFTWARE_REG_FILE, REG_SOFTWARE_KEY_NAME, "Software", CmiMachineKey, SetUpBoot);
  //assert(NT_SUCCESS(Status));

  /* Connect the SAM hive */
  Status = CmiInitializeHive(SAM_REG_FILE,REG_SAM_KEY_NAME, "Sam", CmiMachineKey, SetUpBoot);
  //assert(NT_SUCCESS(Status));

  /* Connect the SECURITY hive */
  Status = CmiInitializeHive(SEC_REG_FILE, REG_SEC_KEY_NAME, "Security", CmiMachineKey, SetUpBoot);
  //assert(NT_SUCCESS(Status));

  /* Connect the DEFAULT hive */
  Status = CmiInitializeHive(USER_REG_FILE, REG_USER_KEY_NAME, ".Default", CmiUserKey, SetUpBoot);
  //assert(NT_SUCCESS(Status));

  /* FIXME : initialize standards symbolic links */

//  CmiCheckRegistry(TRUE);

  DPRINT("CmiInitHives() done\n");

  return(STATUS_SUCCESS);
}


VOID
CmShutdownRegistry(VOID)
{
  DPRINT("CmShutdownRegistry() called\n");

  /* Note:
   *	Don't call UNIMPLEMENTED() here since this function is
   *	called by NtShutdownSystem().
   */
}

/* EOF */
