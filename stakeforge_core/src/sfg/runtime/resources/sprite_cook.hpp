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
