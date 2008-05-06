/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *               2007-2008 Herv� Poussineau
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
#include "../../pch.h"
#include <assert.h>
#include <algorithm>

#include "../../rbuild.h"
#include "mingw.h"
#include "modulehandler.h"
#include "rule.h"

using std::set;
using std::string;
using std::vector;

#define CLEAN_FILE(f) clean_files.push_back ( (f).name.length () > 0 ? backend->GetFullName ( f ) : backend->GetFullPath ( f ) );
#define IsStaticLibrary( module ) ( ( module.type == StaticLibrary ) || ( module.type == HostStaticLibrary ) )

MingwBackend*
MingwModuleHandler::backend = NULL;
FILE*
MingwModuleHandler::fMakefile = NULL;

string
PrefixFilename (
	const string& filename,
	const string& prefix )
{
	if ( !prefix.length() )
		return filename;
	string out;
	const char* pfilename = filename.c_str();
	const char* p1 = strrchr ( pfilename, '/' );
	const char* p2 = strrchr ( pfilename, '\\' );
	if ( p1 || p2 )
	{
		if ( p2 > p1 )
			p1 = p2;
		out += string(pfilename,p1-pfilename) + cSep;
		pfilename = p1 + 1;
	}
	out += prefix + pfilename;
	return out;
}

string
GetTargetMacro ( const Module& module, bool with_dollar )
{
	string s ( module.name );
	strupr ( &s[0] );
	s += "_TARGET";
	if ( with_dollar )
		return ssprintf ( "$(%s)", s.c_str() );
	return s;
}

MingwModuleHandler::MingwModuleHandler (
	const Module& module_ )

	: module(module_)
{
	use_pch = false;
}

MingwModuleHandler::~MingwModuleHandler()
{
}

/*static*/ void
MingwModuleHandler::SetBackend ( MingwBackend* backend_ )
{
	backend = backend_;
}

/*static*/ void
MingwModuleHandler::SetMakefile ( FILE* f )
{
	fMakefile = f;
}

void
MingwModuleHandler::EnablePreCompiledHeaderSupport ()
{
	use_pch = true;
}

