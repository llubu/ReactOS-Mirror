/* $Id: registry.c,v 1.14 2002/06/12 23:21:45 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Rtl registry functions
 * FILE:              lib/ntdll/rtl/registry.c
 * PROGRAMER:         Eric Kohl
 * REVISION HISTORY:
 *                    2000/08/11: Created
 */

/*
 * TODO:
 *   - finish RtlQueryRegistryValues()
 *	- support RTL_QUERY_REGISTRY_DELETE
 *
 *   - finish RtlFormatCurrentUserKeyPath()
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntdll/registry.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL
RtlCheckRegistryKey(IN ULONG RelativeTo,
		    IN PWSTR Path)
{
  HANDLE KeyHandle;
  NTSTATUS Status;

  Status = RtlpGetRegistryHandle(RelativeTo,
				 Path,
				 FALSE,
				 &KeyHandle);
  if (!NT_SUCCESS(Status))
    return(Status);

  NtClose(KeyHandle);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlCreateRegistryKey(IN ULONG RelativeTo,
		     IN PWSTR Path)
{
  HANDLE KeyHandle;
  NTSTATUS Status;

  Status = RtlpGetRegistryHandle(RelativeTo,
				 Path,
				 TRUE,
				 &KeyHandle);
  if (!NT_SUCCESS(Status))
    return(Status);

  NtClose(KeyHandle);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlDeleteRegistryValue(IN ULONG RelativeTo,
		       IN PWSTR Path,
		       IN PWSTR ValueName)
{
  HANDLE KeyHandle;
  NTSTATUS Status;
  UNICODE_STRING Name;

  Status = RtlpGetRegistryHandle(RelativeTo,
				 Path,
				 FALSE,
				 &KeyHandle);
  if (!NT_SUCCESS(Status))
    return(Status);

  RtlInitUnicodeString(&Name,
		       ValueName);

  Status = NtDeleteValueKey(KeyHandle,
			    &Name);

  NtClose(KeyHandle);

  return(Status);
}


NTSTATUS STDCALL
RtlFormatCurrentUserKeyPath(PUNICODE_STRING KeyPath)
{
  /* FIXME: !!! */
  RtlCreateUnicodeString(KeyPath,
			 L"\\Registry\\User\\.Default");
  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
RtlOpenCurrentUser(IN ACCESS_MASK DesiredAccess,
		   OUT PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyPath;
  NTSTATUS Status;

  Status = RtlFormatCurrentUserKeyPath(&KeyPath);
  if (NT_SUCCESS(Status))
    {
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyPath,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);
      Status = NtOpenKey(KeyHandle,
			 DesiredAccess,
			 &ObjectAttributes);
      RtlFreeUnicodeString(&KeyPath);
      if (NT_SUCCESS(Status))
	return(STATUS_SUCCESS);
    }

  RtlInitUnicodeString(&KeyPath,
		       L"\\Registry\\User\\.Default");

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyPath,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = NtOpenKey(KeyHandle,
		     DesiredAccess,
		     &ObjectAttributes);
  return(Status);
}


