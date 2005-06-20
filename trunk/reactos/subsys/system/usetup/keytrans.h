/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: keytrans.h 12852 2005-01-06 13:58:04Z mf $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/keytrans.h
 * PURPOSE:         Keyboard translation functionality
 * PROGRAMMER:      Tinus
 */

#ifndef __KEYTRANS_H__
#define __KEYTRANS_H__

#include <ddk/ntddkbd.h>

NTSTATUS
IntTranslateKey(PKEYBOARD_INPUT_DATA InputData, KEY_EVENT_RECORD *Event);

#endif /* __KEYTRANS_H__ */

/* EOF */
