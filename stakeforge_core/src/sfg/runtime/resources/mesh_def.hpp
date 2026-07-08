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

#include "vertex.hpp"
#include <sfg/common/type_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/aabb.hpp>

namespace sfg
{
	struct primitive_static_def_t
	{
		vector_t<vertex_static_t> vertices		 = {};
		vector_t<primitive_index> indices		 = {};
		u32						  material_index = UINT32_MAX;
	};

	struct primitive_skinned_def_t
	{
		vector_t<vertex_skinned_t> vertices		  = {};
		vector_t<primitive_index>  indices		  = {};
		u32						   material_index = UINT32_MAX;
	};

	struct mesh_def_t
	{
		aabb_t							  local_bounds		 = {};
		string_t						  name				 = {};
		vector_t<primitive_static_def_t>  static_primitives	 = {};
		vector_t<primitive_skinned_def_t> skinned_primitives = {};
	};

	SFG_DEFINE_TYPE_ID(primitive_static_def_t);
	SFG_DEFINE_TYPE_ID(primitive_skinned_def_t);
	SFG_DEFINE_TYPE_ID(mesh_def_t);

	struct primitive_static_def_reflection_t
	{
		primitive_static_def_reflection_t();
	};

	struct primitive_skinned_def_reflection_t
	{
		primitive_skinned_def_reflection_t();
	};

	struct mesh_def_reflection_t
	{
		mesh_def_reflection_t();
	};

	inline primitive_static_def_reflection_t  g_reflect_primitive_static_def;
	inline primitive_skinned_def_reflection_t g_reflect_primitive_skinned_def;
	inline mesh_def_reflection_t			  g_reflect_mesh_def;
}
