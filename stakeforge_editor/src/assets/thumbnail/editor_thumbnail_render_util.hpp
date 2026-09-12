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

#include "assets/editor_asset_type.hpp"

#include <sfg/data/vector.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	struct aabb_t;
	struct world_debug_draw_snapshot_t;

	struct editor_thumbnail_world_t
	{
		world_t*					world				  = nullptr;
		vector_t<resource_handle_t> texture_resources	  = {};
		resource_handle_t			collision_mesh		  = NULL_RESOURCE_HANDLE;
		vec3f_t						collision_mesh_center = vec3f_t::zero;
		entity_id_t					environment_entity	  = NULL_ENTITY_ID;
		entity_id_t					camera_entity		  = NULL_ENTITY_ID;
		entity_id_t					display_entity		  = NULL_ENTITY_ID;
	};

	class editor_thumbnail_render_util_t final
	{
	public:
		editor_thumbnail_render_util_t() = delete;

		static inline constexpr u32 DEBUG_TRIANGLE_VERTEX_MAX = 164000;
		static inline constexpr u32 DEBUG_TRIANGLE_INDEX_MAX  = 164000;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static void setup_world_for_asset(editor_thumbnail_world_t& thumbnail_world, editor_asset_type_e asset_type, resource_handle_t asset_guid);
		static void collect_texture_resources(editor_thumbnail_world_t& thumbnail_world);
		static void write_collision_mesh_debug_draw(const editor_thumbnail_world_t& thumbnail_world, world_debug_draw_snapshot_t& debug_draw);
		static bool is_ready_to_render(const editor_thumbnail_world_t& thumbnail_world);

	private:
		static void place_camera_for_aabb(world_t& world, entity_id_t camera_entity, const aabb_t& aabb);
		static void setup_base_world(editor_thumbnail_world_t& thumbnail_world);
		static void setup_camera_for_asset(editor_thumbnail_world_t& thumbnail_world);
		static void setup_world_for_prefab(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid);
		static void setup_world_for_material(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid);
		static void setup_world_for_mesh(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid);
		static void setup_world_for_animation(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid);
		static void setup_world_for_collision_mesh(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid);
	};
}
