/* $Id: mem.c,v 1.21 2004/02/03 14:24:02 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/mem.c
 * PURPOSE:         Memory functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
MmCopyToCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes)
{
  NTSTATUS Status;

  if (ExGetPreviousMode() == UserMode)
    {
      if ((ULONG)Dest >= KERNEL_BASE)
	{
	  return(STATUS_ACCESS_VIOLATION);
	}
      Status = MmSafeCopyToUser(Dest, Src, NumberOfBytes);
      return(Status);
    }
  else
    {
      memcpy(Dest, Src, NumberOfBytes);
      return(STATUS_SUCCESS);
    }
}

NTSTATUS STDCALL
MmCopyFromCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes)
{
  NTSTATUS Status;

  if (ExGetPreviousMode() == UserMode)
    {
      if ((ULONG)Src >= KERNEL_BASE)
	{
	  return(STATUS_ACCESS_VIOLATION);
	}
      Status = MmSafeCopyFromUser(Dest, Src, NumberOfBytes);
      return(Status);
    }
  else
    {
      memcpy(Dest, Src, NumberOfBytes);
      return(STATUS_SUCCESS);
    }
}


/*
 * @implemented
 */
ULONG
STDCALL
RtlCompareMemory(PVOID	Source1,
	PVOID	Source2,
	ULONG	Length)
/*
 * FUNCTION: Compares blocks of memory and returns the number of equal bytes
 * ARGUMENTS:
 *      Source1 = Block to compare
 *      Source2 = Block to compare
 *      Length = Number of bytes to compare
 * RETURNS: Number of equal bytes
 */
{
   ULONG i,total;

   for (i=0,total=0;i<Length;i++)
     {
	if ( ((PUCHAR)Source1)[i] == ((PUCHAR)Source2)[i] )
	  {
	     total++;
	  }
     }
   return(total);
}


/*
 * @implemented
 */
ULONG
STDCALL
RtlCompareMemoryUlong (
	PVOID	Source,
	ULONG	Length,
	ULONG	Value
	)
/*
 * FUNCTION: Compares a block of ULONGs with an ULONG and returns the number of equal bytes
 * ARGUMENTS:
 *      Source = Block to compare
 *      Length = Number of bytes to compare
 *      Value = Value to compare
 * RETURNS: Number of equal bytes
 */
{
	PULONG ptr = (PULONG)Source;
	ULONG  len = Length / sizeof(ULONG);
	ULONG i;

	for (i = 0; i < len; i++)
	{
		if (*ptr != Value)
			break;
		ptr++;
	}

	return (ULONG)((PCHAR)ptr - (PCHAR)Source);
}


#if 0
VOID RtlCopyBytes(PVOID Destination,
		  CONST VOID* Source,
		  ULONG Length)
{
   RtlCopyMemory(Destination,Source,Length);
}
#endif

#if 0
VOID RtlCopyMemory(VOID* Destination, CONST VOID* Source, ULONG Length)
{
   DPRINT("RtlCopyMemory(Destination %x Source %x Length %d\n",
	  Destination,Source,Length);
   memcpy(Destination,Source,Length);
   DPRINT("*Destination %x\n",*(PULONG)Destination);
}
#endif

/*
 * @implemented
 */
VOID
STDCALL
RtlFillMemory (
	PVOID	Destination,
	ULONG	Length,
	UCHAR	Fill
	)
{
	memset(Destination,Fill,Length);
}


/*
 * @implemented
 */
VOID
STDCALL
RtlFillMemoryUlong (
	PVOID	Destination,
	ULONG	Length,
	ULONG	Fill
	)
{
	PULONG Dest  = Destination;
	ULONG  Count = Length / sizeof(ULONG);

	while (Count > 0)
	{
		*Dest = Fill;
		Dest++;
		Count--;
	}
}


/*
 * @implemented
 */
VOID
STDCALL
RtlMoveMemory (
	PVOID		Destination,
	CONST VOID	* Source,
	ULONG		Length
	)
{
	memmove (
		Destination,
		Source,
		Length
		);
}


/*
 * @implemented
 */
VOID
STDCALL
RtlZeroMemory (
	PVOID	Destination,
	ULONG	Length
	)
{
	RtlFillMemory (
		Destination,
		Length,
		0
		);
}


/*************************************************************************
 * RtlUshortByteSwap
 *
 * Swap the bytes of an unsigned short value.
 *
 * NOTES
 * Based on the inline versions in Wine winternl.h
 *
 * @implemented
 */
USHORT FASTCALL
RtlUshortByteSwap (IN USHORT Source)
{
  return (Source >> 8) | (Source << 8);
}


/*************************************************************************
 * RtlUlongByteSwap
 *
 * Swap the bytes of an unsigned int value.
 *
 * NOTES
 * Based on the inline versions in Wine winternl.h
 *
 * @implemented
 */
ULONG FASTCALL
RtlUlongByteSwap (IN ULONG Source)
{
  return ((ULONG) RtlUshortByteSwap ((USHORT)Source) << 16) | RtlUshortByteSwap ((USHORT)(Source >> 16));
}


/*************************************************************************
 * RtlUlonglongByteSwap
 *
 * Swap the bytes of an unsigned long long value.
 *
 * PARAMS
 *  i [I] Value to swap bytes of
 *
 * RETURNS
 *  The value with its bytes swapped.
 *
 * @implemented
 */
ULONGLONG FASTCALL
RtlUlonglongByteSwap (IN ULONGLONG Source)
{
  return ((ULONGLONG) RtlUlongByteSwap (Source) << 32) | RtlUlongByteSwap (Source>>32);
}

/* EOF */
