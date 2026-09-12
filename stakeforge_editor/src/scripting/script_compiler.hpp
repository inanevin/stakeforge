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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	enum class script_build_configuration_e : u8
	{
		debug,
		release,
	};

	struct script_compile_result_t
	{
		string_t diagnostics;
		string_t output_directory;
		i32		 exit_code = -1;
		bool	 success   = false;
	};

	class script_compiler_t final
	{
	public:
		script_compiler_t()									   = delete;
		~script_compiler_t()								   = delete;
		script_compiler_t(const script_compiler_t&)			   = delete;
		script_compiler_t& operator=(const script_compiler_t&) = delete;

		static script_compile_result_t compile(const char* project_path, script_build_configuration_e configuration, const char* publish_directory = nullptr);

	private:
		static bool publish_file(const char* staging_directory, const char* publish_directory, const char* project_name, const char* extension);
	};
}
