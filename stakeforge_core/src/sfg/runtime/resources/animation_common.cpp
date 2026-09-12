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

#include "animation_common.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	animation_interpolation_reflection_t::animation_interpolation_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_interpolation_e",
			.fields =
				{
					{.name = "linear", .display_name = "Linear"},
					{.name = "step", .display_name = "Step"},
					{.name = "cubic_spline", .display_name = "Cubic Spline"},
				},
			.type_id   = type_id_t<animation_interpolation_e>::value,
			.size	   = sizeof(animation_interpolation_e),
			.alignment = alignof(animation_interpolation_e),
			.flags	   = reflected_type_flag_enum,
		});
	}

	animation_keyframe_v3_reflection_t::animation_keyframe_v3_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_keyframe_v3_t",
			.fields =
				{
					{.name = "time", .display_name = "Time", .offset = offsetof(animation_keyframe_v3_t, time), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "value", .display_name = "Value", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_t, value), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<animation_keyframe_v3_t>::value,
			.size	   = sizeof(animation_keyframe_v3_t),
			.alignment = alignof(animation_keyframe_v3_t),
		});
	}

	animation_keyframe_v3_spline_reflection_t::animation_keyframe_v3_spline_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_keyframe_v3_spline_t",
			.fields =
				{
					{.name = "time", .display_name = "Time", .offset = offsetof(animation_keyframe_v3_spline_t, time), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "in_tangent", .display_name = "In Tangent", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, in_tangent), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "value", .display_name = "Value", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, value), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "out_tangent", .display_name = "Out Tangent", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, out_tangent), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<animation_keyframe_v3_spline_t>::value,
			.size	   = sizeof(animation_keyframe_v3_spline_t),
			.alignment = alignof(animation_keyframe_v3_spline_t),
		});
	}

	animation_keyframe_q_reflection_t::animation_keyframe_q_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_keyframe_q_t",
			.fields =
				{
					{.name = "time", .display_name = "Time", .offset = offsetof(animation_keyframe_q_t, time), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "value", .display_name = "Value", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(animation_keyframe_q_t, value), .size = sizeof(quat_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<animation_keyframe_q_t>::value,
			.size	   = sizeof(animation_keyframe_q_t),
			.alignment = alignof(animation_keyframe_q_t),
		});
	}

	animation_keyframe_q_spline_reflection_t::animation_keyframe_q_spline_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "animation_keyframe_q_spline_t",
			.fields =
				{
					{.name = "time", .display_name = "Time", .offset = offsetof(animation_keyframe_q_spline_t, time), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "in_tangent", .display_name = "In Tangent", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(animation_keyframe_q_spline_t, in_tangent), .size = sizeof(quat_t), .type = reflected_value_type_e::object},
					{.name = "value", .display_name = "Value", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(animation_keyframe_q_spline_t, value), .size = sizeof(quat_t), .type = reflected_value_type_e::object},
					{.name = "out_tangent", .display_name = "Out Tangent", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(animation_keyframe_q_spline_t, out_tangent), .size = sizeof(quat_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<animation_keyframe_q_spline_t>::value,
			.size	   = sizeof(animation_keyframe_q_spline_t),
			.alignment = alignof(animation_keyframe_q_spline_t),
		});
	}
}
