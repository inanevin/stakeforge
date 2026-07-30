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
#include <sfg/math/triangulation_2d.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/animation.hpp>
#include <sfg/runtime/resources/animation_graph.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skeleton.hpp>

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

	void animation_graph_storage_t::reset()
	{
		_aux.reset();
		_pose_bones.reset();
		_poses.reset();
		_asm_transitions.reset();
		_asm_states.reset();
		_clips.reset();
		_masks.reset();
		_params.reset();
		_nodes.reset();
	}

	animation_graph_storage_instance_t animation_graph_storage_t::create_graph(const animation_graph_runtime_t& graph, const chunk_allocator_t& resource_memory, const skeleton_runtime_t& skeleton)
	{
		animation_graph_storage_instance_t instance{
			.initial_pose	 = animation_graph_util_t::create_pose_from_skeleton(skeleton, resource_memory, _poses, _pose_bones, _aux),
			.parameter_count = graph.parameter_count,
			.node_count		 = graph.node_count,
		};

		// parameters
		if (instance.parameter_count != 0)
		{
			instance.parameters = _params.allocate_bytes(sizeof(animation_graph_param_t) * instance.parameter_count, alignof(animation_graph_param_t));

			const animation_graph_param_t* source_parameters	  = resource_memory.get<animation_graph_param_t>(graph.parameters);
			animation_graph_param_t*	   destination_parameters = _params.get<animation_graph_param_t>(instance.parameters);

			for (u32 parameter_index = 0; parameter_index < instance.parameter_count; ++parameter_index)
				destination_parameters[parameter_index] = source_parameters[parameter_index];
		}

		if (instance.node_count == 0)
			return instance;

		// base nodes.
		instance.nodes = _nodes.allocate_bytes(sizeof(animation_graph_node_t) * instance.node_count, alignof(animation_graph_node_t));

		const animation_graph_resource_node_t* source_nodes		 = resource_memory.get<animation_graph_resource_node_t>(graph.nodes);
		animation_graph_node_t*				   destination_nodes = _nodes.get<animation_graph_node_t>(instance.nodes);

		for (u32 node_index = 0; node_index < instance.node_count; ++node_index)
		{
			const animation_graph_resource_node_t& source_node		= source_nodes[node_index];
			animation_graph_node_t&				   destination_node = destination_nodes[node_index];

			destination_node			 = {};
			destination_node.type		 = source_node.type;
			destination_node.pose_handle = animation_graph_util_t::create_pose_from_skeleton(skeleton, resource_memory, _poses, _pose_bones, _aux);

			// asm node
			if (source_node.type == animation_graph_node_type_e::asm_node)
			{
				const animation_graph_resource_asm_t& source_asm	  = source_node.asm_node;
				animation_graph_node_asm_t&			  destination_asm = destination_node.node_asm;
				bool								  has_mask		  = false;

				for (const u64 bitmask : source_asm.mask.bitmasks)
					has_mask |= bitmask != 0;

				if (has_mask)
				{
					destination_node.mask_handle									  = _masks.allocate_bytes(sizeof(animation_graph_mask_t), alignof(animation_graph_mask_t));
					*_masks.get<animation_graph_mask_t>(destination_node.mask_handle) = source_asm.mask;
				}

				destination_asm.state_count		 = source_asm.state_count;
				destination_asm.transition_count = source_asm.transition_count;

				// asm node states
				if (destination_asm.state_count != 0)
				{
					destination_asm.states = _asm_states.allocate_bytes(sizeof(animation_graph_asm_state_t) * destination_asm.state_count, alignof(animation_graph_asm_state_t));

					const animation_graph_resource_state_t* source_states	   = resource_memory.get<animation_graph_resource_state_t>(source_asm.states);
					animation_graph_asm_state_t*			destination_states = _asm_states.get<animation_graph_asm_state_t>(destination_asm.states);

					for (u32 state_index = 0; state_index < destination_asm.state_count; ++state_index)
					{
						const animation_graph_resource_state_t& source_state	  = source_states[state_index];
						animation_graph_asm_state_t&			destination_state = destination_states[state_index];

						destination_state					   = {};
						destination_state.clip_count		   = source_state.clip_count;
						destination_state.blend_triangle_count = source_state.blend_triangle_count;
						destination_state.state_type		   = source_state.state_type;
						destination_state.loop				   = source_state.loop;

						if (source_state.blend_parameter_index != UINT32_MAX)
						{
							SFG_ASSERT(source_state.blend_parameter_index < instance.parameter_count);

							destination_state.blend_parameter = {
								.head = instance.parameters.head + static_cast<u32>(sizeof(animation_graph_param_t)) * source_state.blend_parameter_index,
								.size = sizeof(animation_graph_param_t),
							};
						}

						if (destination_state.clip_count == 0)
							continue;

						// clips
						destination_state.clips = _clips.allocate_bytes(sizeof(animation_graph_clip_t) * destination_state.clip_count, alignof(animation_graph_clip_t));

						const animation_graph_resource_clip_t* source_clips		 = resource_memory.get<animation_graph_resource_clip_t>(source_state.clips);
						animation_graph_clip_t*				   destination_clips = _clips.get<animation_graph_clip_t>(destination_state.clips);

						for (u32 clip_index = 0; clip_index < destination_state.clip_count; ++clip_index)
						{
							destination_clips[clip_index] = {
								.clip			= source_clips[clip_index].clip,
								.playback_speed = source_clips[clip_index].playback_speed,
							};

							if (source_state.state_type == animation_graph_asm_state_type_e::blend_2d)
								destination_clips[clip_index].blend_value_2d = source_clips[clip_index].blend_value_2d;
							else
								destination_clips[clip_index].blend_value = source_clips[clip_index].blend_value;
						}

						if (destination_state.blend_triangle_count != 0)
						{
							destination_state.blend_triangles = _aux.allocate_bytes(sizeof(animation_graph_blend_triangle_t) * destination_state.blend_triangle_count, alignof(animation_graph_blend_triangle_t));

							const animation_graph_blend_triangle_t* source_triangles	  = resource_memory.get<animation_graph_blend_triangle_t>(source_state.blend_triangles);
							animation_graph_blend_triangle_t*		destination_triangles = _aux.get<animation_graph_blend_triangle_t>(destination_state.blend_triangles);

							SFG_MEMCPY(destination_triangles, source_triangles, sizeof(animation_graph_blend_triangle_t) * destination_state.blend_triangle_count);
						}
					}

					if (source_asm.first_state_index != UINT32_MAX)
					{
						SFG_ASSERT(source_asm.first_state_index < destination_asm.state_count);

						destination_asm.first_state = {
							.head = destination_asm.states.head + static_cast<u32>(sizeof(animation_graph_asm_state_t)) * source_asm.first_state_index,
							.size = sizeof(animation_graph_asm_state_t),
						};
					}
				}

				// transitions
				if (destination_asm.transition_count == 0)
					continue;

				destination_asm.transitions = _asm_transitions.allocate_bytes(sizeof(animation_graph_asm_transition_t) * destination_asm.transition_count, alignof(animation_graph_asm_transition_t));

				const animation_graph_resource_transition_t* source_transitions		 = resource_memory.get<animation_graph_resource_transition_t>(source_asm.transitions);
				animation_graph_asm_transition_t*			 destination_transitions = _asm_transitions.get<animation_graph_asm_transition_t>(destination_asm.transitions);

				for (u32 transition_index = 0; transition_index < destination_asm.transition_count; ++transition_index)
				{
					const animation_graph_resource_transition_t& source_transition		= source_transitions[transition_index];
					animation_graph_asm_transition_t&			 destination_transition = destination_transitions[transition_index];

					SFG_ASSERT(source_transition.from_state_index < destination_asm.state_count);
					SFG_ASSERT(source_transition.to_state_index < destination_asm.state_count);

					destination_transition = {
						.from_state =
							{
								.head = destination_asm.states.head + static_cast<u32>(sizeof(animation_graph_asm_state_t)) * source_transition.from_state_index,
								.size = sizeof(animation_graph_asm_state_t),
							},
						.to_state =
							{
								.head = destination_asm.states.head + static_cast<u32>(sizeof(animation_graph_asm_state_t)) * source_transition.to_state_index,
								.size = sizeof(animation_graph_asm_state_t),
							},

						.compare_value	  = source_transition.compare_value,
						.duration		  = source_transition.duration,
						.priority		  = source_transition.priority,
						.type			  = source_transition.type,
						.is_blended		  = source_transition.is_blended,
						.is_interruptible = source_transition.is_interruptible,
						.restart_target	  = source_transition.restart_target,
					};

					if (source_transition.parameter_index == UINT32_MAX)
						destination_transition.parameter = {};
					else
					{
						destination_transition.parameter = {
							.head = instance.parameters.head + static_cast<u32>(sizeof(animation_graph_param_t)) * source_transition.parameter_index,
							.size = sizeof(animation_graph_param_t),
						};
					}
				}

				continue;
			}

			if (source_node.type == animation_graph_node_type_e::ik)
			{
				std::construct_at(&destination_node.node_ik, animation_graph_node_ik_t{});
				continue;
			}

			// bone control
			std::construct_at(&destination_node.node_bone_control, animation_graph_node_bone_control_t{});

			const animation_graph_resource_bone_control_t& source_bone_control		= source_node.bone_control_node;
			animation_graph_node_bone_control_t&		   destination_bone_control = destination_node.node_bone_control;

			destination_bone_control.bone_count	   = source_bone_control.bone_count;
			destination_bone_control.control_type  = source_bone_control.control_type;
			destination_bone_control.control_space = source_bone_control.control_space;

			if (destination_bone_control.bone_count == 0)
				continue;

			destination_bone_control.bone_indices = _aux.allocate_bytes(sizeof(u32) * destination_bone_control.bone_count, alignof(u32));
			destination_bone_control.parameters	  = _aux.allocate_bytes(sizeof(chunk_handle32_t) * destination_bone_control.bone_count, alignof(chunk_handle32_t));

			const animation_graph_resource_bone_control_entry_t* source_bones	   = resource_memory.get<animation_graph_resource_bone_control_entry_t>(source_bone_control.bones);
			u32*												 bone_indices	   = _aux.get<u32>(destination_bone_control.bone_indices);
			chunk_handle32_t*									 parameter_handles = _aux.get<chunk_handle32_t>(destination_bone_control.parameters);

			for (u32 bone_index = 0; bone_index < destination_bone_control.bone_count; ++bone_index)
			{
				const animation_graph_resource_bone_control_entry_t& source_bone = source_bones[bone_index];

				SFG_ASSERT(source_bone.bone_index < skeleton.joint_count);
				SFG_ASSERT(source_bone.parameter_index < instance.parameter_count);

				bone_indices[bone_index]	  = source_bone.bone_index;
				parameter_handles[bone_index] = {
					.head = instance.parameters.head + static_cast<u32>(sizeof(animation_graph_param_t)) * source_bone.parameter_index,
					.size = sizeof(animation_graph_param_t),
				};
			}
		}

		return instance;
	}

	void animation_graph_storage_t::destroy_graph(const animation_graph_storage_instance_t& instance)
	{
		if (instance.node_count != 0)
		{
			animation_graph_node_t* nodes = _nodes.get<animation_graph_node_t>(instance.nodes);

			for (u32 node_index = 0; node_index < instance.node_count; ++node_index)
			{
				animation_graph_node_t& node = nodes[node_index];

				animation_graph_util_t::destroy_pose(node.pose_handle, _poses, _pose_bones, _aux);

				if (node.type == animation_graph_node_type_e::asm_node)
				{
					if (node.node_asm.state_count != 0)
					{
						animation_graph_asm_state_t* states = _asm_states.get<animation_graph_asm_state_t>(node.node_asm.states);

						for (u32 state_index = 0; state_index < node.node_asm.state_count; ++state_index)
						{
							if (states[state_index].clip_count != 0)
								_clips.free(states[state_index].clips);

							if (states[state_index].blend_triangle_count != 0)
								_aux.free(states[state_index].blend_triangles);
						}

						_asm_states.free(node.node_asm.states);
					}

					if (node.node_asm.transition_count != 0)
						_asm_transitions.free(node.node_asm.transitions);

					if (node.mask_handle)
						_masks.free(node.mask_handle);
				}
				else if (node.type == animation_graph_node_type_e::bone_controller && node.node_bone_control.bone_count != 0)
				{
					_aux.free(node.node_bone_control.bone_indices);
					_aux.free(node.node_bone_control.parameters);
				}
			}

			_nodes.free(instance.nodes);
		}

		if (instance.parameter_count != 0)
			_params.free(instance.parameters);

		animation_graph_util_t::destroy_pose(instance.initial_pose, _poses, _pose_bones, _aux);
	}

	animation_graph_param_t* animation_graph_storage_t::find_parameter(chunk_handle32_t parameters, u32 parameter_count, sid_t parameter_hash)
	{
		if (parameter_count == 0)
			return nullptr;

		animation_graph_param_t* values = _params.get<animation_graph_param_t>(parameters);

		for (u32 parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
		{
			if (values[parameter_index].name_hash == parameter_hash)
				return &values[parameter_index];
		}

		return nullptr;
	}

	const animation_graph_param_t* animation_graph_storage_t::find_parameter(chunk_handle32_t parameters, u32 parameter_count, sid_t parameter_hash) const
	{
		if (parameter_count == 0)
			return nullptr;

		const animation_graph_param_t* values = _params.get<animation_graph_param_t>(parameters);

		for (u32 parameter_index = 0; parameter_index < parameter_count; ++parameter_index)
		{
			if (values[parameter_index].name_hash == parameter_hash)
				return &values[parameter_index];
		}

		return nullptr;
	}

	void animation_graph_storage_t::copy_pose_to_bones(chunk_handle32_t pose_handle, span_t<animation_bone_t> bones) const
	{
		const animation_graph_pose_t& pose		 = *_poses.get<animation_graph_pose_t>(pose_handle);
		const animation_graph_bone_t* pose_bones = _pose_bones.get<animation_graph_bone_t>(pose.bones);

		SFG_ASSERT(bones.size == pose.bone_count);

		for (size_t bone_index = 0; bone_index < bones.size; ++bone_index)
			bones.data[bone_index].bone_transform = pose_bones[bone_index].local_matrix;
	}

	void animation_graph_storage_t::advance_graph(chunk_handle32_t nodes, u32 node_count, f32 delta_time)
	{
		animation_graph_node_t* graph_nodes = _nodes.get<animation_graph_node_t>(nodes);

		for (u32 node_index = 0; node_index < node_count; ++node_index)
		{
			animation_graph_node_t& node = graph_nodes[node_index];

			if (node.type == animation_graph_node_type_e::asm_node)
				advance_node_asm(node.node_asm, delta_time);
		}
	}

	void animation_graph_storage_t::process_graph(chunk_handle32_t nodes, u32 node_count, chunk_handle32_t initial_pose, const mat4x3_t& entity_transform, span_t<animation_bone_t> bones, f32 delta_time)
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
					process_node_bone_control(node.node_bone_control, entity_transform, pose_bones, delta_time);
					break;
				case animation_graph_node_type_e::ik:
					process_node_ik(node.node_ik, pose_bones, delta_time);
					break;
				}

				previous_pose_handle = node.pose_handle;
			}
		}

		copy_pose_to_bones(previous_pose_handle, bones);
	}

	bool animation_graph_storage_t::is_asm_transition_eligible(const animation_graph_asm_transition_t& transition) const
	{
		if (!transition.parameter)
		{
			const animation_graph_asm_state_t& state = *_asm_states.get<animation_graph_asm_state_t>(transition.from_state);
			return math::almost_equal(state._current_phase, 1.0f);
		}

		const animation_graph_param_t& parameter		= *_params.get<animation_graph_param_t>(transition.parameter);
		f32							   transition_value = 0.0f;

		switch (parameter.type)
		{
		case animation_param_type_e::f32:
			transition_value = parameter.f32_value;
			break;
		case animation_param_type_e::vec2:
			transition_value = parameter.vec2_value.x;
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

		switch (transition.type)
		{
		case animation_graph_asm_transition_type_e::equals:
			return transition_value == transition.compare_value;
		case animation_graph_asm_transition_type_e::lequals:
			return transition_value <= transition.compare_value;
		case animation_graph_asm_transition_type_e::gequals:
			return transition_value >= transition.compare_value;
		case animation_graph_asm_transition_type_e::less:
			return transition_value < transition.compare_value;
		case animation_graph_asm_transition_type_e::greater:
			return transition_value > transition.compare_value;
		}

		return false;
	}

	animation_graph_storage_t::asm_update_t animation_graph_storage_t::update_node_asm(animation_graph_node_asm_t& node, f32 delta_time)
	{
		asm_update_t update = {};

		if (!node.first_state)
		{
			SFG_WARN("animation state machine has no first state");
			return update;
		}

		if (!node._current_state)
			node._current_state = node.first_state;

		const animation_graph_asm_transition_t* active_transition		  = node._current_transition ? _asm_transitions.get<animation_graph_asm_transition_t>(node._current_transition) : nullptr;
		u32										selected_transition_index = UINT32_MAX;
		u32										selected_priority		  = 0;
		bool									selected_from_target	  = false;

		// check if eligible transition.
		if (active_transition == nullptr || active_transition->is_interruptible)
		{
			const animation_graph_asm_transition_t* transitions = _asm_transitions.get<animation_graph_asm_transition_t>(node.transitions);

			for (u32 transition_index = 0; transition_index < node.transition_count; ++transition_index)
			{
				const animation_graph_asm_transition_t& transition = transitions[transition_index];
				const chunk_handle32_t					transition_handle{
					.head = node.transitions.head + static_cast<u32>(sizeof(animation_graph_asm_transition_t)) * transition_index,
					.size = static_cast<u32>(sizeof(animation_graph_asm_transition_t)),
				};

				if (transition_handle == node._current_transition)
					continue;

				const bool from_current_state	  = transition.from_state == node._current_state;
				const bool from_transition_target = active_transition != nullptr && transition.from_state == active_transition->to_state;

				if (!from_current_state && !from_transition_target)
					continue;

				if (active_transition != nullptr && transition.priority < active_transition->priority)
					continue;

				if (!is_asm_transition_eligible(transition))
					continue;

				if (selected_transition_index != UINT32_MAX && transition.priority <= selected_priority)
					continue;

				selected_transition_index = transition_index;
				selected_priority		  = transition.priority;
				selected_from_target	  = from_transition_target;
			}
		}

		if (selected_transition_index != UINT32_MAX)
		{
			const animation_graph_asm_transition_t& transition = _asm_transitions.get<animation_graph_asm_transition_t>(node.transitions)[selected_transition_index];

			if (selected_from_target)
				node._current_state = active_transition->to_state;

			node._current_transition = {
				.head = node.transitions.head + static_cast<u32>(sizeof(animation_graph_asm_transition_t)) * selected_transition_index,
				.size = static_cast<u32>(sizeof(animation_graph_asm_transition_t)),
			};
			node._current_transition_time = 0.0f;

			if (transition.restart_target)
				_asm_states.get<animation_graph_asm_state_t>(transition.to_state)->_current_phase = 0.0f;
		}

		// if active transition, bump the time & switch state or set blended.
		if (node._current_transition)
		{
			const animation_graph_asm_transition_t& transition = *_asm_transitions.get<animation_graph_asm_transition_t>(node._current_transition);
			node._current_transition_time += delta_time;

			if (node._current_transition_time >= transition.duration)
			{
				node._current_transition	  = {};
				node._current_transition_time = 0.0f;
				node._current_state			  = transition.to_state;
			}
			else if (transition.is_blended)
			{
				update.transition_target = _asm_states.get<animation_graph_asm_state_t>(transition.to_state);
				update.transition_blend	 = node._current_transition_time / transition.duration;
			}
		}

		update.current_state = _asm_states.get<animation_graph_asm_state_t>(node._current_state);
		return update;
	}

	void animation_graph_storage_t::advance_node_asm(animation_graph_node_asm_t& node, f32 delta_time)
	{
		const asm_update_t update = update_node_asm(node, delta_time);

		if (update.current_state == nullptr)
			return;

		advance_asm_state(*update.current_state, delta_time);

		if (update.transition_target != nullptr)
			advance_asm_state(*update.transition_target, delta_time);
	}

	void animation_graph_storage_t::process_node_asm(animation_graph_node_asm_t& node, chunk_handle32_t mask_handle, span_t<animation_graph_bone_t> pose_bones, f32 delta_time)
	{
		const asm_update_t update = update_node_asm(node, delta_time);

		if (update.current_state == nullptr)
			return;

		// no transition, process & write to current pose.
		if (update.transition_target == nullptr)
		{
			process_asm_state(*update.current_state, mask_handle, delta_time, pose_bones);
			return;
		}

		// copy the current pose to target before we process.
		frame_vector_t<animation_graph_bone_t> transition_target_bones(pose_bones.size);
		const span_t<animation_graph_bone_t>   transition_target_pose{
			.data = transition_target_bones.data(),
			.size = transition_target_bones.size(),
		};

		SFG_MEMCPY(transition_target_pose.data, pose_bones.data, sizeof(animation_graph_bone_t) * pose_bones.size);

		// process current state normally, we write to current pose.
		process_asm_state(*update.current_state, mask_handle, delta_time, pose_bones);

		// process transition state, we write to temporary transition pose.
		process_asm_state(*update.transition_target, mask_handle, delta_time, transition_target_pose);

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
				mat4x3_t::transform(vec3f_t::lerp(final_position, target_position, update.transition_blend), quat_t::slerp(final_rotation, target_rotation, update.transition_blend), vec3f_t::lerp(final_scale, target_scale, update.transition_blend));
		}
	}

	void animation_graph_storage_t::process_node_bone_control(animation_graph_node_bone_control_t& node, const mat4x3_t& entity_transform, span_t<animation_graph_bone_t> pose_bones, f32 delta_time)
	{
		if (node.bone_count == 0)
		{
			SFG_WARN("bone control node has no bones");
			return;
		}

		animation_param_type_e parameter_type = animation_param_type_e::f32;

		switch (node.control_type)
		{
		case animation_graph_bone_control_type_e::rotation_override:
		case animation_graph_bone_control_type_e::rotation_additive:
			parameter_type = animation_param_type_e::quat;
			break;
		case animation_graph_bone_control_type_e::position_override:
		case animation_graph_bone_control_type_e::position_additive:
		case animation_graph_bone_control_type_e::look_at:
			parameter_type = animation_param_type_e::vec3;
			break;
		}

		const u32*				bone_indices	  = _aux.get<u32>(node.bone_indices);
		const chunk_handle32_t* parameter_handles = _aux.get<chunk_handle32_t>(node.parameters);

		for (u32 i = 0; i < node.bone_count; ++i)
		{
			const u32					   bone_index = bone_indices[i];
			const animation_graph_param_t& parameter  = *_params.get<animation_graph_param_t>(parameter_handles[i]);

			SFG_ASSERT(bone_index < pose_bones.size);

			if (parameter.type != parameter_type)
			{
				SFG_WARN("bone control parameter type does not match control type");
				continue;
			}

			animation_graph_bone_t& pose_bone				= pose_bones.data[bone_index];
			mat4x3_t				parent_to_control_space = mat4x3_t::identity;

			if (node.control_space != animation_graph_bone_control_space_e::local)
			{
				u32 parent_index = pose_bone.parent_index;

				while (parent_index != UINT32_MAX)
				{
					const animation_graph_bone_t& parent_bone = pose_bones.data[parent_index];

					parent_to_control_space = parent_bone.local_matrix * parent_to_control_space;
					parent_index			= parent_bone.parent_index;
				}

				if (node.control_space == animation_graph_bone_control_space_e::world)
					parent_to_control_space = entity_transform * parent_to_control_space;
			}

			mat4x3_t control_transform = parent_to_control_space * pose_bone.local_matrix;
			vec3f_t	 control_position  = vec3f_t::zero;
			quat_t	 control_rotation  = quat_t::identity;
			vec3f_t	 control_scale	   = vec3f_t::one;

			control_transform.decompose(control_position, control_rotation, control_scale);

			switch (node.control_type)
			{
			case animation_graph_bone_control_type_e::rotation_override:
				control_rotation  = parameter.quat_value;
				control_transform = mat4x3_t::transform(control_position, control_rotation, control_scale);
				break;
			case animation_graph_bone_control_type_e::rotation_additive:
				control_rotation  = parameter.quat_value * control_rotation;
				control_transform = mat4x3_t::transform(control_position, control_rotation, control_scale);
				break;
			case animation_graph_bone_control_type_e::position_override:
				control_position  = parameter.vec3_value;
				control_transform = mat4x3_t::transform(control_position, control_rotation, control_scale);
				break;
			case animation_graph_bone_control_type_e::position_additive:
				control_position += parameter.vec3_value;
				control_transform = mat4x3_t::transform(control_position, control_rotation, control_scale);
				break;
			case animation_graph_bone_control_type_e::look_at:
				control_rotation  = quat_t::look_at(control_position, parameter.vec3_value, vec3f_t::up);
				control_transform = mat4x3_t::transform(control_position, control_rotation, control_scale);
				break;
			}

			pose_bone.local_matrix = node.control_space == animation_graph_bone_control_space_e::local ? control_transform : parent_to_control_space.inverse() * control_transform;
		}
	}

	void animation_graph_storage_t::process_node_ik(animation_graph_node_ik_t& node, span_t<animation_graph_bone_t> pose_bones, f32 delta_time)
	{
	}

	bool animation_graph_storage_t::compute_asm_state_blend_weights(const animation_graph_asm_state_t& state, span_t<f32> blend_weights) const
	{
		// determine weights
		switch (state.state_type)
		{
		case animation_graph_asm_state_type_e::no_blend:
			blend_weights.data[0] = 1.0f;
			return true;
		case animation_graph_asm_state_type_e::blend_1d: {

			if (!state.blend_parameter)
			{
				// SFG_WARN("state is missing blend parameter!");
				return false;
			}

			const animation_graph_clip_t*  clips	 = _clips.get<animation_graph_clip_t>(state.clips);
			const animation_graph_param_t& parameter = *_params.get<animation_graph_param_t>(state.blend_parameter);

			if (parameter.type != animation_param_type_e::f32)
			{
				SFG_WARN("animation parameter type does not match blend type! parameter type: {0}, blend type: 1d", (u32)parameter.type);
				return false;
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

			return true;
		}
		case animation_graph_asm_state_type_e::blend_2d: {
			if (!state.blend_parameter)
			{
				// SFG_WARN("state is missing blend parameter");
				return false;
			}

			const animation_graph_clip_t*  clips	 = _clips.get<animation_graph_clip_t>(state.clips);
			const animation_graph_param_t& parameter = *_params.get<animation_graph_param_t>(state.blend_parameter);

			if (parameter.type != animation_param_type_e::vec2)
			{
				SFG_WARN("animation parameter type does not match blend type! parameter type: {0}, blend type: 2d", (u32)parameter.type);
				return false;
			}

			if (state.clip_count == 1)
			{
				blend_weights[0] = 1.0f;
				return true;
			}

			if (state.blend_triangle_count == 0)
			{
				vec2f_t bounds_min = clips[0].blend_value_2d;
				vec2f_t bounds_max = clips[0].blend_value_2d;

				for (u32 clip_index = 1; clip_index < state.clip_count; ++clip_index)
				{
					bounds_min = vec2f_t::min(bounds_min, clips[clip_index].blend_value_2d);
					bounds_max = vec2f_t::max(bounds_max, clips[clip_index].blend_value_2d);
				}

				const vec2f_t bounds_size	  = bounds_max - bounds_min;
				const bool	  use_x			  = bounds_size.x >= bounds_size.y;
				const f32	  parameter_value = use_x ? parameter.vec2_value.x : parameter.vec2_value.y;
				u32			  lower_index	  = UINT32_MAX;
				u32			  upper_index	  = UINT32_MAX;

				for (u32 clip_index = 0; clip_index < state.clip_count; ++clip_index)
				{
					const f32 clip_value = use_x ? clips[clip_index].blend_value_2d.x : clips[clip_index].blend_value_2d.y;

					if (clip_value <= parameter_value && (lower_index == UINT32_MAX || clip_value > (use_x ? clips[lower_index].blend_value_2d.x : clips[lower_index].blend_value_2d.y)))
						lower_index = clip_index;

					if (clip_value >= parameter_value && (upper_index == UINT32_MAX || clip_value < (use_x ? clips[upper_index].blend_value_2d.x : clips[upper_index].blend_value_2d.y)))
						upper_index = clip_index;
				}

				if (lower_index == UINT32_MAX)
					blend_weights[upper_index] = 1.0f;
				else if (upper_index == UINT32_MAX || lower_index == upper_index)
					blend_weights[lower_index] = 1.0f;
				else
				{
					const f32 lower_value = use_x ? clips[lower_index].blend_value_2d.x : clips[lower_index].blend_value_2d.y;
					const f32 upper_value = use_x ? clips[upper_index].blend_value_2d.x : clips[upper_index].blend_value_2d.y;
					const f32 blend		  = (parameter_value - lower_value) / (upper_value - lower_value);

					blend_weights[lower_index] = 1.0f - blend;
					blend_weights[upper_index] = blend;
				}

				return true;
			}

			const animation_graph_blend_triangle_t* triangles			   = _aux.get<animation_graph_blend_triangle_t>(state.blend_triangles);
			u32										closest_triangle_index = UINT32_MAX;
			vec3f_t									closest_weights		   = vec3f_t::zero;
			f32										closest_distance	   = MATH_INF_F;

			for (u32 triangle_index = 0; triangle_index < state.blend_triangle_count; ++triangle_index)
			{
				const animation_graph_blend_triangle_t& triangle = triangles[triangle_index];
				const vec2f_t&							a		 = clips[triangle.clip_indices[0]].blend_value_2d;
				const vec2f_t&							b		 = clips[triangle.clip_indices[1]].blend_value_2d;
				const vec2f_t&							c		 = clips[triangle.clip_indices[2]].blend_value_2d;
				vec3f_t									weights	 = vec3f_t::zero;
				const bool								valid	 = math::triangle_barycentric_2d(parameter.vec2_value, a, b, c, weights);
				SFG_ASSERT(valid);

				if (weights.x >= -MATH_EPS && weights.y >= -MATH_EPS && weights.z >= -MATH_EPS)
				{
					weights.x = math::max(weights.x, 0.0f);
					weights.y = math::max(weights.y, 0.0f);
					weights.z = math::max(weights.z, 0.0f);

					const f32 total_weight = weights.x + weights.y + weights.z;

					blend_weights[triangle.clip_indices[0]] = weights.x / total_weight;
					blend_weights[triangle.clip_indices[1]] = weights.y / total_weight;
					blend_weights[triangle.clip_indices[2]] = weights.z / total_weight;
					return true;
				}

				const vec3f_t edge_weights	= math::closest_triangle_barycentric_2d(parameter.vec2_value, a, b, c);
				const vec2f_t closest_point = a * edge_weights.x + b * edge_weights.y + c * edge_weights.z;
				const f32	  distance		= (closest_point - parameter.vec2_value).magnitude_sqr();

				if (distance >= closest_distance)
					continue;

				closest_triangle_index = triangle_index;
				closest_weights		   = edge_weights;
				closest_distance	   = distance;
			}

			SFG_ASSERT(closest_triangle_index != UINT32_MAX);

			const animation_graph_blend_triangle_t& closest_triangle = triangles[closest_triangle_index];
			blend_weights[closest_triangle.clip_indices[0]]			 = closest_weights.x;
			blend_weights[closest_triangle.clip_indices[1]]			 = closest_weights.y;
			blend_weights[closest_triangle.clip_indices[2]]			 = closest_weights.z;
			return true;
		}
		}

		return false;
	}

	void animation_graph_storage_t::advance_asm_state(animation_graph_asm_state_t& state, f32 delta_time)
	{
		if (state.clip_count == 0)
		{
			SFG_WARN("animation state has no clips");
			return;
		}

		const animation_graph_clip_t* clips			   = _clips.get<animation_graph_clip_t>(state.clips);
		resource_manager_t&			  resource_manager = resource_manager_t::get();

		if (state.state_type == animation_graph_asm_state_type_e::no_blend)
		{
			const animation_runtime_t* animation = resource_manager.find_runtime<animation_runtime_t>(clips[0].clip);

			if (animation == nullptr)
			{
				SFG_WARN("animation clip is not loaded: {0}", clips[0].clip);
				return;
			}

			animation_graph_util_t::advance_asm_state_phase(state, delta_time, animation->duration / clips[0].playback_speed);
			return;
		}

		frame_vector_t<f32> blend_weights(state.clip_count, 0.0f);
		const span_t<f32>	blend_weights_span{
			.data = blend_weights.data(),
			.size = blend_weights.size(),
		};

		if (!compute_asm_state_blend_weights(state, blend_weights_span))
			return;

		f32 weighted_duration = 0.0f;
		f32 loaded_weight	  = 0.0f;

		for (u32 i = 0; i < state.clip_count; ++i)
		{
			if (blend_weights[i] <= 0.0f)
				continue;

			const animation_runtime_t* animation = resource_manager.find_runtime<animation_runtime_t>(clips[i].clip);

			if (animation == nullptr)
			{
				SFG_WARN("animation clip is not loaded: {0}", clips[i].clip);
				continue;
			}

			weighted_duration += animation->duration / clips[i].playback_speed * blend_weights[i];
			loaded_weight += blend_weights[i];
		}

		const f32 duration = loaded_weight > 0.0f ? weighted_duration / loaded_weight : 0.0f;

		animation_graph_util_t::advance_asm_state_phase(state, delta_time, duration);
	}

	void animation_graph_storage_t::process_asm_state(animation_graph_asm_state_t& state, chunk_handle32_t mask_handle, f32 delta_time, span_t<animation_graph_bone_t> pose_bones)
	{
		if (state.clip_count == 0)
		{
			SFG_WARN("animation state has no clips");
			return;
		}

		const animation_graph_mask_t* mask			   = mask_handle ? _masks.get<animation_graph_mask_t>(mask_handle) : nullptr;
		const animation_graph_clip_t* clips			   = _clips.get<animation_graph_clip_t>(state.clips);
		resource_manager_t&			  resource_manager = resource_manager_t::get();

		// 0d, directly write anim data into pose.
		if (state.state_type == animation_graph_asm_state_type_e::no_blend)
		{
			const animation_runtime_t* animation = resource_manager.find_runtime<animation_runtime_t>(clips[0].clip);

			if (animation == nullptr)
			{
				SFG_WARN("animation clip is not loaded: {0}", clips[0].clip);
				return;
			}

			animation_graph_util_t::advance_asm_state_phase(state, delta_time, animation->duration / clips[0].playback_speed);

			const f32 sample_time = state._current_phase * animation->duration;

			sample_clip(*animation, sample_time, mask, pose_bones);
			return;
		}

		frame_vector_t<f32> blend_weights(state.clip_count, 0.0f);
		const span_t<f32>	blend_weights_span{
			.data = blend_weights.data(),
			.size = blend_weights.size(),
		};

		// determine weights
		if (!compute_asm_state_blend_weights(state, blend_weights_span))
			return;

		frame_vector_t<const animation_runtime_t*> animations(state.clip_count, nullptr);
		f32										   weighted_duration = 0.0f;
		f32										   loaded_weight	 = 0.0f;

		for (u32 i = 0; i < state.clip_count; ++i)
		{
			if (blend_weights[i] <= 0.0f)
				continue;

			const animation_runtime_t* animation = resource_manager.find_runtime<animation_runtime_t>(clips[i].clip);

			if (animation == nullptr)
			{
				SFG_WARN("animation clip is not loaded: {0}", clips[i].clip);
				continue;
			}

			animations[i] = animation;
			weighted_duration += animation->duration / clips[i].playback_speed * blend_weights[i];
			loaded_weight += blend_weights[i];
		}

		const f32 duration = loaded_weight > 0.0f ? weighted_duration / loaded_weight : 0.0f;

		animation_graph_util_t::advance_asm_state_phase(state, delta_time, duration);

		frame_vector_t<animation_graph_bone_t> base_bones(pose_bones.data, pose_bones.data + pose_bones.size);
		frame_vector_t<animation_graph_bone_t> sampled_bones(pose_bones.size);
		const span_t<animation_graph_bone_t>   sampled_pose{
			.data = sampled_bones.data(),
			.size = sampled_bones.size(),
		};
		f32 accumulated_weight = 0.0f;

		for (u32 i = 0; i < state.clip_count; ++i)
		{
			const animation_runtime_t* animation = animations[i];

			if (animation == nullptr)
				continue;

			// copy base to sampled, sample_clip overrides whatever bone was written.
			SFG_MEMCPY(sampled_pose.data, base_bones.data(), sizeof(animation_graph_bone_t) * base_bones.size());

			const f32 sample_time = state._current_phase * animation->duration;

			sample_clip(*animation, sample_time, mask, sampled_pose);

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

	void animation_graph_storage_t::sample_clip(const animation_runtime_t& animation, f32 sample_time, const animation_graph_mask_t* mask, span_t<animation_graph_bone_t> pose_bones)
	{
		const u64* bitmasks = mask != nullptr ? mask->bitmasks : nullptr;

		animation_sampler_t::sample_animation(&animation, sample_time, bitmasks, pose_bones);
	}
}
