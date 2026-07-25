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
#include <sfg/math/math.hpp>
#include <sfg/runtime/animation/animation_graph_util.hpp>
#include <sfg/runtime/resources/animation_graph.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skeleton.hpp>

namespace sfg
{
	void world_animation_controller_t::init(world_t& world, u32 bone_reserve, u32 animation_graph_memory_reserve)
	{
		_animation_graph_storage.init(animation_graph_memory_reserve);
		_bone_memory.init(sizeof(animation_bone_t) * bone_reserve * 2);

		_world					  = &world;
		_resource_reload_listener = resource_manager_t::get().add_reload_listener(on_reload, this);
	}

	void world_animation_controller_t::uninit()
	{
		resource_manager_t::get().remove_reload_listener(_resource_reload_listener);
		_resource_reload_listener = {};

		_bone_memory.uninit();
		_animation_graph_storage.uninit();

		_world = nullptr;
	}

	void world_animation_controller_t::on_reload(resource_manager_t& resource_manager, sid_t resource_id, resource_type_e resource_type, void* user_data)
	{
		if (resource_type != resource_type_e::animation_graph)
			return;

		world_animation_controller_t& controller			= *static_cast<world_animation_controller_t*>(user_data);
		const resource_entry_t*		  animation_graph_entry = resource_manager.find_entry(resource_id);
		SFG_ASSERT(animation_graph_entry != nullptr);

		const chunk_allocator_t&		 resource_memory			  = resource_manager.get_memory();
		const animation_graph_runtime_t* animation_graph			  = resource_memory.get<animation_graph_runtime_t>(animation_graph_entry->runtime);
		ecs_component_table_t&			 system_animation_graph_table = controller._world->get_component_table(type_id_t<component_system_animation_graph_t>::value);

		const ecs_component_table_ref_t table_refs[] = {
			system_animation_graph_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			component_system_animation_graph_t& system_animation_graph = ecs_helpers_t::row_get_mutable<component_system_animation_graph_t>(row, 0);

			if (system_animation_graph.animation_graph != resource_id)
				continue;

			const resource_entry_t* skeleton_entry = resource_manager.find_entry(system_animation_graph.skeleton);
			SFG_ASSERT(skeleton_entry != nullptr);

			const skeleton_runtime_t* skeleton = resource_memory.get<skeleton_runtime_t>(skeleton_entry->runtime);

			controller._animation_graph_storage.destroy_graph({
				.initial_pose	 = system_animation_graph.initial_pose,
				.parameters		 = system_animation_graph.parameters,
				.nodes			 = system_animation_graph.nodes,
				.parameter_count = system_animation_graph.parameter_count,
				.node_count		 = system_animation_graph.node_count,
			});

			const animation_graph_storage_instance_t instance = controller._animation_graph_storage.create_graph(*animation_graph, resource_memory, *skeleton);

			system_animation_graph.initial_pose			  = instance.initial_pose;
			system_animation_graph.parameters			  = instance.parameters;
			system_animation_graph.nodes				  = instance.nodes;
			system_animation_graph.accumulated_delta_time = 0.0f;
			system_animation_graph.parameter_count		  = instance.parameter_count;
			system_animation_graph.node_count			  = instance.node_count;
			system_animation_graph.tick_frame_count		  = 0;
		}
	}

	void world_animation_controller_t::tick_prep(f32 delta_time)
	{
		sync_create_destroy_skinned_renderers();
		sync_create_destroy_animation_graph();
	}

