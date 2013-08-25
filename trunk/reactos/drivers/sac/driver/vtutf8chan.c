/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/vtutf8chan.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "sacdrv.h"

/* GLOBALS *******************************************************************/

CHAR IncomingUtf8ConversionBuffer[4];
WCHAR IncomingUnicodeValue;

SAC_STATIC_ESCAPE_STRING SacStaticEscapeStrings [] =
{
    { VT_ANSI_CURSOR_UP, 2, SacCursorUp },
    { VT_ANSI_CURSOR_DOWN, 2, SacCursorDown },
    { VT_ANSI_CURSOR_RIGHT, 2, SacCursorRight },
    { VT_ANSI_CURSOR_LEFT, 2, SacCursorLeft },
    { VT_220_BACKTAB, 3, SacBackTab },
    { VT_ANSI_ERASE_END_LINE, 2, SacEraseEndOfLine },
    { VT_ANSI_ERASE_START_LINE, 3, SacEraseStartOfLine },
    { VT_ANSI_ERASE_ENTIRE_LINE, 3, SacEraseLine },
    { VT_ANSI_ERASE_DOWN_SCREEN, 2, SacEraseEndOfScreen },
    { VT_ANSI_ERASE_UP_SCREEN, 3, SacEraseStartOfScreen },
    { VT_ANSI_ERASE_ENTIRE_SCREEN, 3, SacEraseScreen },
};

/* FUNCTIONS *****************************************************************/

FORCEINLINE
VOID
VTUTF8ChannelAssertCursor(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel->CursorRow < SAC_VTUTF8_ROW_HEIGHT);
    ASSERT(Channel->CursorCol < SAC_VTUTF8_COL_WIDTH);
}

BOOLEAN
NTAPI
VTUTF8ChannelScanForNumber(IN PWCHAR String,
                           OUT PULONG Number)
{
    /* If the first character is invalid, fail early */
    if ((*String < L'0') || (*String > L'9')) return FALSE;

    /* Otherwise, initialize the output and loop the string */
    *Number = 0;
    while ((*String >= L'0') && (*String <= L'9'))
    {
        /* Save the first decimal */
        *Number = 10 * *Number;

        /* Compute and add the second one */
        *Number += *++String - L'0';
    }

    /* All done */
    return TRUE;
}