/*static*/ const FileLocation*
MingwModuleHandler::PassThruCacheDirectory (const FileLocation* file )
{
	switch ( file->directory )
	{
		case SourceDirectory:
			break;
		case IntermediateDirectory:
			backend->AddDirectoryTarget ( file->relative_path, backend->intermediateDirectory );
			break;
		case OutputDirectory:
			backend->AddDirectoryTarget ( file->relative_path, backend->outputDirectory );
			break;
		case InstallDirectory:
			backend->AddDirectoryTarget ( file->relative_path, backend->installDirectory );
			break;
		default:
			throw InvalidOperationException ( __FILE__,
			                                  __LINE__,
			                                  "Invalid directory %d.",
			                                  file->directory );
	}

	return file;
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetTargetFilename (
	const Module& module,
	string_list* pclean_files )
{
	FileLocation *target = new FileLocation ( *module.output );
	if ( pclean_files )
	{
		string_list& clean_files = *pclean_files;
		CLEAN_FILE ( *target );
	}
	return target;
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetImportLibraryFilename (
	const Module& module,
	string_list* pclean_files )
{
	FileLocation *target = new FileLocation ( *module.dependency );
	if ( pclean_files )
	{
		string_list& clean_files = *pclean_files;
		CLEAN_FILE ( *target );
	}
	return target;
}

/* caller needs to delete the returned object */
MingwModuleHandler*
MingwModuleHandler::InstanciateHandler (
	const Module& module,
	MingwBackend* backend )
{
	MingwModuleHandler* handler;
	switch ( module.type )
	{
		case BuildTool:
			handler = new MingwBuildToolModuleHandler ( module );
			break;
		case StaticLibrary:
			handler = new MingwStaticLibraryModuleHandler ( module );
			break;
		case HostStaticLibrary:
			handler = new MingwHostStaticLibraryModuleHandler ( module );
			break;
		case ObjectLibrary:
			handler = new MingwObjectLibraryModuleHandler ( module );
			break;
		case Kernel:
			handler = new MingwKernelModuleHandler ( module );
			break;
		case NativeCUI:
			handler = new MingwNativeCUIModuleHandler ( module );
			break;
		case Win32CUI:
			handler = new MingwWin32CUIModuleHandler ( module );
			break;
		case Win32SCR:
		case Win32GUI:
			handler = new MingwWin32GUIModuleHandler ( module );
			break;
		case KernelModeDLL:
			handler = new MingwKernelModeDLLModuleHandler ( module );
			break;
		case NativeDLL:
			handler = new MingwNativeDLLModuleHandler ( module );
			break;
		case Win32DLL:
			handler = new MingwWin32DLLModuleHandler ( module );
			break;
		case Win32OCX:
			handler = new MingwWin32OCXModuleHandler ( module );
			break;
		case KernelModeDriver:
			handler = new MingwKernelModeDriverModuleHandler ( module );
			break;
		case BootLoader:
			handler = new MingwBootLoaderModuleHandler ( module );
			break;
		case BootSector:
			handler = new MingwBootSectorModuleHandler ( module );
			break;
		case BootProgram:
			handler = new MingwBootProgramModuleHandler ( module );
			break;
		case Iso:
			handler = new MingwIsoModuleHandler ( module );
			break;
		case LiveIso:
			handler = new MingwLiveIsoModuleHandler ( module );
			break;
		case IsoRegTest:
			handler = new MingwIsoModuleHandler ( module );
			break;
		case LiveIsoRegTest:
			handler = new MingwLiveIsoModuleHandler ( module );
			break;
		case Test:
			handler = new MingwTestModuleHandler ( module );
			break;
		case RpcServer:
			handler = new MingwRpcServerModuleHandler ( module );
			break;
		case RpcClient:
			handler = new MingwRpcClientModuleHandler ( module );
			break;
		case RpcProxy:
			handler = new MingwRpcProxyModuleHandler ( module );
			break;
		case Alias:
			handler = new MingwAliasModuleHandler ( module );
			break;
		case IdlHeader:
			handler = new MingwIdlHeaderModuleHandler ( module );
			break;
		case Cabinet:
			handler = new MingwCabinetModuleHandler ( module );
			break;
		case EmbeddedTypeLib:
			handler = new MingwEmbeddedTypeLibModuleHandler ( module );
			break;
		case ElfExecutable:
			handler = new MingwElfExecutableModuleHandler ( module );
			break;
		default:
			throw UnknownModuleTypeException (
				module.node.location,
				module.type );
			break;
	}
	return handler;
}

string
MingwModuleHandler::GetWorkingDirectory () const
{
	return ".";
}

string
MingwModuleHandler::GetBasename ( const string& filename ) const
{
	size_t index = filename.find_last_of ( '.' );
	if ( index != string::npos )
		return filename.substr ( 0, index );
	return "";
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetActualSourceFilename (
	const FileLocation* file ) const
{
	string filename = file->name;

	string extension = GetExtension ( *file );
	if ( extension == ".spec" || extension == ".SPEC" )
	{
		const FileLocation *objectFile = GetObjectFilename ( file, module );
		FileLocation *sourceFile = new FileLocation (
			objectFile->directory,
			objectFile->relative_path,
			ReplaceExtension ( objectFile->name, ".c" ) );
		delete objectFile;
		return sourceFile;
	}
	else if ( ( extension == ".idl" || extension == ".IDL" ) &&
	          ( module.type == RpcServer || module.type == RpcClient || module.type == RpcProxy ) )
	{
		const FileLocation *objectFile = GetObjectFilename ( file, module );
		FileLocation *sourceFile = new FileLocation (
			objectFile->directory,
			objectFile->relative_path,
			ReplaceExtension ( objectFile->name, ".c" ) );
		delete objectFile;
		return sourceFile;
	}
	else if ( extension == ".mc" || extension == ".MC" )
	{
		const FileLocation *objectFile = GetObjectFilename ( file, module );
		FileLocation *sourceFile = new FileLocation (
			objectFile->directory,
			objectFile->relative_path,
			ReplaceExtension ( objectFile->name, ".rc" ) );
		delete objectFile;
		return sourceFile;
	}
	else
		return new FileLocation ( *file );
}

string
MingwModuleHandler::GetExtraDependencies (
	const FileLocation *file ) const
{
	string extension = GetExtension ( *file );
	if ( extension == ".idl" || extension == ".IDL" )
	{
		const FileLocation *header;
		switch ( module.type )
		{
			case RpcServer: header = GetRpcServerHeaderFilename ( file ); break;
			case RpcClient: header = GetRpcClientHeaderFilename ( file ); break;
			case RpcProxy: header = GetRpcProxyHeaderFilename ( file ); break;
			case IdlHeader: header = GetIdlHeaderFilename ( file ); break;
			default: header = NULL; break;
		}
		if ( !header )
			return "";

		string dependencies = backend->GetFullName ( *header );
		delete header;
		return " " + dependencies;
	}
	else
		return "";
}

string
MingwModuleHandler::GetCompilationUnitDependencies (
	const CompilationUnit& compilationUnit ) const
{
	if ( compilationUnit.GetFiles ().size () <= 1 )
		return "";
	vector<string> sourceFiles;
	for ( size_t i = 0; i < compilationUnit.GetFiles ().size (); i++ )
	{
		const File& file = *compilationUnit.GetFiles ()[i];
		sourceFiles.push_back ( backend->GetFullName ( file.file ) );
	}
	return string ( " " ) + v2s ( sourceFiles, 10 );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetModuleArchiveFilename () const
{
	if ( IsStaticLibrary ( module ) )
		return GetTargetFilename ( module, NULL );
	return new FileLocation ( IntermediateDirectory,
	                          module.output->relative_path,
	                          ReplaceExtension ( module.name, ".temp.a" ) );
}

bool
MingwModuleHandler::IsGeneratedFile ( const File& file ) const
{
	string extension = GetExtension ( file.file );
	return ( extension == ".spec" || extension == ".SPEC" );
}

/*static*/ bool
MingwModuleHandler::ReferenceObjects (
	const Module& module )
{
	if ( module.type == ObjectLibrary )
		return true;
	if ( module.type == RpcServer )
		return true;
	if ( module.type == RpcClient )
		return true;
	if ( module.type == RpcProxy )
		return true;
	if ( module.type == IdlHeader )
		return true;
	return false;
}

void
MingwModuleHandler::OutputCopyCommand ( const FileLocation& source,
                                        const FileLocation& destination )
{
	fprintf ( fMakefile,
	          "\t$(ECHO_CP)\n" );
	fprintf ( fMakefile,
	          "\t${cp} %s %s 1>$(NUL)\n",
	          backend->GetFullName ( source ).c_str (),
	          backend->GetFullName ( *PassThruCacheDirectory ( &destination ) ).c_str () );
}

string
MingwModuleHandler::GetImportLibraryDependency (
	const Module& importedModule )
{
	string dep;
	if ( ReferenceObjects ( importedModule ) )
	{
		const vector<CompilationUnit*>& compilationUnits = importedModule.non_if_data.compilationUnits;
		size_t i;

		dep = GetTargetMacro ( importedModule );
		for ( i = 0; i < compilationUnits.size (); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			const FileLocation& compilationName = compilationUnit.GetFilename ();
			const FileLocation *objectFilename = GetObjectFilename ( &compilationName, importedModule );
			if ( GetExtension ( *objectFilename ) == ".h" )
				dep += ssprintf ( " $(%s_HEADERS)", importedModule.name.c_str () );
			else if ( GetExtension ( *objectFilename ) == ".rc" )
				dep += ssprintf ( " $(%s_MCHEADERS)", importedModule.name.c_str () );
		}
	}
	else
	{
		const FileLocation *library_target = GetImportLibraryFilename ( importedModule, NULL );
		dep = backend->GetFullName ( *library_target );
		delete library_target;
	}
	return dep;
}

void
MingwModuleHandler::GetTargets ( const Module& dependencyModule,
                                 string_list& targets )
{
	if ( dependencyModule.invocations.size () > 0 )
	{
		for ( size_t i = 0; i < dependencyModule.invocations.size (); i++ )
		{
			Invoke& invoke = *dependencyModule.invocations[i];
			invoke.GetTargets ( targets );
		}
	}
	else
		targets.push_back ( GetImportLibraryDependency ( dependencyModule ) );
}

void
MingwModuleHandler::GetModuleDependencies (
	string_list& dependencies )
{
	size_t iend = module.dependencies.size ();

	if ( iend == 0 )
		return;

	for ( size_t i = 0; i < iend; i++ )
	{
		const Dependency& dependency = *module.dependencies[i];
		const Module& dependencyModule = *dependency.dependencyModule;
		GetTargets ( dependencyModule,
		             dependencies );
	}
	vector<FileLocation> v;
	GetDefinitionDependencies ( v );

	for ( size_t i = 0; i < v.size (); i++ )
	{
		const FileLocation& file = v[i];
		dependencies.push_back ( backend->GetFullName ( file ) );
	}
}

void
MingwModuleHandler::GetSourceFilenames ( vector<FileLocation>& list,
                                         bool includeGeneratedFiles ) const
{
	size_t i;

	const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
	for ( i = 0; i < compilationUnits.size (); i++ )
	{
		if ( includeGeneratedFiles || !compilationUnits[i]->IsGeneratedFile () )
		{
			const FileLocation& compilationName = compilationUnits[i]->GetFilename ();
			const FileLocation* sourceFileLocation = GetActualSourceFilename ( &compilationName );
			list.push_back ( *sourceFileLocation );
			delete sourceFileLocation;
		}
	}
	// intentionally make a copy so that we can append more work in
	// the middle of processing without having to go recursive
	vector<If*> v = module.non_if_data.ifs;
	for ( i = 0; i < v.size (); i++ )
	{
		size_t j;
		If& rIf = *v[i];
		// check for sub-ifs to add to list
		const vector<If*>& ifs = rIf.data.ifs;
		for ( j = 0; j < ifs.size (); j++ )
			v.push_back ( ifs[j] );
		const vector<CompilationUnit*>& compilationUnits = rIf.data.compilationUnits;
		for ( j = 0; j < compilationUnits.size (); j++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[j];
			if ( includeGeneratedFiles || !compilationUnit.IsGeneratedFile () )
			{
				const FileLocation& compilationName = compilationUnit.GetFilename ();
				const FileLocation* sourceFileLocation = GetActualSourceFilename ( &compilationName );
				list.push_back ( *sourceFileLocation );
				delete sourceFileLocation;
			}
		}
	}
}

void
MingwModuleHandler::GetSourceFilenamesWithoutGeneratedFiles (
	vector<FileLocation>& list ) const
{
	GetSourceFilenames ( list, false );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetObjectFilename (
	const FileLocation* sourceFile,
	const Module& module ) const
{
	DirectoryLocation destination_directory;
	string newExtension;
	string extension = GetExtension ( *sourceFile );

	if ( module.type == BootSector )
		return new FileLocation ( *module.output );
	else if ( extension == ".rc" || extension == ".RC" )
		newExtension = "_" + module.name + ".coff";
	else if ( extension == ".mc" || extension == ".MC" )
		newExtension = ".rc";
	else if ( extension == ".spec" || extension == ".SPEC" )
		newExtension = ".stubs.o";
	else if ( extension == ".idl" || extension == ".IDL" )
	{
		if ( module.type == RpcServer )
			newExtension = "_s.o";
		else if ( module.type == RpcClient )
			newExtension = "_c.o";
		else if ( module.type == RpcProxy )
			newExtension = "_p.o";
		else
			newExtension = ".h";
	}
	else
		newExtension = "_" + module.name + ".o";

	if ( module.type == BootSector )
		destination_directory = OutputDirectory;
	else
		destination_directory = IntermediateDirectory;

	const FileLocation *obj_file = new FileLocation(
		destination_directory,
		sourceFile->relative_path,
		ReplaceExtension ( sourceFile->name, newExtension ) );
	PassThruCacheDirectory ( obj_file );

	return obj_file;
}

string
MingwModuleHandler::GetModuleCleanTarget ( const Module& module ) const
{
	return module.name + "_clean";
}

void
MingwModuleHandler::GetReferencedObjectLibraryModuleCleanTargets ( vector<string>& moduleNames ) const
{
	for ( size_t i = 0; i < module.non_if_data.libraries.size (); i++ )
	{
		Library& library = *module.non_if_data.libraries[i];
		if ( library.importedModule->type == ObjectLibrary )
			moduleNames.push_back ( GetModuleCleanTarget ( *library.importedModule ) );
	}
}

void
MingwModuleHandler::GenerateCleanTarget () const
{
	if ( module.type == Alias )
		return;

	fprintf ( fMakefile,
	          ".PHONY: %s_clean\n",
	          module.name.c_str() );
	vector<string> referencedModuleNames;
	GetReferencedObjectLibraryModuleCleanTargets ( referencedModuleNames );
	fprintf ( fMakefile,
	          "%s: %s\n\t-@${rm}",
	          GetModuleCleanTarget ( module ).c_str(),
	          v2s ( referencedModuleNames, 10 ).c_str () );
	for ( size_t i = 0; i < clean_files.size(); i++ )
	{
		if ( ( i + 1 ) % 10 == 9 )
			fprintf ( fMakefile, " 2>$(NUL)\n\t-@${rm}" );
		fprintf ( fMakefile, " %s", clean_files[i].c_str() );
	}
	fprintf ( fMakefile, " 2>$(NUL)\n" );

	if( ProxyMakefile::GenerateProxyMakefile(module) )
	{
		DirectoryLocation root;

		if ( backend->configuration.GenerateProxyMakefilesInSourceTree )
			root = SourceDirectory;
		else
			root = OutputDirectory;

		FileLocation proxyMakefile ( root,
		                             module.output->relative_path,
		                            "GNUmakefile" );
		fprintf ( fMakefile, "\t-@${rm} %s 2>$(NUL)\n",
		          backend->GetFullName ( proxyMakefile ).c_str () );
	}

	fprintf ( fMakefile, "clean: %s_clean\n\n", module.name.c_str() );
}

void
MingwModuleHandler::GenerateInstallTarget () const
{
	if ( !module.install )
		return;
	fprintf ( fMakefile, ".PHONY: %s_install\n", module.name.c_str() );
	fprintf ( fMakefile,
	          "%s_install: %s\n",
	          module.name.c_str (),
	          backend->GetFullName ( *module.install ).c_str () );
}

void
MingwModuleHandler::GenerateDependsTarget () const
{
	fprintf ( fMakefile,
	          ".PHONY: %s_depends\n",
	          module.name.c_str() );
	fprintf ( fMakefile,
	          "%s_depends: $(RBUILD_TARGET)\n",
	          module.name.c_str () );
	fprintf ( fMakefile,
	          "\t$(ECHO_RBUILD)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(RBUILD_TARGET) $(RBUILD_FLAGS) -dm%s mingw\n",
	          module.name.c_str () );
}

string
MingwModuleHandler::GetObjectFilenames ()
{
	const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
	if ( compilationUnits.size () == 0 )
		return "";

	string objectFilenames ( "" );
	for ( size_t i = 0; i < compilationUnits.size (); i++ )
	{
		if ( objectFilenames.size () > 0 )
			objectFilenames += " ";
		const FileLocation& compilationName = compilationUnits[i]->GetFilename ();
		const FileLocation *object_file = GetObjectFilename ( &compilationName, module );
		objectFilenames += backend->GetFullName ( *object_file );
		delete object_file;
	}
	return objectFilenames;
}

/* static */ string
MingwModuleHandler::GenerateGccDefineParametersFromVector (
	const vector<Define*>& defines,
	set<string>& used_defs)
{
	string parameters;

	for ( size_t i = 0; i < defines.size (); i++ )
	{
		Define& define = *defines[i];
		if (used_defs.find(define.name) != used_defs.end())
			continue;
		if (parameters.length () > 0)
			parameters += " ";
		if (define.name.find('(') != string::npos)
			parameters += "$(QT)";
		parameters += "-D";
		parameters += define.name;
		if (define.value.length () > 0)
		{
			parameters += "=";
			parameters += define.value;
		}
		if (define.name.find('(') != string::npos)
			parameters += "$(QT)";
		used_defs.insert(used_defs.begin(),define.name);
	}
	return parameters;
}

string
MingwModuleHandler::GenerateGccDefineParameters () const
{
	set<string> used_defs;
	string parameters = GenerateGccDefineParametersFromVector ( module.project.non_if_data.defines, used_defs );
	string s = GenerateGccDefineParametersFromVector ( module.non_if_data.defines, used_defs );
	if ( s.length () > 0 )
	{
		parameters += " ";
		parameters += s;
	}
	return parameters;
}

string
MingwModuleHandler::ConcatenatePaths (
	const string& path1,
	const string& path2 ) const
{
	if ( ( path1.length () == 0 ) || ( path1 == "." ) || ( path1 == "./" ) )
		return path2;
	if ( path1[path1.length ()] == cSep )
		return path1 + path2;
	else
		return path1 + cSep + path2;
}

/* static */ string
MingwModuleHandler::GenerateGccIncludeParametersFromVector ( const vector<Include*>& includes )
{
	string parameters, path_prefix;
	for ( size_t i = 0; i < includes.size (); i++ )
	{
		Include& include = *includes[i];
		if ( parameters.length () > 0 )
			parameters += " ";
		parameters += "-I" + backend->GetFullPath ( *include.directory );;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateGccIncludeParameters () const
{
	string parameters = GenerateGccIncludeParametersFromVector ( module.non_if_data.includes );
	string s = GenerateGccIncludeParametersFromVector ( module.project.non_if_data.includes );
	if ( s.length () > 0 )
	{
		parameters += " ";
		parameters += s;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateCompilerParametersFromVector ( const vector<CompilerFlag*>& compilerFlags, const CompilerType type ) const
{
	string parameters;
	for ( size_t i = 0; i < compilerFlags.size (); i++ )
	{
		CompilerFlag& compilerFlag = *compilerFlags[i];
		if ( compilerFlag.compiler == type )
			parameters += " " + compilerFlag.flag;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateLinkerParametersFromVector ( const vector<LinkerFlag*>& linkerFlags ) const
{
	string parameters;
	for ( size_t i = 0; i < linkerFlags.size (); i++ )
	{
		LinkerFlag& linkerFlag = *linkerFlags[i];
		if ( parameters.length () > 0 )
			parameters += " ";
		parameters += linkerFlag.flag;
	}
	return parameters;
}

string
MingwModuleHandler::GenerateImportLibraryDependenciesFromVector (
	const vector<Library*>& libraries )
{
	string dependencies ( "" );
	int wrap_count = 0;
	for ( size_t i = 0; i < libraries.size (); i++ )
	{
		if ( wrap_count++ == 5 )
			dependencies += " \\\n\t\t", wrap_count = 0;
		else if ( dependencies.size () > 0 )
			dependencies += " ";
		dependencies += GetImportLibraryDependency ( *libraries[i]->importedModule );
	}
	return dependencies;
}

string
MingwModuleHandler::GenerateLinkerParameters () const
{
	return GenerateLinkerParametersFromVector ( module.linkerFlags );
}

void
MingwModuleHandler::GenerateMacro (
	const char* assignmentOperation,
	const string& macro,
	const IfableData& data,
	set<const Define *> *used_defs,
	bool generatingCompilerMacro )
{
	size_t i;
	bool generateAssignment;

	if ( generatingCompilerMacro )
		generateAssignment = (use_pch && module.pch != NULL ) || data.includes.size () > 0 || data.defines.size () > 0 || data.compilerFlags.size () > 0;
	else
		generateAssignment = (use_pch && module.pch != NULL ) || data.includes.size () > 0 || data.defines.size () > 0;
	if ( generateAssignment )
	{
		fprintf ( fMakefile,
		          "%s %s",
		          macro.c_str(),
		          assignmentOperation );
	}

	const FileLocation *pchFilename = GetPrecompiledHeaderFilename ();
	if ( pchFilename )
	{
		fprintf ( fMakefile,
		          " -I%s",
		          backend->GetFullPath ( *pchFilename ).c_str () );
		delete pchFilename;
	}

	if ( generatingCompilerMacro )
	{
		string compilerParameters = GenerateCompilerParametersFromVector ( data.compilerFlags, CompilerTypeDontCare );
		if ( compilerParameters.size () > 0 )
		{
			fprintf (
				fMakefile,
				"%s",
				compilerParameters.c_str () );
		}
	}
	for ( i = 0; i < data.includes.size(); i++ )
	{
		const Include& include = *data.includes[i];
		const FileLocation* includeDirectory = include.directory;
		fprintf (
			fMakefile,
			" -I%s",
			backend->GetFullPath ( *includeDirectory ).c_str() );
	}
	for ( i = 0; i < data.defines.size(); i++ )
	{
		const Define& define = *data.defines[i];
		if ( used_defs )
		{
			set<const Define *>::const_iterator last_define;
			for (last_define = used_defs->begin ();
			     last_define != used_defs->end ();
			     last_define++)
			{
				if ( (*last_define)->name != define.name )
					continue;
				if ( !define.overridable )
				{
					throw InvalidOperationException ( (*last_define)->node->location.c_str (),
					                                  0,
					                                  "Invalid override of define '%s', already defined at %s",
					                                  define.name.c_str (),
					                                  define.node->location.c_str () );
				}
				if ( backend->configuration.Verbose )
					printf("%s: Overriding '%s' already defined at %s\n",
						(*last_define)->node->location.c_str (), define.name.c_str (),
						define.node->location.c_str () );
				break;
			}
			if ( last_define != used_defs->end () )
				continue;
		}
		fprintf (
			fMakefile,
			" -D%s",
			define.name.c_str() );
		if (define.value.length () > 0)
			fprintf (
				fMakefile,
				"=%s",
				define.value.c_str() );
		if ( used_defs )
			used_defs->insert( used_defs->begin (), &define );
	}
	if ( generateAssignment )
	{
		fprintf ( fMakefile, "\n" );
	}
}

void
MingwModuleHandler::GenerateMacros (
	const char* assignmentOperation,
	const IfableData& data,
	const vector<LinkerFlag*>* linkerFlags,
	set<const Define *>& used_defs )
{
	size_t i;

	GenerateMacro ( assignmentOperation,
	                cflagsMacro,
	                data,
	                &used_defs,
	                true );
	GenerateMacro ( assignmentOperation,
	                windresflagsMacro,
	                data,
	                NULL,
	                false );

	if ( linkerFlags != NULL )
	{
		string linkerParameters = GenerateLinkerParametersFromVector ( *linkerFlags );
		if ( linkerParameters.size () > 0 )
		{
			fprintf (
				fMakefile,
				"%s %s %s\n",
				linkerflagsMacro.c_str (),
				assignmentOperation,
				linkerParameters.c_str() );
		}
	}

	if ( data.libraries.size () > 0 )
	{
		string deps = GenerateImportLibraryDependenciesFromVector ( data.libraries );
		if ( deps.size () > 0 )
		{
			fprintf (
				fMakefile,
				"%s %s %s\n",
				libsMacro.c_str(),
				assignmentOperation,
				deps.c_str() );
		}
	}

	const vector<If*>& ifs = data.ifs;
	for ( i = 0; i < ifs.size(); i++ )
	{
		If& rIf = *ifs[i];
		if ( rIf.data.defines.size()
			|| rIf.data.includes.size()
			|| rIf.data.libraries.size()
			|| rIf.data.compilationUnits.size()
			|| rIf.data.compilerFlags.size()
			|| rIf.data.ifs.size() )
		{
			fprintf (
				fMakefile,
				"%s (\"$(%s)\",\"%s\")\n",
				rIf.negated ? "ifneq" : "ifeq",
				rIf.property.c_str(),
				rIf.value.c_str() );
			GenerateMacros (
				"+=",
				rIf.data,
				NULL,
				used_defs );
			fprintf (
				fMakefile,
				"endif\n\n" );
		}
	}
}

void
MingwModuleHandler::CleanupCompilationUnitVector ( vector<CompilationUnit*>& compilationUnits )
{
	for ( size_t i = 0; i < compilationUnits.size (); i++ )
		delete compilationUnits[i];
}

void
MingwModuleHandler::GetModuleSpecificCompilationUnits ( vector<CompilationUnit*>& compilationUnits )
{
}

void
MingwModuleHandler::GenerateSourceMacros (
	const char* assignmentOperation,
	const IfableData& data )
{
	size_t i;

	const vector<CompilationUnit*>& compilationUnits = data.compilationUnits;
	vector<const FileLocation *> headers;
	if ( compilationUnits.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s %s",
			sourcesMacro.c_str (),
			assignmentOperation );
		for ( i = 0; i < compilationUnits.size(); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			const FileLocation& compilationName = compilationUnit.GetFilename ();
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? " \\\n\t" : " " ),
				backend->GetFullName ( compilationName ).c_str () );
		}
		fprintf ( fMakefile, "\n" );
	}

	const vector<If*>& ifs = data.ifs;
	for ( i = 0; i < ifs.size(); i++ )
	{
		If& rIf = *ifs[i];
		if ( rIf.data.defines.size()
			|| rIf.data.includes.size()
			|| rIf.data.libraries.size()
			|| rIf.data.compilationUnits.size()
			|| rIf.data.compilerFlags.size()
			|| rIf.data.ifs.size() )
		{
			fprintf (
				fMakefile,
				"%s (\"$(%s)\",\"%s\")\n",
				rIf.negated ? "ifneq" : "ifeq",
				rIf.property.c_str(),
				rIf.value.c_str() );
			GenerateSourceMacros (
				"+=",
				rIf.data );
			fprintf (
				fMakefile,
				"endif\n\n" );
		}
	}

	vector<CompilationUnit*> sourceCompilationUnits;
	GetModuleSpecificCompilationUnits ( sourceCompilationUnits );
	for ( i = 0; i < sourceCompilationUnits.size (); i++ )
	{
		const FileLocation& compilationName = sourceCompilationUnits[i]->GetFilename ();
		fprintf (
			fMakefile,
			"%s += %s\n",
			sourcesMacro.c_str(),
			backend->GetFullName ( compilationName ).c_str () );
	}
	CleanupCompilationUnitVector ( sourceCompilationUnits );
}

void
MingwModuleHandler::GenerateObjectMacros (
	const char* assignmentOperation,
	const IfableData& data )
{
	size_t i;

	const vector<CompilationUnit*>& compilationUnits = data.compilationUnits;
	vector<const FileLocation *> headers;
	vector<const FileLocation *> mcheaders;
	vector<const FileLocation *> mcresources;
	if ( compilationUnits.size () > 0 )
	{
		for ( i = 0; i < compilationUnits.size (); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			if ( compilationUnit.IsFirstFile () )
			{
				const FileLocation& compilationName = compilationUnit.GetFilename ();
				const FileLocation *object_file = GetObjectFilename ( &compilationName, module );
				fprintf ( fMakefile,
					"%s := %s $(%s)\n",
					objectsMacro.c_str(),
					backend->GetFullName ( *object_file ).c_str (),
					objectsMacro.c_str() );
				delete object_file;
			}
		}
		fprintf (
			fMakefile,
			"%s %s",
			objectsMacro.c_str (),
			assignmentOperation );
		for ( i = 0; i < compilationUnits.size(); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			if ( !compilationUnit.IsFirstFile () )
			{
				const FileLocation& compilationName = compilationUnit.GetFilename ();
				const FileLocation *objectFilename = GetObjectFilename ( &compilationName, module );
				if ( GetExtension ( *objectFilename ) == ".h" )
					headers.push_back ( objectFilename );
				else if ( GetExtension ( *objectFilename ) == ".rc" )
				{
					const FileLocation *headerFilename = GetMcHeaderFilename ( &compilationUnit.GetFilename () );
					mcheaders.push_back ( headerFilename );
					mcresources.push_back ( objectFilename );
				}
				else
				{
					fprintf (
						fMakefile,
						"%s%s",
						( i%10 == 9 ? " \\\n\t" : " " ),
						backend->GetFullName ( *objectFilename ).c_str () );
					delete objectFilename;
				}
			}
		}
		fprintf ( fMakefile, "\n" );
	}
	if ( headers.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s_HEADERS %s",
			module.name.c_str (),
			assignmentOperation );
		for ( i = 0; i < headers.size (); i++ )
		{
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? " \\\n\t" : " " ),
				backend->GetFullName ( *headers[i] ).c_str () );
			delete headers[i];
		}
		fprintf ( fMakefile, "\n" );
	}

	if ( mcheaders.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s_MCHEADERS %s",
			module.name.c_str (),
			assignmentOperation );
		for ( i = 0; i < mcheaders.size (); i++ )
		{
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? " \\\n\t" : " " ),
				backend->GetFullName ( *mcheaders[i] ).c_str () );
			delete mcheaders[i];
		}
		fprintf ( fMakefile, "\n" );
	}

	if ( mcresources.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s_RESOURCES %s",
			module.name.c_str (),
			assignmentOperation );
		for ( i = 0; i < mcresources.size (); i++ )
		{
			fprintf (
				fMakefile,
				"%s%s",
				( i%10 == 9 ? " \\\n\t" : " " ),
				backend->GetFullName ( *mcresources[i] ).c_str () );
			delete mcresources[i];
		}
		fprintf ( fMakefile, "\n" );
	}

	const vector<If*>& ifs = data.ifs;
	for ( i = 0; i < ifs.size(); i++ )
	{
		If& rIf = *ifs[i];
		if ( rIf.data.defines.size()
			|| rIf.data.includes.size()
			|| rIf.data.libraries.size()
			|| rIf.data.compilationUnits.size()
			|| rIf.data.compilerFlags.size()
			|| rIf.data.ifs.size() )
		{
			fprintf (
				fMakefile,
				"%s (\"$(%s)\",\"%s\")\n",
				rIf.negated ? "ifneq" : "ifeq",
				rIf.property.c_str(),
				rIf.value.c_str() );
			GenerateObjectMacros (
				"+=",
				rIf.data );
			fprintf (
				fMakefile,
				"endif\n\n" );
		}
	}

	vector<CompilationUnit*> sourceCompilationUnits;
	GetModuleSpecificCompilationUnits ( sourceCompilationUnits );
	for ( i = 0; i < sourceCompilationUnits.size (); i++ )
	{
		const FileLocation& compilationName = sourceCompilationUnits[i]->GetFilename ();
		const FileLocation *object_file = GetObjectFilename ( &compilationName, module );
		fprintf (
			fMakefile,
			"%s += %s\n",
			objectsMacro.c_str(),
			backend->GetFullName ( *object_file ).c_str () );
		delete object_file;
	}
	CleanupCompilationUnitVector ( sourceCompilationUnits );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetPrecompiledHeaderFilename () const
{
	if ( !module.pch || !use_pch )
		return NULL;
	return new FileLocation ( IntermediateDirectory,
	                          module.pch->file->relative_path,
	                          ReplaceExtension ( module.pch->file->name, "_" + module.name + ".gch" ) );
}

Rule arRule1 ( "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).a: $($(module_name)_OBJS) | $(INTERMEDIATE)$(SEP)$(source_dir)\n",
               "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).a",
               "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule arRule2 ( "\t$(ECHO_AR)\n"
              "\t${ar} -rc $@ $($(module_name)_OBJS)\n",
              NULL );
Rule arHostRule2 ( "\t$(ECHO_AR)\n"
                   "\t${host_ar} -rc $@ $($(module_name)_OBJS)\n",
                   NULL );
Rule gasRule ( "$(source): ${$(module_name)_precondition}\n"
               "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o: $(source)$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
               "\t$(ECHO_GAS)\n"
               "\t${gcc} -x assembler-with-cpp -c $< -o $@ -D__ASM__ $($(module_name)_CFLAGS)\n",
               "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o",
               "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule bootRule ( "$(source): ${$(module_name)_precondition}\n"
                "$(module_output): $(source)$(dependencies) | $(OUTPUT)$(SEP)$(source_dir)\n"
                "\t$(ECHO_NASM)\n"
                "\t$(Q)${nasm} -f win32 $< -o $@ $($(module_name)_NASMFLAGS)\n",
                "$(OUTPUT)$(SEP)$(source_dir)$(SEP)", NULL );
Rule nasmRule ( "$(source): ${$(module_name)_precondition}\n"
                "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o: $(source)$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                "\t$(ECHO_NASM)\n"
                "\t$(Q)${nasm} -f win32 $< -o $@ $($(module_name)_NASMFLAGS)\n",
                "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o",
                "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule windresRule ( "$(source): ${$(module_name)_precondition}\n"
                   "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).coff: $(source)$(dependencies) $(WRC_TARGET) | $(INTERMEDIATE)$(SEP)$(source_dir) $(TEMPORARY)\n"
                   "\t$(ECHO_WRC)\n"
                   "\t${gcc} -xc -E -DRC_INVOKED ${$(module_name)_RCFLAGS} $(source) > $(TEMPORARY)$(SEP)$(module_name).$(source_name_noext).rci.tmp\n"
                   "\t$(Q)$(WRC_TARGET) ${$(module_name)_RCFLAGS} $(TEMPORARY)$(SEP)$(module_name).$(source_name_noext).rci.tmp $(TEMPORARY)$(SEP)$(module_name).$(source_name_noext).res.tmp\n"
                   "\t-@${rm} $(TEMPORARY)$(SEP)$(module_name).$(source_name_noext).rci.tmp 2>$(NUL)\n"
                   "\t${windres} $(TEMPORARY)$(SEP)$(module_name).$(source_name_noext).res.tmp -o $@\n"
                   "\t-@${rm} $(TEMPORARY)$(SEP)$(module_name).$(source_name_noext).res.tmp 2>$(NUL)\n",
                   "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).coff",
                   "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule wmcRule ( "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).rc $(INTERMEDIATE)$(SEP)include$(SEP)reactos$(SEP)$(source_name_noext).h: $(WMC_TARGET) $(source)\n"
               "\t$(ECHO_WMC)\n"
               "\t$(Q)$(WMC_TARGET) -i -H $(INTERMEDIATE)$(SEP)include$(SEP)reactos$(SEP)$(source_name_noext).h -o $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).rc $(source)\n",
               "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).rc", "$(INTERMEDIATE)$(SEP)include$(SEP)reactos$(SEP)$(source_name_noext).h", NULL );
Rule winebuildRule ( "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).spec.def: $(source)$(dependencies) $(WINEBUILD_TARGET) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                     "\t$(ECHO_WINEBLD)\n"
                     "\t$(Q)$(WINEBUILD_TARGET) $(WINEBUILD_FLAGS) -o $(INTERMEDIATE)$(SEP)$(source_path)$(SEP)$(source_name_noext).spec.def --def -E $(source)\n"
                     "$(INTERMEDIATE)$(SEP)$(source_path)$(SEP)$(source_name_noext).stubs.c: $(source_path)$(SEP)$(source_name_noext).spec $(WINEBUILD_TARGET)\n"
                     "\t$(ECHO_WINEBLD)\n"
                     "\t$(Q)$(WINEBUILD_TARGET) $(WINEBUILD_FLAGS) -o $(INTERMEDIATE)$(SEP)$(source_path)$(SEP)$(source_name_noext).stubs.c --pedll $(source_path)$(SEP)$(source_name_noext).spec\n"
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).stubs.o: $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).stubs.c$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                     "\t$(ECHO_CC)\n"
                     "\t${gcc} -c $< -o $@ $($(module_name)_CFLAGS)$(compiler_flags)\n",
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).spec.def",
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).stubs.c",
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).stubs.o",
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule widlHeaderRule ( "$(source): ${$(module_name)_precondition}\n"
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).h: $(source)$(dependencies) $(WIDL_TARGET) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                      "\t$(ECHO_WIDL)\n"
                      "\t$(Q)$(WIDL_TARGET) $($(module_name)_WIDLFLAGS) -h -H $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).h $(source)\n",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).h",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule widlServerRule ( "$(source): ${$(module_name)_precondition}\n"
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.c $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.h: $(source)$(dependencies) $(WIDL_TARGET) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                      "\t$(ECHO_WIDL)\n"
                      "\t$(Q)$(WIDL_TARGET) $($(module_name)_WIDLFLAGS) -h -H $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.h -s -S $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.c $(source)\n"
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.o: $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.c $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.h$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                      "\t$(ECHO_CC)\n"
                      "\t${gcc} -c $< -o $@ $($(module_name)_CFLAGS)$(compiler_flags)\n",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.h",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.c",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_s.o",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule widlClientRule ( "$(source): ${$(module_name)_precondition}\n"
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.c $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.h: $(source)$(dependencies) $(WIDL_TARGET) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                      "\t$(ECHO_WIDL)\n"
                      "\t$(Q)$(WIDL_TARGET) $($(module_name)_WIDLFLAGS) -h -H $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.h -c -C $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.c $(source)\n"
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.o: $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.c $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.h$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                      "\t$(ECHO_CC)\n"
                      "\t${gcc} -c $< -o $@ $($(module_name)_CFLAGS)$(compiler_flags)\n",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.h",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.c",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_c.o",
                      "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule widlProxyRule ( "$(source): ${$(module_name)_precondition}\n"
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.c $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.h: $(source)$(dependencies) $(WIDL_TARGET) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                     "\t$(ECHO_WIDL)\n"
                     "\t$(Q)$(WIDL_TARGET) $($(module_name)_WIDLFLAGS) -h -H $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.h -p -P $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.c $(source)\n"
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.o: $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.c $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.h$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                      "\t$(ECHO_CC)\n"
                      "\t${gcc} -c $< -o $@ $($(module_name)_CFLAGS)$(compiler_flags)\n",
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.h",
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.c",
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_p.o",
                     "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule widlTlbRule ( "$(source): ${$(module_name)_precondition}\n"
                   "$(OUTPUT)$(SEP)$(source_dir)$(SEP)$(module_name).tlb: $(source)$(dependencies) $(WIDL_TARGET) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                   "\t$(ECHO_WIDL)\n"
                   "\t$(Q)$(WIDL_TARGET) $($(module_name)_WIDLFLAGS) -t -T $(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext).tlb $(source)\n",
                   "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)", NULL );
