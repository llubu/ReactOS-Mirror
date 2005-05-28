// rbuild.cpp

#include "pch.h"
#include <typeinfo>

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#endif
#include <assert.h>

#include "rbuild.h"
#include "backend/backend.h"
#include "backend/mingw/mingw.h"

using std::string;
using std::vector;

static string BuildSystem;
static string RootXmlFile = "ReactOS.xml";
static Configuration configuration;

bool
ParseMakeSwitch ( char switchChar2 )
{
	switch ( switchChar2 )
	{
		case 'i':
			configuration.MakeHandlesInstallDirectories = true;
			break;
		default:
			printf ( "Unknown switch -m%c",
			         switchChar2 );
			return false;
	}
	return true;
}

bool
ParseSwitch ( int argc, char** argv, int index )
{
	char switchChar = strlen ( argv[index] ) > 1 ? argv[index][1] : ' ';
	char switchChar2 = strlen ( argv[index] ) > 2 ? argv[index][2] : ' ';
	switch ( switchChar )
	{
		case 'v':
			configuration.Verbose = true;
			break;
		case 'c':
			configuration.CleanAsYouGo = true;
			break;
		case 'd':
			configuration.AutomaticDependencies = false;
			break;
		case 'r':
			RootXmlFile = string(&argv[index][2]);
			break;
		case 'm':
			return ParseMakeSwitch ( switchChar2 );
		default:
			printf ( "Unknown switch -%c",
			         switchChar );
			return false;
	}
	return true;
}

bool
ParseArguments ( int argc, char** argv )
{
	if ( argc < 2 )
		return false;
	
	for ( int i = 1; i < argc; i++ )
	{
		if ( argv[i][0] == '-' )
		{
			if ( !ParseSwitch ( argc, argv, i ) )
				return false;
		}
		else
			BuildSystem = argv[i];
	}
	
	return true;
}

int
main ( int argc, char** argv )
{
	if ( !ParseArguments ( argc, argv ) )
	{
		printf ( "Generates project files for buildsystems\n\n" );
		printf ( "  rbuild [switches] buildsystem\n\n" );
		printf ( "Switches:\n" );
		printf ( "  -v           Be verbose.\n" );
		printf ( "  -c           Clean as you go. Delete generated files as soon as they are not needed anymore.\n" );
		printf ( "  -d           Disable automatic dependencies.\n" );
		printf ( "  -rfile.xml   Name of the root xml file. Default is ReactOS.xml.\n" );
		printf ( "  -mi          Let make handle creation of install directories. Rbuild will not generate the directories.\n" );
		printf ( "\n" );
		printf ( "  buildsystem  Target build system. Can be one of:\n" );
		printf ( "                 mingw   MinGW\n" );
		printf ( "                 devcpp  DevC++\n" );
		return 1;
	}
	try
	{
		string projectFilename ( RootXmlFile );
		printf ( "Reading build files..." );
		Project project ( projectFilename );
		printf ( "done\n" );
		project.WriteConfigurationFile ();
		project.ExecuteInvocations ();
		Backend* backend = Backend::Factory::Create ( BuildSystem,
		                                              project,
		                                              configuration );
		backend->Process ();
		delete backend;

		return 0;
	}
	catch (Exception& ex)
	{
		printf ( "%s\n",
		         ex.Message.c_str () );
		return 1;
	}
}
