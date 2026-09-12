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

#include <sfg/data/span.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_debug_draw_t;
	class world_t;
	class mat4x3_t;
	struct component_system_transform_t;
	struct vec2u16_t;
	struct world_render_snapshot_t;
	enum class debug_draw_depth_e : u8;

	class editor_world_util_t final
	{
	public:
		editor_world_util_t() = delete;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static entity_id_t install_default_scene_light(world_t& world);
		static entity_id_t install_default_scene_dark(world_t& world, f32 spotlight_y = 5.0f);
		static void		   draw_component_icons(world_t& world, entity_id_t editor_camera_entity);
		static void		   draw_selection_gizmos(world_t& world, span_t<const entity_id_t> selected_entities, const vec2u16_t& render_resolution);
		static void		   draw_bounding_boxes(world_t& world, const world_render_snapshot_t& snapshot, entity_id_t editor_camera_entity);
		static void		   draw_transform_axes(world_debug_draw_t& debug_draw, const mat4x3_t& transform, f32 axis_length, f32 thickness_px, debug_draw_depth_e depth);

	private:
		static void draw_constraint_gizmos(world_t& world, entity_id_t entity, const component_system_transform_t& transform, world_debug_draw_t& debug_draw);
	};
}