NTSTATUS STDCALL
RtlQueryRegistryValues(IN ULONG RelativeTo,
		       IN PWSTR Path,
		       IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
		       IN PVOID Context,
		       IN PVOID Environment)
{
  NTSTATUS Status;
  HANDLE BaseKeyHandle;
  HANDLE CurrentKeyHandle;
  PRTL_QUERY_REGISTRY_TABLE QueryEntry;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  PKEY_VALUE_FULL_INFORMATION FullValueInfo;
  ULONG BufferSize;
  ULONG ResultSize;
  ULONG Index;
  ULONG StringLen;
  PWSTR StringPtr;
  PWSTR ExpandBuffer;
  UNICODE_STRING EnvValue;
  UNICODE_STRING EnvExpandedValue;

  DPRINT("RtlQueryRegistryValues() called\n");

  Status = RtlpGetRegistryHandle(RelativeTo,
				 Path,
				 FALSE,
				 &BaseKeyHandle);
  if (!NT_SUCCESS(Status))
    return(Status);

  CurrentKeyHandle = BaseKeyHandle;
  QueryEntry = QueryTable;
  while ((QueryEntry->QueryRoutine != NULL) ||
	 (QueryEntry->Name != NULL))
    {
      if ((QueryEntry->QueryRoutine == NULL) &&
	  ((QueryEntry->Flags & RTL_QUERY_REGISTRY_SUBKEY) != 0))
	{
	  Status = STATUS_INVALID_PARAMETER;
	  break;
	}

      DPRINT("Name: %S\n", QueryEntry->Name);

      if (((QueryEntry->Flags & (RTL_QUERY_REGISTRY_SUBKEY | RTL_QUERY_REGISTRY_TOPKEY)) != 0) &&
	  (BaseKeyHandle != CurrentKeyHandle))
	{
	  NtClose(CurrentKeyHandle);
	  CurrentKeyHandle = BaseKeyHandle;
	}

      if (QueryEntry->Flags & RTL_QUERY_REGISTRY_SUBKEY)
	{
	  DPRINT("Open new subkey: %S\n", QueryEntry->Name);

	  RtlInitUnicodeString(&KeyName,
			       QueryEntry->Name);
	  InitializeObjectAttributes(&ObjectAttributes,
				     &KeyName,
				     OBJ_CASE_INSENSITIVE,
				     BaseKeyHandle,
				     NULL);
	  Status = NtOpenKey(&CurrentKeyHandle,
			     KEY_ALL_ACCESS,
			     &ObjectAttributes);
	  if (!NT_SUCCESS(Status))
	    break;
	}
      else if (QueryEntry->Flags & RTL_QUERY_REGISTRY_DIRECT)
	{
	  DPRINT("Query value directly: %S\n", QueryEntry->Name);

	  RtlInitUnicodeString(&KeyName,
			       QueryEntry->Name);

	  BufferSize = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + 4096;
	  ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(),
				      0,
				      BufferSize);
	  if (ValueInfo == NULL)
	    {
	      Status = STATUS_NO_MEMORY;
	      break;
	    }

	  Status = NtQueryValueKey(CurrentKeyHandle,
				   &KeyName,
				   KeyValuePartialInformation,
				   ValueInfo,
				   BufferSize,
				   &ResultSize);
	  if (!NT_SUCCESS(Status))
	    {
	      if (QueryEntry->Flags & RTL_QUERY_REGISTRY_REQUIRED)
		{
		  RtlFreeHeap(RtlGetProcessHeap(), 0, ValueInfo);
		  Status = STATUS_OBJECT_NAME_NOT_FOUND;
		  break;
		}
	
	      if (QueryEntry->DefaultType == REG_SZ)
		{
		  PUNICODE_STRING ValueString;
		  PUNICODE_STRING SourceString;

		  SourceString = (PUNICODE_STRING)QueryEntry->DefaultData;
		  ValueString = (PUNICODE_STRING)QueryEntry->EntryContext;
		  if (ValueString->Buffer == 0)
		    {
		      ValueString->Length = SourceString->Length;
		      ValueString->MaximumLength = SourceString->MaximumLength;
		      ValueString->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
							    0,
							    ValueString->MaximumLength);
		      if (!ValueString->Buffer)
			break;
		      ValueString->Buffer[0] = 0;
		      memcpy(ValueString->Buffer,
			     SourceString->Buffer,
			     SourceString->MaximumLength);
		    }
		  else
		    {
		      ValueString->Length = min(SourceString->Length,
						ValueString->MaximumLength - sizeof(WCHAR));
		      memcpy(ValueString->Buffer,
			     SourceString->Buffer,
			     ValueString->Length);
		      ((PWSTR)ValueString->Buffer)[ValueString->Length / sizeof(WCHAR)] = 0;
		    }
		}
	      else
		{
		  memcpy(QueryEntry->EntryContext,
			 QueryEntry->DefaultData,
			 QueryEntry->DefaultLength);
		}
	      Status = STATUS_SUCCESS;
	    }
	  else
	    {
	      if (ValueInfo->Type == REG_SZ ||
		  ValueInfo->Type == REG_EXPAND_SZ ||
		  ValueInfo->Type == REG_MULTI_SZ)
#if 0
		  (ValueInfo->Type == REG_EXPAND_SZ && (QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND)))
		  (ValueInfo->Type == REG_MULTI_SZ && (QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND)))
