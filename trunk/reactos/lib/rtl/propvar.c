/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/propvar.c
 * PURPOSE:           Native properties and variants API
 * PROGRAMMER:        
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
PropertyLengthAsVariant (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	return (STATUS_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
RtlCompareVariants (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	return (FALSE);
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
RtlConvertPropertyToVariant (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	return (FALSE);
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
RtlConvertVariantToProperty (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6
	)
{
	return (STATUS_NOT_IMPLEMENTED);
}


/* EOF */
