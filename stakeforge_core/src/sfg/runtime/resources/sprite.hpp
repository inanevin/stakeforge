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

#include "common_resources.hpp"
#include "texture_payload_type.hpp"
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>

namespace sfg
{
	enum class sprite_payload_type_e : u8
	{
		ktx2_uastc,
		png,
	};

	SFG_DEFINE_TYPE_ID(sprite_payload_type_e);

	class sprite_loader_t final
	{
	public:
		sprite_loader_t()								   = delete;
		~sprite_loader_t()								   = delete;
		sprite_loader_t(const sprite_loader_t&)			   = delete;
		sprite_loader_t& operator=(const sprite_loader_t&) = delete;

		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('S', 'P', 'R', 'T');
		static constexpr u32 WIRE_VERSION = 1;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct sprite_header_t
	{
		u32						   data_size		= 0;
		vec2u16_t				   size				= vec2u16_t::zero;
		vec2u16_t				   cell_size		= vec2u16_t::zero;
		vec2u16_t				   padding			= vec2u16_t::zero;
		u16						   row_count		= 1;
		u16						   column_count		= 1;
		sprite_payload_type_e	   payload_type		= sprite_payload_type_e::ktx2_uastc;
		texture_ktx2_compression_e ktx2_compression = texture_ktx2_compression_e::faster;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	struct sprite_runtime_t
	{
		sprite_header_t header	  = {};
		vec2f_t			uv_size	  = vec2f_t::zero;
		vec2f_t			uv_stride = vec2f_t::zero;
	};

	struct sprite_internals_t
	{
		render_resource_handle_t texture = {};
		render_resource_handle_t staging = {};
	};

	extern const resource_type_desc_t sprite_resource_desc;
}
