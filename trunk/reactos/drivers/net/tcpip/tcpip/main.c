/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/main.c
 * PURPOSE:     Driver entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <roscfg.h>
#include <tcpip.h>
#include <dispatch.h>
#include <fileobjs.h>
#include <datagram.h>
#include <loopback.h>
#include <rawip.h>
#include <udp.h>
#include <tcp.h>
#include <rosrtl/string.h>
#include <info.h>
#include <memtrack.h>

//#define NDEBUG

#ifndef NDEBUG
DWORD DebugTraceLevel = 0x7fffffff;
#else
DWORD DebugTraceLevel = 0;
#endif /* NDEBUG */

PDEVICE_OBJECT TCPDeviceObject   = NULL;
PDEVICE_OBJECT UDPDeviceObject   = NULL;
PDEVICE_OBJECT IPDeviceObject    = NULL;
PDEVICE_OBJECT RawIPDeviceObject = NULL;
NDIS_HANDLE GlobalPacketPool     = NULL;
NDIS_HANDLE GlobalBufferPool     = NULL;
KSPIN_LOCK EntityListLock;
TDIEntityInfo *EntityList        = NULL;
ULONG EntityCount                = 0;
ULONG EntityMax                  = 0;
UDP_STATISTICS UDPStats;


VOID TiWriteErrorLog(
    PDRIVER_OBJECT DriverContext,
    NTSTATUS ErrorCode,
    ULONG UniqueErrorValue,
    NTSTATUS FinalStatus,
    PWSTR String,
    ULONG DumpDataCount,
    PULONG DumpData)
/*
 * FUNCTION: Writes an error log entry
 * ARGUMENTS:
 *     DriverContext    = Pointer to the driver or device object
 *     ErrorCode        = An error code to put in the log entry
 *     UniqueErrorValue = UniqueErrorValue in the error log packet
 *     FinalStatus      = FinalStatus in the error log packet
 *     String           = If not NULL, a pointer to a string to put in log entry
 *     DumpDataCount    = Number of ULONGs of dump data
 *     DumpData         = Pointer to dump data for the log entry
 */
{
#ifdef _MSC_VER
    PIO_ERROR_LOG_PACKET LogEntry;
    UCHAR EntrySize;
    ULONG StringSize;
    PUCHAR pString;
    static WCHAR DriverName[] = L"TCP/IP";

    EntrySize = sizeof(IO_ERROR_LOG_PACKET) + 
        (DumpDataCount * sizeof(ULONG)) + sizeof(DriverName);

    if (String) {
        StringSize = (wcslen(String) * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
        EntrySize += (UCHAR)StringSize;
    }

    LogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
		DriverContext, EntrySize);

    if (LogEntry) {
        LogEntry->MajorFunctionCode = -1;
        LogEntry->RetryCount        = -1;
        LogEntry->DumpDataSize      = (USHORT)(DumpDataCount * sizeof(ULONG));
        LogEntry->NumberOfStrings   = (String == NULL) ? 1 : 2;
        LogEntry->StringOffset      = sizeof(IO_ERROR_LOG_PACKET) + (DumpDataCount-1) * sizeof(ULONG);
        LogEntry->EventCategory     = 0;
        LogEntry->ErrorCode         = ErrorCode;
        LogEntry->UniqueErrorValue  = UniqueErrorValue;
        LogEntry->FinalStatus       = FinalStatus;
        LogEntry->SequenceNumber    = -1;
        LogEntry->IoControlCode     = 0;

        if (DumpDataCount)
            RtlCopyMemory(LogEntry->DumpData, DumpData, DumpDataCount * sizeof(ULONG));

        pString = ((PUCHAR)LogEntry) + LogEntry->StringOffset;
        RtlCopyMemory(pString, DriverName, sizeof(DriverName));
        pString += sizeof(DriverName);

        if (String)
            RtlCopyMemory(pString, String, StringSize);

        IoWriteErrorLogEntry(LogEntry);
    }
#endif
}


NTSTATUS TiGetProtocolNumber(
  PUNICODE_STRING FileName,
  PULONG Protocol)