Rule gccRule ( "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o: $(source) ${$(module_name)_precondition}$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
               "\t$(ECHO_CC)\n"
               "\t${gcc} -c $< -o $@ $($(module_name)_CFLAGS)$(compiler_flags)\n",
               "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o", NULL );
Rule gccHostRule ( "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o: $(source) ${$(module_name)_precondition}$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                   "\t$(ECHO_CC)\n"
                   "\t${host_gcc} -c $< -o $@ $($(module_name)_CFLAGS)$(compiler_flags)\n",
                   "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o", NULL );
Rule gppRule ( "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o: $(source) ${$(module_name)_precondition}$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
               "\t$(ECHO_CC)\n"
               "\t${gpp} -c $< -o $@ $($(module_name)_CFLAGS)$(compiler_flags)\n",
               "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o", NULL );
Rule gppHostRule ( "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o: $(source) ${$(module_name)_precondition}$(dependencies) | $(INTERMEDIATE)$(SEP)$(source_dir)\n"
                   "\t$(ECHO_CC)\n"
                   "\t${host_gpp} -c $< -o $@ $($(module_name)_CFLAGS)$(compiler_flags)\n",
                   "$(INTERMEDIATE)$(SEP)$(source_dir)$(SEP)$(source_name_noext)_$(module_name).o", NULL );
