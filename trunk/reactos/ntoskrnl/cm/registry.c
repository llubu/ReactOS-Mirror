/* $Id$
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

/* GLOBALS ******************************************************************/

POBJECT_TYPE  CmiKeyType = NULL;
PREGISTRY_HIVE  CmiVolatileHive = NULL;
KSPIN_LOCK  CmiKeyListLock;

LIST_ENTRY CmiHiveListHead;

ERESOURCE CmiRegistryLock;

volatile BOOLEAN CmiHiveSyncEnabled = FALSE;
volatile BOOLEAN CmiHiveSyncPending = FALSE;
KDPC CmiHiveSyncDpc;
KTIMER CmiHiveSyncTimer;

static GENERIC_MAPPING CmiKeyMapping =
	{KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS};



VOID
CmiCheckKey(BOOLEAN Verbose,
  HANDLE Key);

static NTSTATUS
CmiCreateCurrentControlSetLink(VOID);

static VOID STDCALL
CmiHiveSyncDpcRoutine(PKDPC Dpc,
		      PVOID DeferredContext,
		      PVOID SystemArgument1,
		      PVOID SystemArgument2);

/* FUNCTIONS ****************************************************************/

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

      ASSERT(NT_SUCCESS(Status));

      CmiCheckKey(Verbose, SubKey);

      NtClose(SubKey);

      Index++;
    }

  ASSERT(NT_SUCCESS(Status));
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

  ASSERT(NT_SUCCESS(Status));
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
          ASSERT(NT_SUCCESS(Status));
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


VOID INIT_FUNCTION
CmInitializeRegistry(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  PKEY_OBJECT RootKey;
#if 0
  PSECURITY_CELL RootSecurityCell;
#endif
  HANDLE RootKeyHandle;
  HANDLE KeyHandle;
  NTSTATUS Status;

  /*  Initialize the Key object type  */
  CmiKeyType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  ASSERT(CmiKeyType);
  CmiKeyType->Tag = TAG('R', 'e', 'g', 'K');
  CmiKeyType->TotalObjects = 0;
  CmiKeyType->TotalHandles = 0;
  CmiKeyType->PeakObjects = 0;
  CmiKeyType->PeakHandles = 0;
  CmiKeyType->PagedPoolCharge = 0;
  CmiKeyType->NonpagedPoolCharge = sizeof(KEY_OBJECT);
  CmiKeyType->Mapping = &CmiKeyMapping;
  CmiKeyType->Dump = NULL;
  CmiKeyType->Open = NULL;
  CmiKeyType->Close = NULL;
  CmiKeyType->Delete = CmiObjectDelete;
  CmiKeyType->Parse = CmiObjectParse;
  CmiKeyType->Security = CmiObjectSecurity;
  CmiKeyType->QueryName = CmiObjectQueryName;
  CmiKeyType->OkayToClose = NULL;
  CmiKeyType->Create = CmiObjectCreate;
  CmiKeyType->DuplicationNotify = NULL;
  RtlInitUnicodeString(&CmiKeyType->TypeName, L"Key");

  ObpCreateTypeObject (CmiKeyType);

  /* Initialize the hive list */
  InitializeListHead(&CmiHiveListHead);

  /* Initialize registry lock */
  ExInitializeResourceLite(&CmiRegistryLock);

  /*  Build volatile registry store  */
  Status = CmiCreateVolatileHive (&CmiVolatileHive);
  ASSERT(NT_SUCCESS(Status));

  /* Create '\Registry' key. */
  RtlInitUnicodeString(&KeyName, REG_ROOT_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, 0, NULL, NULL);
  Status = ObCreateObject(KernelMode,
			  CmiKeyType,
			  &ObjectAttributes,
			  KernelMode,
			  NULL,
			  sizeof(KEY_OBJECT),
			  0,
			  0,
			  (PVOID *) &RootKey);
  ASSERT(NT_SUCCESS(Status));
  Status = ObInsertObject(RootKey,
			  NULL,
			  STANDARD_RIGHTS_REQUIRED,
			  0,
			  NULL,
			  &RootKeyHandle);
  ASSERT(NT_SUCCESS(Status));
  RootKey->RegistryHive = CmiVolatileHive;
  RootKey->KeyCellOffset = CmiVolatileHive->HiveHeader->RootKeyOffset;
  RootKey->KeyCell = CmiGetCell (CmiVolatileHive, RootKey->KeyCellOffset, NULL);
  RootKey->ParentKey = RootKey;
  RootKey->Flags = 0;
  RootKey->NumberOfSubKeys = 0;
  RootKey->SubKeys = NULL;
  RootKey->SizeOfSubKeys = 0;
  Status = RtlCreateUnicodeString(&RootKey->Name, L"Registry");
  ASSERT(NT_SUCCESS(Status));

#if 0
  Status = CmiAllocateCell(CmiVolatileHive,
			   0x10, //LONG CellSize,
			   (PVOID *)&RootSecurityCell,
			   &RootKey->KeyCell->SecurityKeyOffset);
  ASSERT(NT_SUCCESS(Status));

  /* Copy the security descriptor */

  CmiVolatileHive->RootSecurityCell = RootSecurityCell;
#endif

  KeInitializeSpinLock(&CmiKeyListLock);

  /* Create '\Registry\Machine' key. */
  RtlInitUnicodeString(&KeyName,
		       L"Machine");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     0,
			     RootKeyHandle,
			     NULL);
  Status = NtCreateKey(&KeyHandle,
		       STANDARD_RIGHTS_REQUIRED,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       NULL);
  ASSERT(NT_SUCCESS(Status));

  /* Create '\Registry\User' key. */
  RtlInitUnicodeString(&KeyName,
		       L"User");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     0,
			     RootKeyHandle,
			     NULL);
  Status = NtCreateKey(&KeyHandle,
		       STANDARD_RIGHTS_REQUIRED,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       NULL);
  ASSERT(NT_SUCCESS(Status));
}


