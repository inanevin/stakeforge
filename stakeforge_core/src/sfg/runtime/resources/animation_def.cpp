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

#include "animation_def.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	animation_channel_v3_def_reflection_t::animation_channel_v3_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_channel_v3_def_t",
			.fields =
				{
					{.name		   = "interpolation",
					 .display_name = "Interpolation",
					 .sub_type_id  = type_id_t<animation_interpolation_e>::value,
					 .offset	   = offsetof(animation_channel_v3_def_t, interpolation),
					 .size		   = sizeof(animation_interpolation_e),
					 .type		   = reflected_value_type_e::u8},
					{.name = "node_index", .display_name = "Node Index", .offset = offsetof(animation_channel_v3_def_t, node_index), .size = sizeof(i32), .type = reflected_value_type_e::i32},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_keyframe_v3_t>(reflected_value_type_e::object, type_id_t<animation_keyframe_v3_t>::value),
					 .name			= "keyframes",
					 .display_name	= "Keyframes",
					 .offset		= offsetof(animation_channel_v3_def_t, keyframes),
					 .size			= sizeof(vector_t<animation_keyframe_v3_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_keyframe_v3_spline_t>(reflected_value_type_e::object, type_id_t<animation_keyframe_v3_spline_t>::value),
					 .name			= "keyframes_spline",
					 .display_name	= "Spline Keyframes",
					 .offset		= offsetof(animation_channel_v3_def_t, keyframes_spline),
					 .size			= sizeof(vector_t<animation_keyframe_v3_spline_t>),
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<animation_channel_v3_def_t>::value,
			.size	   = sizeof(animation_channel_v3_def_t),
			.alignment = alignof(animation_channel_v3_def_t),
		});
	}

	animation_channel_q_def_reflection_t::animation_channel_q_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_channel_q_def_t",
			.fields =
				{
					{.name		   = "interpolation",
					 .display_name = "Interpolation",
					 .sub_type_id  = type_id_t<animation_interpolation_e>::value,
					 .offset	   = offsetof(animation_channel_q_def_t, interpolation),
					 .size		   = sizeof(animation_interpolation_e),
					 .type		   = reflected_value_type_e::u8},
					{.name = "node_index", .display_name = "Node Index", .offset = offsetof(animation_channel_q_def_t, node_index), .size = sizeof(i32), .type = reflected_value_type_e::i32},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_keyframe_q_t>(reflected_value_type_e::object, type_id_t<animation_keyframe_q_t>::value),
					 .name			= "keyframes",
					 .display_name	= "Keyframes",
					 .offset		= offsetof(animation_channel_q_def_t, keyframes),
					 .size			= sizeof(vector_t<animation_keyframe_q_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_keyframe_q_spline_t>(reflected_value_type_e::object, type_id_t<animation_keyframe_q_spline_t>::value),
					 .name			= "keyframes_spline",
					 .display_name	= "Spline Keyframes",
					 .offset		= offsetof(animation_channel_q_def_t, keyframes_spline),
					 .size			= sizeof(vector_t<animation_keyframe_q_spline_t>),
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<animation_channel_q_def_t>::value,
			.size	   = sizeof(animation_channel_q_def_t),
			.alignment = alignof(animation_channel_q_def_t),
		});
	}

	animation_def_reflection_t::animation_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_def_t",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(animation_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.name = "name_hash", .display_name = "Name Hash", .offset = offsetof(animation_def_t, name_hash), .size = sizeof(sid_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u64},
					{.name = "duration", .display_name = "Duration", .offset = offsetof(animation_def_t, duration), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_channel_v3_def_t>(reflected_value_type_e::object, type_id_t<animation_channel_v3_def_t>::value),
					 .name			= "position_channels",
					 .display_name	= "Position Channels",
					 .offset		= offsetof(animation_def_t, position_channels),
					 .size			= sizeof(vector_t<animation_channel_v3_def_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_channel_q_def_t>(reflected_value_type_e::object, type_id_t<animation_channel_q_def_t>::value),
					 .name			= "rotation_channels",
					 .display_name	= "Rotation Channels",
					 .offset		= offsetof(animation_def_t, rotation_channels),
					 .size			= sizeof(vector_t<animation_channel_q_def_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<animation_channel_v3_def_t>(reflected_value_type_e::object, type_id_t<animation_channel_v3_def_t>::value),
					 .name			= "scale_channels",
					 .display_name	= "Scale Channels",
					 .offset		= offsetof(animation_def_t, scale_channels),
					 .size			= sizeof(vector_t<animation_channel_v3_def_t>),
					 .type			= reflected_value_type_e::container},
				},
			.type_id   = type_id_t<animation_def_t>::value,
			.size	   = sizeof(animation_def_t),
			.alignment = alignof(animation_def_t),
		});
	}
}
