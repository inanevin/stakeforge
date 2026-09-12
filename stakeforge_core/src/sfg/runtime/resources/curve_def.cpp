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

#include "curve_def.hpp"
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	vec4f_t curve_def_t::evaluate(f32 time) const
	{
		if (keys.empty())
			return vec4f_t::zero;

		if (keys.size() == 1 || time <= keys.front().time)
			return keys.front().value;

		for (size_t key_index = 1; key_index < keys.size(); ++key_index)
		{
			const curve_key_t& right = keys[key_index];

			if (time > right.time)
				continue;

			const curve_key_t& left = keys[key_index - 1];

			if (interpolation == curve_interpolation_e::step)
				return left.value;

			const f32 duration = math::max(right.time - left.time, MATH_EPS);
			const f32 factor   = math::clamp((time - left.time) / duration, 0.0f, 1.0f);
			return left.value + (right.value - left.value) * factor;
		}

		return keys.back().value;
	}

	curve_reflection_t::curve_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "curve_type_e",
			.fields =
				{
					{.name = "x", .display_name = "X"},
					{.name = "xy", .display_name = "XY"},
					{.name = "xyz", .display_name = "XYZ"},
					{.name = "xyzw", .display_name = "XYZW"},
					{.name = "color", .display_name = "Color"},
				},
			.type_id   = type_id_t<curve_type_e>::value,
			.size	   = sizeof(curve_type_e),
			.alignment = alignof(curve_type_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name = "curve_interpolation_e",
			.fields =
				{
					{.name = "linear", .display_name = "Linear"},
					{.name = "step", .display_name = "Step"},
				},
			.type_id   = type_id_t<curve_interpolation_e>::value,
			.size	   = sizeof(curve_interpolation_e),
			.alignment = alignof(curve_interpolation_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name			 = "curve_key_t",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<curve_key_t*>(ptr), curve_key_t{}); },
			.fields =
				{
					{.name				= "time",
					 .display_name		= "Time",
					 .tooltip			= "Normalized time from zero to one.",
					 .offset			= offsetof(curve_key_t, time),
					 .size				= sizeof(f32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 0.0f,
					 .max_clamp			= 1.0f,
					 .clamp_granularity = 0.001f,
					 .type				= reflected_value_type_e::f32},
					{.name = "value", .display_name = "Value", .tooltip = "Four-channel value at this key.", .sub_type_id = type_id_t<vec4f_t>::value, .offset = offsetof(curve_key_t, value), .size = sizeof(vec4f_t), .type = reflected_value_type_e::object},
				},
			.type_id   = type_id_t<curve_key_t>::value,
			.size	   = sizeof(curve_key_t),
			.alignment = alignof(curve_key_t),
		});

		registry.register_type({
			.name			 = "curve_def_t",
			.display_name	 = "Curve",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<curve_def_t*>(ptr), curve_def_t{}); },
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops<curve_key_t>(reflected_value_type_e::object, type_id_t<curve_key_t>::value),
					 .name			= "keys",
					 .display_name	= "Keys",
					 .tooltip		= "Ordered curve keys edited by the curve graph.",
					 .offset		= offsetof(curve_def_t, keys),
					 .size			= sizeof(vector_t<curve_key_t>),
					 .flags			= reflected_field_flag_no_ui,
					 .type			= reflected_value_type_e::container},
					{.name		   = "type",
					 .display_name = "Type",
					 .tooltip	   = "Number and meaning of visible value channels.",
					 .sub_type_id  = type_id_t<curve_type_e>::value,
					 .offset	   = offsetof(curve_def_t, type),
					 .size		   = sizeof(curve_type_e),
					 .type		   = reflected_value_type_e::u8},
					{.name		   = "interpolation",
					 .display_name = "Mode",
					 .tooltip	   = "Interpolation used between adjacent keys.",
					 .sub_type_id  = type_id_t<curve_interpolation_e>::value,
					 .offset	   = offsetof(curve_def_t, interpolation),
					 .size		   = sizeof(curve_interpolation_e),
					 .type		   = reflected_value_type_e::u8},
					{.name				= "granularity",
					 .display_name		= "Granularity",
					 .tooltip			= "Number of runtime samples stored for fast lookup.",
					 .offset			= offsetof(curve_def_t, granularity),
					 .size				= sizeof(u32),
					 .flags				= reflected_field_flag_clamped,
					 .min_clamp			= 2.0f,
					 .max_clamp			= 4096.0f,
					 .clamp_granularity = 1.0f,
					 .type				= reflected_value_type_e::u32},
				},
			.type_id   = type_id_t<curve_def_t>::value,
			.size	   = sizeof(curve_def_t),
			.alignment = alignof(curve_def_t),
		});
	}
}
