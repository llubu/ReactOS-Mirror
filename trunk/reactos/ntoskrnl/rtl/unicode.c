/* $Id: unicode.c,v 1.26 2002/12/08 16:23:32 robd Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/rtl/unicode.c
 * PURPOSE:         String functions
 * PROGRAMMER:      Jason Filby (jasonfilby@yahoo.com)
 * UPDATE HISTORY:
 *                  Created 10/08/98
 */

#include <ddk/ntddk.h>
//#include <internal/nls.h>
#include <ctype.h>
#include <ntos/minmax.h>
#include <internal/pool.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define TAG_USTR  TAG('U', 'S', 'T', 'R')
#define TAG_ASTR  TAG('A', 'S', 'T', 'R')
#define TAG_OSTR  TAG('O', 'S', 'T', 'R')

/* FUNCTIONS *****************************************************************/

WCHAR
STDCALL
RtlAnsiCharToUnicodeChar(IN CHAR AnsiChar)
{
  ULONG Size;
  WCHAR UnicodeChar;

  Size = 1;
#if 0
  Size = (NlsLeadByteInfo[AnsiChar] == 0) ? 1 : 2;
#endif

  RtlMultiByteToUnicodeN(&UnicodeChar,
			 sizeof(WCHAR),
			 NULL,
			 &AnsiChar,
			 Size);

  return(UnicodeChar);
}


ULONG
STDCALL
RtlAnsiStringToUnicodeSize(IN PANSI_STRING AnsiString)
{
  ULONG Size;

  RtlMultiByteToUnicodeSize(&Size,
			    AnsiString->Buffer,
			    AnsiString->Length);

  return(Size);
}


