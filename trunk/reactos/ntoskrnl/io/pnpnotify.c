/* $Id:$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/pnpnotify.c
 * PURPOSE:         Plug & Play notification functions
 * 
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 *                  Herv� Poussineau (hpoussin@reactos.com)
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include <ntoskrnl.h>
#include <internal/debug.h>

/* TYPES *******************************************************************/

typedef struct _PNP_NOTIFY_ENTRY
{
	LIST_ENTRY PnpNotifyList;
	IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
	PVOID Context;
	UNICODE_STRING Guid;
	PFILE_OBJECT FileObject;
	PDRIVER_NOTIFICATION_CALLBACK_ROUTINE PnpNotificationProc;
} PNP_NOTIFY_ENTRY, *PPNP_NOTIFY_ENTRY;

static KGUARDED_MUTEX PnpNotifyListLock;
static LIST_ENTRY PnpNotifyListHead;

#define TAG_PNP_NOTIFY  TAG('P', 'n', 'P', 'N')

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
ULONG
STDCALL
IoPnPDeliverServicePowerNotification(
	ULONG		VetoedPowerOperation OPTIONAL,
	ULONG		PowerNotification,
	ULONG		Unknown OPTIONAL,
	BOOLEAN  	Synchronous
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoRegisterPlugPlayNotification(
	IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
	IN ULONG EventCategoryFlags,
	IN PVOID EventCategoryData OPTIONAL,
	IN PDRIVER_OBJECT DriverObject,
	IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
	IN PVOID Context,
	OUT PVOID *NotificationEntry)
{
	PPNP_NOTIFY_ENTRY Entry;
	PWSTR SymbolicLinkList;
	NTSTATUS Status;
	
	PAGED_CODE();
	
	DPRINT("IoRegisterPlugPlayNotification(EventCategory 0x%x, EventCategoryFlags 0x%lx, DriverObject %p) called.\n",
		EventCategory,
		EventCategoryFlags,
		DriverObject);
	
	ObReferenceObject(DriverObject);
	
	/* Try to allocate entry for notification before sending any notification */
	Entry = ExAllocatePoolWithTag(
		NonPagedPool,
		sizeof(PNP_NOTIFY_ENTRY),
		TAG_PNP_NOTIFY);
	if (!Entry)
	{
		DPRINT("ExAllocatePool() failed\n");
		ObDereferenceObject(DriverObject);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	
	if (EventCategory == EventCategoryTargetDeviceChange
		&& EventCategoryFlags & PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES)
	{
		Status = IoGetDeviceInterfaces(
			(LPGUID)EventCategoryData,
			NULL, /* PhysicalDeviceObject OPTIONAL */
			0, /* Flags */
			&SymbolicLinkList);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("IoGetDeviceInterfaces() failed with status 0x%08lx\n", Status);
			ExFreePoolWithTag(Entry, TAG_PNP_NOTIFY);
			ObDereferenceObject(DriverObject);
			return Status;
		}
		/* FIXME: enumerate SymbolicLinkList */
		DPRINT1("IoRegisterPlugPlayNotification(): need to send notifications for existing interfaces!\n");
		ExFreePool(SymbolicLinkList);
	}
	
	Entry->PnpNotificationProc = CallbackRoutine;
	Entry->EventCategory = EventCategory;
	Entry->Context = Context;
	switch (EventCategory)
	{
		case EventCategoryDeviceInterfaceChange:
		{
			Status = RtlStringFromGUID(EventCategoryData, &Entry->Guid);
			if (!NT_SUCCESS(Status))
			{
				ExFreePoolWithTag(Entry, TAG_PNP_NOTIFY);
				ObDereferenceObject(DriverObject);
				return Status;
			}
			break;
		}
		case EventCategoryHardwareProfileChange:
		{
			/* nothing to do */
			break;
		}
		case EventCategoryTargetDeviceChange:
		{
			Entry->FileObject = (PFILE_OBJECT)EventCategoryData;
			break;
		}
		default:
		{
			DPRINT1("IoRegisterPlugPlayNotification(): unknown EventCategory 0x%x UNIMPLEMENTED\n", EventCategory);
			break;
		}
	}
	
	KeAcquireGuardedMutex(&PnpNotifyListLock);
	InsertHeadList(&PnpNotifyListHead,
		&Entry->PnpNotifyList);
	KeReleaseGuardedMutex(&PnpNotifyListLock);
	
	DPRINT("IoRegisterPlugPlayNotification() returns NotificationEntry %p\n",
		Entry);
	*NotificationEntry = Entry;
	return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
IoUnregisterPlugPlayNotification(
	IN PVOID NotificationEntry)
{
	PPNP_NOTIFY_ENTRY Entry;
	
	PAGED_CODE();
	
	Entry = (PPNP_NOTIFY_ENTRY)NotificationEntry;
	DPRINT("IoUnregisterPlugPlayNotification(NotificationEntry %p) called\n",
		Entry);
	
	KeAcquireGuardedMutex(&PnpNotifyListLock);
	RtlFreeUnicodeString(&Entry->Guid);
	RemoveEntryList(&Entry->PnpNotifyList);
	KeReleaseGuardedMutex(&PnpNotifyListLock);
	
	return STATUS_SUCCESS;
}

VOID
IopNotifyPlugPlayNotification(
	IN PDEVICE_OBJECT DeviceObject,
	IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
	IN GUID* Event,
	IN PVOID EventCategoryData1,
	IN PVOID EventCategoryData2)
{
	PPNP_NOTIFY_ENTRY ChangeEntry;
	PLIST_ENTRY Entry;
	PVOID NotificationStructure;
	BOOLEAN CallCurrentEntry;
	
	ASSERT(DeviceObject);
	
	KeAcquireGuardedMutex(&PnpNotifyListLock);
	if (IsListEmpty(&PnpNotifyListHead))
	{
		KeReleaseGuardedMutex(&PnpNotifyListLock);
		return;
	}
	
	switch (EventCategory)
	{
		case EventCategoryDeviceInterfaceChange:
		{
			PDEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationInfos;
			NotificationStructure = NotificationInfos = ExAllocatePoolWithTag(
				PagedPool,
				sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION),
				TAG_PNP_NOTIFY);
			NotificationInfos->Version = 1;
			NotificationInfos->Size = sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION);
			RtlCopyMemory(&NotificationInfos->Event, Event, sizeof(GUID));
			RtlCopyMemory(&NotificationInfos->InterfaceClassGuid, EventCategoryData1, sizeof(GUID));
			NotificationInfos->SymbolicLinkName = (PUNICODE_STRING)EventCategoryData2;
			break;
		}
		case EventCategoryHardwareProfileChange:
		{
			PHWPROFILE_CHANGE_NOTIFICATION NotificationInfos;
			NotificationStructure = NotificationInfos = ExAllocatePoolWithTag(
				PagedPool,
				sizeof(HWPROFILE_CHANGE_NOTIFICATION),
				TAG_PNP_NOTIFY);
			NotificationInfos->Version = 1;
			NotificationInfos->Size = sizeof(HWPROFILE_CHANGE_NOTIFICATION);
			RtlCopyMemory(&NotificationInfos->Event, Event, sizeof(GUID));
			break;
		}
		case EventCategoryTargetDeviceChange:
		{
			PTARGET_DEVICE_REMOVAL_NOTIFICATION NotificationInfos;
			NotificationStructure = NotificationInfos = ExAllocatePoolWithTag(
				PagedPool,
				sizeof(TARGET_DEVICE_REMOVAL_NOTIFICATION),
				TAG_PNP_NOTIFY);
			NotificationInfos->Version = 1;
			NotificationInfos->Size = sizeof(TARGET_DEVICE_REMOVAL_NOTIFICATION);
			RtlCopyMemory(&NotificationInfos->Event, Event, sizeof(GUID));
			NotificationInfos->FileObject = (PFILE_OBJECT)EventCategoryData1;
			break;
		}
		default:
		{
			DPRINT1("IopNotifyPlugPlayNotification(): unknown EventCategory 0x%x UNIMPLEMENTED\n", EventCategory);
			return;
		}
	}
	
	/* Loop through procedures registred in PnpNotifyListHead
	 * list to find those that meet some criteria.
	 */
	
	Entry = PnpNotifyListHead.Flink;
	while (Entry != &PnpNotifyListHead)
	{
		ChangeEntry = CONTAINING_RECORD(Entry, PNP_NOTIFY_ENTRY, PnpNotifyList);
		CallCurrentEntry = FALSE;
		
		switch (EventCategory)
		{
			case EventCategoryDeviceInterfaceChange:
			{
				if (ChangeEntry->EventCategory == EventCategory
					&& RtlCompareUnicodeString(&ChangeEntry->Guid, (PUNICODE_STRING)EventCategoryData1, FALSE) == 0)
				{
					CallCurrentEntry = TRUE;
				}
				break;
			}
			case EventCategoryHardwareProfileChange:
			{
				CallCurrentEntry = TRUE;
				break;
			}
			case EventCategoryTargetDeviceChange:
			{
				if (ChangeEntry->FileObject == (PFILE_OBJECT)EventCategoryData1)
					CallCurrentEntry = TRUE;
			}
			default:
			{
				DPRINT1("IopNotifyPlugPlayNotification(): unknown EventCategory 0x%x UNIMPLEMENTED\n", EventCategory);
				break;
			}
		}
		
		if (CallCurrentEntry)
		{
			/* Call entry into new allocated memory */
			DPRINT("IopNotifyPlugPlayNotification(): found suitable callback %p\n",
				ChangeEntry);
			
			(ChangeEntry->PnpNotificationProc)(
				NotificationStructure,
				ChangeEntry->Context);
		}
		
		Entry = Entry->Flink;
	}
	KeReleaseGuardedMutex(&PnpNotifyListLock);
	ExFreePoolWithTag(NotificationStructure, TAG_PNP_NOTIFY);
}

VOID INIT_FUNCTION
IopInitPnpNotificationImplementation(VOID)
{
	KeInitializeGuardedMutex(&PnpNotifyListLock);
	InitializeListHead(&PnpNotifyListHead);
}

/* EOF */