VOID INIT_FUNCTION
CmInit2(PCHAR CommandLine)
{
  ULONG PiceStart = 4;
  BOOLEAN MiniNT = FALSE;
  PWCHAR SystemBootDevice;
  PWCHAR SystemStartOptions;
  ULONG Position;
  NTSTATUS Status;

  /* Create the 'CurrentControlSet' link. */
  Status = CmiCreateCurrentControlSetLink();
  if (!NT_SUCCESS(Status))
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);

  /*
   * Parse the system boot device.
   */
  Position = 0;
  SystemBootDevice = ExAllocatePool(PagedPool,
				    (strlen(CommandLine) + 1) * sizeof(WCHAR));
  if (SystemBootDevice == NULL)
  {
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
  }

  while (*CommandLine != 0 && *CommandLine != ' ')
    SystemBootDevice[Position++] = *(CommandLine++);
  SystemBootDevice[Position++] = 0;

  /*
   * Write the system boot device to registry.
   */
  Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
				 L"\\Registry\\Machine\\System\\CurrentControlSet\\Control",
				 L"SystemBootDevice",
				 REG_SZ,
				 SystemBootDevice,
				 Position * sizeof(WCHAR));
  if (!NT_SUCCESS(Status))
  {
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
  }

  /*
   * Parse the system start options.
   */
  Position = 0;
  SystemStartOptions = SystemBootDevice;
  while ((CommandLine = strchr(CommandLine, '/')) != NULL)
    {
      /* Skip over the slash */
      CommandLine++;

      /* Special options */
      if (!_strnicmp(CommandLine, "MININT", 6))
        MiniNT = TRUE;
      else if (!_strnicmp(CommandLine, "DEBUGPORT=PICE", 14))
        PiceStart = 1;
      
      /* Add a space between the options */
      if (Position != 0)
        SystemStartOptions[Position++] = L' ';

      /* Copy the command */
      while (*CommandLine != 0 && *CommandLine != ' ')
        SystemStartOptions[Position++] = *(CommandLine++);
    }
  SystemStartOptions[Position++] = 0;

  /*
   * Write the system start options to registry.
   */
  Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
				 L"\\Registry\\Machine\\System\\CurrentControlSet\\Control",
				 L"SystemStartOptions",
				 REG_SZ,
				 SystemStartOptions,
				 Position * sizeof(WCHAR));
  if (!NT_SUCCESS(Status))
  {
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
  }

  /*
   * Create a CurrentControlSet\Control\MiniNT key that is used
   * to detect WinPE/MiniNT systems.
   */
  if (MiniNT)
    {
      Status = RtlCreateRegistryKey(RTL_REGISTRY_CONTROL, L"MiniNT");
      if (!NT_SUCCESS(Status))
        KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
    }

  /* Set PICE 'Start' value to 1, if PICE debugging is enabled */
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_SERVICES,
    L"\\Pice",
    L"Start",
    REG_DWORD,
    &PiceStart,
    sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);

  ExFreePool(SystemBootDevice);
}


