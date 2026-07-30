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
#include <sfg/common/type_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/physics/physics_project_settings.hpp>

namespace sfg
{
#define ENGINE_SHADOW_VIEW_MAX 64

	enum class engine_quality_level_e : u8
	{
		low,
		medium,
		high,
		ultra,
	};

	struct engine_shadow_settings_t
	{
		f32 shadow_distance		 = 200.0f;
		f32 shadow_fade_distance = 30.0f;
		u32 texel_budget		 = 16u * 1024u * 1024u;
		u16 min_resolution		 = 256;
		u16 max_resolution		 = 2048;
		u16 max_views			 = 48;

		bool operator==(const engine_shadow_settings_t&) const = default;
	};

	struct project_settings_t
	{
		physics_project_settings_t physics			  = {};
		vector_t<string_t>		   tags				  = {};
		engine_shadow_settings_t   shadows			  = {};
		u32						   world_tick_rate	  = 60;
		u32						   world_physics_rate = 100;
		u32						   max_sim_steps	  = 4;
		f32						   ui_scale			  = 1.0f;
		engine_quality_level_e	   quality_level	  = engine_quality_level_e::high;

		void normalize(const project_settings_t* previous = nullptr);

		bool operator==(const project_settings_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(engine_quality_level_e);
	SFG_DEFINE_TYPE_ID(engine_shadow_settings_t);
	SFG_DEFINE_TYPE_ID(project_settings_t);

	struct project_settings_reflection_t
	{
		project_settings_reflection_t();
	};

	inline project_settings_reflection_t g_reflect_project_settings;
}
