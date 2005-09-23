/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Unicode Conversion Routines
 * FILE:              lib/rtl/unicode.c
 * PROGRAMMER:        Alex Ionescu (alex@relsoft.net)
 *                    Emanuele Aliberti
 *                    Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern BOOLEAN NlsMbCodePageTag;
extern BOOLEAN NlsMbOemCodePageTag;
extern PUSHORT NlsLeadByteInfo;

/* FUNCTIONS *****************************************************************/

/*
* @implemented
*/
WCHAR
STDCALL
RtlAnsiCharToUnicodeChar(IN PUCHAR *AnsiChar)
{
    ULONG Size;
    NTSTATUS Status;
    WCHAR UnicodeChar = 0x20;

    Size = (NlsLeadByteInfo[**AnsiChar] == 0) ? 1 : 2;

    Status = RtlMultiByteToUnicodeN(&UnicodeChar,
                                    sizeof(WCHAR),
                                    NULL,
                                    *AnsiChar,
                                    Size);

    if (!NT_SUCCESS(Status))
    {
        UnicodeChar = 0x20;
    }

    *AnsiChar += Size;
    return UnicodeChar;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  If the dest buffer is too small a partial copy is NOT performed!
 */
NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PANSI_STRING AnsiSource,
   IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    Length = RtlAnsiStringToUnicodeSize(AnsiSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;
    UniDest->Length = (USHORT)Length - sizeof(WCHAR);

    if (AllocateDestinationString == TRUE)
    {
        UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
        UniDest->MaximumLength = Length;
        if (!UniDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (Length >= UniDest->MaximumLength)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    Status = RtlMultiByteToUnicodeN(UniDest->Buffer,
                                    UniDest->Length,
                                    &Index,
                                    AnsiSource->Buffer,
                                    AnsiSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
        return Status;
    }

    UniDest->Buffer[Index / sizeof(WCHAR)] = UNICODE_NULL;
    return Status;
}

/*
 * @implemented
 *
 * RETURNS
 *  The calculated size in bytes including nullterm.
 */
ULONG
STDCALL
RtlxAnsiStringToUnicodeSize(IN PCANSI_STRING AnsiString)
{
    ULONG Size;

    /* Convert from Mb String to Unicode Size */
    RtlMultiByteToUnicodeSize(&Size,
                              AnsiString->Buffer,
                              AnsiString->Length);

    /* Return the size plus the null-char */
    return(Size + sizeof(WCHAR));
}

/*
 * @implemented
 *
 * NOTES
 *  If src->length is zero dest is unchanged.
 *  Dest is never nullterminated.
 */
NTSTATUS
STDCALL
RtlAppendStringToString(IN PSTRING Destination,
                        IN PSTRING Source)
{
    USHORT SourceLength = Source->Length;

    if (SourceLength)
    {
        if (Destination->Length + SourceLength > Destination->MaximumLength)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlMoveMemory(&Destination->Buffer[Destination->Length],
                      Source->Buffer,
                      SourceLength);

        Destination->Length += SourceLength;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  If src->length is zero dest is unchanged.
 *  Dest is nullterminated when the MaximumLength allowes it.
 *  When dest fits exactly in MaximumLength characters the nullterm is ommitted.
 */
NTSTATUS
STDCALL
RtlAppendUnicodeStringToString(
   IN OUT PUNICODE_STRING Destination,
   IN PCUNICODE_STRING Source)
{
    USHORT SourceLength = Source->Length;
    PWCHAR Buffer = &Destination->Buffer[Destination->Length / sizeof(WCHAR)];

    if (SourceLength)
    {
        if ((SourceLength+ Destination->Length) > Destination->MaximumLength)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlMoveMemory(Buffer, Source->Buffer, SourceLength);
        Destination->Length += SourceLength;

        /* append terminating '\0' if enough space */
        if (Destination->MaximumLength > Destination->Length)
        {
            Buffer[SourceLength / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }

    return STATUS_SUCCESS;
}

/**************************************************************************
 *      RtlCharToInteger   (NTDLL.@)
 * @implemented
 * Converts a character string into its integer equivalent.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. value contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_ACCESS_VIOLATION, if value is NULL.
 *
 * NOTES
 *  For base 0 it uses 10 as base and the string should be in the format
 *      "{whitespace} [+|-] [0[x|o|b]] {digits}".
 *  For other bases the string should be in the format
 *      "{whitespace} [+|-] {digits}".
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 *  This function does not read garbage behind '\0' as the native version does.
 */
NTSTATUS
STDCALL
RtlCharToInteger(
    PCSZ str,      /* [I] '\0' terminated single-byte string containing a number */
    ULONG base,    /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    PULONG value)  /* [O] Destination for the converted value */
{
    CHAR chCurrent;
    int digit;
    ULONG RunningTotal = 0;
    char bMinus = 0;

    while (*str != '\0' && *str <= ' ') {
	str++;
    } /* while */

    if (*str == '+') {
	str++;
    } else if (*str == '-') {
	bMinus = 1;
	str++;
    } /* if */

    if (base == 0) {
	base = 10;
	if (str[0] == '0') {
	    if (str[1] == 'b') {
		str += 2;
		base = 2;
	    } else if (str[1] == 'o') {
		str += 2;
		base = 8;
	    } else if (str[1] == 'x') {
		str += 2;
		base = 16;
	    } /* if */
	} /* if */
    } else if (base != 2 && base != 8 && base != 10 && base != 16) {
	return STATUS_INVALID_PARAMETER;
    } /* if */

    if (value == NULL) {
	return STATUS_ACCESS_VIOLATION;
    } /* if */

    while (*str != '\0') {
	chCurrent = *str;
	if (chCurrent >= '0' && chCurrent <= '9') {
	    digit = chCurrent - '0';
	} else if (chCurrent >= 'A' && chCurrent <= 'Z') {
	    digit = chCurrent - 'A' + 10;
	} else if (chCurrent >= 'a' && chCurrent <= 'z') {
	    digit = chCurrent - 'a' + 10;
	} else {
	    digit = -1;
	} /* if */
	if (digit < 0 || digit >= (int)base) {
	    *value = bMinus ? -RunningTotal : RunningTotal;
	    return STATUS_SUCCESS;
	} /* if */

	RunningTotal = RunningTotal * base + digit;
	str++;
    } /* while */

    *value = bMinus ? -RunningTotal : RunningTotal;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
LONG
STDCALL
RtlCompareString(
   IN PSTRING s1,
   IN PSTRING s2,
   IN BOOLEAN CaseInsensitive)
{
   unsigned int len;
   LONG ret = 0;
   LPCSTR p1, p2;

   len = min(s1->Length, s2->Length);
   p1 = s1->Buffer;
   p2 = s2->Buffer;

   if (CaseInsensitive)
   {
     while (!ret && len--) ret = RtlUpperChar(*p1++) - RtlUpperChar(*p2++);
   }
   else
   {
     while (!ret && len--) ret = *p1++ - *p2++;
   }
   if (!ret) ret = s1->Length - s2->Length;
   return ret;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if strings are equal.
 */
BOOLEAN
STDCALL
RtlEqualString(
   IN PSTRING s1,
   IN PSTRING s2,
   IN BOOLEAN CaseInsensitive)
{
    if (s1->Length != s2->Length) return FALSE;
    return !RtlCompareString(s1, s2, CaseInsensitive);
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if strings are equal.
 */
BOOLEAN
STDCALL
RtlEqualUnicodeString(
   IN CONST UNICODE_STRING *s1,
   IN CONST UNICODE_STRING *s2,
   IN BOOLEAN  CaseInsensitive)
{
    if (s1->Length != s2->Length) return FALSE;
    return !RtlCompareUnicodeString(s1, s2, CaseInsensitive );
}

/*
 * @implemented
 */
VOID
STDCALL
RtlFreeAnsiString(IN PANSI_STRING AnsiString)
{
    if (AnsiString->Buffer)
    {
        RtlpFreeStringMemory(AnsiString->Buffer, TAG_ASTR);
        RtlZeroMemory(AnsiString, sizeof(ANSI_STRING));
    }
}

/*
 * @implemented
 */
VOID
STDCALL
RtlFreeOemString(IN POEM_STRING OemString)
{
   if (OemString->Buffer) RtlpFreeStringMemory(OemString->Buffer, TAG_OSTR);
}

/*
 * @implemented
 */
VOID
STDCALL
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
    if (UnicodeString->Buffer)
    {
        RtlpFreeStringMemory(UnicodeString->Buffer, TAG_ASTR);
        RtlZeroMemory(UnicodeString, sizeof(UNICODE_STRING));
    }
}

/*
* @unimplemented
*/
BOOLEAN
STDCALL
RtlIsValidOemCharacter(IN PWCHAR Char)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitAnsiString(IN OUT PANSI_STRING DestinationString,
                  IN PCSZ SourceString)
{
    ULONG DestSize;

    if(SourceString)
    {
        DestSize = strlen(SourceString);
        DestinationString->Length = (USHORT)DestSize;
        DestinationString->MaximumLength = (USHORT)DestSize + sizeof(CHAR);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = (PCHAR)SourceString;
}

/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitString(
   IN OUT PSTRING DestinationString,
   IN PCSZ SourceString)
{
    RtlInitAnsiString(DestinationString, SourceString);
}

/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
STDCALL
RtlInitUnicodeString(IN OUT PUNICODE_STRING DestinationString,
                     IN PCWSTR SourceString)
{
    ULONG DestSize;

    if(SourceString)
    {
        DestSize = wcslen(SourceString) * sizeof(WCHAR);
        DestinationString->Length = (USHORT)DestSize;
        DestinationString->MaximumLength = (USHORT)DestSize + sizeof(WCHAR);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = (PWCHAR)SourceString;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlInitUnicodeStringEx(OUT PUNICODE_STRING DestinationString,
                       IN PCWSTR SourceString)
{
    ULONG DestSize;

    if(SourceString)
    {
        DestSize = wcslen(SourceString) * sizeof(WCHAR);
        if (DestSize > 0xFFFC) return STATUS_NAME_TOO_LONG;
        DestinationString->Length = (USHORT)DestSize;
        DestinationString->MaximumLength = (USHORT)DestSize + sizeof(WCHAR);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = (PWCHAR)SourceString;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  Writes at most length characters to the string str.
 *  Str is nullterminated when length allowes it.
 *  When str fits exactly in length characters the nullterm is ommitted.
 */
NTSTATUS
STDCALL
RtlIntegerToChar(
   IN ULONG Value,
   IN ULONG Base,
   IN ULONG Length,
   IN OUT PCHAR String)
{
   ULONG Radix;
   CHAR  temp[33];
   ULONG v = Value;
   ULONG i;
   PCHAR tp;
   PCHAR sp;

   Radix = Base;
   if (Radix == 0)
      Radix = 10;

   if ((Radix != 2) && (Radix != 8) &&
       (Radix != 10) && (Radix != 16))
   {
      return STATUS_INVALID_PARAMETER;
   }

   tp = temp;
   while (v || tp == temp)
   {
      i = v % Radix;
      v = v / Radix;
      if (i < 10)
         *tp = i + '0';
      else
         *tp = i + 'a' - 10;
      tp++;
   }

   if ((ULONG)((ULONG_PTR)tp - (ULONG_PTR)temp) >= Length)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   sp = String;
   while (tp > temp)
      *sp++ = *--tp;
   *sp = 0;

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlIntegerToUnicode(
    IN ULONG Value,
    IN ULONG Base  OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN OUT LPWSTR String
    )
{
   ULONG Radix;
   WCHAR  temp[33];
   ULONG v = Value;
   ULONG i;
   PWCHAR tp;
   PWCHAR sp;

   Radix = Base;
   if (Radix == 0)
      Radix = 10;

   if ((Radix != 2) && (Radix != 8) &&
       (Radix != 10) && (Radix != 16))
   {
      return STATUS_INVALID_PARAMETER;
   }

   tp = temp;
   while (v || tp == temp)
   {
      i = v % Radix;
      v = v / Radix;
      if (i < 10)
         *tp = i + L'0';
      else
         *tp = i + L'a' - 10;
      tp++;
   }

   if ((ULONG)((ULONG_PTR)tp - (ULONG_PTR)temp) >= Length)
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   sp = String;
   while (tp > temp)
      *sp++ = *--tp;
   *sp = 0;

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlIntegerToUnicodeString(
   IN ULONG Value,
   IN ULONG Base OPTIONAL,
   IN OUT PUNICODE_STRING String)
{
    ANSI_STRING AnsiString;
    CHAR Buffer[16];
    NTSTATUS Status;

    Status = RtlIntegerToChar(Value, Base, sizeof(Buffer), Buffer);
    if (NT_SUCCESS(Status))
    {
        AnsiString.Buffer = Buffer;
        AnsiString.Length = (USHORT)strlen(Buffer);
        AnsiString.MaximumLength = sizeof(Buffer);

        Status = RtlAnsiStringToUnicodeString(String, &AnsiString, FALSE);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlInt64ToUnicodeString (
    IN ULONGLONG Value,
    IN ULONG Base OPTIONAL,
    IN OUT PUNICODE_STRING String)
{
    LARGE_INTEGER LargeInt;
    ANSI_STRING AnsiString;
    CHAR Buffer[32];
    NTSTATUS Status;

    LargeInt.QuadPart = Value;

    Status = RtlLargeIntegerToChar(&LargeInt, Base, sizeof(Buffer), Buffer);
    if (NT_SUCCESS(Status))
    {
        AnsiString.Buffer = Buffer;
        AnsiString.Length = (USHORT)strlen(Buffer);
        AnsiString.MaximumLength = sizeof(Buffer);

        Status = RtlAnsiStringToUnicodeString(String, &AnsiString, FALSE);
    }

    return Status;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if String2 contains String1 as a prefix.
 */
BOOLEAN
STDCALL
RtlPrefixString(
   PANSI_STRING String1,
   PANSI_STRING String2,
   BOOLEAN  CaseInsensitive)
{
   PCHAR pc1;
   PCHAR pc2;
   ULONG Length;

   if (String2->Length < String1->Length)
      return FALSE;

   Length = String1->Length;
   pc1 = String1->Buffer;
   pc2 = String2->Buffer;

   if (pc1 && pc2)
   {
      if (CaseInsensitive)
      {
         while (Length--)
         {
            if (RtlUpperChar (*pc1++) != RtlUpperChar (*pc2++))
               return FALSE;
         }
      }
      else
      {
         while (Length--)
         {
            if (*pc1++ != *pc2++)
               return FALSE;
         }
      }
      return TRUE;
   }
   return FALSE;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if String2 contains String1 as a prefix.
 */
BOOLEAN
STDCALL
RtlPrefixUnicodeString(
   PCUNICODE_STRING String1,
   PCUNICODE_STRING String2,
   BOOLEAN  CaseInsensitive)
{
   PWCHAR pc1;
   PWCHAR pc2;
   ULONG Length;

   if (String2->Length < String1->Length)
      return FALSE;

   Length = String1->Length / 2;
   pc1 = String1->Buffer;
   pc2  = String2->Buffer;

   if (pc1 && pc2)
   {
      if (CaseInsensitive)
      {
         while (Length--)
         {
            if (RtlUpcaseUnicodeChar (*pc1++)
                  != RtlUpcaseUnicodeChar (*pc2++))
               return FALSE;
         }
      }
      else
      {
         while (Length--)
         {
            if( *pc1++ != *pc2++ )
               return FALSE;
         }
      }
      return TRUE;
   }
   return FALSE;
}

/**************************************************************************
 *      RtlUnicodeStringToInteger (NTDLL.@)
 * @implemented
 * Converts an unicode string into its integer equivalent.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. value contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_ACCESS_VIOLATION, if value is NULL.
 *
 * NOTES
 *  For base 0 it uses 10 as base and the string should be in the format
 *      "{whitespace} [+|-] [0[x|o|b]] {digits}".
 *  For other bases the string should be in the format
 *      "{whitespace} [+|-] {digits}".
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 *
 *  Note that regardless of success or failure status, we should leave the
 *  partial value in Value.  An error is never returned based on the chars
 *  in the string.
 *
 * DIFFERENCES
 *  This function does not read garbage on string length 0 as the native
 *  version does.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToInteger(
    PCUNICODE_STRING str, /* [I] Unicode string to be converted */
    ULONG base,                /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    PULONG value)              /* [O] Destination for the converted value */
{
    LPWSTR lpwstr = str->Buffer;
    USHORT CharsRemaining = str->Length / sizeof(WCHAR);
    WCHAR wchCurrent;
    int digit;
    ULONG newbase = 0;
    ULONG RunningTotal = 0;
    char bMinus = 0;

    while (CharsRemaining >= 1 && *lpwstr <= L' ') {
	lpwstr++;
	CharsRemaining--;
    } /* while */

    if (CharsRemaining >= 1) {
	if (*lpwstr == L'+') {
	    lpwstr++;
	    CharsRemaining--;
	} else if (*lpwstr == L'-') {
	    bMinus = 1;
	    lpwstr++;
	    CharsRemaining--;
	} /* if */
    } /* if */

    if (CharsRemaining >= 2 && lpwstr[0] == L'0') {
        if (lpwstr[1] == L'b' || lpwstr[1] == L'B') {
	    lpwstr += 2;
	    CharsRemaining -= 2;
	    newbase = 2;
	} else if (lpwstr[1] == L'o' || lpwstr[1] == L'O') {
	    lpwstr += 2;
	    CharsRemaining -= 2;
	    newbase = 8;
        } else if (lpwstr[1] == L'x' || lpwstr[1] == L'X') {
    	    lpwstr += 2;
	    CharsRemaining -= 2;
	    newbase = 16;
	} /* if */
    }
    if (base == 0 && newbase == 0) {
        base = 10;
    } else if (base == 0 && newbase != 0) {
        base = newbase;
    } else if ((newbase != 0 && base != newbase) ||
               (base != 2 && base != 8 && base != 10 && base != 16)) {
	return STATUS_INVALID_PARAMETER;

    } /* if */

    if (value == NULL) {
	return STATUS_ACCESS_VIOLATION;
    } /* if */

    while (CharsRemaining >= 1) {
	wchCurrent = *lpwstr;
	if (wchCurrent >= L'0' && wchCurrent <= L'9') {
	    digit = wchCurrent - L'0';
	} else if (wchCurrent >= L'A' && wchCurrent <= L'Z') {
	    digit = wchCurrent - L'A' + 10;
	} else if (wchCurrent >= L'a' && wchCurrent <= L'z') {
	    digit = wchCurrent - L'a' + 10;
	} else {
	    digit = -1;
	} /* if */
	if (digit < 0 || digit >= (int)base) {
	    *value = bMinus ? -RunningTotal : RunningTotal;
	    return STATUS_SUCCESS;
	} /* if */

	RunningTotal = RunningTotal * base + digit;
	lpwstr++;
	CharsRemaining--;
    } /* while */

    *value = bMinus ? -RunningTotal : RunningTotal;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * RETURNS
 *  Bytes necessary for the conversion including nullterm.
 */
ULONG
STDCALL
RtlxUnicodeStringToOemSize(IN PCUNICODE_STRING UnicodeString)
{
    ULONG Size;

    /* Convert the Unicode String to Mb Size */
    RtlUnicodeToMultiByteSize(&Size,
                              UnicodeString->Buffer,
                              UnicodeString->Length);

    /* Return the size + the null char */
    return (Size + sizeof(CHAR));
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS RealStatus;
    ULONG Length;
    ULONG Index;

    Length = RtlUnicodeStringToAnsiSize(UniSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    AnsiDest->Length = (USHORT)Length - sizeof(CHAR);

    if (AllocateDestinationString)
    {
        AnsiDest->Buffer = RtlpAllocateStringMemory(Length, TAG_ASTR);
        AnsiDest->MaximumLength = Length;
        if (!AnsiDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (AnsiDest->Length >= AnsiDest->MaximumLength)
    {
        if (!AnsiDest->MaximumLength) return STATUS_BUFFER_OVERFLOW;

        Status = STATUS_BUFFER_OVERFLOW;
        AnsiDest->Length = AnsiDest->MaximumLength - 1;
    }

    RealStatus = RtlUnicodeToMultiByteN(AnsiDest->Buffer,
                                        AnsiDest->Length,
                                        &Index,
                                        UniSource->Buffer,
                                        UniSource->Length);

    if (!NT_SUCCESS(RealStatus) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(AnsiDest->Buffer, TAG_ASTR);
        return RealStatus;
    }

    AnsiDest->Buffer[Index] = ANSI_NULL;
    return Status;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  Does NOT perform a partial copy if unicode is too small!
 */
NTSTATUS
STDCALL
RtlOemStringToUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCOEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    Length = RtlOemStringToUnicodeSize(OemSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    UniDest->Length = (USHORT)Length - sizeof(WCHAR);

    if (AllocateDestinationString)
    {
        UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
        UniDest->MaximumLength = Length;
        if (!UniDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (UniDest->Length >= UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlOemToUnicodeN(UniDest->Buffer,
                              UniDest->Length,
                              &Index,
                              OemSource->Buffer,
                              OemSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
        UniDest->Buffer = NULL;
        return Status;
    }

    UniDest->Buffer[Index / sizeof(WCHAR)] = UNICODE_NULL;
    return Status;
}

/*
 * @implemented
 *
 * NOTES
 *   This function always '\0' terminates the string returned.
 */
NTSTATUS
STDCALL
RtlUnicodeStringToOemString(
   IN OUT POEM_STRING OemDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    Length = RtlUnicodeStringToOemSize(UniSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    OemDest->Length = (USHORT)Length - sizeof(CHAR);

    if (AllocateDestinationString)
    {
        OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
        OemDest->MaximumLength = Length;
        if (!OemDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (OemDest->Length >= OemDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUnicodeToOemN(OemDest->Buffer,
                              OemDest->Length,
                              &Index,
                              UniSource->Buffer,
                              UniSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
        OemDest->Buffer = NULL;
        return Status;
    }

    OemDest->Buffer[Index] = ANSI_NULL;
    return Status;
}

#define ITU_IMPLEMENTED_TESTS (IS_TEXT_UNICODE_ODD_LENGTH|IS_TEXT_UNICODE_SIGNATURE)

/*
 * @implemented
 *
 * RETURNS
 *  The length of the string if all tests were passed, 0 otherwise.
 */
ULONG STDCALL
RtlIsTextUnicode (PVOID Buffer,
                  ULONG Length,
                  ULONG *Flags)
{
   PWSTR s = Buffer;
   ULONG in_flags = (ULONG)-1;
   ULONG out_flags = 0;

   if (Length == 0)
      goto done;

   if (Flags != 0)
      in_flags = *Flags;

   /*
    * Apply various tests to the text string. According to the
    * docs, each test "passed" sets the corresponding flag in
    * the output flags. But some of the tests are mutually
    * exclusive, so I don't see how you could pass all tests ...
    */

   /* Check for an odd length ... pass if even. */
   if (!(Length & 1))
      out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

   /* Check for the BOM (byte order mark). */
   if (*s == 0xFEFF)
      out_flags |= IS_TEXT_UNICODE_SIGNATURE;

#if 0
   /* Check for the reverse BOM (byte order mark). */
   if (*s == 0xFFFE)
      out_flags |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;
#endif

   /* FIXME: Add more tests */

   /*
    * Check whether the string passed all of the tests.
    */
   in_flags &= ITU_IMPLEMENTED_TESTS;
   if ((out_flags & in_flags) != in_flags)
      Length = 0;

done:
   if (Flags != 0)
      *Flags = out_flags;

   return Length;
}

/*
 * @implemented
 *
 * NOTES
 *  Same as RtlOemStringToUnicodeString but doesn't write terminating null
 *  A partial copy is NOT performed if the dest buffer is too small!
 */
NTSTATUS
STDCALL
RtlOemStringToCountedUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCOEM_STRING OemSource,
   IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    Length = RtlOemStringToCountedUnicodeSize(OemSource);

    if (!Length)
    {
        RtlZeroMemory(UniDest, sizeof(UNICODE_STRING));
        return STATUS_SUCCESS;
    }

    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    UniDest->Length = (USHORT)Length;

    if (AllocateDestinationString)
    {
        UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
        UniDest->MaximumLength = Length;
        if (!UniDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (UniDest->Length >= UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlOemToUnicodeN(UniDest->Buffer,
                              UniDest->Length,
                              &Index,
                              OemSource->Buffer,
                              OemSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
        UniDest->Buffer = NULL;
        return Status;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if the names are equal, FALSE if not
 *
 * NOTES
 *  The comparison is case insensitive.
 */
BOOLEAN
STDCALL
RtlEqualComputerName(
   IN PUNICODE_STRING ComputerName1,
   IN PUNICODE_STRING ComputerName2)
{
    OEM_STRING OemString1;
    OEM_STRING OemString2;
    BOOLEAN Result = FALSE;

    if (NT_SUCCESS(RtlUpcaseUnicodeStringToOemString(&OemString1,
                                                     ComputerName1,
                                                     TRUE)))
    {
        if (NT_SUCCESS(RtlUpcaseUnicodeStringToOemString(&OemString2,
                                                         ComputerName2,
                                                         TRUE)))
        {
            Result = RtlEqualString(&OemString1, &OemString2, FALSE);
            RtlFreeOemString(&OemString2);
        }
        RtlFreeOemString(&OemString1);
    }

    return Result;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if the names are equal, FALSE if not
 *
 * NOTES
 *  The comparison is case insensitive.
 */
BOOLEAN
STDCALL
RtlEqualDomainName (
   IN PUNICODE_STRING DomainName1,
   IN PUNICODE_STRING DomainName2
)
{
    return RtlEqualComputerName(DomainName1, DomainName2);
}

/*
 * @implemented
 *
 * RIPPED FROM WINE's ntdll\rtlstr.c rev 1.45
 *
 * Convert a string representation of a GUID into a GUID.
 *
 * PARAMS
 *  str  [I] String representation in the format "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"
 *  guid [O] Destination for the converted GUID
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. guid contains the converted value.
 *  Failure: STATUS_INVALID_PARAMETER, if str is not in the expected format.
 *
 * SEE ALSO
 *  See RtlStringFromGUID.
 */
NTSTATUS
STDCALL
RtlGUIDFromString(
   IN UNICODE_STRING *str,
   OUT GUID* guid
)
{
   int i = 0;
   const WCHAR *lpszCLSID = str->Buffer;
   BYTE* lpOut = (BYTE*)guid;

   //TRACE("(%s,%p)\n", debugstr_us(str), guid);

   /* Convert string: {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
    * to memory:       DWORD... WORD WORD BYTES............
    */
   while (i < 37)
   {
      switch (i)
      {
         case 0:
            if (*lpszCLSID != '{')
               return STATUS_INVALID_PARAMETER;
            break;

         case 9:
         case 14:
         case 19:
         case 24:
            if (*lpszCLSID != '-')
               return STATUS_INVALID_PARAMETER;
            break;

         case 37:
            if (*lpszCLSID != '}')
               return STATUS_INVALID_PARAMETER;
            break;

         default:
            {
               WCHAR ch = *lpszCLSID, ch2 = lpszCLSID[1];
               unsigned char byte;

               /* Read two hex digits as a byte value */
               if      (ch >= '0' && ch <= '9')
                  ch = ch - '0';
               else if (ch >= 'a' && ch <= 'f')
                  ch = ch - 'a' + 10;
               else if (ch >= 'A' && ch <= 'F')
                  ch = ch - 'A' + 10;
               else
                  return STATUS_INVALID_PARAMETER;

               if      (ch2 >= '0' && ch2 <= '9')
                  ch2 = ch2 - '0';
               else if (ch2 >= 'a' && ch2 <= 'f')
                  ch2 = ch2 - 'a' + 10;
               else if (ch2 >= 'A' && ch2 <= 'F')
                  ch2 = ch2 - 'A' + 10;
               else
                  return STATUS_INVALID_PARAMETER;

               byte = ch << 4 | ch2;

               switch (i)
               {
#ifndef WORDS_BIGENDIAN
                     /* For Big Endian machines, we store the data such that the
                      * dword/word members can be read as DWORDS and WORDS correctly. */
                     /* Dword */
                  case 1:
                     lpOut[3] = byte;
                     break;
                  case 3:
                     lpOut[2] = byte;
                     break;
                  case 5:
                     lpOut[1] = byte;
                     break;
                  case 7:
                     lpOut[0] = byte;
                     lpOut += 4;
                     break;
                     /* Word */
                  case 10:
                  case 15:
                     lpOut[1] = byte;
                     break;
                  case 12:
                  case 17:
                     lpOut[0] = byte;
                     lpOut += 2;
                     break;
#endif
                     /* Byte */
                  default:
                     lpOut[0] = byte;
                     lpOut++;
                     break;
               }
               lpszCLSID++; /* Skip 2nd character of byte */
               i++;
            }
      }
      lpszCLSID++;
      i++;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
STDCALL
RtlEraseUnicodeString(
   IN PUNICODE_STRING String)
{
    if (String->Buffer && String->MaximumLength)
    {
        RtlZeroMemory(String->Buffer, String->MaximumLength);
        String->Length = 0;
    }
}

/*
* @implemented
*/
NTSTATUS
STDCALL
RtlHashUnicodeString(
  IN CONST UNICODE_STRING *String,
  IN BOOLEAN CaseInSensitive,
  IN ULONG HashAlgorithm,
  OUT PULONG HashValue)
{
    if (String != NULL && HashValue != NULL)
    {
        switch (HashAlgorithm)
        {
            case HASH_STRING_ALGORITHM_DEFAULT:
            case HASH_STRING_ALGORITHM_X65599:
            {
                WCHAR *c, *end;

                *HashValue = 0;
                end = String->Buffer + (String->Length / sizeof(WCHAR));

                if (CaseInSensitive)
                {
                    for (c = String->Buffer;
                         c != end;
                         c++)
                    {
                        /* only uppercase characters if they are 'a' ... 'z'! */
                        *HashValue = ((65599 * (*HashValue)) +
                                      (ULONG)(((*c) >= L'a' && (*c) <= L'z') ?
                                              (*c) - L'a' + L'A' : (*c)));
                    }
                }
                else
                {
                    for (c = String->Buffer;
                         c != end;
                         c++)
                    {
                        *HashValue = ((65599 * (*HashValue)) + (ULONG)(*c));
                    }
                }
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 *
 * NOTES
 *  Same as RtlUnicodeStringToOemString but doesn't write terminating null
 *  Does a partial copy if the dest buffer is too small
 */
NTSTATUS
STDCALL
RtlUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    Length = RtlUnicodeStringToCountedOemSize(UniSource);

    if (!Length)
    {
        RtlZeroMemory(OemDest, sizeof(UNICODE_STRING));
    }

    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    OemDest->Length = (USHORT)Length;

    if (AllocateDestinationString)
    {
        OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
        OemDest->MaximumLength = Length;
        if (!OemDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (OemDest->Length >= OemDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUnicodeToOemN(OemDest->Buffer,
                              OemDest->Length,
                              &Index,
                              UniSource->Buffer,
                              UniSource->Length);

    /* FIXME: Special check needed and return STATUS_UNMAPPABLE_CHARACTER */

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
        OemDest->Buffer = NULL;
        return Status;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlLargeIntegerToChar(
   IN PLARGE_INTEGER Value,
   IN ULONG  Base,
   IN ULONG  Length,
   IN OUT PCHAR  String)
{
   ULONG Radix;
   CHAR  temp[65];
   ULONGLONG v = Value->QuadPart;
   ULONG i;
   PCHAR tp;
   PCHAR sp;

   Radix = Base;
   if (Radix == 0)
      Radix = 10;

   if ((Radix != 2) && (Radix != 8) &&
         (Radix != 10) && (Radix != 16))
      return STATUS_INVALID_PARAMETER;

   tp = temp;
   while (v || tp == temp)
   {
      i = v % Radix;
      v = v / Radix;
      if (i < 10)
         *tp = i + '0';
      else
         *tp = i + 'a' - 10;
      tp++;
   }

   if ((ULONG)((ULONG_PTR)tp - (ULONG_PTR)temp) >= Length)
      return STATUS_BUFFER_TOO_SMALL;

   sp = String;
   while (tp > temp)
      *sp++ = *--tp;
   *sp = 0;

   return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated. dest->Length is only set upon success.
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
    ULONG i, j;

    if (AllocateDestinationString == TRUE)
    {
        UniDest->MaximumLength = UniSource->Length;
        UniDest->Buffer = RtlpAllocateStringMemory(UniDest->MaximumLength, TAG_USTR);
        if (UniDest->Buffer == NULL) return STATUS_NO_MEMORY;
    }
    else if (UniSource->Length > UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    j = UniSource->Length / sizeof(WCHAR);

    for (i = 0; i < j; i++)
    {
        UniDest->Buffer[i] = RtlUpcaseUnicodeChar(UniSource->Buffer[i]);
    }

    UniDest->Length = UniSource->Length;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToAnsiString(
   IN OUT PANSI_STRING AnsiDest,
   IN PUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    Length = RtlUnicodeStringToAnsiSize(UniSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    AnsiDest->Length = (USHORT)Length - sizeof(CHAR);

    if (AllocateDestinationString)
    {
        AnsiDest->Buffer = RtlpAllocateStringMemory(Length, TAG_ASTR);
        AnsiDest->MaximumLength = Length;
        if (!AnsiDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (AnsiDest->Length >= AnsiDest->MaximumLength)
    {
        if (!AnsiDest->MaximumLength) return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUpcaseUnicodeToMultiByteN(AnsiDest->Buffer,
                                          AnsiDest->Length,
                                          &Index,
                                          UniSource->Buffer,
                                          UniSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(AnsiDest->Buffer, TAG_ASTR);
        AnsiDest->Buffer = NULL;
        return Status;
    }

    AnsiDest->Buffer[Index] = ANSI_NULL;
    return Status;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToCountedOemString(
   IN OUT POEM_STRING OemDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    Length = RtlUnicodeStringToCountedOemSize(UniSource);

    if (!Length)
    {
        RtlZeroMemory(OemDest, sizeof(UNICODE_STRING));
    }

    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    OemDest->Length = (USHORT)Length;

    if (AllocateDestinationString)
    {
        OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
        OemDest->MaximumLength = Length;
        if (!OemDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (OemDest->Length >= OemDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUpcaseUnicodeToOemN(OemDest->Buffer,
                                    OemDest->Length,
                                    &Index,
                                    UniSource->Buffer,
                                    UniSource->Length);

    /* FIXME: Special check needed and return STATUS_UNMAPPABLE_CHARACTER */

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
        OemDest->Buffer = NULL;
        return Status;
    }

    return Status;
}

/*
 * @implemented
 * NOTES
 *  Oem string is allways nullterminated
 *  It performs a partial copy if oem is too small.
 */
NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToOemString (
   IN OUT POEM_STRING OemDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN  AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    Length = RtlUnicodeStringToOemSize(UniSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    OemDest->Length = (USHORT)Length - sizeof(CHAR);

    if (AllocateDestinationString)
    {
        OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
        OemDest->MaximumLength = Length;
        if (!OemDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (OemDest->Length >= OemDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUpcaseUnicodeToOemN(OemDest->Buffer,
                                    OemDest->Length,
                                    &Index,
                                    UniSource->Buffer,
                                    UniSource->Length);

    /* FIXME: Special check needed and return STATUS_UNMAPPABLE_CHARACTER */

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
        OemDest->Buffer = NULL;
        return Status;
    }

    OemDest->Buffer[Index] = ANSI_NULL;
    return Status;
}

/*
 * @implemented
 *
 * RETURNS
 *  Bytes calculated including nullterm
 */
ULONG
STDCALL
RtlxOemStringToUnicodeSize(IN PCOEM_STRING OemString)
{
    ULONG Size;

    /* Convert the Mb String to Unicode Size */
    RtlMultiByteToUnicodeSize(&Size,
                              OemString->Buffer,
                              OemString->Length);

    /* Return the size + null-char */
    return (Size + sizeof(WCHAR));
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlStringFromGUID (IN REFGUID Guid,
                   OUT PUNICODE_STRING GuidString)
{
   STATIC CONST PWCHAR Hex = L"0123456789ABCDEF";
   WCHAR Buffer[40];
   PWCHAR BufferPtr;
   ULONG i;

   if (Guid == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }

   swprintf (Buffer,
             L"{%08lX-%04X-%04X-%02X%02X-",
             Guid->Data1,
             Guid->Data2,
             Guid->Data3,
             Guid->Data4[0],
             Guid->Data4[1]);
   BufferPtr = Buffer + 25;

   /* 6 hex bytes */
   for (i = 2; i < 8; i++)
   {
      *BufferPtr++ = Hex[Guid->Data4[i] >> 4];
      *BufferPtr++ = Hex[Guid->Data4[i] & 0xf];
   }

   *BufferPtr++ = L'}';
   *BufferPtr++ = L'\0';

   return RtlCreateUnicodeString (GuidString, Buffer);
}

/*
 * @implemented
 *
 * RETURNS
 *  Bytes calculated including nullterm
 */
ULONG
STDCALL
RtlxUnicodeStringToAnsiSize(IN PCUNICODE_STRING UnicodeString)
{
    ULONG Size;

    /* Convert the Unicode String to Mb Size */
    RtlUnicodeToMultiByteSize(&Size,
                              UnicodeString->Buffer,
                              UnicodeString->Length);

    /* Return the size + null-char */
    return (Size + sizeof(CHAR));
}

/*
 * @implemented
 */
LONG
STDCALL
RtlCompareUnicodeString(
   IN PCUNICODE_STRING s1,
   IN PCUNICODE_STRING s2,
   IN BOOLEAN  CaseInsensitive)
{
   unsigned int len;
   LONG ret = 0;
   LPCWSTR p1, p2;

   len = min(s1->Length, s2->Length) / sizeof(WCHAR);
   p1 = s1->Buffer;
   p2 = s2->Buffer;

   if (CaseInsensitive)
   {
     while (!ret && len--) ret = RtlUpcaseUnicodeChar(*p1++) - RtlUpcaseUnicodeChar(*p2++);
   }
   else
   {
     while (!ret && len--) ret = *p1++ - *p2++;
   }
   if (!ret) ret = s1->Length - s2->Length;
   return ret;
}

/*
 * @implemented
 */
VOID
STDCALL
RtlCopyString(
   IN OUT PSTRING DestinationString,
   IN PSTRING SourceString OPTIONAL)
{
    ULONG SourceLength;
    PCHAR p1, p2;

    /* Check if there was no source given */
    if(!SourceString)
    {
        /* Simply return an empty string */
        DestinationString->Length = 0;
    }
    else
    {
        /* Choose the smallest length */
        SourceLength = min(DestinationString->MaximumLength,
                           SourceString->Length);

        /* Set it */
        DestinationString->Length = (USHORT)SourceLength;

        /* Save the pointers to each buffer */
        p1 = DestinationString->Buffer;
        p2 = SourceString->Buffer;

        /* Loop the buffer */
        while (SourceLength)
        {
            /* Copy the character and move on */
            *p1++ = * p2++;
            SourceLength--;
        }
    }
}

/*
 * @implemented
 */
VOID
STDCALL
RtlCopyUnicodeString(
   IN OUT PUNICODE_STRING DestinationString,
   IN PCUNICODE_STRING SourceString)
{
    ULONG SourceLength;

    if(SourceString == NULL)
    {
        DestinationString->Length = 0;
    }
    else
    {
        SourceLength = min(DestinationString->MaximumLength,
                           SourceString->Length);
        DestinationString->Length = (USHORT)SourceLength;

        RtlCopyMemory(DestinationString->Buffer,
                      SourceString->Buffer,
                      SourceLength);

        if (DestinationString->Length < DestinationString->MaximumLength)
        {
            DestinationString->Buffer[SourceLength / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }
}

/*
 * @implemented
 *
 * NOTES
 * Creates a nullterminated UNICODE_STRING
 */
BOOLEAN
STDCALL
RtlCreateUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCWSTR  Source)
{
    ULONG Length;

    Length = (wcslen(Source) + 1) * sizeof(WCHAR);
    UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);

    if (UniDest->Buffer == NULL) return FALSE;

    RtlMoveMemory(UniDest->Buffer, Source, Length);
    UniDest->MaximumLength = (USHORT)Length;
    UniDest->Length = Length - sizeof (WCHAR);

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlCreateUnicodeStringFromAsciiz(
   OUT PUNICODE_STRING Destination,
   IN PCSZ Source)
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    RtlInitAnsiString(&AnsiString, Source);

    Status = RtlAnsiStringToUnicodeString(Destination,
                                          &AnsiString,
                                          TRUE);

    return NT_SUCCESS(Status);
}

/*
 * @implemented
 *
 * NOTES
 *  Dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated.
 *  Dest->Length is only set upon success.
 */
NTSTATUS
STDCALL
RtlDowncaseUnicodeString(
   IN OUT PUNICODE_STRING UniDest,
   IN PCUNICODE_STRING UniSource,
   IN BOOLEAN AllocateDestinationString)
{
    ULONG i;
    ULONG StopGap;

    if (AllocateDestinationString)
    {
        UniDest->MaximumLength = UniSource->Length;
        UniDest->Buffer = RtlpAllocateStringMemory(UniSource->Length, TAG_USTR);
        if (UniDest->Buffer == NULL) return STATUS_NO_MEMORY;
    }
    else if (UniSource->Length > UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    UniDest->Length = UniSource->Length;
    StopGap = UniSource->Length / sizeof(WCHAR);

    for (i= 0 ; i < StopGap; i++)
    {
        if (UniSource->Buffer[i] < L'A')
        {
            UniDest->Buffer[i] = UniSource->Buffer[i];
        }
        else if (UniSource->Buffer[i] <= L'Z')
        {
            UniDest->Buffer[i] = (UniSource->Buffer[i] + (L'a' - L'A'));
        }
        else
        {
            UniDest->Buffer[i] = RtlDowncaseUnicodeChar(UniSource->Buffer[i]);
        }
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is '\0' terminated when the MaximumLength allowes it.
 *  When dest fits exactly in MaximumLength characters the '\0' is ommitted.
 */
NTSTATUS
STDCALL
RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
                         IN PCWSTR Source)
{
    USHORT Length;
    PWCHAR DestBuffer;

    if (Source)
    {
        UNICODE_STRING UnicodeSource;

        RtlInitUnicodeString(&UnicodeSource, Source);
        Length = UnicodeSource.Length;

        if (Destination->Length + Length > Destination->MaximumLength)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        DestBuffer = &Destination->Buffer[Destination->Length / sizeof(WCHAR)];
        RtlMoveMemory(DestBuffer, Source, Length);
        Destination->Length += Length;

        /* append terminating '\0' if enough space */
        if(Destination->MaximumLength > Destination->Length)
        {
            DestBuffer[Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is never '\0' terminated.
 */
NTSTATUS
STDCALL
RtlAppendAsciizToString(
   IN OUT   PSTRING  Destination,
   IN PCSZ  Source)
{
    ULONG Length;

    if (Source)
    {
        Length = (USHORT)strlen(Source);

        if (Destination->Length + Length > Destination->MaximumLength)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlMoveMemory(&Destination->Buffer[Destination->Length], Source, Length);
        Destination->Length += Length;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
STDCALL
RtlUpperString(PSTRING DestinationString,
               PSTRING SourceString)
{
    ULONG Length;
    PCHAR Src, Dest;

    Length = min(SourceString->Length,
                 DestinationString->MaximumLength - 1);

    Src = SourceString->Buffer;
    Dest = DestinationString->Buffer;
    DestinationString->Length = Length;
    while (Length)
    {
        *Dest++ = RtlUpperChar(*Src++);
        Length--;
    }
}

/*
 * @implemented
 *
 * NOTES
 *  See RtlpDuplicateUnicodeString
 */
NTSTATUS
STDCALL
RtlDuplicateUnicodeString(
   IN ULONG Flags,
   IN PCUNICODE_STRING SourceString,
   OUT PUNICODE_STRING DestinationString)
{
   if (SourceString == NULL || DestinationString == NULL)
      return STATUS_INVALID_PARAMETER;


   if ((SourceString->Length == 0) && 
       (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE | 
                  RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
   {
      DestinationString->Length = 0;
      DestinationString->MaximumLength = 0;
      DestinationString->Buffer = NULL;
   }
   else
   {
      UINT DestMaxLength = SourceString->Length;

      if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
         DestMaxLength += sizeof(UNICODE_NULL);

      DestinationString->Buffer = RtlpAllocateStringMemory(DestMaxLength, TAG_USTR);
      if (DestinationString->Buffer == NULL)
         return STATUS_NO_MEMORY;

      RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
      DestinationString->Length = SourceString->Length;
      DestinationString->MaximumLength = DestMaxLength;

      if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
         DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
RtlValidateUnicodeString(IN ULONG Flags,
                         IN PUNICODE_STRING UnicodeString)
{
  /* currently no flags are supported! */
  ASSERT(Flags == 0);

  if ((Flags == 0) &&
      ((UnicodeString == NULL) ||
       ((UnicodeString->Length != 0) &&
        (UnicodeString->Buffer != NULL) &&
        ((UnicodeString->Length % sizeof(WCHAR)) == 0) &&
        ((UnicodeString->MaximumLength % sizeof(WCHAR)) == 0) &&
        (UnicodeString->MaximumLength >= UnicodeString->Length))))
  {
    /* a NULL pointer as a unicode string is considered to be a valid unicode
       string! */
    return STATUS_SUCCESS;
  }
  else
  {
    return STATUS_INVALID_PARAMETER;
  }
}

/* EOF */
