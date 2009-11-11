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
#ifndef MINGW_MODULEHANDLER_H
#define MINGW_MODULEHANDLER_H

#include "../backend.h"
#include "mingw.h"

class MingwBackend;
class Rule;

extern std::string
GetTargetMacro ( const Module&, bool with_dollar = true );

extern std::string
PrefixFilename (
	const std::string& filename,
	const std::string& prefix );

enum SpecFileType
{
    None,
    Spec = 1,
    PSpec = 2
};

class MingwModuleHandler
{
public:
	MingwModuleHandler ( const Module& module_ );
	virtual ~MingwModuleHandler();

	static void SetBackend ( MingwBackend* backend_ );
	static void SetMakefile ( FILE* f );
	void EnablePreCompiledHeaderSupport ();

	static const FileLocation* PassThruCacheDirectory (const FileLocation* fileLocation );

	static const FileLocation* GetTargetFilename (
		const Module& module,
		string_list* pclean_files );

	static const FileLocation* GetImportLibraryFilename (
		const Module& module,
		string_list* pclean_files,
		bool delayimp );

	static std::string GenerateGccDefineParametersFromVector ( const std::vector<Define*>& defines, std::set<std::string> &used_defs );
	static std::string GenerateDefineParametersFromVector ( const std::vector<Define*>& defines, CompilerType compiler );
	static std::string GenerateCompilerParametersFromVector ( const std::vector<CompilerFlag*>& compilerFlags, const CompilerType type );
	static std::string GenerateIncludeParametersFromVector ( const std::vector<Include*>& includes, CompilerType compiler );

	static void GenerateParameters ( const char* prefix,
									 const char* assignmentOperation,
									 const IfableData& data );

	std::string GetModuleTargets ( const Module& module );
	void GetObjectsVector ( const IfableData& data,
	                        std::vector<FileLocation>& objectFiles ) const;
	void GenerateSourceMacro();
	void GenerateObjectMacro();
	void GenerateTargetMacro();
	void GenerateOtherMacros();

	static MingwModuleHandler* InstanciateHandler ( const Module& module_,
	                                                MingwBackend* backend_ );
	void GeneratePreconditionDependencies ();
	virtual void Process () { GenerateRules (); }
	void GenerateInvocations () const;
	void GenerateCleanTarget () const;
	void GenerateInstallTarget () const;
	void GenerateDependsTarget () const;
	static bool ReferenceObjects ( const Module& module );
	virtual void AddImplicitLibraries ( Module& module ) { return; }

	void OutputCopyCommand ( const FileLocation& source,
	                         const FileLocation& destination );
	void OutputCopyCommandSingle ( const FileLocation& source,
	                               const FileLocation& destination );
protected:
	virtual void GetModuleSpecificCompilationUnits ( std::vector<CompilationUnit*>& compilationUnits );
	std::string GetWorkingDirectory () const;
	std::string GetBasename ( const std::string& filename ) const;
	std::string GetCompilationUnitDependencies ( const CompilationUnit& compilationUnit ) const;
	const FileLocation* GetModuleArchiveFilename () const;
	std::string GetImportLibraryDependency ( const Module& importedModule, bool delayimp );
	void GetTargets ( const Module& dependencyModule,
	                  string_list& targets );
	void GetModuleDependencies ( string_list& dependencies );
	std::string GetAllDependencies () const;
	const FileLocation* GetObjectFilename ( const FileLocation* sourceFile,
	                                        const Module& module ) const;

