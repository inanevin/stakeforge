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

#include "skeleton_def.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	skeleton_joint_def_reflection_t::skeleton_joint_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "skeleton_joint_def_t",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(skeleton_joint_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e_v2::string},
					{.name = "name_hash", .display_name = "Name Hash", .offset = offsetof(skeleton_joint_def_t, name_hash), .size = sizeof(sid_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e_v2::u64},
					{.name = "parent_index", .display_name = "Parent Index", .offset = offsetof(skeleton_joint_def_t, parent_index), .size = sizeof(u32), .type = reflected_value_type_e_v2::u32},
					{.name = "inverse_bind", .display_name = "Inverse Bind", .sub_type_id = type_id_t<mat4x3_t>::value, .offset = offsetof(skeleton_joint_def_t, inverse_bind), .size = sizeof(mat4x3_t), .type = reflected_value_type_e_v2::object},
				},
			.type_id   = type_id_t<skeleton_joint_def_t>::value,
			.size	   = sizeof(skeleton_joint_def_t),
			.alignment = alignof(skeleton_joint_def_t),
		});
	}

	skeleton_def_reflection_t::skeleton_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "skeleton_def_t",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(skeleton_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e_v2::string},
					{.container_ops = reflection_container_ops_t::vector_ops<skeleton_joint_def_t>(reflected_value_type_e_v2::object, type_id_t<skeleton_joint_def_t>::value),
					 .name			= "joints",
					 .display_name	= "Joints",
					 .offset		= offsetof(skeleton_def_t, joints),
					 .size			= sizeof(vector_t<skeleton_joint_def_t>),
					 .type			= reflected_value_type_e_v2::container},
					{.name = "root_index", .display_name = "Root Index", .offset = offsetof(skeleton_def_t, root_joint_index), .size = sizeof(u32), .type = reflected_value_type_e_v2::u32},
				},
			.type_id   = type_id_t<skeleton_def_t>::value,
			.size	   = sizeof(skeleton_def_t),
			.alignment = alignof(skeleton_def_t),
		});
	}
}
