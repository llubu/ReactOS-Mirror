/* $Id: bitmap.c,v 1.3 2002/09/07 15:13:05 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/bitmap.c
 * PURPOSE:         Bitmap functions
 * UPDATE HISTORY:
 *                  20/08/99 Created by Eric Kohl
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


#define ALIGN(x,align)	(((x)+(align)-1) / (align))


VOID
STDCALL
RtlInitializeBitMap (
	PRTL_BITMAP	BitMapHeader,
	PULONG		BitMapBuffer,
	ULONG		SizeOfBitMap
	)
{
	BitMapHeader->SizeOfBitMap = SizeOfBitMap;
	BitMapHeader->Buffer = BitMapBuffer;
}


BOOLEAN
STDCALL
RtlAreBitsClear (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		Length
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Shift;
	ULONG Count;
	PCHAR Ptr;

	if (StartingIndex >= Size ||
	    !Length ||
	    (StartingIndex + Length > Size))
		return FALSE;

	Ptr = (PCHAR)BitMapHeader->Buffer + (StartingIndex / 8);
	while (Length)
	{
		/* get bit shift in current byte */
		Shift = StartingIndex & 7;

		/* get number of bits to check in current byte */
		Count = (Length > 8 - Shift) ? 8 - Shift : Length;

		/* check byte */
		if (*Ptr++ & (~(0xFF << Count) << Shift))
			return FALSE;

		Length -= Count;
		StartingIndex += Count;
	}

	return TRUE;
}


BOOLEAN
STDCALL
RtlAreBitsSet (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		Length
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Shift;
	ULONG Count;
	PCHAR Ptr;

	if (StartingIndex >= Size ||
	    !Length ||
	    (StartingIndex + Length > Size))
		return FALSE;

	Ptr = (PCHAR)BitMapHeader->Buffer + (StartingIndex / 8);
	while (Length)
	{
		/* get bit shift in current byte */
		Shift = StartingIndex & 7;

		/* get number of bits to check in current byte */
		Count = (Length > 8 - Shift) ? 8 - Shift : Length;

		/* check byte */
		if (~*Ptr++ & (~(0xFF << Count) << Shift))
			return FALSE;

		Length -= Count;
		StartingIndex += Count;
	}

	return TRUE;
}


VOID
STDCALL
RtlClearAllBits (
	IN OUT	PRTL_BITMAP	BitMapHeader
	)
{
	memset (BitMapHeader->Buffer,
	        0x00,
	        ALIGN(BitMapHeader->SizeOfBitMap, 8));
}


VOID
STDCALL
RtlClearBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		NumberToClear
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Count;
	ULONG Shift;
	PCHAR Ptr;

	if (StartingIndex >= Size || NumberToClear == 0)
		return;

	if (StartingIndex + NumberToClear > Size)
		NumberToClear = Size - StartingIndex;

	Ptr = (PCHAR)(BitMapHeader->Buffer + (StartingIndex / 8));
	while (NumberToClear)
	{
		/* bit shift in current byte */
		Shift = StartingIndex & 7;

		/* number of bits to change in current byte */
		Count = (NumberToClear > 8 - Shift ) ? 8 - Shift : NumberToClear;

		/* adjust byte */
		*Ptr++ &= ~(~(0xFF << Count) << Shift);
		NumberToClear -= Count;
		StartingIndex += Count;
	}
}


ULONG
STDCALL
RtlFindClearBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	PCHAR Ptr;
	CHAR  Mask;

	if (NumberToFind > Size || NumberToFind == 0)
		return -1;

	if (HintIndex >= Size)
		HintIndex = 0;

	Index = HintIndex;
	Ptr = (PCHAR)BitMapHeader->Buffer + (Index / 8);
	Mask  = 1 << (Index & 7);

	while (HintIndex < Size)
	{
		/* count clear bits */
		for (Count = 0; Index < Size && ~*Ptr & Mask; Index++)
		{
			if (++Count >= NumberToFind)
				return HintIndex;
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		/* skip set bits */
		for (; Index < Size && *Ptr & Mask; Index++)
		{
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}
		HintIndex = Index;
	}

	return -1;
}


ULONG
STDCALL
RtlFindClearBitsAndSet (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	)
{
	ULONG Index;

	Index = RtlFindClearBits (BitMapHeader,
	                          NumberToFind,
	                          HintIndex);
	if (Index != (ULONG)-1)
		RtlSetBits (BitMapHeader,
		            Index,
		            NumberToFind);

	return Index;
}


