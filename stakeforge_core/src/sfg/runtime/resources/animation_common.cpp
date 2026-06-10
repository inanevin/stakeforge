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
#include <iterator>

namespace sfg
{
	animation_interpolation_reflection_t::animation_interpolation_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<animation_interpolation_e>::value) != nullptr)
			return;

		static const reflected_enum_value_desc_t values[] = {
			{.name = "linear", .display_name = "Linear", .id = "linear"_hs, .value = static_cast<i64>(animation_interpolation_e::linear)},
			{.name = "step", .display_name = "Step", .id = "step"_hs, .value = static_cast<i64>(animation_interpolation_e::step)},
			{.name = "cubic_spline", .display_name = "Cubic Spline", .id = "cubic_spline"_hs, .value = static_cast<i64>(animation_interpolation_e::cubic_spline)},
		};

		registry.register_type({
			.enum_values = {.data = values, .size = std::size(values)},
			.name		 = "animation_interpolation_e",
			.type_id	 = type_id_t<animation_interpolation_e>::value,
			.size		 = sizeof(animation_interpolation_e),
			.alignment	 = alignof(animation_interpolation_e),
		});
	}

	animation_keyframe_v3_reflection_t::animation_keyframe_v3_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<animation_keyframe_v3_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "time", .display_name = "Time", .type = reflected_value_type_e::f32, .offset = offsetof(animation_keyframe_v3_t, time), .size = sizeof(f32)},
			{.name = "value", .display_name = "Value", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_t, value), .size = sizeof(vec3f_t)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "animation_keyframe_v3_t",
			.type_id   = type_id_t<animation_keyframe_v3_t>::value,
			.size	   = sizeof(animation_keyframe_v3_t),
			.alignment = alignof(animation_keyframe_v3_t),
		});
	}

	animation_keyframe_v3_spline_reflection_t::animation_keyframe_v3_spline_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<animation_keyframe_v3_spline_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "time", .display_name = "Time", .type = reflected_value_type_e::f32, .offset = offsetof(animation_keyframe_v3_spline_t, time), .size = sizeof(f32)},
			{.name = "in_tangent", .display_name = "In Tangent", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, in_tangent), .size = sizeof(vec3f_t)},
			{.name = "value", .display_name = "Value", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, value), .size = sizeof(vec3f_t)},
			{.name = "out_tangent", .display_name = "Out Tangent", .type = reflected_value_type_e::object, .value_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(animation_keyframe_v3_spline_t, out_tangent), .size = sizeof(vec3f_t)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "animation_keyframe_v3_spline_t",
			.type_id   = type_id_t<animation_keyframe_v3_spline_t>::value,
			.size	   = sizeof(animation_keyframe_v3_spline_t),
			.alignment = alignof(animation_keyframe_v3_spline_t),
		});
	}

	animation_keyframe_q_reflection_t::animation_keyframe_q_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<animation_keyframe_q_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "time", .display_name = "Time", .type = reflected_value_type_e::f32, .offset = offsetof(animation_keyframe_q_t, time), .size = sizeof(f32)},
			{.name = "value", .display_name = "Value", .type = reflected_value_type_e::quat, .offset = offsetof(animation_keyframe_q_t, value), .size = sizeof(quat_t)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "animation_keyframe_q_t",
			.type_id   = type_id_t<animation_keyframe_q_t>::value,
			.size	   = sizeof(animation_keyframe_q_t),
			.alignment = alignof(animation_keyframe_q_t),
		});
	}

	animation_keyframe_q_spline_reflection_t::animation_keyframe_q_spline_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<animation_keyframe_q_spline_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "time", .display_name = "Time", .type = reflected_value_type_e::f32, .offset = offsetof(animation_keyframe_q_spline_t, time), .size = sizeof(f32)},
			{.name = "in_tangent", .display_name = "In Tangent", .type = reflected_value_type_e::quat, .offset = offsetof(animation_keyframe_q_spline_t, in_tangent), .size = sizeof(quat_t)},
			{.name = "value", .display_name = "Value", .type = reflected_value_type_e::quat, .offset = offsetof(animation_keyframe_q_spline_t, value), .size = sizeof(quat_t)},
			{.name = "out_tangent", .display_name = "Out Tangent", .type = reflected_value_type_e::quat, .offset = offsetof(animation_keyframe_q_spline_t, out_tangent), .size = sizeof(quat_t)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "animation_keyframe_q_spline_t",
			.type_id   = type_id_t<animation_keyframe_q_spline_t>::value,
			.size	   = sizeof(animation_keyframe_q_spline_t),
			.alignment = alignof(animation_keyframe_q_spline_t),
		});
	}
}
