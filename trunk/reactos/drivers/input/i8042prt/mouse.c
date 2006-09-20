/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/mouse.c
 * PURPOSE:     Mouse specific functions
 * PROGRAMMERS: Copyright Victor Kirhenshtein (sauros@iname.com)
                Copyright Jason Filby (jasonfilby@yahoo.com)
                Copyright Martijn Vernooij (o112w8r02@sneakemail.com)
                Copyright 2006 Herv� Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ****************************************************************/

#include "i8042prt.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * These functions are callbacks for filter driver custom interrupt
 * service routines.
 */
static VOID NTAPI
i8042MouIsrWritePort(
	IN PVOID Context,
	IN UCHAR Value)
{
	PI8042_MOUSE_EXTENSION DeviceExtension;

	DeviceExtension = (PI8042_MOUSE_EXTENSION)Context;

	if (DeviceExtension->MouseHook.IsrWritePort)
	{
		DeviceExtension->MouseHook.IsrWritePort(
			DeviceExtension->MouseHook.CallContext,
			Value);
	}
	else
		i8042IsrWritePort(Context, Value, 0xD4);
}

VOID NTAPI
i8042MouQueuePacket(
	IN PVOID Context)
{
	PI8042_MOUSE_EXTENSION DeviceExtension;

	DeviceExtension = (PI8042_MOUSE_EXTENSION)Context;

	if (DeviceExtension->MouseHook.QueueMousePacket)
	{
		DeviceExtension->MouseHook.QueueMousePacket(
			DeviceExtension->MouseHook.CallContext);
		return;
	}

	DeviceExtension->MouseComplete = TRUE;
	DeviceExtension->MouseInBuffer++;
	if (DeviceExtension->MouseInBuffer > DeviceExtension->Common.PortDeviceExtension->Settings.MouseDataQueueSize)
	{
		DPRINT1("Mouse buffer overflow\n");
		DeviceExtension->MouseInBuffer--;
	}

	DPRINT("Irq completes mouse packet\n");
	KeInsertQueueDpc(&DeviceExtension->DpcMouse, NULL, NULL);
}

VOID
i8042MouHandle(
	IN PI8042_MOUSE_EXTENSION DeviceExtension,
	IN UCHAR Output)
{
	PMOUSE_INPUT_DATA MouseInput;
	CHAR Scroll;

	MouseInput = DeviceExtension->MouseBuffer + DeviceExtension->MouseInBuffer;

	switch (DeviceExtension->MouseState)
	{
		case MouseIdle:
			/* This bit should be 1, if not drop the packet, we
			 * might be lucky and get in sync again
			 */
			if (!(Output & 8)) {
				DPRINT1("Bad input, dropping..\n");
				return;
			}

			MouseInput->Buttons = 0;
			MouseInput->RawButtons = 0;
			MouseInput->Flags = MOUSE_MOVE_RELATIVE;

			/* Note how we ignore the overflow bits, like Windows
			 * is said to do. There's no reasonable thing to do
			 * anyway.
			 */

			if (Output & 16)
				MouseInput->LastX = 1;
			else
				MouseInput->LastX = 0;
			if (Output & 32)
				MouseInput->LastY = 1;
			else
				MouseInput->LastY = 0;

			if (Output & 1)
				MouseInput->RawButtons |= MOUSE_LEFT_BUTTON_DOWN;
			if (Output & 2)
				MouseInput->RawButtons |= MOUSE_RIGHT_BUTTON_DOWN;
			if (Output & 4)
				MouseInput->RawButtons |= MOUSE_MIDDLE_BUTTON_DOWN;

			DeviceExtension->MouseState = XMovement;
			break;

		case XMovement:
			if (MouseInput->LastX)
				MouseInput->LastX = (LONG) Output - 256;
			else
				MouseInput->LastX = Output;

			DeviceExtension->MouseState = YMovement;
			break;

		case YMovement:
			if (MouseInput->LastY)
				MouseInput->LastY = (LONG)Output - 256;
			else
				MouseInput->LastY = (LONG)Output;

			/* Windows wants it the other way around */
			MouseInput->LastY = -MouseInput->LastY;

			if (DeviceExtension->MouseType == GenericPS2 ||
			    DeviceExtension->MouseType == Ps2pp)
			{
				i8042MouHandleButtons(
					DeviceExtension,
					MOUSE_LEFT_BUTTON_DOWN |
					MOUSE_RIGHT_BUTTON_DOWN |
					MOUSE_MIDDLE_BUTTON_DOWN);
				i8042MouQueuePacket(DeviceExtension);
				DeviceExtension->MouseState = MouseIdle;
			}
			else
			{
				DeviceExtension->MouseState = ZMovement;
			}
			break;

		case ZMovement:
			Scroll = Output & 0x0f;
			if (Scroll & 8)
				Scroll |= 0xf0;

			if (Scroll)
			{
				MouseInput->RawButtons |= MOUSE_WHEEL;
				MouseInput->ButtonData = (USHORT)(Scroll * -WHEEL_DELTA);
			}

			if (DeviceExtension->MouseType == IntellimouseExplorer)
			{
				if (Output & 16)
					MouseInput->RawButtons |= MOUSE_BUTTON_4_DOWN;
				if (Output & 32)
					MouseInput->RawButtons |= MOUSE_BUTTON_5_DOWN;
			}
			i8042MouHandleButtons(
				DeviceExtension,
				MOUSE_LEFT_BUTTON_DOWN |
				MOUSE_RIGHT_BUTTON_DOWN |
				MOUSE_MIDDLE_BUTTON_DOWN |
				MOUSE_BUTTON_4_DOWN |
				MOUSE_BUTTON_5_DOWN);
			i8042MouQueuePacket(DeviceExtension);
			DeviceExtension->MouseState = MouseIdle;
			break;

		default:
			DPRINT1("Unexpected state 0x%u!\n", DeviceExtension->MouseState);
			ASSERT(FALSE);
	}
}

