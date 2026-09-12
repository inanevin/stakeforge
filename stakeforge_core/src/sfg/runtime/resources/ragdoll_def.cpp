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

#include "ragdoll_def.hpp"
#include "resource_type.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	ragdoll_def_reflection_t::ragdoll_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "ragdoll_part_def_t",
			.fields =
				{
					{.name = "local_position", .display_name = "Position", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(ragdoll_part_def_t, local_position), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "local_rotation", .display_name = "Rotation", .sub_type_id = type_id_t<quat_t>::value, .offset = offsetof(ragdoll_part_def_t, local_rotation), .size = sizeof(quat_t), .type = reflected_value_type_e::object},
					{.name = "twist_axis", .display_name = "Twist Axis", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(ragdoll_part_def_t, twist_axis), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "plane_axis", .display_name = "Plane Axis", .sub_type_id = type_id_t<vec3f_t>::value, .offset = offsetof(ragdoll_part_def_t, plane_axis), .size = sizeof(vec3f_t), .type = reflected_value_type_e::object},
					{.name = "radius", .display_name = "Radius", .offset = offsetof(ragdoll_part_def_t, radius), .size = sizeof(f32), .flags = reflected_field_flag_clamped, .min_clamp = 0.001f, .max_clamp = 10000.0f, .type = reflected_value_type_e::f32},
					{.name		   = "half_height",
					 .display_name = "Half Height",
					 .offset	   = offsetof(ragdoll_part_def_t, half_height),
					 .size		   = sizeof(f32),
					 .flags		   = reflected_field_flag_clamped,
					 .min_clamp	   = 0.001f,
					 .max_clamp	   = 10000.0f,
					 .type		   = reflected_value_type_e::f32},
					{.name = "mass", .display_name = "Mass", .offset = offsetof(ragdoll_part_def_t, mass), .size = sizeof(f32), .flags = reflected_field_flag_clamped, .min_clamp = 0.001f, .max_clamp = 1000000.0f, .type = reflected_value_type_e::f32},
					{.name		   = "normal_half_cone_angle_degrees",
					 .display_name = "Normal Cone",
					 .offset	   = offsetof(ragdoll_part_def_t, normal_half_cone_angle_degrees),
					 .size		   = sizeof(f32),
					 .flags		   = reflected_field_flag_clamped,
					 .min_clamp	   = 0.0f,
					 .max_clamp	   = 180.0f,
					 .type		   = reflected_value_type_e::f32},
					{.name		   = "plane_half_cone_angle_degrees",
					 .display_name = "Plane Cone",
					 .offset	   = offsetof(ragdoll_part_def_t, plane_half_cone_angle_degrees),
					 .size		   = sizeof(f32),
					 .flags		   = reflected_field_flag_clamped,
					 .min_clamp	   = 0.0f,
					 .max_clamp	   = 180.0f,
					 .type		   = reflected_value_type_e::f32},
					{.name		   = "twist_min_angle_degrees",
					 .display_name = "Twist Min",
					 .offset	   = offsetof(ragdoll_part_def_t, twist_min_angle_degrees),
					 .size		   = sizeof(f32),
					 .flags		   = reflected_field_flag_clamped,
					 .min_clamp	   = -180.0f,
					 .max_clamp	   = 180.0f,
					 .type		   = reflected_value_type_e::f32},
					{.name		   = "twist_max_angle_degrees",
					 .display_name = "Twist Max",
					 .offset	   = offsetof(ragdoll_part_def_t, twist_max_angle_degrees),
					 .size		   = sizeof(f32),
					 .flags		   = reflected_field_flag_clamped,
					 .min_clamp	   = -180.0f,
					 .max_clamp	   = 180.0f,
					 .type		   = reflected_value_type_e::f32},
					{.name		   = "max_friction_torque",
					 .display_name = "Friction Torque",
					 .offset	   = offsetof(ragdoll_part_def_t, max_friction_torque),
					 .size		   = sizeof(f32),
					 .flags		   = reflected_field_flag_clamped,
					 .min_clamp	   = 0.0f,
					 .max_clamp	   = 1000000.0f,
					 .type		   = reflected_value_type_e::f32},
					{.name = "joint_index", .display_name = "Joint", .offset = offsetof(ragdoll_part_def_t, joint_index), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					{.name = "parent_part_index", .display_name = "Parent Part", .offset = offsetof(ragdoll_part_def_t, parent_part_index), .size = sizeof(u32), .type = reflected_value_type_e::u32},
				},
			.type_id   = type_id_t<ragdoll_part_def_t>::value,
			.size	   = sizeof(ragdoll_part_def_t),
			.alignment = alignof(ragdoll_part_def_t),
		});

		registry.register_type({
			.name		  = "ragdoll_def_t",
			.display_name = "Ragdoll",
			.fields =
				{
					{.name		   = "target_skeleton",
					 .display_name = "Target Skeleton",
					 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SKELETON,
					 .offset	   = offsetof(ragdoll_def_t, target_skeleton),
					 .size		   = sizeof(resource_handle_t),
					 .type		   = reflected_value_type_e::u64},
					{.name		   = "physical_material",
					 .display_name = "Physical Material",
					 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICAL_MATERIAL,
					 .offset	   = offsetof(ragdoll_def_t, physical_material),
					 .size		   = sizeof(resource_handle_t),
					 .type		   = reflected_value_type_e::u64},
					{.container_ops = reflection_container_ops_t::vector_ops<ragdoll_part_def_t>(reflected_value_type_e::object, type_id_t<ragdoll_part_def_t>::value),
					 .name			= "parts",
					 .display_name	= "Parts",
					 .offset		= offsetof(ragdoll_def_t, parts),
					 .size			= sizeof(vector_t<ragdoll_part_def_t>),
					 .type			= reflected_value_type_e::container},
					{.name = "gravity_factor", .display_name = "Gravity", .offset = offsetof(ragdoll_def_t, gravity_factor), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name		   = "linear_damping",
					 .display_name = "Linear Damping",
					 .offset	   = offsetof(ragdoll_def_t, linear_damping),
					 .size		   = sizeof(f32),
					 .flags		   = reflected_field_flag_clamped,
					 .min_clamp	   = 0.0f,
					 .max_clamp	   = 1.0f,
					 .type		   = reflected_value_type_e::f32},
					{.name		   = "angular_damping",
					 .display_name = "Angular Damping",
					 .offset	   = offsetof(ragdoll_def_t, angular_damping),
					 .size		   = sizeof(f32),
					 .flags		   = reflected_field_flag_clamped,
					 .min_clamp	   = 0.0f,
					 .max_clamp	   = 1.0f,
					 .type		   = reflected_value_type_e::f32},
					{.name = "allow_sleep", .display_name = "Allow Sleep", .offset = offsetof(ragdoll_def_t, allow_sleep), .size = sizeof(u8), .type = reflected_value_type_e::boolean},
				},
			.type_id   = type_id_t<ragdoll_def_t>::value,
			.size	   = sizeof(ragdoll_def_t),
			.alignment = alignof(ragdoll_def_t),
		});
	}
}