static NTSTATUS
CmiCreateCurrentControlSetLink(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[5];
  WCHAR TargetNameBuffer[80];
  ULONG TargetNameLength;
  UNICODE_STRING LinkName;
  UNICODE_STRING LinkValue;
  ULONG CurrentSet;
  ULONG DefaultSet;
  ULONG Failed;
  ULONG LastKnownGood;
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE KeyHandle;

  DPRINT("CmiCreateCurrentControlSetLink() called\n");

  RtlZeroMemory(&QueryTable, sizeof(QueryTable));

  QueryTable[0].Name = L"Current";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = &CurrentSet;

  QueryTable[1].Name = L"Default";
  QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[1].EntryContext = &DefaultSet;

  QueryTable[2].Name = L"Failed";
  QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[2].EntryContext = &Failed;

  QueryTable[3].Name = L"LastKnownGood";
  QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[3].EntryContext = &LastKnownGood;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
				  L"\\Registry\\Machine\\SYSTEM\\Select",
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  DPRINT("Current %ld  Default %ld\n", CurrentSet, DefaultSet);

  swprintf(TargetNameBuffer,
	   L"\\Registry\\Machine\\SYSTEM\\ControlSet%03lu",
	   CurrentSet);
  TargetNameLength = wcslen(TargetNameBuffer) * sizeof(WCHAR);

  DPRINT("Link target '%S'\n", TargetNameBuffer);

  RtlRosInitUnicodeStringFromLiteral(&LinkName,
				  L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet");
  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_OPENLINK,
			     NULL,
			     NULL);
  Status = NtCreateKey(&KeyHandle,
		       KEY_ALL_ACCESS | KEY_CREATE_LINK,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
      return(Status);
    }

  RtlRosInitUnicodeStringFromLiteral(&LinkValue,
				  L"SymbolicLinkValue");
  Status = NtSetValueKey(KeyHandle,
			 &LinkValue,
			 0,
			 REG_LINK,
			 (PVOID)TargetNameBuffer,
			 TargetNameLength);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
    }

  NtClose(KeyHandle);

  return Status;
}


NTSTATUS
CmiConnectHive(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
	       IN PREGISTRY_HIVE RegistryHive)
{
  UNICODE_STRING RemainingPath;
  PKEY_OBJECT ParentKey;
  PKEY_OBJECT NewKey;
  NTSTATUS Status;
  PWSTR SubName;

  DPRINT("CmiConnectHive(%p, %p) called.\n",
	 KeyObjectAttributes, RegistryHive);

  Status = ObFindObject(KeyObjectAttributes,
			(PVOID*)&ParentKey,
			&RemainingPath,
			CmiKeyType);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  DPRINT ("RemainingPath %wZ\n", &RemainingPath);

  if ((RemainingPath.Buffer == NULL) || (RemainingPath.Buffer[0] == 0))
    {
      ObDereferenceObject (ParentKey);
      RtlFreeUnicodeString(&RemainingPath);
      return STATUS_OBJECT_NAME_COLLISION;
    }

  /* Ignore leading backslash */
  SubName = RemainingPath.Buffer;
  if (*SubName == L'\\')
    SubName++;

  /* If RemainingPath contains \ we must return error
     because CmiConnectHive() can not create trees */
  if (wcschr (SubName, L'\\') != NULL)
    {
      ObDereferenceObject (ParentKey);
      RtlFreeUnicodeString(&RemainingPath);
      return STATUS_OBJECT_NAME_NOT_FOUND;
    }

  DPRINT("RemainingPath %wZ  ParentKey %p\n",
	 &RemainingPath, ParentKey);

  Status = ObCreateObject(KernelMode,
			  CmiKeyType,
			  NULL,
			  KernelMode,
			  NULL,
			  sizeof(KEY_OBJECT),
			  0,
			  0,
			  (PVOID*)&NewKey);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ObCreateObject() failed (Status %lx)\n", Status);
      ObDereferenceObject (ParentKey);
      RtlFreeUnicodeString(&RemainingPath);
      return Status;
    }

  NewKey->RegistryHive = RegistryHive;
  NewKey->KeyCellOffset = RegistryHive->HiveHeader->RootKeyOffset;
  NewKey->KeyCell = CmiGetCell (RegistryHive, NewKey->KeyCellOffset, NULL);
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  if (NewKey->KeyCell->NumberOfSubKeys != 0)
    {
      NewKey->SubKeys = ExAllocatePool(NonPagedPool,
				       NewKey->KeyCell->NumberOfSubKeys * sizeof(ULONG));
      if (NewKey->SubKeys == NULL)
	{
	  DPRINT("ExAllocatePool() failed\n");
	  ObDereferenceObject (NewKey);
	  ObDereferenceObject (ParentKey);
	  RtlFreeUnicodeString(&RemainingPath);
	  return STATUS_INSUFFICIENT_RESOURCES;
	}
    }
  else
    {
      NewKey->SubKeys = NULL;
    }

  DPRINT ("SubName %S\n", SubName);

  Status = RtlCreateUnicodeString(&NewKey->Name,
				  SubName);
  RtlFreeUnicodeString(&RemainingPath);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlCreateUnicodeString() failed (Status %lx)\n", Status);
      if (NewKey->SubKeys != NULL)
	{
	  ExFreePool (NewKey->SubKeys);
	}
      ObDereferenceObject (NewKey);
      ObDereferenceObject (ParentKey);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  CmiAddKeyToList (ParentKey, NewKey);
  ObDereferenceObject (ParentKey);

  VERIFY_KEY_OBJECT(NewKey);

  /* Note: Do not dereference NewKey here! */

  return STATUS_SUCCESS;
}