/*
 * Updates ButtonFlags according to RawButtons and a saved state;
 * Only takes in account the bits that are set in Mask
 */
VOID
i8042MouHandleButtons(
	IN PI8042_MOUSE_EXTENSION DeviceExtension,
	IN USHORT Mask)
{
	PMOUSE_INPUT_DATA MouseInput;
	USHORT NewButtonData;
	USHORT ButtonDiff;

	MouseInput = DeviceExtension->MouseBuffer + DeviceExtension->MouseInBuffer;
	NewButtonData = (USHORT)(MouseInput->RawButtons & Mask);
	ButtonDiff = (NewButtonData ^ DeviceExtension->MouseButtonState) & Mask;

	/* Note that the defines are such:
	 * MOUSE_LEFT_BUTTON_DOWN 1
	 * MOUSE_LEFT_BUTTON_UP   2
	 */
	MouseInput->ButtonFlags |= (NewButtonData & ButtonDiff) |
		(((~(NewButtonData)) << 1) & (ButtonDiff << 1)) |
		(MouseInput->RawButtons & 0xfc00);

	DPRINT("Left raw/up/down: %u/%u/%u\n",
		MouseInput->RawButtons & MOUSE_LEFT_BUTTON_DOWN,
		MouseInput->ButtonFlags & MOUSE_LEFT_BUTTON_DOWN,
		MouseInput->ButtonFlags & MOUSE_LEFT_BUTTON_UP);

	DeviceExtension->MouseButtonState =
		(DeviceExtension->MouseButtonState & ~Mask) | (NewButtonData & Mask);
}

static NTSTATUS OldInitialization(PPORT_DEVICE_EXTENSION); /* FIXME */

/* Does lastest initializations for the mouse. This method
 * is called just before connecting the interrupt.
 */
NTSTATUS
i8042MouInitialize(
	IN PI8042_MOUSE_EXTENSION DeviceExtension)
{
	PPORT_DEVICE_EXTENSION PortDeviceExtension;

	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;

/* FIXME */ OldInitialization(PortDeviceExtension);

	return STATUS_SUCCESS;
}

static VOID NTAPI
i8042MouDpcRoutine(
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2)
{
	PI8042_MOUSE_EXTENSION DeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	ULONG MouseTransferred = 0;
	ULONG MouseInBufferCopy;
	KIRQL Irql;
	LARGE_INTEGER Timeout;

	DeviceExtension = (PI8042_MOUSE_EXTENSION)DeferredContext;
	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;

	switch (DeviceExtension->MouseTimeoutState)
	{
		case TimeoutStart:
		{
			DeviceExtension->MouseTimeoutState = NoChange;
			if (DeviceExtension->MouseTimeoutActive &&
			    !KeCancelTimer(&DeviceExtension->TimerMouseTimeout))
			{
				/* The timer fired already, give up */
				DeviceExtension->MouseTimeoutActive = FALSE;
				return;
			}

			Timeout.QuadPart = -15000000; /* 1.5 seconds, should be enough */

			KeSetTimer(
				&DeviceExtension->TimerMouseTimeout,
				Timeout,
				&DeviceExtension->DpcMouseTimeout);
			DeviceExtension->MouseTimeoutActive = TRUE;
			return;
		}

		case TimeoutCancel:
		{
			DeviceExtension->MouseTimeoutState = NoChange;
			KeCancelTimer(&DeviceExtension->TimerMouseTimeout);
			DeviceExtension->MouseTimeoutActive = FALSE;
		}

		default:
			;/* nothing, don't want a warning */
	}

	/* Should be unlikely */
	if (!DeviceExtension->MouseComplete)
		return;

	Irql = KeAcquireInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt);

	DeviceExtension->MouseComplete = FALSE;
	MouseInBufferCopy = DeviceExtension->MouseInBuffer;

	KeReleaseInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt, Irql);

	DPRINT("Send a mouse packet\n");

	if (!DeviceExtension->MouseData.ClassService)
		return;

	DPRINT("Sending %lu mouse move(s)\n", MouseInBufferCopy);
	(*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->MouseData.ClassService)(
		DeviceExtension->MouseData.ClassDeviceObject,
		DeviceExtension->MouseBuffer,
		DeviceExtension->MouseBuffer + MouseInBufferCopy,
		&MouseTransferred);

	Irql = KeAcquireInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt);
	DeviceExtension->MouseInBuffer -= MouseTransferred;
	if (DeviceExtension->MouseInBuffer)
		RtlMoveMemory(
			DeviceExtension->MouseBuffer,
			DeviceExtension->MouseBuffer + MouseTransferred,
			DeviceExtension->MouseInBuffer * sizeof(MOUSE_INPUT_DATA));
	KeReleaseInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt, Irql);
}

