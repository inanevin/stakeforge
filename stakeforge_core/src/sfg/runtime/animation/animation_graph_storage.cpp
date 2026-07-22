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

#include "animation_graph_storage.hpp"
#include "animation_graph_util.hpp"
#include "animation_sampler.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/animation.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

namespace sfg
{

#define ANIMATION_GRAPH_NODE_MEMORY_PERCENT			  12
#define ANIMATION_GRAPH_PARAM_MEMORY_PERCENT		  6
#define ANIMATION_GRAPH_MASK_MEMORY_PERCENT			  3
#define ANIMATION_GRAPH_CLIP_MEMORY_PERCENT			  6
#define ANIMATION_GRAPH_ASM_STATE_MEMORY_PERCENT	  8
#define ANIMATION_GRAPH_ASM_TRANSITION_MEMORY_PERCENT 8
#define ANIMATION_GRAPH_POSE_MEMORY_PERCENT			  7
#define ANIMATION_GRAPH_AUX_MEMORY_PERCENT			  10

	animation_graph_storage_t::~animation_graph_storage_t() = default;

	void animation_graph_storage_t::init(u32 storage_memory)
	{
		const u32 node_memory			= storage_memory * ANIMATION_GRAPH_NODE_MEMORY_PERCENT / 100;
		const u32 param_memory			= storage_memory * ANIMATION_GRAPH_PARAM_MEMORY_PERCENT / 100;
		const u32 mask_memory			= storage_memory * ANIMATION_GRAPH_MASK_MEMORY_PERCENT / 100;
		const u32 clip_memory			= storage_memory * ANIMATION_GRAPH_CLIP_MEMORY_PERCENT / 100;
		const u32 asm_state_memory		= storage_memory * ANIMATION_GRAPH_ASM_STATE_MEMORY_PERCENT / 100;
		const u32 asm_transition_memory = storage_memory * ANIMATION_GRAPH_ASM_TRANSITION_MEMORY_PERCENT / 100;
		const u32 pose_memory			= storage_memory * ANIMATION_GRAPH_POSE_MEMORY_PERCENT / 100;
		const u32 aux_memory			= storage_memory * ANIMATION_GRAPH_AUX_MEMORY_PERCENT / 100;
		const u32 pose_bone_memory		= storage_memory - node_memory - param_memory - mask_memory - clip_memory - asm_state_memory - asm_transition_memory - pose_memory - aux_memory;

		_nodes.init(node_memory);
		_params.init(param_memory);
		_masks.init(mask_memory);
		_clips.init(clip_memory);
		_asm_states.init(asm_state_memory);
		_asm_transitions.init(asm_transition_memory);
		_poses.init(pose_memory);
		_pose_bones.init(pose_bone_memory);
		_aux.init(aux_memory);
	}

	void animation_graph_storage_t::uninit()
	{
		_aux.uninit();
		_pose_bones.uninit();
		_poses.uninit();
		_asm_transitions.uninit();
		_asm_states.uninit();
		_clips.uninit();
		_masks.uninit();
		_params.uninit();
		_nodes.uninit();
	}

	void animation_graph_storage_t::process_graph(chunk_handle32_t nodes, u32 node_count, chunk_handle32_t initial_pose, span_t<animation_bone_t> bones, f32 delta_time)
	{
		chunk_handle32_t previous_pose_handle = initial_pose;

		if (node_count != 0)
		{
			// copy previous pose into current node's pose, process the current pose.
			animation_graph_node_t* graph_nodes = _nodes.get<animation_graph_node_t>(nodes);

			for (u32 node_index = 0; node_index < node_count; ++node_index)
			{
				animation_graph_node_t& node = graph_nodes[node_index];

				animation_graph_util_t::copy_pose(previous_pose_handle, node.pose_handle, _poses, _pose_bones);

				animation_graph_pose_t&				 pose = *_poses.get<animation_graph_pose_t>(node.pose_handle);
				const span_t<animation_graph_bone_t> pose_bones{
					.data = _pose_bones.get<animation_graph_bone_t>(pose.bones),
					.size = pose.bone_count,
				};

				switch (node.type)
				{
				case animation_graph_node_type_e::asm_node:
					process_node_asm(node.node_asm, node.mask_handle, pose_bones, delta_time);
					break;
				case animation_graph_node_type_e::bone_controller:
					process_node_bone_control(node.node_bone_control, pose_bones, delta_time);
					break;
				case animation_graph_node_type_e::ik:
					process_node_ik(node.node_ik, pose_bones, delta_time);
					break;
				}

				previous_pose_handle = node.pose_handle;
			}
		}

		// copy latest pose as the final bone buffer.
		const animation_graph_pose_t& latest_pose		= *_poses.get<animation_graph_pose_t>(previous_pose_handle);
		const animation_graph_bone_t* latest_pose_bones = _pose_bones.get<animation_graph_bone_t>(latest_pose.bones);

		SFG_ASSERT(bones.size == latest_pose.bone_count);

		for (size_t bone_index = 0; bone_index < bones.size; ++bone_index)
			bones.data[bone_index].bone_transform = latest_pose_bones[bone_index].local_matrix;
	}

