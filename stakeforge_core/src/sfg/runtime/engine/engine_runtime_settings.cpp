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

#include "engine_runtime_settings.hpp"
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	engine_runtime_settings_reflection_t::engine_runtime_settings_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		registry.register_type({
			.name		  = "engine_quality_level_e",
			.display_name = "Quality Level",
			.fields		  = {{.name = "low", .display_name = "Low"}, {.name = "medium", .display_name = "Medium"}, {.name = "high", .display_name = "High"}, {.name = "ultra", .display_name = "Ultra"}},
			.type_id	  = type_id_t<engine_quality_level_e>::value,
			.size		  = sizeof(engine_quality_level_e),
			.alignment	  = alignof(engine_quality_level_e),
			.flags		  = reflected_type_flag_enum,
		});
		registry.register_type({
			.name		  = "engine_shadow_settings_t",
			.display_name = "Shadows",
			.fields =
				{
					{.name				= "shadow_distance",
					 .display_name		= "Shadow Distance",
					 .tooltip			= "Maximum camera distance covered by realtime shadows.",
					 .offset			= offsetof(engine_shadow_settings_t, shadow_distance),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 1.0f,
					 .max_clamp			= 2000.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::f32},
					{.name				= "shadow_fade_distance",
					 .display_name		= "Fade Distance",
					 .tooltip			= "Distance over which realtime shadows fade out.",
					 .offset			= offsetof(engine_shadow_settings_t, shadow_fade_distance),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 0.0f,
					 .max_clamp			= 500.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::f32},
					{.name		   = "texel_budget",
					 .display_name = "Texel Budget",
					 .tooltip	   = "Maximum total texels allocated to visible realtime shadow views.",
					 .offset	   = offsetof(engine_shadow_settings_t, texel_budget),
					 .size		   = sizeof(u32),
					 .type		   = reflected_value_type_e::u32},
					{.name = "min_resolution", .display_name = "Minimum Resolution", .offset = offsetof(engine_shadow_settings_t, min_resolution), .size = sizeof(u16), .type = reflected_value_type_e::u16},
					{.name = "max_resolution", .display_name = "Maximum Resolution", .offset = offsetof(engine_shadow_settings_t, max_resolution), .size = sizeof(u16), .type = reflected_value_type_e::u16},
					{.name		   = "max_views",
					 .display_name = "Maximum Views",
					 .tooltip	   = "Maximum directional cascades, spot maps, and point-light cube faces rendered per world view.",
					 .offset	   = offsetof(engine_shadow_settings_t, max_views),
					 .size		   = sizeof(u16),
					 .type		   = reflected_value_type_e::u16},
				},
			.type_id   = type_id_t<engine_shadow_settings_t>::value,
			.size	   = sizeof(engine_shadow_settings_t),
			.alignment = alignof(engine_shadow_settings_t),
		});
		registry.register_type({
			.name		  = "engine_runtime_settings_t",
			.display_name = "Runtime",
			.fields =
				{
					{.name		   = "shadows",
					 .display_name = "Shadows",
					 .sub_type_id  = type_id_t<engine_shadow_settings_t>::value,
					 .offset	   = offsetof(engine_runtime_settings_t, shadows),
					 .size		   = sizeof(engine_shadow_settings_t),
					 .type		   = reflected_value_type_e::object},
					{.name				= "world_tick_rate",
					 .display_name		= "World Tick Rate",
					 .offset			= offsetof(engine_runtime_settings_t, world_tick_rate),
					 .size				= sizeof(u32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 15.0f,
					 .max_clamp			= 240.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::u32},
					{.name				= "world_physics_rate",
					 .display_name		= "World Physics Rate",
					 .offset			= offsetof(engine_runtime_settings_t, world_physics_rate),
					 .size				= sizeof(u32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 30.0f,
					 .max_clamp			= 240.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::u32},
					{.name				= "max_sim_steps",
					 .display_name		= "Max Sim Steps",
					 .offset			= offsetof(engine_runtime_settings_t, max_sim_steps),
					 .size				= sizeof(u32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 1.0f,
					 .max_clamp			= 8.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::u32},
					{.name		   = "quality_level",
					 .display_name = "Quality Level",
					 .sub_type_id  = type_id_t<engine_quality_level_e>::value,
					 .offset	   = offsetof(engine_runtime_settings_t, quality_level),
					 .size		   = sizeof(engine_quality_level_e),
					 .type		   = reflected_value_type_e::u8},
				},
			.type_id   = type_id_t<engine_runtime_settings_t>::value,
			.size	   = sizeof(engine_runtime_settings_t),
			.alignment = alignof(engine_runtime_settings_t),
		});
	}
}
