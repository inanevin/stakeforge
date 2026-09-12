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

#include "texture_payload_type.hpp"
#include <sfg/data/span.hpp>
#include <sfg/gfx/common/format.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	class ostream_t;

	struct ktx2_image_desc_t
	{
		vec2u16_t size		= vec2u16_t::zero;
		format_e  format	= format_e::undefined;
		u8		  mip_count = 0;
	};

	class ktx2_util_t final
	{
	public:
		ktx2_util_t()							   = delete;
		~ktx2_util_t()							   = delete;
		ktx2_util_t(const ktx2_util_t&)			   = delete;
		ktx2_util_t& operator=(const ktx2_util_t&) = delete;

		static bool encode_uastc(span_t<const texture_buffer_t> mips, bool is_linear, texture_ktx2_compression_e compression, const char* source_name, ostream_t& stream);
		static bool decode_uastc(span_t<const u8> data, texture_ktx2_compression_e compression, u64 resource_hash, texture_buffer_t* out_mips, u8 max_mips, ktx2_image_desc_t& out_desc);

	private:
		static void release(texture_buffer_t* mips, u8 mip_count);
	};
}
