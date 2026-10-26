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
#include "tools/scripting/native_compiler.hpp"

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
	void from_json(const nlohmann::json& json, native_ide_session_t& session)
	{
		session.creation_time = json.value<u64>("creation_time", 0);
		session.process_id	  = json.value<u32>("process_id", 0);
	}

	bool native_compiler_t::open_project(const char* solution_path, native_ide_session_t& session, string_t& diagnostics)
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

		const string_t script			 = file_system_t::get_directory_of_file(executable.c_str()) + "stakeforge_sdk/tools/visual_studio/open_project.ps1";
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

	native_compile_result_t native_compiler_t::compile(const char* build_directory, const char* output_dll)
	{
		native_compile_result_t result	   = {};
		const string_t			executable = process::get_executable_path();

		if (executable.empty())
			return result;

		const string_t			 cmake	   = file_system_t::get_directory_of_file(executable.c_str()) + "stakeforge_sdk/tools/cmake/bin/cmake.exe";
		const string_t			 command   = "\"" + cmake + "\" --build \"" + build_directory + "\" --config " + SFG_CONFIGURATION + " --target stakeforge_game";
		process_execute_result_t execution = process::execute(command.c_str());
		result.exit_code				   = execution.exit_code;
		result.diagnostics				   = std::move(execution.output);
		result.success					   = execution.started && execution.exit_code == 0;

		const wstring_t			  output	 = string_util::to_wstr(output_dll);
		WIN32_FILE_ATTRIBUTE_DATA attributes = {};

		if (GetFileAttributesExW(output.c_str(), GetFileExInfoStandard, &attributes))
		{
			result.stamp = {
				.write_time = (static_cast<u64>(attributes.ftLastWriteTime.dwHighDateTime) << 32) | attributes.ftLastWriteTime.dwLowDateTime,
				.size		= (static_cast<u64>(attributes.nFileSizeHigh) << 32) | attributes.nFileSizeLow,
			};
		}
		else if (result.success)
		{
			result.success = false;
			result.diagnostics += "\nBuild succeeded but the expected game DLL is missing: ";
			result.diagnostics += output_dll;
		}

		const string_t log_path = string_t(build_directory) + "build.log";

		if (!serializer_t::write_to_file(command + "\n\n" + result.diagnostics, log_path.c_str()))
			SFG_ERR("could not write native build diagnostics to {0}", log_path);

		if (!result.success)
			SFG_ERR("C++ project build failed (exit {0}). Log: {1}\n{2}", result.exit_code, log_path, result.diagnostics);

		return result;
	}

	native_stage_result_t native_compiler_t::stage(const char* output_dll, const char* shadow_directory, const native_binary_stamp_t& previous_stamp)
	{
		native_stage_result_t result	  = {};
		const wstring_t		  source	  = string_util::to_wstr(output_dll);
		const HANDLE		  source_file = CreateFileW(source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (source_file == INVALID_HANDLE_VALUE)
		{
			const DWORD error  = GetLastError();
			result.state	   = error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION || error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? native_stage_state_e::retry : native_stage_state_e::failed;
			result.diagnostics = "Cannot read game DLL " + string_t(output_dll) + ", Windows error " + std::to_string(error);
			return result;
		}

		BY_HANDLE_FILE_INFORMATION info = {};

		if (!GetFileInformationByHandle(source_file, &info))
		{
			result.diagnostics = "Cannot inspect game DLL, Windows error " + std::to_string(GetLastError());
			CloseHandle(source_file);
			return result;
		}

		result.stamp = {
			.write_time = (static_cast<u64>(info.ftLastWriteTime.dwHighDateTime) << 32) | info.ftLastWriteTime.dwLowDateTime,
			.size		= (static_cast<u64>(info.nFileSizeHigh) << 32) | info.nFileSizeLow,
		};

		if (result.stamp == previous_stamp)
		{
			CloseHandle(source_file);
			result.state = native_stage_state_e::unchanged;
			return result;
		}

		const string_t	pdb_source		= file_system_t::get_directory_of_file(output_dll) + "stakeforge_game.pdb";
		const wstring_t wide_pdb_source = string_util::to_wstr(pdb_source);
		const HANDLE	pdb_file		= CreateFileW(wide_pdb_source.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (pdb_file == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND)
		{
			result.state	   = native_stage_state_e::retry;
			result.diagnostics = "Cannot read game PDB, Windows error " + std::to_string(GetLastError());
			CloseHandle(source_file);
			return result;
		}

		static u64	   generation		= 0;
		const string_t directory		= file_system_t::get_absolute_path(shadow_directory) + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()) + "-" + std::to_string(++generation) + "/";
		result.path						= directory + "stakeforge_game.dll";
		const wstring_t destination		= string_util::to_wstr(result.path);
		const wstring_t pdb_destination = string_util::to_wstr(directory + "stakeforge_game.pdb");
		bool			copied			= file_system_t::ensure_directory(directory.c_str());
		DWORD			copy_error		= ERROR_SUCCESS;

		if (copied && !CopyFileW(source.c_str(), destination.c_str(), TRUE))
		{
			copy_error = GetLastError();
			copied	   = false;
		}

		if (copied && pdb_file != INVALID_HANDLE_VALUE && !CopyFileW(wide_pdb_source.c_str(), pdb_destination.c_str(), TRUE))
		{
			copy_error = GetLastError();
			copied	   = false;
		}

		if (!copied)
			result.diagnostics = copy_error == ERROR_SUCCESS ? "Cannot create native DLL copy directory: " + directory : "Cannot copy game DLL/PDB to " + directory + ", Windows error " + std::to_string(copy_error);

		if (pdb_file != INVALID_HANDLE_VALUE)
			CloseHandle(pdb_file);

		CloseHandle(source_file);
		result.state = copied ? native_stage_state_e::ready : native_stage_state_e::failed;
		return result;
	}

	bool native_compiler_t::load(const char* path, native_module_t& module, string_t& diagnostics)
	{
		SFG_ASSERT(module.handle == nullptr);

		const wstring_t absolute_path = string_util::to_wstr(file_system_t::get_absolute_path(path));
		const HMODULE	handle		  = LoadLibraryExW(absolute_path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

		if (handle == nullptr)
		{
			diagnostics = "Cannot load game DLL " + string_t(path) + ", Windows error " + std::to_string(GetLastError());
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		module.path	  = path;
		module.handle = handle;
		SFG_INFO("loaded native game DLL: {0}", module.path);
		return true;
	}

	bool native_compiler_t::unload(native_module_t& module, string_t& diagnostics)
	{
		SFG_ASSERT(module.handle != nullptr);

		if (!FreeLibrary(static_cast<HMODULE>(module.handle)))
		{
			diagnostics = "Cannot unload game DLL " + module.path + ", Windows error " + std::to_string(GetLastError());
			SFG_ERR("{0}", diagnostics);
			return false;
		}

		SFG_INFO("unloaded native game DLL: {0}", module.path);
		module.handle = nullptr;
		module.path.resize(0);
		return true;
	}
}
