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

#include "sprite.hpp"
#include <sfg/common/type_id.hpp>

namespace sfg
{
	class ostream_t;
	struct resource_header_t;

	struct sprite_cook_config_t
	{
		u16						   row_count		= 0;
		u16						   column_count		= 0;
		u16						   padding_x		= 0;
		u16						   padding_y		= 0;
		texture_ktx2_compression_e ktx2_compression = texture_ktx2_compression_e::faster;
		sprite_payload_type_e	   payload_type		= sprite_payload_type_e::ktx2_uastc;
	};

	class sprite_cooker final
	{
	public:
		sprite_cooker()								   = delete;
		~sprite_cooker()							   = delete;
		sprite_cooker(const sprite_cooker&)			   = delete;
		sprite_cooker& operator=(const sprite_cooker&) = delete;

		static bool cook_from_file(const sprite_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream);
	};

	SFG_DEFINE_TYPE_ID(sprite_cook_config_t);

	struct sprite_cook_reflection_t
	{
		sprite_cook_reflection_t();
	};

	inline sprite_cook_reflection_t g_reflect_sprite_cook;
}
