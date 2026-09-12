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

#include "world/editor_world_view_settings.hpp"

#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	editor_world_view_settings_reflection_t::editor_world_view_settings_reflection_t()
	{
		reflection_registry_t::get().register_type({
			.name		  = "editor_world_view_settings_t",
			.display_name = "World View Settings",
			.fields =
				{
					{.name				= "physics_ray_force",
					 .display_name		= "Physics Ray Force",
					 .offset			= offsetof(editor_world_view_settings_t, physics_ray_force),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 0.0f,
					 .max_clamp			= 100000.0f,
					 .clamp_granularity = 100.0f,
					 .type				= reflected_value_type_e::f32},
					{.name				= "grid_scale",
					 .display_name		= "Grid Scale",
					 .offset			= offsetof(editor_world_view_settings_t, grid_scale),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 0.01f,
					 .max_clamp			= 100.0f,
					 .clamp_granularity = 0.1f,
					 .type				= reflected_value_type_e::f32},
					{.name				= "snap_translate",
					 .display_name		= "Snap Translate",
					 .offset			= offsetof(editor_world_view_settings_t, snap_translate),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 0.0f,
					 .max_clamp			= 100.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::f32},
					{.name				= "snap_rotate",
					 .display_name		= "Snap Rotate",
					 .offset			= offsetof(editor_world_view_settings_t, snap_rotate),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 0.0f,
					 .max_clamp			= 100.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::f32},
					{.name				= "snap_scale",
					 .display_name		= "Snap Scale",
					 .offset			= offsetof(editor_world_view_settings_t, snap_scale),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 0.0f,
					 .max_clamp			= 100.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::f32},
				},
			.type_id   = type_id_t<editor_world_view_settings_t>::value,
			.size	   = sizeof(editor_world_view_settings_t),
			.alignment = alignof(editor_world_view_settings_t),
		});
	}
}
