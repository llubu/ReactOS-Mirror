/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/reply.c
 * PURPOSE:         Local Procedure Call: Receive (Replies)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
LpcpFreeDataInfoMessage(IN PLPCP_PORT_OBJECT Port,
                        IN ULONG MessageId,
                        IN ULONG CallbackId)
{
    PLPCP_MESSAGE Message;
    PLIST_ENTRY ListHead, NextEntry;

    /* Check if the port we want is the connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) > LPCP_UNCONNECTED_PORT)
    {
        /* Use it */
        Port = Port->ConnectionPort;
    }

    /* Loop the list */
    ListHead = &Port->LpcDataInfoChainHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the message */
        Message = CONTAINING_RECORD(NextEntry, LPCP_MESSAGE, Entry);

        /* Make sure it matches */
        if ((Message->Request.MessageId == MessageId) &&
            (Message->Request.CallbackId == CallbackId))
        {
            /* Unlink and free it */
            RemoveEntryList(&Message->Entry);
            InitializeListHead(&Message->Entry);
            LpcpFreeToPortZone(Message, TRUE);
            break;
        }

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }
}

VOID
NTAPI
LpcpSaveDataInfoMessage(IN PLPCP_PORT_OBJECT Port,
                        IN PLPCP_MESSAGE Message,
                        IN ULONG LockFlags)
{
    PAGED_CODE();

    /* Acquire the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Check if the port we want is the connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) > LPCP_UNCONNECTED_PORT)
    {
        /* Use it */
        Port = Port->ConnectionPort;
    }

    /* Link the message */
    InsertTailList(&Port->LpcDataInfoChainHead, &Message->Entry);

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);
}

