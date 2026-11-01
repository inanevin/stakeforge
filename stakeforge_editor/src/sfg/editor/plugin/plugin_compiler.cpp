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
#include "editor/plugin/plugin_compiler.hpp"

#include <sfg/core/data/string_util.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/platform/process.hpp>
#include <sfg/core/serialization/serialization.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace sfg
{
	void from_json(const nlohmann::json& json, plugin_ide_session_t& session)
	{
		session.creation_time = json.value<u64>("creation_time", 0);
		session.process_id	  = json.value<u32>("process_id", 0);
	}

	bool plugin_compiler_t::open_project(const char* solution_path, plugin_ide_session_t& session, string_t& diagnostics)
	{
		session					  = {};
		const string_t executable = process::get_executable_path();

		if (executable.empty())
			return false;

		wchar_t	   system_directory[MAX_PATH] = {};
		const UINT length					  = GetSystemDirectoryW(system_directory, MAX_PATH);

		if (length == 0 || length >= MAX_PATH)
		{
			diagnostics = "Cannot locate Windows PowerShell, Windows error " + std::to_string(GetLastError());
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		const string_t script			 = file_system_t::get_directory_of_file(executable.c_str()) + "assets/packaging/visual_studio/open_project.ps1";
		const string_t absolute_solution = file_system_t::get_absolute_path(solution_path);

		if (!file_system_t::exists(script.c_str()))
		{
			diagnostics = "The SDK is missing Visual Studio automation: " + script;
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		const wstring_t wide_host = wstring_t(system_directory, length) + L"/WindowsPowerShell/v1.0/powershell.exe";
		string_t		host	  = {};
		const int		bytes	  = WideCharToMultiByte(CP_UTF8, 0, wide_host.c_str(), static_cast<int>(wide_host.size()), nullptr, 0, nullptr, nullptr);

		if (bytes == 0)
		{
			diagnostics = "Cannot convert the PowerShell path, Windows error " + std::to_string(GetLastError());
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		host.resize(static_cast<size_t>(bytes));

		if (WideCharToMultiByte(CP_UTF8, 0, wide_host.c_str(), static_cast<int>(wide_host.size()), host.data(), bytes, nullptr, nullptr) == 0)
		{
			diagnostics = "Cannot convert the PowerShell path, Windows error " + std::to_string(GetLastError());
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		const string_t command = "\"" + host + "\" -NoLogo -NoProfile -NonInteractive -STA -ExecutionPolicy Bypass -File \"" + script + "\" -SolutionPath \"" + absolute_solution + "\"";

		process_execute_result_t execution = process::execute(command.c_str());
		diagnostics						   = std::move(execution.output);
		const string_t log_path			   = file_system_t::get_directory_of_file(absolute_solution.c_str()) + "visual_studio.log";

		if (!serializer_t::write_to_file(diagnostics, log_path.c_str()))
			SFG_ERR("could not save Visual Studio diagnostics to {0}", log_path);

		if (!execution.started || execution.exit_code != 0)
		{
			SFG_ERR("could not open Visual Studio (exit {0}). Log: {1}\n{2}", execution.exit_code, log_path, diagnostics);
			return false;
		}

		const nlohmann::json response = nlohmann::json::parse(diagnostics, nullptr, false);

		if (response.is_discarded())
		{
			diagnostics = "Could not read the Visual Studio session. See " + log_path;
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		response.get_to(session);
		diagnostics = "Opened " + absolute_solution + " in Visual Studio process " + std::to_string(session.process_id);
		SFG_INFO("{0}", diagnostics);
		return true;
	}

	plugin_compile_result_t plugin_compiler_t::compile(const char* build_directory, const char* output_game_dll, const char* configuration, bool build_game_editor)
	{
		plugin_compile_result_t result	   = {};
		const string_t			executable = process::get_executable_path();

		if (executable.empty())
			return result;

		const string_t target = file_system_t::get_filename_from_path(output_game_dll) + (build_game_editor ? "_editor" : "");

		const string_t			 cmake	   = file_system_t::get_directory_of_file(executable.c_str()) + "assets/stakeforge_sdk/tools/cmake/bin/cmake.exe";
		const string_t			 command   = "\"" + cmake + "\" --build \"" + build_directory + "\" --config " + configuration + " --target \"" + target + "\"";
		process_execute_result_t execution = process::execute(command.c_str());
		result.exit_code				   = execution.exit_code;
		result.diagnostics				   = std::move(execution.output);
		result.success					   = execution.started && execution.exit_code == 0;

		const wstring_t			  output	 = string_util::to_wstr(output_game_dll);
		WIN32_FILE_ATTRIBUTE_DATA attributes = {};

		if (GetFileAttributesExW(output.c_str(), GetFileExInfoStandard, &attributes))
		{
			result.stamp.game = {
				.write_time = (static_cast<u64>(attributes.ftLastWriteTime.dwHighDateTime) << 32) | attributes.ftLastWriteTime.dwLowDateTime,
				.size		= (static_cast<u64>(attributes.nFileSizeHigh) << 32) | attributes.nFileSizeLow,
			};
		}
		else if (result.success)
		{
			result.success = false;
			result.diagnostics += "\nBuild succeeded but the expected game DLL is missing: ";
			result.diagnostics += output_game_dll;
		}

		if (result.success && build_game_editor)
		{
			const string_t	game_editor_path   = file_system_t::remove_extensions_from_path(output_game_dll) + "_editor.dll";
			const wstring_t game_editor_output = string_util::to_wstr(game_editor_path);

			if (GetFileAttributesExW(game_editor_output.c_str(), GetFileExInfoStandard, &attributes))
			{
				result.stamp.game_editor = {
					.write_time = (static_cast<u64>(attributes.ftLastWriteTime.dwHighDateTime) << 32) | attributes.ftLastWriteTime.dwLowDateTime,
					.size		= (static_cast<u64>(attributes.nFileSizeHigh) << 32) | attributes.nFileSizeLow,
				};
			}
			else
			{
				result.success = false;
				result.diagnostics += "\nBuild succeeded but the expected game editor DLL is missing: " + game_editor_path;
			}
		}

		const string_t log_path = string_t(build_directory) + "build.log";

		if (!serializer_t::write_to_file(command + "\n\n" + result.diagnostics, log_path.c_str()))
			SFG_ERR("could not write plugin build diagnostics to {0}", log_path);

		if (!result.success)
			SFG_ERR("C++ project build failed (exit {0}). Log: {1}\n{2}", result.exit_code, log_path, result.diagnostics);

		return result;
	}

	plugin_stage_result_t plugin_compiler_t::stage(const char* output_game_dll, const char* shadow_directory, const plugin_module_stamp_t& previous_stamp)
	{
		plugin_stage_result_t result	 = {};
		const string_t		  base		 = file_system_t::remove_extensions_from_path(output_game_dll);
		const string_t		  sources[]	 = {output_game_dll, base + "_editor.dll", base + ".pdb", base + "_editor.pdb", base + ".plugin_ready"};
		HANDLE				  files[]	 = {INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
		u64					  ready_time = 0;

		for (u32 index = 0; index < std::size(files); ++index)
		{
			const wstring_t source = string_util::to_wstr(sources[index]);
			files[index]		   = CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

			if (files[index] == INVALID_HANDLE_VALUE)
			{
				const DWORD error = GetLastError();

				if ((index == 2 || index == 3) && error == ERROR_FILE_NOT_FOUND)
					continue;

				result.state	   = error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION || error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? plugin_stage_state_e::retry : plugin_stage_state_e::failed;
				result.diagnostics = "Cannot read plugin build file " + sources[index] + ", Windows error " + std::to_string(error);
				break;
			}

			if (index < 2 || index == 4)
			{
				BY_HANDLE_FILE_INFORMATION info = {};

				if (!GetFileInformationByHandle(files[index], &info))
				{
					result.diagnostics = "Cannot inspect " + sources[index] + ", Windows error " + std::to_string(GetLastError());
					break;
				}

				const plugin_binary_stamp_t stamp{
					.write_time = (static_cast<u64>(info.ftLastWriteTime.dwHighDateTime) << 32) | info.ftLastWriteTime.dwLowDateTime,
					.size		= (static_cast<u64>(info.nFileSizeHigh) << 32) | info.nFileSizeLow,
				};

				if (index == 0)
					result.stamp.game = stamp;
				else if (index == 1)
					result.stamp.game_editor = stamp;
				else
					ready_time = stamp.write_time;
			}
		}

		if (result.diagnostics.empty() && (ready_time < result.stamp.game.write_time || ready_time < result.stamp.game_editor.write_time))
		{
			result.state	   = plugin_stage_state_e::retry;
			result.diagnostics = "Waiting for the runtime and game editor build to finish.";
		}

		if (result.diagnostics.empty() && result.stamp == previous_stamp)
			result.state = plugin_stage_state_e::unchanged;
		else if (result.diagnostics.empty())
		{
			static u64	   generation = 0;
			const string_t directory  = file_system_t::get_absolute_path(shadow_directory) + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()) + "-" + std::to_string(++generation) + "/";
			bool		   copied	  = file_system_t::ensure_directory(directory.c_str());

			if (!copied)
				result.diagnostics = "Cannot create plugin DLL copy directory: " + directory;

			for (u32 index = 0; copied && index < 4; ++index)
			{
				if (files[index] == INVALID_HANDLE_VALUE)
					continue;

				const string_t	destination		 = directory + file_system_t::get_filename_and_extension_from_path(sources[index]);
				const wstring_t source			 = string_util::to_wstr(sources[index]);
				const wstring_t wide_destination = string_util::to_wstr(destination);
				copied							 = CopyFileW(source.c_str(), wide_destination.c_str(), TRUE) != FALSE;

				if (!copied)
					result.diagnostics = "Cannot copy " + sources[index] + " to " + directory + ", Windows error " + std::to_string(GetLastError());

				if (index == 0)
					result.game_path = destination;
				else if (index == 1)
					result.game_editor_path = destination;
			}

			result.state = copied ? plugin_stage_state_e::ready : plugin_stage_state_e::failed;
		}

		for (HANDLE file : files)
		{
			if (file != INVALID_HANDLE_VALUE)
				CloseHandle(file);
		}

		return result;
	}
}