/* This timer DPC will be called when the mouse reset times out.
 * I'll just send the 'disable mouse port' command to the controller
 * and say the mouse doesn't exist.
 */
static VOID NTAPI
i8042DpcRoutineMouseTimeout(
	IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2)
{
	PI8042_MOUSE_EXTENSION DeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	KIRQL Irql;

	DeviceExtension = (PI8042_MOUSE_EXTENSION)DeferredContext;
	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;

	Irql = KeAcquireInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt);

	DPRINT1("Mouse initialization timeout! (substate %x). Disabling mouse.\n",
		DeviceExtension->MouseResetState);

	i8042Flush(PortDeviceExtension);
	i8042ChangeMode(PortDeviceExtension, CCB_MOUSE_INT_ENAB, CCB_MOUSE_DISAB);
	i8042Flush(PortDeviceExtension);

	PortDeviceExtension->Flags &= ~MOUSE_PRESENT;

	KeReleaseInterruptSpinLock(PortDeviceExtension->HighestDIRQLInterrupt, Irql);
}

/*
 * Runs the mouse IOCTL_INTERNAL dispatch.
 */
NTSTATUS
i8042MouInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PI8042_MOUSE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	Irp->IoStatus.Information = 0;
	DeviceExtension = (PI8042_MOUSE_EXTENSION)DeviceObject->DeviceExtension;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_INTERNAL_MOUSE_CONNECT:
		{
			SIZE_T Size;
			PIO_WORKITEM WorkItem = NULL;
			PI8042_HOOK_WORKITEM WorkItemData = NULL;

			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_MOUSE_CONNECT\n");
			if (Stack->Parameters.DeviceIoControl.InputBufferLength != sizeof(CONNECT_DATA))
			{
				Status = STATUS_INVALID_PARAMETER;
				goto cleanup;
			}

			DeviceExtension->MouseData =
				*((PCONNECT_DATA)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

			/* Send IOCTL_INTERNAL_I8042_HOOK_MOUSE to device stack */
			WorkItem = IoAllocateWorkItem(DeviceObject);
			if (!WorkItem)
			{
				DPRINT("IoAllocateWorkItem() failed\n");
				Status = STATUS_INSUFFICIENT_RESOURCES;
				goto cleanup;
			}
			WorkItemData = ExAllocatePool(
				NonPagedPool,
				sizeof(I8042_HOOK_WORKITEM));
			if (!WorkItemData)
			{
				DPRINT("ExAllocatePool() failed\n");
				Status = STATUS_INSUFFICIENT_RESOURCES;
				goto cleanup;
			}
			WorkItemData->WorkItem = WorkItem;
			WorkItemData->Irp = Irp;

			/* Initialize extension */
			DeviceExtension->Common.Type = Mouse;
			Size = DeviceExtension->Common.PortDeviceExtension->Settings.MouseDataQueueSize * sizeof(MOUSE_INPUT_DATA);
			DeviceExtension->MouseBuffer = ExAllocatePool(
				NonPagedPool,
				Size);
			if (!DeviceExtension->MouseBuffer)
			{
				DPRINT("ExAllocatePool() failed\n");
				Status = STATUS_INSUFFICIENT_RESOURCES;
				goto cleanup;
			}
			RtlZeroMemory(DeviceExtension->MouseBuffer, Size);
			DeviceExtension->MouseAttributes.InputDataQueueLength =
				DeviceExtension->Common.PortDeviceExtension->Settings.MouseDataQueueSize;
			KeInitializeDpc(
				&DeviceExtension->DpcMouse,
				i8042MouDpcRoutine,
				DeviceExtension);
			KeInitializeDpc(
				&DeviceExtension->DpcMouseTimeout,
				i8042DpcRoutineMouseTimeout,
				DeviceExtension);
			KeInitializeTimer(&DeviceExtension->TimerMouseTimeout);
			DeviceExtension->Common.PortDeviceExtension->MouseExtension = DeviceExtension;
			DeviceExtension->Common.PortDeviceExtension->Flags |= MOUSE_CONNECTED;

			IoMarkIrpPending(Irp);
			IoQueueWorkItem(WorkItem,
				i8042SendHookWorkItem,
				DelayedWorkQueue,
				WorkItemData);
			Status = STATUS_PENDING;
			break;

cleanup:
			if (DeviceExtension->MouseBuffer)
				ExFreePool(DeviceExtension->MouseBuffer);
			if (WorkItem)
				IoFreeWorkItem(WorkItem);
			if (WorkItemData)
				ExFreePool(WorkItemData);
			break;
		}
		case IOCTL_INTERNAL_MOUSE_DISCONNECT:
		{
			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_MOUSE_DISCONNECT\n");
			/* MSDN says that operation is to implemented.
			 * To implement it, we just have to do:
			 * DeviceExtension->MouseData.ClassService = NULL;
			 */
			Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
		case IOCTL_INTERNAL_I8042_HOOK_MOUSE:
		{
			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_I8042_HOOK_MOUSE\n");
			/* Nothing to do here */
			Status = STATUS_SUCCESS;
			break;
		}
		default:
		{
			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / unknown ioctl code 0x%lx\n",
				Stack->Parameters.DeviceIoControl.IoControlCode);
			ASSERT(FALSE);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	Irp->IoStatus.Status = Status;
	if (Status != STATUS_PENDING)
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

/* Test if packets are taking too long to come in. If they do, we
 * might have gotten out of sync and should just drop what we have.
 *
 * If we want to be totally right, we'd also have to keep a count of
 * errors, and totally reset the mouse after too much of them (can
 * happen if the user is using a KVM switch and an OS on another port
 * resets the mouse, or if the user hotplugs the mouse, or if we're just
 * generally unlucky). Also note the input parsing routine where we
 * drop invalid input packets.
 */
static VOID
i8042MouInputTestTimeout(
	IN PI8042_MOUSE_EXTENSION DeviceExtension)
{
	ULARGE_INTEGER Now;

	if (DeviceExtension->MouseState == MouseExpectingACK ||
	    DeviceExtension->MouseState == MouseResetting)
		return;

	Now.QuadPart = KeQueryInterruptTime();

	if (DeviceExtension->MouseState != MouseIdle) {
		/* Check if the last byte came too long ago */
		if (Now.QuadPart - DeviceExtension->MousePacketStartTime.QuadPart >
		    DeviceExtension->Common.PortDeviceExtension->Settings.MouseSynchIn100ns)
		{
			DPRINT("Mouse input packet timeout\n");
			DeviceExtension->MouseState = MouseIdle;
		}
	}

	if (DeviceExtension->MouseState == MouseIdle)
		DeviceExtension->MousePacketStartTime.QuadPart = Now.QuadPart;
}

/*
 * Call the customization hook. The ToReturn parameter is about wether
 * we should go on with the interrupt. The return value is what
 * we should return (indicating to the system wether someone else
 * should try to handle the interrupt)
 */
static BOOLEAN
i8042MouCallIsrHook(
	IN PI8042_MOUSE_EXTENSION DeviceExtension,
	IN UCHAR Status,
	IN UCHAR Input,
	OUT PBOOLEAN ToReturn)
{
	BOOLEAN HookReturn, HookContinue;

	HookContinue = FALSE;

	if (DeviceExtension->MouseHook.IsrRoutine)
	{
		DPRINT1("IsrRoutine(%p %p %p 0x%x 0x%x %p %lu %lu)\n",
			DeviceExtension->MouseHook.Context, DeviceExtension->MouseBuffer + DeviceExtension->MouseInBuffer,
			&DeviceExtension->Common.PortDeviceExtension->Packet, Status, &Input, &HookContinue,
			DeviceExtension->MouseState, DeviceExtension->MouseResetState);
		HookReturn = DeviceExtension->MouseHook.IsrRoutine(
			DeviceExtension->MouseHook.Context,
			DeviceExtension->MouseBuffer + DeviceExtension->MouseInBuffer,
			&DeviceExtension->Common.PortDeviceExtension->Packet,
			Status,
			&Input,
			&HookContinue,
			&DeviceExtension->MouseState,
			&DeviceExtension->MouseResetState);
		DPRINT1("IsrRoutine(%p %p %p 0x%x 0x%x %p %lu %lu)\n",
			DeviceExtension->MouseHook.Context, DeviceExtension->MouseBuffer + DeviceExtension->MouseInBuffer,
			&DeviceExtension->Common.PortDeviceExtension->Packet, Status, &Input, &HookContinue,
			DeviceExtension->MouseState, DeviceExtension->MouseResetState);

		if (!HookContinue)
		{
			*ToReturn = HookReturn;
			return TRUE;
		}
	}
	return FALSE;
}

static BOOLEAN
i8042MouResetIsr(
	IN PI8042_MOUSE_EXTENSION DeviceExtension,
	IN UCHAR Status,
	IN UCHAR Value)
{
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	BOOLEAN ToReturn = FALSE;

	if (i8042MouCallIsrHook(DeviceExtension, Status, Value, &ToReturn))
		return ToReturn;

	if (MouseResetting != DeviceExtension->MouseState)
		return FALSE;
	DeviceExtension->MouseTimeoutState = TimeoutStart;
	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;

	switch (DeviceExtension->MouseResetState)
	{
		case 1100: /* the first ack, drop it. */
			DeviceExtension->MouseResetState = ExpectingReset;
			return TRUE;
		case ExpectingReset:
			/* First, 0xFF is sent. The mouse is supposed to say AA00 if ok, FC00 if not. */
			if (0xAA == Value)
			{
				DeviceExtension->MouseResetState++;
			}
			else
			{
				PortDeviceExtension->Flags &= ~MOUSE_PRESENT;
				DeviceExtension->MouseState = MouseIdle;
				DPRINT("Mouse returned bad reset reply: %x (expected aa)\n", Value);
			}
			return TRUE;
		case ExpectingResetId:
			if (0x00 == Value)
			{
				DeviceExtension->MouseResetState++;
				DeviceExtension->MouseType = GenericPS2;
				i8042MouIsrWritePort(PortDeviceExtension, 0xF2);
			}
			else
			{
				PortDeviceExtension->Flags &= ~MOUSE_PRESENT;
				DeviceExtension->MouseState = MouseIdle;
				DPRINT1("Mouse returned bad reset reply part two: %x (expected 0)\n", Value);
			}
			return TRUE;
		case ExpectingGetDeviceIdACK:
			if (MOUSE_ACK == Value)
			{
				DeviceExtension->MouseResetState++;
			}
			else if (MOUSE_NACK == Value || MOUSE_ERROR == Value)
			{
				DeviceExtension->MouseResetState++;
				/* Act as if 00 (normal mouse) was received */
				DPRINT("Mouse doesn't support 0xd2, (returns %x, expected %x), faking\n", Value, MOUSE_ACK);
				i8042MouResetIsr(DeviceExtension, Status, 0);
			}
			return TRUE;
		case ExpectingGetDeviceIdValue:
			switch (Value)
			{
				case 0x02:
					DeviceExtension->MouseAttributes.MouseIdentifier =
						BALLPOINT_I8042_HARDWARE;
					break;
				case 0x03:
				case 0x04:
					DeviceExtension->MouseAttributes.MouseIdentifier =
						WHEELMOUSE_I8042_HARDWARE;
					break;
				default:
					DeviceExtension->MouseAttributes.MouseIdentifier =
						MOUSE_I8042_HARDWARE;
			}
			DeviceExtension->MouseResetState++;
			i8042MouIsrWritePort(PortDeviceExtension, 0xE8);
			return TRUE;
		case ExpectingSetResolutionDefaultACK:
			DeviceExtension->MouseResetState++;
			i8042MouIsrWritePort(PortDeviceExtension, 0x00);
			return TRUE;
		case ExpectingSetResolutionDefaultValueACK:
			DeviceExtension->MouseResetState = ExpectingSetScaling1to1ACK;
			i8042MouIsrWritePort(PortDeviceExtension, 0xE6);
			return TRUE;
		case ExpectingSetScaling1to1ACK:
		case ExpectingSetScaling1to1ACK2:
			DeviceExtension->MouseResetState++;
			i8042MouIsrWritePort(PortDeviceExtension, 0xE6);
			return TRUE;
		case ExpectingSetScaling1to1ACK3:
			DeviceExtension->MouseResetState++;
			i8042MouIsrWritePort(PortDeviceExtension, 0xE9);
			return TRUE;
		case ExpectingReadMouseStatusACK:
			DeviceExtension->MouseResetState++;
			return TRUE;
		case ExpectingReadMouseStatusByte1:
			DeviceExtension->MouseLogiBuffer[0] = Value;
			DeviceExtension->MouseResetState++;
			return TRUE;
		case ExpectingReadMouseStatusByte2:
			DeviceExtension->MouseLogiBuffer[1] = Value;
			DeviceExtension->MouseResetState++;
			return TRUE;
		case ExpectingReadMouseStatusByte3:
			DeviceExtension->MouseLogiBuffer[2] = Value;
			/* Now MouseLogiBuffer is a set of info. If the second
			 * byte is 0, the mouse didn't understand the magic
			 * code. Otherwise, it it a Logitech and the second byte
			 * is the number of buttons, bit 7 of the first byte tells
			 * if it understands special E7 commands, the rest is an ID.
			 */
			if (DeviceExtension->MouseLogiBuffer[1])
			{
				DeviceExtension->MouseAttributes.NumberOfButtons =
					DeviceExtension->MouseLogiBuffer[1];
				DeviceExtension->MouseType = Ps2pp;
				i8042MouIsrWritePort(PortDeviceExtension, 0xF3);
				DeviceExtension->MouseResetState = ExpectingSetSamplingRateACK;
				/* TODO: Go through EnableWheel and Enable5Buttons */
				return TRUE;
			}
			DeviceExtension->MouseResetState = EnableWheel;
			i8042MouResetIsr(DeviceExtension, Status, Value);
			return TRUE;
		case EnableWheel:
			i8042MouIsrWritePort(PortDeviceExtension, 0xF3);
			DeviceExtension->MouseResetState = 1001;
			return TRUE;
		case 1001:
			i8042MouIsrWritePort(PortDeviceExtension, 0xC8);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1002:
		case 1004:
			i8042MouIsrWritePort(PortDeviceExtension, 0xF3);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1003:
			i8042MouIsrWritePort(PortDeviceExtension, 0x64);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1005:
			i8042MouIsrWritePort(PortDeviceExtension, 0x50);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1006:
			i8042MouIsrWritePort(PortDeviceExtension, 0xF2);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1007:
			/* Ignore ACK */
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1008:
			if (0x03 == Value) {
				/* It's either an Intellimouse or Intellimouse Explorer. */
				DeviceExtension->MouseAttributes.NumberOfButtons = 3;
				DeviceExtension->MouseAttributes.MouseIdentifier =
					WHEELMOUSE_I8042_HARDWARE;
				DeviceExtension->MouseType = Intellimouse;
				DeviceExtension->MouseResetState = Enable5Buttons;
				i8042MouResetIsr(DeviceExtension, Status, Value);
			}
			else
			{
				/* Just set the default settings and be done */
				i8042MouIsrWritePort(PortDeviceExtension, 0xF3);
				DeviceExtension->MouseResetState = ExpectingSetSamplingRateACK;
			}
			return TRUE;
		case Enable5Buttons:
			i8042MouIsrWritePort(PortDeviceExtension, 0xF3);
			DeviceExtension->MouseResetState = 1021;
			return TRUE;
		case 1022:
		case 1024:
			i8042MouIsrWritePort(PortDeviceExtension, 0xF3);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1021:
		case 1023:
			i8042MouIsrWritePort(PortDeviceExtension, 0xC8);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1025:
			i8042MouIsrWritePort(PortDeviceExtension, 0x50);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1026:
			i8042MouIsrWritePort(PortDeviceExtension, 0xF2);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case 1027:
			if (0x04 == Value)
			{
				DeviceExtension->MouseAttributes.NumberOfButtons = 5;
				DeviceExtension->MouseAttributes.MouseIdentifier =
					WHEELMOUSE_I8042_HARDWARE;
				DeviceExtension->MouseType = IntellimouseExplorer;
			}
			i8042MouIsrWritePort(PortDeviceExtension, 0xF3);
			DeviceExtension->MouseResetState = ExpectingSetSamplingRateACK;
			return TRUE;
		case ExpectingSetSamplingRateACK:
			i8042MouIsrWritePort(PortDeviceExtension,
				(UCHAR)DeviceExtension->MouseAttributes.SampleRate);
			DeviceExtension->MouseResetState++;
			return TRUE;
		case ExpectingSetSamplingRateValueACK:
			if (MOUSE_NACK == Value)
			{
				i8042MouIsrWritePort(PortDeviceExtension, 0x3C);
				DeviceExtension->MouseAttributes.SampleRate = PortDeviceExtension->Settings.SampleRate;
				DeviceExtension->MouseResetState = 1040;
				return TRUE;
			}
		case 1040:  /* Fallthrough */
			i8042MouIsrWritePort(PortDeviceExtension, 0xE8);
			DeviceExtension->MouseResetState = ExpectingFinalResolutionACK;
			return TRUE;
		case ExpectingFinalResolutionACK:
			i8042MouIsrWritePort(PortDeviceExtension,
				(UCHAR)(PortDeviceExtension->Settings.MouseResolution & 0xff));
			DPRINT("Mouse resolution %lu\n",
				PortDeviceExtension->Settings.MouseResolution);
			DeviceExtension->MouseResetState = ExpectingFinalResolutionValueACK;
			return TRUE;
		case ExpectingFinalResolutionValueACK:
			i8042MouIsrWritePort(PortDeviceExtension, 0xF4);
			DeviceExtension->MouseResetState = ExpectingEnableACK;
			return TRUE;
		case ExpectingEnableACK:
			DeviceExtension->MouseState = MouseIdle;
			DeviceExtension->MouseTimeoutState = TimeoutCancel;
			DPRINT("Mouse type = %u\n", DeviceExtension->MouseType);
			return TRUE;
		default:
			if (DeviceExtension->MouseResetState < 100 || DeviceExtension->MouseResetState > 999)
				DPRINT1("MouseResetState went out of range: %lu\n", DeviceExtension->MouseResetState);
			return FALSE;
	}
}

BOOLEAN NTAPI
i8042MouInterruptService(
	IN PKINTERRUPT Interrupt,
	PVOID Context)
{
	PI8042_MOUSE_EXTENSION DeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	ULONG Counter;
	UCHAR Output, PortStatus;
	NTSTATUS Status;

	DeviceExtension = (PI8042_MOUSE_EXTENSION)Context;
	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;
	Counter = PortDeviceExtension->Settings.PollStatusIterations;

	while (Counter)
	{
		Status = i8042ReadStatus(PortDeviceExtension, &PortStatus);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("i8042ReadStatus() failed with status 0x%08lx\n", Status);
			return FALSE;
		}
		Status = i8042ReadMouseData(PortDeviceExtension, &Output);
		if (NT_SUCCESS(Status))
			break;
		KeStallExecutionProcessor(1);
		Counter--;
	}
	if (Counter == 0)
	{
		DPRINT("Spurious i8042 mouse interrupt\n");
		return FALSE;
	}

	DPRINT("Got: 0x%02x\n", Output);

	if (i8042PacketIsr(PortDeviceExtension, Output))
	{
		if (PortDeviceExtension->PacketComplete)
		{
			DPRINT("Packet complete\n");
			KeInsertQueueDpc(&DeviceExtension->DpcMouse, NULL, NULL);
		}
		DPRINT("Irq eaten by packet\n");
		return TRUE;
	}

	DPRINT("Irq is mouse input\n");

	i8042MouInputTestTimeout(DeviceExtension);

	if (i8042MouResetIsr(DeviceExtension, PortStatus, Output))
	{
		DPRINT("Handled by ResetIsr or hooked Isr\n");
		if (NoChange != DeviceExtension->MouseTimeoutState) {
			KeInsertQueueDpc(&DeviceExtension->DpcMouse, NULL, NULL);
		}
		return TRUE;
	}

	if (DeviceExtension->MouseType == Ps2pp)
		i8042MouHandlePs2pp(DeviceExtension, Output);
	else
		i8042MouHandle(DeviceExtension, Output);

	return TRUE;
}

/* ================================================
 * FIXME: this part is taken from psaux.sys driver,
 * and doesn't follow the coding style of this file
 * ===============================================*/

#define CONTROLLER_COMMAND_MOUSE_ENABLE 0xA8
#define PSMOUSE_CMD_ENABLE              0x00f4
#define PSMOUSE_CMD_GETID               (0x0200 | MOU_CMD_GET_ID)
#define PSMOUSE_CMD_RESET_BAT           (0x0200 | MOU_CMD_RESET)
#define PSMOUSE_RET_ACK                 MOUSE_ACK
#define PSMOUSE_RET_NAK                 MOUSE_NACK
#define MOUSE_INTERRUPTS_ON             (CCB_TRANSLATE | \
                                         CCB_SYSTEM_FLAG | \
                                         CCB_MOUSE_INT_ENAB)

