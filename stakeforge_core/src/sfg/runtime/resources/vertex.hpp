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
