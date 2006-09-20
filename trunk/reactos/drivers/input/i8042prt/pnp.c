/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/pnp.c
 * PURPOSE:     IRP_MJ_PNP operations
 * PROGRAMMERS: Copyright 2006 Herv� Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "i8042prt.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/* This is all pretty confusing. There's more than one way to
 * disable/enable the keyboard. You can send KBD_ENABLE to the
 * keyboard, and it will start scanning keys. Sending KBD_DISABLE
 * will disable the key scanning but also reset the parameters to
 * defaults.
 *
 * You can also send 0xAE to the controller for enabling the
 * keyboard clock line and 0xAD for disabling it. Then it'll
 * automatically get turned on at the next command. The last
 * way is by modifying the bit that drives the clock line in the
 * 'command byte' of the controller. This is almost, but not quite,
 * the same as the AE/AD thing. The difference can be used to detect
 * some really old broken keyboard controllers which I hope won't be
 * necessary.
 *
 * We change the command byte, sending KBD_ENABLE/DISABLE seems to confuse
 * some kvm switches.
 */
BOOLEAN
i8042ChangeMode(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR FlagsToDisable,
	IN UCHAR FlagsToEnable)
{
	UCHAR Value;
	NTSTATUS Status;

	if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, KBD_READ_MODE))
	{
		DPRINT1("Can't read i8042 mode\n");
		return FALSE;
	}

	Status = i8042ReadDataWait(DeviceExtension, &Value);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("No response after read i8042 mode\n");
		return FALSE;
	}

	Value &= ~FlagsToDisable;
	Value |= FlagsToEnable;

	if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, KBD_WRITE_MODE))
	{
		DPRINT1("Can't set i8042 mode\n");
		return FALSE;
	}

	if (!i8042Write(DeviceExtension, DeviceExtension->DataPort, Value))
	{
		DPRINT1("Can't send i8042 mode\n");
		return FALSE;
	}

	return TRUE;
}

static NTSTATUS
i8042BasicDetect(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	NTSTATUS Status;
	UCHAR Value = 0;

	i8042Flush(DeviceExtension);

	if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, CTRL_SELF_TEST))
	{
		DPRINT1("Writing CTRL_SELF_TEST command failed\n");
		return STATUS_IO_TIMEOUT;
	}

	Status = i8042ReadDataWait(DeviceExtension, &Value);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("Failed to read CTRL_SELF_TEST response, status 0x%08lx\n", Status);
		return Status;
	}

	if (Value != 0x55)
	{
		DPRINT1("Got 0x%02x instead of 0x55\n", Value);
		return STATUS_IO_DEVICE_ERROR;
	}

	/* Don't enable keyboard and mouse interrupts, disable keyboard/mouse */
	if (!i8042ChangeMode(DeviceExtension, CCB_KBD_INT_ENAB | CCB_MOUSE_INT_ENAB, CCB_KBD_DISAB | CCB_MOUSE_DISAB))
		return STATUS_IO_DEVICE_ERROR;

	/*
	 * We used to send a KBD_LINE_TEST (0xAB) command here, but on at least HP
	 * Pavilion notebooks the response to that command was incorrect.
	 * So now we just assume that a keyboard is attached.
	 */
	DeviceExtension->Flags |= KEYBOARD_PRESENT;

	if (i8042Write(DeviceExtension, DeviceExtension->ControlPort, MOUSE_LINE_TEST))
	{
		Status = i8042ReadDataWait(DeviceExtension, &Value);
		if (NT_SUCCESS(Status) && Value == 0)
			DeviceExtension->Flags |= MOUSE_PRESENT;
	}

	if (IsFirstStageSetup())
		/* Ignore the mouse */
		DeviceExtension->Flags &= ~MOUSE_PRESENT;

	return STATUS_SUCCESS;
}

