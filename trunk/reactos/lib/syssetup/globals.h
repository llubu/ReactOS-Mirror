/*
 * Copyright (C) 2004 Eric Kohl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

typedef struct _SETUPDATA
{
  TCHAR OwnerName[51];
  TCHAR OwnerOrganization[51];
  TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];	/* max. 63 characters */
  TCHAR AdminPassword[15];				/* max. 14 characters */

} SETUPDATA, *PSETUPDATA;


extern HINSTANCE hDllInstance;
extern HINF hSysSetupInf;

/* wizard.c */
VOID InstallWizard (VOID);

/* EOF */
