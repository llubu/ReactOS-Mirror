/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: callback.c,v 1.10 2003/07/10 06:27:13 royce Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/callback.c
 * PURPOSE:         Executive callbacks
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * PORTABILITY:     Checked.
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/*
 * NOTE:
 *	These funtions are not implemented in NT4.
 *	They are implemented in Win2k.
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
ExCreateCallback (
	OUT	PCALLBACK_OBJECT	* CallbackObject,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	BOOLEAN			Create,
	IN	BOOLEAN			AllowMultipleCallbacks
	)
{
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
ExNotifyCallback (
	IN	PCALLBACK_OBJECT	CallbackObject,
	IN	PVOID	Argument1,
	IN	PVOID	Argument2
	)
{
	return;
}

/*
 * @unimplemented
 */
PVOID
STDCALL
ExRegisterCallback (
	IN	PCALLBACK_OBJECT	CallbackObject,
	IN	PCALLBACK_FUNCTION	CallbackFunction,
	IN	PVOID			CallbackContext
	)
{
	return NULL;
}

/*
 * @unimplemented
 */
VOID
STDCALL
ExUnregisterCallback (
	IN	PVOID	CallbackRegistration
	)
{
	return;
}

/* EOF */