	std::string GetPreconditionDependenciesName () const;
	static std::string GetObjectsMacro ( const Module& );
	std::string GetLinkingDependenciesMacro () const;
	std::string GetLibsMacro () const;
	std::string GetLinkerMacro () const;
	static std::string GetDebugFormat ();
	void GenerateCleanObjectsAsYouGoCode () const;
	void GenerateLinkerCommand () const;
	void GenerateBuildMapCode ( const FileLocation *mapTarget = NULL );
	void GenerateRules ();
	void GenerateImportLibraryTargetIfNeeded ();
	void GetDefinitionDependencies ( std::vector<FileLocation>& dependencies ) const;
	std::string GetLinkingDependencies () const;
	static MingwBackend* backend;
	static FILE* fMakefile;
	bool use_pch;
private:
	std::string ConcatenatePaths ( const std::string& path1,
	                               const std::string& path2 ) const;
	std::string GenerateLinkerParametersFromVector ( const std::vector<LinkerFlag*>& linkerFlags ) const;
	std::string GenerateImportLibraryDependenciesFromVector ( const std::vector<Library*>& libraries );
	std::string GenerateLinkerParameters () const;
	void GenerateMacros ( const char* op,
	                      const IfableData& data,
	                      const std::vector<LinkerFlag*>* linkerFlags,
	                      std::set<const Define *>& used_defs );
	void GenerateSourceMacros ( const IfableData& data );
	void GenerateObjectMacros ( const IfableData& data );
	const FileLocation* GetPrecompiledHeaderFilename () const;
	const FileLocation* GetPrecompiledHeaderPath () const;
	const FileLocation* GetDlldataFilename () const;
	void GenerateGccCommand ( const FileLocation* sourceFile,
	                          const Rule *rule,
	                          const std::string& extraDependencies );
	void GenerateCommands ( const CompilationUnit& compilationUnit,
	                        const std::string& extraDependencies );
	void GenerateObjectFileTargets ( const IfableData& data );
	void GenerateObjectFileTargets ();
	const FileLocation* GenerateArchiveTarget ();
	void GetMcObjectDependencies   ( std::vector<FileLocation>& dependencies,
	                                 const FileLocation *file ) const;
	void GetSpecObjectDependencies ( std::vector<FileLocation>& dependencies,
	                                 const FileLocation *file ) const;
	void GetSpecImplibDependencies ( std::vector<FileLocation>& dependencies,
	                                 const FileLocation *file ) const;
	void GetWidlObjectDependencies ( std::vector<FileLocation>& dependencies,
	                                 const FileLocation *file ) const;
	void GetDefaultDependencies ( string_list& dependencies ) const;
	void GetInvocationDependencies ( const Module& module, string_list& dependencies );
	SpecFileType IsSpecDefinitionFile () const;
	const FileLocation* GetDefinitionFilename () const;
	void GenerateBuildNonSymbolStrippedCode ();
	void CleanupCompilationUnitVector ( std::vector<CompilationUnit*>& compilationUnits );
	void GetRpcHeaderDependencies ( std::vector<FileLocation>& dependencies ) const;
	void GetMcHeaderDependencies ( std::vector<FileLocation>& dependencies ) const;
	static std::string GetPropertyValue ( const Module& module, const std::string& name );
	const FileLocation* GetRpcServerHeaderFilename ( const FileLocation *base ) const;
	const FileLocation* GetRpcClientHeaderFilename ( const FileLocation *base ) const;
	const FileLocation* GetRpcProxyHeaderFilename ( const FileLocation *base ) const;
	const FileLocation* GetIdlHeaderFilename ( const FileLocation *base ) const;
	const FileLocation* GetMcHeaderFilename ( const FileLocation *base ) const;
	std::string GetModuleCleanTarget ( const Module& module ) const;
	void GetReferencedObjectLibraryModuleCleanTargets ( std::vector<std::string>& moduleNames ) const;
public:
	const Module& module;
	string_list clean_files;
	std::string commonflagsMacro;
	std::string cflagsMacro;
	std::string cxxflagsMacro;
	std::string nasmflagsMacro;
	std::string windresflagsMacro;
	std::string widlflagsMacro;
	std::string linkerflagsMacro;
	std::string sourcesMacro;
	std::string objectsMacro;
	std::string libsMacro;
	std::string linkDepsMacro;
};


class MingwBuildToolModuleHandler : public MingwModuleHandler
{
public:
	MingwBuildToolModuleHandler ( const Module& module );
	virtual void Process ();
private:
	void GenerateBuildToolModuleTarget ();
};


class MingwKernelModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModuleHandler ( const Module& module );
	virtual void Process ();
private:
	void GenerateKernelModuleTarget ();
};


class MingwKernelModeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwKernelModeDLLModuleHandler ( const Module& module );
	virtual void Process ();
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateKernelModeDLLModuleTarget ();
};


class MingwNativeDLLModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeDLLModuleHandler ( const Module& module );
	virtual void Process ();
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateNativeDLLModuleTarget ();
};