static BOOLEAN
i8042DetectKeyboard(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	NTSTATUS Status;

	if (!i8042ChangeMode(DeviceExtension, 0, CCB_KBD_DISAB))
		return FALSE;

	i8042Flush(DeviceExtension);

	/* Set LEDs (that is not fatal if some error occurs) */
	Status = i8042SynchWritePort(DeviceExtension, 0, KBD_CMD_SET_LEDS, TRUE);
	if (NT_SUCCESS(Status))
	{
		Status = i8042SynchWritePort(DeviceExtension, 0, 0, TRUE);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("Can't finish SET_LEDS (0x%08lx)\n", Status);
			return FALSE;
		}
	}
	else
	{
		DPRINT("Warning: can't write SET_LEDS (0x%08lx)\n", Status);
	}

	/* Turn on translation */
	if (!i8042ChangeMode(DeviceExtension, 0, CCB_TRANSLATE))
		return FALSE;

	return TRUE;
}

static BOOLEAN
i8042DetectMouse(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	BOOLEAN Ok = FALSE;
	NTSTATUS Status;
	UCHAR Value;
	UCHAR ExpectedReply[] = { MOUSE_ACK, 0xAA, 0x00 };
	UCHAR ReplyByte;

	i8042Flush(DeviceExtension);

	if (!i8042Write(DeviceExtension, DeviceExtension->ControlPort, CTRL_WRITE_MOUSE)
	  ||!i8042Write(DeviceExtension, DeviceExtension->DataPort, MOU_CMD_RESET))
	{
		DPRINT1("Failed to write reset command to mouse\n");
		goto cleanup;
	}

	for (ReplyByte = 0;
	     ReplyByte < sizeof(ExpectedReply) / sizeof(ExpectedReply[0]);
	     ReplyByte++)
	{
		Status = i8042ReadDataWait(DeviceExtension, &Value);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("No ACK after mouse reset, status 0x%08lx\n", Status);
			goto cleanup;
		}
		else if (Value != ExpectedReply[ReplyByte])
		{
			DPRINT1("Unexpected reply: 0x%02x (expected 0x%02x)\n",
			        Value, ExpectedReply[ReplyByte]);
			goto cleanup;
		}
	}

	Ok = TRUE;

cleanup:
	if (!Ok)
	{
		/* There is probably no mouse present. On some systems,
		   the probe locks the entire keyboard controller. Let's
		   try to get access to the keyboard again by sending a
		   reset */
		i8042Flush(DeviceExtension);
		i8042Write(DeviceExtension, DeviceExtension->ControlPort, CTRL_SELF_TEST);
		i8042ReadDataWait(DeviceExtension, &Value);
		i8042Flush(DeviceExtension);
	}

	DPRINT("Mouse %sdetected\n", Ok ? "" : "not ");

	return Ok;
}

static NTSTATUS
i8042ConnectKeyboardInterrupt(
	IN PI8042_KEYBOARD_EXTENSION DeviceExtension)
{
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	KIRQL DirqlMax;
	NTSTATUS Status;

	DPRINT("i8042ConnectKeyboardInterrupt()\n");

	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;
	DirqlMax = MAX(
		PortDeviceExtension->KeyboardInterrupt.Dirql,
		PortDeviceExtension->MouseInterrupt.Dirql);

	DPRINT("KeyboardInterrupt.Vector         %lu\n",
		PortDeviceExtension->KeyboardInterrupt.Vector);
	DPRINT("KeyboardInterrupt.Dirql          %lu\n",
		PortDeviceExtension->KeyboardInterrupt.Dirql);
	DPRINT("KeyboardInterrupt.DirqlMax       %lu\n",
		DirqlMax);
	DPRINT("KeyboardInterrupt.InterruptMode  %s\n",
		PortDeviceExtension->KeyboardInterrupt.InterruptMode == LevelSensitive ? "LevelSensitive" : "Latched");
	DPRINT("KeyboardInterrupt.ShareInterrupt %s\n",
		PortDeviceExtension->KeyboardInterrupt.ShareInterrupt ? "yes" : "no");
	DPRINT("KeyboardInterrupt.Affinity       0x%lx\n",
		PortDeviceExtension->KeyboardInterrupt.Affinity);
	Status = IoConnectInterrupt(
		&PortDeviceExtension->KeyboardInterrupt.Object,
		i8042KbdInterruptService,
		DeviceExtension, &PortDeviceExtension->SpinLock,
		PortDeviceExtension->KeyboardInterrupt.Vector, PortDeviceExtension->KeyboardInterrupt.Dirql, DirqlMax,
		PortDeviceExtension->KeyboardInterrupt.InterruptMode, PortDeviceExtension->KeyboardInterrupt.ShareInterrupt,
		PortDeviceExtension->KeyboardInterrupt.Affinity, FALSE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoConnectInterrupt() failed with status 0x%08x\n", Status);
		return Status;
	}

	if (DirqlMax == PortDeviceExtension->KeyboardInterrupt.Dirql)
		PortDeviceExtension->HighestDIRQLInterrupt = PortDeviceExtension->KeyboardInterrupt.Object;
	PortDeviceExtension->Flags |= KEYBOARD_INITIALIZED;
	return STATUS_SUCCESS;
}

