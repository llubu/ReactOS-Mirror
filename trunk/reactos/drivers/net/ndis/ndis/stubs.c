/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/stubs.c
 * PURPOSE:     Stubs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <ndissys.h>


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteQueryStatistics(
    IN  NDIS_HANDLE     NdisAdapterHandle,
    IN  PNDIS_REQUEST   NdisRequest,
    IN  NDIS_STATUS     Status)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteUnbindAdapter(
    IN  NDIS_HANDLE UnbindAdapterContext,
    IN  NDIS_STATUS Status)
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
VOID
EXPORT
NdisInitializeListHead(
    IN  PLIST_ENTRY ListHead)
{
	InitializeListHead(ListHead);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisInterlockedAddUlong (
    IN  PULONG          Addend,
    IN  ULONG           Increment,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
  ExInterlockedAddUlong ( Addend, Increment, (PKSPIN_LOCK)SpinLock );
}


/*
 * @implemented
 */
PLIST_ENTRY
EXPORT
NdisInterlockedInsertHeadList(
    IN  PLIST_ENTRY     ListHead,
    IN  PLIST_ENTRY     ListEntry,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
  return ExInterlockedInsertHeadList ( ListHead, ListEntry, (PKSPIN_LOCK)SpinLock );
}


/*
 * @implemented
 */
PLIST_ENTRY
EXPORT
NdisInterlockedInsertTailList(
    IN  PLIST_ENTRY     ListHead,
    IN  PLIST_ENTRY     ListEntry,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
  return ExInterlockedInsertTailList ( ListHead, ListEntry, (PKSPIN_LOCK)SpinLock );
}


/*
 * @implemented
 */
PLIST_ENTRY
EXPORT
NdisInterlockedRemoveHeadList(
    IN  PLIST_ENTRY     ListHead,
    IN  PNDIS_SPIN_LOCK SpinLock)
{
  return ExInterlockedRemoveHeadList ( ListHead, (PKSPIN_LOCK)SpinLock );
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMapFile(
    OUT PNDIS_STATUS    Status,
    OUT PVOID           * MappedBuffer,
    IN  NDIS_HANDLE     FileHandle)
{
    UNIMPLEMENTED
}

typedef struct _NDIS_HANDLE_OBJECT
{
  HANDLE FileHandle;
  BOOL Mapped;
  ULONG FileLength;
  PVOID MapBuffer;
} NDIS_HANDLE_OBJECT, *PNDIS_HANDLE_OBJECT;

inline
PNDIS_HANDLE_OBJECT
NDIS_HANDLE_TO_POBJECT ( NDIS_HANDLE handle )
{
  return (PNDIS_HANDLE_OBJECT)handle;
}

inline
NDIS_HANDLE
NDIS_POBJECT_TO_HANDLE ( PNDIS_HANDLE_OBJECT obj )
{
  return (NDIS_HANDLE)obj;
}

const WCHAR* NDIS_FILE_FOLDER = L"\\SystemRoot\\System32\\Drivers\\";

/*
 * @implemented
 */
VOID
EXPORT
NdisCloseFile(
    IN  NDIS_HANDLE FileHandle)
{
  PNDIS_HANDLE_OBJECT FileHandleObject;

  ASSERT_IRQL(PASSIVE_LEVEL);

  ASSERT ( FileHandle );

  FileHandleObject = NDIS_HANDLE_TO_POBJECT(FileHandle);

  ASSERT ( FileHandleObject->FileHandle );

  /*
  if ( FileHandleObject->Mapped )
    NdisUnmapFile ( FileHandle );
  */

  ZwClose ( FileHandleObject->FileHandle );

  memset ( FileHandleObject, 0, sizeof(NDIS_HANDLE_OBJECT) );

  ExFreePool ( FileHandleObject );
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMDeInitializeDeviceInstance(
    IN  NDIS_HANDLE NdisMiniportHandle)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMRegisterLayeredMiniport(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength,
    OUT PNDIS_HANDLE                    DriverHandle)
{
    UNIMPLEMENTED

	return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMWanIndicateReceive(
    OUT PNDIS_STATUS    Status,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     NdisLinkContext,
    IN  PUCHAR          PacketBuffer,
    IN  UINT            PacketSize)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMWanIndicateReceiveComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMWanSendComplete(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  PNDIS_WAN_PACKET    Packet,
    IN  NDIS_STATUS         Status)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisOpenFile(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            FileHandle,
    OUT PUINT                   FileLength,
    IN  PNDIS_STRING            FileName,
    IN  NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress)
{
  NDIS_STRING FullFileName;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PNDIS_HANDLE_OBJECT FileHandleObject = NULL;
  IO_STATUS_BLOCK IoStatusBlock;

  ASSERT_IRQL(PASSIVE_LEVEL);

  *Status = NDIS_STATUS_SUCCESS;
  FullFileName.Buffer = NULL;

  ASSERT ( Status && FileName );

  FullFileName.Length = sizeof(NDIS_FILE_FOLDER);
  FullFileName.MaximumLength = FileName->MaximumLength + sizeof(NDIS_FILE_FOLDER);
  FullFileName.Buffer = ExAllocatePool ( NonPagedPool, FullFileName.MaximumLength );

  if ( !FullFileName.Buffer )
  {
    *Status = NDIS_STATUS_RESOURCES;
    goto cleanup;
  }

  FileHandleObject = ExAllocatePool ( NonPagedPool, sizeof(NDIS_HANDLE_OBJECT) );
  if ( !FileHandleObject )
  {
    *Status = NDIS_STATUS_RESOURCES;
    goto cleanup;
  }
  memset ( FileHandleObject, 0, sizeof(NDIS_HANDLE_OBJECT) );

  memmove ( FullFileName.Buffer, NDIS_FILE_FOLDER, FullFileName.Length );
  *Status = RtlAppendUnicodeStringToString ( &FullFileName, FileName );
  if ( !NT_SUCCESS(*Status) )
    goto cleanup;

  InitializeObjectAttributes ( &ObjectAttributes,
    &FullFileName,
    OBJ_CASE_INSENSITIVE,
    NULL,
    NULL );

  *Status = ZwCreateFile (
    &FileHandleObject->FileHandle,
    FILE_READ_DATA|SYNCHRONIZE,
    &ObjectAttributes,
    &IoStatusBlock,
    NULL, // PLARGE_INTEGER AllocationSize
    0, // ULONG FileAttributes
    FILE_SHARE_READ, // ULONG ShareAccess
    CREATE_NEW, // ULONG CreateDisposition
    FILE_SYNCHRONOUS_IO_NONALERT, // ULONG CreateOptions
    0, // PVOID EaBuffer
    0 ); // ULONG EaLength

  //if ( !NT_SUCCESS(*Status) )
  //  goto cleanup;

cleanup:
  if ( FullFileName.Buffer != NULL )
  {
    ExFreePool ( FullFileName.Buffer );
    FullFileName.Buffer = NULL;
  }
  if ( !NT_SUCCESS(*Status) && FileHandleObject != NULL )
  {
    ExFreePool ( FileHandleObject );
    FileHandleObject = NULL;
    *FileHandle = NULL;
  }
  else
    *FileHandle = NDIS_POBJECT_TO_HANDLE(FileHandleObject);

  return;
}


/*
NdisOpenGlobalConfiguration
*/

#if 0
VOID
EXPORT
NdisRegisterTdiCallBack(
    IN  TDI_REGISTER_CALLBACK   RegsterCallback)
{
    UNIMPLEMENTED
}
#endif


/*
NdisScheduleWorkItem
*/


#if 0
VOID
EXPORT
NdisSetProtocolFilter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  RECEIVE_HANDLER         ReceiveHandler,
    IN  RECEIVE_PACKET_HANDLER  ReceivePacketHandler,
    IN  NDIS_MEDIUM             Medium,
    IN  UINT                    Offset,
    IN  UINT                    Size,
    IN  PUCHAR                  Pattern)
{
    UNIMPLEMENTED
}
#endif


/*
 * @unimplemented
 */
CCHAR
EXPORT
NdisSystemProcessorCount(
    VOID)
{
	return (CCHAR)1;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisUnmapFile(
    IN  NDIS_HANDLE FileHandle)
{
    UNIMPLEMENTED
}


/*
NdisUpcaseUnicodeString
NdisUpdateSharedMemory@4
*/


/*
NdisWriteEventLogEntry
*/



/* NDIS 5.0 extensions */

/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompletePnPEvent(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNET_PNP_EVENT  NetPnPEvent)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisConvertStringToAtmAddress(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_STRING    String,
    OUT PATM_ADDRESS    AtmAddress)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisGetCurrentProcessorCounts(
    OUT PULONG  pIdleCount,
    OUT PULONG  pKernelAndUser,
    OUT PULONG  pIndex)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisGetDriverHandle(
    IN  PNDIS_HANDLE    NdisBindingHandle,
    OUT PNDIS_HANDLE    NdisDriverHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
PNDIS_PACKET
EXPORT
NdisGetReceivedPacket(
    IN  PNDIS_HANDLE    NdisBindingHandle,
    IN  PNDIS_HANDLE    MacContext)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NULL;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisGetSystemUptime(
    OUT PULONG  pSystemUpTime)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
LONG
EXPORT
NdisInterlockedDecrement(
    IN  PLONG   Addend)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  return InterlockedDecrement ( Addend );
}


/*
 * @implemented
 */
LONG
EXPORT
NdisInterlockedIncrement(
    IN  PLONG   Addend)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  return InterlockedIncrement ( Addend );
}


/*
 * @implemented
 */
PSINGLE_LIST_ENTRY 
EXPORT
NdisInterlockedPopEntrySList(
    IN  PSLIST_HEADER   ListHead,
    IN  PKSPIN_LOCK     Lock)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  return ExInterlockedPopEntrySList ( ListHead, Lock );
}


/*
 * @implemented
 */
PSINGLE_LIST_ENTRY 
EXPORT
NdisInterlockedPushEntrySList(
    IN  PSLIST_HEADER       ListHead,
    IN  PSINGLE_LIST_ENTRY  ListEntry,
    IN  PKSPIN_LOCK         Lock)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  return ExInterlockedPushEntrySList ( ListHead, ListEntry, Lock );
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMDeregisterDevice(
    IN  NDIS_HANDLE NdisDeviceHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMGetDeviceProperty(
    IN      NDIS_HANDLE         MiniportAdapterHandle,
    IN OUT  PDEVICE_OBJECT      *PhysicalDeviceObject           OPTIONAL,
    IN OUT  PDEVICE_OBJECT      *FunctionalDeviceObject         OPTIONAL,
    IN OUT  PDEVICE_OBJECT      *NextDeviceObject               OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST   *AllocatedResources             OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST   *AllocatedResourcesTranslated   OPTIONAL)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMInitializeScatterGatherDma(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  BOOLEAN     Dma64BitAddresses,
    IN  ULONG       MaximumPhysicalMapping)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMPromoteMiniport(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMQueryAdapterInstanceName(
    OUT PNDIS_STRING    AdapterInstanceName,
    IN  NDIS_HANDLE     MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterDevice(
    IN  NDIS_HANDLE         NdisWrapperHandle,
    IN  PNDIS_STRING        DeviceName,
    IN  PNDIS_STRING        SymbolicName,
    IN  PDRIVER_DISPATCH    MajorFunctions[],
    OUT PDEVICE_OBJECT      *pDeviceObject,
    OUT NDIS_HANDLE         *NdisDeviceHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMRegisterUnloadHandler(
    IN  NDIS_HANDLE     NdisWrapperHandle,
    IN  PDRIVER_UNLOAD  UnloadHandler)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMRemoveMiniport(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMSetMiniportSecondary(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_HANDLE PrimaryMiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}



/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisQueryAdapterInstanceName(
    OUT PNDIS_STRING    AdapterInstanceName,
    IN  NDIS_HANDLE     NdisBindingHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisQueryBufferSafe(
    IN  PNDIS_BUFFER    Buffer,
    OUT PVOID           *VirtualAddress OPTIONAL,
    OUT PUINT           Length,
    IN  UINT            Priority)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
ULONG
EXPORT
NdisReadPcmciaAttributeMemory(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisReleaseReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock,
    IN  PLOCK_STATE     LockState)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
ULONG
EXPORT
NdisWritePcmciaAttributeMemory(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return 0;
}


/* NDIS 5.0 extensions for intermediate drivers */

/*
 * @unimplemented
 */
VOID
EXPORT
NdisIMAssociateMiniport(
    IN  NDIS_HANDLE DriverHandle,
    IN  NDIS_HANDLE ProtocolHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS 
EXPORT
NdisIMCancelInitializeDeviceInstance(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DeviceInstance)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIMCopySendCompletePerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIMCopySendPerPacketInfo(
    IN  PNDIS_PACKET    DstPacket,
    IN  PNDIS_PACKET    SrcPacket)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisIMDeregisterLayeredMiniport(
    IN  NDIS_HANDLE DriverHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_HANDLE
EXPORT
NdisIMGetBindingContext(
    IN  NDIS_HANDLE NdisBindingHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return (NDIS_HANDLE)NULL;
}


/*
 * @unimplemented
 */
NDIS_HANDLE
EXPORT
NdisIMGetDeviceContext(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return (NDIS_HANDLE)NULL;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisIMInitializeDeviceInstanceEx(
    IN  NDIS_HANDLE     DriverHandle,
    IN  PNDIS_STRING    DriverInstance,
    IN  NDIS_HANDLE     DeviceContext   OPTIONAL)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    UNIMPLEMENTED

    return NDIS_STATUS_FAILURE;
}

/* EOF */
