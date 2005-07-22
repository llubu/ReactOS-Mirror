/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/connect.c
 * PURPOSE:         Communication mechanism
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 * 	EiConnectPort/12
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
EiConnectPort(IN PEPORT* ConnectedPort,
	      IN PEPORT NamedPort,
	      IN PSECTION_OBJECT Section,
	      IN LARGE_INTEGER SectionOffset,
	      IN ULONG ViewSize,
	      OUT PVOID* ClientSendViewBase,
	      OUT PVOID* ServerSendViewBase,
	      OUT PULONG ReceiveViewSize,
	      OUT PVOID* ReceiveViewBase,
	      OUT PULONG MaximumMessageSize,
	      IN OUT PVOID ConnectData,
	      IN OUT PULONG ConnectDataLength)
{
  PEPORT_CONNECT_REQUEST_MESSAGE RequestMessage;
  ULONG RequestConnectDataLength;
  PEPORT OurPort;
  PQUEUEDMESSAGE Reply;
  PEPORT_CONNECT_REPLY_MESSAGE CReply;
  NTSTATUS Status;
  KIRQL oldIrql;

  if (ConnectDataLength == NULL)
    {
      RequestConnectDataLength = 0;
    }
  else
    {
      RequestConnectDataLength = *ConnectDataLength;
    }

  /*
   * Create a port to represent our side of the connection
   */
  Status = ObCreateObject (KernelMode,
			   LpcPortObjectType,
			   NULL,
			   KernelMode,
			   NULL,
			   sizeof(EPORT),
			   0,
			   0,
			   (PVOID*)&OurPort);
  if (!NT_SUCCESS(Status))
    {
      return (Status);
    }
  LpcpInitializePort(OurPort, EPORT_TYPE_CLIENT_COMM_PORT, NamedPort);

  /*
   * Allocate a request message.
   */
  RequestMessage = ExAllocatePool(NonPagedPool,
				  sizeof(EPORT_CONNECT_REQUEST_MESSAGE) +
				  RequestConnectDataLength);
  if (RequestMessage == NULL)
    {
      ObDereferenceObject(OurPort);
      return(STATUS_NO_MEMORY);
    }

  /*
   * Initialize the request message.
   */
  RequestMessage->MessageHeader.DataSize =
    sizeof(EPORT_CONNECT_REQUEST_MESSAGE) + RequestConnectDataLength -
    sizeof(LPC_MESSAGE);
  RequestMessage->MessageHeader.MessageSize =
    sizeof(EPORT_CONNECT_REQUEST_MESSAGE) + RequestConnectDataLength;
  DPRINT("RequestMessageSize %d\n",
	 RequestMessage->MessageHeader.MessageSize);
  RequestMessage->MessageHeader.SectionSize = 0;
  RequestMessage->ConnectingProcess = PsGetCurrentProcess();
  ObReferenceObjectByPointer(RequestMessage->ConnectingProcess,
			     PROCESS_VM_OPERATION,
			     NULL,
			     KernelMode);
  RequestMessage->SendSectionObject = (struct _SECTION_OBJECT*)Section;
  RequestMessage->SendSectionOffset = SectionOffset;
  RequestMessage->SendViewSize = ViewSize;
  RequestMessage->ConnectDataLength = RequestConnectDataLength;
  if (RequestConnectDataLength > 0)
    {
      memcpy(RequestMessage->ConnectData, ConnectData,
	     RequestConnectDataLength);
    }

  /*
   * Queue the message to the named port
   */
  EiReplyOrRequestPort(NamedPort,
		       &RequestMessage->MessageHeader,
		       LPC_CONNECTION_REQUEST,
		       OurPort);
  KeReleaseSemaphore(&NamedPort->Semaphore, IO_NO_INCREMENT, 1, FALSE);
  ExFreePool(RequestMessage);

  /*
   * Wait for them to accept our connection
   */
  KeWaitForSingleObject(&OurPort->Semaphore,
			UserRequest,
			UserMode,
			FALSE,
			NULL);

  /*
   * Dequeue the response
   */
  KeAcquireSpinLock (&OurPort->Lock, &oldIrql);
  Reply = EiDequeueMessagePort (OurPort);
  KeReleaseSpinLock (&OurPort->Lock, oldIrql);
  CReply = (PEPORT_CONNECT_REPLY_MESSAGE)&Reply->Message;

  /*
   * Do some initial cleanup.
   */
  ObDereferenceObject(PsGetCurrentProcess());

  /*
   * Check for connection refusal.
   */
  if (CReply->MessageHeader.MessageType == LPC_CONNECTION_REFUSED)
    {
      ObDereferenceObject(OurPort);
      ExFreePool(Reply);
      /*
       * FIXME: Check what NT does here. Giving the user data back on
       * connect failure sounds reasonable; it probably wouldn't break
       * anything anyway.
       */
      if (ConnectDataLength != NULL)
	{
	  *ConnectDataLength = CReply->ConnectDataLength;
	  memcpy(ConnectData, CReply->ConnectData, CReply->ConnectDataLength);
	}
      return(STATUS_PORT_CONNECTION_REFUSED);
    }

  /*
   * Otherwise we are connected. Copy data back to the client.
   */
  *ServerSendViewBase = CReply->SendServerViewBase;
  *ReceiveViewSize = CReply->ReceiveClientViewSize;
  *ReceiveViewBase = CReply->ReceiveClientViewBase;
  *MaximumMessageSize = CReply->MaximumMessageSize;
  if (ConnectDataLength != NULL)
    {
      *ConnectDataLength = CReply->ConnectDataLength;
      memcpy(ConnectData, CReply->ConnectData, CReply->ConnectDataLength);
    }

  /*
   * Create our view of the send section object.
   */
  if (Section != NULL)
    {
      *ClientSendViewBase = 0;
      Status = MmMapViewOfSection(Section,
				  PsGetCurrentProcess(),
				  ClientSendViewBase,
				  0,
				  ViewSize,
				  &SectionOffset,
				  &ViewSize,
				  ViewUnmap,
				  0 /* MEM_TOP_DOWN? */,
				  PAGE_READWRITE);
      if (!NT_SUCCESS(Status))
	{
	  /* FIXME: Cleanup here. */
	  return(Status);
	}
    }

  /*
   * Do the final initialization of our port.
   */
  OurPort->State = EPORT_CONNECTED_CLIENT;

  /*
   * Cleanup.
   */
  ExFreePool(Reply);
  *ConnectedPort = OurPort;
  return(STATUS_SUCCESS);
}

