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
#include <sfg/data/string_view.hpp>

namespace sfg
{
	class ostream_t;
	class istream_t;

	class serializer_t
	{
	public:
		static bool		 write_to_file(string_view_t file_input, const char* target_path);
		static bool		 save_to_file(const char* path, const ostream_t& stream);
		static bool		 save_to_file_atomic(const char* path, const ostream_t& stream);
		static bool		 save_to_file_compressed(const char* path, const ostream_t& stream);
		static istream_t load_from_file(const char* path);
		static istream_t load_from_file_slice(const char* path, u64 offset, u64 size);
		static istream_t load_from_file_compressed(const char* path);
	};

}
