/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "image_util.hpp"
#include "io/log.hpp"
#include "math/vector2ui16.hpp"
#include "math/math.hpp"
#include "gfx/common/texture_buffer.hpp"
#include "memory/memory.hpp"
#include "memory/memory_tracer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "vendor/stb/stb_image.h"
#include "vendor/stb/stb_image_write.h"
#include "vendor/stb/stb_image_resize.h"

namespace SFG
{
	void* image_util::load_from_file_ch(const char* file, u8 force_channels)
	{
		int		 x = 0, y = 0, comp = 0;
		stbi_uc* data = stbi_load(file, &x, &y, &comp, static_cast<int>(force_channels));

		if (data == nullptr)
		{
			SFG_ERR("Failed loading image from file! {0}", file);
			return nullptr;
		}

		return data;
	}

	void* image_util::load_from_file_ch(const char* file, vector2ui16& out_size, u8 force_channels)
	{
		int		 x = 0, y = 0, comp = 0;
		stbi_uc* data = stbi_load(file, &x, &y, &comp, static_cast<int>(force_channels));

		if (data == nullptr)
		{
			SFG_ERR("Failed loading image from file! {0}", file);
			return nullptr;
		}

		out_size = vector2ui16(static_cast<u16>(x), static_cast<u16>(y));

		return data;
	}

	void* image_util::load_from_file(const char* file, u8& out_channels)
	{
		int		 x = 0, y = 0, comp = 0;
		stbi_uc* data = stbi_load(file, &x, &y, &comp, 0);

		if (data == nullptr)
		{
			SFG_ERR("Failed loading image from file! {0}", file);
			return nullptr;
		}

		out_channels = static_cast<int>(comp);

		return data;
	}

	void* image_util::load_from_file(const char* file, vector2ui16& out_size, u8& out_channels)
	{
		int		 x = 0, y = 0, comp = 0;
		stbi_uc* data = stbi_load(file, &x, &y, &comp, 0);

		if (data == nullptr)
		{
			SFG_ERR("Failed loading image from file! {0}", file);
			return nullptr;
		}

		out_channels = static_cast<int>(comp);
		out_size	 = vector2ui16(static_cast<u16>(x), static_cast<u16>(y));

		return data;
	}

	void image_util::compress_to_buffer(void* data, size_t sz, ostream& stream)
	{
		//     int stbi_write_png_to_func(stbi_write_func *func, void *context, int w, int h, int comp, const void  *data, int stride_in_bytes);
	}

	void image_util::generate_mips(texture_buffer* out_buffers, u8 target_levels, mip_gen_filter filter, u8 channels, bool is_linear, bool premultiplied_alpha)
	{
		const texture_buffer& buf		  = out_buffers[0];
		u8*					  last_pixels = buf.pixels;
		u16					  last_w	  = buf.size.x;
		u16					  last_h	  = buf.size.y;

		for (u8 i = 0; i < target_levels - 1; i++)
		{
			u16 w = last_w / 2;
			u16 h = last_h / 2;

			if (w < 1)
				w = 1;

			if (h < 1)
				h = 1;

			texture_buffer mip = {};
			mip.size		   = vector2ui16(w, h);
			mip.pixels		   = (u8*)SFG_MALLOC(w * h * buf.bpp);
			mip.bpp			   = buf.bpp;
			PUSH_ALLOCATION_SZ(w * h * mip.bpp);
			const stbir_colorspace cs = is_linear ? stbir_colorspace::STBIR_COLORSPACE_LINEAR : stbir_colorspace::STBIR_COLORSPACE_SRGB;

			int ret = 0;

			const u32 alpha_ch = mip.bpp == 1 ? 0 : 3;

			u32 flags = premultiplied_alpha ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0;

			if (mip.bpp == 4 || mip.bpp == 1)
				ret = stbir_resize_uint8_generic(last_pixels, last_w, last_h, 0, mip.pixels, w, h, 0, channels, alpha_ch, flags, stbir_edge::STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter), cs, 0);
			else
				ret = stbir_resize_uint16_generic((u16*)last_pixels, last_w, last_h, 0, (u16*)mip.pixels, w, h, 0, channels, alpha_ch, flags, stbir_edge::STBIR_EDGE_CLAMP, static_cast<stbir_filter>(filter), cs, 0);

			last_w			   = w;
			last_h			   = h;
			last_pixels		   = mip.pixels;
			out_buffers[i + 1] = mip;
		}
	}

	u8 image_util::calculate_mip_levels(u16 width, u16 height)
	{
		return static_cast<u8>(math::floor_log2(math::max(width, height))) + 1;
	}

	void image_util::free(void* data)
	{
		::STBI_FREE(data);
	}

}
