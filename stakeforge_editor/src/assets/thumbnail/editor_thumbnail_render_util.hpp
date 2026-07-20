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
		static void setup_world_for_skybox(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid);
		static void setup_world_for_animation(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid);
		static void setup_world_for_collision_mesh(editor_thumbnail_world_t& thumbnail_world, resource_handle_t asset_guid);
	};
}