Rule emptyRule ( "", NULL );

void
MingwModuleHandler::GenerateGccCommand (
	const FileLocation* sourceFile,
	const Rule *rule,
	const string& extraDependencies )
{
	const FileLocation *generatedSourceFileName = GetActualSourceFilename ( sourceFile );
	const FileLocation *pchFilename = GetPrecompiledHeaderFilename ();
	string dependencies = extraDependencies;

	string flags;
	string extension = GetExtension ( *sourceFile );
	if ( extension == ".cc" || extension == ".cpp" || extension == ".cxx" )
		flags = GenerateCompilerParametersFromVector ( module.non_if_data.compilerFlags, CompilerTypeCPP );
	else
		flags = GenerateCompilerParametersFromVector ( module.non_if_data.compilerFlags, CompilerTypeCC );

	if ( pchFilename )
	{
		dependencies += " " + backend->GetFullName ( *pchFilename );
		delete pchFilename;
	}

	/* WIDL generated headers may be used */
	vector<FileLocation> rpcDependencies;
	GetRpcHeaderDependencies ( rpcDependencies );
	if ( rpcDependencies.size () > 0 )
		dependencies += " " + v2s ( backend, rpcDependencies, 5 );

	rule->Execute ( fMakefile, backend, module, generatedSourceFileName, clean_files, dependencies, flags );

	delete generatedSourceFileName;
}

string
MingwModuleHandler::GetPropertyValue ( const Module& module, const std::string& name )
{
	for ( size_t i = 0; i < module.project.non_if_data.properties.size (); i++ )
	{
		const Property& property = *module.project.non_if_data.properties[i];
		if ( property.name == name )
			return property.value;
	}
	return string ( "" );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetRpcServerHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + "_s.h";
	return new FileLocation ( IntermediateDirectory, base->relative_path, newname );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetRpcClientHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + "_c.h";
	return new FileLocation ( IntermediateDirectory, base->relative_path, newname );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetRpcProxyHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + "_p.h";
	return new FileLocation ( IntermediateDirectory, base->relative_path, newname );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetIdlHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + ".h";
	return new FileLocation ( IntermediateDirectory, base->relative_path, newname );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetMcHeaderFilename ( const FileLocation *base ) const
{
	string newname = GetBasename ( base->name ) + ".h";
	return new FileLocation ( IntermediateDirectory, "include/reactos" , newname );
}

void
MingwModuleHandler::GenerateCommands (
	const CompilationUnit& compilationUnit,
	const string& extraDependencies )
{
	const FileLocation& sourceFile = compilationUnit.GetFilename ();
	string extension = GetExtension ( sourceFile );
	std::transform ( extension.begin (), extension.end (), extension.begin (), tolower );

	struct
	{
		HostType host;
		ModuleType type;
		string extension;
		Rule* rule;
	} rules[] = {
		{ HostDontCare, TypeDontCare, ".s", &gasRule },
		{ HostDontCare, BootSector, ".asm", &bootRule },
		{ HostDontCare, TypeDontCare, ".asm", &nasmRule },
		{ HostDontCare, TypeDontCare, ".rc", &windresRule },
		{ HostDontCare, TypeDontCare, ".mc", &wmcRule },
		{ HostDontCare, TypeDontCare, ".spec", &winebuildRule },
		{ HostDontCare, RpcServer, ".idl", &widlServerRule },
		{ HostDontCare, RpcClient, ".idl", &widlClientRule },
		{ HostDontCare, RpcProxy, ".idl", &widlProxyRule },
		{ HostDontCare, EmbeddedTypeLib, ".idl", &widlTlbRule },
		{ HostDontCare, TypeDontCare, ".idl", &widlHeaderRule },
		{ HostTrue, TypeDontCare, ".c", &gccHostRule },
		{ HostTrue, TypeDontCare, ".cc", &gppHostRule },
		{ HostTrue, TypeDontCare, ".cpp", &gppHostRule },
		{ HostTrue, TypeDontCare, ".cxx", &gppHostRule },
		{ HostFalse, TypeDontCare, ".c", &gccRule },
		{ HostFalse, TypeDontCare, ".cc", &gppRule },
		{ HostFalse, TypeDontCare, ".cpp", &gppRule },
		{ HostFalse, TypeDontCare, ".cxx", &gppRule },
		{ HostFalse, Cabinet, ".*", &emptyRule }
	};
	size_t i;
	Rule *customRule = NULL;

	for ( i = 0; i < sizeof ( rules ) / sizeof ( rules[0] ); i++ )
	{
		if ( rules[i].host != HostDontCare && rules[i].host != module.host )
			continue;
		if ( rules[i].type != TypeDontCare && rules[i].type != module.type )
			continue;
		if ( rules[i].extension != extension && rules[i].extension != ".*")
			continue;
		customRule = rules[i].rule;
		break;
	}

	if ( extension == ".c" || extension == ".cc" || extension == ".cpp" || extension == ".cxx" )
	{
		GenerateGccCommand ( &sourceFile,
		                     customRule,
		                     GetCompilationUnitDependencies ( compilationUnit ) + GetExtraDependencies ( &sourceFile ) + extraDependencies );
	}
	else if ( customRule )
		customRule->Execute ( fMakefile, backend, module, &sourceFile, clean_files );
	else
	{
		throw InvalidOperationException ( __FILE__,
		                                  __LINE__,
		                                  "Unsupported filename extension '%s' in file '%s'",
		                                  extension.c_str (),
		                                  backend->GetFullName ( sourceFile ).c_str () );
	}
}

void
MingwModuleHandler::GenerateBuildMapCode ( const FileLocation *mapTarget )
{
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDMAP),full)\n" );

	FileLocation mapFilename ( OutputDirectory,
	                           module.output->relative_path,
	                           GetBasename ( module.output->name ) + ".map" );
	CLEAN_FILE ( mapFilename );

	fprintf ( fMakefile,
	          "\t$(ECHO_OBJDUMP)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)${objdump} -d -S %s > %s\n",
	          mapTarget ? backend->GetFullName ( *mapTarget ).c_str () :  "$@",
	          backend->GetFullName ( mapFilename ).c_str () );

	fprintf ( fMakefile,
	          "else\n" );
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDMAP),yes)\n" );

	fprintf ( fMakefile,
	          "\t$(ECHO_NM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)${nm} --numeric-sort %s > %s\n",
	          mapTarget ? backend->GetFullName ( *mapTarget ).c_str () :  "$@",
	          backend->GetFullName ( mapFilename ).c_str () );

	fprintf ( fMakefile,
	          "endif\n" );

	fprintf ( fMakefile,
	          "endif\n" );
}

void
MingwModuleHandler::GenerateBuildNonSymbolStrippedCode ()
{
	fprintf ( fMakefile,
	          "ifeq ($(ROS_BUILDNOSTRIP),yes)\n" );

	FileLocation nostripFilename ( OutputDirectory,
	                               module.output->relative_path,
	                               GetBasename ( module.output->name ) + ".nostrip" + GetExtension ( *module.output ) );
	CLEAN_FILE ( nostripFilename );

	OutputCopyCommand ( *module.output, nostripFilename );

	fprintf ( fMakefile,
	          "endif\n" );
}

void
MergeStringVector ( const Backend* backend,
                    const vector<FileLocation>& input,
                    vector<string>& output )
{
	int wrap_at = 25;
	string s;
	int wrap_count = -1;
	for ( size_t i = 0; i < input.size (); i++ )
	{
		if ( wrap_count++ == wrap_at )
		{
			output.push_back ( s );
			s = "";
			wrap_count = 0;
		}
		else if ( s.size () > 0)
			s += " ";
		s += backend->GetFullName ( input[i] );
	}
	if ( s.length () > 0 )
		output.push_back ( s );
}

void
MingwModuleHandler::GetObjectsVector ( const IfableData& data,
                                       vector<FileLocation>& objectFiles ) const
{
	for ( size_t i = 0; i < data.compilationUnits.size (); i++ )
	{
		CompilationUnit& compilationUnit = *data.compilationUnits[i];
		const FileLocation& compilationName = compilationUnit.GetFilename ();
		const FileLocation *object_file = GetObjectFilename ( &compilationName, module );
		objectFiles.push_back ( *object_file );
		delete object_file;
	}
}

void
MingwModuleHandler::GenerateCleanObjectsAsYouGoCode () const
{
	if ( backend->configuration.CleanAsYouGo )
	{
		vector<FileLocation> objectFiles;
		GetObjectsVector ( module.non_if_data,
		                   objectFiles );
		vector<string> lines;
		MergeStringVector ( backend,
		                    objectFiles,
		                    lines );
		for ( size_t i = 0; i < lines.size (); i++ )
		{
			fprintf ( fMakefile,
			          "\t-@${rm} %s 2>$(NUL)\n",
			          lines[i].c_str () );
		}
	}
}

void
MingwModuleHandler::GenerateRunRsymCode () const
{
	fprintf ( fMakefile,
	          "\t$(ECHO_RSYM)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(RSYM_TARGET) $@ $@\n\n" );
}

void
MingwModuleHandler::GenerateRunStripCode () const
{
	fprintf ( fMakefile,
	          "ifeq ($(ROS_LEAN_AND_MEAN),yes)\n" );
	fprintf ( fMakefile,
	          "\t$(ECHO_STRIP)\n" );
	fprintf ( fMakefile,
	          "\t${strip} -s -x -X $@\n\n" );
	fprintf ( fMakefile,
	          "endif\n" );
}