NTSTATUS
STDCALL
RtlAnsiStringToUnicodeString(IN OUT PUNICODE_STRING DestinationString,
			     IN PANSI_STRING SourceString,
			     IN BOOLEAN AllocateDestinationString)
{
	NTSTATUS Status;
	ULONG Length;

	if (NlsMbCodePageTag == TRUE)
		Length = RtlAnsiStringToUnicodeSize (SourceString);
	else
		Length = SourceString->Length * sizeof(WCHAR);

	if (Length > 65535)
		return STATUS_INVALID_PARAMETER_2;

	if (AllocateDestinationString == TRUE)
	{
		DestinationString->MaximumLength = Length + sizeof(WCHAR);
		DestinationString->Buffer =
		  ExAllocatePoolWithTag (NonPagedPool,
					 DestinationString->MaximumLength,
					 TAG_USTR);
		if (DestinationString->Buffer == NULL)
			return STATUS_NO_MEMORY;
	}
	else
	{
		if (Length + sizeof(WCHAR) > DestinationString->MaximumLength)
		{
			DPRINT("STATUS_BUFFER_TOO_SMALL\n");
			return STATUS_BUFFER_TOO_SMALL;
		}
	}
	DestinationString->Length = Length;

	RtlZeroMemory (DestinationString->Buffer,
	               DestinationString->Length);

	Status = RtlMultiByteToUnicodeN (DestinationString->Buffer,
	                                 DestinationString->Length,
	                                 NULL,
	                                 SourceString->Buffer,
	                                 SourceString->Length);
	if (!NT_SUCCESS(Status))
	{
		if (AllocateDestinationString)
			ExFreePool (DestinationString->Buffer);
		return Status;
	}

	DestinationString->Buffer[Length / sizeof(WCHAR)] = 0;

	return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlAppendAsciizToString(IN OUT PSTRING Destination,
			IN PCSZ Source)
{
	ULONG Length;
	PCHAR Ptr;

	if (Source == NULL)
		return STATUS_SUCCESS;

	Length = strlen (Source);
	if (Destination->Length + Length >= Destination->MaximumLength)
		return STATUS_BUFFER_TOO_SMALL;

	Ptr = Destination->Buffer + Destination->Length;
	memmove (Ptr,
	         Source,
	         Length);
	Ptr += Length;
	*Ptr = 0;

	Destination->Length += Length;

	return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlAppendStringToString(IN OUT PSTRING Destination,
			IN PSTRING Source)
{
  PCHAR Ptr;

  if (Source->Length == 0)
    return(STATUS_SUCCESS);

  if (Destination->Length + Source->Length >= Destination->MaximumLength)
    return(STATUS_BUFFER_TOO_SMALL);

  Ptr = Destination->Buffer + Destination->Length;
  memmove(Ptr,
	  Source->Buffer,
	  Source->Length);
  Ptr += Source->Length;
  *Ptr = 0;

  Destination->Length += Source->Length;

  return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
RtlAppendUnicodeStringToString(IN OUT PUNICODE_STRING Destination,
			       IN PUNICODE_STRING Source)
{
	PWCHAR Src, Dest;
	ULONG i;

	if ((Source->Length + Destination->Length) >= Destination->MaximumLength)
		return STATUS_BUFFER_TOO_SMALL;

	Src  = Source->Buffer;
	Dest = Destination->Buffer + (Destination->Length / sizeof (WCHAR));
	for (i = 0; i < (Source->Length / sizeof(WCHAR)); i++)
	{
		*Dest = *Src;
		Dest++;
		Src++;
	}
	*Dest = 0;

	Destination->Length += Source->Length;

	return STATUS_SUCCESS;
}


NTSTATUS STDCALL
RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
			 IN PWSTR Source)
{
  PWCHAR Src;
  PWCHAR Dest;
  ULONG  i;
  ULONG  slen;

  slen = wcslen(Source) * sizeof(WCHAR);

  if (Destination->Length + slen >= Destination->MaximumLength)
    return(STATUS_BUFFER_TOO_SMALL);

  Src = Source;
  Dest = Destination->Buffer + (Destination->Length / sizeof(WCHAR));

  for (i = 0; i < (slen / sizeof(WCHAR)); i++)
    {
      *Dest = *Src;
      Dest++;
      Src++;
    }
  *Dest = 0;

  Destination->Length += slen;

  return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
RtlCharToInteger(IN PCSZ String,
		 IN ULONG Base,
		 IN OUT PULONG Value)
{
  ULONG Val;

  *Value = 0;

  if (Base == 0)
    {
      Base = 10;
      if (*String == '0')
	{
	  Base = 8;
	  String++;
	  if ((*String == 'x') && isxdigit (String[1]))
	    {
	      String++;
	      Base = 16;
	    }
	}
    }

  if (!isxdigit (*String))
    return(STATUS_INVALID_PARAMETER);

  while (isxdigit (*String) &&
	 (Val = isdigit (*String) ? * String - '0' : (islower (*String)
	  ? toupper (*String) : *String) - 'A' + 10) < Base)
    {
      *Value = *Value * Base + Val;
      String++;
    }

  return(STATUS_SUCCESS);
}


LONG
STDCALL
RtlCompareString(IN PSTRING String1,
		 IN PSTRING String2,
		 IN BOOLEAN CaseInsensitive)
{
	ULONG len1, len2;
	PCHAR s1, s2;
	CHAR  c1, c2;

	if (String1 && String2)
	{
		len1 = String1->Length;
		len2 = String2->Length;
		s1 = String1->Buffer;
		s2 = String2->Buffer;

		if (s1 && s2)
		{
			if (CaseInsensitive)
			{
				while (1)
				{
					c1 = len1-- ? RtlUpperChar (*s1++) : 0;
					c2 = len2-- ? RtlUpperChar (*s2++) : 0;
					if (!c1 || !c2 || c1 != c2)
						return c1 - c2;
				}
			}
			else
			{
				while (1)
				{
					c1 = len1-- ? *s1++ : 0;
					c2 = len2-- ? *s2++ : 0;
					if (!c1 || !c2 || c1 != c2)
						return c1 - c2;
				}
			}
		}
	}

	return 0;
}


LONG
STDCALL
RtlCompareUnicodeString(IN PUNICODE_STRING String1,
			IN PUNICODE_STRING String2,
			IN BOOLEAN CaseInsensitive)
{
	ULONG len1, len2;
	PWCHAR s1, s2;
	WCHAR  c1, c2;

	if (String1 && String2)
	{
		len1 = String1->Length / sizeof(WCHAR);
		len2 = String2->Length / sizeof(WCHAR);
		s1 = String1->Buffer;
		s2 = String2->Buffer;

		if (s1 && s2)
		{
			if (CaseInsensitive)
			{
				while (1)
				{
					c1 = len1-- ? RtlUpcaseUnicodeChar (*s1++) : 0;
					c2 = len2-- ? RtlUpcaseUnicodeChar (*s2++) : 0;
					if (!c1 || !c2 || c1 != c2)
						return c1 - c2;
				}
			}
			else
			{
				while (1)
				{
					c1 = len1-- ? *s1++ : 0;
					c2 = len2-- ? *s2++ : 0;
					if (!c1 || !c2 || c1 != c2)
						return c1 - c2;
				}
			}
		}
	}

	return 0;
}


VOID
STDCALL
RtlCopyString(IN OUT PSTRING DestinationString,
	      IN PSTRING SourceString)
{
	ULONG copylen, i;
	PCHAR Src, Dest;

	if(SourceString == NULL)
	{
		DestinationString->Length = 0;
		return;
	}

	copylen = min (DestinationString->MaximumLength - sizeof(CHAR),
	               SourceString->Length);
	Src = SourceString->Buffer;
	Dest = DestinationString->Buffer;

	for (i = 0; i < copylen; i++)
	{
		*Dest = *Src;
		Dest++;
		Src++;
	}
	*Dest = 0;

	DestinationString->Length = copylen;
}


VOID
STDCALL
RtlCopyUnicodeString(IN OUT PUNICODE_STRING DestinationString,
		     IN PUNICODE_STRING SourceString)
{
	ULONG copylen, i;
	PWCHAR Src, Dest;

	if(SourceString==NULL)
	{
		DestinationString->Length=0;
		return;
	}

	copylen = min(DestinationString->MaximumLength - sizeof(WCHAR),
	              SourceString->Length);
	Src = SourceString->Buffer;
	Dest = DestinationString->Buffer;

	for (i = 0; i < (copylen / sizeof (WCHAR)); i++)
	{
		*Dest = *Src;
		Dest++;
		Src++;
	}
	*Dest = 0;

	DestinationString->Length = copylen;
}


BOOLEAN
STDCALL
RtlCreateUnicodeString(IN OUT PUNICODE_STRING Destination,
		       IN PWSTR Source)
{
	ULONG Length;

	Length = (wcslen (Source) + 1) * sizeof(WCHAR);

	Destination->Buffer = ExAllocatePoolWithTag (NonPagedPool,
						     Length,
						     TAG_USTR);
	if (Destination->Buffer == NULL)
		return FALSE;

	memmove (Destination->Buffer,
	         Source,
	         Length);

	Destination->MaximumLength = Length;
	Destination->Length = Length - sizeof (WCHAR);

	return TRUE;
}


BOOLEAN
STDCALL
RtlCreateUnicodeStringFromAsciiz(IN OUT PUNICODE_STRING Destination,
				 IN PCSZ Source)
{
  ANSI_STRING AnsiString;
  NTSTATUS Status;

  RtlInitAnsiString(&AnsiString,
		    Source);

  Status = RtlAnsiStringToUnicodeString(Destination,
					&AnsiString,
					TRUE);

  return(NT_SUCCESS(Status));
}


NTSTATUS
STDCALL
RtlDowncaseUnicodeString(IN OUT PUNICODE_STRING DestinationString,
			 IN PUNICODE_STRING SourceString,
			 IN BOOLEAN AllocateDestinationString)
{
  ULONG i;
  PWCHAR Src, Dest;
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
      DestinationString->Buffer = 
	ExAllocatePoolWithTag (NonPagedPool,
			       SourceString->Length + sizeof(WCHAR),
			       TAG_USTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (SourceString->Length >= DestinationString->MaximumLength)
	return STATUS_BUFFER_TOO_SMALL;
    }
  DestinationString->Length = SourceString->Length;
  
  Src = SourceString->Buffer;
  Dest = DestinationString->Buffer;
  for (i=0; i < SourceString->Length / sizeof(WCHAR); i++)
    {
      if (*Src < L'A')
	{
	  *Dest = *Src;
	}
      else if (*Src <= L'Z')
	{
	  *Dest = (*Src + (L'a' - L'A'));
	}
      else
	{
	  /* FIXME: characters above 'Z' */
	  *Dest = *Src;
	}
      
      Dest++;
      Src++;
    }
  *Dest = 0;
  
  return STATUS_SUCCESS;
}


BOOLEAN
STDCALL
RtlEqualString(IN PSTRING String1,
	       IN PSTRING String2,
	       IN BOOLEAN CaseInsensitive)
{
	unsigned long s1l=String1->Length;
	unsigned long s2l=String2->Length;
	unsigned long i;
	char c1, c2;

	if (s1l != s2l)
		return FALSE;

	for (i = 0; i < s1l; i++)
	{
		c1 = *String1->Buffer;
		c2 = *String2->Buffer;

		if (CaseInsensitive == TRUE)
		{
			c1 = RtlUpperChar (c1);
			c2 = RtlUpperChar (c2);
		}

		if (c1 != c2)
		{
			String1->Buffer -= i;
			String2->Buffer -= i;
			return FALSE;
		}

		String1->Buffer++;
		String2->Buffer++;
	}

	String1->Buffer -= i;
	String2->Buffer -= i;

	return TRUE;
}


BOOLEAN
STDCALL
RtlEqualUnicodeString(IN PUNICODE_STRING String1,
		      IN PUNICODE_STRING String2,
		      IN BOOLEAN CaseInsensitive)
{
	unsigned long s1l = String1->Length / sizeof(WCHAR);
	unsigned long s2l = String2->Length / sizeof(WCHAR);
	unsigned long i;
	WCHAR wc1, wc2;
	PWCHAR pw1, pw2;

	if (s1l != s2l)
		return FALSE;

	pw1 = String1->Buffer;
	pw2 = String2->Buffer;

	for (i = 0; i < s1l; i++)
	{
		if(CaseInsensitive == TRUE)
		{
			wc1 = RtlUpcaseUnicodeChar (*pw1);
			wc2 = RtlUpcaseUnicodeChar (*pw2);
		}
		else
		{
			wc1 = *pw1;
			wc2 = *pw2;
		}

		if (wc1 != wc2)
			return FALSE;

		pw1++;
		pw2++;
	}

	return TRUE;
}


VOID
STDCALL
RtlFreeAnsiString(IN PANSI_STRING AnsiString)
{
	if (AnsiString->Buffer == NULL)
		return;

	ExFreePool (AnsiString->Buffer);

	AnsiString->Buffer = NULL;
	AnsiString->Length = 0;
	AnsiString->MaximumLength = 0;
}


VOID
STDCALL
RtlFreeOemString(IN POEM_STRING OemString)
{
	if (OemString->Buffer == NULL)
		return;

	ExFreePool (OemString->Buffer);

	OemString->Buffer = NULL;
	OemString->Length = 0;
	OemString->MaximumLength = 0;
}


VOID
STDCALL
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
	if (UnicodeString->Buffer == NULL)
		return;

	ExFreePool (UnicodeString->Buffer);

	UnicodeString->Buffer = NULL;
	UnicodeString->Length = 0;
	UnicodeString->MaximumLength = 0;
}


VOID
STDCALL
RtlInitAnsiString(IN OUT PANSI_STRING DestinationString,
		  IN PCSZ SourceString)
{
	ULONG DestSize;

	if (SourceString == NULL)
	{
		DestinationString->Length = 0;
		DestinationString->MaximumLength = 0;
	}
	else
	{
		DestSize = strlen ((const char *)SourceString);
		DestinationString->Length = DestSize;
		DestinationString->MaximumLength = DestSize + sizeof(CHAR);
	}
	DestinationString->Buffer = (PCHAR)SourceString;
}


VOID
STDCALL
RtlInitString(IN OUT PSTRING DestinationString,
	      IN PCSZ SourceString)
{
	ULONG DestSize;

	if (SourceString == NULL)
	{
		DestinationString->Length = 0;
		DestinationString->MaximumLength = 0;
	}
	else
	{
		DestSize = strlen((const char *)SourceString);
		DestinationString->Length = DestSize;
		DestinationString->MaximumLength = DestSize + sizeof(CHAR);
	}
	DestinationString->Buffer = (PCHAR)SourceString;
}


VOID
STDCALL
RtlInitUnicodeString(IN OUT PUNICODE_STRING DestinationString,
		     IN PCWSTR SourceString)
{
	ULONG DestSize;

	DPRINT("RtlInitUnicodeString(DestinationString %x, SourceString %x)\n",
	       DestinationString,
	       SourceString);

	if (SourceString == NULL)
	{
		DestinationString->Length = 0;
		DestinationString->MaximumLength = 0;
	}
	else
	{
		DestSize = wcslen((PWSTR)SourceString) * sizeof(WCHAR);
		DestinationString->Length = DestSize;
		DestinationString->MaximumLength = DestSize + sizeof(WCHAR);
	}
	DestinationString->Buffer = (PWSTR)SourceString;
}


NTSTATUS
STDCALL
RtlIntegerToChar(IN ULONG Value,
		 IN ULONG Base,
		 IN ULONG Length,
		 IN OUT PCHAR String)
{
	ULONG Radix;
	CHAR  temp[33];
	ULONG v = 0;
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

	if ((ULONG) (tp - temp) >= Length)
		return STATUS_BUFFER_TOO_SMALL;

	sp = String;
	while (tp > temp)
		*sp++ = *--tp;
	*sp = 0;

	return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlIntegerToUnicodeString(IN ULONG Value,
			  IN ULONG Base,	/* optional */
			  IN OUT PUNICODE_STRING String)
{
	ANSI_STRING AnsiString;
	CHAR Buffer[33];
	NTSTATUS Status;

	Status = RtlIntegerToChar (Value,
	                           Base,
	                           33,
	                           Buffer);
	if (!NT_SUCCESS(Status))
		return Status;

	AnsiString.Buffer = Buffer;
	AnsiString.Length = strlen (Buffer);
	AnsiString.MaximumLength = 33;

	Status = RtlAnsiStringToUnicodeString (String,
	                                       &AnsiString,
	                                       FALSE);

	return Status;
}


NTSTATUS
STDCALL
RtlOemStringToCountedUnicodeString(IN OUT PUNICODE_STRING DestinationString,
				   IN POEM_STRING SourceString,
				   IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbCodePageTag == TRUE)
    Length = RtlAnsiStringToUnicodeSize (SourceString);
  else
    Length = SourceString->Length * sizeof(WCHAR);
  
  if (Length > 65535)
    return STATUS_INVALID_PARAMETER_2;
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(WCHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag (NonPagedPool,
			       DestinationString->MaximumLength,
			       TAG_USTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (Length > DestinationString->MaximumLength)
	return STATUS_BUFFER_TOO_SMALL;
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory (DestinationString->Buffer,
		 DestinationString->Length);
  
  Status = RtlOemToUnicodeN (DestinationString->Buffer,
			     DestinationString->Length,
			     NULL,
			     SourceString->Buffer,
			     SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool (DestinationString->Buffer);
      
      return Status;
    }

  DestinationString->Buffer[Length / sizeof(WCHAR)] = 0;
  
  return STATUS_SUCCESS;
}


ULONG
STDCALL
RtlOemStringToUnicodeSize(IN POEM_STRING OemString)
{
  ULONG Size;

  RtlMultiByteToUnicodeSize(&Size,
			    OemString->Buffer,
			    OemString->Length);

  return(Size);
}


NTSTATUS
STDCALL
RtlOemStringToUnicodeString(IN OUT PUNICODE_STRING DestinationString,
			    IN POEM_STRING SourceString,
			    IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbCodePageTag == TRUE)
    Length = RtlAnsiStringToUnicodeSize (SourceString);
  else
    Length = SourceString->Length * sizeof(WCHAR);
  
  if (Length > 65535)
    return STATUS_INVALID_PARAMETER_2;
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(WCHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag (NonPagedPool,
			       DestinationString->MaximumLength,
			       TAG_USTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (Length + sizeof(WCHAR) > DestinationString->MaximumLength)
      {
	DPRINT("STATUS_BUFFER_TOO_SMALL\n");
	return STATUS_BUFFER_TOO_SMALL;
      }
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory (DestinationString->Buffer,
		 DestinationString->Length);
  
  Status = RtlOemToUnicodeN (DestinationString->Buffer,
			     DestinationString->Length,
			     NULL,
			     SourceString->Buffer,
	                           SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool (DestinationString->Buffer);
      return Status;
    }

  DestinationString->Buffer[Length / sizeof(WCHAR)] = 0;
  
  return STATUS_SUCCESS;
}


BOOLEAN
STDCALL
RtlPrefixString(IN PANSI_STRING String1,
		IN PANSI_STRING String2,
		IN BOOLEAN CaseInsensitive)
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


BOOLEAN
STDCALL
RtlPrefixUnicodeString(IN PUNICODE_STRING String1,
		       IN PUNICODE_STRING String2,
		       IN BOOLEAN CaseInsensitive)
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


ULONG
STDCALL
RtlUnicodeStringToAnsiSize(IN PUNICODE_STRING UnicodeString)
{
  ULONG Size;

  RtlUnicodeToMultiByteSize(&Size,
			    UnicodeString->Buffer,
			    UnicodeString->Length);

  return(Size+1);
}


NTSTATUS
STDCALL
RtlUnicodeStringToAnsiString(IN OUT PANSI_STRING DestinationString,
			     IN PUNICODE_STRING SourceString,
			     IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbCodePageTag == TRUE){
    Length = RtlUnicodeStringToAnsiSize (SourceString); Length--;
  }
  else
    Length = SourceString->Length / sizeof(WCHAR);
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(CHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag (NonPagedPool,
			       DestinationString->MaximumLength,
			       TAG_ASTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (Length >= DestinationString->MaximumLength)
	return STATUS_BUFFER_TOO_SMALL;
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory (DestinationString->Buffer,
		 DestinationString->Length);
  
  Status = RtlUnicodeToMultiByteN (DestinationString->Buffer,
				   DestinationString->Length,
				   NULL,
				   SourceString->Buffer,
				   SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool (DestinationString->Buffer);
		return Status;
    }
  
  DestinationString->Buffer[Length] = 0;
  
  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlUnicodeStringToCountedOemString(IN OUT POEM_STRING DestinationString,
				   IN PUNICODE_STRING SourceString,
				   IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  ULONG Size;
  
  if (NlsMbOemCodePageTag == TRUE)
    Length = RtlUnicodeStringToAnsiSize (SourceString)/* + 1*/;
  else
    Length = SourceString->Length / sizeof(WCHAR) + 1;
  
  if (Length > 0x0000FFFF)
    return STATUS_INVALID_PARAMETER_2;
  
  DestinationString->Length = (WORD)(Length - 1);
  
  if (AllocateDestinationString)
    {
      DestinationString->Buffer = ExAllocatePoolWithTag (NonPagedPool,
							 Length,
							 TAG_ASTR);
      
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
      
      RtlZeroMemory (DestinationString->Buffer,
		     Length);
      DestinationString->MaximumLength = (WORD)Length;
	}
  else
    {
      if (Length > DestinationString->MaximumLength)
	{
	  if (DestinationString->MaximumLength == 0)
	    return STATUS_BUFFER_OVERFLOW;
	  DestinationString->Length =
	    DestinationString->MaximumLength - 1;
	}
    }
  
  Status = RtlUnicodeToOemN (DestinationString->Buffer,
			     DestinationString->Length,
			     &Size,
			     SourceString->Buffer,
			     SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool (DestinationString->Buffer);

      return Status;
    }
  
  DestinationString->Buffer[Size] = 0;
  
  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlUnicodeStringToInteger(IN PUNICODE_STRING String,
			  IN ULONG Base,
			  OUT PULONG Value)
{
	PWCHAR Str;
	ULONG lenmin = 0;
	ULONG i;
	ULONG Val;
	BOOLEAN addneg = FALSE;

    *Value = 0;
	Str = String->Buffer;

	for (i = 0; i < String->Length / sizeof(WCHAR); i++)
	{
		if (*Str == L'b')
		{
			Base = 2;
			lenmin++;
		}
		else if (*Str == L'o')
		{
			Base = 8;
			lenmin++;
		}
		else if (*Str == L'd')
		{
			Base = 10;
			lenmin++;
		}
		else if (*Str == L'x')
		{
			Base = 16;
			lenmin++;
		}
		else if (*Str == L'+')
		{
			lenmin++;
		}
		else if (*Str == L'-')
		{
			addneg = TRUE;
			lenmin++;
		}
		else if ((*Str > L'1') && (Base == 2))
		{
			return STATUS_INVALID_PARAMETER;
		}
		else if (((*Str > L'7') || (*Str < L'0')) && (Base == 8))
		{
			return STATUS_INVALID_PARAMETER;
		}
		else if (((*Str > L'9') || (*Str < L'0')) && (Base == 10))
		{
			return STATUS_INVALID_PARAMETER;
		}
		else if ((((*Str > L'9') || (*Str < L'0')) ||
		          ((towupper (*Str) > L'F') ||
		           (towupper (*Str) < L'A'))) && (Base == 16))
		{
			return STATUS_INVALID_PARAMETER;
		}
		else
			Str++;
	}

	Str = String->Buffer + lenmin;

	if (Base == 0)
		Base = 10;

	while (iswxdigit (*Str) &&
	       (Val = iswdigit (*Str) ? *Str - L'0' : (iswlower (*Str)
	        ? toupper (*Str) : *Str) - L'A' + 10) < Base)
	{
		*Value = *Value * Base + Val;
		Str++;
	}

	if (addneg == TRUE)
		*Value *= -1;

	return STATUS_SUCCESS;
}


ULONG
STDCALL
RtlUnicodeStringToOemSize(IN PUNICODE_STRING UnicodeString)
{
  ULONG Size;

  RtlUnicodeToMultiByteSize(&Size,
			    UnicodeString->Buffer,
			    UnicodeString->Length);
  return(Size+1);
}


NTSTATUS
STDCALL
RtlUnicodeStringToOemString(IN OUT POEM_STRING DestinationString,
			    IN PUNICODE_STRING SourceString,
			    IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbOemCodePageTag == TRUE){
    Length = RtlUnicodeStringToAnsiSize (SourceString); Length--;
  }
  else
    Length = SourceString->Length / sizeof(WCHAR);
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(CHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag (NonPagedPool,
			       DestinationString->MaximumLength,
			       TAG_OSTR);
      if (DestinationString->Buffer == NULL)
	return STATUS_NO_MEMORY;
    }
  else
    {
      if (Length >= DestinationString->MaximumLength)
	return STATUS_BUFFER_TOO_SMALL;
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory(DestinationString->Buffer,
		DestinationString->Length);
  
  Status = RtlUnicodeToOemN(DestinationString->Buffer,
			    DestinationString->Length,
			    NULL,
			    SourceString->Buffer,
			    SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool(DestinationString->Buffer);
      return Status;
    }
  
  DestinationString->Buffer[Length] = 0;
  
  return STATUS_SUCCESS;
}


WCHAR
STDCALL
RtlUpcaseUnicodeChar(IN WCHAR Source)
{
  if (Source < L'a')
    return(Source);

  if (Source <= L'z')
    return(Source - (L'a' - L'A'));

  /* FIXME: characters above 'z' */

  return(Source);
}


NTSTATUS STDCALL
RtlUpcaseUnicodeString(IN OUT PUNICODE_STRING DestinationString,
		       IN PUNICODE_STRING SourceString,
		       IN BOOLEAN AllocateDestinationString)
{
  ULONG i;
  PWCHAR Src, Dest;
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
      DestinationString->Buffer = 
	ExAllocatePoolWithTag(NonPagedPool,
			      SourceString->Length + sizeof(WCHAR),
			      TAG_USTR);
      if (DestinationString->Buffer == NULL)
	return(STATUS_NO_MEMORY);
    }
  else
    {
      if (SourceString->Length >= DestinationString->MaximumLength)
	return(STATUS_BUFFER_TOO_SMALL);
    }
  DestinationString->Length = SourceString->Length;
  
  Src = SourceString->Buffer;
  Dest = DestinationString->Buffer;
  for (i=0; i < SourceString->Length / sizeof(WCHAR); i++)
    {
      *Dest = RtlUpcaseUnicodeChar(*Src);
      Dest++;
      Src++;
    }
  *Dest = 0;
  
  return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToAnsiString(IN OUT PANSI_STRING DestinationString,
				   IN PUNICODE_STRING SourceString,
				   IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  
  if (NlsMbCodePageTag == TRUE){
    Length = RtlUnicodeStringToAnsiSize(SourceString); Length--;
  }
  else
    Length = SourceString->Length / sizeof(WCHAR);
  
  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(CHAR);
      DestinationString->Buffer = 
	ExAllocatePoolWithTag(NonPagedPool,
			      DestinationString->MaximumLength,
			      TAG_ASTR);
      if (DestinationString->Buffer == NULL)
	return(STATUS_NO_MEMORY);
    }
  else
    {
      if (Length >= DestinationString->MaximumLength)
	return(STATUS_BUFFER_TOO_SMALL);
    }
  DestinationString->Length = Length;
  
  RtlZeroMemory(DestinationString->Buffer,
		DestinationString->Length);
  
  Status = RtlUpcaseUnicodeToMultiByteN(DestinationString->Buffer,
					DestinationString->Length,
					NULL,
					SourceString->Buffer,
					SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool(DestinationString->Buffer);
      return(Status);
    }
  
  DestinationString->Buffer[Length] = 0;
  
  return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToCountedOemString(IN OUT POEM_STRING DestinationString,
					 IN PUNICODE_STRING SourceString,
					 IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;
  ULONG Size;

  if (NlsMbCodePageTag == TRUE)
    Length = RtlUnicodeStringToAnsiSize (SourceString)/* + 1*/;
  else
    Length = SourceString->Length / sizeof(WCHAR) + 1;

  if (Length > 0x0000FFFF)
    return(STATUS_INVALID_PARAMETER_2);

  DestinationString->Length = (WORD)(Length - 1);

  if (AllocateDestinationString == TRUE)
    {
      DestinationString->Buffer =
	ExAllocatePoolWithTag(NonPagedPool,
			      Length,
			      TAG_OSTR);
      if (DestinationString->Buffer == NULL)
	return(STATUS_NO_MEMORY);

      RtlZeroMemory(DestinationString->Buffer,
		    Length);
      DestinationString->MaximumLength = (WORD)Length;
    }
  else
    {
      if (Length > DestinationString->MaximumLength)
	{
	  if (DestinationString->MaximumLength == 0)
	    return(STATUS_BUFFER_OVERFLOW);
	  DestinationString->Length =
	    DestinationString->MaximumLength - 1;
	}
    }

  Status = RtlUpcaseUnicodeToOemN(DestinationString->Buffer,
				  DestinationString->Length,
				  &Size,
				  SourceString->Buffer,
				  SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool(DestinationString->Buffer);
      return(Status);
    }

  DestinationString->Buffer[Size] = 0;

  return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
RtlUpcaseUnicodeStringToOemString(IN OUT POEM_STRING DestinationString,
				  IN PUNICODE_STRING SourceString,
				  IN BOOLEAN AllocateDestinationString)
{
  NTSTATUS Status;
  ULONG Length;

  if (NlsMbOemCodePageTag == TRUE) {
    Length = RtlUnicodeStringToOemSize(SourceString); Length--;
  }
  else
    Length = SourceString->Length / sizeof(WCHAR);

  if (AllocateDestinationString == TRUE)
    {
      DestinationString->MaximumLength = Length + sizeof(CHAR);
      DestinationString->Buffer =
	ExAllocatePoolWithTag(NonPagedPool,
			      DestinationString->MaximumLength,
			      TAG_OSTR);
      if (DestinationString->Buffer == NULL)
	return(STATUS_NO_MEMORY);
    }
  else
    {
      if (Length >= DestinationString->MaximumLength)
	return(STATUS_BUFFER_TOO_SMALL);
    }
  DestinationString->Length = Length;

  RtlZeroMemory(DestinationString->Buffer,
		DestinationString->Length);

  Status = RtlUpcaseUnicodeToOemN(DestinationString->Buffer,
				  DestinationString->Length,
				  NULL,
				  SourceString->Buffer,
				  SourceString->Length);
  if (!NT_SUCCESS(Status))
    {
      if (AllocateDestinationString)
	ExFreePool(DestinationString->Buffer);
      return(Status);
    }

  DestinationString->Buffer[Length] = 0;

  return(STATUS_SUCCESS);
}


CHAR
STDCALL
RtlUpperChar(IN CHAR Source)
{
  WCHAR Unicode;
  CHAR Destination;

	if (NlsMbCodePageTag == FALSE)
	{
		/* single-byte code page */
		/* ansi->unicode */
		Unicode = (WCHAR)Source;
#if 0
		Unicode = NlsAnsiToUnicodeData[Source];
#endif

		/* upcase conversion */
		Unicode = RtlUpcaseUnicodeChar (Unicode);

		/* unicode -> ansi */
		Destination = (CHAR)Unicode;
#if 0
		Destination = NlsUnicodeToAnsiData[Unicode];
#endif
	}
	else
	{
		/* single-byte code page */
		/* FIXME: implement the multi-byte stuff!! */
		Destination = Source;
	}

  return(Destination);
}


VOID
STDCALL
RtlUpperString(PSTRING DestinationString,
	       PSTRING SourceString)
{
  ULONG Length;
  ULONG i;
  PCHAR Src;
  PCHAR Dest;

  Length = min(SourceString->Length,
	       DestinationString->MaximumLength - 1);

  Src = SourceString->Buffer;
  Dest = DestinationString->Buffer;
  for (i = 0; i < Length; i++)
    {
      *Dest = RtlUpperChar(*Src);
      Src++;
      Dest++;
    }
  *Dest = 0;

  DestinationString->Length = SourceString->Length;
}


ULONG
STDCALL
RtlxAnsiStringToUnicodeSize(IN PANSI_STRING AnsiString)
{
  return RtlAnsiStringToUnicodeSize(AnsiString);
}


ULONG
STDCALL
RtlxOemStringToUnicodeSize(IN POEM_STRING OemString)
{
  return RtlOemStringToUnicodeSize((PANSI_STRING)OemString);
}


ULONG
STDCALL
RtlxUnicodeStringToAnsiSize(IN PUNICODE_STRING UnicodeString)
{
  return RtlUnicodeStringToAnsiSize(UnicodeString);
}


ULONG
STDCALL
RtlxUnicodeStringToOemSize(IN PUNICODE_STRING UnicodeString)
{
  return RtlUnicodeStringToOemSize(UnicodeString);
}

/* EOF */
