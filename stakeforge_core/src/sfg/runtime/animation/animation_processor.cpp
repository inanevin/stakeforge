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
#include <sfg/math/triangulation_2d.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/animation_library.hpp>
#include <sfg/runtime/resources/skeleton.hpp>
#include <sfg/runtime/resources/animation.hpp>
#include <sfg/runtime/animation/animation_sampler.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

namespace sfg
{
	void animation_processor_t::init(world_t& world, size_t animation_processor_page_size)
	{
		_world					  = &world;
		_resource_reload_listener = resource_manager_t::get().add_reload_listener(on_resource_reload, this);

		_aux.init(32 * 1024);

		_bone_aux.init(animation_processor_page_size);
		_decomposition_aux.init(animation_processor_page_size);

		_states.reserve(512);
		_libraries.reserve(64);

		_frame_counter = 0;
	}

	void animation_processor_t::uninit()
	{
		resource_manager_t::get().remove_reload_listener(_resource_reload_listener);
		_resource_reload_listener = {};

		_states.clear();
		_libraries.clear();
		_decomposition_aux.uninit();
		_bone_aux.uninit();
		_aux.uninit();

		_world = nullptr;
	}

	void animation_processor_t::clear()
	{
		_states.clear();
		_libraries.clear();
		_decomposition_aux.reset();
		_bone_aux.reset();
		_aux.reset();

		_frame_counter = 0;
	}

	void animation_processor_t::destroy_entity(entity_id_t id)
	{
		const ecs_component_table_t& system_table		  = _world->get_component_table<component_system_animation_library_t>();
		const ecs_component_table_t& system_skinned_table = _world->get_component_table<component_system_skinned_mesh_renderer_t>();
		const ecs_component_table_t& system_player_table  = _world->get_component_table<component_system_animation_player_t>();

		if (system_player_table.has(id))
			dealloc_for_player(id);

		if (system_table.has(id))
			dealloc_for_entity(id);

		if (system_skinned_table.has(id))
			dealloc_for_skinned_renderer(id);
	}