typedef struct _I8042_MOUSE_EXTENSION_OLD
{
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	UCHAR pkt[8];
	UCHAR ack;
	UINT RepliesExpected;
} I8042_MOUSE_EXTENSION_OLD, *PI8042_MOUSE_EXTENSION_OLD;

/* Sends a byte to the mouse */
static INT SendByte(
	IN PI8042_MOUSE_EXTENSION_OLD DeviceExtension,
	IN UCHAR byte)
{
	INT timeout = 100; /* 100 msec */
	UCHAR scancode;
	LARGE_INTEGER Millisecond_Timeout;

	Millisecond_Timeout.QuadPart = -10000L;

	DeviceExtension->ack = 0;

	i8042IsrWritePort(DeviceExtension->PortDeviceExtension, byte, CTRL_WRITE_MOUSE);
	while ((DeviceExtension->ack == 0) && timeout--)
	{
		if (i8042ReadKeyboardData(DeviceExtension->PortDeviceExtension, &scancode))
		{
			switch(scancode)
			{
				case PSMOUSE_RET_ACK:
					DeviceExtension->ack = 1;
					break;
				case PSMOUSE_RET_NAK:
					DeviceExtension->ack = -1;
					break;
				default:
					DeviceExtension->ack = 1; /* Workaround for mice which don't ACK the Get ID command */
					if (DeviceExtension->RepliesExpected)
						DeviceExtension->pkt[--DeviceExtension->RepliesExpected] = scancode;
					break;
			}
			return (INT)(-(DeviceExtension->ack <= 0));
		}
		KeDelayExecutionThread(KernelMode, FALSE, &Millisecond_Timeout);
	}
	return (INT)(-(DeviceExtension->ack <= 0));
}