void
MingwModuleHandler::GenerateLinkerCommand (
	const string& dependencies,
	const string& linkerParameters,
	const string& pefixupParameters )
{
	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	const FileLocation *definitionFilename = GetDefinitionFilename ();
	string linker = module.cplusplus ? "${gpp}" : "${gcc}";
	string objectsMacro = GetObjectsMacro ( module );
	string libsMacro = GetLibsMacro ();

	string target_macro ( GetTargetMacro ( module ) );
	string target_folder ( backend->GetFullPath ( *target_file ) );

	string linkerScriptArgument;
	if ( module.linkerScript != NULL )
		linkerScriptArgument = ssprintf ( " -Wl,-T,%s", backend->GetFullName ( *module.linkerScript->file ).c_str () );
	else
		linkerScriptArgument = "";

	fprintf ( fMakefile,
		"%s: %s %s $(RSYM_TARGET) $(PEFIXUP_TARGET) | %s\n",
		target_macro.c_str (),
		definitionFilename ? backend->GetFullName ( *definitionFilename ).c_str () : "",
		dependencies.c_str (),
		target_folder.c_str () );
	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );
	string targetName ( module.output->name );

	if ( !module.IsDLL () )
	{
		fprintf ( fMakefile,
		          "\t%s %s%s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          linkerScriptArgument.c_str (),
		          target_macro.c_str (),
		          objectsMacro.c_str (),
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str () );
	}
	else if ( module.HasImportLibrary () )
	{
		FileLocation temp_exp ( TemporaryDirectory,
		                        "",
		                        module.name + ".temp.exp" );
		CLEAN_FILE ( temp_exp );

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-exp %s%s%s\n",
		          targetName.c_str (),
		          definitionFilename ? backend->GetFullName ( *definitionFilename ).c_str () : "",
		          backend->GetFullName ( temp_exp ).c_str (),
		          module.mangledSymbols ? "" : " --kill-at",
		          module.underscoreSymbols ? " --add-underscore" : "" );

		fprintf ( fMakefile,
		          "\t%s %s%s %s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          linkerScriptArgument.c_str (),
		          backend->GetFullName ( temp_exp ).c_str (),
		          target_macro.c_str (),
		          objectsMacro.c_str (),
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str () );

		fprintf ( fMakefile,
		          "\t$(Q)$(PEFIXUP_TARGET) %s -exports%s\n",
		          target_macro.c_str (),
		          pefixupParameters.c_str() );

		fprintf ( fMakefile,
		          "\t-@${rm} %s 2>$(NUL)\n",
		          backend->GetFullName ( temp_exp ).c_str () );
	}
	else
	{
		/* XXX: need to workaround binutils bug, which exports
		 * all functions in a dll if no .def file or an empty
		 * one has been provided... */
		/* See bug 1244 */
		//printf ( "%s will have all its functions exported\n",
		//         module.target->name.c_str () );
		fprintf ( fMakefile,
		          "\t%s %s%s -o %s %s %s %s\n",
		          linker.c_str (),
		          linkerParameters.c_str (),
		          linkerScriptArgument.c_str (),
		          target_macro.c_str (),
		          objectsMacro.c_str (),
		          libsMacro.c_str (),
		          GetLinkerMacro ().c_str () );
	}

	GenerateBuildMapCode ();
	GenerateBuildNonSymbolStrippedCode ();
	GenerateRunRsymCode ();
	GenerateRunStripCode ();
	GenerateCleanObjectsAsYouGoCode ();

	if ( definitionFilename )
		delete definitionFilename;
	delete target_file;
}

void
MingwModuleHandler::GeneratePhonyTarget() const
{
	string targetMacro ( GetTargetMacro ( module ) );
	const FileLocation *target_file = GetTargetFilename ( module, NULL );

	fprintf ( fMakefile,
	          ".PHONY: %s\n\n",
	          targetMacro.c_str ());
	fprintf ( fMakefile, "%s: | %s\n",
	          targetMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	delete target_file;
}

void
MingwModuleHandler::GenerateObjectFileTargets ( const IfableData& data )
{
	size_t i;
	string moduleDependencies;

	const vector<CompilationUnit*>& compilationUnits = data.compilationUnits;
	for ( i = 0; i < compilationUnits.size (); i++ )
	{
		CompilationUnit& compilationUnit = *compilationUnits[i];
		const FileLocation& compilationName = compilationUnit.GetFilename ();
		const FileLocation *objectFilename = GetObjectFilename ( &compilationName, module );
		if ( GetExtension ( *objectFilename ) == ".h" )
			moduleDependencies += ssprintf ( " $(%s_HEADERS)", module.name.c_str () );
		else if ( GetExtension ( *objectFilename ) == ".rc" )
			moduleDependencies += ssprintf ( " $(%s_RESOURCES)", module.name.c_str () );
		delete objectFilename;
	}

	for ( i = 0; i < compilationUnits.size (); i++ )
	{
		GenerateCommands ( *compilationUnits[i],
		                   moduleDependencies );
		fprintf ( fMakefile,
		          "\n" );
	}

	const vector<If*>& ifs = data.ifs;
	for ( i = 0; i < ifs.size(); i++ )
	{
		GenerateObjectFileTargets ( ifs[i]->data );
	}

	vector<CompilationUnit*> sourceCompilationUnits;
	GetModuleSpecificCompilationUnits ( sourceCompilationUnits );
	for ( i = 0; i < sourceCompilationUnits.size (); i++ )
	{
		GenerateCommands ( *sourceCompilationUnits[i],
		                   moduleDependencies );
	}
	CleanupCompilationUnitVector ( sourceCompilationUnits );
}

void
MingwModuleHandler::GenerateObjectFileTargets ()
{
	const FileLocation *pchFilename = GetPrecompiledHeaderFilename ();

	if ( pchFilename )
	{
		string cc = ( module.host == HostTrue ? "${host_gcc}" : "${gcc}" );
		string cppc = ( module.host == HostTrue ? "${host_gpp}" : "${gpp}" );

		const FileLocation& baseHeaderFile = *module.pch->file;
		CLEAN_FILE ( *pchFilename );
		string dependencies = backend->GetFullName ( baseHeaderFile );
		/* WIDL generated headers may be used */
		vector<FileLocation> rpcDependencies;
		GetRpcHeaderDependencies ( rpcDependencies );
		if ( rpcDependencies.size () > 0 )
			dependencies += " " + v2s ( backend, rpcDependencies, 5 );
		fprintf ( fMakefile,
		          "%s: %s ${%s_precondition} | %s\n",
		          backend->GetFullName ( *pchFilename ).c_str(),
		          dependencies.c_str(),
		          module.name.c_str (),
		          backend->GetFullPath ( *pchFilename ).c_str() );
		fprintf ( fMakefile, "\t$(ECHO_PCH)\n" );
		fprintf ( fMakefile,
		          "\t%s -o %s %s -g %s\n\n",
		          module.cplusplus ? cppc.c_str() : cc.c_str(),
		          backend->GetFullName ( *pchFilename ).c_str(),
		          cflagsMacro.c_str(),
		          backend->GetFullName ( baseHeaderFile ).c_str() );
		delete pchFilename;
	}

	GenerateObjectFileTargets ( module.non_if_data );
	fprintf ( fMakefile, "\n" );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GenerateArchiveTarget ()
{
	const FileLocation *archiveFilename = GetModuleArchiveFilename ();
	const FileLocation *definitionFilename = GetDefinitionFilename ();

	arRule1.Execute ( fMakefile, backend, module, archiveFilename, clean_files );

	if ( IsStaticLibrary ( module ) && definitionFilename )
	{
		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib $@%s%s\n",
		          module.importLibrary->dllname.c_str (),
		          backend->GetFullName ( *definitionFilename ).c_str (),
		          module.mangledSymbols ? "" : " --kill-at",
		          module.underscoreSymbols ? " --add-underscore" : "" );
	}

	if ( definitionFilename )
		delete definitionFilename;

	if(module.type == HostStaticLibrary)
		arHostRule2.Execute ( fMakefile, backend, module, archiveFilename, clean_files );
	else
		arRule2.Execute ( fMakefile, backend, module, archiveFilename, clean_files );

	GenerateCleanObjectsAsYouGoCode ();

	fprintf ( fMakefile, "\n" );

	return archiveFilename;
}

string
MingwModuleHandler::GetCFlagsMacro () const
{
	return ssprintf ( "$(%s_CFLAGS)",
	                  module.name.c_str () );
}

/*static*/ string
MingwModuleHandler::GetObjectsMacro ( const Module& module )
{
	return ssprintf ( "$(%s_OBJS)",
	                  module.name.c_str () );
}

string
MingwModuleHandler::GetLinkingDependenciesMacro () const
{
	return ssprintf ( "$(%s_LINKDEPS)", module.name.c_str () );
}

string
MingwModuleHandler::GetLibsMacro () const
{
	return ssprintf ( "$(%s_LIBS)", module.name.c_str () );
}

string
MingwModuleHandler::GetLinkerMacro () const
{
	return ssprintf ( "$(%s_LFLAGS)",
	                  module.name.c_str () );
}

string
MingwModuleHandler::GetModuleTargets ( const Module& module )
{
	if ( ReferenceObjects ( module ) )
		return GetObjectsMacro ( module );
	else
	{
		const FileLocation *target_file = GetTargetFilename ( module, NULL );
		string target = backend->GetFullName ( *target_file ).c_str ();
		delete target_file;
		return target;
	}
}

void
MingwModuleHandler::GenerateSourceMacro ()
{
	sourcesMacro = ssprintf ( "%s_SOURCES", module.name.c_str ());

	GenerateSourceMacros (
		"=",
		module.non_if_data );

	// future references to the macro will be to get its values
	sourcesMacro = ssprintf ("$(%s)", sourcesMacro.c_str ());
}

void
MingwModuleHandler::GenerateObjectMacro ()
{
	objectsMacro = ssprintf ("%s_OBJS", module.name.c_str ());

	GenerateObjectMacros (
		"=",
		module.non_if_data );

	// future references to the macro will be to get its values
	objectsMacro = ssprintf ("$(%s)", objectsMacro.c_str ());
}

void
MingwModuleHandler::GenerateTargetMacro ()
{
	fprintf ( fMakefile,
		"%s := %s\n",
		GetTargetMacro ( module, false ).c_str (),
		GetModuleTargets ( module ).c_str () );
}

void
MingwModuleHandler::GetRpcHeaderDependencies (
	vector<FileLocation>& dependencies ) const
{
	for ( size_t i = 0; i < module.non_if_data.libraries.size (); i++ )
	{
		Library& library = *module.non_if_data.libraries[i];
		if ( library.importedModule->type == RpcServer ||
		     library.importedModule->type == RpcClient ||
		     library.importedModule->type == RpcProxy ||
		     library.importedModule->type == IdlHeader )
		{
			for ( size_t j = 0; j < library.importedModule->non_if_data.compilationUnits.size (); j++ )
			{
				CompilationUnit& compilationUnit = *library.importedModule->non_if_data.compilationUnits[j];
				const FileLocation& sourceFile = compilationUnit.GetFilename ();
				string extension = GetExtension ( sourceFile );
				if ( extension == ".idl" || extension == ".IDL" )
				{
					string basename = GetBasename ( sourceFile.name );
					if ( library.importedModule->type == RpcServer )
					{
						const FileLocation *header = GetRpcServerHeaderFilename ( &sourceFile );
						dependencies.push_back ( *header );
						delete header;
					}
					if ( library.importedModule->type == RpcClient )
					{
						const FileLocation *header = GetRpcClientHeaderFilename ( &sourceFile );
						dependencies.push_back ( *header );
						delete header;
					}
					if ( library.importedModule->type == RpcProxy )
					{
						const FileLocation *header = GetRpcProxyHeaderFilename ( &sourceFile );
						dependencies.push_back ( *header );
						delete header;
					}
					if ( library.importedModule->type == IdlHeader )
					{
						const FileLocation *header = GetIdlHeaderFilename ( &sourceFile );
						dependencies.push_back ( *header );
						delete header;
					}
				}
			}
		}
	}
}

void
MingwModuleHandler::GenerateOtherMacros ()
{
	set<const Define *> used_defs;

	cflagsMacro = ssprintf ("%s_CFLAGS", module.name.c_str ());
	nasmflagsMacro = ssprintf ("%s_NASMFLAGS", module.name.c_str ());
	windresflagsMacro = ssprintf ("%s_RCFLAGS", module.name.c_str ());
	widlflagsMacro = ssprintf ("%s_WIDLFLAGS", module.name.c_str ());
	linkerflagsMacro = ssprintf ("%s_LFLAGS", module.name.c_str ());
	libsMacro = ssprintf("%s_LIBS", module.name.c_str ());
	linkDepsMacro = ssprintf ("%s_LINKDEPS", module.name.c_str ());

	GenerateMacros (
		"=",
		module.non_if_data,
		&module.linkerFlags,
		used_defs );

	if ( module.host == HostFalse )
	{
		GenerateMacros (
			"+=",
			module.project.non_if_data,
			NULL,
			used_defs );
	}

	vector<FileLocation> s;
	if ( module.importLibrary )
	{
		const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
		for ( size_t i = 0; i < compilationUnits.size (); i++ )
		{
			CompilationUnit& compilationUnit = *compilationUnits[i];
			const FileLocation& sourceFile = compilationUnit.GetFilename ();
			string extension = GetExtension ( sourceFile );
			if ( extension == ".spec" || extension == ".SPEC" )
				GetSpecObjectDependencies ( s, &sourceFile );
		}
	}
	if ( s.size () > 0 )
	{
		fprintf (
			fMakefile,
			"%s +=",
			linkDepsMacro.c_str() );
		for ( size_t i = 0; i < s.size(); i++ )
			fprintf ( fMakefile,
			          " %s",
			          backend->GetFullName ( s[i] ).c_str () );
		fprintf ( fMakefile, "\n" );
	}

	string globalCflags = "";
	if ( module.host == HostFalse )
		globalCflags += " $(PROJECT_CFLAGS)";
	else
		globalCflags += " -Wall -Wpointer-arith -D__REACTOS__";
	globalCflags += " -g";
	if ( backend->usePipe )
		globalCflags += " -pipe";
	if ( !module.allowWarnings )
		globalCflags += " -Werror";
	if ( module.host == HostTrue )
	{
		if ( module.cplusplus )
			globalCflags += " $(HOST_CPPFLAGS)";
		else
			globalCflags += " -Wno-strict-aliasing $(HOST_CFLAGS)";
	}
	else
	{
		if ( module.cplusplus )
		{
			// HACK: use host headers when building C++
			globalCflags += " $(HOST_CPPFLAGS)";
		}
		else
			globalCflags += " -nostdinc";
	}

	// Always force disabling of sibling calls optimisation for GCC
	// (TODO: Move to version-specific once this bug is fixed in GCC)
	globalCflags += " -fno-optimize-sibling-calls";

	fprintf (
		fMakefile,
		"%s +=%s\n",
		cflagsMacro.c_str (),
		globalCflags.c_str () );

	if ( module.host == HostFalse )
	{
		fprintf (
			fMakefile,
			"%s += $(PROJECT_RCFLAGS)\n",
			windresflagsMacro.c_str () );

		fprintf (
			fMakefile,
			"%s += $(PROJECT_WIDLFLAGS) -I%s\n",
			widlflagsMacro.c_str (),
			module.output->relative_path.c_str () );

		fprintf (
			fMakefile,
			"%s_LFLAGS += $(PROJECT_LFLAGS) -g\n",
			module.name.c_str () );
	}
	else
	{
		fprintf (
			fMakefile,
			"%s_LFLAGS += $(HOST_LFLAGS)\n",
			module.name.c_str () );
	}

	fprintf (
		fMakefile,
		"%s += $(%s)\n",
		linkDepsMacro.c_str (),
		libsMacro.c_str () );

	string cflags = TypeSpecificCFlags();
	if ( cflags.size() > 0 )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          cflagsMacro.c_str (),
		          cflags.c_str () );
	}

	string nasmflags = TypeSpecificNasmFlags();
	if ( nasmflags.size () > 0 )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          nasmflagsMacro.c_str (),
		          nasmflags.c_str () );
	}

	string linkerflags = TypeSpecificLinkerFlags();
	if ( linkerflags.size() > 0 )
	{
		fprintf ( fMakefile,
		          "%s += %s\n\n",
		          linkerflagsMacro.c_str (),
		          linkerflags.c_str () );
	}

	if ( IsStaticLibrary ( module ) && module.isStartupLib )
	{
		fprintf ( fMakefile,
		          "%s += -Wno-main\n\n",
		          cflagsMacro.c_str () );
	}

	fprintf ( fMakefile, "\n\n" );

	// future references to the macros will be to get their values
	cflagsMacro = ssprintf ("$(%s)", cflagsMacro.c_str ());
	nasmflagsMacro = ssprintf ("$(%s)", nasmflagsMacro.c_str ());
	widlflagsMacro = ssprintf ("$(%s)", widlflagsMacro.c_str ());
}

