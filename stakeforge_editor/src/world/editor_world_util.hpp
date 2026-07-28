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

		static entity_id_t install_default_scene(world_t& world);
		static void		   draw_component_icons(world_t& world, entity_id_t editor_camera_entity);
		static void		   draw_selection_gizmos(world_t& world, span_t<const entity_id_t> selected_entities, const vec2u16_t& render_resolution);
		static void		   draw_skeletons(world_t& world);
		static void		   draw_bounding_boxes(world_t& world, const world_render_snapshot_t& snapshot, entity_id_t editor_camera_entity);
		static void		   draw_transform_axes(world_debug_draw_t& debug_draw, const mat4x3_t& transform, f32 axis_length, f32 thickness_px, debug_draw_depth_e depth);
		static void		   draw_skeleton_slot(world_debug_draw_t& debug_draw, const mat4x3_t& transform, const char* name, f32 half_extent, f32 axis_length, f32 text_size);

	private:
		static void draw_constraint_gizmos(world_t& world, entity_id_t entity, const component_system_transform_t& transform, world_debug_draw_t& debug_draw);
	};
}