/* Send a PS/2 command to the mouse. */
static INT SendCommand(
	IN PI8042_MOUSE_EXTENSION_OLD DeviceExtension,
	IN PUCHAR param,
	IN INT command)
{
	LARGE_INTEGER Millisecond_Timeout;
	UCHAR scancode;
	INT timeout = 500; /* 500 msec */
	UCHAR send = (command >> 12) & 0xf;
	UCHAR receive = (command >> 8) & 0xf;
	UCHAR i;

	Millisecond_Timeout.QuadPart = -10000L;

	DeviceExtension->RepliesExpected = receive;
	if (command == PSMOUSE_CMD_RESET_BAT)
		timeout = 2000; /* 2 sec */

	if (command & 0xff)
		if (SendByte(DeviceExtension, command & 0xff))
			return (INT)(DeviceExtension->RepliesExpected = 0) - 1;

	for (i = 0; i < send; i++)
		if (SendByte(DeviceExtension, param[i]))
			return (INT)(DeviceExtension->RepliesExpected = 0) - 1;

	while (DeviceExtension->RepliesExpected && timeout--)
	{
		if (DeviceExtension->RepliesExpected == 1 && command == PSMOUSE_CMD_RESET_BAT)
			timeout = 100;

		if (DeviceExtension->RepliesExpected == 1 && command == PSMOUSE_CMD_GETID &&
			DeviceExtension->pkt[1] != 0xab && DeviceExtension->pkt[1] != 0xac)
		{
			DeviceExtension->RepliesExpected = 0;
			break;
		}

		if (i8042ReadKeyboardData(DeviceExtension->PortDeviceExtension, &scancode))
		{
			DeviceExtension->pkt[--DeviceExtension->RepliesExpected] = scancode;
		}

		KeDelayExecutionThread (KernelMode, FALSE, &Millisecond_Timeout);
	}

	for (i = 0; i < receive; i++)
		param[i] = DeviceExtension->pkt[(receive - 1) - i];

	if (DeviceExtension->RepliesExpected)
		return (int)(DeviceExtension->RepliesExpected = 0) - 1;

	return 0;
}

