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
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	enum class world_render_reflection_probe_capture_type_e : u8
	{
		skybox,
		scene,
	};

	enum class world_render_reflection_probe_capture_mode_e : u8
	{
		once,
		realtime,
		manual,
	};

	struct world_render_reflection_probe_t
	{
		quat_t										 prev_rot				= quat_t::identity;
		quat_t										 rot					= quat_t::identity;
		vec3f_t										 prev_pos				= vec3f_t::zero;
		f32											 diffuse_intensity		= 1.0f;
		vec3f_t										 pos					= vec3f_t::zero;
		f32											 specular_intensity		= 1.0f;
		vec3f_t										 prev_scale				= vec3f_t::one;
		f32											 blend_distance			= 1.0f;
		vec3f_t										 scale					= vec3f_t::one;
		f32											 resolution				= 256.0f;
		vec3f_t										 extents				= {5.0f, 5.0f, 5.0f};
		u32											 stable_id				= UINT32_MAX;
		u32											 realtime_tick_interval = 30;
		world_render_reflection_probe_capture_type_e capture_type			= world_render_reflection_probe_capture_type_e::scene;
		world_render_reflection_probe_capture_mode_e capture_mode			= world_render_reflection_probe_capture_mode_e::once;
		u8											 is_global				= 0;
	};
}
