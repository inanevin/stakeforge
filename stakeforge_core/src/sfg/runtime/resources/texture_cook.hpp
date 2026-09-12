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

#include <sfg/common/type_id.hpp>

#include "texture_payload_type.hpp"
#include <sfg/data/span.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	class ostream_t;
	struct resource_header_t;

	struct texture_cook_config_t
	{
		vec2u16_t				   size				= vec2u16_t::zero;
		texture_payload_type_e	   payload_type		= texture_payload_type_e::ktx2_uastc;
		texture_ktx2_compression_e ktx2_compression = texture_ktx2_compression_e::faster;
		bool					   generate_mipmaps = false;
		bool					   is_linear		= false;
		bool					   use_streaming	= true;
		bool					   force_4_channels = false;
	};

	class texture_cooker
	{
	public:
		static bool cook_from_file(const texture_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream);
		static bool cook_from_data(const texture_cook_config_t& cfg, span_t<u8> data, resource_header_t& out_header, ostream_t& stream);
	};

	SFG_DEFINE_TYPE_ID(texture_cook_config_t);

	struct texture_cook_config_reflection_t
	{
		texture_cook_config_reflection_t();
	};

	inline texture_cook_config_reflection_t g_reflect_texture_cook_config;
}
