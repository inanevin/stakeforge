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

#include "vertex.hpp"
#include <sfg/common/type_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

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
		vector_t<resource_handle_t>		  preview_materials	 = {};
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