VOID
NTAPI
LpcpMoveMessage(IN PPORT_MESSAGE Destination,
                IN PPORT_MESSAGE Origin,
                IN PVOID Data,
                IN ULONG MessageType,
                IN PCLIENT_ID ClientId)
{
    /* Set the Message size */
    LPCTRACE((LPC_REPLY_DEBUG | LPC_SEND_DEBUG),
             "Destination/Origin: %p/%p. Data: %p. Length: %lx\n",
             Destination,
             Origin,
             Data,
             Origin->u1.Length);
    Destination->u1.Length = Origin->u1.Length;

    /* Set the Message Type */
    Destination->u2.s2.Type = !MessageType ?
                              Origin->u2.s2.Type : MessageType & 0xFFFF;

    /* Check if we have a Client ID */
    if (ClientId)
    {
        /* Set the Client ID */
        Destination->ClientId.UniqueProcess = ClientId->UniqueProcess;
        Destination->ClientId.UniqueThread = ClientId->UniqueThread;
    }
    else
    {
        /* Otherwise, copy it */
        Destination->ClientId.UniqueProcess = Origin->ClientId.UniqueProcess;
        Destination->ClientId.UniqueThread = Origin->ClientId.UniqueThread;
    }

    /* Copy the MessageId and ClientViewSize */
    Destination->MessageId = Origin->MessageId;
    Destination->ClientViewSize = Origin->ClientViewSize;

    /* Copy the Message Data */
    RtlMoveMemory(Destination + 1,
                  Data,
                  ((Destination->u1.Length & 0xFFFF) + 3) &~3);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReplyPort(IN HANDLE PortHandle,
            IN PPORT_MESSAGE LpcReply)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(IN HANDLE PortHandle,
                         OUT PVOID *PortContext OPTIONAL,
                         IN PPORT_MESSAGE ReplyMessage OPTIONAL,
                         OUT PPORT_MESSAGE ReceiveMessage,
                         IN PLARGE_INTEGER Timeout OPTIONAL)
{
    PLPCP_PORT_OBJECT Port, ReceivePort;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode(), WaitMode = PreviousMode;
    NTSTATUS Status;
    PLPCP_MESSAGE Message;
    PETHREAD Thread = PsGetCurrentThread(), WakeupThread;
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    ULONG ConnectionInfoLength;
    PAGED_CODE();
    LPCTRACE(LPC_REPLY_DEBUG,
             "Handle: %lx. Messages: %p/%p. Context: %p\n",
             PortHandle,
             ReplyMessage,
             ReceiveMessage,
             PortContext);

    /* If this is a system thread, then let it page out its stack */
    if (Thread->SystemThread) WaitMode = UserMode;

    /* Check if caller has a reply message */
    if (ReplyMessage)
    {
        /* Validate its length */
        if ((ReplyMessage->u1.s1.DataLength + sizeof(PORT_MESSAGE)) >
            ReplyMessage->u1.s1.TotalLength)
        {
            /* Fail */
            return STATUS_INVALID_PARAMETER;
        }

        /* Make sure it has a valid ID */
        if (!ReplyMessage->MessageId) return STATUS_INVALID_PARAMETER;
    }

    /* Get the Port object */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if the caller has a reply message */
    if (ReplyMessage)
    {
        /* Validate its length in respect to the port object */
        if ((ReplyMessage->u1.s1.TotalLength > Port->MaxMessageLength) ||
            (ReplyMessage->u1.s1.TotalLength <= ReplyMessage->u1.s1.DataLength))
        {
            /* Too large, fail */
            ObDereferenceObject(Port);
            return STATUS_PORT_MESSAGE_TOO_LONG;
        }
    }

    /* Check if this is anything but a client port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_CLIENT_PORT)
    {
        /* Use the connection port */
        ReceivePort = Port->ConnectionPort;
    }
    else
    {
        /* Otherwise, use the port itself */
        ReceivePort = Port;
    }

    /* Check if the caller gave a reply message */
    if (ReplyMessage)
    {
        /* Get the ETHREAD corresponding to it */
        Status = PsLookupProcessThreadByCid(&ReplyMessage->ClientId,
                                            NULL,
                                            &WakeupThread);
        if (!NT_SUCCESS(Status))
        {
            /* No thread found, fail */
            ObDereferenceObject(Port);
            return Status;
        }

        /* Allocate a message from the port zone */
        Message = LpcpAllocateFromPortZone();
        if (!Message)
        {
            /* Fail if we couldn't allocate a message */
            ObDereferenceObject(WakeupThread);
            ObDereferenceObject(Port);
            return STATUS_NO_MEMORY;
        }

        /* Keep the lock acquired */
        KeAcquireGuardedMutex(&LpcpLock);

        /* Make sure this is the reply the thread is waiting for */
        if (WakeupThread->LpcReplyMessageId != ReplyMessage->MessageId)
        {
            /* It isn't, fail */
            LpcpFreeToPortZone(Message, TRUE);
            KeReleaseGuardedMutex(&LpcpLock);
            ObDereferenceObject(WakeupThread);
            ObDereferenceObject(Port);
            return STATUS_REPLY_MESSAGE_MISMATCH;
        }

        /* Copy the message */
        LpcpMoveMessage(&Message->Request,
                        ReplyMessage,
                        ReplyMessage + 1,
                        LPC_REPLY,
                        NULL);

        /* Free any data information */
        LpcpFreeDataInfoMessage(Port,
                                ReplyMessage->MessageId,
                                ReplyMessage->CallbackId);

        /* Reference the thread while we use it */
        ObReferenceObject(WakeupThread);
        Message->RepliedToThread = WakeupThread;

        /* Set this as the reply message */
        WakeupThread->LpcReplyMessageId = 0;
        WakeupThread->LpcReplyMessage = (PVOID)Message;

        /* Check if we have messages on the reply chain */
        if (!(WakeupThread->LpcExitThreadCalled) &&
            !(IsListEmpty(&WakeupThread->LpcReplyChain)))
        {
            /* Remove us from it and reinitialize it */
            RemoveEntryList(&WakeupThread->LpcReplyChain);
            InitializeListHead(&WakeupThread->LpcReplyChain);
        }

        /* Check if this is the message the thread had received */
        if ((Thread->LpcReceivedMsgIdValid) &&
            (Thread->LpcReceivedMessageId == ReplyMessage->MessageId))
        {
            /* Clear this data */
            Thread->LpcReceivedMessageId = 0;
            Thread->LpcReceivedMsgIdValid = FALSE;
        }

        /* Release the lock and release the LPC semaphore to wake up waiters */
        KeReleaseGuardedMutex(&LpcpLock);
        LpcpCompleteWait(&WakeupThread->LpcReplySemaphore);

        /* Now we can let go of the thread */
        ObDereferenceObject(WakeupThread);
    }

    /* Now wait for someone to reply to us */
    LpcpReceiveWait(ReceivePort->MsgQueue.Semaphore, WaitMode);
    if (Status != STATUS_SUCCESS) goto Cleanup;

    /* Wait done, get the LPC lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Check if we've received nothing */
    if (IsListEmpty(&ReceivePort->MsgQueue.ReceiveHead))
    {
        /* Check if this was a waitable port and wake it */
        if (ReceivePort->Flags & LPCP_WAITABLE_PORT)
        {
            /* Reset its event */
            KeResetEvent(&ReceivePort->WaitEvent);
        }

        /* Release the lock and fail */
        KeReleaseGuardedMutex(&LpcpLock);
        ObDereferenceObject(Port);
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the message on the queue */
    Message = CONTAINING_RECORD(RemoveHeadList(&ReceivePort->
                                               MsgQueue.ReceiveHead),
                                LPCP_MESSAGE,
                                Entry);

    /* Check if the queue is empty now */
    if (IsListEmpty(&ReceivePort->MsgQueue.ReceiveHead))
    {
        /* Check if this was a waitable port */
        if (ReceivePort->Flags & LPCP_WAITABLE_PORT)
        {
            /* Reset its event */
            KeResetEvent(&ReceivePort->WaitEvent);
        }
    }

    /* Re-initialize the message's list entry */
    InitializeListHead(&Message->Entry);

    /* Set this as the received message */
    Thread->LpcReceivedMessageId = Message->Request.MessageId;
    Thread->LpcReceivedMsgIdValid = TRUE;

    /* Done touching global data, release the lock */
    KeReleaseGuardedMutex(&LpcpLock);

    /* Check if this was a connection request */
    if (LpcpGetMessageType(&Message->Request) == LPC_CONNECTION_REQUEST)
    {
        /* Get the connection message */
        ConnectMessage = (PLPCP_CONNECTION_MESSAGE)(Message + 1);
        LPCTRACE(LPC_REPLY_DEBUG,
                 "Request Messages: %p/%p\n",
                 Message,
                 ConnectMessage);

        /* Get its length */
        ConnectionInfoLength = Message->Request.u1.s1.DataLength -
                               sizeof(LPCP_CONNECTION_MESSAGE);

        /* Return it as the receive message */
        *ReceiveMessage = Message->Request;

        /* Clear our stack variable so the message doesn't get freed */
        Message = NULL;

        /* Setup the receive message */
        ReceiveMessage->u1.s1.TotalLength = sizeof(LPCP_MESSAGE) +
                                            ConnectionInfoLength;
        ReceiveMessage->u1.s1.DataLength = ConnectionInfoLength;
        RtlMoveMemory(ReceiveMessage + 1,
                      ConnectMessage + 1,
                      ConnectionInfoLength);

        /* Clear the port context if the caller requested one */
        if (PortContext) *PortContext = NULL;
    }
    else if (Message->Request.u2.s2.Type != LPC_REPLY)
    {
        /* Otherwise, this is a new message or event */
        LPCTRACE(LPC_REPLY_DEBUG,
                 "Non-Reply Messages: %p/%p\n",
                 &Message->Request,
                 (&Message->Request) + 1);

        /* Copy it */
        LpcpMoveMessage(ReceiveMessage,
                        &Message->Request,
                        (&Message->Request) + 1,
                        0,
                        NULL);

        /* Return its context */
        if (PortContext) *PortContext = Message->PortContext;

        /* And check if it has data information */
        if (Message->Request.u2.s2.DataInfoOffset)
        {
            /* It does, save it, and don't free the message below */
            LpcpSaveDataInfoMessage(Port, Message, 1);
            Message = NULL;
        }
    }
    else
    {
        /* This is a reply message, should never happen! */
        ASSERT(FALSE);
    }

    /* If we have a message pointer here, free it */
    if (Message) LpcpFreeToPortZone(Message, FALSE);

Cleanup:
    /* All done, dereference the port and return the status */
    LPCTRACE(LPC_REPLY_DEBUG,
             "Port: %p. Status: %p\n",
             Port,
             Status);
    ObDereferenceObject(Port);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReplyWaitReceivePort(IN HANDLE PortHandle,
                       OUT PVOID *PortContext OPTIONAL,
                       IN PPORT_MESSAGE ReplyMessage OPTIONAL,
                       OUT PPORT_MESSAGE ReceiveMessage)
{
    /* Call the newer API */
    return NtReplyWaitReceivePortEx(PortHandle,
                                    PortContext,
                                    ReplyMessage,
                                    ReceiveMessage,
                                    NULL);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReplyWaitReplyPort(IN HANDLE PortHandle,
                     IN PPORT_MESSAGE ReplyMessage)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReadRequestData(IN HANDLE PortHandle,
                  IN PPORT_MESSAGE Message,
                  IN ULONG Index,
                  IN PVOID Buffer,
                  IN ULONG BufferLength,
                  OUT PULONG Returnlength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtWriteRequestData(IN HANDLE PortHandle,
                   IN PPORT_MESSAGE Message,
                   IN ULONG Index,
                   IN PVOID Buffer,
                   IN ULONG BufferLength,
                   OUT PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