ULONG
STDCALL
RtlFindFirstRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	PCHAR Ptr;
	CHAR  Mask;

	if (*StartingIndex > Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}

	Index = *StartingIndex;
	Ptr = (PCHAR)BitMapHeader->Buffer + (Index / 8);
	Mask = 1 << (Index & 7);

	/* skip set bits */
	for (; Index < Size && *Ptr & Mask; Index++)
	{
		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	/* return index of first clear bit */
	if (Index >= Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}
	else
		*StartingIndex = Index;

	/* count clear bits */
	for (Count = 0; Index < Size && ~*Ptr & Mask; Index++)
	{
		Count++;
		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	return Count;
}


ULONG
STDCALL
RtlFindFirstRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	PCHAR Ptr;
	CHAR  Mask;

	if (*StartingIndex > Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}

	Index = *StartingIndex;
	Ptr = (PCHAR)BitMapHeader->Buffer + (Index / 8);
	Mask = 1 << (Index & 7);

	/* skip clear bits */
	for (; Index < Size && ~*Ptr & Mask; Index++)
	{
		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	/* return index of first set bit */
	if (Index >= Size)
	{
		*StartingIndex = (ULONG)-1;
		return 0;
	}
	else
		*StartingIndex = Index;

	/* count set bits */
	for (Count = 0; Index < Size && *Ptr & Mask; Index++)
	{
		Count++;
		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	return Count;
}


ULONG
STDCALL
RtlFindLongestRunClear (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	PCHAR Ptr = (PCHAR)BitMapHeader->Buffer;
	ULONG Index = 0;
	ULONG Count;
	ULONG Max = 0;
	ULONG Start;
	ULONG Maxstart = 0;
	CHAR  Mask = 1;

	while (Index < Size)
	{
		Start = Index;

		/* count clear bits */
		for (Count = 0; Index < Size && ~*Ptr & Mask; Index++)
		{
			Count++;
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		/* skip set bits */
		for (; Index < Size && *Ptr & Mask; Index++)
		{
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		if (Count > Max)
		{
			Max = Count;
			Maxstart = Start;
		}
	}

	if (StartingIndex)
		*StartingIndex = Maxstart;

	return Max;
}


ULONG
STDCALL
RtlFindLongestRunSet (
	PRTL_BITMAP	BitMapHeader,
	PULONG		StartingIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	PCHAR Ptr = (PCHAR)BitMapHeader->Buffer;
	ULONG Index = 0;
	ULONG Count;
	ULONG Max = 0;
	ULONG Start;
	ULONG Maxstart = 0;
	CHAR  Mask = 1;

	while (Index < Size)
	{
		Start = Index;

		/* count set bits */
		for (Count = 0; Index < Size && *Ptr & Mask; Index++)
		{
			Count++;
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		/* skip clear bits */
		for (; Index < Size && ~*Ptr & Mask; Index++)
		{
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		if (Count > Max)
		{
			Max = Count;
			Maxstart = Start;
		}
	}

	if (StartingIndex)
		*StartingIndex = Maxstart;

	return Max;
}


ULONG
STDCALL
RtlFindSetBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	PCHAR Ptr;
	CHAR  Mask;

	if (NumberToFind > Size || NumberToFind == 0)
		return (ULONG)-1;

	if (HintIndex >= Size)
		HintIndex = 0;

	Index = HintIndex;
	Ptr = (PCHAR)BitMapHeader->Buffer + (Index / 8);
	Mask = 1 << (Index & 7);

	while (HintIndex < Size)
	{
		/* count set bits */
		for (Count = 0; Index < Size && *Ptr & Mask; Index++)
		{
			if (++Count >= NumberToFind)
				return HintIndex;
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}

		/* skip clear bits */
		for (; Index < Size && ~*Ptr & Mask; Index++)
		{
			Mask <<= 1;
			if (Mask == 0)
			{
				Mask = 1;
				Ptr++;
			}
		}
		HintIndex = Index;
	}

	return (ULONG)-1;
}


ULONG
STDCALL
RtlFindSetBitsAndClear (
	PRTL_BITMAP	BitMapHeader,
	ULONG		NumberToFind,
	ULONG		HintIndex
	)
{
	ULONG Index;

	Index = RtlFindSetBits (BitMapHeader,
	                        NumberToFind,
	                        HintIndex);
	if (Index != (ULONG)-1)
		RtlClearBits (BitMapHeader,
		              Index,
		              NumberToFind);

	return Index;
}


ULONG
STDCALL
RtlNumberOfClearBits (
	PRTL_BITMAP	BitMapHeader
	)
{
	PCHAR Ptr = (PCHAR)BitMapHeader->Buffer;
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	CHAR Mask;

	for (Mask = 1, Index = 0, Count = 0; Index < Size; Index++)
	{
		if (~*Ptr & Mask)
			Count++;

		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	return Count;
}


ULONG
STDCALL
RtlNumberOfSetBits (
	PRTL_BITMAP	BitMapHeader
	)
{
	PCHAR Ptr = (PCHAR)BitMapHeader->Buffer;
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Index;
	ULONG Count;
	CHAR Mask;

	for (Mask = 1, Index = 0, Count = 0; Index < Size; Index++)
	{
		if (*Ptr & Mask)
			Count++;

		Mask <<= 1;
		if (Mask == 0)
		{
			Mask = 1;
			Ptr++;
		}
	}

	return Count;
}


VOID
STDCALL
RtlSetAllBits (
	IN OUT	PRTL_BITMAP	BitMapHeader
	)
{
	memset (BitMapHeader->Buffer,
	        0xFF,
	        ALIGN(BitMapHeader->SizeOfBitMap, 8));
}


VOID
STDCALL
RtlSetBits (
	PRTL_BITMAP	BitMapHeader,
	ULONG		StartingIndex,
	ULONG		NumberToSet
	)
{
	ULONG Size = BitMapHeader->SizeOfBitMap;
	ULONG Count;
	ULONG Shift;
	PCHAR Ptr;

	if (StartingIndex >= Size || NumberToSet == 0)
		return;

	if (StartingIndex + NumberToSet > Size)
		NumberToSet = Size - StartingIndex;

	Ptr = (PCHAR)BitMapHeader->Buffer + (StartingIndex / 8);
	while (NumberToSet)
	{
		/* bit shift in current byte */
		Shift = StartingIndex & 7;

		/* number of bits to change in current byte */
		Count = (NumberToSet > 8 - Shift) ? 8 - Shift : NumberToSet;

		/* adjust byte */
		*Ptr++ |= ~(0xFF << Count) << Shift;
		NumberToSet -= Count;
		StartingIndex += Count;
	}
}

/* EOF */
