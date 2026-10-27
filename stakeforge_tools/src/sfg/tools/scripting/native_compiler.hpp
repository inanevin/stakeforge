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

#include "tools/tools_exports.hpp"

#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/data/string.hpp>

namespace sfg
{
	struct native_binary_stamp_t
	{
		u64 write_time = 0;
		u64 size	   = 0;

		bool operator==(const native_binary_stamp_t& other) const = default;
	};

	enum class native_stage_state_e : u8
	{
		failed,
		retry,
		unchanged,
		ready,
	};

	struct native_compile_result_t
	{
		string_t			  diagnostics;
		native_binary_stamp_t stamp		= {};
		i32					  exit_code = -1;
		bool				  success	= false;
	};

	struct native_stage_result_t
	{
		string_t			  path;
		string_t			  diagnostics;
		native_binary_stamp_t stamp = {};
		native_stage_state_e  state = native_stage_state_e::failed;
	};

	struct native_ide_session_t
	{
		u64 creation_time = 0;
		u32 process_id	  = 0;
	};

	class SFG_TOOLS_API native_compiler_t final
	{
	public:
		native_compiler_t()											 = delete;
		~native_compiler_t()										 = delete;
		native_compiler_t(const native_compiler_t& other)			 = delete;
		native_compiler_t& operator=(const native_compiler_t& other) = delete;

		static native_compile_result_t compile(const char* build_directory, const char* output_dll, const char* configuration);
		static native_stage_result_t   stage(const char* output_dll, const char* shadow_directory, const native_binary_stamp_t& previous_stamp);
		static bool					   open_project(const char* solution_path, native_ide_session_t& session, string_t& diagnostics);
	};
}
