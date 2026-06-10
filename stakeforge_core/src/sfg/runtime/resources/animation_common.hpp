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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>

namespace sfg
{
	enum class animation_interpolation_e : u8
	{
		linear,
		step,
		cubic_spline,
	};

	struct animation_keyframe_v3_t
	{
		vec3f_t value = vec3f_t::zero;
		f32		time  = 0.0f;
	};

	struct animation_keyframe_v3_spline_t
	{
		vec3f_t in_tangent	= vec3f_t::zero;
		vec3f_t value		= vec3f_t::zero;
		vec3f_t out_tangent = vec3f_t::zero;
		f32		time		= 0.0f;
	};

	struct animation_keyframe_q_t
	{
		quat_t value = quat_t::identity;
		f32	   time	 = 0.0f;
	};

	struct animation_keyframe_q_spline_t
	{
		quat_t in_tangent  = quat_t::identity;
		quat_t value	   = quat_t::identity;
		quat_t out_tangent = quat_t::identity;
		f32	   time		   = 0.0f;
	};

	SFG_DEFINE_TYPE_ID(animation_interpolation_e);
	SFG_DEFINE_TYPE_ID(animation_keyframe_v3_t);
	SFG_DEFINE_TYPE_ID(animation_keyframe_v3_spline_t);
	SFG_DEFINE_TYPE_ID(animation_keyframe_q_t);
	SFG_DEFINE_TYPE_ID(animation_keyframe_q_spline_t);

	struct animation_interpolation_reflection_t
	{
		animation_interpolation_reflection_t();
	};

	struct animation_keyframe_v3_reflection_t
	{
		animation_keyframe_v3_reflection_t();
	};

	struct animation_keyframe_v3_spline_reflection_t
	{
		animation_keyframe_v3_spline_reflection_t();
	};

	struct animation_keyframe_q_reflection_t
	{
		animation_keyframe_q_reflection_t();
	};

	struct animation_keyframe_q_spline_reflection_t
	{
		animation_keyframe_q_spline_reflection_t();
	};

	inline animation_interpolation_reflection_t		 g_reflect_animation_interpolation;
	inline animation_keyframe_v3_reflection_t		 g_reflect_animation_keyframe_v3;
	inline animation_keyframe_v3_spline_reflection_t g_reflect_animation_keyframe_v3_spline;
	inline animation_keyframe_q_reflection_t		 g_reflect_animation_keyframe_q;
	inline animation_keyframe_q_spline_reflection_t	 g_reflect_animation_keyframe_q_spline;
}