/**********************************************************************
 * NAME							EXPORTED
 * 	NtConnectPort/8
 *
 * DESCRIPTION
 *	Connect to a named port and wait for the other side to
 *	accept or reject the connection request.
 *
 * ARGUMENTS
 *	ConnectedPort
 *	PortName
 *	Qos
 *	WriteMap
 *	ReadMap
 *	MaxMessageSize
 *	ConnectInfo
 *	UserConnectInfoLength
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
NTSTATUS STDCALL
NtConnectPort (PHANDLE				UnsafeConnectedPortHandle,
	       PUNICODE_STRING			PortName,
	       PSECURITY_QUALITY_OF_SERVICE	Qos,
	       PLPC_SECTION_WRITE		UnsafeWriteMap,
	       PLPC_SECTION_READ		UnsafeReadMap,
	       PULONG				UnsafeMaximumMessageSize,
	       PVOID				UnsafeConnectData,
	       PULONG				UnsafeConnectDataLength)
{
  HANDLE ConnectedPortHandle;
  LPC_SECTION_WRITE WriteMap;
  LPC_SECTION_READ ReadMap;
  ULONG MaximumMessageSize;
  PVOID ConnectData;
  ULONG ConnectDataLength;
  PSECTION_OBJECT SectionObject;
  LARGE_INTEGER SectionOffset;
  PEPORT ConnectedPort;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status = STATUS_SUCCESS;
  PEPORT NamedPort;
  
  PreviousMode = ExGetPreviousMode();
  
  if (PreviousMode != KernelMode)
    {
      _SEH_TRY
        {
          ProbeForWrite(UnsafeConnectedPortHandle,
                        sizeof(HANDLE),
                        sizeof(ULONG));
          if (UnsafeMaximumMessageSize != NULL)
            {
              ProbeForWrite(UnsafeMaximumMessageSize,
                            sizeof(ULONG),
                            sizeof(ULONG));
            }
        }
      _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
      _SEH_END;
      
      if (!NT_SUCCESS(Status))
        {
          return Status;
        }
    }

  /*
   * Copy in write map and partially validate.
   */
  if (UnsafeWriteMap != NULL)
    {
      if (PreviousMode != KernelMode)
        {
          _SEH_TRY
            {
              ProbeForWrite(UnsafeWriteMap,
                            sizeof(LPC_SECTION_WRITE),
                            1);
              RtlCopyMemory(&WriteMap,
                            UnsafeWriteMap,
                            sizeof(LPC_SECTION_WRITE));
            }
          _SEH_HANDLE
            {
              Status = _SEH_GetExceptionCode();
            }
          _SEH_END;

          if (!NT_SUCCESS(Status))
            {
              return Status;
            }
        }
      else
        {
          RtlCopyMemory(&WriteMap,
                        UnsafeWriteMap,
                        sizeof(LPC_SECTION_WRITE));
        }

      if (WriteMap.Length != sizeof(LPC_SECTION_WRITE))
	{
	  return(STATUS_INVALID_PARAMETER_4);
	}
      SectionOffset.QuadPart = WriteMap.SectionOffset;
    }
  else
    {
      WriteMap.SectionHandle = INVALID_HANDLE_VALUE;
    }

  /*
   * Handle connection data.
   */
  if (UnsafeConnectData == NULL)
    {
      ConnectDataLength = 0;
      ConnectData = NULL;
    }
  else
    {
      if (PreviousMode != KernelMode)
        {
          _SEH_TRY
            {
              ProbeForRead(UnsafeConnectDataLength,
                           sizeof(ULONG),
                           1);
              ConnectDataLength = *UnsafeConnectDataLength;
            }
          _SEH_HANDLE
            {
              Status = _SEH_GetExceptionCode();
            }
          _SEH_END;

          if (!NT_SUCCESS(Status))
            {
              return Status;
            }
        }
      else
        {
          ConnectDataLength = *UnsafeConnectDataLength;
        }

      if (ConnectDataLength != 0)
        {
          ConnectData = ExAllocatePool(NonPagedPool, ConnectDataLength);
          if (ConnectData == NULL)
            {
              return(STATUS_NO_MEMORY);
            }

          if (PreviousMode != KernelMode)
            {
              _SEH_TRY
                {
                  ProbeForWrite(UnsafeConnectData,
                                ConnectDataLength,
                                1);
                  RtlCopyMemory(ConnectData,
                                UnsafeConnectData,
                                ConnectDataLength);
                }
              _SEH_HANDLE
                {
                  Status = _SEH_GetExceptionCode();
                }
              _SEH_END;

              if (!NT_SUCCESS(Status))
                {
                  ExFreePool(ConnectData);
                  return Status;
                }
            }
          else
            {
              RtlCopyMemory(ConnectData,
                            UnsafeConnectData,
                            ConnectDataLength);
            }
        }
    }

  /*
   * Reference the named port.
   */
  Status = ObReferenceObjectByName (PortName,
                                    0,
                                    NULL,
                                    PORT_ALL_ACCESS,  /* DesiredAccess */
                                    LpcPortObjectType,
                                    UserMode,
                                    NULL,
                                    (PVOID*)&NamedPort);
  if (!NT_SUCCESS(Status))
    {
      if (KeGetPreviousMode() != KernelMode)
	{
	  ExFreePool(ConnectData);
	}
      return(Status);
    }

  /*
   * Reference the send section object.
   */
  if (WriteMap.SectionHandle != INVALID_HANDLE_VALUE)
    {
      Status = ObReferenceObjectByHandle(WriteMap.SectionHandle,
					 SECTION_MAP_READ | SECTION_MAP_WRITE,
					 MmSectionObjectType,
					 UserMode,
					 (PVOID*)&SectionObject,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject(NamedPort);
	  if (KeGetPreviousMode() != KernelMode)
	    {
	      ExFreePool(ConnectData);
	    }
	  return(Status);
	}
    }
  else
    {
      SectionObject = NULL;
    }

  /*
   * Do the connection establishment.
   */
  Status = EiConnectPort(&ConnectedPort,
			 NamedPort,
			 SectionObject,
			 SectionOffset,
			 WriteMap.ViewSize,
			 &WriteMap.ViewBase,
			 &WriteMap.TargetViewBase,
			 &ReadMap.ViewSize,
			 &ReadMap.ViewBase,
			 &MaximumMessageSize,
			 ConnectData,
			 &ConnectDataLength);
  if (!NT_SUCCESS(Status))
    {
      /* FIXME: Again, check what NT does here. */
      if (UnsafeConnectDataLength != NULL)
	{
	  if (PreviousMode != KernelMode)
	    {
              _SEH_TRY
                {
                  RtlCopyMemory(UnsafeConnectData,
                                ConnectData,
                                ConnectDataLength);
                  *UnsafeConnectDataLength = ConnectDataLength;
                }
              _SEH_HANDLE
                {
                  Status = _SEH_GetExceptionCode();
                }
              _SEH_END;
	    }
	  else
	    {
               RtlCopyMemory(UnsafeConnectData,
                             ConnectData,
                             ConnectDataLength);
               *UnsafeConnectDataLength = ConnectDataLength;
	    }

          ExFreePool(ConnectData);
	}
      return(Status);
    }

  /*
   * Do some initial cleanup.
   */
  if (SectionObject != NULL)
    {
      ObDereferenceObject(SectionObject);
      SectionObject = NULL;
    }
  ObDereferenceObject(NamedPort);
  NamedPort = NULL;

  /*
   * Copy the data back to the caller.
   */

  if (UnsafeConnectDataLength != NULL)
    {
      if (PreviousMode != KernelMode)
	{
          _SEH_TRY
            {
              *UnsafeConnectDataLength = ConnectDataLength;
              
              if (ConnectData != NULL)
                {
                  RtlCopyMemory(UnsafeConnectData,
                                ConnectData,
                                ConnectDataLength);
                }
            }
          _SEH_HANDLE
            {
              Status = _SEH_GetExceptionCode();
            }
          _SEH_END;

	  if (!NT_SUCCESS(Status))
	    {
              if (ConnectData != NULL)
                {
                  ExFreePool(ConnectData);
                }
              return(Status);
	    }
	}
      else
        {
          *UnsafeConnectDataLength = ConnectDataLength;
          
          if (ConnectData != NULL)
            {
              RtlCopyMemory(UnsafeConnectData,
                            ConnectData,
                            ConnectDataLength);
            }
        }

      if (ConnectData != NULL)
	{
	  ExFreePool(ConnectData);
	}
    }
  Status = ObInsertObject(ConnectedPort,
			  NULL,
			  PORT_ALL_ACCESS,
			  0,
			  NULL,
			  &ConnectedPortHandle);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  if (PreviousMode != KernelMode)
    {
      _SEH_TRY
        {
          *UnsafeConnectedPortHandle = ConnectedPortHandle;
          
          if (UnsafeWriteMap != NULL)
            {
              RtlCopyMemory(UnsafeWriteMap,
                            &WriteMap,
                            sizeof(LPC_SECTION_WRITE));
            }

          if (UnsafeReadMap != NULL)
            {
              RtlCopyMemory(UnsafeReadMap,
                            &ReadMap,
                            sizeof(LPC_SECTION_READ));
            }

          if (UnsafeMaximumMessageSize != NULL)
            {
              *UnsafeMaximumMessageSize = MaximumMessageSize;
            }
        }
      _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
      _SEH_END;
      
      if (!NT_SUCCESS(Status))
        {
          return Status;
        }
    }
  else
    {
      *UnsafeConnectedPortHandle = ConnectedPortHandle;
      
      if (UnsafeWriteMap != NULL)
        {
          RtlCopyMemory(UnsafeWriteMap,
                        &WriteMap,
                        sizeof(LPC_SECTION_WRITE));
        }

      if (UnsafeReadMap != NULL)
        {
          RtlCopyMemory(UnsafeReadMap,
                        &ReadMap,
                        sizeof(LPC_SECTION_READ));
        }

      if (UnsafeMaximumMessageSize != NULL)
        {
          *UnsafeMaximumMessageSize = MaximumMessageSize;
        }
    }

  /*
   * All done.
   */

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtAcceptConnectPort/6
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	ServerPortHandle
 *	NamedPortHandle
 *	LpcMessage
 *	AcceptIt
 *	WriteMap
 *	ReadMap
 *
 * RETURN VALUE
 */
