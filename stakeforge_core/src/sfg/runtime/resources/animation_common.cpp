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
					{.name = "time", .display_name = "Time", .offset = offsetof(animation_keyframe_v3_t, time), .size = sizeof(f32), .type = reflected_value_type_e_v2::f32},
					{.name = "value", .display_name = "Value", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_t, value), .size = sizeof(vec3f_t), .type = reflected_value_type_e_v2::object},
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
					{.name = "time", .display_name = "Time", .offset = offsetof(animation_keyframe_v3_spline_t, time), .size = sizeof(f32), .type = reflected_value_type_e_v2::f32},
					{.name = "in_tangent", .display_name = "In Tangent", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, in_tangent), .size = sizeof(vec3f_t), .type = reflected_value_type_e_v2::object},
					{.name = "value", .display_name = "Value", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, value), .size = sizeof(vec3f_t), .type = reflected_value_type_e_v2::object},
					{.name = "out_tangent", .display_name = "Out Tangent", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, out_tangent), .size = sizeof(vec3f_t), .type = reflected_value_type_e_v2::object},
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
					{.name = "time", .display_name = "Time", .offset = offsetof(animation_keyframe_q_t, time), .size = sizeof(f32), .type = reflected_value_type_e_v2::f32},
					{.name = "value", .display_name = "Value", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(animation_keyframe_q_t, value), .size = sizeof(quat_t), .type = reflected_value_type_e_v2::object},
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
					{.name = "time", .display_name = "Time", .offset = offsetof(animation_keyframe_q_spline_t, time), .size = sizeof(f32), .type = reflected_value_type_e_v2::f32},
					{.name = "in_tangent", .display_name = "In Tangent", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(animation_keyframe_q_spline_t, in_tangent), .size = sizeof(quat_t), .type = reflected_value_type_e_v2::object},
					{.name = "value", .display_name = "Value", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(animation_keyframe_q_spline_t, value), .size = sizeof(quat_t), .type = reflected_value_type_e_v2::object},
					{.name = "out_tangent", .display_name = "Out Tangent", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(animation_keyframe_q_spline_t, out_tangent), .size = sizeof(quat_t), .type = reflected_value_type_e_v2::object},
				},
			.type_id   = type_id_t<animation_keyframe_q_spline_t>::value,
			.size	   = sizeof(animation_keyframe_q_spline_t),
			.alignment = alignof(animation_keyframe_q_spline_t),
		});
	}
}
