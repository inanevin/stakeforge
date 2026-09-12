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

#include "image_util.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/math.hpp>
#include <sfg/gfx/common/texture_buffer.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/memory/memory_tracer.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <sfg/vendor/stb/stb_image.h>
#include <sfg/vendor/stb/stb_image_write.h>
#include <sfg/vendor/stb/stb_image_resize.h>

namespace sfg
{
	namespace
	{
		void write_png_data(void* context, void* data, int size)
		{
			static_cast<ostream_t*>(context)->write_raw(static_cast<const u8*>(data), static_cast<size_t>(size));
		}
	}

	void* image_util_t::load_from_file_ch(const char* file, u8 force_channels)
	{
		int		 x = 0, y = 0, comp = 0;
		stbi_uc* data = stbi_load(file, &x, &y, &comp, static_cast<int>(force_channels));

		if (data == nullptr)
		{
			SFG_ERR("Failed loading image from file! {0}", file);
			return nullptr;
		}
		SFG_MEMTRACE_ALLOC(data, x * y * (force_channels == 0 ? comp : force_channels));

		return data;
	}

	void* image_util_t::load_from_file_ch(const char* file, vec2u16_t& out_size, u8 force_channels)
	{
		int		 x = 0, y = 0, comp = 0;
		stbi_uc* data = stbi_load(file, &x, &y, &comp, static_cast<int>(force_channels));

		if (data == nullptr)
		{
			SFG_ERR("Failed loading image from file! {0}", file);
			return nullptr;
		}

		out_size = vec2u16_t(static_cast<u16>(x), static_cast<u16>(y));
		SFG_MEMTRACE_ALLOC(data, x * y * (force_channels == 0 ? comp : force_channels));

		return data;
	}

	void* image_util_t::load_from_file(const char* file, u8& out_channels)
	{
		int		 x = 0, y = 0, comp = 0;
		stbi_uc* data = stbi_load(file, &x, &y, &comp, 0);

		if (data == nullptr)
		{
			SFG_ERR("Failed loading image from file! {0}", file);
			return nullptr;
		}

		out_channels = static_cast<int>(comp);
		SFG_MEMTRACE_ALLOC(data, x * y * comp);
		return data;
	}

	void* image_util_t::load_from_file(const char* file, vec2u16_t& out_size, u8& out_channels)
	{
		int		 x = 0, y = 0, comp = 0;
		stbi_uc* data = stbi_load(file, &x, &y, &comp, 0);

		if (data == nullptr)
		{
			SFG_ERR("Failed loading image from file! {0}", file);
			return nullptr;
		}

		out_channels = static_cast<int>(comp);
		out_size	 = vec2u16_t(static_cast<u16>(x), static_cast<u16>(y));
		SFG_MEMTRACE_ALLOC(data, x * y * comp);
		return data;
	}

	void image_util_t::generate_mips(texture_buffer_t* out_buffers, u8 target_levels, mip_gen_filter filter, u8 channels, bool is_linear, bool premultiplied_alpha)
	{
		const texture_buffer_t& buf			= out_buffers[0];
		u8*						last_pixels = buf.pixels;
		u16						last_w		= buf.size.x;
		u16						last_h		= buf.size.y;

		for (u8 i = 0; i < target_levels - 1; i++)
		{
			u16 w = last_w / 2;
			u16 h = last_h / 2;

			if (w < 1)
				w = 1;

			if (h < 1)
				h = 1;

			texture_buffer_t mip	  = {};
			mip.size				  = vec2u16_t(w, h);
			mip.pixels				  = (u8*)SFG_MALLOC(w * h * buf.bpp);
			mip.bpp					  = buf.bpp;
			mip.row_pitch			  = static_cast<u32>(w) * static_cast<u32>(mip.bpp);
			mip.data_size			  = mip.row_pitch * h;
			const stbir_colorspace cs = is_linear ? stbir_colorspace::STBIR_COLORSPACE_LINEAR : stbir_colorspace::STBIR_COLORSPACE_SRGB;

			int ret = 0;

			const i32 alpha_ch = channels == 4 ? 3 : STBIR_ALPHA_CHANNEL_NONE;

			u32 flags = premultiplied_alpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0;

			if (mip.bpp <= 4)
				ret = stbir_resize_uint8_generic(last_pixels, last_w, last_h, 0, mip.pixels, w, h, 0, channels, alpha_ch, flags, stbir_edge::STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter), cs, 0);
			else
				ret = stbir_resize_uint16_generic((u16*)last_pixels, last_w, last_h, 0, (u16*)mip.pixels, w, h, 0, channels, alpha_ch, flags, stbir_edge::STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter), cs, 0);

			last_w			   = w;
			last_h			   = h;
			last_pixels		   = mip.pixels;
			out_buffers[i + 1] = mip;
		}
	}

	bool image_util_t::resize_rgba8(span_t<const u8> src, const vec2u16_t& src_size, span_t<u8> dst, const vec2u16_t& dst_size)
	{
		SFG_ASSERT(src.data != nullptr);
		SFG_ASSERT(dst.data != nullptr);
		SFG_ASSERT(src_size.x != 0);
		SFG_ASSERT(src_size.y != 0);
		SFG_ASSERT(dst_size.x != 0);
		SFG_ASSERT(dst_size.y != 0);
		SFG_ASSERT(src.size == static_cast<size_t>(src_size.x) * static_cast<size_t>(src_size.y) * 4);
		SFG_ASSERT(dst.size == static_cast<size_t>(dst_size.x) * static_cast<size_t>(dst_size.y) * 4);

		const int result = stbir_resize_uint8_generic(src.data, src_size.x, src_size.y, 0, dst.data, dst_size.x, dst_size.y, 0, 4, 3, 0, stbir_edge::STBIR_EDGE_CLAMP, stbir_filter::STBIR_FILTER_DEFAULT, stbir_colorspace::STBIR_COLORSPACE_SRGB, nullptr);
		return result != 0;
	}

	bool image_util_t::write_png(const texture_buffer_t& buffer, u8 channels, ostream_t& stream)
	{
		return stbi_write_png_to_func(write_png_data, &stream, buffer.size.x, buffer.size.y, channels, buffer.pixels, buffer.row_pitch) != 0 && stream.get_size() != 0;
	}

	u8 image_util_t::calculate_mip_levels(u16 width, u16 height)
	{
		return static_cast<u8>(math::floor_log2(math::max(width, height))) + 1;
	}

	void image_util_t::free(void* data)
	{
		SFG_MEMTRACE_DEALLOC(data);
		::STBI_FREE(data);
	}

}