NTSTATUS
CmiDisconnectHive (IN POBJECT_ATTRIBUTES KeyObjectAttributes,
		   OUT PREGISTRY_HIVE *RegistryHive)
{
  PKEY_OBJECT KeyObject;
  PREGISTRY_HIVE Hive;
  HANDLE KeyHandle;
  NTSTATUS Status;

  DPRINT("CmiDisconnectHive() called\n");

  *RegistryHive = NULL;

  Status = ObOpenObjectByName (KeyObjectAttributes,
			       CmiKeyType,
			       NULL,
			       KernelMode,
			       STANDARD_RIGHTS_REQUIRED,
			       NULL,
			       &KeyHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ObOpenObjectByName() failed (Status %lx)\n", Status);
      return Status;
    }

  Status = ObReferenceObjectByHandle (KeyHandle,
				      STANDARD_RIGHTS_REQUIRED,
				      CmiKeyType,
				      KernelMode,
				      (PVOID*)&KeyObject,
				      NULL);
  NtClose (KeyHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ObReferenceObjectByName() failed (Status %lx)\n", Status);
      return Status;
    }
  DPRINT ("KeyObject %p  Hive %p\n", KeyObject, KeyObject->RegistryHive);

  if (!(KeyObject->KeyCell->Flags & REG_KEY_ROOT_CELL))
    {
      DPRINT1 ("Key is not the Hive-Root-Key\n");
      ObDereferenceObject (KeyObject);
      return STATUS_INVALID_PARAMETER;
    }

  if (ObGetObjectHandleCount (KeyObject) != 0 ||
      ObGetObjectPointerCount (KeyObject) != 2)
    {
      DPRINT1 ("Hive is still in use\n");
      ObDereferenceObject (KeyObject);
      return STATUS_UNSUCCESSFUL;
    }

  Hive = KeyObject->RegistryHive;

  /* Dereference KeyObject twice to delete it */
  ObDereferenceObject (KeyObject);
  ObDereferenceObject (KeyObject);

  *RegistryHive = Hive;

  DPRINT ("CmiDisconnectHive() done\n");

  return STATUS_SUCCESS;
}


