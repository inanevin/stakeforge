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
#include "animation_sampler.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/resources/animation.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

namespace sfg
{

#define ANIMATION_GRAPH_NODE_MEMORY_PERCENT		 30
#define ANIMATION_GRAPH_PARAM_MEMORY_PERCENT	 15
#define ANIMATION_GRAPH_MASK_MEMORY_PERCENT		 10
#define ANIMATION_GRAPH_CLIP_MEMORY_PERCENT		 15
#define ANIMATION_GRAPH_ASM_STATE_MEMORY_PERCENT 20

	animation_graph_storage_t::~animation_graph_storage_t() = default;

	void animation_graph_storage_t::init(u32 storage_memory)
	{
		const u32 node_memory			= storage_memory * ANIMATION_GRAPH_NODE_MEMORY_PERCENT / 100;
		const u32 param_memory			= storage_memory * ANIMATION_GRAPH_PARAM_MEMORY_PERCENT / 100;
		const u32 mask_memory			= storage_memory * ANIMATION_GRAPH_MASK_MEMORY_PERCENT / 100;
		const u32 clip_memory			= storage_memory * ANIMATION_GRAPH_CLIP_MEMORY_PERCENT / 100;
		const u32 asm_state_memory		= storage_memory * ANIMATION_GRAPH_ASM_STATE_MEMORY_PERCENT / 100;
		const u32 asm_transition_memory = storage_memory - node_memory - param_memory - mask_memory - clip_memory - asm_state_memory;

		_nodes.init(node_memory);
		_params.init(param_memory);
		_masks.init(mask_memory);
		_clips.init(clip_memory);
		_asm_states.init(asm_state_memory);
		_asm_transitions.init(asm_transition_memory);
	}

	void animation_graph_storage_t::uninit()
	{
		_asm_transitions.uninit();
		_asm_states.uninit();
		_clips.uninit();
		_masks.uninit();
		_params.uninit();
		_nodes.uninit();
	}

	void animation_graph_storage_t::process_graph(chunk_handle32_t first_node, span_t<animation_bone_t> bones, f32 delta_time)
	{
		chunk_handle32_t node_handle = first_node;

		while (node_handle)
		{
			animation_graph_node_t& node = *_nodes.get<animation_graph_node_t>(node_handle);

			process_node(node, delta_time);
			node_handle = node.next_node;
		}
	}

	void animation_graph_storage_t::process_node(animation_graph_node_t& node, f32 delta_time)
	{
		switch (node.type)
		{
		case animation_graph_node_type_e::asm_node:
			process_node_asm(node.node_asm, node.mask_handle, delta_time);
			break;
		case animation_graph_node_type_e::bone_controller:
			process_node_bone_control(node.node_bone_control, delta_time);
			break;
		case animation_graph_node_type_e::ik:
			process_node_ik(node.node_ik, delta_time);
			break;
		}
	}

	void animation_graph_storage_t::process_node_asm(animation_graph_node_asm_t& node, chunk_handle32_t mask_handle, f32 delta_time)
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

		if (!node._current_transition)
		{
			// check for transition switch.
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
			else
			{
				transition_target_node	= _asm_states.get<animation_graph_asm_state_t>(transition.to_state);
				transition_target_blend = node._current_transition_time / transition.duration;
			}
		}

		animation_graph_asm_state_t& current_state = *_asm_states.get<animation_graph_asm_state_t>(node._current_state);
		current_state._current_time += delta_time;

		if (current_state._duration > 0.0f)
			current_state._current_time = current_state.loop ? math::fmodf(current_state._current_time, current_state._duration) : math::min(current_state._current_time, current_state._duration);
		else
			current_state._current_time = 0.0f;

		animation_pose_t current_pose = process_asm_state(current_state, mask_handle, current_state._current_time);

