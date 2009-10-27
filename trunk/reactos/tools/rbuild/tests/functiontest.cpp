/*
 * Copyright (C) 2005 Casper S. Hornstrup
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "test.h"

using std::string;

void FunctionTest::Run ()
{
	string fixedupFilename = NormalizeFilename ( "." SSEP "dir1" SSEP "dir2" SSEP ".." SSEP "filename.txt" );
	ARE_EQUAL ( "dir1" SSEP "filename.txt", fixedupFilename );
	ARE_EQUAL ( "file.txt", GetFilename ( "file.txt" ) );
	ARE_EQUAL ( "file.txt", GetFilename ( "dir" SSEP "file.txt" ) );
}
