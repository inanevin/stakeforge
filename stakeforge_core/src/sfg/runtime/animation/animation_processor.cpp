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

#include "animation_processor.hpp"
#include <sfg/math/math.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/animation_library.hpp>
#include <sfg/runtime/resources/skeleton.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

namespace sfg
{
	void animation_processor_t::init(world_t& world, size_t aux_size, size_t max_library_support)
	{
		_world = &world;
		_aux.init(aux_size);
		_bone_aux.init(max_library_support * (MAX_SKELETON_BONES * 3 + 1));

		_states.reserve(1000);
		_libraries.reserve(max_library_support);
	}

	void animation_processor_t::uninit()
	{
		_world = nullptr;
		_aux.uninit();
	}

	void animation_processor_t::tick(f32 dt)
	{
		ecs_component_table_t& table_lib		   = _world->get_component_table(type_id_t<component_animation_library_t>::value);
		ecs_component_table_t& table_sys_lib	   = _world->get_component_table(type_id_t<component_system_animation_library_t>::value);
		ecs_component_table_t& table_disabled	   = _world->get_component_table(type_id_t<component_disabled_t>::value);
		ecs_component_table_t& table_sys_transform = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		// allocate & deallocate system animation library
		{
			frame_vector_t<entity_id_t> add_sys_entities	= {};
			frame_vector_t<entity_id_t> remove_sys_entities = {};

			// missing sys
			{
				ecs_component_table_ref_t refs[] = {table_lib.ref(), !table_sys_lib.ref(), !table_disabled.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					add_sys_entities.push_back(row.id);
			}

			// remove sys
			{
				ecs_component_table_ref_t refs[] = {!table_lib.ref(), table_sys_lib.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					remove_sys_entities.push_back(row.id);
			}

			// changed
			{
				ecs_component_table_ref_t refs[] = {table_lib.ref(), table_sys_lib.ref(), !table_disabled.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				{
					const component_animation_library_t&		lib		= row.get<component_animation_library_t>();
					const component_system_animation_library_t& sys_lib = row.get<component_system_animation_library_t>();

					if (lib.animation_library == sys_lib.animation_library)
						continue;

					remove_sys_entities.push_back(row.id);
					add_sys_entities.push_back(row.id);
				}
			}

			for (entity_id_t id : remove_sys_entities)
				dealloc_for_entity(id);

			for (entity_id_t id : add_sys_entities)
				alloc_for_entity(id);
		}

		const entity_id_t ent_main_camera = _world->get_main_camera_entity();

		if (ent_main_camera == NULL_ENTITY_ID)
			return;

		const component_camera_t& comp_camera = _world->get_component_table<component_camera_t>().get_as<component_camera_t>(ent_main_camera);

		const float	  fov	  = comp_camera.fov_degrees;
		const vec3f_t cam_pos = _world->get_entity_pos_abs(ent_main_camera);
		const vec3f_t cam_fw  = _world->get_entity_rot_abs(ent_main_camera).get_forward().normalized();

		ecs_component_table_ref_t refs[] = {table_lib.ref(), table_sys_lib.ref(), !table_disabled.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_animation_library_t&		lib				 = row.get<component_animation_library_t>();
			const component_system_animation_library_t& sys_lib			 = row.get<component_system_animation_library_t>();
			const vec3f_t								pos				 = _world->get_entity_pos_abs(row.id);
			const vec3f_t								cam_to_pos		 = (pos - cam_pos).normalized();
			const float									dot				 = vec3f_t::dot(cam_to_pos, cam_fw);
			const bool									sample_animation = dot < math::cos(lib.cull_angle_limit * DEG_2_RAD);

			animator_pose_t layer_poses[MAX_ANIMATION_LIBRARY_LAYERS] = {};

			animator_library_t& anim_lib = _libraries.get({sys_lib.lib_alloc.index, sys_lib.lib_alloc.generation});

			for (u32 i = 0; i < anim_lib.layer_count; i++)
			{
				animator_layer_t& layer = anim_lib.layers[i];
				if (layer.active_state.is_null())
					continue;

				animator_state_t& active_state = _states.get(layer.active_state);
				animator_pose_t&  pose		   = layer_poses[i];

				if (layer.current_switch.active)
				{
					process_state(active_state, pose, layer.mask, 1.0f, dt, sample_animation);
					process_state(_states.get(layer.current_switch.target_state), pose, layer.mask, layer.current_switch.current_time / layer.current_switch.duration, dt, sample_animation);

					layer.current_switch.current_time += dt;

					if (layer.current_switch.current_time >= layer.current_switch.duration)
					{
						switch_layer_state({sys_lib.lib_alloc.index, sys_lib.lib_alloc.generation}, 0, layer.current_switch.target_state, 0.0f);
						layer.current_switch = {};
					}

					continue;
				}

				process_state(active_state, pose, layer.mask, 1.0f, dt, sample_animation);
			}

			if (!sample_animation)
				continue;

			for (u32 i = 1; i < anim_lib.layer_count; i++)
			{
				animator_layer_t& layer = anim_lib.layers[i];
				if (layer.active_state.is_null())
					continue;

				blend_poses(layer_poses[i - 1], layer_poses[i], layer.weight);
			}

			const animator_pose_t& final_pose = layer_poses[anim_lib.layer_count - 1];
		}
	}

	void animation_processor_t::switch_layer_state(animator_library_handle_t library, u32 layer_index, animator_state_handle_t state, f32 transition_duration)
	{
		if (_libraries.is_valid(library))
		{
			SFG_ERR("library handle is not valid!");
			return;
		}

		if (!_states.is_valid(state))
		{
			SFG_ERR("state handle is not valid!");
			return;
		}

		animator_library_t& lib = _libraries.get(library);

		if (lib.layer_count <= layer_index)
		{
			SFG_ERR("layer_index index is too big! index: {0}, library layer_index count: {1}", layer_index, lib.layer_count);
			return;
		}

		_states.get(state).current_time = 0.0f;

		animator_layer_t& layer = lib.layers[layer_index];
		if (math::almost_equal(transition_duration, 0.0f))
		{
			layer.active_state			= state;
			layer.current_switch.active = false;
			return;
		}

		layer.current_switch.active		  = true;
		layer.current_switch.duration	  = transition_duration;
		layer.current_switch.current_time = 0.0f;
		layer.current_switch.target_state = state;
	}

	animator_state_handle_t animation_processor_t::find_state_handle(animator_library_handle_t library, sid_t name_hash, u32 layer_index)
	{
		if (_libraries.is_valid(library))
		{
			SFG_ERR("library handle is not valid!");
			return {};
		}

		animator_library_t& lib = _libraries.get(library);

		if (layer_index != UINT32_MAX)
		{
			if (lib.layer_count <= layer_index)
			{
				SFG_ERR("layer_index index is too big! index: {0}, library layer_index count: {1}", layer_index, lib.layer_count);
				return {};
			}

			const animator_state_handle_t* states = _aux.get<animator_state_handle_t>(lib.layers[layer_index].state_handles);
			for (u32 j = 0; j < lib.layers[layer_index].state_count; j++)
			{
				const animator_state_t& state = _states.get(states[j]);
				if (state.name_hash == name_hash)
					return states[j];
			}
		}

		for (u32 i = 0; i < lib.layer_count; i++)
		{
			const animator_state_handle_t* states = _aux.get<animator_state_handle_t>(lib.layers[i].state_handles);
			for (u32 j = 0; j < lib.layers[i].state_count; j++)
			{
				const animator_state_t& state = _states.get(states[j]);
				if (state.name_hash == name_hash)
					return states[j];
			}
		}

		SFG_WARN("failed finding state handle!");
		return {};
	}

	void animation_processor_t::alloc_for_entity(entity_id_t id)
	{
		const component_animation_library_t&  comp_lib = _world->get_component_table<component_animation_library_t>().get_as<component_animation_library_t>(id);
		component_system_animation_library_t& sys	   = _world->get_component_table<component_system_animation_library_t>().get_as<component_system_animation_library_t>(id);
		resource_manager_t&					  rm	   = resource_manager_t::get();

		const animator_library_handle_t handle = _libraries.add();
		sys.lib_alloc						   = {handle.index, handle.generation};

		animator_library_t& anim_lib = _libraries.get(handle);

		const animation_library_runtime_t* res_lib = rm.find_runtime<animation_library_runtime_t>(comp_lib.animation_library);

		if (res_lib == nullptr)
		{
			SFG_WARN("entity has animation library component referencing a missing resource! {0}", id);
			_world->get_component_table<component_system_animation_library_t>().remove(id);
			return;
		}

		const skeleton_runtime_t* res_skeleton = rm.find_runtime<skeleton_runtime_t>(res_lib->skeleton);

		if (res_skeleton == nullptr)
		{
			SFG_WARN("entity has animation library component referencing a missing skeleton! {0}", id);
			_world->get_component_table<component_system_animation_library_t>().remove(id);
			return;
		}

		const u32 skeleton_joint_count = res_skeleton->joint_count;
		const u32 total_space		   = 1 + skeleton_joint_count * 3; // skinning matrix + final skinned + inverse binds + local manipulate
		sys.bone_alloc				   = _bone_aux.allocate<mat4x3_t>(total_space);

		mat4x3_t* const matrices = _bone_aux.get<mat4x3_t>(sys.bone_alloc);
		matrices[0]				 = res_skeleton->skinning_transform;

		const skeleton_joint_runtime_t* joints = rm.get_memory().get<skeleton_joint_runtime_t>(res_skeleton->joints);

		for (u32 i = 0; i < skeleton_joint_count; i++)
		{
			// inverse binds && locals
			matrices[1 + skeleton_joint_count + i]	   = joints[i].inverse_bind;
			matrices[1 + skeleton_joint_count * 2 + i] = joints[i].local;
		}

		const animation_library_layer_runtime_t* res_layers			= res_lib->layers;
		skeleton_mask_runtime_t*				 res_skeleton_masks = res_skeleton->mask_count == 0 ? nullptr : rm.get_memory().get<skeleton_mask_runtime_t>(res_skeleton->masks);

		anim_lib.skeleton_handle = res_lib->skeleton;
		anim_lib.layer_count	 = res_lib->layer_count;

		for (u32 i = 0; i < res_lib->layer_count; i++)
		{
			animator_layer_t&						 layer	   = anim_lib.layers[i];
			const animation_library_layer_runtime_t& res_layer = res_layers[i];

			layer.weight	= i == 0 ? 1.0f : res_layer.weight;
			layer.name_hash = res_layer.name_hash;
			layer.mask		= {};

			if (res_layer.mask != NULL_SID)
			{
				for (u32 j = 0; j < res_skeleton->mask_count; j++)
				{
					const skeleton_mask_runtime_t& mask_rt = res_skeleton_masks[j];

					if (mask_rt.name_hash == res_layer.mask)
					{
						layer.mask = mask_rt.value;
						break;
					}
				}
			}

			const animation_library_state_runtime_t* states = res_layer.state_count == 0 ? nullptr : rm.get_memory().get<animation_library_state_runtime_t>(res_layers[i].states);

			layer.state_handles = res_layer.state_count == 0 ? chunk_handle32_t{} : _aux.allocate<animator_state_handle_t>(res_layer.state_count);
			layer.state_count	= res_layer.state_count;

			for (u32 j = 0; j < res_layer.state_count; j++)
			{
				const animation_library_state_runtime_t& res_state	  = states[j];
				const animator_state_handle_t			 state_handle = _states.add();
				animator_state_t&						 state		  = _states.get(state_handle);

				state.layer_index		   = i;
				state.clip_count		   = res_state.clip_count;
				state.speed				   = res_state.speed;
				state.blend_type		   = res_state.blend_type;
				state.blend_position_value = res_state.initial_blend_value;
				state.loop				   = res_state.loop;
				state.name_hash			   = res_state.name_hash;

				if (res_state.triangle_count != 0)
				{
					state.delaunay_triangles = _aux.allocate<vec2f_t>(res_state.triangle_count * 3);
					state.triangle_count	 = res_state.triangle_count;

					animation_library_state_delaunay_triangle_t*	   state_tris = _aux.get<animation_library_state_delaunay_triangle_t>(state.delaunay_triangles);
					const animation_library_state_delaunay_triangle_t* tris		  = rm.get_memory().get<animation_library_state_delaunay_triangle_t>(res_state.delaunay_triangles);
					for (u32 t = 0; t < res_state.triangle_count; t++)
					{
						state_tris[i] = tris[i];
					}
				}
				for (u32 k = 0; k < res_state.clip_count; k++)
				{
					const animation_library_clip_runtime_t& res_clip = res_state.clips[k];

					state.clips[k].blend_position = res_clip.weight_value;
					state.clips[k].clip_handle	  = res_clip.animation_clip;
					state.clips[k].duration		  = res_clip.duration;
					state.clips[k].start_time	  = res_clip.start_time;
				}

				_aux.get<animator_state_handle_t>(layer.state_handles)[j] = state_handle;

				if (j == res_layer.default_active_state)
					layer.active_state = state_handle;
			}
		}
	}

	void animation_processor_t::dealloc_for_entity(entity_id_t id)
	{
		const component_animation_library_t&		lib = _world->get_component_table<component_animation_library_t>().get_as<component_animation_library_t>(id);
		const component_system_animation_library_t& sys = _world->get_component_table<component_system_animation_library_t>().get_as<component_system_animation_library_t>(id);

		const animator_library_handle_t lib_handle = {sys.lib_alloc.index, sys.lib_alloc.generation};
		const animator_library_t&		anim_lib   = _libraries.get(lib_handle);

		if (sys.bone_alloc)
			_bone_aux.free(sys.bone_alloc);

		for (u32 i = 0; i < anim_lib.layer_count; i++)
		{
			const animator_layer_t&		   layer		 = anim_lib.layers[i];
			const animator_state_handle_t* state_handles = layer.state_count == 0 ? nullptr : _aux.get<animator_state_handle_t>(layer.state_handles);

			for (u32 j = 0; j < layer.state_count; j++)
			{
				animator_state_t& state = _states.get(state_handles[j]);
				if (state.triangle_count != 0)
					_aux.free(state.delaunay_triangles);

				_states.remove(state_handles[j]);
			}
			_aux.free(layer.state_handles);
		}
	}

	void animation_processor_t::process_state(animator_state_t& state, animator_pose_t& write_pose, const skeleton_mask_t& mask, f32 weight, f32 dt, bool sample_animation)
	{
		if (state.clip_count == 0)
			return;

		// state processing accesses resource memory for sampling
		// TODO: think of an alternative more local memory access.

		if (state.blend_type == animation_library_blend_type_e::no_blend || state.clip_count == 1)
		{
			// sample clip 0
		}
		else if (state.blend_type == animation_library_blend_type_e::blend_1d)
		{
			const float target		= state.blend_position_value.x;
			u32			lower_idx	= UINT32_MAX;
			u32			higher_idx	= UINT32_MAX;
			f32			lower_val	= -10.0f;
			f32			higher_val	= 10.0f;
			f32			lower_diff	= 0.0f;
			f32			higher_diff = 0.0f;

			for (u32 i = 0; i < state.clip_count; i++)
			{
				const animator_clip_t& clip = state.clips[i];
				const float			   p	= clip.blend_position.x;
				if (p < target && p > lower_val)
				{
					lower_val  = p;
					lower_idx  = i;
					lower_diff = target - p;
				}

				if (p > target && p < higher_val)
				{
					higher_val	= p;
					higher_idx	= i;
					higher_diff = p - target;
				}
			}

			if (lower_idx == UINT32_MAX && higher_idx == UINT32_MAX)
				return;

			if (lower_idx == UINT32_MAX)
			{
				// sample_clip higher_idx at full
			}
			else if (higher_idx == UINT32_MAX)
			{
				// sample_clip lower_idx at full
			}
			else
			{
				const float mult		 = 1.0f / (lower_diff + higher_diff);
				const float higher_blend = higher_diff * mult;
				// sample both, blend higher in 1.0f - higher_blend
			}
		}
		else if (state.clip_count > 3)
		{
			const vec2f_t& blend_pos = state.blend_position_value;

			const animation_library_state_delaunay_triangle_t* tris = _aux.get<animation_library_state_delaunay_triangle_t>(state.delaunay_triangles);

			for (u32 i = 0; i < state.triangle_count; i++)
			{
				const animation_library_state_delaunay_triangle_t& tri	   = tris[i];
				const vec2f_t									   offset  = blend_pos - tri.v0;
				const f32										   weight1 = tri.coeff1.x * offset.x + tri.coeff1.y * offset.y;
				const f32										   weight2 = tri.coeff2.x * offset.x + tri.coeff2.y * offset.y;
				const f32										   weight0 = 1.0f - weight1 - weight2;

				const float eps = 0.0001f;
				if (weight0 > eps && weight1 > eps && weight2 > eps)
				{
					// sample tri clips & blend.
					break;
				}
			}
		}
	}

	void animation_processor_t::blend_poses(const animator_pose_t& pose, animator_pose_t& target, f32 weight)
	{
	}
}
