/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ps2.c
 * PURPOSE:         PS/2 controller emulation
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "io.h"
#include "ps2.h"
#include "pic.h"

#include "keyboard.h"
#include "mouse.h"

/* PRIVATE VARIABLES **********************************************************/

#define BUFFER_SIZE 32

typedef struct _PS2_PORT
{
    BOOLEAN IsEnabled;

    BOOLEAN QueueEmpty;
    BYTE    Queue[BUFFER_SIZE];
    UINT    QueueStart;
    UINT    QueueEnd;
    HANDLE  QueueMutex;
} PS2_PORT, *PPS2_PORT;

/*
 * Port 1: Keyboard
 * Port 2: Mouse
 */
#define PS2_PORTS  2
static PS2_PORT Ports[PS2_PORTS];

#define PS2_DEFAULT_CONFIG  0x47
static BYTE ControllerConfig = PS2_DEFAULT_CONFIG;
static BYTE ControllerCommand = 0x00;

static BYTE StatusRegister = 0x00;
// static BYTE InputBuffer  = 0x00; // PS/2 Input  Buffer
static BYTE OutputBuffer = 0x00; // PS/2 Output Buffer

/* PRIVATE FUNCTIONS **********************************************************/

static BYTE WINAPI PS2ReadPort(ULONG Port)
{
    if (Port == PS2_CONTROL_PORT)
    {
        /* Be sure bit 2 is always set */
        StatusRegister |= 1 << 2;

        // FIXME: Should clear bits 6 and 7 because there are
        // no timeouts and no parity errors.

        return StatusRegister;
    }
    else if (Port == PS2_DATA_PORT)
    {
        /*
         * If there is something to read (response byte from the
         * controller or data from a PS/2 device), read it.
         */
        if (StatusRegister &   (1 << 0)) // || StatusRegister &   (1 << 5) for second PS/2 port
            StatusRegister &= ~(1 << 0); //    StatusRegister &= ~(1 << 5);

        /* Always return the available byte stored in the output buffer */
        return OutputBuffer;
    }

    return 0;
}

