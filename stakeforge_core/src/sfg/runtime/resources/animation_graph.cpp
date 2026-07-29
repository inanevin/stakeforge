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

#include "animation_graph.hpp"
#include "animation_graph_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool animation_graph_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read animation graph resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());

		animation_graph_def_t def = {};

		if (!reflection_registry_t::get().type_from_stream(type_id_t<animation_graph_def_t>::value, &def, nullptr, stream))
		{
			SFG_ERR("failed to deserialize animation graph definition: {0}", entry.hash);
			return false;
		}

		if (def.entry_node_id == ANIMATION_GRAPH_DEF_NULL_ID || def.output_node_id == ANIMATION_GRAPH_DEF_NULL_ID)
		{
			SFG_WARN("animation graph is missing entry or exit node ids!");
			return false;
		}

		if (def.nodes.empty())
		{
			SFG_WARN("animation graph has no nodes");
			return false;
		}

		const animation_graph_node_def_t* source_node = def.find_node(def.entry_node_id);

		u32 node_count = 0;

		while (source_node != nullptr)
		{
			if (node_count == def.nodes.size())
			{
				SFG_ERR("animation graph contains a cycle: {0}", entry.hash);
				return false;
			}

			++node_count;

			if (source_node->id == def.output_node_id)
				break;

			source_node = def.find_node(source_node->next_node_id);

			if (source_node == nullptr)
			{
				SFG_ERR("animation graph exit node is unreachable!");
				return false;
			}
		}

		if (node_count == 0)
		{
			SFG_ERR("animation graph is empty!");
			return false;
		}

		chunk_allocator_t&		   mem	   = ctx.resource_manager.get_memory();
		animation_graph_runtime_t* runtime = mem.get<animation_graph_runtime_t>(entry.runtime);

		*runtime				 = {};
		runtime->target_skeleton = def.target_skeleton;
		runtime->parameter_count = static_cast<u32>(def.parameters.size());
		runtime->node_count		 = node_count;

		animation_graph_param_t* parameters = nullptr;

		if (runtime->parameter_count != 0)
		{
			runtime->parameters = mem.allocate_bytes(sizeof(animation_graph_param_t) * runtime->parameter_count, alignof(animation_graph_param_t));

			parameters = mem.get<animation_graph_param_t>(runtime->parameters);

			for (u32 parameter_index = 0; parameter_index < runtime->parameter_count; ++parameter_index)
			{
				const animation_graph_param_def_t& source	   = def.parameters[parameter_index];
				animation_graph_param_t&		   destination = parameters[parameter_index];

				destination			  = {};
				destination.name_hash = TO_SID(source.name.c_str());
				destination.type	  = source.type;

				switch (source.type)
				{
				case animation_param_type_e::f32:
					destination.f32_value = source.f32_value;
					break;
				case animation_param_type_e::vec2:
					destination.vec2_value = source.vec2_value;
					break;
				case animation_param_type_e::vec3:
					destination.vec3_value = source.vec3_value;
					break;
				case animation_param_type_e::quat:
					destination.quat_value = source.quat_value;
					break;
				case animation_param_type_e::boolean:
					destination.bool_value = source.bool_value;
					break;
				}
			}
		}

		if (runtime->node_count == 0)
			return true;

		runtime->nodes = mem.allocate_bytes(sizeof(animation_graph_resource_node_t) * runtime->node_count, alignof(animation_graph_resource_node_t));

		animation_graph_resource_node_t* nodes = mem.get<animation_graph_resource_node_t>(runtime->nodes);
		source_node							   = def.find_node(def.entry_node_id);

		for (u32 node_index = 0; node_index < runtime->node_count; ++node_index)
		{
			animation_graph_resource_node_t& destination = nodes[node_index];

			destination		 = {};
			destination.type = source_node->type;

			if (source_node->type == animation_graph_node_type_e::asm_node)
			{
				const animation_graph_node_asm_def_t& source_asm	  = source_node->asm_node;
				animation_graph_resource_asm_t&		  destination_asm = destination.asm_node;

				destination_asm.state_count		 = static_cast<u32>(source_asm.states.size());
				destination_asm.transition_count = static_cast<u32>(source_asm.transitions.size());

				for (const u32 bone_index : source_asm.masked_bones)
				{
					SFG_ASSERT(bone_index < MAX_SKELETON_BONES);
					destination_asm.mask.bitmasks[bone_index / 64] |= 1ull << (bone_index % 64);
				}

				if (destination_asm.state_count != 0)
				{
					destination_asm.states = mem.allocate_bytes(sizeof(animation_graph_resource_state_t) * destination_asm.state_count, alignof(animation_graph_resource_state_t));

					animation_graph_resource_state_t* states = mem.get<animation_graph_resource_state_t>(destination_asm.states);

					for (u32 state_index = 0; state_index < destination_asm.state_count; ++state_index)
					{
						const animation_graph_asm_state_def_t& source_state		 = source_asm.states[state_index];
						animation_graph_resource_state_t&	   destination_state = states[state_index];

						destination_state			 = {};
						destination_state.clip_count = static_cast<u32>(source_state.clips.size());
						destination_state.state_type = source_state.state_type;
						destination_state.loop		 = source_state.loop;

						for (u32 parameter_index = 0; parameter_index < runtime->parameter_count; ++parameter_index)
						{
							if (parameters[parameter_index].name_hash == source_state.blend_parameter_id)
							{
								destination_state.blend_parameter_index = parameter_index;
								break;
							}
						}

						if (source_state.id == source_asm.first_state_id)
							destination_asm.first_state_index = state_index;

						if (destination_state.clip_count != 0)
						{
							destination_state.clips = mem.allocate_bytes(sizeof(animation_graph_resource_clip_t) * destination_state.clip_count, alignof(animation_graph_resource_clip_t));

							animation_graph_resource_clip_t* clips = mem.get<animation_graph_resource_clip_t>(destination_state.clips);

							for (u32 clip_index = 0; clip_index < destination_state.clip_count; ++clip_index)
							{
								const animation_graph_clip_def_t& source_clip = source_state.clips[clip_index];

								clips[clip_index] = {
									.clip			= source_clip.clip,
									.blend_value_2d = source_clip.blend_value_2d,
									.blend_value	= source_clip.blend_value,
									.playback_speed = source_clip.playback_speed,
								};
							}
						}
					}
				}

				if (destination_asm.transition_count != 0)
				{
					destination_asm.transitions = mem.allocate_bytes(sizeof(animation_graph_resource_transition_t) * destination_asm.transition_count, alignof(animation_graph_resource_transition_t));

					animation_graph_resource_transition_t* transitions = mem.get<animation_graph_resource_transition_t>(destination_asm.transitions);

					for (u32 transition_index = 0; transition_index < destination_asm.transition_count; ++transition_index)
					{
						const animation_graph_asm_transition_def_t& source_transition	   = source_asm.transitions[transition_index];
						animation_graph_resource_transition_t&		destination_transition = transitions[transition_index];

						destination_transition = {
							.compare_value = source_transition.compare_value,
							.duration	   = source_transition.duration,
							.type		   = source_transition.type,
							.is_blended	   = source_transition.is_blended,
						};

						for (u32 state_index = 0; state_index < destination_asm.state_count; ++state_index)
						{
							if (source_asm.states[state_index].id == source_transition.from_state_id)
								destination_transition.from_state_index = state_index;

							if (source_asm.states[state_index].id == source_transition.to_state_id)
								destination_transition.to_state_index = state_index;
						}

						for (u32 parameter_index = 0; parameter_index < runtime->parameter_count; ++parameter_index)
						{
							if (parameters[parameter_index].name_hash == source_transition.parameter_id)
							{
								destination_transition.parameter_index = parameter_index;
								break;
							}
						}
					}
				}
			}
			else if (source_node->type == animation_graph_node_type_e::bone_controller)
			{
				const animation_graph_node_bone_control_def_t& source_bone_control		= source_node->bone_control_node;
				animation_graph_resource_bone_control_t&	   destination_bone_control = destination.bone_control_node;

				destination_bone_control.bone_count	   = static_cast<u32>(source_bone_control.bones.size());
				destination_bone_control.control_type  = source_bone_control.control_type;
				destination_bone_control.control_space = source_bone_control.control_space;

				if (destination_bone_control.bone_count != 0)
				{
					destination_bone_control.bones = mem.allocate_bytes(sizeof(animation_graph_resource_bone_control_entry_t) * destination_bone_control.bone_count, alignof(animation_graph_resource_bone_control_entry_t));

					animation_graph_resource_bone_control_entry_t* bones = mem.get<animation_graph_resource_bone_control_entry_t>(destination_bone_control.bones);

					for (u32 bone_index = 0; bone_index < destination_bone_control.bone_count; ++bone_index)
					{
						const animation_graph_bone_control_entry_def_t& source_bone		 = source_bone_control.bones[bone_index];
						animation_graph_resource_bone_control_entry_t&	destination_bone = bones[bone_index];

						destination_bone = {.bone_index = source_bone.bone_index};

						for (u32 parameter_index = 0; parameter_index < runtime->parameter_count; ++parameter_index)
						{
							if (parameters[parameter_index].name_hash == source_bone.parameter_id)
							{
								destination_bone.parameter_index = parameter_index;
								break;
							}
						}
					}
				}
			}

			source_node = source_node->id == def.output_node_id ? nullptr : def.find_node(source_node->next_node_id);
		}

		return true;
	}

	void animation_graph_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&		   mem	   = ctx.resource_manager.get_memory();
		animation_graph_runtime_t* runtime = mem.get<animation_graph_runtime_t>(entry.runtime);

		if (runtime->node_count != 0)
		{
			animation_graph_resource_node_t* nodes = mem.get<animation_graph_resource_node_t>(runtime->nodes);

			for (u32 node_index = 0; node_index < runtime->node_count; ++node_index)
			{
				animation_graph_resource_node_t& node = nodes[node_index];

				if (node.type == animation_graph_node_type_e::asm_node)
				{
					if (node.asm_node.state_count != 0)
					{
						animation_graph_resource_state_t* states = mem.get<animation_graph_resource_state_t>(node.asm_node.states);

						for (u32 state_index = 0; state_index < node.asm_node.state_count; ++state_index)
						{
							if (states[state_index].clip_count != 0)
								mem.free(states[state_index].clips);
						}

						mem.free(node.asm_node.states);
					}

					if (node.asm_node.transition_count != 0)
						mem.free(node.asm_node.transitions);
				}
				else if (node.type == animation_graph_node_type_e::bone_controller && node.bone_control_node.bone_count != 0)
				{
					mem.free(node.bone_control_node.bones);
				}
			}

			mem.free(runtime->nodes);
		}

		if (runtime->parameter_count != 0)
			mem.free(runtime->parameters);

		*runtime = {};
	}

	const resource_type_desc_t animation_graph_resource_desc = {
		.type				 = resource_type_e::animation_graph,
		.runtime_size		 = sizeof(animation_graph_runtime_t),
		.runtime_alignment	 = alignof(animation_graph_runtime_t),
		.internals_size		 = sizeof(animation_graph_internals_t),
		.internals_alignment = alignof(animation_graph_internals_t),
		.wire_magic			 = animation_graph_loader_t::WIRE_MAGIC,
		.wire_version		 = animation_graph_loader_t::WIRE_VERSION,
		.load				 = animation_graph_loader_t::load,
		.unload				 = animation_graph_loader_t::unload,
	};
}
