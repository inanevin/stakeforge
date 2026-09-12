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
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/math/vec4u.hpp>

namespace sfg
{
	struct vertex_static_t
	{
		vec3f_t pos		= vec3f_t::zero;
		vec3f_t normal	= vec3f_t::zero;
		vec4f_t tangent = vec4f_t::zero;
		vec2f_t uv		= vec2f_t::zero;
	};

	struct vertex_skinned_t
	{
		vec3f_t pos			 = vec3f_t::zero;
		vec3f_t normal		 = vec3f_t::zero;
		vec4f_t tangent		 = vec4f_t::zero;
		vec2f_t uv			 = vec2f_t::zero;
		vec4f_t bone_weights = vec4f_t::zero;
		vec4u_t bone_indices = vec4u_t::zero;
	};

	SFG_DEFINE_TYPE_ID(vertex_static_t);
	SFG_DEFINE_TYPE_ID(vertex_skinned_t);

	struct vertex_static_reflection_t
	{
		vertex_static_reflection_t();
	};

	struct vertex_skinned_reflection_t
	{
		vertex_skinned_reflection_t();
	};

	inline vertex_static_reflection_t  g_reflect_vertex_static;
	inline vertex_skinned_reflection_t g_reflect_vertex_skinned;

	struct vertex_debug_line_t
	{
		vec4f_t color				= vec4f_t::zero;
		vec3f_t position			= vec3f_t::zero;
		vec3f_t other_position		= vec3f_t::zero;
		f32		corner				= 0.0f;
		f32		signed_thickness_px = 1.0f;
	};

	struct vertex_debug_triangle_t
	{
		vec3f_t position = vec3f_t::zero;
		vec4f_t color	 = vec4f_t::zero;
	};

	struct vertex_debug_text_t
	{
		vec4f_t color  = vec4f_t::zero;
		vec3f_t anchor = vec3f_t::zero;
		vec2f_t offset = vec2f_t::zero;
		vec2f_t uv	   = vec2f_t::zero;
		f32		mode   = 0.0f;
	};
}
