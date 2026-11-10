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

#include "world_render_config.hpp"
#include "core/runtime/project/project_settings.hpp"
#include "core/runtime/resources/common_resources.hpp"

namespace sfg
{
	world_render_context_config_t world_render_context_config_t::make_main(vec2u16_t render_resolution)
	{
		return {
			.size				  = render_resolution,
			.entity_max			  = 1024 * 10,
			.mesh_draw_max		  = 32768,
			.material_max		  = 4096,
			.sprite_max			  = 256,
			.particle_max		  = 8192,
			.bone_max			  = 4096,
			.light_max			  = 1024,
			.reflection_probe_max = 256,
			.shadow_view_max	  = ENGINE_SHADOW_VIEW_MAX,
		};
	}

	world_render_context_config_t world_render_context_config_t::make_preview(vec2u16_t render_resolution)
	{
		return {
			.size				  = render_resolution,
			.entity_max			  = 128,
			.mesh_draw_max		  = 1024,
			.material_max		  = 256,
			.sprite_max			  = 16,
			.particle_max		  = 0,
			.bone_max			  = MAX_SKELETON_BONES,
			.light_max			  = 16,
			.reflection_probe_max = 8,
			.shadow_view_max	  = 8,
		};
	}
}