/*EXPORTED*/ NTSTATUS STDCALL
NtAcceptConnectPort (PHANDLE			ServerPortHandle,
		     HANDLE			NamedPortHandle,
		     PLPC_MESSAGE		LpcMessage,
		     BOOLEAN			AcceptIt,
		     PLPC_SECTION_WRITE	WriteMap,
		     PLPC_SECTION_READ	ReadMap)
{
  NTSTATUS	Status;
  PEPORT		NamedPort;
  PEPORT		OurPort = NULL;
  PQUEUEDMESSAGE	ConnectionRequest;
  KIRQL		oldIrql;
  PEPORT_CONNECT_REQUEST_MESSAGE CRequest;
  PEPORT_CONNECT_REPLY_MESSAGE CReply;
  ULONG Size;

  Size = sizeof(EPORT_CONNECT_REPLY_MESSAGE);
  if (LpcMessage)
  {
     Size += LpcMessage->DataSize;
  }

  CReply = ExAllocatePool(NonPagedPool, Size);
  if (CReply == NULL)
    {
      return(STATUS_NO_MEMORY);
    }

  Status = ObReferenceObjectByHandle(NamedPortHandle,
				     PORT_ALL_ACCESS,
				     LpcPortObjectType,
				     UserMode,
				     (PVOID*)&NamedPort,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(CReply);
      return (Status);
    }

  /*
   * Create a port object for our side of the connection
   */
  if (AcceptIt)
    {
      Status = ObCreateObject(ExGetPreviousMode(),
			      LpcPortObjectType,
			      NULL,
			      ExGetPreviousMode(),
			      NULL,
			      sizeof(EPORT),
			      0,
			      0,
			      (PVOID*)&OurPort);
      if (!NT_SUCCESS(Status))
	{
	  ExFreePool(CReply);
	  ObDereferenceObject(NamedPort);
	  return(Status);
	}

      Status = ObInsertObject ((PVOID)OurPort,
			       NULL,
			       PORT_ALL_ACCESS,
			       0,
			       NULL,
			       ServerPortHandle);
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject(OurPort);
	  ExFreePool(CReply);
	  ObDereferenceObject(NamedPort);
	  return(Status);
	}

      LpcpInitializePort(OurPort, EPORT_TYPE_SERVER_COMM_PORT, NamedPort);
    }

  /*
   * Dequeue the connection request
   */
  KeAcquireSpinLock(&NamedPort->Lock, &oldIrql);
  ConnectionRequest = EiDequeueConnectMessagePort (NamedPort);
  KeReleaseSpinLock(&NamedPort->Lock, oldIrql);
  CRequest = (PEPORT_CONNECT_REQUEST_MESSAGE)(&ConnectionRequest->Message);

  /*
   * Prepare the reply.
   */
  if (LpcMessage != NULL)
    {
      memcpy(&CReply->MessageHeader, LpcMessage, sizeof(LPC_MESSAGE));
      memcpy(&CReply->ConnectData, (PVOID)(LpcMessage + 1),
	     LpcMessage->DataSize);
      CReply->MessageHeader.MessageSize =
	sizeof(EPORT_CONNECT_REPLY_MESSAGE) + LpcMessage->DataSize;
      CReply->MessageHeader.DataSize = CReply->MessageHeader.MessageSize -
	sizeof(LPC_MESSAGE);
      CReply->ConnectDataLength = LpcMessage->DataSize;
    }
  else
    {
      CReply->MessageHeader.MessageSize = sizeof(EPORT_CONNECT_REPLY_MESSAGE);
      CReply->MessageHeader.DataSize = sizeof(EPORT_CONNECT_REPLY_MESSAGE) -
	sizeof(LPC_MESSAGE);
      CReply->ConnectDataLength = 0;
    }
  if (!AcceptIt)
    {
      EiReplyOrRequestPort(ConnectionRequest->Sender,
			   &CReply->MessageHeader,
			   LPC_CONNECTION_REFUSED,
			   NamedPort);
      KeReleaseSemaphore(&ConnectionRequest->Sender->Semaphore,
			 IO_NO_INCREMENT,
			 1,
			 FALSE);
      ObDereferenceObject(ConnectionRequest->Sender);
      ExFreePool(ConnectionRequest);
      ExFreePool(CReply);
      ObDereferenceObject(NamedPort);
      return (STATUS_SUCCESS);
    }

  /*
   * Prepare the connection.
   */
  if (WriteMap != NULL)
    {
      PSECTION_OBJECT SectionObject;
      LARGE_INTEGER SectionOffset;

      Status = ObReferenceObjectByHandle(WriteMap->SectionHandle,
					 SECTION_MAP_READ | SECTION_MAP_WRITE,
					 MmSectionObjectType,
					 UserMode,
					 (PVOID*)&SectionObject,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}

      SectionOffset.QuadPart = WriteMap->SectionOffset;
      WriteMap->TargetViewBase = 0;
      CReply->ReceiveClientViewSize = WriteMap->ViewSize;
      Status = MmMapViewOfSection(SectionObject,
				  CRequest->ConnectingProcess,
				  &WriteMap->TargetViewBase,
				  0,
				  CReply->ReceiveClientViewSize,
				  &SectionOffset,
				  &CReply->ReceiveClientViewSize,
				  ViewUnmap,
				  0 /* MEM_TOP_DOWN? */,
				  PAGE_READWRITE);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}

      WriteMap->ViewBase = 0;
      Status = MmMapViewOfSection(SectionObject,
				  PsGetCurrentProcess(),
				  &WriteMap->ViewBase,
				  0,
				  WriteMap->ViewSize,
				  &SectionOffset,
				  &WriteMap->ViewSize,
				  ViewUnmap,
				  0 /* MEM_TOP_DOWN? */,
				  PAGE_READWRITE);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}

      ObDereferenceObject(SectionObject);
    }
  if (ReadMap != NULL && CRequest->SendSectionObject != NULL)
    {
      LARGE_INTEGER SectionOffset;

      SectionOffset = CRequest->SendSectionOffset;
      ReadMap->ViewSize = CRequest->SendViewSize;
      ReadMap->ViewBase = 0;
      Status = MmMapViewOfSection(CRequest->SendSectionObject,
				  PsGetCurrentProcess(),
				  &ReadMap->ViewBase,
				  0,
				  CRequest->SendViewSize,
				  &SectionOffset,
				  &CRequest->SendViewSize,
				  ViewUnmap,
				  0 /* MEM_TOP_DOWN? */,
				  PAGE_READWRITE);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }

  /*
   * Finish the reply.
   */
  if (ReadMap != NULL)
    {
      CReply->SendServerViewBase = ReadMap->ViewBase;
    }
  else
    {
      CReply->SendServerViewBase = 0;
    }
  if (WriteMap != NULL)
    {
      CReply->ReceiveClientViewBase = WriteMap->TargetViewBase;
    }
  CReply->MaximumMessageSize = PORT_MAX_MESSAGE_LENGTH;


  /*
   * Connect the two ports
   */
  OurPort->OtherPort = ConnectionRequest->Sender;
  OurPort->OtherPort->OtherPort = OurPort;
  EiReplyOrRequestPort(ConnectionRequest->Sender,
		       (PLPC_MESSAGE)CReply,
		       LPC_REPLY,
		       OurPort);
  ExFreePool(ConnectionRequest);
  ExFreePool(CReply);

  ObDereferenceObject(OurPort);
  ObDereferenceObject(NamedPort);

  return (STATUS_SUCCESS);
}

