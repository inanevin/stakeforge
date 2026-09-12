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
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

namespace sfg
{
#define SKELETON_JOINT_NO_PARENT UINT32_MAX

	struct skeleton_joint_def_t
	{
		mat4x3_t local		  = mat4x3_t::identity;
		mat4x3_t inverse_bind = mat4x3_t::identity;
		string_t name		  = {};
		sid_t	 name_hash	  = NULL_SID;
		u32		 parent_index = SKELETON_JOINT_NO_PARENT;
	};

	struct skeleton_slot_def_t
	{
		resource_handle_t preview_mesh	   = NULL_RESOURCE_HANDLE;
		char			  slot_name[128]   = {};
		u32				  slot_joint_index = SKELETON_JOINT_NO_PARENT;
		vec3f_t			  local_position   = vec3f_t::zero;
		quat_t			  local_rotation   = quat_t::identity;
		vec3f_t			  preview_scale	   = vec3f_t::one;
	};

	struct skeleton_mask_def_t
	{
		vector_t<u32> joint_indices = {};
		char		  name[256]		= {};
	};

	struct skeleton_def_t
	{
		vector_t<skeleton_joint_def_t> joints			  = {};
		vector_t<u32>				   evaluation_order	  = {};
		vector_t<skeleton_slot_def_t>  slots			  = {};
		vector_t<skeleton_mask_def_t>  masks			  = {};
		string_t					   name				  = {};
		mat4x3_t					   skinning_transform = mat4x3_t::identity;
		aabb_t						   local_bounds		  = {};
		resource_handle_t			   preview_mesh		  = NULL_RESOURCE_HANDLE;
		resource_handle_t			   preview_animation  = NULL_RESOURCE_HANDLE;
		u32							   root_joint_index	  = UINT32_MAX;

		bool build_evaluation_order();
		bool is_evaluation_order_valid() const;
	};

	SFG_DEFINE_TYPE_ID(skeleton_joint_def_t);
	SFG_DEFINE_TYPE_ID(skeleton_slot_def_t);
	SFG_DEFINE_TYPE_ID(skeleton_mask_def_t);
	SFG_DEFINE_TYPE_ID(skeleton_def_t);

	struct skeleton_joint_def_reflection_t
	{
		skeleton_joint_def_reflection_t();
	};

	struct skeleton_slot_def_reflection_t
	{
		skeleton_slot_def_reflection_t();
	};

	struct skeleton_mask_def_reflection_t
	{
		skeleton_mask_def_reflection_t();
	};

	struct skeleton_def_reflection_t
	{
		skeleton_def_reflection_t();
	};

	inline skeleton_joint_def_reflection_t g_reflect_skeleton_joint_def;
	inline skeleton_slot_def_reflection_t  g_reflect_skeleton_slot_def;
	inline skeleton_mask_def_reflection_t  g_reflect_skeleton_mask_def;
	inline skeleton_def_reflection_t	   g_reflect_skeleton_def;
}
