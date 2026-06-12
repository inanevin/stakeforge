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

#include <sfg/common/type_id.hpp>

#include "skybox_hdr.hpp"

namespace sfg
{
	class ostream_t;
	struct resource_header_t;

	struct skybox_hdr_cook_config_t
	{
		vec2u16_t radiance_size			  = {512, 512};
		vec2u16_t irradiance_size		  = {32, 32};
		vec2u16_t prefilter_size		  = {128, 128};
		vec2u16_t brdf_lut_size			  = {256, 256};
		u32		  irradiance_sample_count = 512;
		u32		  prefilter_sample_count  = 128;
		u32		  brdf_lut_sample_count	  = 512;
		f32		  intensity				  = 1.0f;
		f32		  rotation				  = 0.0f;
		u8		  prefilter_mips		  = 8;
	};

	class skybox_hdr_cooker
	{
	public:
		static bool cook_from_file(const skybox_hdr_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream);
	};

	SFG_DEFINE_TYPE_ID(skybox_hdr_cook_config_t);

	struct skybox_hdr_cook_config_reflection_t
	{
		skybox_hdr_cook_config_reflection_t();
	};

	inline skybox_hdr_cook_config_reflection_t g_reflect_skybox_hdr_cook_config;
}
