/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/beep.c
 * PURPOSE:         Speaker function (it's only one)
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 31/01/99
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>


/* CONSTANTS *****************************************************************/

#define TIMER2     0x42
#define TIMER3     0x43
#define PORT_B     0x61
#define CLOCKFREQ  1193167


/* FUNCTIONS *****************************************************************/
/*
 * FUNCTION: Beeps the speaker.
 * ARGUMENTS:
 *       Frequency = If 0, the speaker will be switched off, otherwise
 *                   the speaker beeps with the specified frequency.
 */

BOOLEAN
STDCALL
HalMakeBeep (
	ULONG	Frequency
	)
{
   UCHAR b;
   
    /* save flags and disable interrupts */
#if defined(__GNUC__)
    __asm__("pushf\n\t" \
            "cli\n\t");
#elif defined(_MSC_VER)
    __asm	pushfd
    __asm	cli
#else
#error Unknown compiler for inline assembler
#endif

    /* speaker off */
    b = READ_PORT_UCHAR((PUCHAR)PORT_B);
    WRITE_PORT_UCHAR((PUCHAR)PORT_B, (UCHAR)(b & 0xFC));

    if (Frequency)
    {
        DWORD Divider = CLOCKFREQ / Frequency;

        if (Divider > 0x10000)
        {
            /* restore flags */
#if defined(__GNUC__)
            __asm__("popf\n\t");
#elif defined(_MSC_VER)
            __asm	popfd
#else
#error Unknown compiler for inline assembler
#endif

            return FALSE;
        }

        /* set timer divider */
        WRITE_PORT_UCHAR((PUCHAR)TIMER3, 0xB6);
        WRITE_PORT_UCHAR((PUCHAR)TIMER2, (UCHAR)(Divider & 0xFF));
        WRITE_PORT_UCHAR((PUCHAR)TIMER2, (UCHAR)((Divider>>8) & 0xFF));

        /* speaker on */
        WRITE_PORT_UCHAR((PUCHAR)PORT_B, (UCHAR)(READ_PORT_UCHAR((PUCHAR)PORT_B) | 0x03));
    }

    /* restore flags */
#if defined(__GNUC__)
    __asm__("popf\n\t");
#elif defined(_MSC_VER)
    __asm	popfd
#else
#error Unknown compiler for inline assembler
#endif

    return TRUE;
}