	void animation_processor_t::tick(f32 dt)
	{
		const ecs_component_table_t& table_lib		   = _world->get_component_table(type_id_t<component_animation_library_t>::value);
		const ecs_component_table_t& table_sys_lib	   = _world->get_component_table(type_id_t<component_system_animation_library_t>::value);
		const ecs_component_table_t& table_disabled	   = _world->get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& table_skinned	   = _world->get_component_table<component_skinned_mesh_renderer_t>();
		const ecs_component_table_t& table_sys_skinned = _world->get_component_table<component_system_skinned_mesh_renderer_t>();

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

		// update renderer storage before sampling animations
		{
			frame_vector_t<entity_id_t> add_sys_entities	= {};
			frame_vector_t<entity_id_t> remove_sys_entities = {};

			{
				ecs_component_table_ref_t refs[] = {table_skinned.ref(), !table_sys_skinned.ref(), !table_disabled.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					add_sys_entities.push_back(row.id);
			}

			{
				ecs_component_table_ref_t refs[] = {!table_skinned.ref(), table_sys_skinned.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
					remove_sys_entities.push_back(row.id);
			}

			{
				ecs_component_table_ref_t refs[] = {table_skinned.ref(), table_sys_skinned.ref(), !table_disabled.ref()};

				for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				{
					const component_skinned_mesh_renderer_t&		skinned		= row.get<component_skinned_mesh_renderer_t>();
					const component_system_skinned_mesh_renderer_t& sys_skinned = row.get<component_system_skinned_mesh_renderer_t>();

					if (skinned.skeleton == sys_skinned.skeleton)
						continue;

					remove_sys_entities.push_back(row.id);
					add_sys_entities.push_back(row.id);
				}
			}

			for (entity_id_t id : remove_sys_entities)
				dealloc_for_skinned_renderer(id);

			for (entity_id_t id : add_sys_entities)
				alloc_for_skinned_renderer(id);
		}

		tick_players(dt);

		const entity_id_t ent_main_camera = _world->get_main_camera_entity();

		const vec3f_t cam_pos = ent_main_camera == NULL_ENTITY_ID ? vec3f_t::zero : _world->get_entity_pos_abs(ent_main_camera);
		const vec3f_t cam_fw  = ent_main_camera == NULL_ENTITY_ID ? vec3f_t::zero : _world->get_entity_rot_abs(ent_main_camera).get_forward().normalized();

		ecs_component_table_ref_t refs[] = {table_lib.ref(), table_sys_lib.ref(), !table_disabled.ref()};

		auto is_culled = [&](const vec3f_t& pos, f32 angle_degrees) -> bool {
			const vec3f_t cam_to_pos = (pos - cam_pos).normalized();
			const float	  dot		 = vec3f_t::dot(cam_to_pos, cam_fw);
			return dot < math::cos(angle_degrees * DEG_2_RAD);
		};

		auto is_throttled = [&](const vec3f_t& pos, f32 begin, f32 full, u32 max_throttle) -> bool {
			begin = math::max(begin, 0.0f);
			full  = math::max(begin, full);

			const f32 dist_sqr = (pos - cam_pos).magnitude_sqr();
			const f32 mapped   = begin == full ? (dist_sqr > begin * begin ? 1.0f : 0.0f) : math::clamp(math::remap(dist_sqr, begin * begin, full * full, 0.0f, 1.0f), 0.0f, 1.0f);
			const u32 blanks   = static_cast<u32>(math::lerp(1.0f, static_cast<f32>(math::max(max_throttle, 1u)), mapped));
			return _frame_counter % blanks != 0;
		};

		frame_vector_t<decomposed_bone_t> temp_decomposed  = {};
		frame_vector_t<decomposed_bone_t> temp2_decomposed = {};
		frame_vector_t<decomposed_bone_t> clip_decomposed  = {};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_animation_library_t&  lib	  = row.get<component_animation_library_t>();
			component_system_animation_library_t& sys_lib = row.get_mutable<component_system_animation_library_t>();
			const vec3f_t						  pos	  = _world->get_entity_pos_abs(row.id);
			const bool skip_sample = ent_main_camera == NULL_ENTITY_ID || (lib.use_cull && is_culled(pos, lib.cull_angle_limit)) || lib.use_throttle && is_throttled(pos, lib.throttle_begin_distance, lib.throttle_full_distance, lib.max_throttle_tick_blanks) ||
									 (!lib.use_throttle && lib.tick_blanks > 1 && _frame_counter % lib.tick_blanks != 0);

			const u32 jc			  = sys_lib.joint_count;
			sys_lib.sample_this_frame = !skip_sample;

			animator_library_t& anim_lib = _libraries.get({sys_lib.lib_alloc.index, sys_lib.lib_alloc.generation});

			decomposed_bone_t* const decomposed = _decomposition_aux.get<decomposed_bone_t>(sys_lib.decompose_alloc);

			temp_decomposed.resize(jc);
			temp2_decomposed.resize(jc);
			clip_decomposed.resize(jc);

			skeleton_mask_t src_position = {};
			skeleton_mask_t src_rotation = {};
			skeleton_mask_t src_scale	 = {};

			if (!anim_lib.layers[0].active_state.is_null())
			{
				animator_layer_t& layer	 = anim_lib.layers[0];
				animator_state_t& active = _states.get(layer.active_state);

				// no switch, write directly to memory, first layer full weight.
				if (!layer.current_switch.active)
				{
					process_state({
						.state			   = active,
						.decomposed		   = decomposed,
						.scratch		   = clip_decomposed.data(),
						.mask			   = layer.mask,
						.out_position_mask = src_position,
						.out_rotation_mask = src_rotation,
						.out_scale_mask	   = src_scale,
						.joint_count	   = jc,
						.dt				   = dt,
						.sample_animation  = !skip_sample,
					});
				}
				else
				{
					skeleton_mask_t transition_position = {};
					skeleton_mask_t transition_rotation = {};
					skeleton_mask_t transition_scale	= {};

					// if skipping sample & only progressing time, won't be writing to memory just call process
					// we write to memory as we sample, first layer write directly
					process_state({
						.state			   = active,
						.decomposed		   = decomposed,
						.scratch		   = clip_decomposed.data(),
						.mask			   = layer.mask,
						.out_position_mask = src_position,
						.out_rotation_mask = src_rotation,
						.out_scale_mask	   = src_scale,
						.joint_count	   = jc,
						.dt				   = dt,
						.sample_animation  = !skip_sample,
					});

					if (!skip_sample)
						SFG_MEMCPY(temp_decomposed.data(), decomposed, sizeof(decomposed_bone_t) * jc);

					// write the transition to temp memory
					process_state({
						.state			   = _states.get(layer.current_switch.target_state),
						.decomposed		   = temp_decomposed.data(),
						.scratch		   = clip_decomposed.data(),
						.mask			   = layer.mask,
						.out_position_mask = transition_position,
						.out_rotation_mask = transition_rotation,
						.out_scale_mask	   = transition_scale,
						.joint_count	   = jc,
						.dt				   = dt,
						.sample_animation  = !skip_sample,
					});

					if (!skip_sample)
					{
						// blend
						blend_decomposed({
							.store					= decomposed,
							.target					= temp_decomposed.data(),
							.target_position_writes = transition_position,
							.target_rotation_writes = transition_rotation,
							.target_scale_writes	= transition_scale,
							.joint_count			= jc,
							.blend					= math::min(layer.current_switch.current_time / layer.current_switch.duration, 1.0f),
						});
					}

					layer.current_switch.current_time += dt;
				}
			}

			for (u32 i = 1; i < anim_lib.layer_count; i++)
			{
				animator_layer_t& layer = anim_lib.layers[i];

				if (layer.active_state.is_null())
					continue;

				animator_state_t& active = _states.get(layer.active_state);
				const bool		  sample = !skip_sample && !math::almost_equal(layer.weight, 0.0f);

				if (sample)
					SFG_MEMCPY(temp_decomposed.data(), decomposed, sizeof(decomposed_bone_t) * jc);

				skeleton_mask_t out_position = {};
				skeleton_mask_t out_rotation = {};
				skeleton_mask_t out_scale	 = {};

				// no active switch, write to demp decomposed if sampling & blend to original source.
				// sample into temporaries.
				process_state({
					.state			   = active,
					.decomposed		   = temp_decomposed.data(),
					.scratch		   = clip_decomposed.data(),
					.mask			   = layer.mask,
					.out_position_mask = out_position,
					.out_rotation_mask = out_rotation,
					.out_scale_mask	   = out_scale,
					.joint_count	   = jc,
					.dt				   = dt,
					.sample_animation  = sample,
				});

				if (layer.current_switch.active)
				{
					skeleton_mask_t transition_position = {};
					skeleton_mask_t transition_rotation = {};
					skeleton_mask_t transition_scale	= {};

					if (sample)
						SFG_MEMCPY(temp2_decomposed.data(), temp_decomposed.data(), sizeof(decomposed_bone_t) * jc);

					process_state({
						.state			   = _states.get(layer.current_switch.target_state),
						.decomposed		   = temp2_decomposed.data(),
						.scratch		   = clip_decomposed.data(),
						.mask			   = layer.mask,
						.out_position_mask = transition_position,
						.out_rotation_mask = transition_rotation,
						.out_scale_mask	   = transition_scale,
						.joint_count	   = jc,
						.dt				   = dt,
						.sample_animation  = sample,
					});

					const f32 transition_blend = math::min(layer.current_switch.current_time / layer.current_switch.duration, 1.0f);

					if (sample && transition_blend > 0.0f)
					{
						// blend temporaries
						blend_decomposed({
							.store					= temp_decomposed.data(),
							.target					= temp2_decomposed.data(),
							.target_position_writes = transition_position,
							.target_rotation_writes = transition_rotation,
							.target_scale_writes	= transition_scale,
							.joint_count			= jc,
							.blend					= transition_blend,
						});

						out_position |= transition_position;
						out_rotation |= transition_rotation;
						out_scale |= transition_scale;
					}

					layer.current_switch.current_time += dt;
				}

				if (sample)
				{
					// blend to source
					blend_decomposed({
						.store					= decomposed,
						.target					= temp_decomposed.data(),
						.target_position_writes = out_position,
						.target_rotation_writes = out_rotation,
						.target_scale_writes	= out_scale,
						.joint_count			= jc,
						.blend					= layer.weight,
					});
				}
			}

			for (u32 i = 0; i < anim_lib.layer_count; i++)
			{
				animator_layer_t& layer = anim_lib.layers[i];

				if (layer.current_switch.active && layer.current_switch.current_time >= layer.current_switch.duration)
				{
					reset_state(_states.get(layer.active_state));
					layer.active_state	 = layer.current_switch.target_state;
					layer.current_switch = {};
				}
			}
		}

		_frame_counter++;
	}

	void animation_processor_t::calculate_skinning_matrices(f32 dt)
	{
		const ecs_component_table_t& table_lib		   = _world->get_component_table<component_animation_library_t>();
		const ecs_component_table_t& table_sys_lib	   = _world->get_component_table(type_id_t<component_system_animation_library_t>::value);
		const ecs_component_table_t& table_disabled	   = _world->get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& table_sys_skinned = _world->get_component_table<component_system_skinned_mesh_renderer_t>();

		ecs_component_table_ref_t refs[] = {table_sys_lib.ref(), !table_disabled.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_system_animation_library_t& sys_lib = row.get<component_system_animation_library_t>();

			if (!sys_lib.sample_this_frame)
				continue;

			const decomposed_bone_t* decomposed = _decomposition_aux.get<decomposed_bone_t>(sys_lib.decompose_alloc);
			mat4x3_t*				 matrices	= _bone_aux.get<mat4x3_t>(sys_lib.bone_alloc);
			const u32				 jc			= sys_lib.joint_count;

			const u32* eval_order	  = _bone_aux.get<u32>(sys_lib.evaluation_order);
			const u32* parent_indices = _bone_aux.get<u32>(sys_lib.parent_indices);

			for (u32 i = 0; i < jc; ++i)
			{
				const u32 joint_index  = eval_order[i];
				const u32 parent_index = parent_indices[joint_index];

				const mat4x3_t& local = mat4x3_t::transform(decomposed[joint_index].position, decomposed[joint_index].rotation, decomposed[joint_index].scale);

				if (parent_index != SKELETON_JOINT_NO_PARENT)
					matrices[1 + jc + joint_index] = matrices[1 + jc + parent_index] * local;
				else
					matrices[1 + jc + joint_index] = local;
			}
		}

		ecs_component_table_ref_t skinned_refs[] = {table_lib.ref(), table_sys_lib.ref(), table_sys_skinned.ref(), !table_disabled.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = skinned_refs, .size = std::size(skinned_refs)}))
		{
			const bool									in_place	= row.get<component_animation_library_t>().in_place;
			const component_system_animation_library_t& sys_lib		= row.get<component_system_animation_library_t>();
			component_system_skinned_mesh_renderer_t&	sys_skinned = row.get_mutable<component_system_skinned_mesh_renderer_t>();
			const animator_library_t&					anim_lib	= _libraries.get({sys_lib.lib_alloc.index, sys_lib.lib_alloc.generation});

			if (anim_lib.skeleton_handle != sys_skinned.skeleton)
				continue;

			// if one adds skinned renderer while anim library is in-between samples,
			// visuals will stay in t-pose until anim library samples. needs_skinning prevents that.
			// idk very rare case scenario for a small branch cost, at least its easily predictable.
			if (!sys_lib.sample_this_frame && !sys_skinned.needs_skinning && sys_skinned.in_place == in_place)
				continue;

			const mat4x3_t* matrices	   = _bone_aux.get<mat4x3_t>(sys_lib.bone_alloc);
			mat4x3_t*		final_matrices = _bone_aux.get<mat4x3_t>(sys_skinned.bone_alloc);
			mat4x3_t		skinning	   = matrices[0];
			const u32		jc			   = sys_lib.joint_count;

			if (in_place)
			{
				const vec3f_t position = skinning * matrices[1 + jc + sys_skinned.motion_joint_index].get_translation();

				// x & z comps
				skinning[9] += sys_skinned.motion_reference_position.x - position.x;
				skinning[11] += sys_skinned.motion_reference_position.y - position.z;
			}

			for (u32 i = 0; i < jc; i++)
			{
				final_matrices[i] = skinning * matrices[1 + jc + i] * matrices[1 + i]; // skinning * skel space abs * inv_binds
			}

			sys_skinned.needs_skinning = false;
			sys_skinned.in_place	   = in_place;
		}

