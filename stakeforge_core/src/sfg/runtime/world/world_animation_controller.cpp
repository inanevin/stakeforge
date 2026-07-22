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

#include "world_animation_controller.hpp"
#include "ecs.hpp"
#include "ecs_helpers.hpp"
#include "engine_components.hpp"
#include "system_components.hpp"
#include "world.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skeleton.hpp>

namespace sfg
{
	void world_animation_controller_t::init(world_t& world, u32 bone_reserve, u32 animation_graph_memory_reserve)
	{
		_animation_graph_storage.init(animation_graph_memory_reserve);
		_bone_memory.init(sizeof(animation_bone_t) * bone_reserve);

		_world = &world;
	}

	void world_animation_controller_t::uninit()
	{
		_bone_memory.uninit();
		_animation_graph_storage.uninit();

		_world = nullptr;
	}

	void world_animation_controller_t::tick_prep(f32 delta_time)
	{
		sync_create_destroy_skinned_renderers();
		sync_create_destroy_animation_graph();
	}

	void world_animation_controller_t::tick_logic(f32 delta_time)
	{
	}

	void world_animation_controller_t::sync_create_destroy_skinned_renderers()
	{
		ecs_component_table_t&		 system_skinned_mesh_renderer_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		const ecs_component_table_t& disabled_table						= _world->get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& skinned_mesh_renderer_table		= _world->get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);

		// destroy disabled
		{
			const ecs_component_table_ref_t table_refs[] = {
				system_skinned_mesh_renderer_table.ref(),
				disabled_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				destroy_entities.push_back(row.id);
			}

			for (const entity_id_t id : destroy_entities)
			{
				destroy_skinned_renderer(id);
			}
		}

		// destroy missing skinned mesh
		{
			const ecs_component_table_ref_t table_refs[] = {
				system_skinned_mesh_renderer_table.ref(),
				!skinned_mesh_renderer_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				destroy_entities.push_back(row.id);
			}

			for (const entity_id_t id : destroy_entities)
			{
				destroy_skinned_renderer(id);
			}
		}

		// create skinned mesh
		{
			struct skinned_renderer_create_t
			{
				resource_handle_t skeleton = NULL_RESOURCE_HANDLE;
				entity_id_t		  id	   = NULL_ENTITY_ID;
			};

			const resource_manager_t&		resource_manager = resource_manager_t::get();
			const ecs_component_table_ref_t table_refs[]	 = {
				skinned_mesh_renderer_table.ref(),
				!disabled_table.ref(),
				!system_skinned_mesh_renderer_table.ref(),
			};
			frame_vector_t<skinned_renderer_create_t> create_renderers = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_skinned_mesh_renderer_t& skinned_mesh_renderer = ecs_helpers_t::row_get<component_skinned_mesh_renderer_t>(row, 0);
				const resource_entry_t*					 skeleton_entry		   = resource_manager.find_entry(skinned_mesh_renderer.skeleton);

				if (skeleton_entry == nullptr)
					continue;

				create_renderers.push_back({
					.skeleton = skinned_mesh_renderer.skeleton,
					.id		  = row.id,
				});
			}