NTSTATUS
NTAPI
VTUTF8ChannelAnsiDispatch(
	IN NTSTATUS Status,
	IN ULONG AnsiCode,
	IN PWCHAR Data,
	IN ULONG Length
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelProcessAttributes(
	IN PSAC_CHANNEL Channel,
	IN UCHAR Attribute
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

//
// This function is the guts of the sequences that SAC supports.
//
// It is written to conform to the way that Microsoft's SAC driver interprets
// the ANSI standard. If you want to extend and/or "fix" it, please use a flag
// that can be set in the Registry to enable "extended" ANSI support or etc...
//
// Hermes, I'm looking at you, buddy.
//
ULONG
NTAPI
VTUTF8ChannelConsumeEscapeSequence(IN PSAC_CHANNEL Channel,
                                   IN PWCHAR String)
{
    ULONG Number, Number2, Number3, i, Action, Result;
    PWCHAR Sequence;
    PSAC_CURSOR_DATA Cursor;
    ASSERT(String[0] == VT_ANSI_ESCAPE);

    /* Microsoft's driver does this after the O(n) check below. Be smarter. */
    if (String[1] != VT_ANSI_COMMAND) return 0;

    /* Now that we know it's a valid command, look through the common cases */
    for (i = 0; i < RTL_NUMBER_OF(SacStaticEscapeStrings); i++)
    {
        /* Check if an optimized sequence was detected */
        if (!wcsncmp(String + 1,
                     SacStaticEscapeStrings[i].Sequence,
                     SacStaticEscapeStrings[i].Size))
        {
            /* Yep, return the right action, length, and set optionals to 1 */
            Action = SacStaticEscapeStrings[i].Action;
            Result = SacStaticEscapeStrings[i].Size + 1;
            Number = Number2 = Number3 = 1;
            goto ProcessString;
        }
    }

    /* It's a more complex sequence, start parsing it */
    Result = 0;
    Sequence = String + 2;

    /* First, check for the cursor sequences. This is useless due to above. */
    switch (*Sequence)
    {
        case VT_ANSI_CURSOR_UP_CHAR:
            Action = SacCursorUp;
            goto ProcessString;

        case VT_ANSI_CURSOR_DOWN_CHAR:
            Action = SacCursorDown;
            goto ProcessString;

        case VT_ANSI_CURSOR_RIGHT_CHAR:
            Action = SacCursorLeft; //bug
            goto ProcessString;

        case VT_ANSI_CURSOR_LEFT_CHAR:
            Action = SacCursorRight; //bug
            goto ProcessString;

        case VT_ANSI_ERASE_LINE_CHAR:
            Action = SacEraseEndOfLine;
            goto ProcessString;

        default:
            break;
    }

    /* This must be a sequence starting with ESC[# */
    if (!VTUTF8ChannelScanForNumber(Sequence, &Number)) return 0;
    while ((*Sequence >= L'0') && (*Sequence <= L'9')) Sequence++;

    /* Check if this is ESC[#m */
    if (*Sequence == VT_ANSI_SET_ATTRIBUTE_CHAR)
    {
        /* Some attribute is being set, go over the ones we support */
        switch (Number)
        {
            /* Make the font standard */
            case Normal:
                Action = SacFontNormal;
                break;

            /* Make the font bold */
            case Bold:
                Action = SacFontBold;
                break;

            /* Make the font blink */
            case SlowBlink:
                Action = SacFontBlink;
                break;

            /* Make the font colors inverted */
            case Inverse:
                Action = SacFontInverse;
                break;

            /* Make the font standard intensity */
            case BoldOff:
                Action = SacFontBoldOff;
                break;

            /* Turn off blinking */
            case BlinkOff:
                Action = SacFontBlinkOff;
                break;

            /* Turn off inverted colors */
            case InverseOff:
                Action = SacFontInverseOff;
                break;

            /* Something else... */
            default:

                /* Is a background color being set? */
                if ((Number < SetBackColorStart) || (Number > SetBackColorMax))
                {
                    /* Nope... is it the foreground color? */
                    if ((Number < SetColorStart) || (Number > SetColorMax))
                    {
                        /* Nope. SAC expects no other attributes so bail out */
                        ASSERT(FALSE);
                        return 0;
                    }

                    /* Yep -- the number will tell us which */
                    Action = SacSetFontColor;
                }
                else
                {
                    /* Yep -- the number will tell us which */
                    Action = SacSetBackgroundColor;
                }
                break;
        }

        /* In all cases, we're done here */
        goto ProcessString;
    }

    /* The only allowed possibility now is ESC[#;# */
    if (*Sequence != VT_ANSI_SEPARATOR_CHAR) return 0;
    Sequence++;
    if (!VTUTF8ChannelScanForNumber(Sequence, &Number2)) return 0;
    while ((*Sequence >= L'0') && (*Sequence <= L'9')) Sequence++;

    /* There's two valid forms accepted at this point: ESC[#;#m and ESC[#;#H */
    switch (*Sequence)
    {
        /* This is ESC[#;#m -- used to set both colors at once */
        case VT_ANSI_SET_ATTRIBUTE_CHAR:
            Action = SacSetColors;
            goto ProcessString;

        /* This is ESC[#;#H -- used to set the cursor position */
        case VT_ANSI_CUP_CURSOR_CHAR:
            Action = SacSetCursorPosition;
            goto ProcessString;

        /* Finally, this *could* be ESC[#;#; -- we'll keep parsing */
        case VT_ANSI_SEPARATOR_CHAR:
            Sequence++;
            break;

        /* Abandon anything else */
        default:
            return 0;
    }

    /* The SAC seems to accept a few more possibilities if a ';' follows... */
    switch (*Sequence)
    {
        /* Both ESC[#;#;H and ESC[#;#;f are really the same command */
        case VT_ANSI_CUP_CURSOR_CHAR:
        case VT_ANSI_HVP_CURSOR_CHAR:
            /* It's unclear why MS doesn't allow the HVP sequence on its own */
            Action = SacSetCursorPosition;
            goto ProcessString;

        /* And this is the ESC[#;#;r command to set the scroll region... */
        case VT_ANSI_SCROLL_CHAR:
            /* Again, not clear why ESC[#;#r isn't supported */
            Action = SacSetScrollRegion;
            goto ProcessString;

        /* Anything else must be ESC[#;#;# */
        default:
            break;
    }

    /* Get the last "#" */
    if (!VTUTF8ChannelScanForNumber(Sequence, &Number3)) return 0;
    while ((*Sequence >= L'0') && (*Sequence <= L'9')) Sequence++;

    /* And now the only remaining possibility is ESC[#;#;#;m */
    if (*Sequence == VT_ANSI_SET_ATTRIBUTE_CHAR)
    {
        /* Which sets both color and attributes in one command */
        Action = SacSetColorsAndAttributes;
        goto ProcessString;
    }

    /* No other sequences supported */
    return 0;

ProcessString:
    /* Unless we got here from the optimized path, calculate the length */
    if (!Result) Result = Sequence - String + 1;

    /* Get the current cell buffer */
    Cursor = (PSAC_CURSOR_DATA)Channel->OBuffer;
    VTUTF8ChannelAssertCursor(Channel);

    /* Handle all the supported SAC ANSI commands */
    switch (Action)
    {
        case SacCursorUp:
            if (Channel->CursorRow < Number)
            {
                Channel->CursorRow = 0;
            }
            else
            {
                Channel->CursorRow -= Number;
            }
            VTUTF8ChannelAssertCursor(Channel);
            break;

        case SacCursorDown:
            if (Channel->CursorRow >= SAC_VTUTF8_ROW_HEIGHT)
            {
                Channel->CursorRow = SAC_VTUTF8_ROW_HEIGHT;
            }
            else
            {
                Channel->CursorRow += Number;
            }
            VTUTF8ChannelAssertCursor(Channel);
            break;

        case SacCursorLeft:
            if (Channel->CursorCol < Number)
            {
                Channel->CursorCol = 0;
            }
            else
            {
                Channel->CursorCol -= Number;
            }
            VTUTF8ChannelAssertCursor(Channel);
            break;

        case SacCursorRight:
            if (Channel->CursorCol >= SAC_VTUTF8_COL_WIDTH)
            {
                Channel->CursorCol = SAC_VTUTF8_COL_WIDTH;
            }
            else
            {
                Channel->CursorCol += Number;
            }
            VTUTF8ChannelAssertCursor(Channel);
            break;

        case SacFontNormal:
            Channel->CursorFlags = 0;
            Channel->CursorBackColor = SetBackColorBlack;
            Channel->CursorColor = SetColorWhite;
            break;

        case SacFontBlink:
            Channel->CursorFlags |= SAC_CURSOR_FLAG_BLINK;
            break;

        case SacFontBlinkOff:
            Channel->CursorFlags &= ~SAC_CURSOR_FLAG_BLINK;
            break;

        case SacFontBold:
            Channel->CursorFlags |= SAC_CURSOR_FLAG_BOLD;
            break;

        case SacFontBoldOff:
            Channel->CursorFlags &= ~SAC_CURSOR_FLAG_BOLD;
            break;

        case SacFontInverse:
            Channel->CursorFlags |= SAC_CURSOR_FLAG_INVERTED;
            break;

        case SacFontInverseOff:
            Channel->CursorFlags &= ~SAC_CURSOR_FLAG_INVERTED;
            break;

        case SacEraseEndOfLine:
            for (i = Channel->CursorCol; i < SAC_VTUTF8_COL_WIDTH; i++)
            {
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorFlags = Channel->CursorFlags;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorBackColor = Channel->CursorColor;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorColor = Channel->CursorBackColor;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorValue = ' ';
            }
            break;

        case SacEraseStartOfLine:
            for (i = 0; i < (Channel->CursorCol + 1); i++)
            {
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorFlags = Channel->CursorFlags;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorBackColor = Channel->CursorColor;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorColor = Channel->CursorBackColor;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorValue = ' ';
            }
            break;

        case SacEraseLine:
            for (i = 0; i < SAC_VTUTF8_COL_WIDTH; i++)
            {
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorFlags = Channel->CursorFlags;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorBackColor = Channel->CursorColor;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorColor = Channel->CursorBackColor;
                Cursor[(Channel->CursorRow * SAC_VTUTF8_COL_WIDTH) +
                       (i * SAC_VTUTF8_ROW_HEIGHT)].CursorValue = ' ';
            }
            break;

        case SacEraseEndOfScreen:
            ASSERT(FALSE); // todo
            break;

        case SacEraseStartOfScreen:
            ASSERT(FALSE); // todo
            break;

        case SacEraseScreen:
            ASSERT(FALSE); // todo
            break;

        case SacSetCursorPosition:
            ASSERT(FALSE); // todo
            break;

        case SacSetScrollRegion:
            ASSERT(FALSE); // todo
            break;

        case SacSetColors:
            Channel->CursorColor = Number;
            Channel->CursorBackColor = Number2;
            break;

        case SacSetBackgroundColor:
            Channel->CursorBackColor = Number;
            break;

        case SacSetFontColor:
            Channel->CursorColor = Number;
            break;

        case SacSetColorsAndAttributes:
            Channel->CursorFlags = Number;
            Channel->CursorColor = Number2;
            Channel->CursorBackColor = Number3;
            break;
        default:
            break;
    }

    /* Return the length of the sequence */
    return Result;
}

NTSTATUS
NTAPI
VTUTF8ChannelOInit(IN PSAC_CHANNEL Channel)
{
    PSAC_CURSOR_DATA Cursor;
    ULONG x, y;
    CHECK_PARAMETER(Channel);

    /* Set the current channel cursor parameters */
    Channel->CursorFlags = 0;
    Channel->CursorBackColor = SetBackColorBlack;
    Channel->CursorColor = SetColorWhite;

    /* Loop the output buffer height by width */
    Cursor = (PSAC_CURSOR_DATA)Channel->OBuffer;
    y = SAC_VTUTF8_ROW_HEIGHT;
    do
    {
        x = SAC_VTUTF8_COL_WIDTH;
        do
        {
            /* For every character, set the defaults */
            Cursor->CursorValue = ' ';
            Cursor->CursorBackColor = SetBackColorBlack;
            Cursor->CursorColor = SetColorWhite;

            /* Move to the next character */
            Cursor++;
        } while (--x);
    } while (--y);

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelCreate(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;
    CHECK_PARAMETER(Channel);

    /* Allocate the output buffer */
    Channel->OBuffer = SacAllocatePool(SAC_VTUTF8_OBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->OBuffer);

    /* Allocate the input buffer */
    Channel->IBuffer = SacAllocatePool(SAC_VTUTF8_IBUFFER_SIZE, GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(Channel->IBuffer);

    /* Initialize the output stream */
    Status = VTUTF8ChannelOInit(Channel);
    if (NT_SUCCESS(Status)) return Status;

    /* Reset all flags and return success */
    _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 0);
    _InterlockedExchange(&Channel->ChannelHasNewIBufferData, 0);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelDestroy(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER(Channel);

    /* Free the buffer and then destroy the channel */
    if (Channel->OBuffer) SacFreePool(Channel->OBuffer);
    if (Channel->IBuffer) SacFreePool(Channel->IBuffer);
    return ChannelDestroy(Channel);
}

NTSTATUS
NTAPI
VTUTF8ChannelORead(IN PSAC_CHANNEL Channel,
                   IN PCHAR Buffer,
                   IN ULONG BufferSize,
                   OUT PULONG ByteCount)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelOFlush(
	IN PSAC_CHANNEL Channel
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelOWrite2(IN PSAC_CHANNEL Channel,
                     IN PCHAR String,
                     IN ULONG Size)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
VTUTF8ChannelOEcho(IN PSAC_CHANNEL Channel,
                   IN PCHAR String,
                   IN ULONG Size)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pwch;
    ULONG i, k, TranslatedCount, UTF8TranslationSize;
    BOOLEAN Result;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);

    /* Return success if there's nothing to echo */
    if (!(Size / sizeof(WCHAR))) return Status;

    /* Start with the input string*/
    pwch = (PWCHAR)String;

    /* First, figure out how much is outside of the block length alignment */
    k = (Size / sizeof(WCHAR)) % MAX_UTF8_ENCODE_BLOCK_LENGTH;
    if (k)
    {
        /* Translate the misaligned portion */
        Result = SacTranslateUnicodeToUtf8(pwch,
                                           k,
                                           Utf8ConversionBuffer,
                                           Utf8ConversionBufferSize,
                                           &UTF8TranslationSize,
                                           &TranslatedCount);
        ASSERT(k == TranslatedCount);
        if (!Result)
        {
            /* If we couldn't translate, write failure to break out below */
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            /* Write the misaligned portion into the buffer */
            Status = ConMgrWriteData(Channel,
                                     Utf8ConversionBuffer,
                                     UTF8TranslationSize);
        }

        /* If translation or write failed, bail out */
        if (!NT_SUCCESS(Status)) goto Return;
    }

    /* Push the string to its new location (this could be a noop if aligned) */
    pwch += k;

    /* Now figure out how many aligned blocks we have, and loop each one */
    k = (Size / sizeof(WCHAR)) / MAX_UTF8_ENCODE_BLOCK_LENGTH;
    for (i = 0; i < k; i++)
    {
        /* Translate the aligned block */
        Result = SacTranslateUnicodeToUtf8(pwch,
                                           MAX_UTF8_ENCODE_BLOCK_LENGTH,
                                           Utf8ConversionBuffer,
                                           Utf8ConversionBufferSize,
                                           &UTF8TranslationSize,
                                           &TranslatedCount);
        ASSERT(MAX_UTF8_ENCODE_BLOCK_LENGTH == TranslatedCount);
        ASSERT(UTF8TranslationSize > 0);
        if (!Result)
        {
            /* Set failure here, we'll break out below */
            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            /* Move the string location to the next aligned block */
            pwch += MAX_UTF8_ENCODE_BLOCK_LENGTH;

            /* Write the aligned block into the buffer */
            Status = ConMgrWriteData(Channel,
                                     Utf8ConversionBuffer,
                                     UTF8TranslationSize);
        }

        /* If translation or write failed, bail out */
        if (!NT_SUCCESS(Status)) break;
    }

Return:
    ASSERT(pwch == (PWSTR)(String + Size));
    if (NT_SUCCESS(Status)) Status = ConMgrFlushData(Channel);
    return Status;
}

NTSTATUS
NTAPI
VTUTF8ChannelOWrite(IN PSAC_CHANNEL Channel,
                    IN PCHAR String,
                    IN ULONG Length)
{
    NTSTATUS Status;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(String);

    /* Call the lower level function */
    Status = VTUTF8ChannelOWrite2(Channel, String, Length / sizeof(WCHAR));
    if (NT_SUCCESS(Status))
    {
        /* Is the channel enabled for output? */
        if ((ConMgrIsWriteEnabled(Channel)) && (Channel->WriteEnabled))
        {
            /* Go ahead and output it */
            Status = VTUTF8ChannelOEcho(Channel, String, Length);
        }
        else
        {
            /* Otherwise, just remember that we have new data */
            _InterlockedExchange(&Channel->ChannelHasNewOBufferData, 1);
        }
    }

    /* We're done */
    return Status;
}

ULONG
NTAPI
VTUTF8ChannelGetIBufferIndex(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);
    ASSERT((Channel->IBufferIndex % sizeof(WCHAR)) == 0);
    ASSERT(Channel->IBufferIndex < SAC_VTUTF8_IBUFFER_SIZE);

    /* Return the current buffer index */
    return Channel->IBufferIndex;
}

VOID
NTAPI
VTUTF8ChannelSetIBufferIndex(IN PSAC_CHANNEL Channel,
                             IN ULONG BufferIndex)
{
    NTSTATUS Status;
    ASSERT(Channel);
    ASSERT((Channel->IBufferIndex % sizeof(WCHAR)) == 0);
    ASSERT(Channel->IBufferIndex < SAC_VTUTF8_IBUFFER_SIZE);

    /* Set the new index, and if it's not zero, it means we have data */
    Channel->IBufferIndex = BufferIndex;
    _InterlockedExchange(&Channel->ChannelHasNewIBufferData, BufferIndex != 0);

    /* If we have new data, and an event has been registered... */
    if (!(Channel->IBufferIndex) &&
        (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT))
    {
        /* Go ahead and signal it */
        ChannelClearEvent(Channel, HasNewDataEvent);
        UNREFERENCED_PARAMETER(Status);
    }
}

NTSTATUS
NTAPI
VTUTF8ChannelIRead(IN PSAC_CHANNEL Channel,
                   IN PCHAR Buffer,
                   IN ULONG BufferSize,
                   IN PULONG ReturnBufferSize)
{
    ULONG CopyChars;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Buffer);
    CHECK_PARAMETER_WITH_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    /* Assume failure */
    *ReturnBufferSize = 0;

    /* Check how many bytes are in the buffer */
    if (Channel->ChannelInputBufferLength(Channel) == 0)
    {
        /* Apparently nothing. Make sure the flag indicates so too */
        ASSERT(ChannelHasNewIBufferData(Channel) == FALSE);
    }
    else
    {
        /* Use the smallest number of bytes either in the buffer or requested */
        CopyChars = min(Channel->ChannelInputBufferLength(Channel) * sizeof(WCHAR),
                        BufferSize);
        ASSERT(CopyChars <= Channel->ChannelInputBufferLength(Channel));

        /* Copy them into the caller's buffer */
        RtlCopyMemory(Buffer, Channel->IBuffer, CopyChars);

        /* Update the channel's index past the copied (read) bytes */
        VTUTF8ChannelSetIBufferIndex(Channel,
                                     VTUTF8ChannelGetIBufferIndex(Channel) - CopyChars);

        /* Are there still bytes that haven't been read yet? */
        if (Channel->ChannelInputBufferLength(Channel))
        {
            /* Shift them up in the buffer */
            RtlMoveMemory(Channel->IBuffer,
                          &Channel->IBuffer[CopyChars],
                          Channel->ChannelInputBufferLength(Channel) *
                          sizeof(WCHAR));
        }

        /* Return the number of bytes we actually copied */
        *ReturnBufferSize = CopyChars;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
VTUTF8ChannelIBufferIsFull(IN PSAC_CHANNEL Channel,
                           OUT PBOOLEAN BufferStatus)
{
    CHECK_PARAMETER1(Channel);

    /* If the index is beyond the length, the buffer must be full */
    *BufferStatus = VTUTF8ChannelGetIBufferIndex(Channel) > SAC_VTUTF8_IBUFFER_SIZE;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
VTUTF8ChannelIBufferLength(IN PSAC_CHANNEL Channel)
{
    ASSERT(Channel);

    /* The index is the length, so divide by two to get character count */
    return VTUTF8ChannelGetIBufferIndex(Channel) / sizeof(WCHAR);
}

WCHAR
NTAPI
VTUTF8ChannelIReadLast(IN PSAC_CHANNEL Channel)
{
    PWCHAR LastCharLocation;
    WCHAR LastChar = 0;
    ASSERT(Channel);

    /* Check if there's anything to read in the buffer */
    if (Channel->ChannelInputBufferLength(Channel))
    {
        /* Go back one character */
        VTUTF8ChannelSetIBufferIndex(Channel,
                                     VTUTF8ChannelGetIBufferIndex(Channel) -
                                     sizeof(WCHAR));

        /* Read it, and clear its current value */
        LastCharLocation = (PWCHAR)&Channel->IBuffer[VTUTF8ChannelGetIBufferIndex(Channel)];
        LastChar = *LastCharLocation;
        *LastCharLocation = UNICODE_NULL;
    }

    /* Return the last character */
    return LastChar;
}

NTSTATUS
NTAPI
VTUTF8ChannelIWrite(IN PSAC_CHANNEL Channel,
                    IN PCHAR Buffer,
                    IN ULONG BufferSize)
{
    NTSTATUS Status;
    BOOLEAN IsFull;
    ULONG Index, i;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Buffer);
    CHECK_PARAMETER_WITH_STATUS(BufferSize > 0, STATUS_INVALID_BUFFER_SIZE);

    /* First, check if the input buffer still has space */
    Status = VTUTF8ChannelIBufferIsFull(Channel, &IsFull);
    if (!NT_SUCCESS(Status)) return Status;
    if (IsFull) return STATUS_UNSUCCESSFUL;

    /* Get the current buffer index */
    Index = VTUTF8ChannelGetIBufferIndex(Channel);
    if ((SAC_VTUTF8_IBUFFER_SIZE - Index) < BufferSize)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy the new data */
    for (i = 0; i < BufferSize; i++)
    {
        /* Convert the character */
        if (SacTranslateUtf8ToUnicode(Buffer[i],
                                      IncomingUtf8ConversionBuffer,
                                      &IncomingUnicodeValue))
        {
            /* Write it into the buffer */
            *(PWCHAR)&Channel->IBuffer[VTUTF8ChannelGetIBufferIndex(Channel)] =
                IncomingUnicodeValue;

            /* Update the index */
            Index = VTUTF8ChannelGetIBufferIndex(Channel);
            VTUTF8ChannelSetIBufferIndex(Channel, Index + sizeof(WCHAR));
        }
    }

    /* Signal the event, if one was set */
    if (Channel->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT)
    {
        ChannelSetEvent(Channel, HasNewDataEvent);
    }

    /* All done */
    return STATUS_SUCCESS;
}