	void world_animation_controller_t::tick_logic(f32 delta_time)
	{
		ecs_component_table_t&				system_skinned_mesh_renderer_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		ecs_component_table_t&				system_animation_graph_table	   = _world->get_component_table(type_id_t<component_system_animation_graph_t>::value);
		const ecs_component_table_t&		system_transform_table			   = _world->get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t&		animation_graph_table			   = _world->get_component_table(type_id_t<component_animation_graph_t>::value);
		const ecs_component_table_t&		disabled_table					   = _world->get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t&		skinned_mesh_renderer_table		   = _world->get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
		const resource_manager_t&			resource_manager				   = resource_manager_t::get();
		const chunk_allocator32_t&			resource_memory					   = resource_manager.get_memory();
		const entity_id_t					main_camera_entity				   = _world->get_main_camera_entity();
		const component_system_transform_t* main_camera_transform			   = main_camera_entity == NULL_ENTITY_ID ? nullptr : ecs_helpers_t::table_find_as_const<component_system_transform_t>(system_transform_table, main_camera_entity);

		// finalize static poses once
		{
			const ecs_component_table_ref_t table_refs[] = {
				!disabled_table.ref(),
				system_skinned_mesh_renderer_table.ref(),
				!system_animation_graph_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer = ecs_helpers_t::row_get_mutable<component_system_skinned_mesh_renderer_t>(row, 1);

				if (system_skinned_mesh_renderer.final_bones_calculated)
					continue;

				const component_skinned_mesh_renderer_t& skinned_mesh_renderer = ecs_helpers_t::table_get_as_const<component_skinned_mesh_renderer_t>(skinned_mesh_renderer_table, row.id);
				const resource_entry_t*					 skeleton_entry		   = resource_manager.find_entry(skinned_mesh_renderer.skeleton);
				const skeleton_runtime_t&				 skeleton			   = *resource_memory.get<skeleton_runtime_t>(skeleton_entry->runtime);

				animation_graph_util_t::finalize_bones(skeleton, resource_memory, system_skinned_mesh_renderer.bones_handle, system_skinned_mesh_renderer.inverse_binds_handle, _bone_memory);

				system_skinned_mesh_renderer.final_bones_calculated = true;
			}
		}

		// process animated poses every tick
		{
			const ecs_component_table_ref_t table_refs[] = {
				!disabled_table.ref(),
				system_skinned_mesh_renderer_table.ref(),
				system_animation_graph_table.ref(),
				animation_graph_table.ref(),
				system_transform_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer = ecs_helpers_t::row_get_mutable<component_system_skinned_mesh_renderer_t>(row, 1);
				component_system_animation_graph_t&		  system_animation_graph	   = ecs_helpers_t::row_get_mutable<component_system_animation_graph_t>(row, 2);
				const component_animation_graph_t&		  animation_graph			   = ecs_helpers_t::row_get<component_animation_graph_t>(row, 3);
				const component_system_transform_t&		  system_transform			   = ecs_helpers_t::row_get<component_system_transform_t>(row, 4);

				const component_skinned_mesh_renderer_t& skinned_mesh_renderer = ecs_helpers_t::table_get_as_const<component_skinned_mesh_renderer_t>(skinned_mesh_renderer_table, row.id);
				const resource_entry_t*					 skeleton_entry		   = resource_manager.find_entry(skinned_mesh_renderer.skeleton);
				const skeleton_runtime_t&				 skeleton			   = *resource_memory.get<skeleton_runtime_t>(skeleton_entry->runtime);

				if (!system_skinned_mesh_renderer.final_bones_calculated)
				{
					animation_graph_util_t::finalize_bones(skeleton, resource_memory, system_skinned_mesh_renderer.bones_handle, system_skinned_mesh_renderer.inverse_binds_handle, _bone_memory);
					system_skinned_mesh_renderer.final_bones_calculated = true;
				}

				if (animation_graph.is_culled && main_camera_transform != nullptr)
				{
					const vec3f_t camera_to_entity = (system_transform.abs_pos - main_camera_transform->abs_pos).normalized();
					const f32	  camera_dot	   = vec3f_t::dot(main_camera_transform->abs_rot.get_forward(), camera_to_entity);
					const f32	  cull_dot_limit   = math::cos(math::degrees_to_radians(animation_graph.cull_angle_limit));

					if (camera_dot < cull_dot_limit)
						continue;
				}

				u32 effective_tick_rate = animation_graph.tick_rate;

				if (animation_graph.throttling_enabled && main_camera_transform != nullptr)
				{
					const f32 distance			 = vec3f_t::distance(system_transform.abs_pos, main_camera_transform->abs_pos);
					f32		  throttle_tick_rate = 1.0f;

					if (distance > animation_graph.throttle_begin_distance)
					{
						if (distance >= animation_graph.throttle_full_distance)
							throttle_tick_rate = static_cast<f32>(animation_graph.max_throttle_tick_rate);
						else
							throttle_tick_rate = math::remap(distance, animation_graph.throttle_begin_distance, animation_graph.throttle_full_distance, 1.0f, static_cast<f32>(animation_graph.max_throttle_tick_rate));
					}

					const u32 rounded_throttle_tick_rate = static_cast<u32>(math::round(throttle_tick_rate));
					effective_tick_rate					 = effective_tick_rate > rounded_throttle_tick_rate ? effective_tick_rate : rounded_throttle_tick_rate;
				}

				system_animation_graph.accumulated_delta_time += delta_time * animation_graph.speed;
				++system_animation_graph.tick_frame_count;

				if (system_animation_graph.tick_frame_count < effective_tick_rate)
					continue;

				const f32 graph_delta_time = system_animation_graph.accumulated_delta_time;

				system_animation_graph.accumulated_delta_time = 0.0f;
				system_animation_graph.tick_frame_count		  = 0;

				const span_t<animation_bone_t> bones{
					.data = _bone_memory.get<animation_bone_t>(system_skinned_mesh_renderer.bones_handle),
					.size = skeleton.joint_count,
				};

				_animation_graph_storage.process_graph(system_animation_graph.nodes, system_animation_graph.node_count, system_animation_graph.initial_pose, system_transform.abs_mat, bones, graph_delta_time);
				animation_graph_util_t::finalize_bones(skeleton, resource_memory, system_skinned_mesh_renderer.bones_handle, system_skinned_mesh_renderer.inverse_binds_handle, _bone_memory);

				system_skinned_mesh_renderer.final_bones_calculated = true;
			}
		}
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

		// destroy changed skeleton
		{
			const ecs_component_table_ref_t table_refs[] = {
				system_skinned_mesh_renderer_table.ref(),
				skinned_mesh_renderer_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer = ecs_helpers_t::row_get<component_system_skinned_mesh_renderer_t>(row, 0);
				const component_skinned_mesh_renderer_t&		skinned_mesh_renderer		 = ecs_helpers_t::row_get<component_skinned_mesh_renderer_t>(row, 1);

				if (system_skinned_mesh_renderer.skeleton != skinned_mesh_renderer.skeleton)
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
		const ecs_component_table_t& skinned_mesh_renderer_table		= _world->get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
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

		// destroy missing animation graph
		{
			const ecs_component_table_ref_t table_refs[] = {
				system_animation_graph_table.ref(),
				!animation_graph_table.ref(),
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

		// destroy changed resources
		{
			const ecs_component_table_ref_t table_refs[] = {
				system_animation_graph_table.ref(),
				animation_graph_table.ref(),
				skinned_mesh_renderer_table.ref(),
			};
			frame_vector_t<entity_id_t> destroy_entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_animation_graph_t& system_animation_graph = ecs_helpers_t::row_get<component_system_animation_graph_t>(row, 0);
				const component_animation_graph_t&		  animation_graph		 = ecs_helpers_t::row_get<component_animation_graph_t>(row, 1);
				const component_skinned_mesh_renderer_t&  skinned_mesh_renderer	 = ecs_helpers_t::row_get<component_skinned_mesh_renderer_t>(row, 2);

				if (system_animation_graph.animation_graph != animation_graph.animation_graph || system_animation_graph.skeleton != skinned_mesh_renderer.skeleton)
					destroy_entities.push_back(row.id);
			}

			for (const entity_id_t id : destroy_entities)
			{
				destroy_animation_graph(id);
			}
		}

		// create anim graph
		{
			struct animation_graph_create_t
			{
				const animation_graph_runtime_t* animation_graph		= nullptr;
				const skeleton_runtime_t*		 skeleton				= nullptr;
				resource_handle_t				 animation_graph_handle = NULL_RESOURCE_HANDLE;
				resource_handle_t				 skeleton_handle		= NULL_RESOURCE_HANDLE;
				entity_id_t						 id						= NULL_ENTITY_ID;
			};

			const ecs_component_table_ref_t table_refs[] = {
				!disabled_table.ref(),
				system_skinned_mesh_renderer_table.ref(),
				animation_graph_table.ref(),
				!system_animation_graph_table.ref(),
			};
			const resource_manager_t&				 resource_manager = resource_manager_t::get();
			const chunk_allocator32_t&				 resource_memory  = resource_manager.get_memory();
			frame_vector_t<animation_graph_create_t> create_graphs	  = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_animation_graph_t&		 animation_graph	   = ecs_helpers_t::row_get<component_animation_graph_t>(row, 2);
				const component_skinned_mesh_renderer_t& skinned_mesh_renderer = ecs_helpers_t::table_get_as_const<component_skinned_mesh_renderer_t>(skinned_mesh_renderer_table, row.id);
				const resource_entry_t*					 animation_graph_entry = resource_manager.find_entry(animation_graph.animation_graph);
				const resource_entry_t*					 skeleton_entry		   = resource_manager.find_entry(skinned_mesh_renderer.skeleton);

				if (animation_graph_entry == nullptr || skeleton_entry == nullptr)
					continue;

				const animation_graph_runtime_t* animation_graph_runtime = resource_memory.get<animation_graph_runtime_t>(animation_graph_entry->runtime);
				const skeleton_runtime_t*		 skeleton_runtime		 = resource_memory.get<skeleton_runtime_t>(skeleton_entry->runtime);

				if (animation_graph_runtime->target_skeleton != NULL_RESOURCE_HANDLE && animation_graph_runtime->target_skeleton != skinned_mesh_renderer.skeleton)
					continue;

				create_graphs.push_back({
					.animation_graph		= animation_graph_runtime,
					.skeleton				= skeleton_runtime,
					.animation_graph_handle = animation_graph.animation_graph,
					.skeleton_handle		= skinned_mesh_renderer.skeleton,
					.id						= row.id,
				});
			}

			for (const animation_graph_create_t& create : create_graphs)
			{
				create_animation_graph(create.id, create.animation_graph_handle, *create.animation_graph, create.skeleton_handle, *create.skeleton);
			}
		}
	}

	void world_animation_controller_t::create_animation_graph(entity_id_t id, resource_handle_t animation_graph_handle, const animation_graph_runtime_t& animation_graph, resource_handle_t skeleton_handle, const skeleton_runtime_t& skeleton)
	{
		const chunk_allocator32_t&				 resource_memory = resource_manager_t::get().get_memory();
		const animation_graph_storage_instance_t instance		 = _animation_graph_storage.create_graph(animation_graph, resource_memory, skeleton);

		ecs_component_table_t&				system_animation_graph_table = _world->get_component_table(type_id_t<component_system_animation_graph_t>::value);
		component_system_animation_graph_t& system_animation_graph		 = ecs_helpers_t::table_add_or_get_as<component_system_animation_graph_t>(system_animation_graph_table, id);

		system_animation_graph.animation_graph = animation_graph_handle;
		system_animation_graph.skeleton		   = skeleton_handle;
		system_animation_graph.initial_pose	   = instance.initial_pose;
		system_animation_graph.parameters	   = instance.parameters;
		system_animation_graph.nodes		   = instance.nodes;
		system_animation_graph.parameter_count = instance.parameter_count;
		system_animation_graph.node_count	   = instance.node_count;
	}

	void world_animation_controller_t::destroy_animation_graph(entity_id_t id)
	{
		ecs_component_table_t&					  system_animation_graph_table = _world->get_component_table(type_id_t<component_system_animation_graph_t>::value);
		const component_system_animation_graph_t& system_animation_graph	   = ecs_helpers_t::table_get_as<component_system_animation_graph_t>(system_animation_graph_table, id);

		const ecs_component_table_t& system_skinned_mesh_renderer_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);

		if (ecs_t::table_has(system_skinned_mesh_renderer_table, id))
		{
			component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer = ecs_helpers_t::table_get_as<component_system_skinned_mesh_renderer_t>(system_skinned_mesh_renderer_table, id);

			if (system_skinned_mesh_renderer.skeleton == system_animation_graph.skeleton)
			{
				const span_t<animation_bone_t> bones{
					.data = _bone_memory.get<animation_bone_t>(system_skinned_mesh_renderer.bones_handle),
					.size = system_skinned_mesh_renderer.bones_handle.size / sizeof(animation_bone_t),
				};

				_animation_graph_storage.copy_pose_to_bones(system_animation_graph.initial_pose, bones);
				system_skinned_mesh_renderer.final_bones_calculated = false;
			}
		}

		_animation_graph_storage.destroy_graph({
			.initial_pose	 = system_animation_graph.initial_pose,
			.parameters		 = system_animation_graph.parameters,
			.nodes			 = system_animation_graph.nodes,
			.parameter_count = system_animation_graph.parameter_count,
			.node_count		 = system_animation_graph.node_count,
		});

		ecs_t::table_remove(system_animation_graph_table, id);
	}

	void world_animation_controller_t::create_skinned_renderer(entity_id_t id, resource_handle_t skeleton_handle)
	{
		const resource_manager_t&  resource_manager = resource_manager_t::get();
		const resource_entry_t*	   skeleton_entry	= resource_manager.find_entry(skeleton_handle);
		const chunk_allocator32_t& resource_memory	= resource_manager.get_memory();
		const skeleton_runtime_t*  skeleton			= resource_memory.get<skeleton_runtime_t>(skeleton_entry->runtime);

		const skeleton_joint_runtime_t* joints				 = resource_memory.get<skeleton_joint_runtime_t>(skeleton->joints);
		const chunk_handle32_t			bones_handle		 = allocate_bones(skeleton->joint_count);
		const chunk_handle32_t			inverse_binds_handle = allocate_bones(skeleton->joint_count);
		animation_bone_t*				bones				 = _bone_memory.get<animation_bone_t>(bones_handle);
		animation_bone_t*				inverse_binds		 = _bone_memory.get<animation_bone_t>(inverse_binds_handle);

		for (u32 joint_index = 0; joint_index < skeleton->joint_count; ++joint_index)
		{
			bones[joint_index].bone_transform		  = joints[joint_index].local;
			inverse_binds[joint_index].bone_transform = joints[joint_index].inverse_bind;
		}

		ecs_component_table_t&					  system_skinned_mesh_renderer_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer		 = ecs_helpers_t::table_add_or_get_as<component_system_skinned_mesh_renderer_t>(system_skinned_mesh_renderer_table, id);

		system_skinned_mesh_renderer.skeleton				= skeleton_handle;
		system_skinned_mesh_renderer.bones_handle			= bones_handle;
		system_skinned_mesh_renderer.inverse_binds_handle	= inverse_binds_handle;
		system_skinned_mesh_renderer.final_bones_calculated = false;
	}

	void world_animation_controller_t::destroy_skinned_renderer(entity_id_t id)
	{
		ecs_component_table_t&							system_skinned_mesh_renderer_table = _world->get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		const component_system_skinned_mesh_renderer_t& system_skinned_mesh_renderer	   = ecs_helpers_t::table_get_as<component_system_skinned_mesh_renderer_t>(system_skinned_mesh_renderer_table, id);

		deallocate_bones(system_skinned_mesh_renderer.bones_handle);
		deallocate_bones(system_skinned_mesh_renderer.inverse_binds_handle);
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