/* Detect if mouse is just a standard ps/2 mouse */
static BOOLEAN TestMouse(
	IN PI8042_MOUSE_EXTENSION_OLD DeviceExtension)
{
	UCHAR param[4];

	param[0] = param[1] = 0xa5;

	/*
	 * First, we check if it's a mouse. It should send 0x00 or 0x03
	 * in case of an IntelliMouse in 4-byte mode or 0x04 for IM Explorer.
	 */
	if(SendCommand(DeviceExtension, param, PSMOUSE_CMD_GETID))
		return -1;

	if(param[0] != 0x00 && param[0] != 0x03 && param[0] != 0x04)
		return -1;

	/*
	 * Then we reset and disable the mouse so that it doesn't generate events.
	 */

	return TRUE;
}

/* Initialize the PS/2 mouse support */
static BOOLEAN SetupMouse(
	IN PI8042_MOUSE_EXTENSION_OLD DeviceExtension)
{
	LARGE_INTEGER Millisecond_Timeout;

	Millisecond_Timeout.QuadPart = -10000L;

	/* setup */
	DeviceExtension->RepliesExpected = 0;
	DeviceExtension->ack = 0;

	/* Enable the PS/2 mouse port */
	i8042Write(DeviceExtension->PortDeviceExtension, DeviceExtension->PortDeviceExtension->ControlPort, CONTROLLER_COMMAND_MOUSE_ENABLE);

	if(TestMouse(DeviceExtension))
	{
		DPRINT("Detected Mouse\n");

		if(SendCommand(DeviceExtension, NULL, PSMOUSE_CMD_ENABLE))
			DPRINT1("Failed to enable mouse!\n");

		/* Enable controller interrupts */
		i8042Write(DeviceExtension->PortDeviceExtension, DeviceExtension->PortDeviceExtension->ControlPort, KBD_WRITE_MODE);
		i8042Write(DeviceExtension->PortDeviceExtension, DeviceExtension->PortDeviceExtension->DataPort, MOUSE_INTERRUPTS_ON);
	}

	return TRUE;
}

static NTSTATUS OldInitialization(
	IN PPORT_DEVICE_EXTENSION PortDeviceExtension)
{
	I8042_MOUSE_EXTENSION_OLD DeviceExtension;

	RtlZeroMemory(&DeviceExtension, sizeof(I8042_MOUSE_EXTENSION_OLD));
	DeviceExtension.PortDeviceExtension = PortDeviceExtension;
	SetupMouse(&DeviceExtension);
	return STATUS_SUCCESS;
}