/*
 * FUNCTION: Returns the protocol number from a file name
 * ARGUMENTS:
 *     FileName = Pointer to string with file name
 *     Protocol = Pointer to buffer to put protocol number in
 * RETURNS:
 *     Status of operation
 */
{
  UNICODE_STRING us;
  NTSTATUS Status;
  ULONG Value;
  PWSTR Name;

  TI_DbgPrint(MAX_TRACE, ("Called. FileName (%wZ).\n", FileName));

  Name = FileName->Buffer;

  if (*Name++ != (WCHAR)L'\\')
    return STATUS_UNSUCCESSFUL;

  if (*Name == (WCHAR)NULL)
    return STATUS_UNSUCCESSFUL;

  RtlInitUnicodeString(&us, Name);

  Status = RtlUnicodeStringToInteger(&us, 10, &Value);
  if (!NT_SUCCESS(Status) || ((Value > 255)))
    return STATUS_UNSUCCESSFUL;

  *Protocol = Value;

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Creates a file object
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
NTSTATUS TiCreateFileObject(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
{
  PFILE_FULL_EA_INFORMATION EaInfo;
  PTRANSPORT_CONTEXT Context;
  PIO_STACK_LOCATION IrpSp;
  PTA_IP_ADDRESS Address;
  TDI_REQUEST Request;
  PVOID ClientContext;
  NTSTATUS Status;
  ULONG Protocol;

  TI_DbgPrint(DEBUG_IRP, ("Called. DeviceObject is at (0x%X), IRP is at (0x%X).\n", DeviceObject, Irp));

  EaInfo = Irp->AssociatedIrp.SystemBuffer;
CP
  /* Parameter check */
  /* No EA information means that we're opening for SET/QUERY_INFORMATION
   * style calls. */
#if 0
  if (!EaInfo) {
    TI_DbgPrint(MIN_TRACE, ("No EA information in IRP.\n"));
    return STATUS_INVALID_PARAMETER;
  }
#endif
CP
  /* Allocate resources here. We release them again if something failed */
  Context = ExAllocatePool(NonPagedPool, sizeof(TRANSPORT_CONTEXT));
  if (!Context) {
    TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
    return STATUS_INSUFFICIENT_RESOURCES;
  }
CP
  Context->RefCount   = 1;
  Context->CancelIrps = FALSE;
  KeInitializeEvent(&Context->CleanupEvent, NotificationEvent, FALSE);
CP
  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  IrpSp->FileObject->FsContext = Context;
  Request.RequestContext       = Irp;
CP
  /* Branch to the right handler */
  if (EaInfo && 
      (EaInfo->EaNameLength == TDI_TRANSPORT_ADDRESS_LENGTH) && 
      (RtlCompareMemory
       (&EaInfo->EaName, TdiTransportAddress,
	TDI_TRANSPORT_ADDRESS_LENGTH) == TDI_TRANSPORT_ADDRESS_LENGTH)) {
    /* This is a request to open an address */
CP

	/* XXX This should probably be done in IoCreateFile() */
    /* Parameter checks */

    Address = (PTA_IP_ADDRESS)(EaInfo->EaName + EaInfo->EaNameLength + 1); //0-term

    if ((EaInfo->EaValueLength < sizeof(TA_IP_ADDRESS)) ||
      (Address->TAAddressCount != 1) ||
      (Address->Address[0].AddressLength < TDI_ADDRESS_LENGTH_IP) ||
      (Address->Address[0].AddressType != TDI_ADDRESS_TYPE_IP)) {
      TI_DbgPrint(MIN_TRACE, ("Parameters are invalid:\n"));
      TI_DbgPrint(MIN_TRACE, ("AddressCount: %d\n", Address->TAAddressCount));
      if( Address->TAAddressCount == 1 ) {
	  TI_DbgPrint(MIN_TRACE, ("AddressLength: %\n", 
				  Address->Address[0].AddressLength));
	  TI_DbgPrint(MIN_TRACE, ("AddressType: %\n", 
				  Address->Address[0].AddressType));
      }
      ExFreePool(Context);
      return STATUS_INVALID_PARAMETER;
    }
CP
    /* Open address file object */


    /* Protocol depends on device object so find the protocol */
    if (DeviceObject == TCPDeviceObject)
      Protocol = IPPROTO_TCP;
    else if (DeviceObject == UDPDeviceObject)
      Protocol = IPPROTO_UDP;
    else if (DeviceObject == IPDeviceObject)
      Protocol = IPPROTO_RAW;
    else if (DeviceObject == RawIPDeviceObject) {
      Status = TiGetProtocolNumber(&IrpSp->FileObject->FileName, &Protocol);
      if (!NT_SUCCESS(Status)) {
        TI_DbgPrint(MIN_TRACE, ("Raw IP protocol number is invalid.\n"));
        ExFreePool(Context);
        return STATUS_INVALID_PARAMETER;
      }
    } else {
      TI_DbgPrint(MIN_TRACE, ("Invalid device object at (0x%X).\n", DeviceObject));
      ExFreePool(Context);
      return STATUS_INVALID_PARAMETER;
    }
CP
    Status = FileOpenAddress(&Request, Address, Protocol, NULL);
    if (NT_SUCCESS(Status)) {
      IrpSp->FileObject->FsContext2 = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
      Context->Handle.AddressHandle = Request.Handle.AddressHandle;
    }
CP
  } else if (EaInfo && 
	     (EaInfo->EaNameLength == TDI_CONNECTION_CONTEXT_LENGTH) && 
	     (RtlCompareMemory
	      (&EaInfo->EaName, TdiConnectionContext,
	       TDI_CONNECTION_CONTEXT_LENGTH) == 
	      TDI_CONNECTION_CONTEXT_LENGTH)) {
    /* This is a request to open a connection endpoint */
CP
    /* Parameter checks */

    if (EaInfo->EaValueLength < sizeof(PVOID)) {
      TI_DbgPrint(MIN_TRACE, ("Parameters are invalid.\n"));
      ExFreePool(Context);
      return STATUS_INVALID_PARAMETER;
    }

    /* Can only do connection oriented communication using TCP */

    if (DeviceObject != TCPDeviceObject) {
      TI_DbgPrint(MIN_TRACE, ("Bad device object.\n"));
      ExFreePool(Context);
      return STATUS_INVALID_PARAMETER;
    }

    ClientContext = *((PVOID*)(EaInfo->EaName + EaInfo->EaNameLength));

    /* Open connection endpoint file object */

    Status = FileOpenConnection(&Request, ClientContext);
    if (NT_SUCCESS(Status)) {
      IrpSp->FileObject->FsContext2 = (PVOID)TDI_CONNECTION_FILE;
      Context->Handle.ConnectionContext = Request.Handle.ConnectionContext;
    }
  } else {
    /* This is a request to open a control connection */
    Status = FileOpenControlChannel(&Request);
    if (NT_SUCCESS(Status)) {
      IrpSp->FileObject->FsContext2 = (PVOID)TDI_CONTROL_CHANNEL_FILE;
      Context->Handle.ControlChannel = Request.Handle.ControlChannel;
    }
  }

  if (!NT_SUCCESS(Status))
    ExFreePool(Context);

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status = (0x%X).\n", Status));

  return Status;
}


VOID TiCleanupFileObjectComplete(
  PVOID Context,
  NTSTATUS Status)
/*
 * FUNCTION: Completes an object cleanup IRP I/O request
 * ARGUMENTS:
 *     Context = Pointer to the IRP for this request
 *     Status  = Final status of the operation
 */
{
  PIRP Irp;
  PIO_STACK_LOCATION IrpSp;
  PTRANSPORT_CONTEXT TranContext;
  KIRQL OldIrql;

  Irp         = (PIRP)Context;
  IrpSp       = IoGetCurrentIrpStackLocation(Irp);
  TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;

  Irp->IoStatus.Status = Status;
  
  IoAcquireCancelSpinLock(&OldIrql);

  /* Remove the initial reference provided at object creation time */
  TranContext->RefCount--;

#ifdef DBG
  if (TranContext->RefCount != 0)
    TI_DbgPrint(DEBUG_REFCOUNT, ("TranContext->RefCount is %i, should be 0.\n", TranContext->RefCount));
#endif

  KeSetEvent(&TranContext->CleanupEvent, 0, FALSE);

  IoReleaseCancelSpinLock(OldIrql);
}


/*
 * FUNCTION: Releases resources used by a file object
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 * NOTES:
 *     This function does not pend
 */
NTSTATUS TiCleanupFileObject(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  PTRANSPORT_CONTEXT Context;
  TDI_REQUEST Request;
  NTSTATUS Status;
  KIRQL OldIrql;

  IrpSp   = IoGetCurrentIrpStackLocation(Irp);
  Context = IrpSp->FileObject->FsContext;    
  if (!Context) {
    TI_DbgPrint(MIN_TRACE, ("Parameters are invalid.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  IoAcquireCancelSpinLock(&OldIrql);

  Context->CancelIrps = TRUE;
  KeResetEvent(&Context->CleanupEvent);

  IoReleaseCancelSpinLock(OldIrql);

  Request.RequestNotifyObject = TiCleanupFileObjectComplete;
  Request.RequestContext      = Irp;

  switch ((ULONG_PTR)IrpSp->FileObject->FsContext2) {
  case TDI_TRANSPORT_ADDRESS_FILE:
    Request.Handle.AddressHandle = Context->Handle.AddressHandle;
    Status = FileCloseAddress(&Request);
    break;

  case TDI_CONNECTION_FILE:
    Request.Handle.ConnectionContext = Context->Handle.ConnectionContext;
    Status = FileCloseConnection(&Request);
    break;

  case TDI_CONTROL_CHANNEL_FILE:
    Request.Handle.ControlChannel = Context->Handle.ControlChannel;
    Status = FileCloseControlChannel(&Request);
    break;

  default:
    /* This should never happen */

    TI_DbgPrint(MIN_TRACE, ("Unknown transport context.\n"));

    IoAcquireCancelSpinLock(&OldIrql);
    Context->CancelIrps = FALSE;
    IoReleaseCancelSpinLock(OldIrql);

    return STATUS_INVALID_PARAMETER;
  }

  if (Status != STATUS_PENDING)
    TiCleanupFileObjectComplete(Irp, Status);
  
  KeWaitForSingleObject(&Context->CleanupEvent,
    UserRequest, KernelMode, FALSE, NULL);
  
  return Irp->IoStatus.Status;
}


NTSTATUS STDCALL
TiDispatchOpenClose(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
/*
 * FUNCTION: Main dispath routine
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;
  PTRANSPORT_CONTEXT Context;

  TI_DbgPrint(DEBUG_IRP, ("Called. DeviceObject is at (0x%X), IRP is at (0x%X).\n", DeviceObject, Irp));

  IoMarkIrpPending(Irp);
  Irp->IoStatus.Status      = STATUS_PENDING;
  Irp->IoStatus.Information = 0;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MajorFunction) {
  /* Open an address file, connection endpoint, or control connection */
  case IRP_MJ_CREATE:
    Status = TiCreateFileObject(DeviceObject, Irp);
    break;

  /* Close an address file, connection endpoint, or control connection */
  case IRP_MJ_CLOSE:
    Context = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;
    if (Context)
        ExFreePool(Context);
    Status = STATUS_SUCCESS;
    break;

  /* Release resources bound to an address file, connection endpoint, 
     or control connection */
  case IRP_MJ_CLEANUP:
    Status = TiCleanupFileObject(DeviceObject, Irp);
    break;

  default:
    Status = STATUS_INVALID_DEVICE_REQUEST;
  }

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status is (0x%X)\n", Status));

  return IRPFinish( Irp, Status );
}


NTSTATUS
#ifndef _MSC_VER
STDCALL
#endif
TiDispatchInternal(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Internal IOCTL dispatch routine
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
{
  NTSTATUS Status;
  BOOL Complete = TRUE;
  PIO_STACK_LOCATION IrpSp;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  TI_DbgPrint(DEBUG_IRP, ("Called. DeviceObject is at (0x%X), IRP is at (0x%X) MN (%d).\n",
    DeviceObject, Irp, IrpSp->MinorFunction));

  Irp->IoStatus.Status      = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  switch (IrpSp->MinorFunction) {
  case TDI_RECEIVE:
    Status = DispTdiReceive(Irp);
    break;

  case TDI_RECEIVE_DATAGRAM:
    Status = DispTdiReceiveDatagram(Irp);
    break;

  case TDI_SEND:
    Status = DispTdiSend(Irp);
    Complete = FALSE; /* Completed in DispTdiSend */
    break;

  case TDI_SEND_DATAGRAM:
    Status = DispTdiSendDatagram(Irp);
    break;

  case TDI_ACCEPT:
    Status = DispTdiAccept(Irp);
    break;

  case TDI_LISTEN:
    Status = DispTdiListen(Irp);
    break;

  case TDI_CONNECT:
    Status = DispTdiConnect(Irp);
    break;

  case TDI_DISCONNECT:
    Status = DispTdiDisconnect(Irp);
    break;

  case TDI_ASSOCIATE_ADDRESS:
    Status = DispTdiAssociateAddress(Irp);
    break;

  case TDI_DISASSOCIATE_ADDRESS:
    Status = DispTdiDisassociateAddress(Irp);
    break;

  case TDI_QUERY_INFORMATION:
    Status = DispTdiQueryInformation(DeviceObject, Irp);
    break;

  case TDI_SET_INFORMATION:
    Status = DispTdiSetInformation(Irp);
    break;

  case TDI_SET_EVENT_HANDLER:
    Status = DispTdiSetEventHandler(Irp);
    break;

  case TDI_ACTION:
    Status = STATUS_SUCCESS;
    break;

  /* An unsupported IOCTL code was submitted */
  default:
    Status = STATUS_INVALID_DEVICE_REQUEST;
  }

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status = (0x%X).\n", Status));

  if( Complete ) 
      IRPFinish( Irp, Status );

  return Status;
}


NTSTATUS
#ifndef _MSC_VER
STDCALL
#endif
TiDispatch(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Dispatch routine for IRP_MJ_DEVICE_CONTROL requests
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
{
  NTSTATUS Status;
  PIO_STACK_LOCATION IrpSp;

  IrpSp  = IoGetCurrentIrpStackLocation(Irp);

  TI_DbgPrint(DEBUG_IRP, ("Called. IRP is at (0x%X).\n", Irp));

  Irp->IoStatus.Information = 0;

#ifdef _MSC_VER
  Status = TdiMapUserRequest(DeviceObject, Irp, IrpSp);
  if (NT_SUCCESS(Status)) {
    TiDispatchInternal(DeviceObject, Irp);
    Status = STATUS_PENDING;
  } else {
#else
  if (TRUE) {
#endif
    /* See if this request is TCP/IP specific */
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_TCP_QUERY_INFORMATION_EX:
      TI_DbgPrint(MIN_TRACE, ("TCP_QUERY_INFORMATION_EX\n")); 
      Status = DispTdiQueryInformationEx(Irp, IrpSp);
      break;

    case IOCTL_TCP_SET_INFORMATION_EX:
      TI_DbgPrint(MIN_TRACE, ("TCP_SET_INFORMATION_EX\n")); 
      Status = DispTdiSetInformationEx(Irp, IrpSp);
      break;

    default:
      TI_DbgPrint(MIN_TRACE, ("Unknown IOCTL 0x%X\n",
          IrpSp->Parameters.DeviceIoControl.IoControlCode));
      Status = STATUS_NOT_IMPLEMENTED;
      break;
    }
  }

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status = (0x%X).\n", Status));

  return IRPFinish( Irp, Status );
}


VOID STDCALL TiUnload(
  PDRIVER_OBJECT DriverObject)
/*
 * FUNCTION: Unloads the driver
 * ARGUMENTS:
 *     DriverObject = Pointer to driver object created by the system
 */
{
#ifdef DBG
  KIRQL OldIrql;

  KeAcquireSpinLock(&AddressFileListLock, &OldIrql);
  if (!IsListEmpty(&AddressFileListHead)) {
    TI_DbgPrint(MIN_TRACE, ("Open address file objects exists.\n"));
  }
  KeReleaseSpinLock(&AddressFileListLock, OldIrql);
#endif

  /* Unregister loopback adapter */
  LoopUnregisterAdapter(NULL);

  /* Unregister protocol with NDIS */
  LANUnregisterProtocol();

  /* Shutdown transport level protocol subsystems */
  TCPShutdown();
  UDPShutdown();
  RawIPShutdown();
  DGShutdown();

  /* Shutdown network level protocol subsystem */
  IPShutdown();

  /* Free NDIS buffer descriptors */
  if (GlobalBufferPool)
    NdisFreeBufferPool(GlobalBufferPool);

  /* Free NDIS packet descriptors */
  if (GlobalPacketPool)
    NdisFreePacketPool(GlobalPacketPool);

  /* Release all device objects */

  if (TCPDeviceObject)
    IoDeleteDevice(TCPDeviceObject);

  if (UDPDeviceObject)
    IoDeleteDevice(UDPDeviceObject);

  if (RawIPDeviceObject)
    IoDeleteDevice(RawIPDeviceObject);

  if (IPDeviceObject)
    IoDeleteDevice(IPDeviceObject);

  if (EntityList)
    ExFreePool(EntityList);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS
#ifndef _MSC_VER
STDCALL
#endif
DriverEntry(
  PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Main driver entry point
 * ARGUMENTS:
 *     DriverObject = Pointer to a driver object for this driver
 *     RegistryPath = Registry node for configuration parameters
 * RETURNS:
 *     Status of driver initialization
 */
{
  NTSTATUS Status;
  UNICODE_STRING strDeviceName;
  UNICODE_STRING strNdisDeviceName;
  NDIS_STATUS NdisStatus;
  NDIS_STRING DeviceName;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));
  
  TrackingInit();
  TrackTag(NDIS_BUFFER_TAG);
  TrackTag(NDIS_PACKET_TAG);
  TrackTag(FBSD_MALLOC);
  TrackTag(EXALLOC_TAG);

  InitOskitTCP();

  /* TdiInitialize() ? */

  /* FIXME: Create symbolic links in Win32 namespace */

  /* Create IP device object */
  RtlRosInitUnicodeStringFromLiteral(&strDeviceName, DD_IP_DEVICE_NAME);
  Status = IoCreateDevice(DriverObject, 0, &strDeviceName,
    FILE_DEVICE_NETWORK, 0, FALSE, &IPDeviceObject);
  if (!NT_SUCCESS(Status)) {
    TI_DbgPrint(MIN_TRACE, ("Failed to create IP device object. Status (0x%X).\n", Status));
    return Status;
  }

  /* Create RawIP device object */
  RtlRosInitUnicodeStringFromLiteral(&strDeviceName, DD_RAWIP_DEVICE_NAME);
  Status = IoCreateDevice(DriverObject, 0, &strDeviceName,
    FILE_DEVICE_NETWORK, 0, FALSE, &RawIPDeviceObject);
  if (!NT_SUCCESS(Status)) {
    TI_DbgPrint(MIN_TRACE, ("Failed to create RawIP device object. Status (0x%X).\n", Status));
    TiUnload(DriverObject);
    return Status;
  }

  /* Create UDP device object */
  RtlRosInitUnicodeStringFromLiteral(&strDeviceName, DD_UDP_DEVICE_NAME);
  Status = IoCreateDevice(DriverObject, 0, &strDeviceName,
    FILE_DEVICE_NETWORK, 0, FALSE, &UDPDeviceObject);
  if (!NT_SUCCESS(Status)) {
    TI_DbgPrint(MIN_TRACE, ("Failed to create UDP device object. Status (0x%X).\n", Status));
    TiUnload(DriverObject);
    return Status;
  }

  /* Create TCP device object */
  RtlRosInitUnicodeStringFromLiteral(&strDeviceName, DD_TCP_DEVICE_NAME);
  Status = IoCreateDevice(DriverObject, 0, &strDeviceName,
    FILE_DEVICE_NETWORK, 0, FALSE, &TCPDeviceObject);
  if (!NT_SUCCESS(Status)) {
    TI_DbgPrint(MIN_TRACE, ("Failed to create TCP device object. Status (0x%X).\n", Status));
    TiUnload(DriverObject);
    return Status;
  }

  /* Setup network layer and transport layer entities */
  KeInitializeSpinLock(&EntityListLock);
  EntityList = ExAllocatePool(NonPagedPool, sizeof(TDIEntityID) * MAX_TDI_ENTITIES );
  if (!NT_SUCCESS(Status)) {
	  TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
    TiUnload(DriverObject);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  EntityList[0].tei_entity   = CL_NL_ENTITY;
  EntityList[0].tei_instance = 0;
  EntityList[0].context      = 0;
  EntityList[0].info_req     = InfoNetworkLayerTdiQueryEx;
  EntityList[0].info_set     = InfoNetworkLayerTdiSetEx;
  EntityList[1].tei_entity   = CL_TL_ENTITY;
  EntityList[1].tei_instance = 0;
  EntityList[1].context      = 0;
  EntityList[1].info_req     = InfoTransportLayerTdiQueryEx;
  EntityList[1].info_set     = InfoTransportLayerTdiSetEx;
  EntityCount = 2;
  EntityMax   = MAX_TDI_ENTITIES;

  /* Allocate NDIS packet descriptors */
  NdisAllocatePacketPool(&NdisStatus, &GlobalPacketPool, 100, sizeof(PACKET_CONTEXT));
  if (NdisStatus != NDIS_STATUS_SUCCESS) {
    TiUnload(DriverObject);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /* Allocate NDIS buffer descriptors */
  NdisAllocateBufferPool(&NdisStatus, &GlobalBufferPool, 100);
  if (NdisStatus != NDIS_STATUS_SUCCESS) {
    TiUnload(DriverObject);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /* Initialize address file list and protecting spin lock */
  InitializeListHead(&AddressFileListHead);
  KeInitializeSpinLock(&AddressFileListLock);

  /* Initialize connection endpoint list and protecting spin lock */
  InitializeListHead(&ConnectionEndpointListHead);
  KeInitializeSpinLock(&ConnectionEndpointListLock);

  /* Initialize interface list and protecting spin lock */
  InitializeListHead(&InterfaceListHead);
  KeInitializeSpinLock(&InterfaceListLock);

  /* Initialize network level protocol subsystem */
  IPStartup(DriverObject, RegistryPath);

  /* Initialize transport level protocol subsystems */
  DGStartup();
  RawIPStartup();
  UDPStartup();
  TCPStartup();

  /* Register protocol with NDIS */
  /* This used to be IP_DEVICE_NAME but the DDK says it has to match your entry in the SCM */
  RtlInitUnicodeString(&strNdisDeviceName, TCPIP_PROTOCOL_NAME);	
  Status = LANRegisterProtocol(&strNdisDeviceName);
  if (!NT_SUCCESS(Status)) {
	  TI_DbgPrint(MIN_TRACE,("Failed to register protocol with NDIS; status 0x%x\n", Status));
	  TiWriteErrorLog(
      DriverObject,
      EVENT_TRANSPORT_REGISTER_FAILED,
      TI_ERROR_DRIVERENTRY,
      Status,
      NULL,
      0,
      NULL);
    TiUnload(DriverObject);
    return Status;
  }

  /* Open loopback adapter */
  if (!NT_SUCCESS(LoopRegisterAdapter(NULL, NULL))) {
    TI_DbgPrint(MIN_TRACE, ("Failed to create loopback adapter. Status (0x%X).\n", Status));
    TiUnload(DriverObject);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /* Use direct I/O */
  IPDeviceObject->Flags    |= DO_DIRECT_IO;
  RawIPDeviceObject->Flags |= DO_DIRECT_IO;
  UDPDeviceObject->Flags   |= DO_DIRECT_IO;
  TCPDeviceObject->Flags   |= DO_DIRECT_IO;

  /* Initialize the driver object with this driver's entry points */
  DriverObject->MajorFunction[IRP_MJ_CREATE]  = TiDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE]   = TiDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CLEANUP] = TiDispatchOpenClose;
  DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = TiDispatchInternal;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = TiDispatch;

  DriverObject->DriverUnload = TiUnload;

  PREPARE_TESTS

  return STATUS_SUCCESS;
}


VOID
#ifndef _MSC_VER
STDCALL
#endif
IPAddInterface(
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4)
{
    UNIMPLEMENTED
}


VOID
#ifndef _MSC_VER
STDCALL
#endif
IPDelInterface(
	DWORD	Unknown0)
{
    UNIMPLEMENTED
}


VOID
#ifndef _MSC_VER
STDCALL
#endif
LookupRoute(
	DWORD	Unknown0,
	DWORD	Unknown1)
{
    UNIMPLEMENTED
}

/* EOF */
