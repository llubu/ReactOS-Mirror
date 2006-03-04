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
/* $Id$
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/mkhive.c
 * PURPOSE:         Hive maker
 * PROGRAMMER:      Eric Kohl
 */

#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "mkhive.h"
#include "registry.h"
#include "reginf.h"
#include "binhive.h"

#ifdef _MSC_VER
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#endif//_MSC_VER

#ifndef WIN32
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif


void usage (void)
{
  printf ("Usage: mkhive <srcdir> <dstdir> [addinf]\n\n");
  printf ("  srcdir  - inf files are read from this directory\n");
  printf ("  dstdir  - binary hive files are created in this directory\n");
  printf ("  addinf  - additional inf files with full path\n");
}

void convert_path(char *dst, char *src)
{
  int i;

  i = 0;
  while (src[i] != 0)
    {
#ifdef WIN32
      if (src[i] == '/')
	{
	  dst[i] = '\\';
	}
#else
      if (src[i] == '\\')
	{
	  dst[i] = '/';
	}
#endif
      else
	{
	  dst[i] = src[i];
	}

      i++;
    }
  dst[i] = 0;
}


int main (int argc, char *argv[])
{
  char FileName[PATH_MAX];
  int Param;

  printf ("Binary hive maker\n");

  if (argc < 3)
    {
      usage ();
      return 1;
    }

  RegInitializeRegistry ();

  convert_path (FileName, argv[1]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "hivesys.inf");
  ImportRegistryFile (FileName, "AddReg", FALSE);

  convert_path (FileName, argv[1]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "hivecls.inf");
  ImportRegistryFile (FileName, "AddReg", FALSE);

  convert_path (FileName, argv[1]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "hivesft.inf");
  ImportRegistryFile (FileName, "AddReg", FALSE);

  convert_path (FileName, argv[1]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "hivedef.inf");
  ImportRegistryFile (FileName, "AddReg", FALSE);

  for (Param = 3; Param < argc; Param++)
  {
    convert_path (FileName, argv[Param]);
    ImportRegistryFile (FileName, "AddReg", FALSE);
  }

  convert_path (FileName, argv[2]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "system");
  if (!ExportBinaryHive (FileName, "\\Registry\\Machine\\SYSTEM"))
  {
     return 1;
  }

  convert_path (FileName, argv[2]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "software");
  if (!ExportBinaryHive (FileName, "\\Registry\\Machine\\SOFTWARE"))
  {
     return 1;
  }

  convert_path (FileName, argv[2]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "sam");
  if (!ExportBinaryHive (FileName, "\\Registry\\Machine\\SAM"))
  {
     return 1;
  }

  convert_path (FileName, argv[2]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "security");
  if (!ExportBinaryHive (FileName, "\\Registry\\Machine\\SECURITY"))
  {
     return 1;
  }

  convert_path (FileName, argv[2]);
  strcat (FileName, DIR_SEPARATOR_STRING);
  strcat (FileName, "default");
  if (!ExportBinaryHive (FileName, "\\Registry\\User\\.DEFAULT"))
  {
     return 1;
  }

//  RegShutdownRegistry ();

  printf ("  Done.\n");

  return 0;
}

/* EOF */
