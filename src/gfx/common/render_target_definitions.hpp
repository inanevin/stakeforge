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

#include "format.hpp"

namespace SFG
{

	class render_target_definitions
	{
	public:
		static inline format get_format_gbuffer_albedo()
		{
			return format::r8g8b8a8_srgb;
		}

		static inline format get_format_gbuffer_normal()
		{
			return format::r10g0b10a2_unorm;
		}

		static inline format get_format_gbuffer_orm()
		{
			return format::r8g8b8a8_unorm;
		}

		static inline format get_format_lighting()
		{
			return format::r16g16b16a16_sfloat;
		}

		static inline format get_format_post_combine()
		{
			return format::r16g16b16a16_sfloat;
		}

		static inline format get_format_depth_default()
		{
			return format::d32_sfloat;
		}

		static inline format get_format_depth_default_read()
		{
			return format::r32_sfloat;
		}

		static inline format get_format_gbuffer_emissive()
		{
			return format::r16g16b16a16_sfloat;
		}

		static inline format get_format_swapchain()
		{
			return format::r8g8b8a8_srgb;
		}

		static inline format get_format_ssao_ao_out()
		{
			return format::r8_unorm;
		}

		static inline format get_format_object_id()
		{
			return format::r32_uint;
		}

		static inline format get_format_selection()
		{
			return format::r8g8b8a8_srgb;
		}

		static inline format get_format_shadows()
		{
			return format::d32_sfloat;
		}

		static inline format get_format_editor()
		{
			return format::r8g8b8a8_srgb;
		}
	};
}