static NTSTATUS
i8042ConnectMouseInterrupt(
	IN PI8042_MOUSE_EXTENSION DeviceExtension)
{
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	KIRQL DirqlMax;
	NTSTATUS Status;

	DPRINT("i8042ConnectMouseInterrupt()\n");

	Status = i8042MouInitialize(DeviceExtension);
	if (!NT_SUCCESS(Status))
		return Status;

	PortDeviceExtension = DeviceExtension->Common.PortDeviceExtension;
	DirqlMax = MAX(
		PortDeviceExtension->KeyboardInterrupt.Dirql,
		PortDeviceExtension->MouseInterrupt.Dirql);

	DPRINT("MouseInterrupt.Vector         %lu\n",
		PortDeviceExtension->MouseInterrupt.Vector);
	DPRINT("MouseInterrupt.Dirql          %lu\n",
		PortDeviceExtension->MouseInterrupt.Dirql);
	DPRINT("MouseInterrupt.DirqlMax       %lu\n",
		DirqlMax);
	DPRINT("MouseInterrupt.InterruptMode  %s\n",
		PortDeviceExtension->MouseInterrupt.InterruptMode == LevelSensitive ? "LevelSensitive" : "Latched");
	DPRINT("MouseInterrupt.ShareInterrupt %s\n",
		PortDeviceExtension->MouseInterrupt.ShareInterrupt ? "yes" : "no");
	DPRINT("MouseInterrupt.Affinity       0x%lx\n",
		PortDeviceExtension->MouseInterrupt.Affinity);
	Status = IoConnectInterrupt(
		&PortDeviceExtension->MouseInterrupt.Object,
		i8042MouInterruptService,
		DeviceExtension, &PortDeviceExtension->SpinLock,
		PortDeviceExtension->MouseInterrupt.Vector, PortDeviceExtension->MouseInterrupt.Dirql, DirqlMax,
		PortDeviceExtension->MouseInterrupt.InterruptMode, PortDeviceExtension->MouseInterrupt.ShareInterrupt,
		PortDeviceExtension->MouseInterrupt.Affinity, FALSE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoConnectInterrupt() failed with status 0x%08x\n", Status);
		goto cleanup;
	}

	if (DirqlMax == PortDeviceExtension->MouseInterrupt.Dirql)
		PortDeviceExtension->HighestDIRQLInterrupt = PortDeviceExtension->MouseInterrupt.Object;

	PortDeviceExtension->Flags |= MOUSE_INITIALIZED;
	Status = STATUS_SUCCESS;

cleanup:
	if (!NT_SUCCESS(Status))
	{
		PortDeviceExtension->Flags &= ~MOUSE_INITIALIZED;
		if (PortDeviceExtension->MouseInterrupt.Object)
		{
			IoDisconnectInterrupt(PortDeviceExtension->MouseInterrupt.Object);
			PortDeviceExtension->HighestDIRQLInterrupt = PortDeviceExtension->KeyboardInterrupt.Object;
		}
	}
	return Status;
}

