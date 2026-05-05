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

#pragma once

#include <sfg/common/size_definitions.hpp>
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
		static void	 generate_mips(texture_buffer_t* out_buffers, u8 target_levels, mip_gen_filter filter, u8 channels, bool is_linear, bool premultiplied_alpha);
		static u8	 calculate_mip_levels(u16 width, u16 height);
		static void	 free(void* data);
	};
}