	void animation_graph_storage_t::process_node_asm(animation_graph_node_asm_t& node, chunk_handle32_t mask_handle, span_t<animation_graph_bone_t> pose_bones, f32 delta_time)
	{
		if (!node.first_state)
		{
			SFG_WARN("animation state machine has no first state");
			return;
		}

		if (!node._current_state)
			node._current_state = node.first_state;

		animation_graph_asm_state_t* transition_target_node	 = nullptr;
		f32							 transition_target_blend = 0.0f;

		// check if eligible transition.
		if (!node._current_transition)
		{
			for (u32 i = 0; i < node.transition_count; ++i)
			{
				const animation_graph_asm_transition_t& transition = _asm_transitions.get<animation_graph_asm_transition_t>(node.transitions)[i];

				if (transition.from_state != node._current_state)
					continue;

				const animation_graph_param_t& parameter		= *_params.get<animation_graph_param_t>(transition.parameter);
				f32							   transition_value = 0.0f;

				switch (parameter.type)
				{
				case animation_param_type_e::f32:
					transition_value = parameter.f32_value;
					break;
				case animation_param_type_e::vec3:
					transition_value = parameter.vec3_value.x;
					break;
				case animation_param_type_e::quat:
					transition_value = parameter.quat_value.x;
					break;
				case animation_param_type_e::boolean:
					transition_value = parameter.bool_value;
					break;
				}

				bool passes = false;

				switch (transition.type)
				{
				case animation_graph_asm_transition_type_e::equals:
					passes = transition_value == transition.compare_value;
					break;
				case animation_graph_asm_transition_type_e::lequals:
					passes = transition_value <= transition.compare_value;
					break;
				case animation_graph_asm_transition_type_e::gequals:
					passes = transition_value >= transition.compare_value;
					break;
				case animation_graph_asm_transition_type_e::less:
					passes = transition_value < transition.compare_value;
					break;
				case animation_graph_asm_transition_type_e::greater:
					passes = transition_value > transition.compare_value;
					break;
				}

				if (!passes)
					continue;

				node._current_transition = {
					.head = node.transitions.head + static_cast<u32>(sizeof(animation_graph_asm_transition_t)) * i,
					.size = static_cast<u32>(sizeof(animation_graph_asm_transition_t)),
				};
				node._current_transition_time = 0.0f;
				break;
			}
		}

		// if active transition, bump the time & switch state or set blended.
		if (node._current_transition)
		{
			const animation_graph_asm_transition_t& transition = *_asm_transitions.get<animation_graph_asm_transition_t>(node._current_transition);
			node._current_transition_time += delta_time;

			if (node._current_transition_time > transition.duration)
			{
				node._current_transition	  = {};
				node._current_transition_time = 0.0f;
				node._current_state			  = transition.to_state;
			}
			else if (transition.is_blended)
			{
				transition_target_node	= _asm_states.get<animation_graph_asm_state_t>(transition.to_state);
				transition_target_blend = node._current_transition_time / transition.duration;
			}
		}

		// process current state time
		animation_graph_asm_state_t& current_state = *_asm_states.get<animation_graph_asm_state_t>(node._current_state);
		animation_graph_util_t::advance_asm_state_time(current_state, delta_time);

		// no transition, process & write to current pose.
		if (transition_target_node == nullptr)
		{
			process_asm_state(current_state, mask_handle, current_state._current_time, pose_bones);
			return;
		}

		// we have transition, we will be sampling from target node, bump time.
		animation_graph_util_t::advance_asm_state_time(*transition_target_node, delta_time);

		// copy the current pose to target before we process.
		frame_vector_t<animation_graph_bone_t> transition_target_bones(pose_bones.size);
		const span_t<animation_graph_bone_t>   transition_target_pose{
			.data = transition_target_bones.data(),
			.size = transition_target_bones.size(),
		};

		SFG_MEMCPY(transition_target_pose.data, pose_bones.data, sizeof(animation_graph_bone_t) * pose_bones.size);

		// process current state normally, we write to current pose.
		process_asm_state(current_state, mask_handle, current_state._current_time, pose_bones);

		// process transition state, we write to temporary transition pose.
		process_asm_state(*transition_target_node, mask_handle, transition_target_node->_current_time, transition_target_pose);

		// blend the transition target into current pose
		for (size_t bone_index = 0; bone_index < pose_bones.size; ++bone_index)
		{
			animation_graph_bone_t&		  final_bone			 = pose_bones.data[bone_index];
			const animation_graph_bone_t& transition_target_bone = transition_target_pose.data[bone_index];

			vec3f_t final_position = vec3f_t::zero;
			quat_t	final_rotation = quat_t::identity;
			vec3f_t final_scale	   = vec3f_t::one;

			final_bone.local_matrix.decompose(final_position, final_rotation, final_scale);

			vec3f_t target_position = vec3f_t::zero;
			quat_t	target_rotation = quat_t::identity;
			vec3f_t target_scale	= vec3f_t::one;

			transition_target_bone.local_matrix.decompose(target_position, target_rotation, target_scale);

			final_bone.local_matrix =
				mat4x3_t::transform(vec3f_t::lerp(final_position, target_position, transition_target_blend), quat_t::slerp(final_rotation, target_rotation, transition_target_blend), vec3f_t::lerp(final_scale, target_scale, transition_target_blend));
		}
	}

