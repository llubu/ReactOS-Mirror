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
#include "../pch.h"

#include "../rbuild.h"
#include "backend.h"

using std::string;
using std::vector;
using std::map;

map<string,Backend::Factory*>*
Backend::Factory::factories = NULL;
int
Backend::Factory::ref = 0;

Backend::Factory::Factory ( const std::string& name_, const std::string& description_ )
{
	string name(name_);
	strlwr ( &name[0] );
	if ( !ref++ )
		factories = new map<string,Factory*>;
	(*factories)[name] = this;
	m_name = name;
	m_description = description_;
}

Backend::Factory::~Factory ()
{
	if ( !--ref )
	{
		delete factories;
		factories = NULL;
	}
}

/*static*/ Backend*
Backend::Factory::Create ( const string& name,
                           Project& project,
                           Configuration& configuration )
{
	string sname ( name );
	strlwr ( &sname[0] );
	if ( !factories || !factories->size () )
		throw InvalidOperationException ( __FILE__,
		                                  __LINE__,
		                                  "No registered factories" );
	Backend::Factory* f = (*factories)[sname];
	if ( !f )
	{
		throw UnknownBackendException ( sname );
		return NULL;
	}
	return (*f) ( project, configuration );
}

Backend::Backend ( Project& project,
                   Configuration& configuration )
	: ProjectNode ( project ),
	  configuration ( configuration )
{
}

Backend::~Backend()
{
}

string
Backend::GetFullPath ( const FileLocation& file ) const
{
	string directory;
	switch ( file.directory )
	{
		case SourceDirectory:
			directory = "";
			break;
		case IntermediateDirectory:
			directory = Environment::GetIntermediatePath ();
			break;
		case OutputDirectory:
			directory = Environment::GetOutputPath ();
			break;
		case InstallDirectory:
			directory = Environment::GetInstallPath ();
			break;
		default:
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %d.",
			                                  file.directory );
	}

	if ( file.relative_path.length () > 0 )
	{
		if ( directory.length () > 0 )
			directory += sSep;
		directory += file.relative_path;
	}
	return directory;
}

string
Backend::GetFullName ( const FileLocation& file ) const
{
	string directory = GetFullPath ( file );

	if ( directory.length () > 0 )
		directory += sSep;

	return directory + file.name;
}
