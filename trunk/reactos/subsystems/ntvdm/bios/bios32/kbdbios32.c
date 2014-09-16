/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            kbdbios32.c
 * PURPOSE:         VDM Keyboard 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "callback.h"

#include "kbdbios32.h"
#include "../kbdbios.h"
#include "bios32p.h"

#include "io.h"
#include "hardware/ps2.h"

/* PRIVATE VARIABLES **********************************************************/

static BYTE BiosKeyboardMap[256];

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN BiosKbdBufferPush(WORD Data)
{
    /* Get the location of the element after the tail */
    WORD NextElement = Bda->KeybdBufferTail + sizeof(WORD);

    /* Wrap it around if it's at or beyond the end */
    if (NextElement >= Bda->KeybdBufferEnd) NextElement = Bda->KeybdBufferStart;

    /* If it's full, fail */
    if (NextElement == Bda->KeybdBufferHead)
    {
        DPRINT1("BIOS keyboard buffer full.\n");
        return FALSE;
    }

    /* Put the value in the queue */
    *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferTail)) = Data;
    Bda->KeybdBufferTail = NextElement;

    /* Return success */
    return TRUE;
}

static BOOLEAN BiosKbdBufferTop(LPWORD Data)
{
    /* If it's empty, fail */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return FALSE;

    /* Otherwise, get the value and return success */
    *Data = *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferHead));

    return TRUE;
}

static BOOLEAN BiosKbdBufferPop(VOID)
{
    /* If it's empty, fail */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return FALSE;

    /* Remove the value from the queue */
    Bda->KeybdBufferHead += sizeof(WORD);

    /* Check if we are at, or have passed, the end of the buffer */
    if (Bda->KeybdBufferHead >= Bda->KeybdBufferEnd)
    {
        /* Return it to the beginning */
        Bda->KeybdBufferHead = Bda->KeybdBufferStart;
    }

    /* Return success */
    return TRUE;
}

static WORD BiosPeekCharacter(VOID)
{
    WORD CharacterData = 0;

    /* Get the key from the queue, but don't remove it */
    if (BiosKbdBufferTop(&CharacterData)) return CharacterData;
    else return 0xFFFF;
}

static WORD BiosGetCharacter(VOID)
{
    WORD CharacterData = 0;

    /* Check if there is a key available, and if so, remove it from the queue */
    if (BiosKbdBufferTop(&CharacterData)) BiosKbdBufferPop();
    else CharacterData = 0xFFFF;

    return CharacterData;
}

