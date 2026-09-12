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

#pragma once

#include <sfg/common/type_id.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	enum class curve_type_e : u8
	{
		x,
		xy,
		xyz,
		xyzw,
		color,
	};

	SFG_DEFINE_TYPE_ID(curve_type_e);

	enum class curve_interpolation_e : u8
	{
		linear,
		step,
	};

	SFG_DEFINE_TYPE_ID(curve_interpolation_e);

	struct curve_key_t
	{
		vec4f_t value = vec4f_t::zero;
		f32		time  = 0.0f;

		bool operator==(const curve_key_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(curve_key_t);

	struct curve_def_t
	{
		vector_t<curve_key_t> keys = {
			{.value = vec4f_t::zero, .time = 0.0f},
			{.value = vec4f_t::one, .time = 1.0f},
		};
		u32					  granularity	= 256;
		curve_type_e		  type			= curve_type_e::x;
		curve_interpolation_e interpolation = curve_interpolation_e::linear;

		vec4f_t evaluate(f32 time) const;
		bool	operator==(const curve_def_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(curve_def_t);

	struct curve_reflection_t
	{
		curve_reflection_t();
	};

	inline curve_reflection_t g_reflect_curve;
}
