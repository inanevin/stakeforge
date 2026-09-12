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
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

namespace sfg
{
#define RAGDOLL_PART_MAX	   64
#define RAGDOLL_PART_NO_PARENT UINT32_MAX

	struct ragdoll_part_def_t
	{
		vec3f_t local_position				   = vec3f_t::zero;
		quat_t	local_rotation				   = quat_t::identity;
		vec3f_t twist_axis					   = vec3f_t::right;
		vec3f_t plane_axis					   = vec3f_t::up;
		f32		radius						   = 0.1f;
		f32		half_height					   = 0.2f;
		f32		mass						   = 5.0f;
		f32		normal_half_cone_angle_degrees = 45.0f;
		f32		plane_half_cone_angle_degrees  = 45.0f;
		f32		twist_min_angle_degrees		   = -45.0f;
		f32		twist_max_angle_degrees		   = 45.0f;
		f32		max_friction_torque			   = 0.0f;
		u32		joint_index					   = UINT32_MAX;
		u32		parent_part_index			   = RAGDOLL_PART_NO_PARENT;
	};

	struct ragdoll_def_t
	{
		vector_t<ragdoll_part_def_t> parts			   = {};
		resource_handle_t			 target_skeleton   = NULL_RESOURCE_HANDLE;
		resource_handle_t			 physical_material = NULL_RESOURCE_HANDLE;
		f32							 gravity_factor	   = 1.0f;
		f32							 linear_damping	   = 0.05f;
		f32							 angular_damping   = 0.05f;
		u8							 allow_sleep	   = 1;
	};

	SFG_DEFINE_TYPE_ID(ragdoll_part_def_t);
	SFG_DEFINE_TYPE_ID(ragdoll_def_t);

	struct ragdoll_def_reflection_t
	{
		ragdoll_def_reflection_t();
	};

	inline ragdoll_def_reflection_t g_reflect_ragdoll_def;
}