	void animation_graph_storage_t::process_node_bone_control(animation_graph_node_bone_control_t& node, span_t<animation_graph_bone_t> pose_bones, f32 delta_time)
	{
	}

	void animation_graph_storage_t::process_node_ik(animation_graph_node_ik_t& node, span_t<animation_graph_bone_t> pose_bones, f32 delta_time)
	{
	}

	void animation_graph_storage_t::process_asm_state(animation_graph_asm_state_t& state, chunk_handle32_t mask_handle, f32 sample_time, span_t<animation_graph_bone_t> pose_bones)
	{
		if (state.clip_count == 0)
		{
			SFG_WARN("animation state has no clips");
			return;
		}

		const animation_graph_mask_t* mask	= mask_handle ? _masks.get<animation_graph_mask_t>(mask_handle) : nullptr;
		const animation_graph_clip_t* clips = _clips.get<animation_graph_clip_t>(state.clips);

		// 0d, directly write anim data into pose.
		if (state.state_type == animation_graph_asm_state_type_e::no_blend)
		{
			sample_clip(clips[0].clip, sample_time, mask, pose_bones);
			return;
		}

		frame_vector_t<f32> blend_weights(state.clip_count, 0.0f);

		// determine weights
		switch (state.state_type)
		{
		case animation_graph_asm_state_type_e::no_blend:
			break;
		case animation_graph_asm_state_type_e::blend_1d: {
			const animation_graph_param_t& parameter = *_params.get<animation_graph_param_t>(state.blend_parameter);
			if (parameter.type != animation_param_type_e::f32)
			{
				SFG_WARN("animation parameter type does not match blend type! parameter type: {0}, blend type: 1d", (u32)parameter.type);
				return;
			}

			u32 lower_index = UINT32_MAX;
			u32 upper_index = UINT32_MAX;

			for (u32 i = 0; i < state.clip_count; ++i)
			{
				if (clips[i].blend_value <= parameter.f32_value && (lower_index == UINT32_MAX || clips[i].blend_value > clips[lower_index].blend_value))
					lower_index = i;

				if (clips[i].blend_value >= parameter.f32_value && (upper_index == UINT32_MAX || clips[i].blend_value < clips[upper_index].blend_value))
					upper_index = i;
			}

			if (lower_index == UINT32_MAX)
				blend_weights[upper_index] = 1.0f;
			else if (upper_index == UINT32_MAX || lower_index == upper_index)
				blend_weights[lower_index] = 1.0f;
			else
			{
				const f32 range			   = clips[upper_index].blend_value - clips[lower_index].blend_value;
				const f32 blend			   = (parameter.f32_value - clips[lower_index].blend_value) / range;
				blend_weights[lower_index] = 1.0f - blend;
				blend_weights[upper_index] = blend;
			}
			break;
		}
		case animation_graph_asm_state_type_e::blend_2d: {
			const animation_graph_param_t& parameter = *_params.get<animation_graph_param_t>(state.blend_parameter);
			if (parameter.type != animation_param_type_e::vec2)
			{
				SFG_WARN("animation parameter type does not match blend type! parameter type: {0}, blend type: 2d", (u32)parameter.type);
				return;
			}

			const f32 epsilon_sq   = MATH_EPS * MATH_EPS;
			f32		  total_weight = 0.0f;
			u32		  exact_index  = UINT32_MAX;

			for (u32 i = 0; i < state.clip_count; ++i)
			{
				const vec2f_t offset	  = clips[i].blend_value_2d - parameter.vec2_value;
				const f32	  distance_sq = offset.magnitude_sqr();

				if (distance_sq <= epsilon_sq)
				{
					exact_index = i;
					break;
				}

				blend_weights[i] = 1.0f / distance_sq;
				total_weight += blend_weights[i];
			}

			if (exact_index != UINT32_MAX)
			{
				for (u32 i = 0; i < state.clip_count; ++i)
					blend_weights[i] = 0.0f;

				blend_weights[exact_index] = 1.0f;
			}
			else
			{
				for (u32 i = 0; i < state.clip_count; ++i)
					blend_weights[i] /= total_weight;
			}
			break;
		}
		}

		frame_vector_t<animation_graph_bone_t> base_bones(pose_bones.data, pose_bones.data + pose_bones.size);
		frame_vector_t<animation_graph_bone_t> sampled_bones(pose_bones.size);
		const span_t<animation_graph_bone_t>   sampled_pose{
			.data = sampled_bones.data(),
			.size = sampled_bones.size(),
		};
		f32 accumulated_weight = 0.0f;

		for (u32 i = 0; i < state.clip_count; ++i)
		{
			if (blend_weights[i] <= 0.0f)
				continue;

			// copy base to sampled, sample_clip overrides whatever bone was written.
			SFG_MEMCPY(sampled_pose.data, base_bones.data(), sizeof(animation_graph_bone_t) * base_bones.size());
			sample_clip(clips[i].clip, sample_time, mask, sampled_pose);

			if (accumulated_weight == 0.0f)
			{
				SFG_MEMCPY(pose_bones.data, sampled_pose.data, sizeof(animation_graph_bone_t) * pose_bones.size);
				accumulated_weight = blend_weights[i];
				continue;
			}

			const f32 total_weight = accumulated_weight + blend_weights[i];
			const f32 blend		   = blend_weights[i] / total_weight;

			// blend sampled anim into final w/ weight
			for (size_t bone_index = 0; bone_index < pose_bones.size; ++bone_index)
			{
				animation_graph_bone_t&		  final_bone   = pose_bones.data[bone_index];
				const animation_graph_bone_t& sampled_bone = sampled_pose.data[bone_index];

				vec3f_t final_position = vec3f_t::zero;
				quat_t	final_rotation = quat_t::identity;
				vec3f_t final_scale	   = vec3f_t::one;

				final_bone.local_matrix.decompose(final_position, final_rotation, final_scale);

				vec3f_t sampled_position = vec3f_t::zero;
				quat_t	sampled_rotation = quat_t::identity;
				vec3f_t sampled_scale	 = vec3f_t::one;

				sampled_bone.local_matrix.decompose(sampled_position, sampled_rotation, sampled_scale);

				final_bone.local_matrix = mat4x3_t::transform(vec3f_t::lerp(final_position, sampled_position, blend), quat_t::slerp(final_rotation, sampled_rotation, blend), vec3f_t::lerp(final_scale, sampled_scale, blend));
			}

			accumulated_weight = total_weight;
		}
	}

	void animation_graph_storage_t::sample_clip(resource_handle_t clip, f32 sample_time, const animation_graph_mask_t* mask, span_t<animation_graph_bone_t> pose_bones)
	{
		resource_manager_t&		   resource_manager = resource_manager_t::get();
		const animation_runtime_t* animation		= resource_manager.find_runtime<animation_runtime_t>(clip);

		if (animation == nullptr)
		{
			SFG_WARN("animation clip is not loaded: {0}", clip);
			return;
		}

		const u64* bitmasks = mask != nullptr ? mask->bitmasks : nullptr;
		animation_sampler_t::sample_animation(animation, sample_time, bitmasks, pose_bones);
	}
}
