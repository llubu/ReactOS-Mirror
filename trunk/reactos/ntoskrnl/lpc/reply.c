/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/reply.c
 * PURPOSE:         Communication mechanism
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
EiReplyOrRequestPort (IN	PEPORT		Port,
		      IN	PPORT_MESSAGE	LpcReply,
		      IN	ULONG		MessageType,
		      IN	PEPORT		Sender)
{
   KIRQL oldIrql;
   PQUEUEDMESSAGE MessageReply;
   ULONG Size;

   if (Port == NULL)
     {
       KEBUGCHECK(0);
     }

   Size = sizeof(QUEUEDMESSAGE);
   if (LpcReply && LpcReply->u1.s1.TotalLength > sizeof(PORT_MESSAGE))
     {
       Size += LpcReply->u1.s1.TotalLength - sizeof(PORT_MESSAGE);
     }
   MessageReply = ExAllocatePoolWithTag(NonPagedPool, Size, 
					TAG_LPC_MESSAGE);
   MessageReply->Sender = Sender;

   if (LpcReply != NULL)
     {
       memcpy(&MessageReply->Message, LpcReply, LpcReply->u1.s1.TotalLength);
     }
   else
     {
       MessageReply->Message.u1.s1.TotalLength = sizeof(PORT_MESSAGE);
       MessageReply->Message.u1.s1.DataLength = 0;
     }

   MessageReply->Message.ClientId.UniqueProcess = PsGetCurrentProcessId();
   MessageReply->Message.ClientId.UniqueThread = PsGetCurrentThreadId();
   MessageReply->Message.u2.s2.Type = MessageType;
   MessageReply->Message.MessageId = InterlockedIncrementUL(&LpcpNextMessageId);

   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   EiEnqueueMessagePort(Port, MessageReply);
   KeReleaseSpinLock(&Port->Lock, oldIrql);

   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtReplyPort (IN	HANDLE		PortHandle,
	     IN	PPORT_MESSAGE	LpcReply)
{
   NTSTATUS Status;
   PEPORT Port;

   DPRINT("NtReplyPort(PortHandle %x, LpcReply %x)\n", PortHandle, LpcReply);

   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,   /* AccessRequired */
				      LpcPortObjectType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtReplyPort() = %x\n", Status);
	return(Status);
     }

   if (EPORT_DISCONNECTED == Port->State)
     {
	ObDereferenceObject(Port);
	return STATUS_PORT_DISCONNECTED;
     }

   Status = EiReplyOrRequestPort(Port->OtherPort,
				 LpcReply,
				 LPC_REPLY,
				 Port);
   KeReleaseSemaphore(&Port->OtherPort->Semaphore, IO_NO_INCREMENT, 1, FALSE);

   ObDereferenceObject(Port);

   return(Status);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtReplyWaitReceivePortEx
 *
 * DESCRIPTION
 *	Can be used with waitable ports.
 *	Present only in w2k+.
 *
 * ARGUMENTS
 * 	PortHandle
 * 	PortId
 * 	LpcReply
 * 	LpcMessage
 * 	Timeout
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtReplyWaitReceivePortEx(IN  HANDLE		PortHandle,
			 OUT PULONG		PortId,
			 IN  PPORT_MESSAGE	LpcReply,
			 OUT PPORT_MESSAGE	LpcMessage,
			 IN  PLARGE_INTEGER	Timeout)
{
   PEPORT Port;
   KIRQL oldIrql;
   PQUEUEDMESSAGE Request;
   BOOLEAN Disconnected;
   LARGE_INTEGER to;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PreviousMode = ExGetPreviousMode();

   DPRINT("NtReplyWaitReceivePortEx(PortHandle %x, LpcReply %x, "
	  "LpcMessage %x)\n", PortHandle, LpcReply, LpcMessage);

   if (PreviousMode != KernelMode)
     {
       _SEH_TRY
         {
           ProbeForWrite(LpcMessage,
                         sizeof(PORT_MESSAGE),
                         1);
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

   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      LpcPortObjectType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("NtReplyWaitReceivePortEx() = %x\n", Status);
	return(Status);
     }
   if( Port->State == EPORT_DISCONNECTED )
     {
       /* If the port is disconnected, force the timeout to be 0
        * so we don't wait for new messages, because there won't be
        * any, only try to remove any existing messages
	*/
       Disconnected = TRUE;
       to.QuadPart = 0;
       Timeout = &to;
     }
   else Disconnected = FALSE;

   /*
    * Send the reply, only if port is connected
    */
   if (LpcReply != NULL && !Disconnected)
     {
	Status = EiReplyOrRequestPort(Port->OtherPort,
				      LpcReply,
				      LPC_REPLY,
				      Port);
	KeReleaseSemaphore(&Port->OtherPort->Semaphore, IO_NO_INCREMENT, 1,
			   FALSE);

	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Port);
	     DPRINT1("NtReplyWaitReceivePortEx() = %x\n", Status);
	     return(Status);
	  }
     }

   /*
    * Want for a message to be received
    */
   Status = KeWaitForSingleObject(&Port->Semaphore,
				  UserRequest,
				  UserMode,
				  FALSE,
				  Timeout);
   if( Status == STATUS_TIMEOUT )
     {
       /*
	* if the port is disconnected, and there are no remaining messages,
        * return STATUS_PORT_DISCONNECTED
	*/
       ObDereferenceObject(Port);
       return(Disconnected ? STATUS_PORT_DISCONNECTED : STATUS_TIMEOUT);
     }

   if (!NT_SUCCESS(Status))
     {
       if (STATUS_THREAD_IS_TERMINATING != Status)
	 {
	   DPRINT1("NtReplyWaitReceivePortEx() = %x\n", Status);
	 }
       ObDereferenceObject(Port);
       return(Status);
     }

   /*
    * Dequeue the message
    */
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   Request = EiDequeueMessagePort(Port);
   KeReleaseSpinLock(&Port->Lock, oldIrql);

   if (Request->Message.u2.s2.Type == LPC_CONNECTION_REQUEST)
     {
       PORT_MESSAGE Header;
       PEPORT_CONNECT_REQUEST_MESSAGE CRequest;

       CRequest = (PEPORT_CONNECT_REQUEST_MESSAGE)&Request->Message;
       memcpy(&Header, &Request->Message, sizeof(PORT_MESSAGE));
       Header.u1.s1.DataLength = CRequest->ConnectDataLength;
       Header.u1.s1.TotalLength = Header.u1.s1.DataLength + sizeof(PORT_MESSAGE);
       
       if (PreviousMode != KernelMode)
         {
           _SEH_TRY
             {
               ProbeForWrite((PVOID)(LpcMessage + 1),
                             CRequest->ConnectDataLength,
                             1);

               RtlCopyMemory(LpcMessage,
                             &Header,
                             sizeof(PORT_MESSAGE));
               RtlCopyMemory((PVOID)(LpcMessage + 1),
                             CRequest->ConnectData,
                             CRequest->ConnectDataLength);
             }
           _SEH_HANDLE
             {
               Status = _SEH_GetExceptionCode();
             }
           _SEH_END;
         }
       else
         {
           RtlCopyMemory(LpcMessage,
                         &Header,
                         sizeof(PORT_MESSAGE));
           RtlCopyMemory((PVOID)(LpcMessage + 1),
                         CRequest->ConnectData,
                         CRequest->ConnectDataLength);
         }
     }
   else
     {
       if (PreviousMode != KernelMode)
         {
           _SEH_TRY
             {
               ProbeForWrite(LpcMessage,
                             Request->Message.u1.s1.TotalLength,
                             1);

               RtlCopyMemory(LpcMessage,
                             &Request->Message,
                             Request->Message.u1.s1.TotalLength);
             }
           _SEH_HANDLE
             {
               Status = _SEH_GetExceptionCode();
             }
           _SEH_END;
         }
       else
         {
           RtlCopyMemory(LpcMessage,
                         &Request->Message,
                         Request->Message.u1.s1.TotalLength);
         }
     }
   if (!NT_SUCCESS(Status))
     {
       /*
	* Copying the message to the caller's buffer failed so
	* undo what we did and return.
	* FIXME: Also increment semaphore.
	*/
       KeAcquireSpinLock(&Port->Lock, &oldIrql);
       EiEnqueueMessageAtHeadPort(Port, Request);
       KeReleaseSpinLock(&Port->Lock, oldIrql);
       ObDereferenceObject(Port);
       return(Status);
     }
   if (Request->Message.u2.s2.Type == LPC_CONNECTION_REQUEST)
     {
       KeAcquireSpinLock(&Port->Lock, &oldIrql);
       EiEnqueueConnectMessagePort(Port, Request);
       KeReleaseSpinLock(&Port->Lock, oldIrql);
     }
   else
     {
       ExFreePool(Request);
     }

   /*
    * Dereference the port
    */
   ObDereferenceObject(Port);
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME						EXPORTED
 *	NtReplyWaitReceivePort
 *
 * DESCRIPTION
 *	Can be used with waitable ports.
 *
 * ARGUMENTS
 * 	PortHandle
 * 	PortId
 * 	LpcReply
 * 	LpcMessage
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtReplyWaitReceivePort (IN  HANDLE		PortHandle,
			OUT PULONG		PortId,
			IN  PPORT_MESSAGE	LpcReply,
			OUT PPORT_MESSAGE	LpcMessage)
{
  return(NtReplyWaitReceivePortEx (PortHandle,
				   PortId,
				   LpcReply,
				   LpcMessage,
				   NULL));
}

/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL
NtReplyWaitReplyPort (HANDLE		PortHandle,
		      PPORT_MESSAGE	ReplyMessage)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
LpcRequestWaitReplyPort (
	IN PEPORT		Port,
	IN PPORT_MESSAGE	LpcMessageRequest,
	OUT PPORT_MESSAGE	LpcMessageReply
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
