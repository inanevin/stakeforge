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