static VOID WINAPI PS2WritePort(ULONG Port, BYTE Data)
{
    if (Port == PS2_CONTROL_PORT)
    {
        switch (Data)
        {
            /* Read configuration byte */
            case 0x20:
            {
                OutputBuffer = ControllerConfig;
                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* Write configuration byte */
            case 0x60:
            /* Write controller output port */
            case 0xD1:
            /* Write to the first PS/2 port output buffer */
            case 0xD2:
            /* Write to the second PS/2 port output buffer */
            case 0xD3:
            /* Write to the second PS/2 port input buffer */
            case 0xD4:
            {
                /* These commands require a response */
                ControllerCommand = Data;
                StatusRegister |= (1 << 3); // This is a controller command
                break;
            }

            /* Disable second PS/2 port */
            case 0xA7:
            {
                Ports[1].IsEnabled = FALSE;
                break;
            }

            /* Enable second PS/2 port */
            case 0xA8:
            {
                Ports[1].IsEnabled = TRUE;
                break;
            }

            /* Test second PS/2 port */
            case 0xA9:
            {
                OutputBuffer = 0x00; // Success code
                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* Test PS/2 controller */
            case 0xAA:
            {
                OutputBuffer = 0x55; // Success code
                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* Test first PS/2 port */
            case 0xAB:
            {
                OutputBuffer = 0x00; // Success code
                StatusRegister |= (1 << 0); // There is something to read
                break;
            }

            /* Disable first PS/2 port */
            case 0xAD:
            {
                Ports[0].IsEnabled = FALSE;
                break;
            }

            /* Enable first PS/2 port */
            case 0xAE:
            {
                Ports[0].IsEnabled = TRUE;
                break;
            }

            /* Read controller output port */
            case 0xD0:
            {
                // TODO: Not implemented
                break;
            }

            /* CPU Reset */
            case 0xF0:
            case 0xF2:
            case 0xF4:
            case 0xF6:
            case 0xF8:
            case 0xFA:
            case 0xFC:
            case 0xFE:
            {
                /* Stop the VDM */
                EmulatorTerminate();
                break;
            }
        }
    }
    else if (Port == PS2_DATA_PORT)
    {
        /* Check if the controller is waiting for a response */
        if (StatusRegister & (1 << 3)) // If we have data for the controller
        {
            StatusRegister &= ~(1 << 3);

            /* Check which command it was */
            switch (ControllerCommand)
            {
                /* Write configuration byte */
                case 0x60:
                {
                    ControllerConfig = Data;
                    break;
                }

                /* Write controller output */
                case 0xD1:
                {
                    /* Check if bit 0 is unset */
                    if (!(Data & (1 << 0)))
                    {
                        /* CPU disabled - Stop the VDM */
                        EmulatorTerminate();
                    }

                    /* Update the A20 line setting */
                    EmulatorSetA20(Data & (1 << 1));

                    break;
                }

                /* Push the data byte into the first PS/2 port queue */
                case 0xD2:
                {
                    PS2QueuePush(0, Data);
                    break;
                }

                /* Push the data byte into the second PS/2 port queue */
                case 0xD3:
                {
                    PS2QueuePush(1, Data);
                    break;
                }

                /*
                 * Send a command to the second PS/2 port (by default
                 * it is a command for the first PS/2 port)
                 */
                case 0xD4:
                {
                    if (Ports[1].IsEnabled)
                        // Ports[1].Function
                        MouseCommand(Data);

                    break;
                }
            }

            return;
        }

        // TODO: Implement PS/2 device commands
        if (Ports[0].IsEnabled)
            // Ports[0].Function
            KeyboardCommand(Data);
    }
}

static BOOLEAN PS2PortQueueRead(BYTE PS2Port)
{
    BOOLEAN Result = TRUE;
    PPS2_PORT Port;

    if (PS2Port >= PS2_PORTS) return FALSE;
    Port = &Ports[PS2Port];

    if (!Port->IsEnabled) return FALSE;

    /* Make sure the queue is not empty (fast check) */
    if (Port->QueueEmpty) return FALSE;

    WaitForSingleObject(Port->QueueMutex, INFINITE);

    /*
     * Recheck whether the queue is not empty (it may
     * have changed after having grabbed the mutex).
     */
    if (Port->QueueEmpty)
    {
        Result = FALSE;
        goto Done;
    }

    /* Get the data */
    OutputBuffer = Port->Queue[Port->QueueStart];
    StatusRegister |= (1 << 0); // There is something to read
    // Sometimes StatusRegister |= (1 << 5); for the second PS/2 port

    /* Remove the value from the queue */
    Port->QueueStart++;
    Port->QueueStart %= BUFFER_SIZE;

    /* Check if the queue is now empty */
    if (Port->QueueStart == Port->QueueEnd)
        Port->QueueEmpty = TRUE;

Done:
    ReleaseMutex(Port->QueueMutex);
    return Result;
}

/* PUBLIC FUNCTIONS ***********************************************************/

// PS2SendToPort
BOOLEAN PS2QueuePush(BYTE PS2Port, BYTE Data)
{
    BOOLEAN Result = TRUE;
    PPS2_PORT Port;

    if (PS2Port >= PS2_PORTS) return FALSE;
    Port = &Ports[PS2Port];

    if (!Port->IsEnabled) return FALSE;

    WaitForSingleObject(Port->QueueMutex, INFINITE);

    /* Check if the queue is full */
    if (!Port->QueueEmpty && (Port->QueueStart == Port->QueueEnd))
    {
        Result = FALSE;
        goto Done;
    }

    /* Insert the value in the queue */
    Port->Queue[Port->QueueEnd] = Data;
    Port->QueueEnd++;
    Port->QueueEnd %= BUFFER_SIZE;

    /* Since we inserted a value, it's not empty anymore */
    Port->QueueEmpty = FALSE;

/*
    // Get the data
    OutputBuffer = Port->Queue[Port->QueueStart];
    StatusRegister |= (1 << 0); // There is something to read
    // FIXME: Sometimes StatusRegister |= (1 << 5); for the second PS/2 port

    if (PS2Port == 0)
        PicInterruptRequest(1);
    else if (PS2Port == 1)
        PicInterruptRequest(12);
*/

Done:
    ReleaseMutex(Port->QueueMutex);
    return Result;
}

VOID GenerateIrq1(VOID)
{
    /* Generate an interrupt if interrupts for the first PS/2 port are enabled */
    if (ControllerConfig & 0x01)
    {
        /* Generate an IRQ 1 if there is data ready in the output queue */
        if (PS2PortQueueRead(0)) PicInterruptRequest(1);
    }
}

VOID GenerateIrq12(VOID)
{
    /* Generate an interrupt if interrupts for the second PS/2 port are enabled */
    if (ControllerConfig & 0x02)
    {
        /* Generate an IRQ 12 if there is data ready in the output queue */
        if (PS2PortQueueRead(1)) PicInterruptRequest(12);
    }
}

BOOLEAN PS2Initialize(VOID)
{
    /* Initialize the PS/2 ports */
    Ports[0].IsEnabled  = TRUE;
    Ports[0].QueueEmpty = TRUE;
    Ports[0].QueueStart = 0;
    Ports[0].QueueEnd   = 0;
    Ports[0].QueueMutex = CreateMutex(NULL, FALSE, NULL);

    Ports[1].IsEnabled  = TRUE;
    Ports[1].QueueEmpty = TRUE;
    Ports[1].QueueStart = 0;
    Ports[1].QueueEnd   = 0;
    Ports[1].QueueMutex = CreateMutex(NULL, FALSE, NULL);

    /* Register the I/O Ports */
    RegisterIoPort(PS2_CONTROL_PORT, PS2ReadPort, PS2WritePort);
    RegisterIoPort(PS2_DATA_PORT   , PS2ReadPort, PS2WritePort);

    return TRUE;
}

VOID PS2Cleanup(VOID)
{
    CloseHandle(Ports[1].QueueMutex);
    CloseHandle(Ports[0].QueueMutex);
}

/* EOF */