#endif
		{
		  PUNICODE_STRING ValueString;

		  ValueString = (PUNICODE_STRING)QueryEntry->EntryContext;
		  if (ValueString->Buffer == NULL)
		    {
		      ValueString->MaximumLength = ValueInfo->DataLength + sizeof(WCHAR);
		      ValueString->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
							    0,
							    ValueString->MaximumLength);
		      if (ValueString->Buffer == NULL)
			{
			  Status = STATUS_INSUFFICIENT_RESOURCES;
			  break;
			}
		      ValueString->Buffer[0] = 0;
		     }
		  ValueString->Length = min(ValueInfo->DataLength,
					    ValueString->MaximumLength - sizeof(WCHAR));
		  memcpy(ValueString->Buffer,
			 ValueInfo->Data,
			 ValueString->Length);
		  ((PWSTR)ValueString->Buffer)[ValueString->Length / sizeof(WCHAR)] = 0;
		}
	      else
		{
		  memcpy(QueryEntry->EntryContext,
			 ValueInfo->Data,
			 ValueInfo->DataLength);
		}
	    }

	  if (QueryEntry->Flags & RTL_QUERY_REGISTRY_DELETE)
	    {
	      DPRINT1("FIXME: Delete value: %S\n", QueryEntry->Name);

	    }

	  RtlFreeHeap(RtlGetProcessHeap(),
		      0,
		      ValueInfo);
	}
      else
	{
	  DPRINT("Query value via query routine: %S\n", QueryEntry->Name);
	  if (QueryEntry->Name != NULL)
	    {
	      RtlInitUnicodeString(&KeyName,
				   QueryEntry->Name);

	      BufferSize = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + 4096;
	      ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(),
					  0,
					  BufferSize);
	      if (ValueInfo == NULL)
		{
		  Status = STATUS_NO_MEMORY;
		  break;
		}

	      Status = NtQueryValueKey(CurrentKeyHandle,
				       &KeyName,
				       KeyValuePartialInformation,
				       ValueInfo,
				       BufferSize,
				       &ResultSize);
	      if (!NT_SUCCESS(Status))
		{
		  Status = QueryEntry->QueryRoutine(QueryEntry->Name,
						    QueryEntry->DefaultType,
						    QueryEntry->DefaultData,
						    QueryEntry->DefaultLength,
						    Context,
						    QueryEntry->EntryContext);
		}
	      else if ((ValueInfo->Type == REG_MULTI_SZ) &&
		       !(QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
		{
		  DPRINT("Expand REG_MULTI_SZ type\n");
		  StringPtr = (PWSTR)ValueInfo->Data;
		  while (*StringPtr != 0)
		    {
		      StringLen = (wcslen(StringPtr) + 1) * sizeof(WCHAR);
		      Status = QueryEntry->QueryRoutine(QueryEntry->Name,
							REG_SZ,
							(PVOID)StringPtr,
							StringLen,
							Context,
							QueryEntry->EntryContext);
		      if(!NT_SUCCESS(Status))
			break;
		      StringPtr = (PWSTR)((PUCHAR)StringPtr + StringLen);
		    }
		}
	      else if ((ValueInfo->Type == REG_EXPAND_SZ) &&
		       !(QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
		{
		  DPRINT("Expand REG_EXPAND_SZ type\n");

		  ExpandBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
						 0,
						 ValueInfo->DataLength * 2);
		  if (ExpandBuffer == NULL)
		    {
		      Status = STATUS_NO_MEMORY;
		      break;
		    }

		  RtlInitUnicodeString(&EnvValue,
				       (PWSTR)ValueInfo->Data);
		  EnvExpandedValue.Length = 0;
		  EnvExpandedValue.MaximumLength = ValueInfo->DataLength * 2 * sizeof(WCHAR);
		  EnvExpandedValue.Buffer = ExpandBuffer;
		  *ExpandBuffer = 0;

		  RtlExpandEnvironmentStrings_U(Environment,
						&EnvValue,
						&EnvExpandedValue,
						&StringLen);

		  StringLen = (wcslen(ExpandBuffer) + 1) * sizeof(WCHAR);
		  Status = QueryEntry->QueryRoutine(FullValueInfo->Name,
						    REG_SZ,
						    (PVOID)ExpandBuffer,
						    StringLen,
						    Context,
						    QueryEntry->EntryContext);

		  RtlFreeHeap(RtlGetProcessHeap(),
			      0,
			      ExpandBuffer);
		}
	      else
		{
		  Status = QueryEntry->QueryRoutine(QueryEntry->Name,
						    ValueInfo->Type,
						    ValueInfo->Data,
						    ValueInfo->DataLength,
						    Context,
						    QueryEntry->EntryContext);
		}

	      if (QueryEntry->Flags & RTL_QUERY_REGISTRY_DELETE)
		{
		  DPRINT1("FIXME: Delete value: %S\n", QueryEntry->Name);

		}

	      RtlFreeHeap(RtlGetProcessHeap(),
			  0,
			  ValueInfo);
	      if (!NT_SUCCESS(Status))
		break;
	    }
	  else if (QueryEntry->Flags & RTL_QUERY_REGISTRY_NOVALUE)
	    {
	      DPRINT("Simple callback\n");
	      Status = QueryEntry->QueryRoutine(NULL,
						REG_NONE,
						NULL,
						0,
						Context,
						QueryEntry->EntryContext);
	      if (!NT_SUCCESS(Status))
		break;
	    }
	  else
	    {
	      DPRINT("Enumerate values\n");

	      BufferSize = sizeof(KEY_VALUE_FULL_INFORMATION) + 4096;
	      FullValueInfo = RtlAllocateHeap(RtlGetProcessHeap(),
					      0,
					      BufferSize);
	      if (FullValueInfo == NULL)
		{
		  Status = STATUS_NO_MEMORY;
		  break;
		}

	      Index = 0;
	      while (TRUE)
		{
		  Status = NtEnumerateValueKey(CurrentKeyHandle,
					       Index,
					       KeyValueFullInformation,
					       FullValueInfo,
					       BufferSize,
					       &ResultSize);
		  if (!NT_SUCCESS(Status))
		    {
		      if ((Status == STATUS_NO_MORE_ENTRIES) &&
			  (Index == 0) &&
			  (QueryEntry->Flags & RTL_QUERY_REGISTRY_REQUIRED))
			{
			  Status = STATUS_OBJECT_NAME_NOT_FOUND;
			}
		      else if (Status == STATUS_NO_MORE_ENTRIES)
			{
			  Status = STATUS_SUCCESS;
			}
		      break;
		    }

		  DPRINT("FullValueInfo->Type: %lu\n", FullValueInfo->Type);
		  if ((FullValueInfo->Type == REG_MULTI_SZ) &&
		      !(QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
		    {
		      DPRINT("Expand REG_MULTI_SZ type\n");
		      StringPtr = (PWSTR)((PVOID)FullValueInfo + FullValueInfo->DataOffset);
		      while (*StringPtr != 0)
			{
			  StringLen = (wcslen(StringPtr) + 1) * sizeof(WCHAR);
			  Status = QueryEntry->QueryRoutine(QueryEntry->Name,
							    REG_SZ,
							    (PVOID)StringPtr,
							    StringLen,
							    Context,
							    QueryEntry->EntryContext);
			  if(!NT_SUCCESS(Status))
			    break;
			  StringPtr = (PWSTR)((PUCHAR)StringPtr + StringLen);
			}
		    }
		  else if ((FullValueInfo->Type == REG_EXPAND_SZ) &&
			   !(QueryEntry->Flags & RTL_QUERY_REGISTRY_NOEXPAND))
		    {
		      DPRINT("Expand REG_EXPAND_SZ type\n");

		      StringPtr = (PWSTR)((PVOID)FullValueInfo + FullValueInfo->DataOffset);
		      ExpandBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
						     0,
						     FullValueInfo->DataLength * 2);
		      if (ExpandBuffer == NULL)
			{
			  Status = STATUS_NO_MEMORY;
			  break;
			}

		      RtlInitUnicodeString(&EnvValue,
					   StringPtr);
		      EnvExpandedValue.Length = 0;
		      EnvExpandedValue.MaximumLength = FullValueInfo->DataLength * 2 * sizeof(WCHAR);
		      EnvExpandedValue.Buffer = ExpandBuffer;
		      *ExpandBuffer = 0;

		      RtlExpandEnvironmentStrings_U(Environment,
						    &EnvValue,
						    &EnvExpandedValue,
						    &StringLen);

		      StringLen = (wcslen(ExpandBuffer) + 1) * sizeof(WCHAR);
		      Status = QueryEntry->QueryRoutine(FullValueInfo->Name,
							REG_SZ,
							(PVOID)ExpandBuffer,
							StringLen,
							Context,
							QueryEntry->EntryContext);

		      RtlFreeHeap(RtlGetProcessHeap(),
				  0,
				  ExpandBuffer);
		    }
		  else
		    {
		      Status = QueryEntry->QueryRoutine(FullValueInfo->Name,
							FullValueInfo->Type,
							(PVOID)FullValueInfo + FullValueInfo->DataOffset,
							FullValueInfo->DataLength,
							Context,
							QueryEntry->EntryContext);
		    }

		  if (!NT_SUCCESS(Status))
		    break;

		  /* FIXME: How will these be deleted? */

		  Index++;
		}

	      RtlFreeHeap(RtlGetProcessHeap(),
			  0,
			  FullValueInfo);

	      if (!NT_SUCCESS(Status))
		break;
	    }
	}

      QueryEntry++;
    }

  if (CurrentKeyHandle != BaseKeyHandle)
    NtClose(CurrentKeyHandle);

  NtClose(BaseKeyHandle);

  return(Status);
}


NTSTATUS STDCALL
RtlWriteRegistryValue(IN ULONG RelativeTo,
		      IN PWSTR Path,
		      IN PWSTR ValueName,
		      IN ULONG ValueType,
		      IN PVOID ValueData,
		      IN ULONG ValueLength)
{
  HANDLE KeyHandle;
  NTSTATUS Status;
  UNICODE_STRING Name;

  Status = RtlpGetRegistryHandle(RelativeTo,
				 Path,
				 TRUE,
				 &KeyHandle);
  if (!NT_SUCCESS(Status))
    return(Status);

  RtlInitUnicodeString(&Name,
		       ValueName);

  Status = NtSetValueKey(KeyHandle,
			 &Name,
			 0,
			 ValueType,
			 ValueData,
			 ValueLength);
  if (NT_SUCCESS(Status))
    NtClose(KeyHandle);

  return(Status);
}


NTSTATUS STDCALL
RtlpNtCreateKey(OUT HANDLE KeyHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN ULONG Unused1,
		OUT PULONG Disposition,
		IN ULONG Unused2)
{
  if (ObjectAttributes != NULL)
    ObjectAttributes->Attributes &= ~(OBJ_PERMANENT | OBJ_EXCLUSIVE);

  return(NtCreateKey(KeyHandle,
		     DesiredAccess,
		     ObjectAttributes,
		     0,
		     NULL,
		     0,
		     Disposition));
}


NTSTATUS STDCALL
RtlpNtEnumerateSubKey(IN HANDLE KeyHandle,
		      OUT PUNICODE_STRING SubKeyName,
		      IN ULONG Index,
		      IN ULONG Unused)
{
  PKEY_BASIC_INFORMATION KeyInfo = NULL;
  ULONG BufferLength = 0;
  ULONG ReturnedLength;
  NTSTATUS Status;

  if (SubKeyName->MaximumLength != 0)
    {
      BufferLength = SubKeyName->MaximumLength +
		     sizeof(KEY_BASIC_INFORMATION);
      KeyInfo = RtlAllocateHeap(RtlGetProcessHeap(),
				0,
				BufferLength);
      if (KeyInfo == NULL)
	return(STATUS_NO_MEMORY);
    }

  Status = NtEnumerateKey(KeyHandle,
			  Index,
			  KeyBasicInformation,
			  KeyInfo,
			  BufferLength,
			  &ReturnedLength);
  if (NT_SUCCESS(Status))
    {
      if (KeyInfo->NameLength + sizeof(WCHAR) <= SubKeyName->MaximumLength)
	{
	  memmove(SubKeyName->Buffer,
		  KeyInfo->Name,
		  KeyInfo->NameLength);
	  SubKeyName->Buffer[KeyInfo->NameLength / sizeof(WCHAR)] = 0;
	  SubKeyName->Length = KeyInfo->NameLength;
	}
      else
	{
	  Status = STATUS_BUFFER_OVERFLOW;
	}
    }

  if (KeyInfo != NULL)
    {
      RtlFreeHeap(RtlGetProcessHeap(),
		  0,
		  KeyInfo);
    }

  return(Status);
}


NTSTATUS STDCALL
RtlpNtMakeTemporaryKey(IN HANDLE KeyHandle)
{
  return(NtDeleteKey(KeyHandle));
}


NTSTATUS STDCALL
RtlpNtOpenKey(OUT HANDLE KeyHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES ObjectAttributes,
	      IN ULONG Unused)
{
  if (ObjectAttributes != NULL)
    ObjectAttributes->Attributes &= ~(OBJ_PERMANENT | OBJ_EXCLUSIVE);

  return(NtOpenKey(KeyHandle,
		   DesiredAccess,
		   ObjectAttributes));
}


NTSTATUS STDCALL
RtlpNtQueryValueKey(IN HANDLE KeyHandle,
		    OUT PULONG Type OPTIONAL,
		    OUT PVOID Data OPTIONAL,
		    IN OUT PULONG DataLength OPTIONAL,
		    IN ULONG Unused)
{
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  UNICODE_STRING ValueName;
  ULONG BufferLength;
  ULONG ReturnedLength;
  NTSTATUS Status;

  RtlInitUnicodeString(&ValueName,
		       NULL);

  BufferLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION);
  if (DataLength != NULL)
    BufferLength = *DataLength;

  ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(),
			      0,
			      BufferLength);
  if (ValueInfo == NULL)
    return(STATUS_NO_MEMORY);

  Status = NtQueryValueKey(KeyHandle,
			   &ValueName,
			   KeyValuePartialInformation,
			   ValueInfo,
			   BufferLength,
			   &ReturnedLength);
  if (NT_SUCCESS(Status))
    {
      if (DataLength != NULL)
	*DataLength = ValueInfo->DataLength;

      if (Type != NULL)
	*Type = ValueInfo->Type;

      if (Data != NULL)
	{
	  memmove(Data,
		  ValueInfo->Data,
		  ValueInfo->DataLength);
	}
    }

  RtlFreeHeap(RtlGetProcessHeap(),
	      0,
	      ValueInfo);

  return(Status);
}


