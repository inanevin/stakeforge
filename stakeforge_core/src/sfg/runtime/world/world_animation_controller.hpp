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
	class mat4x3_t;
	class quat_t;
	struct skeleton_runtime_t;
	struct vec3f_t;
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
		void clear();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void destroy_entity(entity_id_t entity);
		void reset_pose_after_ragdoll(entity_id_t entity);
		void tick_prep(f32 delta_time);
		void tick_logic(f32 delta_time);
		bool set_graph_parameter_f32(entity_id_t entity, sid_t parameter_hash, f32 value);
		bool set_graph_parameter_vec2(entity_id_t entity, sid_t parameter_hash, const vec2f_t& value);
		bool set_graph_parameter_vec3(entity_id_t entity, sid_t parameter_hash, const vec3f_t& value);
		bool set_graph_parameter_quat(entity_id_t entity, sid_t parameter_hash, const quat_t& value);
		bool set_graph_parameter_bool(entity_id_t entity, sid_t parameter_hash, bool value);
		bool get_graph_parameter_f32(entity_id_t entity, sid_t parameter_hash, f32& out_value) const;
		bool get_graph_parameter_vec2(entity_id_t entity, sid_t parameter_hash, vec2f_t& out_value) const;
		bool get_graph_parameter_vec3(entity_id_t entity, sid_t parameter_hash, vec3f_t& out_value) const;
		bool get_graph_parameter_quat(entity_id_t entity, sid_t parameter_hash, quat_t& out_value) const;
		bool get_graph_parameter_bool(entity_id_t entity, sid_t parameter_hash, bool& out_value) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		bool get_slot_pos_abs(entity_id_t entity, sid_t slot_name_hash, vec3f_t& out_position) const;
		bool get_slot_rot_abs(entity_id_t entity, sid_t slot_name_hash, quat_t& out_rotation) const;

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

		void sync_create_destroy_animation_player();
		void create_animation_player(entity_id_t id, resource_handle_t animation_handle);
		void destroy_animation_player(entity_id_t id);

		void sync_create_destroy_animation_graph();
		void create_animation_graph(entity_id_t id, resource_handle_t animation_graph_handle, const animation_graph_runtime_t& animation_graph, resource_handle_t skeleton_handle, const skeleton_runtime_t& skeleton);
		void destroy_animation_graph(entity_id_t id);

		animation_graph_param_t*	   find_graph_parameter(entity_id_t entity, sid_t parameter_hash);
		const animation_graph_param_t* find_graph_parameter(entity_id_t entity, sid_t parameter_hash) const;
		bool						   get_slot_transform_abs(entity_id_t entity, sid_t slot_name_hash, mat4x3_t& out_transform) const;
		chunk_handle32_t			   allocate_bones(u32 bone_count);
		void						   deallocate_bones(chunk_handle32_t handle);
		static void					   on_reload(resource_manager_t& resource_manager, sid_t resource_id, resource_type_e resource_type, void* user_data);

	private:
		animation_graph_storage_t		  _animation_graph_storage	= {};
		chunk_allocator32_t				  _bone_memory				= {};
		world_t*						  _world					= nullptr;
		resource_reload_listener_handle_t _resource_reload_listener = {};
	};
}
