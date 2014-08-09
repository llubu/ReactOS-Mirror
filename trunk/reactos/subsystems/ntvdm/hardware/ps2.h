/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ps2.h
 * PURPOSE:         PS/2 controller emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _PS2_H_
#define _PS2_H_

/* INCLUDES *******************************************************************/

#include "ntvdm.h"

/* DEFINES ********************************************************************/

#define KEYBOARD_BUFFER_SIZE 32
#define PS2_DATA_PORT 0x60
#define PS2_CONTROL_PORT 0x64
#define PS2_DEFAULT_CONFIG 0x05
#define KEYBOARD_ACK 0xFA
#define KEYBOARD_RESEND 0xFE

/* FUNCTIONS ******************************************************************/

BOOLEAN KeyboardQueuePush(BYTE ScanCode);
BOOLEAN KeyboardQueuePop(BYTE *ScanCode);
VOID PS2Dispatch(PINPUT_RECORD InputRecord);
VOID GenerateKeyboardInterrupts(VOID);

BOOLEAN PS2Initialize(HANDLE ConsoleInput);
VOID PS2Cleanup(VOID);

#endif // _PS2_H_

/* EOF */