static VOID WINAPI BiosKeyboardService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Wait for keystroke and read */
        case 0x00:
        /* Wait for extended keystroke and read */
        case 0x10:  // FIXME: Temporarily do the same as INT 16h, 00h
        {
            /* Read the character (and wait if necessary) */
            WORD Character = BiosGetCharacter();

            if (Character == 0xFFFF)
            {
                /* No key available. Set the handler CF to repeat the BOP */
                setCF(1);
                break;
            }

            setAX(Character);

            break;
        }

        /* Get keystroke status */
        case 0x01:
        /* Get extended keystroke status */
        case 0x11:  // FIXME: Temporarily do the same as INT 16h, 01h
        {
            WORD Character = BiosPeekCharacter();

            if (Character != 0xFFFF)
            {
                /* There is a character, clear ZF and return it */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;
                setAX(Character);
            }
            else
            {
                /* No character, set ZF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_ZF;
            }

            break;
        }

        /* Get shift status */
        case 0x02:
        {
            /* Return the lower byte of the keyboard shift status word */
            setAL(LOBYTE(Bda->KeybdShiftFlags));
            break;
        }

        /* Reserved */
        case 0x04:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x04 is RESERVED\n");
            break;
        }

        /* Push keystroke */
        case 0x05:
        {
            /* Return 0 if success, 1 if failure */
            setAL(BiosKbdBufferPush(getCX()) == FALSE);
            break;
        }

        /* Get extended shift status */
        case 0x12:
        {
            /*
             * Be careful! The returned word is similar to Bda->KeybdShiftFlags
             * but the high byte is organized differently:
             * the bytes 2 and 3 of the high byte are not the same...
             */
            WORD KeybdShiftFlags = (Bda->KeybdShiftFlags & 0xF3FF);

            /* Return the extended keyboard shift status word */
            setAX(KeybdShiftFlags);
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

// Keyboard IRQ 1
static VOID WINAPI BiosKeyboardIrq(LPWORD Stack)
{
    BYTE ScanCode, VirtualKey;
    WORD Character;

    /* Get the scan code and virtual key code */
    ScanCode   = IOReadB(PS2_DATA_PORT);
    VirtualKey = MapVirtualKey(ScanCode & 0x7F, MAPVK_VSC_TO_VK);

    /* Check if this is a key press or release */
    if (!(ScanCode & (1 << 7)))
    {
        /* Key press */
        if (VirtualKey == VK_NUMLOCK ||
            VirtualKey == VK_CAPITAL ||
            VirtualKey == VK_SCROLL  ||
            VirtualKey == VK_INSERT)
        {
            /* For toggle keys, toggle the lowest bit in the keyboard map */
            BiosKeyboardMap[VirtualKey] ^= ~(1 << 0);
        }

        /* Set the highest bit */
        BiosKeyboardMap[VirtualKey] |= (1 << 7);

        /* Find out which character this is */
        Character = 0;
        if (ToAscii(VirtualKey, ScanCode, BiosKeyboardMap, &Character, 0) == 0)
        {
            /* Not ASCII */
            Character = 0;
        }

        /* Push it onto the BIOS keyboard queue */
        BiosKbdBufferPush(MAKEWORD(Character, ScanCode));
    }
    else
    {
        /* Key release, unset the highest bit */
        BiosKeyboardMap[VirtualKey] &= ~(1 << 7);
    }

    /* Clear the keyboard flags */
    Bda->KeybdShiftFlags = 0;

    /* Set the appropriate flags based on the state */
    if (BiosKeyboardMap[VK_RSHIFT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RSHIFT;
    if (BiosKeyboardMap[VK_LSHIFT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LSHIFT;
    if (BiosKeyboardMap[VK_CONTROL]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CTRL;
    if (BiosKeyboardMap[VK_MENU]     & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_ALT;
    if (BiosKeyboardMap[VK_SCROLL]   & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL_ON;
    if (BiosKeyboardMap[VK_NUMLOCK]  & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK_ON;
    if (BiosKeyboardMap[VK_CAPITAL]  & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK_ON;
    if (BiosKeyboardMap[VK_INSERT]   & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT_ON;
    if (BiosKeyboardMap[VK_RMENU]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RALT;
    if (BiosKeyboardMap[VK_LMENU]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LALT;
    if (BiosKeyboardMap[VK_SNAPSHOT] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SYSRQ;
    if (BiosKeyboardMap[VK_PAUSE]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_PAUSE;
    if (BiosKeyboardMap[VK_SCROLL]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL;
    if (BiosKeyboardMap[VK_NUMLOCK]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK;
    if (BiosKeyboardMap[VK_CAPITAL]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK;
    if (BiosKeyboardMap[VK_INSERT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT;

    PicIRQComplete(Stack);
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN KbdBios32Initialize(VOID)
{
    /* Initialize the common Keyboard BIOS Support Library */
    if (!KbdBiosInitialize()) return FALSE;

    /* Initialize the BDA */
    Bda->KeybdBufferStart = FIELD_OFFSET(BIOS_DATA_AREA, KeybdBuffer);
    Bda->KeybdBufferEnd   = Bda->KeybdBufferStart + BIOS_KBD_BUFFER_SIZE * sizeof(WORD);
    Bda->KeybdBufferHead  = Bda->KeybdBufferTail = Bda->KeybdBufferStart;

    // FIXME: Fill the keyboard buffer with invalid values, for diagnostic purposes...
    RtlFillMemory(((LPVOID)((ULONG_PTR)Bda + Bda->KeybdBufferStart)), BIOS_KBD_BUFFER_SIZE * sizeof(WORD), 'A');

    /* Register the BIOS 32-bit Interrupts */

    /* Initialize software vector handlers */
    RegisterBiosInt32(BIOS_KBD_INTERRUPT, BiosKeyboardService);

    /* Set up the HW vector interrupts */
    EnableHwIRQ(1, BiosKeyboardIrq);

    return TRUE;
}

VOID KbdBios32Cleanup(VOID)
{
    /* Cleanup the common Keyboard BIOS Support Library */
    KbdBiosCleanup();
}

/* EOF */