static NTSTATUS
EnableInterrupts(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	UCHAR FlagsToDisable = 0;
	UCHAR FlagsToEnable = 0;

	/* Select the devices we have */
	if (DeviceExtension->Flags & KEYBOARD_PRESENT)
	{
		FlagsToDisable |= CCB_KBD_DISAB;
		FlagsToEnable |= CCB_KBD_INT_ENAB;
	}
	if (DeviceExtension->Flags & MOUSE_PRESENT)
	{
		FlagsToDisable |= CCB_MOUSE_DISAB;
		FlagsToEnable |= CCB_MOUSE_INT_ENAB;
	}
	if (!i8042ChangeMode(DeviceExtension, FlagsToDisable, FlagsToEnable))
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

static NTSTATUS
StartProcedure(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	NTSTATUS Status;

	if (DeviceExtension->DataPort == 0)
	{
		/* Unable to do something at the moment */
		return STATUS_SUCCESS;
	}

	if (!(DeviceExtension->Flags & (KEYBOARD_PRESENT | MOUSE_PRESENT)))
	{
		/* Try to detect them */
		DPRINT("Check if the controller is really a i8042\n");
		Status = i8042BasicDetect(DeviceExtension);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("i8042BasicDetect() failed with status 0x%08lx\n", Status);
			return STATUS_UNSUCCESSFUL;
		}
		DPRINT("Detecting keyboard\n");
		if (!i8042DetectKeyboard(DeviceExtension))
			return STATUS_UNSUCCESSFUL;
		DPRINT("Detecting mouse\n");
		if (!i8042DetectMouse(DeviceExtension))
			return STATUS_UNSUCCESSFUL;
		DPRINT("Keyboard present: %s\n", DeviceExtension->Flags & KEYBOARD_PRESENT ? "YES" : "NO");
		DPRINT("Mouse present   : %s\n", DeviceExtension->Flags & MOUSE_PRESENT ? "YES" : "NO");
	}

	/* Connect interrupts */
	if (DeviceExtension->Flags & KEYBOARD_PRESENT &&
	    DeviceExtension->Flags & KEYBOARD_CONNECTED &&
	    DeviceExtension->Flags & KEYBOARD_STARTED &&
	    !(DeviceExtension->Flags & (MOUSE_PRESENT | KEYBOARD_INITIALIZED)))
	{
		/* No mouse, and the keyboard is ready */
		Status = i8042ConnectKeyboardInterrupt(DeviceExtension->KeyboardExtension);
		if (NT_SUCCESS(Status))
		{
			DeviceExtension->Flags |= KEYBOARD_INITIALIZED;
			Status = EnableInterrupts(DeviceExtension);
		}
	}
	else if (DeviceExtension->Flags & MOUSE_PRESENT &&
	         DeviceExtension->Flags & MOUSE_CONNECTED &&
	         DeviceExtension->Flags & MOUSE_STARTED &&
	         !(DeviceExtension->Flags & (KEYBOARD_PRESENT | MOUSE_INITIALIZED)))
	{
		/* No keyboard, and the mouse is ready */
		Status = i8042ConnectMouseInterrupt(DeviceExtension->MouseExtension);
		if (NT_SUCCESS(Status))
		{
			DeviceExtension->Flags |= MOUSE_INITIALIZED;
			Status = EnableInterrupts(DeviceExtension);
		}
	}
	else if (DeviceExtension->Flags & KEYBOARD_PRESENT &&
	         DeviceExtension->Flags & KEYBOARD_CONNECTED &&
	         DeviceExtension->Flags & KEYBOARD_STARTED &&
	         DeviceExtension->Flags & MOUSE_PRESENT &&
	         DeviceExtension->Flags & MOUSE_CONNECTED &&
	         DeviceExtension->Flags & MOUSE_STARTED &&
	         !(DeviceExtension->Flags & (KEYBOARD_INITIALIZED | MOUSE_INITIALIZED)))
	{
		/* The keyboard and mouse are ready */
		Status = i8042ConnectKeyboardInterrupt(DeviceExtension->KeyboardExtension);
		if (NT_SUCCESS(Status))
		{
			DeviceExtension->Flags |= KEYBOARD_INITIALIZED;
			Status = i8042ConnectMouseInterrupt(DeviceExtension->MouseExtension);
			if (NT_SUCCESS(Status))
			{
				DeviceExtension->Flags |= MOUSE_INITIALIZED;
				Status = EnableInterrupts(DeviceExtension);
			}
		}
	}
	else
	{
		/* Nothing to do */
		Status = STATUS_SUCCESS;
	}

	return Status;
}

