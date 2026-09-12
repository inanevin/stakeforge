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

#include "animation_def.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/resource_type.hpp>

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

	animation_event_def_reflection_t::animation_event_def_reflection_t()
	{
		reflection_registry_t::get().register_type({
			.name = "animation_event_def_t",
			.fields =
				{
					{
						.name		  = "name",
						.display_name = "Name",
						.offset		  = offsetof(animation_event_def_t, name),
						.size		  = sizeof(animation_event_def_t::name),
						.type		  = reflected_value_type_e::char_array,
					},
					{
						.name		  = "time",
						.display_name = "Time (s)",
						.offset		  = offsetof(animation_event_def_t, time),
						.size		  = sizeof(f32),
						.type		  = reflected_value_type_e::f32,
					},
				},
			.type_id   = type_id_t<animation_event_def_t>::value,
			.size	   = sizeof(animation_event_def_t),
			.alignment = alignof(animation_event_def_t),
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
					{.name		   = "preview_mesh",
					 .display_name = "Preview Mesh",
					 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MESH,
					 .offset	   = offsetof(animation_def_t, preview_mesh),
					 .size		   = sizeof(resource_handle_t),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::u64},
					{.name		   = "preview_skeleton",
					 .display_name = "Preview Skeleton",
					 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SKELETON,
					 .offset	   = offsetof(animation_def_t, preview_skeleton),
					 .size		   = sizeof(resource_handle_t),
					 .flags		   = reflected_field_flag_no_ui,
					 .type		   = reflected_value_type_e::u64},
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
					{
						.container_ops = reflection_container_ops_t::vector_ops<animation_event_def_t>(reflected_value_type_e::object, type_id_t<animation_event_def_t>::value),
						.name		   = "events",
						.display_name  = "Events",
						.offset		   = offsetof(animation_def_t, events),
						.size		   = sizeof(vector_t<animation_event_def_t>),
						.type		   = reflected_value_type_e::container,
					},
				},
			.type_id   = type_id_t<animation_def_t>::value,
			.size	   = sizeof(animation_def_t),
			.alignment = alignof(animation_def_t),
		});
	}
}
