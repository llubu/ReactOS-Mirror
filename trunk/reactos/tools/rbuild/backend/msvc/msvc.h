/*
 * Copyright (C) 2005 Trevor McCort
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
#ifndef __MSVC_H__
#define __MSVC_H__

#include <fstream>
#include <vector>
#include <string>

#include "../backend.h"


class FileUnit
{
	public:
		std::string filename;
		std::string folder;
};

enum OptimizationType
{
	RosBuild,
	Debug,
	Release,
	Speed,
};

enum ConfigurationType
{
	ConfigUnknown,
	ConfigApp,
	ConfigDll,
	ConfigEmpty,
	ConfigLib
};

enum BinaryType
{
	BinUnknown,
	Lib,
	Dll,
	Exe,
	Sys
};

enum HeadersType
{
	MSVCHeaders,
	ReactOSHeaders
};

class MSVCConfiguration
{
	public:
		MSVCConfiguration(const OptimizationType optimization,
		                  const HeadersType headers = MSVCHeaders,
		                  const std::string &name = "");
		virtual ~MSVCConfiguration() {}
		std::string name;
		OptimizationType optimization;
		HeadersType headers;
};

class MSVCBackend : public Backend
{
	public:

		MSVCBackend(Project &project,
		              Configuration& configuration);
		virtual ~MSVCBackend() {}

		virtual void Process();

	private:

		void ProcessModules();
		void ProcessFile(std::string &filename);

		bool CheckFolderAdded(std::string &folder);
		void AddFolders(std::string &folder);

		void OutputFolders();
		void OutputFileUnits();

		std::string DspFileName ( const Module& module ) const;
		std::string VcprojFileName ( const Module& module ) const;
		std::string DswFileName ( const Module& module ) const;
		std::string SlnFileName ( const Module& module ) const;
		std::string OptFileName ( const Module& module ) const;
		std::string SuoFileName ( const Module& module ) const;
		std::string NcbFileName ( const Module& module ) const;

		std::vector<MSVCConfiguration*> m_configurations;

		std::vector<FileUnit> m_fileUnits;
		std::vector<std::string> m_folders;

		int m_unitCount;

		FILE* m_dswFile;
		FILE* m_slnFile;
		FILE* m_rulesFile;

		// functions in msvcmaker.cpp:

		void _generate_dsp ( const Module& module );
		void _generate_dsw_header ( FILE* OUT );
		void _generate_dsw_project (
			FILE* OUT,
			const Module& module,
			std::string dsp_file,
			const std::vector<Dependency*>& dependencies );

		void _generate_dsw_footer ( FILE* OUT );
		void _generate_wine_dsw ( FILE* OUT );

		// functions in vcprojmaker.cpp:

		std::string _strip_gcc_deffile(std::string Filename, std::string sourcedir, std::string objdir);
		std::string _get_solution_version ( void );
		std::string _get_studio_version ( void );
		std::string _gen_guid();
		std::string _replace_str(
			std::string string1,
			const std::string &find_str,
			const std::string &replace_str);

		std::string _get_vc_dir ( void ) const;

		// These are used in both _generate_vcproj and _generate_standard_configuration
		std::vector<std::string> header_files;
		std::vector<std::string> includes;
		std::vector<std::string> includes_ros;
		std::vector<std::string> libraries;
		std::set<std::string> common_defines;
		std::string baseaddr;

		void _generate_standard_configuration(
			FILE* OUT,
			const Module& module,
			const MSVCConfiguration& cfg,
			BinaryType binaryType );
		void _generate_makefile_configuration( FILE* OUT, const Module& module, const MSVCConfiguration& cfg );

		void _generate_vcproj ( const Module& module );
		void _generate_vcxproj ( const Module& module );

		void _generate_sln_header ( FILE* OUT );
		void _generate_sln_footer ( FILE* OUT );
		void _generate_sln ( FILE* OUT );
		//void _generate_rules_file ( FILE* OUT );
		void _generate_sln_project (
			FILE* OUT,
			const Module& module,
			std::string vcproj_file,
			std::string sln_guid,
			std::string vcproj_guid,
			const std::vector<Library*>& libraries );
		void _generate_sln_configurations (
			FILE* OUT,
			std::string vcproj_guid );
		void _clean_project_files ( void );
		void _get_object_files ( const Module& module, std::vector<std::string>& out ) const;
		void _get_def_files ( const Module& module, std::vector<std::string>& out ) const;
		void _install_files ( const std::string& vcdir, const std::string& config );
		bool _copy_file ( const std::string& inputname, const std::string& targetname ) const;
		const Property* _lookup_property ( const Module& module, const std::string& name ) const;
};

#endif // __MSVC_H__
