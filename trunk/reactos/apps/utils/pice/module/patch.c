/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    patch.c

Abstract:

    hooking of kernel internal keyboard interrupt handler

Environment:

    Kernel mode only

Author:

    Klaus P. Gerlicher
 	Reactos Port: Eugene Ingerman

Revision History:

    10-Jul-1999:	created
    15-Nov-2000:    general cleanup of source files
  	12/1/2001		reactos port

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/

////////////////////////////////////////////////////
// INCLUDES
////
#include "remods.h"
#include "precomp.h"

//#include <asm/system.h>

#include <ddk/ntddkbd.h>
#include <ddk/ntdd8042.h>

////////////////////////////////////////////////////
// GLOBALS
////

static PUCHAR pPatchAddress;
static ULONG ulOldOffset = 0;
static ULONG ulKeyPatchFlags;

void (*old_handle_scancode)(UCHAR,int);
char tempPatch[256];
UCHAR ucBreakKey = 'D'; // key that will break into debugger in combination with CTRL

////////////////////////////////////////////////////
// FUNCTIONS
////

//***********************************************************************************
//	PiceKbdIsr - keyboard isr hook routine.
//	IsrContext - context that we passed to keyboard driver in  internal iocontrol
//	pCurrentInput, pCurrentOutput - not implemented yet
//	StatusByte -  keyboard status register
//	pByte - pointer to the byte read from keyboard data port. can be changed.
//	pContinueProcessing - should keyboard driver continue processing this byte.
//***********************************************************************************
BOOLEAN PiceKbdIsr (
    PVOID                   IsrContext,
    PKEYBOARD_INPUT_DATA    pCurrentInput,
    POUTPUT_PACKET          pCurrentOutput,
    UCHAR                   StatusByte,
    PUCHAR                  pByte,
    PBOOLEAN                pContinueProcessing,
    PKEYBOARD_SCAN_STATE    pScanState
    )
{
	static BOOLEAN bControl = FALSE;
	BOOLEAN bForward=TRUE;              // should we let keyboard driver process this keystroke
	BOOLEAN isDown=!(*pByte & 0x80);
	UCHAR ucKey = *pByte & 0x7f;

    ENTER_FUNC();
	// BUG!! should protect with spinlock since bControl is static.
    DPRINT((0,"PiceKbdIsr(%x,%u)\n",pByte,isDown));
    DPRINT((0,"PiceKbdIsr(1): bControl = %u bForward = %u bEnterNow = %u\n",bControl,bForward,bEnterNow));

	if(isDown)
	{
        // CTRL pressed
		if(ucKey==0x1d)
		{
			bControl=TRUE;
		}
		else if(bControl==TRUE && ucKey==AsciiToScan(ucBreakKey)) // CTRL-D
		{
            // fake a CTRL-D release call
			bForward=FALSE;
            bEnterNow=TRUE;
			bControl=FALSE;
		}
        else if((ucKey == 66|| ucKey == 68) && bStepping)
        {
			bForward=FALSE;
        }

	}
	else
	{
        // CTRL released
		if(ucKey==0x1d)
		{
			bControl=FALSE;
		}
        else if((ucKey == 66|| ucKey == 68) && bStepping)
        {
			bForward=FALSE;
        }
    }
	*ContinueProcessing = bForward;
    LEAVE_FUNC();
	return TRUE;
}

//***********************************************************************************
//	PiceSendIoctl - send internal_io_control to the driver
//	Target - Device Object that receives control request
//	Ioctl - request
//	InputBuffer - Type3Buffer will be pointing here
//	InputBufferLength - length of inputbuffer
//***********************************************************************************
NTSTATUS PiceSendIoctl(PDEVICE_OBJECT Target, ULONG Ioctl,
					PVOID InputBuffer, ULONG InputBufferLength)
{
    KEVENT          event;
    NTSTATUS        status = STATUS_SUCCESS;
    IO_STATUS_BLOCK iosb;
    PIRP            irp;

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE
                      );

    if (NULL == (irp = IoBuildDeviceIoControlRequest(Ioctl,
                                                     Target,
                                                     InputBuffer,
                                                     InputBufferLength,
                                                     0,
                                                     0,
                                                     TRUE,
                                                     &event,
                                                     &iosb))) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Target, irp);

    if (STATUS_PENDING == status) {

		status = KeWaitForSingleObject(&event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        assert(STATUS_SUCCESS == status);
        status = iosb.Status;
    }

    return status;
}

//**************************************************
// PatchKeyboardDriver - set keyboard driver hook.
// We use interface supported by standard keyboard drivers.
//**************************************************
BOOLEAN PatchKeyboardDriver(void)
{
	PINTERNAL_I8042_HOOK_KEYBOARD phkData;
    UNICODE_STRING DevName;
	PDEVICE_OBJECT kbdDevice = NULL;
	PFILE_OBJECT FO = NULL;
	NTSTATUS status;

	ENTER_FUNC();
	//When we have i8042 driver this should be changed!!!!!!!
	RtlInitUnicodeString(&DevName, L"\\Device\\Keyboard");

	//Get pointer to keyboard device
    if( !NT_SUCCESS( IoGetDeviceObjectPointer( &DevName, FILE_READ_ACCESS, &FO, &kbdDevice ) ) )
		return FALSE;

	phkData = ExAllocatePool( PagedPool, sizeof( INTERNAL_I8042_HOOK_KEYBOARD ) );
	RtlZeroMemory( phkData, sizeof( INTERNAL_I8042_HOOK_KEYBOARD ) );

	phkData->IsrRoutine = (PI8042_KEYBOARD_ISR) PiceKbdIsr;
	phkData->Context = (PVOID) NULL; //DeviceObject;

	//call keyboard device internal io control to hook keyboard input stream
	status = PiceSendIoctl( kbdDevice, IOCTL_INTERNAL_I8042_HOOK_KEYBOARD,
			phkData, sizeof( INTERNAL_I8042_HOOK_KEYBOARD ) );


	ObDereferenceObject(FO);
	ExFreePool(phkData);

	LEAVE_FUNC();

    return NT_SUCCESS(status);
}

void RestoreKeyboardDriver(void)
{
    ENTER_FUNC();
    DbgPrint("RestoreKeyboardDriver: Not Implemented yet!!!\n");
	LEAVE_FUNC();
}
