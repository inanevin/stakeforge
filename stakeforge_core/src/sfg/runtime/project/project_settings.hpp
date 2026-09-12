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