void
MingwModuleHandler::GenerateRules ()
{
	string targetMacro = GetTargetMacro ( module );
	//CLEAN_FILE ( targetMacro );
	CLEAN_FILE ( FileLocation ( SourceDirectory, "", targetMacro ) );

	// generate phony target for module name
	fprintf ( fMakefile, ".PHONY: %s\n",
		module.name.c_str () );
	string dependencies = GetTargetMacro ( module );
	if ( module.type == Test )
		dependencies += " $(REGTESTS_RUN_TARGET)";
	fprintf ( fMakefile, "%s: %s\n\n",
		module.name.c_str (),
		dependencies.c_str () );
	if ( module.type == Test )
	{
		fprintf ( fMakefile,
		          "\t@%s\n",
		          targetMacro.c_str ());
	}

	if ( !ReferenceObjects ( module ) )
	{
		const FileLocation* ar_target = GenerateArchiveTarget ();
		delete ar_target;
	}

	GenerateObjectFileTargets ();
}

void
MingwModuleHandler::GetInvocationDependencies (
	const Module& module,
	string_list& dependencies )
{
	for ( size_t i = 0; i < module.invocations.size (); i++ )
	{
		Invoke& invoke = *module.invocations[i];
		if ( invoke.invokeModule == &module )
			/* Protect against circular dependencies */
			continue;
		invoke.GetTargets ( dependencies );
	}
}

void
MingwModuleHandler::GenerateInvocations () const
{
	if ( module.invocations.size () == 0 )
		return;

	size_t iend = module.invocations.size ();
	for ( size_t i = 0; i < iend; i++ )
	{
		const Invoke& invoke = *module.invocations[i];

		if ( invoke.invokeModule->type != BuildTool )
		{
			throw XMLInvalidBuildFileException (
				module.node.location,
				"Only modules of type buildtool can be invoked." );
		}

		string invokeTarget = module.GetInvocationTarget ( i );
		string_list invoke_targets;
		assert ( invoke_targets.size() );
		invoke.GetTargets ( invoke_targets );
		fprintf ( fMakefile,
		          ".PHONY: %s\n\n",
		          invokeTarget.c_str () );
		fprintf ( fMakefile,
		          "%s:",
		          invokeTarget.c_str () );
		size_t j, jend = invoke_targets.size();
		for ( j = 0; j < jend; j++ )
		{
			fprintf ( fMakefile,
			          " %s",
			          invoke_targets[i].c_str () );
		}
		fprintf ( fMakefile, "\n\n%s", invoke_targets[0].c_str () );
		for ( j = 1; j < jend; j++ )
			fprintf ( fMakefile,
			          " %s",
			          invoke_targets[i].c_str () );
		fprintf ( fMakefile,
		          ": %s\n",
		          NormalizeFilename ( backend->GetFullName ( *invoke.invokeModule->output ) ).c_str () );
		fprintf ( fMakefile, "\t$(ECHO_INVOKE)\n" );
		fprintf ( fMakefile,
		          "\t%s %s\n\n",
		          NormalizeFilename ( backend->GetFullName ( *invoke.invokeModule->output ) ).c_str (),
		          invoke.GetParameters ().c_str () );
	}
}

string
MingwModuleHandler::GetPreconditionDependenciesName () const
{
	return module.name + "_precondition";
}

void
MingwModuleHandler::GetDefaultDependencies (
	string_list& dependencies ) const
{
	/* Avoid circular dependency */
	if ( module.host == HostTrue )
		return;

	if ( module.name != "psdk" )
		dependencies.push_back ( "$(PSDK_TARGET) $(psdk_HEADERS)" );

	/* Check if any dependent library relies on the generated headers */
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		for ( size_t j = 0; j < m.non_if_data.compilationUnits.size (); j++ )
		{
			CompilationUnit& compilationUnit = *m.non_if_data.compilationUnits[j];
			const FileLocation& sourceFile = compilationUnit.GetFilename ();
			string extension = GetExtension ( sourceFile );
			if (extension == ".mc" || extension == ".MC" )
			{
				string dependency = ssprintf ( "$(%s_MCHEADERS)", m.name.c_str () );
				dependencies.push_back ( dependency );
			}
		}
	}
}

void
MingwModuleHandler::GeneratePreconditionDependencies ()
{
	string preconditionDependenciesName = GetPreconditionDependenciesName ();
	vector<FileLocation> sourceFilenames;
	GetSourceFilenamesWithoutGeneratedFiles ( sourceFilenames );
	string_list dependencies;
	GetDefaultDependencies ( dependencies );
	GetModuleDependencies ( dependencies );

	GetInvocationDependencies ( module, dependencies );

	if ( dependencies.size() )
	{
		fprintf ( fMakefile,
		          "%s =",
		          preconditionDependenciesName.c_str () );
		for ( size_t i = 0; i < dependencies.size(); i++ )
			fprintf ( fMakefile,
			          " %s",
			          dependencies[i].c_str () );
		fprintf ( fMakefile, "\n\n" );
	}

	fprintf ( fMakefile, "\n" );
}

bool
MingwModuleHandler::IsWineModule () const
{
	if ( module.importLibrary == NULL)
		return false;

	size_t index = module.importLibrary->source->name.rfind ( ".spec.def" );
	return ( index != string::npos );
}

/* caller needs to delete the returned object */
const FileLocation*
MingwModuleHandler::GetDefinitionFilename () const
{
	if ( module.importLibrary == NULL )
		return NULL;

	DirectoryLocation directory;
	if ( IsWineModule () )
		directory = IntermediateDirectory;
	else
		directory = SourceDirectory;

	return new FileLocation ( directory,
	                          module.importLibrary->source->relative_path,
	                          module.importLibrary->source->name );
}

void
MingwModuleHandler::GenerateImportLibraryTargetIfNeeded ()
{
	if ( module.importLibrary != NULL )
	{
		const FileLocation *library_target = GetImportLibraryFilename ( module, &clean_files );
		const FileLocation *defFilename = GetDefinitionFilename ();
		string empty = "tools" + sSep + "rbuild" + sSep + "empty.def";

		vector<FileLocation> deps;
		GetDefinitionDependencies ( deps );

		fprintf ( fMakefile, "# IMPORT LIBRARY RULE:\n" );

		fprintf ( fMakefile, "%s:",
		          backend->GetFullName ( *library_target ).c_str () );

		if ( defFilename )
		{
			fprintf ( fMakefile, " %s",
			          backend->GetFullName ( *defFilename ).c_str () );
		}

		size_t i, iend = deps.size();
		for ( i = 0; i < iend; i++ )
			fprintf ( fMakefile, " %s",
			          backend->GetFullName ( deps[i] ).c_str () );

		fprintf ( fMakefile, " | %s\n",
		          backend->GetFullPath ( *library_target ).c_str () );

		fprintf ( fMakefile, "\t$(ECHO_DLLTOOL)\n" );

		fprintf ( fMakefile,
		          "\t${dlltool} --dllname %s --def %s --output-lib %s%s%s\n\n",
		          module.output->name.c_str (),
		          defFilename ? backend->GetFullName ( *defFilename ).c_str ()
		                      : empty.c_str (),
		          backend->GetFullName ( *library_target ).c_str (),
		          module.mangledSymbols ? "" : " --kill-at",
		          module.underscoreSymbols ? " --add-underscore" : "" );

		if ( defFilename )
			delete defFilename;
		delete library_target;
	}
}

void
MingwModuleHandler::GetSpecObjectDependencies (
	vector<FileLocation>& dependencies,
	const FileLocation *file ) const
{
	string basename = GetBasename ( file->name );

	FileLocation defDependency ( IntermediateDirectory,
	                             file->relative_path,
	                             basename + ".spec.def" );
	dependencies.push_back ( defDependency );

	FileLocation stubsDependency ( IntermediateDirectory,
	                               file->relative_path,
	                             basename + ".stubs.c" );
	dependencies.push_back ( stubsDependency );
}

void
MingwModuleHandler::GetMcObjectDependencies (
	vector<FileLocation>& dependencies,
	const FileLocation *file ) const
{
	string basename = GetBasename ( file->name );

	FileLocation defDependency ( IntermediateDirectory,
	                             "include/reactos",
	                             basename + ".h" );
	dependencies.push_back ( defDependency );

	FileLocation stubsDependency ( IntermediateDirectory,
	                               file->relative_path,
	                             basename + ".rc" );
	dependencies.push_back ( stubsDependency );
}

void
MingwModuleHandler::GetWidlObjectDependencies (
	vector<FileLocation>& dependencies,
	const FileLocation *file ) const
{
	string basename = GetBasename ( file->name );
	const FileLocation *generatedHeaderFilename = GetRpcServerHeaderFilename ( file );

	FileLocation serverSourceDependency ( IntermediateDirectory,
	                                      file->relative_path,
	                                      basename + "_s.c" );
	dependencies.push_back ( serverSourceDependency );
	dependencies.push_back ( *generatedHeaderFilename );

	delete generatedHeaderFilename;
}

void
MingwModuleHandler::GetDefinitionDependencies (
	vector<FileLocation>& dependencies ) const
{
	const vector<CompilationUnit*>& compilationUnits = module.non_if_data.compilationUnits;
	for ( size_t i = 0; i < compilationUnits.size (); i++ )
	{
		const CompilationUnit& compilationUnit = *compilationUnits[i];
		const FileLocation& sourceFile = compilationUnit.GetFilename ();
		string extension = GetExtension ( sourceFile );
		if ( extension == ".spec" || extension == ".SPEC" )
			GetSpecObjectDependencies ( dependencies, &sourceFile );
		if ( extension == ".idl" || extension == ".IDL" )
		{
			if ( ( module.type == RpcServer ) || ( module.type == RpcClient ) || ( module.type == RpcProxy ) )
				GetWidlObjectDependencies ( dependencies, &sourceFile );
		}
	}
}

enum DebugSupportType
{
	DebugKernelMode,
	DebugUserMode
};

static void
MingwAddDebugSupportLibraries ( Module& module, DebugSupportType type )
{
	Library* pLibrary;

	switch(type)
	{
		case DebugKernelMode:
			pLibrary = new Library ( module, "debugsup_ntoskrnl" );
			break;

		case DebugUserMode:
			pLibrary = new Library ( module, "debugsup_ntdll" );
			break;

		default:
			assert(0);
	}

	module.non_if_data.libraries.push_back(pLibrary);
}

MingwBuildToolModuleHandler::MingwBuildToolModuleHandler ( const Module& module_ )
	: MingwModuleHandler ( module_ )
{
}

void
MingwBuildToolModuleHandler::Process ()
{
	GenerateBuildToolModuleTarget ();
}

void
MingwBuildToolModuleHandler::GenerateBuildToolModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateRules ();

	string linker;
	if ( module.cplusplus )
		linker = "${host_gpp}";
	else
		linker = "${host_gcc}";

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );
	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );
	fprintf ( fMakefile,
	          "\t%s %s -o $@ %s %s\n\n",
	          linker.c_str (),
	          GetLinkerMacro ().c_str (),
	          objectsMacro.c_str (),
	          libsMacro.c_str () );

	delete target_file;
}


MingwKernelModuleHandler::MingwKernelModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwKernelModuleHandler::Process ()
{
	GenerateKernelModuleTarget ();
}

