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
#include <sfg/data/span.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/runtime/animation/animation_bone.hpp>
#include <sfg/runtime/animation/animation_graph_storage.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/resources/resource_reload_listener.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	struct animation_graph_runtime_t;
	struct skeleton_runtime_t;
	class world_t;

	class world_animation_controller_t final
	{
	public:
		world_animation_controller_t()												 = default;
		~world_animation_controller_t()												 = default;
		world_animation_controller_t(const world_animation_controller_t&)			 = delete;
		world_animation_controller_t& operator=(const world_animation_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world, u32 bone_max_count, u32 animation_graph_budget_bytes);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick_prep(f32 delta_time);
		void tick_logic(f32 delta_time);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline span_t<const animation_bone_t> get_bones(chunk_handle32_t handle) const
		{
			return {
				.data = _bone_memory.get<animation_bone_t>(handle),
				.size = handle.size / sizeof(animation_bone_t),
			};
		}

	private:
		void sync_create_destroy_skinned_renderers();
		void create_skinned_renderer(entity_id_t id, resource_handle_t skeleton_handle);
		void destroy_skinned_renderer(entity_id_t id);

		void sync_create_destroy_animation_graph();
		void create_animation_graph(entity_id_t id, resource_handle_t animation_graph_handle, const animation_graph_runtime_t& animation_graph, resource_handle_t skeleton_handle, const skeleton_runtime_t& skeleton);
		void destroy_animation_graph(entity_id_t id);

		chunk_handle32_t allocate_bones(u32 bone_count);
		void			 deallocate_bones(chunk_handle32_t handle);
		static void		 on_reload(resource_manager_t& resource_manager, sid_t resource_id, resource_type_e resource_type, void* user_data);

	private:
		animation_graph_storage_t		  _animation_graph_storage	= {};
		chunk_allocator32_t				  _bone_memory				= {};
		world_t*						  _world					= nullptr;
		resource_reload_listener_handle_t _resource_reload_listener = {};
	};
}
