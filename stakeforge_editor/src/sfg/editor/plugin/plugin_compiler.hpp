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
#pragma once

#include "editor/editor_exports.hpp"

#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/data/string.hpp>

namespace sfg
{
	struct plugin_binary_stamp_t
	{
		u64 write_time = 0;
		u64 size	   = 0;

		bool operator==(const plugin_binary_stamp_t& other) const = default;
	};

	struct plugin_module_stamp_t
	{
		plugin_binary_stamp_t game		  = {};
		plugin_binary_stamp_t game_editor = {};

		bool operator==(const plugin_module_stamp_t& other) const = default;
	};

	enum class plugin_stage_state_e : u8
	{
		failed,
		retry,
		unchanged,
		ready,
	};

	struct plugin_compile_result_t
	{
		string_t			  diagnostics;
		plugin_module_stamp_t stamp		= {};
		i32					  exit_code = -1;
		bool				  success	= false;
	};

	struct plugin_stage_result_t
	{
		string_t			  game_editor_path = {};
		string_t			  game_path;
		string_t			  diagnostics;
		plugin_module_stamp_t stamp = {};
		plugin_stage_state_e  state = plugin_stage_state_e::failed;
	};

	struct plugin_ide_session_t
	{
		u64 creation_time = 0;
		u32 process_id	  = 0;
	};

	class SFG_EDITOR_API plugin_compiler_t final
	{
	public:
		plugin_compiler_t()											 = delete;
		~plugin_compiler_t()										 = delete;
		plugin_compiler_t(const plugin_compiler_t& other)			 = delete;
		plugin_compiler_t& operator=(const plugin_compiler_t& other) = delete;

		static plugin_compile_result_t compile(const char* build_directory, const char* output_game_dll, const char* configuration, bool build_game_editor = true);
		static plugin_stage_result_t   stage(const char* output_game_dll, const char* shadow_directory, const plugin_module_stamp_t& previous_stamp);
		static bool					   open_project(const char* solution_path, plugin_ide_session_t& session, string_t& diagnostics);
	};
}
