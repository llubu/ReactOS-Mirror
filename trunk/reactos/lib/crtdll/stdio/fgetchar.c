/* $Id: fgetchar.c,v 1.6 2003/07/11 17:25:32 royce Exp $
 *
 *  ReactOS msvcrt library
 *
 *  fgetchar.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <msvcrt/stdio.h>
#include <msvcrt/conio.h>

int	_fgetchar (void)
{
	return _getch();
}

/*
 * @implemented
 */
int	_fgetwchar (void)
{
	return _getch();
}
