/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "script_compiler.hpp"

#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/platform/process.hpp>

namespace sfg
{
	bool script_compiler_t::publish_file(const char* staging_directory, const char* publish_directory, const char* project_name, const char* extension)
	{
		const string_t file_name	= string_t(project_name) + extension;
		const string_t staged_path	= string_t(staging_directory) + file_name;
		const string_t target_path	= string_t(publish_directory) + file_name;
		const string_t pending_path = target_path + ".new";

		if (!file_system_t::copy_file(staged_path.c_str(), pending_path.c_str()))
			return false;

		return file_system_t::replace_file(pending_path.c_str(), target_path.c_str());
	}

	script_compile_result_t script_compiler_t::compile(const char* project_path, script_build_configuration_e configuration, const char* publish_directory)
	{
		const char* configuration_name = configuration == script_build_configuration_e::release ? "Release" : "Debug";

		script_compile_result_t result					   = {};
		const string_t			project_name			   = file_system_t::get_filename_from_path(project_path);
		const string_t			project_directory		   = file_system_t::get_directory_of_file(project_path);
		string_t				staging_directory		   = project_directory + "Build/" + configuration_name + "/";
		string_t				resolved_publish_directory = publish_directory == nullptr ? "" : file_system_t::get_absolute_path(publish_directory);

		file_system_t::fix_path_end_slash(staging_directory);
		result.output_directory = staging_directory;

		if (!file_system_t::ensure_directory(staging_directory.c_str()))
		{
			result.diagnostics = "could not ensure the C# build staging directory.";
			return result;
		}

		if (!resolved_publish_directory.empty())
		{
			file_system_t::fix_path_end_slash(resolved_publish_directory);

			if (!file_system_t::ensure_directory(resolved_publish_directory.c_str()))
			{
				result.diagnostics = "could not ensure the C# script publish directory.";
				return result;
			}
		}

		string_t command_line = "dotnet build \"";
		command_line += project_path;
		command_line += "\" --nologo --configuration ";
		command_line += configuration_name;
		command_line += " --output \"";
		command_line += staging_directory;
		command_line += "\"";

		process_execute_result_t process_result = process::execute(command_line.c_str());
		result.exit_code						= process_result.exit_code;
		result.diagnostics						= std::move(process_result.output);

		if (!process_result.started || process_result.exit_code != 0)
			return result;

		if (resolved_publish_directory.empty())
		{
			SFG_INFO("compiled {0} script assembly: {1}", configuration_name, project_path);
			result.success = true;
			return result;
		}

		if (!publish_file(staging_directory.c_str(), resolved_publish_directory.c_str(), project_name.c_str(), ".deps.json"))
		{
			result.diagnostics += "\nCompilation succeeded, but the dependency file could not be published.";
			return result;
		}

		if (!publish_file(staging_directory.c_str(), resolved_publish_directory.c_str(), project_name.c_str(), ".pdb"))
		{
			result.diagnostics += "\nCompilation succeeded, but the debug symbols could not be published.";
			return result;
		}

		if (!publish_file(staging_directory.c_str(), resolved_publish_directory.c_str(), project_name.c_str(), ".dll"))
		{
			result.diagnostics += "\nCompilation succeeded, but the script assembly could not be published.";
			return result;
		}

		SFG_INFO("compiled {0} script assembly: {1}", configuration_name, project_path);
		result.success = true;
		return result;
	}
}