		if (transition_target_node != nullptr)
		{
			transition_target_node->_current_time += delta_time;

			if (transition_target_node->_duration > 0.0f)
				transition_target_node->_current_time = transition_target_node->loop ? math::fmodf(transition_target_node->_current_time, transition_target_node->_duration) : math::min(transition_target_node->_current_time, transition_target_node->_duration);
			else
				transition_target_node->_current_time = 0.0f;

			animation_pose_t transition_target_pose = process_asm_state(*transition_target_node, mask_handle, transition_target_node->_current_time);
		}
	}

	void animation_graph_storage_t::process_node_bone_control(animation_graph_node_bone_control_t& node, f32 delta_time)
	{
	}

	void animation_graph_storage_t::process_node_ik(animation_graph_node_ik_t& node, f32 delta_time)
	{
	}

	animation_pose_t animation_graph_storage_t::process_asm_state(animation_graph_asm_state_t& state, chunk_handle32_t mask_handle, f32 sample_time)
	{
		if (state.clip_count == 0)
		{
			SFG_WARN("animation state has no clips");
			return {};
		}

		const animation_graph_mask_t* mask	= mask_handle ? _masks.get<animation_graph_mask_t>(mask_handle) : nullptr;
		const animation_graph_clip_t* clips = _clips.get<animation_graph_clip_t>(state.clips);
		frame_vector_t<f32>			  blend_weights(state.clip_count, 0.0f);

		switch (state.state_type)
		{
		case animation_graph_asm_state_type_e::no_blend:
			blend_weights[0] = 1.0f;
			break;
		case animation_graph_asm_state_type_e::blend_1d: {
			const animation_graph_param_t& parameter = *_params.get<animation_graph_param_t>(state.blend_parameter);
			if (parameter.type != animation_param_type_e::f32)
			{
				SFG_WARN("animation parameter type does not match blend type! parameter type: {0}, blend type: 1d", (u32)parameter.type);
				return {};
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
				return {};
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

		animation_pose_t		final_pose		   = {};
		frame_vector_t<vec3f_t> blended_positions  = {};
		frame_vector_t<quat_t>	blended_rotations  = {};
		frame_vector_t<vec3f_t> blended_scales	   = {};
		f32						accumulated_weight = 0.0f;

		for (u32 i = 0; i < state.clip_count; ++i)
		{
			if (blend_weights[i] <= 0.0f)
				continue;

			animation_pose_t pose = sample_clip(clips[i].clip, sample_time, mask);

			if (final_pose.bones.empty())
			{
				final_pose.bones.resize(pose.bones.size());
				blended_positions.resize(pose.bones.size(), vec3f_t::zero);
				blended_rotations.resize(pose.bones.size(), quat_t::identity);
				blended_scales.resize(pose.bones.size(), vec3f_t::one);

				for (size_t bone_index = 0; bone_index < pose.bones.size(); ++bone_index)
				{
					final_pose.bones[bone_index].bone_index = pose.bones[bone_index].bone_index;
					pose.bones[bone_index].local_matrix.decompose(blended_positions[bone_index], blended_rotations[bone_index], blended_scales[bone_index]);
				}

				accumulated_weight = blend_weights[i];
			}
			else
			{
				const f32 total_weight = accumulated_weight + blend_weights[i];
				const f32 blend		   = blend_weights[i] / total_weight;

				for (size_t bone_index = 0; bone_index < pose.bones.size(); ++bone_index)
				{
					vec3f_t position = vec3f_t::zero;
					quat_t	rotation = quat_t::identity;
					vec3f_t scale	 = vec3f_t::one;
					pose.bones[bone_index].local_matrix.decompose(position, rotation, scale);

					blended_positions[bone_index] = vec3f_t::lerp(blended_positions[bone_index], position, blend);
					blended_rotations[bone_index] = quat_t::slerp(blended_rotations[bone_index], rotation, blend);
					blended_scales[bone_index]	  = vec3f_t::lerp(blended_scales[bone_index], scale, blend);
				}

				accumulated_weight = total_weight;
			}
		}

		// blend sampled poses using blend_weights.
		for (size_t bone_index = 0; bone_index < final_pose.bones.size(); ++bone_index)
			final_pose.bones[bone_index].local_matrix = mat4x3_t::transform(blended_positions[bone_index], blended_rotations[bone_index], blended_scales[bone_index]);

		return final_pose;
	}

	animation_pose_t animation_graph_storage_t::sample_clip(resource_handle_t clip, f32 sample_time, const animation_graph_mask_t* mask)
	{
		resource_manager_t&		   resource_manager = resource_manager_t::get();
		const animation_runtime_t* animation		= resource_manager.find_runtime<animation_runtime_t>(clip);

		if (animation == nullptr)
		{
			SFG_WARN("animation clip is not loaded: {0}", clip);
			return {};
		}

		const u64* bitmask0 = mask != nullptr ? &mask->bitmasks[0] : nullptr;
		const u64* bitmask1 = mask != nullptr ? &mask->bitmasks[1] : nullptr;

		return animation_sampler_t::sample_animation(animation, sample_time, bitmask0, bitmask1);
	}
}
