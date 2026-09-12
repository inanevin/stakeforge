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
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/render/render_resource_handle.hpp>
#include <cstddef>

namespace sfg
{
	class texture_loader_t
	{
	public:
		static constexpr u8	 MAX_MIPS	  = 16;
		static constexpr u32 WIRE_MAGIC	  = make_resource_wire_magic('T', 'E', 'X', 'R');
		static constexpr u32 WIRE_VERSION = 15;

		static bool load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset);
		static void unload(resource_entry_t& entry, resource_context_t& ctx);
	};

	struct texture_mip_header_t
	{
		u32		  byte_offset = 0;
		u32		  data_size	  = 0;
		u32		  row_pitch	  = 0;
		vec2u16_t size		  = vec2u16_t::zero;
		u8		  bpp		  = 0;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	struct texture_header_t
	{
		texture_mip_header_t	   mips[texture_loader_t::MAX_MIPS] = {};
		vec4f_t					   average_color					= vec4f_t::zero;
		format_e				   texture_format					= format_e::undefined;
		texture_payload_type_e	   payload_type						= texture_payload_type_e::ktx2_uastc;
		texture_ktx2_compression_e ktx2_compression					= texture_ktx2_compression_e::default_quality;
		vec2u16_t				   size								= vec2u16_t::zero;
		u8						   bpp								= 0;
		u8						   mip_count						= 0;
		u8						   is_linear						= 0;
		u8						   use_streaming					= 1;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	enum class texture_residency_e : u8
	{
		placeholder,
		streaming,
		resident,
		failed,
	};

	struct texture_runtime_t
	{
		texture_buffer_t	mips[texture_loader_t::MAX_MIPS] = {};
		texture_header_t	header							 = {};
		texture_residency_e residency						 = texture_residency_e::placeholder;
	};

	struct texture_internals_t
	{
		render_resource_handle_t texture = {};
		render_resource_handle_t staging = {};
	};

	extern const resource_type_desc_t texture_resource_desc;
}