void
MingwKernelModuleHandler::GenerateKernelModuleTarget ()
{
	string targetMacro ( GetTargetMacro ( module ) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s",
		                                     module.GetEntryPoint(!(Environment::GetArch() == "arm")).c_str (),
		                                     module.baseaddress.c_str () );

		GenerateLinkerCommand ( dependencies,
		                        linkerParameters + " $(NTOSKRNL_SHARED)",
		                        " -sections" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwStaticLibraryModuleHandler::MingwStaticLibraryModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwStaticLibraryModuleHandler::Process ()
{
	GenerateStaticLibraryModuleTarget ();
}

void
MingwStaticLibraryModuleHandler::GenerateStaticLibraryModuleTarget ()
{
	GenerateRules ();
}


MingwHostStaticLibraryModuleHandler::MingwHostStaticLibraryModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwHostStaticLibraryModuleHandler::Process ()
{
	GenerateHostStaticLibraryModuleTarget ();
}

void
MingwHostStaticLibraryModuleHandler::GenerateHostStaticLibraryModuleTarget ()
{
	GenerateRules ();
}


MingwObjectLibraryModuleHandler::MingwObjectLibraryModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwObjectLibraryModuleHandler::Process ()
{
	GenerateObjectLibraryModuleTarget ();
}

void
MingwObjectLibraryModuleHandler::GenerateObjectLibraryModuleTarget ()
{
	GenerateRules ();
}


MingwKernelModeDLLModuleHandler::MingwKernelModeDLLModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

MingwEmbeddedTypeLibModuleHandler::MingwEmbeddedTypeLibModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwEmbeddedTypeLibModuleHandler::Process ()
{
	GenerateRules ();
}


void
MingwKernelModeDLLModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddDebugSupportLibraries ( module, DebugKernelMode );
}

void
MingwKernelModeDLLModuleHandler::Process ()
{
	GenerateKernelModeDLLModuleTarget ();
}

void
MingwKernelModeDLLModuleHandler::GenerateKernelModeDLLModuleTarget ()
{
	string targetMacro ( GetTargetMacro ( module ) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        " -sections" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwKernelModeDriverModuleHandler::MingwKernelModeDriverModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwKernelModeDriverModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddDebugSupportLibraries ( module, DebugKernelMode );
}

void
MingwKernelModeDriverModuleHandler::Process ()
{
	GenerateKernelModeDriverModuleTarget ();
}


void
MingwKernelModeDriverModuleHandler::GenerateKernelModeDriverModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        " -sections" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwNativeDLLModuleHandler::MingwNativeDLLModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwNativeDLLModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
}

void
MingwNativeDLLModuleHandler::Process ()
{
	GenerateNativeDLLModuleTarget ();
}

void
MingwNativeDLLModuleHandler::GenerateNativeDLLModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwNativeCUIModuleHandler::MingwNativeCUIModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwNativeCUIModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
}

void
MingwNativeCUIModuleHandler::Process ()
{
	GenerateNativeCUIModuleTarget ();
}

void
MingwNativeCUIModuleHandler::GenerateNativeCUIModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,native -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -nostartfiles -nostdlib",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwWin32DLLModuleHandler::MingwWin32DLLModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

MingwWin32OCXModuleHandler::MingwWin32OCXModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

static bool
LinksToCrt( Module &module )
{
	for ( size_t i = 0; i < module.non_if_data.libraries.size (); i++ )
	{
		Library& library = *module.non_if_data.libraries[i];
		if ( library.name == "libcntpr" || library.name == "crt" )
			return true;
	}
	return false;
}

static void
MingwAddImplicitLibraries( Module &module )
{
	Library* pLibrary;
	bool links_to_crt;

	if ( module.type != Win32DLL
	  && module.type != Win32OCX
	  && module.type != Win32CUI
	  && module.type != Win32GUI
	  && module.type != Win32SCR )
	{
		// no implicit libraries
		return;
	}

	links_to_crt = LinksToCrt ( module );

	if ( !module.isDefaultEntryPoint )
	{
		if ( module.GetEntryPoint(false) == "0" )
		{
			if ( !links_to_crt )
			{
				pLibrary = new Library ( module, "mingw_common" );
				module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin() , pLibrary );

				pLibrary = new Library ( module, "msvcrt" );
				module.non_if_data.libraries.push_back ( pLibrary );
				links_to_crt = true;
			}
		}
		return;
	}

	if ( module.IsDLL () )
	{
		//pLibrary = new Library ( module, "__mingw_dllmain" );
		//module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin(), pLibrary );
	}
	else
	{
		pLibrary = new Library ( module, module.isUnicode ? "mingw_wmain" : "mingw_main" );
		module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin(), pLibrary );
	}

	pLibrary = new Library ( module, "mingw_common" );
	module.non_if_data.libraries.insert ( module.non_if_data.libraries.begin() + 1, pLibrary );

	if ( !links_to_crt )
	{
		// always link in msvcrt to get the basic routines
		pLibrary = new Library ( module, "msvcrt" );
		module.non_if_data.libraries.push_back ( pLibrary );
	}
}

void
MingwWin32DLLModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
}

void
MingwWin32DLLModuleHandler::Process ()
{
	GenerateWin32DLLModuleTarget ();
}

void
MingwWin32DLLModuleHandler::GenerateWin32DLLModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


void
MingwWin32OCXModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
}

void
MingwWin32OCXModuleHandler::Process ()
{
	GenerateWin32OCXModuleTarget ();
}

void
MingwWin32OCXModuleHandler::GenerateWin32OCXModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000 -shared",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwWin32CUIModuleHandler::MingwWin32CUIModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwWin32CUIModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
}

void
MingwWin32CUIModuleHandler::Process ()
{
	GenerateWin32CUIModuleTarget ();
}

void
MingwWin32CUIModuleHandler::GenerateWin32CUIModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwWin32GUIModuleHandler::MingwWin32GUIModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwWin32GUIModuleHandler::AddImplicitLibraries ( Module& module )
{
	MingwAddImplicitLibraries ( module );
	MingwAddDebugSupportLibraries ( module, DebugUserMode );
}

void
MingwWin32GUIModuleHandler::Process ()
{
	GenerateWin32GUIModuleTarget ();
}

void
MingwWin32GUIModuleHandler::GenerateWin32GUIModuleTarget ()
{
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,windows -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwBootLoaderModuleHandler::MingwBootLoaderModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwBootLoaderModuleHandler::Process ()
{
	GenerateBootLoaderModuleTarget ();
}

void
MingwBootLoaderModuleHandler::GenerateBootLoaderModuleTarget ()
{
	string targetName ( module.output->name );
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	FileLocation junk_tmp ( TemporaryDirectory,
	                        "",
	                        module.name + ".junk.tmp" );
	CLEAN_FILE ( junk_tmp );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateRules ();

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_LD)\n" );

	if (Environment::GetArch() == "arm")
	{
		fprintf ( fMakefile,
		         "\t${gcc} -Wl,--subsystem,native -Wl,--section-start,startup=0x8000 -o %s %s %s %s\n",
		         backend->GetFullName ( junk_tmp ).c_str (),
		         objectsMacro.c_str (),
		         linkDepsMacro.c_str (),
		         GetLinkerMacro ().c_str ());
	}
	else
	{
		fprintf ( fMakefile,
		         "\t${gcc} -Wl,--subsystem,native -Wl,-Ttext,0x8000 -o %s %s %s %s\n",
		         backend->GetFullName ( junk_tmp ).c_str (),
		         objectsMacro.c_str (),
		         linkDepsMacro.c_str (),
		         GetLinkerMacro ().c_str ());
	}
	fprintf ( fMakefile,
	          "\t${objcopy} -O binary %s $@\n",
	          backend->GetFullName ( junk_tmp ).c_str () );
	GenerateBuildMapCode ( &junk_tmp );
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          backend->GetFullName ( junk_tmp ).c_str () );

	delete target_file;
}


MingwBootSectorModuleHandler::MingwBootSectorModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwBootSectorModuleHandler::Process ()
{
	GenerateBootSectorModuleTarget ();
}

void
MingwBootSectorModuleHandler::GenerateBootSectorModuleTarget ()
{
	string objectsMacro = GetObjectsMacro ( module );

	GenerateRules ();

	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: %s\n",
	          module.name.c_str (),
	          objectsMacro.c_str () );
}


MingwBootProgramModuleHandler::MingwBootProgramModuleHandler (
	const Module& module_ )
	: MingwModuleHandler ( module_ )
{
}

void
MingwBootProgramModuleHandler::Process ()
{
	GenerateBootProgramModuleTarget ();
}

void
MingwBootProgramModuleHandler::GenerateBootProgramModuleTarget ()
{
	string targetName ( module.output->name );
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	FileLocation junk_tmp ( TemporaryDirectory,
	                        "",
	                        module.name + ".junk.tmp" );
	FileLocation junk_elf ( TemporaryDirectory,
	                        "",
	                        module.name + ".junk.elf" );
	FileLocation junk_cpy ( TemporaryDirectory,
	                        "",
	                        module.name + ".junk.elf" );
	CLEAN_FILE ( junk_tmp );
	CLEAN_FILE ( junk_elf );
	CLEAN_FILE ( junk_cpy );
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();
	const Module *payload = module.project.LocateModule ( module.payload );

	GenerateRules ();

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: %s %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str (),
	          payload->name.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_BOOTPROG)\n" );

	fprintf ( fMakefile, "\t$(%s_PREPARE) $(OUTPUT)$(SEP)%s %s\n",
		module.buildtype.c_str (),
		NormalizeFilename( backend->GetFullName ( *payload->output ) ).c_str (),
		backend->GetFullName ( junk_cpy ).c_str () );

	fprintf ( fMakefile, "\t${objcopy} $(%s_FLATFORMAT) %s %s\n",
		module.buildtype.c_str (),
		backend->GetFullName ( junk_cpy ).c_str (),
		backend->GetFullName ( junk_tmp ).c_str () );

	fprintf ( fMakefile, "\t${ld} $(%s_LINKFORMAT) %s %s -g -o %s\n",
		module.buildtype.c_str (),
		linkDepsMacro.c_str (),
		backend->GetFullName ( junk_tmp ).c_str (),
		backend->GetFullName ( junk_elf ).c_str () );

	fprintf ( fMakefile, "\t${objcopy} $(%s_COPYFORMAT) %s $(INTERMEDIATE)$(SEP)%s\n",
		module.buildtype.c_str (),
		backend->GetFullName ( junk_elf ).c_str (),
		backend->GetFullName ( *module.output ) .c_str () );

	fprintf ( fMakefile,
	          "\t-@${rm} %s %s %s 2>$(NUL)\n",
	          backend->GetFullName ( junk_tmp ).c_str (),
	          backend->GetFullName ( junk_elf ).c_str (),
	          backend->GetFullName ( junk_cpy ).c_str () );

	delete target_file;
}


MingwIsoModuleHandler::MingwIsoModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwIsoModuleHandler::Process ()
{
	GenerateIsoModuleTarget ();
}

void
MingwIsoModuleHandler::OutputBootstrapfileCopyCommands (
	const string& bootcdDirectory )
{
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			FileLocation targetFile ( OutputDirectory,
			                          m.bootstrap->base.length () > 0
			                                   ? bootcdDirectory + sSep + m.bootstrap->base
			                                   : bootcdDirectory,
			                          m.bootstrap->nameoncd );
			OutputCopyCommand ( *m.output, targetFile );
		}
	}
}

void
MingwIsoModuleHandler::OutputCdfileCopyCommands (
	const string& bootcdDirectory )
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		FileLocation targetFile ( OutputDirectory,
		                          cdfile.target->relative_path.length () > 0
		                              ? bootcdDirectory + sSep + cdfile.target->relative_path
		                              : bootcdDirectory,
		                          cdfile.target->name );
		OutputCopyCommand ( *cdfile.source, targetFile );
	}
}

void
MingwIsoModuleHandler::GetBootstrapCdDirectories ( vector<FileLocation>& out,
                                                   const string& bootcdDirectory )
{
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			FileLocation targetDirectory ( OutputDirectory,
			                               m.bootstrap->base.length () > 0
			                                   ? bootcdDirectory + sSep + m.bootstrap->base
			                                   : bootcdDirectory,
			                               "" );
			out.push_back ( targetDirectory );
		}
	}
}

void
MingwIsoModuleHandler::GetNonModuleCdDirectories ( vector<FileLocation>& out,
                                                   const string& bootcdDirectory )
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		FileLocation targetDirectory ( OutputDirectory,
		                               cdfile.target->relative_path.length () > 0
		                                   ? bootcdDirectory + sSep + cdfile.target->relative_path
		                                   : bootcdDirectory,
		                               "" );
		out.push_back( targetDirectory );
	}
}

void
MingwIsoModuleHandler::GetCdDirectories ( vector<FileLocation>& out,
                                          const string& bootcdDirectory )
{
	GetBootstrapCdDirectories ( out, bootcdDirectory );
	GetNonModuleCdDirectories ( out, bootcdDirectory );
}

void
MingwIsoModuleHandler::GetBootstrapCdFiles (
	vector<FileLocation>& out ) const
{
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( !m.enabled )
			continue;
		if ( m.bootstrap != NULL )
		{
			out.push_back ( *m.output );
		}
	}
}

void
MingwIsoModuleHandler::GetNonModuleCdFiles (
	vector<FileLocation>& out ) const
{
	for ( size_t i = 0; i < module.project.cdfiles.size (); i++ )
	{
		const CDFile& cdfile = *module.project.cdfiles[i];
		out.push_back ( *cdfile.source );
	}
}

void
MingwIsoModuleHandler::GetCdFiles (
	vector<FileLocation>& out ) const
{
	GetBootstrapCdFiles ( out );
	GetNonModuleCdFiles ( out );
}

