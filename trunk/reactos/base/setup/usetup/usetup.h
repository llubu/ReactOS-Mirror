/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/usetup.h
 * PURPOSE:         Text-mode setup
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __USETUP_H__
#define __USETUP_H__

/* C Headers */
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>

/* PSDK/NDK */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <fmifs/fmifs.h>

/* VFAT */
#include <fslib/vfatlib.h>

/* DDK Disk Headers */
#include <ntddscsi.h>

/* Blue Driver Header */
#include <blue/ntddblue.h>

/* Helper Header */
#include <reactos/helper.h>

/* ReactOS Version */
#include <reactos/buildno.h>

/* Internal Headers */
#include "console.h"
#include "partlist.h"
#include "inffile.h"
#include "inicache.h"
#include "progress.h"
#include "filequeue.h"
#include "bootsup.h"
#include "keytrans.h"
#include "registry.h"
#include "fslist.h"
#include "format.h"
#include "cabinet.h"
#include "filesup.h"
#include "drivesup.h"
#include "genlist.h"
#include "settings.h"

extern HANDLE ProcessHeap;
extern UNICODE_STRING SourceRootPath;

#endif /* __USETUP_H__*/

/* EOF */