static NTSTATUS
CmiInitControlSetLink (VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ControlSetKeyName;
  UNICODE_STRING ControlSetLinkName;
  UNICODE_STRING ControlSetValueName;
  HANDLE KeyHandle;
  NTSTATUS Status;

  /* Create 'ControlSet001' key */
  RtlRosInitUnicodeStringFromLiteral (&ControlSetKeyName,
				   L"\\Registry\\Machine\\SYSTEM\\ControlSet001");
  InitializeObjectAttributes (&ObjectAttributes,
			      &ControlSetKeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = NtCreateKey (&KeyHandle,
			KEY_ALL_ACCESS,
			&ObjectAttributes,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtCreateKey() failed (Status %lx)\n", Status);
      return Status;
    }
  NtClose (KeyHandle);

  /* Link 'CurrentControlSet' to 'ControlSet001' key */
  RtlRosInitUnicodeStringFromLiteral (&ControlSetLinkName,
				   L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet");
  InitializeObjectAttributes (&ObjectAttributes,
			      &ControlSetLinkName,
			      OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_OPENLINK,
			      NULL,
			      NULL);
  Status = NtCreateKey (&KeyHandle,
			KEY_ALL_ACCESS | KEY_CREATE_LINK,
			&ObjectAttributes,
			0,
			NULL,
			REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
			NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtCreateKey() failed (Status %lx)\n", Status);
      return Status;
    }

  RtlRosInitUnicodeStringFromLiteral (&ControlSetValueName,
				   L"SymbolicLinkValue");
  Status = NtSetValueKey (KeyHandle,
			  &ControlSetValueName,
			  0,
			  REG_LINK,
			  (PVOID)ControlSetKeyName.Buffer,
			  ControlSetKeyName.Length);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtSetValueKey() failed (Status %lx)\n", Status);
    }
  NtClose (KeyHandle);

  return STATUS_SUCCESS;
}


NTSTATUS
CmiInitHives(BOOLEAN SetupBoot)
{
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;

  NTSTATUS Status;

  WCHAR ConfigPath[MAX_PATH];

  ULONG BufferSize;
  ULONG ResultSize;
  PWSTR EndPtr;


  DPRINT("CmiInitHives() called\n");

  if (SetupBoot == TRUE)
    {
      RtlRosInitUnicodeStringFromLiteral(&KeyName,
				      L"\\Registry\\Machine\\HARDWARE");
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);
      Status =  NtOpenKey(&KeyHandle,
			  KEY_ALL_ACCESS,
			  &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
	  return(Status);
	}

      RtlRosInitUnicodeStringFromLiteral(&ValueName,
				      L"InstallPath");

      BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 4096;
      ValueInfo = ExAllocatePool(PagedPool,
				 BufferSize);
      if (ValueInfo == NULL)
	{
	  NtClose(KeyHandle);
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      Status = NtQueryValueKey(KeyHandle,
			       &ValueName,
			       KeyValuePartialInformation,
			       ValueInfo,
			       BufferSize,
			       &ResultSize);
      NtClose(KeyHandle);
      if (!NT_SUCCESS(Status))
	{
	  ExFreePool(ValueInfo);
	  return(Status);
	}

      RtlCopyMemory(ConfigPath,
		    ValueInfo->Data,
		    ValueInfo->DataLength);
      ConfigPath[ValueInfo->DataLength / sizeof(WCHAR)] = (WCHAR)0;
      ExFreePool(ValueInfo);
    }
  else
    {
      wcscpy(ConfigPath, L"\\SystemRoot");
    }
  wcscat(ConfigPath, L"\\system32\\config");

  DPRINT("ConfigPath: %S\n", ConfigPath);

  EndPtr = ConfigPath + wcslen(ConfigPath);

  CmiDoVerify = TRUE;

  /* FIXME: Save boot log */

  /* Connect the SYSTEM hive only if it has been created */
  if (SetupBoot == TRUE)
    {
      wcscpy(EndPtr, REG_SYSTEM_FILE_NAME);
      DPRINT ("ConfigPath: %S\n", ConfigPath);

      RtlInitUnicodeString (&KeyName,
			    REG_SYSTEM_KEY_NAME);
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);

      RtlInitUnicodeString (&FileName,
			    ConfigPath);
      Status = CmiLoadHive (&ObjectAttributes,
			    &FileName,
			    0);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1 ("CmiLoadHive() failed (Status %lx)\n", Status);
	  return Status;
	}

      Status = CmiInitControlSetLink ();
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CmiInitControlSetLink() failed (Status %lx)\n", Status);
	  return(Status);
	}
    }

  /* Connect the SOFTWARE hive */
  wcscpy(EndPtr, REG_SOFTWARE_FILE_NAME);
  RtlInitUnicodeString (&FileName,
			ConfigPath);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SOFTWARE_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = CmiLoadHive (&ObjectAttributes,
			&FileName,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SAM hive */
  wcscpy(EndPtr, REG_SAM_FILE_NAME);
  RtlInitUnicodeString (&FileName,
			ConfigPath);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SAM_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = CmiLoadHive (&ObjectAttributes,
			&FileName,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SECURITY hive */
  wcscpy(EndPtr, REG_SEC_FILE_NAME);
  RtlInitUnicodeString (&FileName,
			ConfigPath);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SEC_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = CmiLoadHive (&ObjectAttributes,
			&FileName,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the DEFAULT hive */
  wcscpy(EndPtr, REG_DEFAULT_USER_FILE_NAME);
  RtlInitUnicodeString (&FileName,
			ConfigPath);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_DEFAULT_USER_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = CmiLoadHive (&ObjectAttributes,
			&FileName,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

//  CmiCheckRegistry(TRUE);

  /* Start automatic hive synchronization */
  KeInitializeDpc(&CmiHiveSyncDpc,
		  CmiHiveSyncDpcRoutine,
		  NULL);
  KeInitializeTimer(&CmiHiveSyncTimer);
  CmiHiveSyncEnabled = TRUE;

  DPRINT("CmiInitHives() done\n");

  return(STATUS_SUCCESS);
}


VOID
CmShutdownRegistry(VOID)
{
  PREGISTRY_HIVE Hive;
  PLIST_ENTRY Entry;

  DPRINT("CmShutdownRegistry() called\n");

  /* Stop automatic hive synchronization */
  CmiHiveSyncEnabled = FALSE;

  /* Cancel pending hive synchronization */
  if (CmiHiveSyncPending == TRUE)
    {
      KeCancelTimer(&CmiHiveSyncTimer);
      CmiHiveSyncPending = FALSE;
    }

  /* Acquire hive list lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  Entry = CmiHiveListHead.Flink;
  while (Entry != &CmiHiveListHead)
    {
      Hive = CONTAINING_RECORD(Entry, REGISTRY_HIVE, HiveList);

      if (!(IsNoFileHive(Hive) || IsNoSynchHive(Hive)))
	{
	  /* Flush non-volatile hive */
	  CmiFlushRegistryHive(Hive);
	}

      Entry = Entry->Flink;
    }

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  DPRINT("CmShutdownRegistry() done\n");
}


VOID STDCALL
CmiHiveSyncRoutine(PVOID DeferredContext)
{
  PREGISTRY_HIVE Hive;
  PLIST_ENTRY Entry;

  DPRINT("CmiHiveSyncRoutine() called\n");

  CmiHiveSyncPending = FALSE;

  /* Acquire hive list lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  Entry = CmiHiveListHead.Flink;
  while (Entry != &CmiHiveListHead)
    {
      Hive = CONTAINING_RECORD(Entry, REGISTRY_HIVE, HiveList);

      if (!(IsNoFileHive(Hive) || IsNoSynchHive(Hive)))
	{
	  /* Flush non-volatile hive */
	  CmiFlushRegistryHive(Hive);
	}

      Entry = Entry->Flink;
    }

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  DPRINT("DeferredContext %x\n", DeferredContext);
  ExFreePool(DeferredContext);

  DPRINT("CmiHiveSyncRoutine() done\n");
}


