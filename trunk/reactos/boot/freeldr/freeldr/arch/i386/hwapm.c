/*
 *  FreeLoader
 *
 *  Copyright (C) 2004  Eric Kohl
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

static BOOLEAN
FindApmBios(VOID)
{
  REGS  RegsIn;
  REGS  RegsOut;

  RegsIn.b.ah = 0x53;
  RegsIn.b.al = 0x00;
  RegsIn.w.bx = 0x0000;

  Int386(0x15, &RegsIn, &RegsOut);

  if (INT386_SUCCESS(RegsOut))
    {
      DPRINTM(DPRINT_HWDETECT, "Found APM BIOS\n");
      DPRINTM(DPRINT_HWDETECT, "AH: %x\n", RegsOut.b.ah);
      DPRINTM(DPRINT_HWDETECT, "AL: %x\n", RegsOut.b.al);
      DPRINTM(DPRINT_HWDETECT, "BH: %x\n", RegsOut.b.bh);
      DPRINTM(DPRINT_HWDETECT, "BL: %x\n", RegsOut.b.bl);
      DPRINTM(DPRINT_HWDETECT, "CX: %x\n", RegsOut.w.cx);

      return TRUE;
    }

  DPRINTM(DPRINT_HWDETECT, "No APM BIOS found\n");

  return FALSE;
}


VOID
DetectApmBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    CM_PARTIAL_RESOURCE_LIST PartialResourceList;

    if (FindApmBios())
    {
        /* Create 'Configuration Data' value */
        memset(&PartialResourceList, 0, sizeof(CM_PARTIAL_RESOURCE_LIST));
        PartialResourceList.Version = 0;
        PartialResourceList.Revision = 0;
        PartialResourceList.Count = 0;

        /* Create new bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0x0,
                               0x0,
                               0xFFFFFFFF,
                               "APM",
                               &PartialResourceList,
                               sizeof(CM_PARTIAL_RESOURCE_LIST) -
                                   sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),
                               &BiosKey);

        /* Increment bus number */
        (*BusNumber)++;
    }

    /* FIXME: Add configuration data */
}

/* EOF */