static NTSTATUS
i8042PnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCM_RESOURCE_LIST AllocatedResources,
	IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor, ResourceDescriptorTranslated;
	INTERRUPT_DATA InterruptData;
	BOOLEAN FoundDataPort = FALSE;
	BOOLEAN FoundControlPort = FALSE;
	BOOLEAN FoundIrq = FALSE;
	ULONG i;
	NTSTATUS Status;

	DPRINT("i8042PnpStartDevice(%p)\n", DeviceObject);
	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PortDeviceExtension = DeviceExtension->PortDeviceExtension;

	ASSERT(DeviceExtension->PnpState == dsStopped);

	if (!AllocatedResources)
	{
		DPRINT("No allocated resources sent to driver\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	if (AllocatedResources->Count != 1)
	{
		DPRINT("Wrong number of allocated resources sent to driver\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	if (AllocatedResources->List[0].PartialResourceList.Version != 1
	 || AllocatedResources->List[0].PartialResourceList.Revision != 1
	 || AllocatedResourcesTranslated->List[0].PartialResourceList.Version != 1
	 || AllocatedResourcesTranslated->List[0].PartialResourceList.Revision != 1)
	{
		DPRINT("Revision mismatch: %u.%u != 1.1 or %u.%u != 1.1\n",
			AllocatedResources->List[0].PartialResourceList.Version,
			AllocatedResources->List[0].PartialResourceList.Revision,
			AllocatedResourcesTranslated->List[0].PartialResourceList.Version,
			AllocatedResourcesTranslated->List[0].PartialResourceList.Revision);
		return STATUS_REVISION_MISMATCH;
	}

	/* Get Irq and optionally control port and data port */
	for (i = 0; i < AllocatedResources->List[0].PartialResourceList.Count; i++)
	{
		ResourceDescriptor = &AllocatedResources->List[0].PartialResourceList.PartialDescriptors[i];
		ResourceDescriptorTranslated = &AllocatedResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];
		switch (ResourceDescriptor->Type)
		{
			case CmResourceTypePort:
			{
				if (ResourceDescriptor->u.Port.Length == 1)
				{
					/* We assume that the first ressource will
					 * be the control port and the second one
					 * will be the data port...
					 */
					if (!FoundDataPort)
					{
						PortDeviceExtension->DataPort = (PUCHAR)ResourceDescriptor->u.Port.Start.u.LowPart;
						DPRINT("Found data port: 0x%lx\n", PortDeviceExtension->DataPort);
						FoundDataPort = TRUE;
					}
					else if (!FoundControlPort)
					{
						PortDeviceExtension->ControlPort = (PUCHAR)ResourceDescriptor->u.Port.Start.u.LowPart;
						DPRINT("Found control port: 0x%lx\n", PortDeviceExtension->ControlPort);
						FoundControlPort = TRUE;
					}
					else
					{
						DPRINT("Too much I/O ranges provided\n", ResourceDescriptor->u.Port.Length);
						return STATUS_INVALID_PARAMETER;
					}
				}
				else
					DPRINT1("Invalid I/O range length: 0x%lx\n", ResourceDescriptor->u.Port.Length);
				break;
			}
			case CmResourceTypeInterrupt:
			{
				if (FoundIrq)
					return STATUS_INVALID_PARAMETER;
				InterruptData.Dirql = (KIRQL)ResourceDescriptorTranslated->u.Interrupt.Level;
				InterruptData.Vector = ResourceDescriptorTranslated->u.Interrupt.Vector;
				InterruptData.Affinity = ResourceDescriptorTranslated->u.Interrupt.Affinity;
				if (ResourceDescriptorTranslated->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
					InterruptData.InterruptMode = Latched;
				else
					InterruptData.InterruptMode = LevelSensitive;
				InterruptData.ShareInterrupt = (ResourceDescriptorTranslated->ShareDisposition == CmResourceShareShared);
				DPRINT("Found irq resource: %lu\n", ResourceDescriptor->u.Interrupt.Vector);
				FoundIrq = TRUE;
				break;
			}
			default:
				DPRINT("Unknown resource descriptor type 0x%x\n", ResourceDescriptor->Type);
		}
	}

	if (!FoundIrq)
	{
		DPRINT("Interrupt resource was not found in allocated resources list\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	else if (DeviceExtension->Type == Keyboard && (!FoundDataPort || !FoundControlPort))
	{
		DPRINT("Some required resources were not found in allocated resources list\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	else if (DeviceExtension->Type == Mouse && (FoundDataPort || FoundControlPort))
	{
		DPRINT("Too much resources were provided in allocated resources list\n");
		return STATUS_INVALID_PARAMETER;
	}

	switch (DeviceExtension->Type)
	{
		case Keyboard:
		{
			RtlCopyMemory(
				&PortDeviceExtension->KeyboardInterrupt,
				&InterruptData,
				sizeof(INTERRUPT_DATA));
			PortDeviceExtension->Flags |= KEYBOARD_STARTED;
			Status = StartProcedure(PortDeviceExtension);
			break;
		}
		case Mouse:
		{
			RtlCopyMemory(
				&PortDeviceExtension->MouseInterrupt,
				&InterruptData,
				sizeof(INTERRUPT_DATA));
			PortDeviceExtension->Flags |= MOUSE_STARTED;
			Status = StartProcedure(PortDeviceExtension);
			break;
		}
		default:
		{
			DPRINT1("Unknown FDO type %u\n", DeviceExtension->Type);
			ASSERT(FALSE);
			Status = STATUS_INVALID_DEVICE_REQUEST;
		}
	}

	if (NT_SUCCESS(Status))
		DeviceExtension->PnpState = dsStarted;

	return Status;
}

NTSTATUS NTAPI
i8042Pnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	ULONG MinorFunction;
	I8042_DEVICE_TYPE DeviceType;
	ULONG_PTR Information = 0;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;
	DeviceType = ((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Type;

	switch (MinorFunction)
	{
		case IRP_MN_START_DEVICE: /* 0x0 */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

			/* Call lower driver (if any) */
			if (DeviceType != PhysicalDeviceObject)
			{
				Status = ForwardIrpAndWait(DeviceObject, Irp);
				if (NT_SUCCESS(Status))
					Status = i8042PnpStartDevice(
						DeviceObject,
						Stack->Parameters.StartDevice.AllocatedResources,
						Stack->Parameters.StartDevice.AllocatedResourcesTranslated);
			}
			else
				Status = STATUS_SUCCESS;
			break;
		}
		case IRP_MN_QUERY_DEVICE_RELATIONS: /* (optional) 0x7 */
		{
			switch (Stack->Parameters.QueryDeviceRelations.Type)
			{
				case BusRelations:
				{
					DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
					return ForwardIrpAndForget(DeviceObject, Irp);
				}
				case RemovalRelations:
				{
					DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
					return ForwardIrpAndForget(DeviceObject, Irp);
				}
				default:
					DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceRelations.Type);
					ASSERT(FALSE);
					return ForwardIrpAndForget(DeviceObject, Irp);
			}
			break;
		}
		default:
		{
			DPRINT1("IRP_MJ_PNP / unknown minor function 0x%x\n", MinorFunction);
			ASSERT(FALSE);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