void
MingwIsoModuleHandler::GenerateIsoModuleTarget ()
{
	string bootcdDirectory = "cd";
	FileLocation bootcd ( OutputDirectory,
	                      bootcdDirectory,
	                      "" );
	FileLocation bootcdReactos ( OutputDirectory,
	                             bootcdDirectory + sSep + Environment::GetCdOutputPath (),
	                             "" );
	vector<FileLocation> vSourceFiles, vCdFiles;
	vector<FileLocation> vCdDirectories;

	// unattend.inf
	FileLocation srcunattend ( SourceDirectory,
	                           "boot" + sSep + "bootdata" + sSep + "bootcdregtest",
	                           "unattend.inf" );
	FileLocation tarunattend ( bootcdReactos.directory,
	                           bootcdReactos.relative_path,
	                           "unattend.inf" );
	if (module.type == IsoRegTest)
		vSourceFiles.push_back ( srcunattend );

	// bootsector
	const Module* bootModule;
	bootModule = module.project.LocateModule ( module.type == IsoRegTest
	                                               ? "isobtrt"
	                                               : "isoboot" );
	const FileLocation *isoboot = bootModule->output;
	vSourceFiles.push_back ( *isoboot );

	// prepare reactos.dff and reactos.inf
	FileLocation reactosDff ( SourceDirectory,
	                          "boot" + sSep + "bootdata" + sSep + "packages",
	                          "reactos.dff" );
	FileLocation reactosInf ( bootcdReactos.directory,
	                          bootcdReactos.relative_path,
	                          "reactos.inf" );

	vSourceFiles.push_back ( reactosDff );

	string IsoName;

	if (module.type == IsoRegTest)
		IsoName = "ReactOS-RegTest.iso";
	else
		IsoName = "ReactOS.iso";


	string sourceFiles = v2s ( backend, vSourceFiles, 5 );

	// fill cdrom
	GetCdDirectories ( vCdDirectories, bootcdDirectory );
	GetCdFiles ( vCdFiles );
	string cdDirectories = "";//v2s ( vCdDirectories, 5 );
	string cdFiles = v2s ( backend, vCdFiles, 5 );

	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: all %s %s %s $(CABMAN_TARGET) $(CDMAKE_TARGET) %s\n",
	          module.name.c_str (),
	          backend->GetFullName ( *isoboot ).c_str (),
	          sourceFiles.c_str (),
	          cdFiles.c_str (),
	          cdDirectories.c_str () );
	fprintf ( fMakefile,
	          "\t$(Q)$(CABMAN_TARGET) -C %s -L %s -I -P $(OUTPUT)\n",
	          backend->GetFullName ( reactosDff ).c_str (),
	          backend->GetFullPath ( bootcdReactos ).c_str () );
	fprintf ( fMakefile,
	          "\t$(Q)$(CABMAN_TARGET) -C %s -RC %s -L %s -N -P $(OUTPUT)\n",
	          backend->GetFullName ( reactosDff ).c_str (),
	          backend->GetFullName ( reactosInf ).c_str (),
	          backend->GetFullPath ( bootcdReactos ).c_str ());
	fprintf ( fMakefile,
	          "\t-@${rm} %s 2>$(NUL)\n",
	          backend->GetFullName ( reactosInf ).c_str () );
	OutputBootstrapfileCopyCommands ( bootcdDirectory );
	OutputCdfileCopyCommands ( bootcdDirectory );

	if (module.type == IsoRegTest)
		OutputCopyCommand ( srcunattend, tarunattend );

	fprintf ( fMakefile, "\t$(ECHO_CDMAKE)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(CDMAKE_TARGET) -v -j -m -b %s %s REACTOS %s\n",
	          backend->GetFullName ( *isoboot ).c_str (),
	          backend->GetFullPath ( bootcd ).c_str (),
	          IsoName.c_str() );
	fprintf ( fMakefile,
	          "\n" );
}


MingwLiveIsoModuleHandler::MingwLiveIsoModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwLiveIsoModuleHandler::Process ()
{
	GenerateLiveIsoModuleTarget ();
}

void
MingwLiveIsoModuleHandler::CreateDirectory ( const string& directory )
{
	FileLocation dir ( OutputDirectory,
	                   directory,
	                   "" );
	MingwModuleHandler::PassThruCacheDirectory ( &dir );
}

void
MingwLiveIsoModuleHandler::OutputModuleCopyCommands ( string& livecdDirectory,
                                                      string& reactosDirectory )
{
	for ( size_t i = 0; i < module.project.modules.size (); i++ )
	{
		const Module& m = *module.project.modules[i];
		if ( !m.enabled )
			continue;
		if ( m.install )
		{
			const Module& aliasedModule = backend->GetAliasedModuleOrModule ( m  );
			FileLocation destination ( OutputDirectory,
			                           m.install->relative_path.length () > 0
			                               ? livecdDirectory + sSep + reactosDirectory + sSep + m.install->relative_path
			                               : livecdDirectory + sSep + reactosDirectory,
			                           m.install->name );
			OutputCopyCommand ( *aliasedModule.output,
			                    destination);
		}
	}
}

void
MingwLiveIsoModuleHandler::OutputNonModuleCopyCommands ( string& livecdDirectory,
                                                         string& reactosDirectory )
{
	for ( size_t i = 0; i < module.project.installfiles.size (); i++ )
	{
		const InstallFile& installfile = *module.project.installfiles[i];
		FileLocation target ( OutputDirectory,
		                      installfile.target->relative_path.length () > 0
		                          ? livecdDirectory + sSep + reactosDirectory + sSep + installfile.target->relative_path
		                          : livecdDirectory + sSep + reactosDirectory,
		                      installfile.target->name );
		OutputCopyCommand ( *installfile.source, target );
	}
}

void
MingwLiveIsoModuleHandler::OutputProfilesDirectoryCommands ( string& livecdDirectory )
{
	CreateDirectory ( livecdDirectory + sSep + "Profiles" );
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "All Users") ;
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "All Users" + sSep + "Desktop" );
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "Default User" );
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "Default User" + sSep + "Desktop" );
	CreateDirectory ( livecdDirectory + sSep + "Profiles" + sSep + "Default User" + sSep + "My Documents" );

	FileLocation livecdIni ( SourceDirectory,
	                         "boot" + sSep + "bootdata",
	                         "livecd.ini" );
	FileLocation destination ( OutputDirectory,
	                           livecdDirectory,
	                           "freeldr.ini" );
	OutputCopyCommand ( livecdIni,
	                    destination );
}

void
MingwLiveIsoModuleHandler::OutputLoaderCommands ( string& livecdDirectory )
{
	FileLocation freeldr ( OutputDirectory,
	                       "boot" + sSep + "freeldr" + sSep + "freeldr",
	                       "freeldr.sys" );
	FileLocation destination ( OutputDirectory,
	                           livecdDirectory + sSep + "loader",
	                           "setupldr.sys" );
	OutputCopyCommand ( freeldr,
	                    destination );
}

void
MingwLiveIsoModuleHandler::OutputRegistryCommands ( string& livecdDirectory )
{
	FileLocation reactosSystem32ConfigDirectory ( OutputDirectory,
	                                              livecdDirectory + sSep + "reactos" + sSep + "system32" + sSep + "config",
	                                              "" );
	fprintf ( fMakefile,
	          "\t$(ECHO_MKHIVE)\n" );
	fprintf ( fMakefile,
	          "\t$(MKHIVE_TARGET) boot%cbootdata %s boot%cbootdata%clivecd.inf boot%cbootdata%chiveinst.inf\n",
	          cSep, backend->GetFullPath ( reactosSystem32ConfigDirectory ).c_str (),
	          cSep, cSep, cSep, cSep );
}

void
MingwLiveIsoModuleHandler::GenerateLiveIsoModuleTarget ()
{
	string livecdDirectory = module.name;
	FileLocation livecd ( OutputDirectory, livecdDirectory, "" );

	string IsoName;

	const Module* bootModule;
	bootModule = module.project.LocateModule ( module.name == "livecdregtest"
	                                               ? "isobtrt"
	                                               : "isoboot" );
	const FileLocation *isoboot = bootModule->output;
	if (module.name == "livecdregtest")
		IsoName = "ReactOS-LiveCD-RegTest.iso";
	else
		IsoName = "ReactOS-LiveCD.iso";

	string reactosDirectory = "reactos";
	string livecdReactosNoFixup = livecdDirectory + sSep + reactosDirectory;
	FileLocation livecdReactos ( OutputDirectory,
	                             livecdReactosNoFixup,
	                             "" );
	CLEAN_FILE ( livecdReactos );

	fprintf ( fMakefile, ".PHONY: %s\n\n",
	          module.name.c_str ());
	fprintf ( fMakefile,
	          "%s: all %s %s $(MKHIVE_TARGET) $(CDMAKE_TARGET)\n",
	          module.name.c_str (),
	          backend->GetFullName ( *isoboot) .c_str (),
	          backend->GetFullPath ( livecdReactos ).c_str () );
	OutputModuleCopyCommands ( livecdDirectory,
	                           reactosDirectory );
	OutputNonModuleCopyCommands ( livecdDirectory,
	                              reactosDirectory );
	OutputProfilesDirectoryCommands ( livecdDirectory );
	OutputLoaderCommands ( livecdDirectory );
	OutputRegistryCommands ( livecdDirectory );
	fprintf ( fMakefile, "\t$(ECHO_CDMAKE)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(CDMAKE_TARGET) -v -m -j -b %s %s REACTOS %s\n",
	          backend->GetFullName( *isoboot ).c_str (),
	          backend->GetFullPath ( livecd ).c_str (),
	          IsoName.c_str() );
	fprintf ( fMakefile,
	          "\n" );
}


MingwTestModuleHandler::MingwTestModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwTestModuleHandler::Process ()
{
	GenerateTestModuleTarget ();
}

/* caller needs to delete the returned object */
void
MingwTestModuleHandler::GetModuleSpecificCompilationUnits ( vector<CompilationUnit*>& compilationUnits )
{
	compilationUnits.push_back ( new CompilationUnit ( new File ( IntermediateDirectory, module.output->relative_path + sSep + "..", module.name + "_hooks.c", false, "", false ) ) );
	compilationUnits.push_back ( new CompilationUnit ( new File ( IntermediateDirectory, module.output->relative_path + sSep + "..", module.name + "_stubs.S", false, "", false ) ) );
	compilationUnits.push_back ( new CompilationUnit ( new File ( IntermediateDirectory, module.output->relative_path + sSep + "..", module.name + "_startup.c", false, "", false ) ) );
}

void
MingwTestModuleHandler::GenerateTestModuleTarget ()
{
	string targetMacro ( GetTargetMacro ( module ) );
	string workingDirectory = GetWorkingDirectory ( );
	string linkDepsMacro = GetLinkingDependenciesMacro ();

	GenerateImportLibraryTargetIfNeeded ();

	if ( module.non_if_data.compilationUnits.size () > 0 )
	{
		GenerateRules ();

		string dependencies = linkDepsMacro + " " + objectsMacro;

		string linkerParameters = ssprintf ( "-Wl,--subsystem,console -Wl,--entry,%s -Wl,--image-base,%s -Wl,--file-alignment,0x1000 -Wl,--section-alignment,0x1000",
		                                     module.GetEntryPoint(true).c_str (),
		                                     module.baseaddress.c_str () );
		GenerateLinkerCommand ( dependencies,
		                        linkerParameters,
		                        "" );
	}
	else
	{
		GeneratePhonyTarget();
	}
}


MingwRpcServerModuleHandler::MingwRpcServerModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwRpcServerModuleHandler::Process ()
{
	GenerateRules ();
}


MingwRpcClientModuleHandler::MingwRpcClientModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwRpcClientModuleHandler::Process ()
{
	GenerateRules ();
}


MingwRpcProxyModuleHandler::MingwRpcProxyModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwRpcProxyModuleHandler::Process ()
{
	GenerateRules ();
}


MingwAliasModuleHandler::MingwAliasModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwAliasModuleHandler::Process ()
{
}

MingwIdlHeaderModuleHandler::MingwIdlHeaderModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwIdlHeaderModuleHandler::Process ()
{
	GenerateRules ();
}

MingwCabinetModuleHandler::MingwCabinetModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwCabinetModuleHandler::Process ()
{
	string targetMacro ( GetTargetMacro (module) );

	GenerateRules ();
	
	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: | %s\n",
	          targetMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_CABMAN)\n" );
	fprintf ( fMakefile,
	          "\t$(Q)$(CABMAN_TARGET) -M raw -S %s $(%s_SOURCES)\n",      // Escape the asterisk for Make
	          targetMacro.c_str (),
			  module.name.c_str());
}

MingwElfExecutableModuleHandler::MingwElfExecutableModuleHandler (
	const Module& module_ )

	: MingwModuleHandler ( module_ )
{
}

void
MingwElfExecutableModuleHandler::Process ()
{
	string targetName ( module.output->name );
	string targetMacro ( GetTargetMacro (module) );
	string workingDirectory = GetWorkingDirectory ();
	string objectsMacro = GetObjectsMacro ( module );
	string linkDepsMacro = GetLinkingDependenciesMacro ();
	string libsMacro = GetLibsMacro ();

	GenerateRules ();

	const FileLocation *target_file = GetTargetFilename ( module, NULL );
	fprintf ( fMakefile, "%s: %s %s | %s\n",
	          targetMacro.c_str (),
	          objectsMacro.c_str (),
	          linkDepsMacro.c_str (),
	          backend->GetFullPath ( *target_file ).c_str () );

	fprintf ( fMakefile, "\t$(ECHO_BOOTPROG)\n" );

	fprintf ( fMakefile, "\t${gcc} $(%s_LINKFORMAT) %s %s -g -o %s\n",
	          module.buildtype.c_str(),
	          objectsMacro.c_str(),
	          libsMacro.c_str(),
	          targetMacro.c_str () );

	delete target_file;
}