static VOID STDCALL
CmiHiveSyncDpcRoutine(PKDPC Dpc,
		      PVOID DeferredContext,
		      PVOID SystemArgument1,
		      PVOID SystemArgument2)
{
  PWORK_QUEUE_ITEM WorkQueueItem;

  WorkQueueItem = ExAllocatePool(NonPagedPool,
				 sizeof(WORK_QUEUE_ITEM));
  if (WorkQueueItem == NULL)
    {
      DbgPrint("Failed to allocate work item\n");
      return;
    }

  ExInitializeWorkItem(WorkQueueItem,
		       CmiHiveSyncRoutine,
		       WorkQueueItem);

  DPRINT("DeferredContext %x\n", WorkQueueItem);
  ExQueueWorkItem(WorkQueueItem,
		  CriticalWorkQueue);
}


VOID
CmiSyncHives(VOID)
{
  LARGE_INTEGER Timeout;

  DPRINT("CmiSyncHives() called\n");

  if (CmiHiveSyncEnabled == FALSE ||
      CmiHiveSyncPending == TRUE)
    return;

  CmiHiveSyncPending = TRUE;

  Timeout.QuadPart = (LONGLONG)-50000000;
  KeSetTimer(&CmiHiveSyncTimer,
	     Timeout,
	     &CmiHiveSyncDpc);
}

/* EOF */