/**********************************************************************
 * NAME							EXPORTED
 * 	NtSecureConnectPort/9
 *
 * DESCRIPTION
 *	Connect to a named port and wait for the other side to
 *	accept the connection. Possibly verify that the server
 *	matches the ServerSid (trusted server).
 *	Present in w2k+.
 *
 * ARGUMENTS
 *	ConnectedPort
 *	PortName: fully qualified name in the Ob name space;
 *	Qos
 *	WriteMap
 *	ServerSid
 *	ReadMap
 *	MaxMessageSize
 *	ConnectInfo
 *	UserConnectInfoLength
 *
 * RETURN VALUE
 */
NTSTATUS STDCALL
NtSecureConnectPort (OUT    PHANDLE				ConnectedPort,
		     IN     PUNICODE_STRING			PortName,
		     IN     PSECURITY_QUALITY_OF_SERVICE	Qos,
		     IN OUT PLPC_SECTION_WRITE			WriteMap		OPTIONAL,
		     IN     PSID				ServerSid		OPTIONAL,
		     IN OUT PLPC_SECTION_READ			ReadMap			OPTIONAL,
		     OUT    PULONG				MaxMessageSize		OPTIONAL,
		     IN OUT PVOID				ConnectInfo		OPTIONAL,
		     IN OUT PULONG				UserConnectInfoLength	OPTIONAL)
{
  /* TODO: implement a new object type: WaitablePort */
  /* TODO: verify the process' SID that hosts the rendez-vous port equals ServerSid */
  return NtConnectPort (ConnectedPort,
		        PortName,
		        Qos,
		        WriteMap,
		        ReadMap,
		        MaxMessageSize,
		        ConnectInfo,
		        UserConnectInfoLength);
}

/* EOF */