class MingwNativeCUIModuleHandler : public MingwModuleHandler
{
public:
	MingwNativeCUIModuleHandler ( const Module& module );
	virtual void Process ();
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateNativeCUIModuleTarget ();
};


class MingwWin32DLLModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32DLLModuleHandler ( const Module& module );
	virtual void Process ();
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateWin32DLLModuleTarget ();
};


class MingwWin32OCXModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32OCXModuleHandler ( const Module& module );
	virtual void Process ();
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateWin32OCXModuleTarget ();
};


class MingwWin32CUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32CUIModuleHandler ( const Module& module );
	virtual void Process ();
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateWin32CUIModuleTarget ();
};


class MingwWin32GUIModuleHandler : public MingwModuleHandler
{
public:
	MingwWin32GUIModuleHandler ( const Module& module );
	virtual void Process ();
	void AddImplicitLibraries ( Module& module );
private:
	void GenerateWin32GUIModuleTarget ();
};


class MingwBootLoaderModuleHandler : public MingwModuleHandler
{
public:
	MingwBootLoaderModuleHandler ( const Module& module );
	virtual void Process ();
private:
	void GenerateBootLoaderModuleTarget ();
};


class MingwBootProgramModuleHandler : public MingwModuleHandler
{
public:
	MingwBootProgramModuleHandler ( const Module& module );
	virtual void Process ();
	std::string GetProgTextAddrMacro ();
private:
	void GenerateBootProgramModuleTarget ();
};


class MingwIsoModuleHandler : public MingwModuleHandler
{
public:
	MingwIsoModuleHandler ( const Module& module );
	virtual void Process ();
private:
	void GenerateIsoModuleTarget ();
	void GetBootstrapCdDirectories ( std::vector<FileLocation>& out, const std::string& bootcdDirectory );
	void GetNonModuleCdDirectories ( std::vector<FileLocation>& out, const std::string& bootcdDirectory );
	void GetCdDirectories ( std::vector<FileLocation>& out, const std::string& bootcdDirectory );
	void GetBootstrapCdFiles ( std::vector<FileLocation>& out ) const;
	void GetNonModuleCdFiles ( std::vector<FileLocation>& out ) const;
	void GetCdFiles ( std::vector<FileLocation>& out ) const;
	void OutputBootstrapfileCopyCommands ( const std::string& bootcdDirectory,
	                                       std::vector<FileLocation>& destinations );
	void OutputCdfileCopyCommands ( const std::string& bootcdDirectory,
	                                std::vector<FileLocation>& destinations );
};


class MingwLiveIsoModuleHandler : public MingwModuleHandler
{
public:
	MingwLiveIsoModuleHandler ( const Module& module );
	virtual void Process ();
private:
	void GenerateLiveIsoModuleTarget ();
	void CreateDirectory ( const std::string& directory );
	void OutputModuleCopyCommands ( std::string& livecdDirectory,
	                                std::string& livecdReactos,
	                                std::vector<FileLocation>& destinations );
	void OutputNonModuleCopyCommands ( std::string& livecdDirectory,
	                                   std::string& livecdReactos,
	                                   std::vector<FileLocation>& destinations );
	void OutputProfilesDirectoryCommands ( std::string& livecdDirectory,
	                                       std::vector<FileLocation>& destinations );
	void OutputLoaderCommands ( std::string& livecdDirectory,
	                            std::vector<FileLocation>& destinations );
	void OutputRegistryCommands ( std::string& livecdDirectory );
};


class MingwTestModuleHandler : public MingwModuleHandler
{
public:
	MingwTestModuleHandler ( const Module& module );
	virtual void Process ();
protected:
	virtual void GetModuleSpecificCompilationUnits ( std::vector<CompilationUnit*>& compilationUnits );
private:
	void GenerateTestModuleTarget ();
};

class MingwAliasModuleHandler : public MingwModuleHandler
{
public:
	MingwAliasModuleHandler ( const Module& module );
	virtual void Process ();
};

class MingwCabinetModuleHandler : public MingwModuleHandler
{
public:
	MingwCabinetModuleHandler ( const Module& module );
	virtual void Process ();
};

class MingwElfExecutableModuleHandler : public MingwModuleHandler
{
public:
	MingwElfExecutableModuleHandler ( const Module& module );
	virtual void Process ();
};

#endif /* MINGW_MODULEHANDLER_H */
