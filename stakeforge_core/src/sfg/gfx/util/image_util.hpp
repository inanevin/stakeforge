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
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	struct vec2u16_t;
	struct texture_buffer_t;
	class ostream_t;

	class image_util_t
	{
	public:
		enum class mip_gen_filter
		{
			def = 0,
			box,
			triangle,
			cubic_spline,
			catmullrom,
			mitchell,
		};

		static void* load_from_file_ch(const char* file, u8 force_channels);
		static void* load_from_file_ch(const char* file, vec2u16_t& out_size, u8 force_channels);
		static void* load_from_file(const char* file, u8& out_channels);
		static void* load_from_file(const char* file, vec2u16_t& out_size, u8& out_channels);
		static bool	 write_png(const texture_buffer_t& buffer, u8 channels, ostream_t& stream);
		static bool	 resize_rgba8(span_t<const u8> src, const vec2u16_t& src_size, span_t<u8> dst, const vec2u16_t& dst_size);
		static void	 generate_mips(texture_buffer_t* out_buffers, u8 target_levels, mip_gen_filter filter, u8 channels, bool is_linear, bool premultiplied_alpha);
		static u8	 calculate_mip_levels(u16 width, u16 height);
		static void	 free(void* data);
	};
}