			for (const skinned_renderer_create_t& create : create_renderers)
			{
				create_skinned_renderer(create.id, create.skeleton);
			}
		}
	}

	void world_animation_controller_t::sync_create_destroy_animation_graph()
	{
		ecs_component_table_t&		 system_animation_graph_table		= _world->get_component_table(type_id_t<component_system_animation_graph_t>::value);
		const ecs_component_table_t& disabled_table						= _world->get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& system_skinned_mesh_renderer_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		const ecs_component_table_t& animation_graph_table				= _world->get_component_table(type_id_t<component_animation_graph_t>::value);

		// destroy disabled
		{
			const ecs_component_table_ref_t table_refs[] = {
				system_animation_graph_table.ref(),
				disabled_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				destroy_entities.push_back(row.id);
			}

			for (const entity_id_t id : destroy_entities)
			{
				destroy_animation_graph(id);
			}
		}

		// destroy missing skinned mesh
		{
			const ecs_component_table_ref_t table_refs[] = {
				system_animation_graph_table.ref(),
				!system_skinned_mesh_renderer_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				destroy_entities.push_back(row.id);
			}

			for (const entity_id_t id : destroy_entities)
			{
				destroy_animation_graph(id);
			}
		}

		// create anim graph
		{
			const ecs_component_table_ref_t table_refs[] = {
				!disabled_table.ref(),
				system_skinned_mesh_renderer_table.ref(),
				animation_graph_table.ref(),
				!system_animation_graph_table.ref(),
			};
			frame_vector_t<entity_id_t> create_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				create_entities.push_back(row.id);
			}

			for (const entity_id_t id : create_entities)
			{
				create_animation_graph(id);
			}
		}
	}

	void world_animation_controller_t::create_animation_graph(entity_id_t id)
	{
		ecs_component_table_t& system_animation_graph_table = _world->get_component_table(type_id_t<component_system_animation_graph_t>::value);

		ecs_helpers_t::table_add_or_get_as<component_system_animation_graph_t>(system_animation_graph_table, id);
	}

	void world_animation_controller_t::destroy_animation_graph(entity_id_t id)
	{
		ecs_component_table_t& system_animation_graph_table = _world->get_component_table(type_id_t<component_system_animation_graph_t>::value);

		ecs_t::table_remove(system_animation_graph_table, id);
	}

	void world_animation_controller_t::create_skinned_renderer(entity_id_t id, resource_handle_t skeleton_handle)
	{
		const resource_manager_t&  resource_manager = resource_manager_t::get();
		const resource_entry_t*	   skeleton_entry	= resource_manager.find_entry(skeleton_handle);
		const chunk_allocator32_t& resource_memory	= resource_manager.get_memory();
		const skeleton_runtime_t*  skeleton			= resource_memory.get<skeleton_runtime_t>(skeleton_entry->runtime);

		const skeleton_joint_runtime_t* joints			 = resource_memory.get<skeleton_joint_runtime_t>(skeleton->joints);
		const u32*						evaluation_order = resource_memory.get<u32>(skeleton->evaluation_order);
		frame_vector_t<mat4x3_t>		absolute_transforms(skeleton->joint_count, mat4x3_t::identity);

		for (u32 i = 0; i < skeleton->joint_count; ++i)
		{
			const u32						joint_index = evaluation_order[i];
			const skeleton_joint_runtime_t& joint		= joints[joint_index];
			absolute_transforms[joint_index]			= joint.parent_index == SKELETON_JOINT_NO_PARENT ? joint.local : absolute_transforms[joint.parent_index] * joint.local;
		}

		const mat4x3_t			root_inverse = skeleton->root_joint_index == SKELETON_JOINT_NO_PARENT ? mat4x3_t::identity : absolute_transforms[skeleton->root_joint_index].inverse();
		const chunk_handle32_t	bones_handle = allocate_bones(skeleton->joint_count);
		animation_bone_t* const bones		 = _bone_memory.get<animation_bone_t>(bones_handle);

		for (u32 joint_index = 0; joint_index < skeleton->joint_count; ++joint_index)
		{
			bones[joint_index].bone_transform = root_inverse * absolute_transforms[joint_index] * joints[joint_index].inverse_bind;
		}

		ecs_component_table_t&					  system_skinned_mesh_renderer_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer		 = ecs_helpers_t::table_add_or_get_as<component_system_skinned_mesh_renderer_t>(system_skinned_mesh_renderer_table, id);

		system_skinned_mesh_renderer.bones_handle = bones_handle;
	}

	void world_animation_controller_t::destroy_skinned_renderer(entity_id_t id)
	{
		ecs_component_table_t&							system_skinned_mesh_renderer_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		const component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer	   = ecs_helpers_t::table_get_as<component_system_skinned_mesh_renderer_t>(system_skinned_mesh_renderer_table, id);

		deallocate_bones(system_skinned_mesh_renderer.bones_handle);
		ecs_t::table_remove(system_skinned_mesh_renderer_table, id);
	}

	chunk_handle32_t world_animation_controller_t::allocate_bones(u32 bone_count)
	{
		return _bone_memory.allocate<animation_bone_t>(bone_count);
	}

	void world_animation_controller_t::deallocate_bones(chunk_handle32_t handle)
	{
		_bone_memory.free(handle);
	}
}
