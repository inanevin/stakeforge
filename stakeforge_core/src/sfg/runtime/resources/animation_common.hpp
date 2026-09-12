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