		const ecs_component_table_t& table_player	  = _world->get_component_table<component_animation_player_t>();
		const ecs_component_table_t& table_sys_player = _world->get_component_table<component_system_animation_player_t>();
		ecs_component_table_ref_t	 player_refs[]	  = {table_player.ref(), table_sys_player.ref(), table_sys_skinned.ref(), !table_lib.ref(), !table_disabled.ref()};
		resource_manager_t&			 rm				  = resource_manager_t::get();

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = player_refs, .size = std::size(player_refs)}))
		{
			const bool								   in_place = row.get<component_animation_player_t>().in_place;
			const component_system_animation_player_t& player	= row.get<component_system_animation_player_t>();
			component_system_skinned_mesh_renderer_t&  renderer = row.get_mutable<component_system_skinned_mesh_renderer_t>();

			if ((!renderer.needs_skinning && renderer.in_place == in_place) || player.last_sample_time < 0.0f)
				continue;

			const skeleton_runtime_t&		skeleton   = *rm.find_runtime<skeleton_runtime_t>(player.skeleton);
			const skeleton_joint_runtime_t* joints	   = rm.get_memory().get<skeleton_joint_runtime_t>(skeleton.joints);
			const u32*						eval_order = rm.get_memory().get<u32>(skeleton.evaluation_order);
			const decomposed_bone_t*		decomposed = _decomposition_aux.get<decomposed_bone_t>(player.decompose_alloc);
			mat4x3_t*						matrices   = _bone_aux.get<mat4x3_t>(renderer.bone_alloc);

			for (u32 i = 0; i < skeleton.joint_count; ++i)
			{
				const u32				 joint_index  = eval_order[i];
				const u32				 parent_index = joints[joint_index].parent_index;
				const decomposed_bone_t& bone		  = decomposed[joint_index];
				const mat4x3_t			 local		  = mat4x3_t::transform(bone.position, bone.rotation, bone.scale);

				matrices[joint_index] = parent_index == SKELETON_JOINT_NO_PARENT ? local : matrices[parent_index] * local;
			}

			mat4x3_t skinning = skeleton.skinning_transform;

			if (in_place)
			{
				const vec3f_t position = skinning * matrices[renderer.motion_joint_index].get_translation();

				skinning[9] += renderer.motion_reference_position.x - position.x;
				skinning[11] += renderer.motion_reference_position.y - position.z;
			}

			for (u32 i = 0; i < skeleton.joint_count; ++i)
				matrices[i] = skinning * matrices[i] * joints[i].inverse_bind;

			renderer.needs_skinning = false;
			renderer.in_place		= in_place;
		}
	}

	void animation_processor_t::switch_layer_state(animator_library_handle_t library, u32 layer_index, animator_state_handle_t state, f32 transition_duration)
	{
		if (!_libraries.is_valid(library))
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

		animator_layer_t& layer = lib.layers[layer_index];

		if (layer.current_switch.active)
		{
			reset_state(_states.get(layer.current_switch.target_state));
		}

		if (math::almost_equal(transition_duration, 0.0f))
		{
			reset_state(_states.get(layer.active_state));

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
		if (!_libraries.is_valid(library))
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

			animator_layer_t& layer = lib.layers[layer_index];

			const animator_state_handle_t* states = layer.state_count == 0 ? nullptr : _aux.get<animator_state_handle_t>(layer.state_handles);

			for (u32 j = 0; j < layer.state_count; j++)
			{
				const animator_state_t& state = _states.get(states[j]);

				if (state.name_hash == name_hash)
					return states[j];
			}

			return {};
		}

		for (u32 i = 0; i < lib.layer_count; i++)
		{
			animator_layer_t& layer = lib.layers[i];

			const animator_state_handle_t* states = layer.state_count == 0 ? nullptr : _aux.get<animator_state_handle_t>(layer.state_handles);

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

	void animation_processor_t::on_resource_reload(resource_manager_t& resource_manager, sid_t resource_id, resource_type_e resource_type, void* user_data)
	{
		if (resource_type != resource_type_e::animation_library && resource_type != resource_type_e::animation && resource_type != resource_type_e::skeleton)
			return;

		animation_processor_t&		 processor = *static_cast<animation_processor_t*>(user_data);
		frame_vector_t<entity_id_t>	 entities  = {};
		const ecs_component_table_t& libraries = processor._world->get_component_table<component_system_animation_library_t>();
		const ecs_component_table_t& players   = processor._world->get_component_table<component_system_animation_player_t>();
		const ecs_component_table_t& renderers = processor._world->get_component_table<component_system_skinned_mesh_renderer_t>();

		{
			ecs_component_table_ref_t refs[] = {libraries.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				const component_system_animation_library_t& sys		 = row.get<component_system_animation_library_t>();
				const animator_library_t&					library	 = processor._libraries.get({sys.lib_alloc.index, sys.lib_alloc.generation});
				bool										affected = resource_type == resource_type_e::animation_library && sys.animation_library == resource_id;

				affected |= resource_type == resource_type_e::skeleton && library.skeleton_handle == resource_id;

				if (resource_type == resource_type_e::animation)
				{
					for (u32 i = 0; i < library.layer_count && !affected; ++i)
					{
						const animator_layer_t&		   layer  = library.layers[i];
						const animator_state_handle_t* states = layer.state_count == 0 ? nullptr : processor._aux.get<animator_state_handle_t>(layer.state_handles);

						for (u32 j = 0; j < layer.state_count && !affected; ++j)
						{
							const animator_state_t& state = processor._states.get(states[j]);

							for (u32 k = 0; k < state.clip_count; ++k)
								affected |= state.clips[k].clip_handle == resource_id;
						}
					}
				}

				if (affected)
					entities.push_back(row.id);
			}
		}

		{
			ecs_component_table_ref_t refs[] = {players.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				const component_system_animation_player_t& player = row.get<component_system_animation_player_t>();

				if ((resource_type == resource_type_e::animation && player.animation == resource_id) || (resource_type == resource_type_e::skeleton && player.skeleton == resource_id))
					entities.push_back(row.id);
			}
		}

		if (resource_type == resource_type_e::skeleton)
		{
			ecs_component_table_ref_t refs[] = {renderers.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				if (row.get<component_system_skinned_mesh_renderer_t>().skeleton == resource_id)
					entities.push_back(row.id);
			}
		}

		std::sort(entities.begin(), entities.end());
		const auto end = std::unique(entities.begin(), entities.end());

		for (auto it = entities.begin(); it != end; ++it)
			processor.destroy_entity(*it);
	}

	void animation_processor_t::alloc_for_entity(entity_id_t id)
	{
		const component_animation_library_t& comp_lib = _world->get_component_table<component_animation_library_t>().get_as<component_animation_library_t>(id);
		resource_manager_t&					 rm		  = resource_manager_t::get();
		const animation_library_runtime_t*	 res_lib  = rm.find_runtime<animation_library_runtime_t>(comp_lib.animation_library);

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

		component_system_animation_library_t& sys = _world->get_component_table<component_system_animation_library_t>().add_or_get_as<component_system_animation_library_t>(id);

		const animator_library_handle_t handle = _libraries.add();
		sys.lib_alloc						   = {handle.index, handle.generation};
		sys.animation_library				   = comp_lib.animation_library;

		animator_library_t& anim_lib = _libraries.get(handle);

		const u32 skeleton_joint_count = res_skeleton->joint_count;
		sys.bone_alloc				   = _bone_aux.allocate<mat4x3_t>(get_count_for_lib_alloc(skeleton_joint_count));
		sys.decompose_alloc			   = _decomposition_aux.allocate<decomposed_bone_t>(skeleton_joint_count);
		sys.joint_count				   = skeleton_joint_count;
		sys.evaluation_order		   = _bone_aux.allocate<u32>(skeleton_joint_count);
		sys.parent_indices			   = _bone_aux.allocate<u32>(skeleton_joint_count);

		u32* const				 eval_order		= _bone_aux.get<u32>(sys.evaluation_order);
		u32* const				 parent_indices = _bone_aux.get<u32>(sys.parent_indices);
		const u32*				 res_eval		= rm.get_memory().get<u32>(res_skeleton->evaluation_order);
		mat4x3_t* const			 matrices		= _bone_aux.get<mat4x3_t>(sys.bone_alloc);
		decomposed_bone_t* const decomposed		= _decomposition_aux.get<decomposed_bone_t>(sys.decompose_alloc);

		matrices[0] = res_skeleton->skinning_transform;

		const skeleton_joint_runtime_t* joints = rm.get_memory().get<skeleton_joint_runtime_t>(res_skeleton->joints);

		// write joint space abs transforms to last storage slot.
		for (u32 i = 0; i < skeleton_joint_count; ++i)
		{
			eval_order[i]	  = res_eval[i];
			parent_indices[i] = joints[i].parent_index;

			const u32 joint_index  = eval_order[i];
			const u32 parent_index = joints[joint_index].parent_index;

			if (parent_index != SKELETON_JOINT_NO_PARENT)
				matrices[1 + skeleton_joint_count + joint_index] = matrices[1 + skeleton_joint_count + parent_index] * joints[joint_index].local;
			else
				matrices[1 + skeleton_joint_count + joint_index] = joints[joint_index].local;
		}

		for (u32 i = 0; i < skeleton_joint_count; i++)
		{
			const mat4x3_t& local = joints[i].local;

			// first slot is inverse binds
			matrices[1 + i] = joints[i].inverse_bind;

			// second slot holds joint space abs transforms

			decomposed_bone_t& decomp = decomposed[i];
			local.decompose(decomp.position, decomp.rotation, decomp.scale);
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
					state.delaunay_triangles = _aux.allocate<animation_library_state_delaunay_triangle_t>(res_state.triangle_count);
					state.triangle_count	 = res_state.triangle_count;

					animation_library_state_delaunay_triangle_t*	   state_tris = _aux.get<animation_library_state_delaunay_triangle_t>(state.delaunay_triangles);
					const animation_library_state_delaunay_triangle_t* tris		  = rm.get_memory().get<animation_library_state_delaunay_triangle_t>(res_state.delaunay_triangles);

					for (u32 t = 0; t < res_state.triangle_count; t++)
					{
						state_tris[t] = tris[t];
					}
				}

				for (u32 k = 0; k < res_state.clip_count; k++)
				{
					const animation_library_clip_runtime_t& res_clip = res_state.clips[k];
					const animation_runtime_t*				anim	 = rm.find_runtime<animation_runtime_t>(res_clip.animation_clip);

					state.clips[k].blend_position = res_clip.weight_value;
					state.clips[k].clip_handle	  = res_clip.animation_clip;
					state.clips[k].speed		  = res_clip.playback_speed;
					state.clips[k].start_time	  = res_clip.start_time;
				}

				_aux.get<animator_state_handle_t>(layer.state_handles)[j] = state_handle;

				if (j == res_layer.default_active_state)
					layer.active_state = state_handle;
				else if (j == 0 && res_layer.default_active_state == UINT32_MAX)
					layer.active_state = state_handle;
			}
		}
	}

	void animation_processor_t::dealloc_for_entity(entity_id_t id)
	{
		component_system_animation_library_t& sys = _world->get_component_table<component_system_animation_library_t>().get_as<component_system_animation_library_t>(id);

		const animator_library_handle_t lib_handle = {sys.lib_alloc.index, sys.lib_alloc.generation};
		const animator_library_t&		anim_lib   = _libraries.get(lib_handle);

		if (sys.bone_alloc)
			_bone_aux.free(sys.bone_alloc);

		if (sys.decompose_alloc)
			_decomposition_aux.free(sys.decompose_alloc);

		if (sys.parent_indices)
			_bone_aux.free(sys.parent_indices);

		if (sys.evaluation_order)
			_bone_aux.free(sys.evaluation_order);

		sys = {};
		_world->get_component_table<component_system_animation_library_t>().remove(id);

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

			if (layer.state_handles)
				_aux.free(layer.state_handles);
		}

		_libraries.remove(lib_handle);
	}

	void animation_processor_t::alloc_for_skinned_renderer(entity_id_t id)
	{
		const component_skinned_mesh_renderer_t& skinned  = _world->get_component_table<component_skinned_mesh_renderer_t>().get_as<component_skinned_mesh_renderer_t>(id);
		resource_manager_t&						 rm		  = resource_manager_t::get();
		const skeleton_runtime_t*				 skeleton = rm.find_runtime<skeleton_runtime_t>(skinned.skeleton);

		if (skeleton == nullptr)
		{
			SFG_WARN("entity has skinned mesh renderer referencing a missing skeleton! {0}", id);
			return;
		}

		const chunk_handle32_t					  bone_alloc = _bone_aux.allocate<mat4x3_t>(skeleton->joint_count);
		component_system_skinned_mesh_renderer_t& sys		 = _world->get_component_table<component_system_skinned_mesh_renderer_t>().add_or_get_as<component_system_skinned_mesh_renderer_t>(id);

		sys = {.skeleton = skinned.skeleton, .bone_alloc = bone_alloc, .needs_skinning = true};

		reset_skinned_renderer_pose(sys, *skeleton);
	}

	void animation_processor_t::reset_skinned_renderer_pose(component_system_skinned_mesh_renderer_t& renderer, const skeleton_runtime_t& skeleton)
	{
		const chunk_allocator_t&		resource_memory = resource_manager_t::get().get_memory();
		mat4x3_t*						matrices		= _bone_aux.get<mat4x3_t>(renderer.bone_alloc);
		const skeleton_joint_runtime_t* joints			= resource_memory.get<skeleton_joint_runtime_t>(skeleton.joints);
		const u32*						eval_order		= resource_memory.get<u32>(skeleton.evaluation_order);

		for (u32 i = 0; i < skeleton.joint_count; ++i)
		{
			const u32 joint_index  = eval_order[i];
			const u32 parent_index = joints[joint_index].parent_index;

			matrices[joint_index] = parent_index == SKELETON_JOINT_NO_PARENT ? joints[joint_index].local : matrices[parent_index] * joints[joint_index].local;
		}

		renderer.motion_joint_index = skeleton.motion_joint_index == SKELETON_JOINT_NO_PARENT ? skeleton.root_joint_index : skeleton.motion_joint_index;

		const vec3f_t motion_position = skeleton.skinning_transform * matrices[renderer.motion_joint_index].get_translation();

		renderer.motion_reference_position = {motion_position.x, motion_position.z};
		renderer.in_place				   = false;

		for (u32 i = 0; i < skeleton.joint_count; ++i)
			matrices[i] = skeleton.skinning_transform * matrices[i] * joints[i].inverse_bind;

		renderer.needs_skinning = true;
	}

	void animation_processor_t::dealloc_for_skinned_renderer(entity_id_t id)
	{
		ecs_component_table_t&							table = _world->get_component_table<component_system_skinned_mesh_renderer_t>();
		const component_system_skinned_mesh_renderer_t& sys	  = table.get_as<component_system_skinned_mesh_renderer_t>(id);

		_bone_aux.free(sys.bone_alloc);
		table.remove(id);
	}

	void animation_processor_t::tick_players(f32 dt)
	{
		const ecs_component_table_t& table_player	   = _world->get_component_table<component_animation_player_t>();
		const ecs_component_table_t& table_sys_player  = _world->get_component_table<component_system_animation_player_t>();
		const ecs_component_table_t& table_sys_skinned = _world->get_component_table<component_system_skinned_mesh_renderer_t>();
		const ecs_component_table_t& table_lib		   = _world->get_component_table<component_animation_library_t>();
		const ecs_component_table_t& table_disabled	   = _world->get_component_table<component_disabled_t>();
		resource_manager_t&			 rm				   = resource_manager_t::get();
		frame_vector_t<entity_id_t>	 entities		   = {};

		// release players that no longer own the pose
		{
			ecs_component_table_ref_t refs[] = {table_sys_player.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			{
				const component_system_animation_player_t&		player	 = row.get<component_system_animation_player_t>();
				const component_system_skinned_mesh_renderer_t* renderer = table_sys_skinned.find_as_const<component_system_skinned_mesh_renderer_t>(row.id);

				if (!table_player.has(row.id) || renderer == nullptr || table_lib.has(row.id) || table_disabled.has(row.id) || player.skeleton != renderer->skeleton)
					entities.push_back(row.id);
			}
		}

		for (entity_id_t id : entities)
		{
			dealloc_for_player(id);

			component_system_skinned_mesh_renderer_t* renderer = table_sys_skinned.find_as<component_system_skinned_mesh_renderer_t>(id);

			if (renderer != nullptr)
			{
				const skeleton_runtime_t* skeleton = rm.find_runtime<skeleton_runtime_t>(renderer->skeleton);

				if (skeleton != nullptr)
					reset_skinned_renderer_pose(*renderer, *skeleton);
			}
		}

		entities.resize(0);

		{
			ecs_component_table_ref_t refs[] = {table_player.ref(), table_sys_skinned.ref(), !table_sys_player.ref(), !table_lib.ref(), !table_disabled.ref()};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				entities.push_back(row.id);
		}

		for (entity_id_t id : entities)
			alloc_for_player(id);

		// sample only when the pose or playback time changes
		ecs_component_table_ref_t refs[] = {table_player.ref(), table_sys_player.ref(), table_sys_skinned.ref(), !table_lib.ref(), !table_disabled.ref()};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_animation_player_t&		  player   = row.get<component_animation_player_t>();
			component_system_animation_player_t&	  sys	   = row.get_mutable<component_system_animation_player_t>();
			component_system_skinned_mesh_renderer_t& renderer = row.get_mutable<component_system_skinned_mesh_renderer_t>();
			const skeleton_runtime_t*				  skeleton = rm.find_runtime<skeleton_runtime_t>(sys.skeleton);

			if (skeleton == nullptr)
				continue;

			if (sys.animation != player.animation)
			{
				sys.animation		 = player.animation;
				sys.sample_time		 = 0.0f;
				sys.last_sample_time = -1.0f;

				reset_skinned_renderer_pose(renderer, *skeleton);
			}

			const animation_runtime_t* animation = rm.find_runtime<animation_runtime_t>(player.animation);

			if (animation == nullptr)
				continue;

			decomposed_bone_t* decomposed = _decomposition_aux.get<decomposed_bone_t>(sys.decompose_alloc);

			if (sys.last_sample_time < 0.0f || sys.mask != player.mask)
			{
				const skeleton_joint_runtime_t* joints = rm.get_memory().get<skeleton_joint_runtime_t>(skeleton->joints);

				for (u32 i = 0; i < skeleton->joint_count; ++i)
					joints[i].local.decompose(decomposed[i].position, decomposed[i].rotation, decomposed[i].scale);

				sys.mask			 = player.mask;
				sys.mask_index		 = UINT32_MAX;
				sys.last_sample_time = -1.0f;

				if (player.mask != NULL_SID && skeleton->mask_count != 0)
				{
					const skeleton_mask_runtime_t* masks = rm.get_memory().get<skeleton_mask_runtime_t>(skeleton->masks);

					for (u32 i = 0; i < skeleton->mask_count; ++i)
					{
						if (masks[i].name_hash == player.mask)
						{
							sys.mask_index = i;
							break;
						}
					}
				}
			}

			if (player.is_scrub)
				sys.sample_time = math::clamp(player.scrub_ratio, 0.0f, 1.0f) * animation->duration;
			else
			{
				sys.sample_time += dt * player.speed_multiplier;

				if (player.is_looping && animation->duration > 0.0f)
				{
					sys.sample_time = math::fmodf(sys.sample_time, animation->duration);

					if (sys.sample_time < 0.0f)
						sys.sample_time += animation->duration;
				}
				else
					sys.sample_time = math::clamp(sys.sample_time, 0.0f, animation->duration);
			}

			if (sys.sample_time == sys.last_sample_time && !renderer.needs_skinning)
				continue;

			const skeleton_mask_t  empty_mask	   = {};
			const skeleton_mask_t& mask			   = sys.mask_index == UINT32_MAX ? empty_mask : rm.get_memory().get<skeleton_mask_runtime_t>(skeleton->masks)[sys.mask_index].value;
			skeleton_mask_t		   position_writes = {};
			skeleton_mask_t		   rotation_writes = {};
			skeleton_mask_t		   scale_writes	   = {};

			animation_sampler_t::sample_animation({
				.animation		   = animation,
				.bones			   = decomposed,
				.mask			   = mask,
				.out_position_mask = position_writes,
				.out_rotation_mask = rotation_writes,
				.out_scale_mask	   = scale_writes,
				.sample_time	   = sys.sample_time,
			});

			sys.last_sample_time	= sys.sample_time;
			renderer.needs_skinning = true;
		}
	}

	void animation_processor_t::alloc_for_player(entity_id_t id)
	{
		const component_animation_player_t&				player	 = _world->get_component_table<component_animation_player_t>().get_as<component_animation_player_t>(id);
		const component_system_skinned_mesh_renderer_t& renderer = _world->get_component_table<component_system_skinned_mesh_renderer_t>().get_as<component_system_skinned_mesh_renderer_t>(id);
		resource_manager_t&								rm		 = resource_manager_t::get();
		const skeleton_runtime_t*						skeleton = rm.find_runtime<skeleton_runtime_t>(renderer.skeleton);

		if (skeleton == nullptr || rm.find_runtime<animation_runtime_t>(player.animation) == nullptr)
			return;

		const chunk_handle32_t				 decompose_alloc = _decomposition_aux.allocate<decomposed_bone_t>(skeleton->joint_count);
		component_system_animation_player_t& sys			 = _world->get_component_table<component_system_animation_player_t>().add_or_get_as<component_system_animation_player_t>(id);

		sys = {.animation = player.animation, .skeleton = renderer.skeleton, .decompose_alloc = decompose_alloc};
	}

	void animation_processor_t::dealloc_for_player(entity_id_t id)
	{
		ecs_component_table_t&					   table = _world->get_component_table<component_system_animation_player_t>();
		const component_system_animation_player_t& sys	 = table.get_as<component_system_animation_player_t>(id);

		_decomposition_aux.free(sys.decompose_alloc);
		table.remove(id);
	}

	void animation_processor_t::process_state(const process_state_params_t& params)
	{
		animator_state_t& state = params.state;

		if (state.clip_count == 0)
			return;

		// state processing accesses resource memory for sampling
		// TODO: think of an alternative more local memory access.
		u32 clip_indices[3] = {};
		f32 weights[3]		= {};
		u32 clip_count		= 0;

		if (state.blend_type == animation_library_blend_type_e::no_blend || state.clip_count == 1)
		{
			weights[0] = 1.0f;
			clip_count = 1;
		}
		else if (state.blend_type == animation_library_blend_type_e::blend_1d)
		{
			const f32 target	 = state.blend_position_value.x;
			u32		  lower_idx	 = UINT32_MAX;
			u32		  higher_idx = UINT32_MAX;
			f32		  lower_val	 = -10.0f;
			f32		  higher_val = 10.0f;

			for (u32 i = 0; i < state.clip_count; i++)
			{
				const f32 position = state.clips[i].blend_position.x;

				if (position <= target && position > lower_val)
				{
					lower_val = position;
					lower_idx = i;
				}

				if (position >= target && position < higher_val)
				{
					higher_val = position;
					higher_idx = i;
				}
			}

			if (lower_idx == UINT32_MAX && higher_idx == UINT32_MAX)
				return;

			clip_count = 1;
			weights[0] = 1.0f;

			if (lower_idx == UINT32_MAX)
			{
				// sample_clip higher_idx at full
				clip_indices[0] = higher_idx;
			}
			else if (higher_idx == UINT32_MAX || lower_idx == higher_idx)
			{
				// sample_clip lower_idx at full
				clip_indices[0] = lower_idx;
			}
			else
			{
				clip_indices[0] = lower_idx;
				clip_indices[1] = higher_idx;
				weights[1]		= (target - lower_val) / (higher_val - lower_val);
				weights[0]		= 1.0f - weights[1];
				clip_count		= 2;
			}
		}
		else if (state.blend_type == animation_library_blend_type_e::blend_2d && state.triangle_count == 0)
		{
			// without triangles, blend the two clips on the closest segment in the sorted chain.
			f32 closest_distance = MATH_INF_F;

			for (u32 i = 0; i + 1 < state.clip_count; ++i)
			{
				// project onto this segment, clamping to its ends and choosing the first clip if both coincide.
				const vec2f_t& a			 = state.clips[i].blend_position;
				const vec2f_t  edge			 = state.clips[i + 1].blend_position - a;
				const f32	   length_sqr	 = edge.magnitude_sqr();
				const f32	   blend		 = length_sqr > 0.0f ? math::clamp(vec2f_t::dot(state.blend_position_value - a, edge) / length_sqr, 0.0f, 1.0f) : 0.0f;
				const vec2f_t  closest_point = a + edge * blend;
				const f32	   distance		 = (state.blend_position_value - closest_point).magnitude_sqr();

				if (distance >= closest_distance)
					continue;

				closest_distance = distance;
				clip_indices[0]	 = i;
				clip_indices[1]	 = i + 1;
				weights[0]		 = 1.0f - blend;
				weights[1]		 = blend;
				clip_count		 = 2;
			}
		}
		else if (state.clip_count > 2)
		{
			const animation_library_state_delaunay_triangle_t* tris = state.triangle_count == 0 ? nullptr : _aux.get<animation_library_state_delaunay_triangle_t>(state.delaunay_triangles);

			for (u32 i = 0; i < state.triangle_count; i++)
			{
				const animation_library_state_delaunay_triangle_t& tri	   = tris[i];
				const vec2f_t									   offset  = state.blend_position_value - tri.v0;
				const f32										   weight1 = tri.coeff1.x * offset.x + tri.coeff1.y * offset.y;
				const f32										   weight2 = tri.coeff2.x * offset.x + tri.coeff2.y * offset.y;
				const f32										   weight0 = 1.0f - weight1 - weight2;
				const f32										   eps	   = -0.00001f;

				if (weight0 > eps && weight1 > eps && weight2 > eps)
				{
					clip_indices[0] = tri.clip_index0;
					clip_indices[1] = tri.clip_index1;
					clip_indices[2] = tri.clip_index2;
					weights[0]		= math::max(weight0, 0.0f);
					weights[1]		= math::max(weight1, 0.0f);
					weights[2]		= math::max(weight2, 0.0f);
					clip_count		= 3;
					break;
				}
			}

			// find closest edge
			if (clip_count == 0)
			{
				f32 closest_distance = MATH_INF_F;

				for (u32 i = 0; i < state.triangle_count; i++)
				{
					const animation_library_state_delaunay_triangle_t& tri			 = tris[i];
					const vec2f_t&									   a			 = state.clips[tri.clip_index0].blend_position;
					const vec2f_t&									   b			 = state.clips[tri.clip_index1].blend_position;
					const vec2f_t&									   c			 = state.clips[tri.clip_index2].blend_position;
					const vec3f_t									   edge_weights	 = math::closest_triangle_barycentric_2d(state.blend_position_value, a, b, c);
					const vec2f_t									   closest_point = a * edge_weights.x + b * edge_weights.y + c * edge_weights.z;
					const f32										   distance		 = (state.blend_position_value - closest_point).magnitude_sqr();

					if (distance >= closest_distance)
						continue;

					closest_distance = distance;
					clip_indices[0]	 = tri.clip_index0;
					clip_indices[1]	 = tri.clip_index1;
					clip_indices[2]	 = tri.clip_index2;
					weights[0]		 = edge_weights.x;
					weights[1]		 = edge_weights.y;
					weights[2]		 = edge_weights.z;
					clip_count		 = 3;
				}
			}
		}

		if (clip_count == 0)
			return;

		const animation_runtime_t* animations[3] = {};

		for (u32 i = 0; i < clip_count; i++)
		{
			if (weights[i] <= 0.0f)
				continue;

			animations[i] = resource_manager_t::get().find_runtime<animation_runtime_t>(state.clips[clip_indices[i]].clip_handle);

			if (animations[i] == nullptr)
				return;
		}

		f32 total_weight = 0.0f;
		f32 duration	 = 0.0f;

		for (u32 i = 0; i < clip_count; i++)
		{
			if (weights[i] <= 0.0f)
				continue;

			const animator_clip_t& clip = state.clips[clip_indices[i]];

			const f32 anim_res_duration = animations[i]->duration;

			if (!math::almost_equal(clip.speed, 0.0f))
			{
				total_weight += weights[i];
				duration += ((anim_res_duration - math::clamp(clip.start_time, 0.0f, anim_res_duration)) / clip.speed) * weights[i];
			}
		}

		if (!math::almost_equal(total_weight, 0.0f))
			duration /= total_weight;

		// keep the loop count before wrapping so large steps can cross several loops.
		const f32  previous_phase = state.current_phase;
		const f32  phase_delta	  = !math::almost_equal(duration, 0.0f) && !math::almost_equal(state.speed, 0.0f) ? params.dt / (duration / state.speed) : 0.0f;
		const f32  next_phase	  = previous_phase + phase_delta;
		const i32  loop_count	  = state.loop ? static_cast<i32>(math::floor(next_phase)) : 0;
		const bool reverse		  = phase_delta < 0.0f;

		state.current_phase = state.loop ? next_phase - static_cast<f32>(loop_count) : math::clamp(next_phase, 0.0f, 1.0f);

		// reset inactive clips so they do not replay events when their blend weight returns.
		u32 active_clips = 0;

		for (u32 i = 0; i < clip_count; ++i)
		{
			if (weights[i] > 0.0f)
				active_clips |= 1u << clip_indices[i];
		}

		for (u32 i = 0; i < state.clip_count; ++i)
		{
			if ((active_clips & (1u << i)) == 0)
				state.clips[i].last_sample = UINT32_MAX;
		}

		f32 accumulated_weight = 0.0f;

		for (u32 i = 0; i < clip_count; i++)
		{
			animator_clip_t& clip = state.clips[clip_indices[i]];

			if (weights[i] <= 0.0f)
				continue;

			const animation_runtime_t& animation	 = *animations[i];
			const f32				   start_time	 = math::clamp(clip.start_time, 0.0f, animation.duration);
			const f32				   clip_duration = animation.duration - start_time;
			const bool				   frozen		 = math::almost_equal(clip.speed, 0.0f);
			const f32				   sample_time	 = start_time + (frozen ? 0.0f : state.current_phase * clip_duration);

			// use millisecond ticks for events while keeping pose sampling in seconds.
			const u32  sample			= static_cast<u32>(math::round(sample_time * ANIMATION_TICKS_PER_SECOND));
			bool	   include_previous = clip.last_sample == UINT32_MAX;
			u32		   previous_sample	= include_previous ? static_cast<u32>(math::round((start_time + (frozen ? 0.0f : previous_phase * clip_duration)) * ANIMATION_TICKS_PER_SECOND)) : clip.last_sample;
			const bool phase_delta_zero = math::almost_equal(phase_delta, 0.0f);

			if (!frozen && !phase_delta_zero && clip_duration > 0.0f)
				clip.last_sample = sample;

			if (clip.event_callback != nullptr && animation.event_count != 0 && !frozen && !phase_delta_zero && clip_duration > 0.0f)
			{
				const u32 start_sample = static_cast<u32>(math::round(start_time * ANIMATION_TICKS_PER_SECOND));
				const u32 end_sample   = static_cast<u32>(math::round(animation.duration * ANIMATION_TICKS_PER_SECOND));

				// fire crossed events once, in playback order, including the first tick on entry.
				const auto dispatch_events = [&](u32 begin, u32 end, bool include_begin) {
					for (u32 event_index = 0; event_index < animation.event_count; ++event_index)
					{
						const animation_event_t& event = animation.events[reverse ? animation.event_count - event_index - 1 : event_index];

						const bool crossed = reverse ? event.time < begin && event.time >= end : event.time > begin && event.time <= end;

						if (crossed || (include_begin && event.time == begin))
							clip.event_callback(event.name_hash, clip.event_user_data);
					}
				};

				// split the event interval at every loop boundary.
				const u32 crossings = static_cast<u32>(reverse ? -loop_count : loop_count);

				for (u32 crossing = 0; crossing < crossings; ++crossing)
				{
					dispatch_events(previous_sample, reverse ? start_sample : end_sample, include_previous);

					previous_sample	 = reverse ? end_sample : start_sample;
					include_previous = true;
				}

				dispatch_events(previous_sample, sample, include_previous);
			}

			// events still advance when pose sampling is skipped.
			if (!params.sample_animation)
				continue;

			const bool first = math::almost_equal(accumulated_weight, 0.0f);

			accumulated_weight += weights[i];

			skeleton_mask_t position_writes = {};
			skeleton_mask_t rotation_writes = {};
			skeleton_mask_t scale_writes	= {};

			animation_sampler_t::sample_animation({
				.animation		   = animations[i],
				.bones			   = first ? params.decomposed : params.scratch,
				.mask			   = params.mask,
				.out_position_mask = position_writes,
				.out_rotation_mask = rotation_writes,
				.out_scale_mask	   = scale_writes,
				.sample_time	   = sample_time,
			});

			if (!first)
			{
				blend_decomposed({
					.store					= params.decomposed,
					.target					= params.scratch,
					.target_position_writes = position_writes,
					.target_rotation_writes = rotation_writes,
					.target_scale_writes	= scale_writes,
					.joint_count			= params.joint_count,
					.blend					= weights[i] / accumulated_weight,
				});
			}

			params.out_position_mask |= position_writes;
			params.out_rotation_mask |= rotation_writes;
			params.out_scale_mask |= scale_writes;
		}
	}

	void animation_processor_t::reset_state(animator_state_t& state)
	{
		state.current_phase = 0.0f;

		// allow starting events to fire again when this state restarts.
		for (u32 i = 0; i < state.clip_count; ++i)
			state.clips[i].last_sample = UINT32_MAX;
	}

	u32 animation_processor_t::get_count_for_lib_alloc(u32 skeleton_joint_count)
	{
		// skinning transform + inverse binds + joint space abs transforms
		return (1 + skeleton_joint_count * 2);
	}

	void animation_processor_t::blend_decomposed(const blend_decomposed_params_t& params)
	{
		if (params.blend == 0.0f)
			return;

		const bool full_weight = params.blend == 1.0f;

		for (u32 i = 0; i < params.joint_count; i++)
		{
			decomposed_bone_t&		 src	  = params.store[i];
			const decomposed_bone_t& incoming = params.target[i];

			if (params.target_position_writes.masked(i))
				src.position = full_weight ? incoming.position : vec3f_t::lerp(src.position, incoming.position, params.blend);

			if (params.target_rotation_writes.masked(i))
				src.rotation = full_weight ? incoming.rotation : quat_t::slerp(src.rotation, incoming.rotation, params.blend);

			if (params.target_scale_writes.masked(i))
				src.scale = full_weight ? incoming.scale : vec3f_t::lerp(src.scale, incoming.scale, params.blend);
		}
	}
}
