/* $Id: bootlog.c,v 1.5 2004/09/28 12:50:23 ekohl Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/bootlog.c
 * PURPOSE:        Boot log file support
 * PROGRAMMER:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


/* GLOBALS ******************************************************************/

static BOOLEAN IopBootLogCreate = FALSE;
static BOOLEAN IopBootLogEnabled = FALSE;
static BOOLEAN IopLogFileEnabled = FALSE;
static ULONG IopLogEntryCount = 0;
static ERESOURCE IopBootLogResource;


/* FUNCTIONS ****************************************************************/

VOID INIT_FUNCTION
IopInitBootLog(VOID)
{
  ExInitializeResourceLite(&IopBootLogResource);
}


VOID
IopStartBootLog(VOID)
{
  IopBootLogCreate = TRUE;
  IopBootLogEnabled = TRUE;
}


VOID
IopStopBootLog(VOID)
{
  IopBootLogEnabled = FALSE;
}


VOID
IopBootLog(PUNICODE_STRING DriverName,
	   BOOLEAN Success)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR Buffer[256];
  WCHAR ValueNameBuffer[8];
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE ControlSetKey;
  HANDLE BootLogKey;
  NTSTATUS Status;

  if (IopBootLogEnabled == FALSE)
    return;

  ExAcquireResourceExclusiveLite(&IopBootLogResource, TRUE);

  DPRINT("Boot log: %S %wZ\n",
	 Success ? L"Loaded driver" : L"Did not load driver",
	 DriverName);

  swprintf(Buffer,
	   L"%s %wZ",
	   Success ? L"Loaded driver" : L"Did not load driver",
	   DriverName);

  swprintf(ValueNameBuffer,
	   L"%lu",
	   IopLogEntryCount);

  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\System\\CurrentControlSet");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = NtOpenKey(&ControlSetKey,
		     KEY_ALL_ACCESS,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  RtlInitUnicodeString(&KeyName, L"BootLog");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     ControlSetKey,
			     NULL);
  Status = NtCreateKey(&BootLogKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_NON_VOLATILE,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
      NtClose(ControlSetKey);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  RtlInitUnicodeString(&ValueName, ValueNameBuffer);
  Status = NtSetValueKey(BootLogKey,
			 &ValueName,
			 0,
			 REG_SZ,
			 (PVOID)Buffer,
			 (wcslen(Buffer) + 1) * sizeof(WCHAR));
  NtClose(BootLogKey);
  NtClose(ControlSetKey);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
    }
  else
    {
      IopLogEntryCount++;
    }

  ExReleaseResourceLite(&IopBootLogResource);
}


static NTSTATUS
IopWriteLogFile(PWSTR LogText)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  PWSTR CrLf = L"\r\n";
  NTSTATUS Status;

  DPRINT("IopWriteLogFile() called\n");

  RtlInitUnicodeString(&FileName,
		       L"\\SystemRoot\\rosboot.log");
  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_APPEND_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			0,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateFile() failed (Status %lx)\n", Status);
      return Status;
    }

  if (LogText != NULL)
    {
      Status = NtWriteFile(FileHandle,
			   NULL,
			   NULL,
			   NULL,
			   &IoStatusBlock,
			   (PVOID)LogText,
			   wcslen(LogText) * sizeof(WCHAR),
			   NULL,
			   NULL);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
	  NtClose(FileHandle);
	  return Status;
	}
    }

  /* L"\r\n" */
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       (PVOID)CrLf,
		       2 * sizeof(WCHAR),
		       NULL,
		       NULL);

  NtClose(FileHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
    }

  return Status;
}


static NTSTATUS
IopCreateLogFile(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  LARGE_INTEGER ByteOffset;
  WCHAR Signature;
  NTSTATUS Status;

  DPRINT("IopSaveBootLogToFile() called\n");

  ExAcquireResourceExclusiveLite(&IopBootLogResource, TRUE);

  RtlInitUnicodeString(&FileName,
		       L"\\SystemRoot\\rosboot.log");
  InitializeObjectAttributes(&ObjectAttributes,
			     &FileName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);

  Status = NtCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			0,
			0,
			FILE_SUPERSEDE,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateFile() failed (Status %lx)\n", Status);
      return Status;
    }

  ByteOffset.QuadPart = (LONGLONG)0;

  Signature = 0xFEFF;
  Status = NtWriteFile(FileHandle,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       (PVOID)&Signature,
		       sizeof(WCHAR),
		       &ByteOffset,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtWriteKey() failed (Status %lx)\n", Status);
    }

  NtClose(FileHandle);

  return Status;
}


VOID
IopSaveBootLogToFile(VOID)
{
  PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
  WCHAR ValueNameBuffer[8];
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;
  ULONG BufferSize;
  ULONG ResultLength;
  ULONG i;
  NTSTATUS Status;

  if (IopBootLogCreate == FALSE)
    return;

  DPRINT("IopSaveBootLogToFile() called\n");

  ExAcquireResourceExclusiveLite(&IopBootLogResource, TRUE);

  Status = IopCreateLogFile();
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopCreateLogFile() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  Status = IopWriteLogFile(L"ReactOS "KERNEL_VERSION_STR);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopWriteLogFile() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  Status = IopWriteLogFile(NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopWriteLogFile() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }


  BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256 * sizeof(WCHAR);
  KeyInfo = ExAllocatePool(PagedPool,
			   BufferSize);
  if (KeyInfo == NULL)
    {
      CHECKPOINT1;
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\System\\CurrentControlSet\\BootLog");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = NtOpenKey(&KeyHandle,
		     KEY_ALL_ACCESS,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      CHECKPOINT1;
      ExFreePool(KeyInfo);
      ExReleaseResourceLite(&IopBootLogResource);
      return;
    }

  for (i = 0; ; i++)
    {
      swprintf(ValueNameBuffer,
	       L"%lu", i);

      RtlInitUnicodeString(&ValueName,
			   ValueNameBuffer);

      Status = NtQueryValueKey(KeyHandle,
			       &ValueName,
			       KeyValuePartialInformation,
			       KeyInfo,
			       BufferSize,
			       &ResultLength);
      if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
	{
	  break;
	}

      if (!NT_SUCCESS(Status))
	{
	  CHECKPOINT1;
	  NtClose(KeyHandle);
	  ExFreePool(KeyInfo);
	  ExReleaseResourceLite(&IopBootLogResource);
	  return;
	}

      Status = IopWriteLogFile((PWSTR)&KeyInfo->Data);
      if (!NT_SUCCESS(Status))
	{
	  CHECKPOINT1;
	  NtClose(KeyHandle);
	  ExFreePool(KeyInfo);
	  ExReleaseResourceLite(&IopBootLogResource);
	  return;
	}

      /* Delete keys */
      NtDeleteValueKey(KeyHandle,
		       &ValueName);
    }

  NtClose(KeyHandle);

  ExFreePool(KeyInfo);

  IopLogFileEnabled = TRUE;
  ExReleaseResourceLite(&IopBootLogResource);

  DPRINT("IopSaveBootLogToFile() done\n");
}

/* EOF */
