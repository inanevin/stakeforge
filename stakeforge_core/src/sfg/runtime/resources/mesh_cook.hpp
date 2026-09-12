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

namespace sfg
{
	class istream_t;
	class ostream_t;
	struct mesh_def_t;
	struct resource_header_t;

	class mesh_cooker
	{
	public:
		static bool serialize_def_blob(const mesh_def_t& def, ostream_t& stream);
		static bool deserialize_def_blob(istream_t& stream, mesh_def_t& out);
		static bool cook_from_file(const char* full_path, resource_header_t& out_header, ostream_t& stream);
		static bool cook_from_def(const mesh_def_t& def, resource_header_t& out_header, ostream_t& stream, bool compress = true);
	};
}
