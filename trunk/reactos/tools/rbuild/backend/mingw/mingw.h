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

#pragma once

#include "../backend.h"

#ifdef WIN32
	#define NUL "NUL"
#else
	#define NUL "/dev/null"
#endif

class Directory;
class MingwModuleHandler;


class MingwBackend : public Backend
{
public:
	MingwBackend ( Project& project,
	               Configuration& configuration );
	~MingwBackend ();
	virtual void Process ();
	std::string AddDirectoryTarget ( const std::string& directory,
	                                 Directory* directoryTree );
	const Module& GetAliasedModuleOrModule ( const Module& module ) const;
	bool compilerNeedsHelper;
	std::string compilerPrefix;
	std::string compilerCommand;
	std::string nasmCommand;
	std::string binutilsPrefix;
	bool binutilsNeedsHelper;
	std::string binutilsCommand;
	bool usePipe, manualBinutilsSetting;
	Directory* intermediateDirectory;
	Directory* outputDirectory;
	Directory* installDirectory;

	std::string GetFullName ( const FileLocation& file ) const;
	std::string GetFullPath ( const FileLocation& file ) const;
	std::string GetFullNamePrefixSpaces ( const FileLocation& file ) const;
	std::string GetRegistrySourceFiles () const;
	std::string GetRegistryTargetFiles () const;
	std::string v2s ( const std::vector<FileLocation>& files, int wrap_at, bool prefixSpaces = false ) const;
	std::string v2s ( const string_list& v, int wrap_at ) const;

private:
	void CreateMakefile ();
	void CloseMakefile () const;
	void GenerateHeader () const;
	void GenerateGlobalProperties ( const char* assignmentOperation,
									  const IfableData& data ) const;
	std::string GenerateProjectLDFLAGS () const;
	void GenerateDirectories ();
	void GenerateGlobalVariables () const;
	bool IncludeInAllTarget ( const Module& module ) const;
	void GenerateAllTarget ( const std::vector<MingwModuleHandler*>& handlers ) const;
	void GenerateRegTestsRunTarget () const;
	void GenerateXmlBuildFilesMacro() const;
	void GenerateTestSupportCode ();
	void GenerateCompilationUnitSupportCode ();
	void GenerateSysSetup ();
	std::string GetProxyMakefileTree () const;
	void GenerateProxyMakefiles ();
	void CheckAutomaticDependencies ();
	bool TryToDetectThisCompiler ( const std::string& compiler );
	void DetectCompiler ();
	std::string GetCompilerVersion ( const std::string& compilerCommand );
	bool IsSupportedCompilerVersion ( const std::string& compilerVersion );
	bool TryToDetectThisNetwideAssembler ( const std::string& assembler );
	bool TryToDetectThisBinutils ( const std::string& binutils );
	std::string GetBinutilsVersion ( const std::string& binutilsCommand );
	std::string GetBinutilsVersionDate ( const std::string& binutilsCommand );
	bool IsSupportedBinutilsVersion ( const std::string& binutilsVersion );
	std::string GetVersionString ( const std::string& versionCommand );
	std::string GetNetwideAssemblerVersion ( const std::string& nasmCommand );
	void DetectBinutils ();
	void DetectNetwideAssembler ();
	void DetectPipeSupport ();
	void DetectPCHSupport ();
	bool CanEnablePreCompiledHeaderSupportForModule ( const Module& module );
	void ProcessModules ();
	void CheckAutomaticDependenciesForModuleOnly ();
	void ProcessNormal ();
	std::string GetNonModuleInstallDirectories ( const std::string& installDirectory );
	std::string GetInstallDirectories ( const std::string& installDirectory );
	void GetNonModuleInstallFiles ( std::vector<std::string>& out ) const;
	void GetInstallFiles ( std::vector<std::string>& out ) const;
	void GetNonModuleInstallTargetFiles ( std::vector<FileLocation>& out ) const;
	void GetModuleInstallTargetFiles ( std::vector<FileLocation>& out ) const;
	void GetInstallTargetFiles ( std::vector<FileLocation>& out ) const;
	void OutputInstallTarget ( const FileLocation& source, const FileLocation& target );
	void OutputNonModuleInstallTargets ();
	void OutputModuleInstallTargets ();
	void OutputRegistryInstallTarget ();
	void GenerateInstallTarget ();
	void GetModuleTestTargets ( std::vector<std::string>& out ) const;
	void GenerateTestTarget ();
	void GenerateDirectoryTargets ();
	FILE* fMakefile;
	bool use_pch;
	bool DetectMicrosoftCompiler ( std::string& version, std::string& path );
	bool DetectMicrosoftLinker ( std::string& version, std::string& path );
};


class ProxyMakefile
{
public:
	ProxyMakefile ( const Project& project );
	~ProxyMakefile ();
	void GenerateProxyMakefiles ( bool verbose,
                                      std::string outputTree );
	static bool GenerateProxyMakefile ( const Module& module );

private:
	std::string GeneratePathToParentDirectory ( int numberOfParentDirectories );
	std::string GetPathToTopDirectory ( Module& module );
	void GenerateProxyMakefileForModule ( Module& module,
                                              bool verbose,
                                              std::string outputTree );
	const Project& project;
};

struct ModuleHandlerInformations
{
	HostType DefaultHost;
	const char* cflags;
	const char* nasmflags;
	const char* linkerflags;
};

extern const struct ModuleHandlerInformations ModuleHandlerInformations[];