NTSTATUS STDCALL
RtlpNtSetValueKey(IN HANDLE KeyHandle,
		  IN ULONG Type,
		  IN PVOID Data,
		  IN ULONG DataLength)
{
  UNICODE_STRING ValueName;

  RtlInitUnicodeString(&ValueName,
		       NULL);
  return(NtSetValueKey(KeyHandle,
		       &ValueName,
		       0,
		       Type,
		       Data,
		       DataLength));
}


/* INTERNAL FUNCTIONS ******************************************************/

NTSTATUS
RtlpGetRegistryHandle(ULONG RelativeTo,
		      PWSTR Path,
		      BOOLEAN Create,
		      PHANDLE KeyHandle)
{
  UNICODE_STRING KeyName;
  WCHAR KeyBuffer[MAX_PATH];
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;

  DPRINT("RtlpGetRegistryHandle()\n");

  if (RelativeTo & RTL_REGISTRY_HANDLE)
    {
      Status = NtDuplicateObject(NtCurrentProcess(),
				 (HANDLE)Path,
				 NtCurrentProcess(),
				 KeyHandle,
				 0,
				 FALSE,
				 DUPLICATE_SAME_ACCESS);
      return(Status);
    }

  if (RelativeTo & RTL_REGISTRY_OPTIONAL)
    RelativeTo &= ~RTL_REGISTRY_OPTIONAL;

  if (RelativeTo >= RTL_REGISTRY_MAXIMUM)
    return(STATUS_INVALID_PARAMETER);

  KeyName.Length = 0;
  KeyName.MaximumLength = MAX_PATH;
  KeyName.Buffer = KeyBuffer;
  KeyBuffer[0] = 0;

  switch (RelativeTo)
    {
      case RTL_REGISTRY_ABSOLUTE:
	RtlAppendUnicodeToString(&KeyName,
				 L"\\");
	break;

      case RTL_REGISTRY_SERVICES:
	RtlAppendUnicodeToString(&KeyName,
				 L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
	break;

      case RTL_REGISTRY_CONTROL:
	RtlAppendUnicodeToString(&KeyName,
				 L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\");
	break;

      case RTL_REGISTRY_WINDOWS_NT:
	RtlAppendUnicodeToString(&KeyName,
				 L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\");
	break;

      case RTL_REGISTRY_DEVICEMAP:
	RtlAppendUnicodeToString(&KeyName,
				 L"\\Registry\\Machine\\Hardware\\DeviceMap\\");
	break;

      case RTL_REGISTRY_USER:
	Status = RtlFormatCurrentUserKeyPath(&KeyName);
	if (!NT_SUCCESS(Status))
	  return(Status);
	break;

      /* ReactOS specific */
      case RTL_REGISTRY_ENUM:
	RtlAppendUnicodeToString(&KeyName,
				 L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
	break;
    }

  DPRINT("KeyName %wZ\n", &KeyName);

  if (Path[0] == L'\\' && RelativeTo != RTL_REGISTRY_ABSOLUTE)
    {
      Path++;
    }
  RtlAppendUnicodeToString(&KeyName,
			   Path);

  DPRINT("KeyName %wZ\n", &KeyName);

  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  if (Create == TRUE)
    {
      Status = NtCreateKey(KeyHandle,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes,
			   0,
			   NULL,
			   0,
			   NULL);
    }
  else
    {
      Status = NtOpenKey(KeyHandle,
			 KEY_ALL_ACCESS,
			 &ObjectAttributes);
    }

  return(Status);
}

/* EOF */
