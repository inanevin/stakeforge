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
	bool skeleton_def_t::build_evaluation_order()
	{
		evaluation_order.resize(0);

		const u32 joint_count = static_cast<u32>(joints.size());
		if (joint_count == 0 || root_joint_index >= joint_count)
			return false;

		vector_t<u32> first_child = {};
		first_child.resize(joint_count, SKELETON_JOINT_NO_PARENT);

		vector_t<u32> next_sibling = {};
		next_sibling.resize(joint_count, SKELETON_JOINT_NO_PARENT);

		for (u32 i = joint_count; i != 0; --i)
		{
			const u32 joint_index  = i - 1;
			const u32 parent_index = joints[joint_index].parent_index;
			if (parent_index == SKELETON_JOINT_NO_PARENT)
				continue;

			if (parent_index >= joint_count)
			{
				evaluation_order.resize(0);
				return false;
			}

			next_sibling[joint_index] = first_child[parent_index];
			first_child[parent_index] = joint_index;
		}

		evaluation_order.reserve(joint_count);

		if (joints[root_joint_index].parent_index == SKELETON_JOINT_NO_PARENT)
			evaluation_order.push_back(root_joint_index);

		for (u32 i = 0; i < joint_count; ++i)
		{
			if (i != root_joint_index && joints[i].parent_index == SKELETON_JOINT_NO_PARENT)
				evaluation_order.push_back(i);
		}

		for (u32 i = 0; i < evaluation_order.size(); ++i)
		{
			const u32 joint_index = evaluation_order[i];

			for (u32 child_index = first_child[joint_index]; child_index != SKELETON_JOINT_NO_PARENT; child_index = next_sibling[child_index])
				evaluation_order.push_back(child_index);
		}

		if (evaluation_order.size() != joint_count)
		{
			evaluation_order.resize(0);
			return false;
		}

		return true;
	}

	bool skeleton_def_t::is_evaluation_order_valid() const
	{
		const u32 joint_count = static_cast<u32>(joints.size());
		if (joint_count == 0 || root_joint_index >= joint_count || evaluation_order.size() != joint_count)
			return false;

		vector_t<u8> evaluated = {};
		evaluated.resize(joint_count);

		for (const u32 joint_index : evaluation_order)
		{
			if (joint_index >= joint_count || evaluated[joint_index] != 0)
				return false;

			const u32 parent_index = joints[joint_index].parent_index;
			if (parent_index != SKELETON_JOINT_NO_PARENT && (parent_index >= joint_count || evaluated[parent_index] == 0))
				return false;

			evaluated[joint_index] = 1;
		}

		return true;
	}

	skeleton_joint_def_reflection_t::skeleton_joint_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "skeleton_joint_def_t",
			.fields =
				{
					{.name = "name", .display_name = "Name", .offset = offsetof(skeleton_joint_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.name = "name_hash", .display_name = "Name Hash", .offset = offsetof(skeleton_joint_def_t, name_hash), .size = sizeof(sid_t), .flags = reflected_field_flag_no_ui, .type = reflected_value_type_e::u64},
					{.name = "parent_index", .display_name = "Parent Index", .offset = offsetof(skeleton_joint_def_t, parent_index), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					{.name = "local", .display_name = "Local", .sub_type_id = type_id_t<mat4x3_t>::value, .offset = offsetof(skeleton_joint_def_t, local), .size = sizeof(mat4x3_t), .type = reflected_value_type_e::object},
					{.name = "inverse_bind", .display_name = "Inverse Bind", .sub_type_id = type_id_t<mat4x3_t>::value, .offset = offsetof(skeleton_joint_def_t, inverse_bind), .size = sizeof(mat4x3_t), .type = reflected_value_type_e::object},
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
					{.name = "name", .display_name = "Name", .offset = offsetof(skeleton_def_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
					{.container_ops = reflection_container_ops_t::vector_ops<skeleton_joint_def_t>(reflected_value_type_e::object, type_id_t<skeleton_joint_def_t>::value),
					 .name			= "joints",
					 .display_name	= "Joints",
					 .offset		= offsetof(skeleton_def_t, joints),
					 .size			= sizeof(vector_t<skeleton_joint_def_t>),
					 .type			= reflected_value_type_e::container},
					{.container_ops = reflection_container_ops_t::vector_ops<u32>(reflected_value_type_e::u32),
					 .name			= "evaluation_order",
					 .display_name	= "Evaluation Order",
					 .offset		= offsetof(skeleton_def_t, evaluation_order),
					 .size			= sizeof(vector_t<u32>),
					 .type			= reflected_value_type_e::container},
					{.name = "root_index", .display_name = "Root Index", .offset = offsetof(skeleton_def_t, root_joint_index), .size = sizeof(u32), .type = reflected_value_type_e::u32},
				},
			.type_id   = type_id_t<skeleton_def_t>::value,
			.size	   = sizeof(skeleton_def_t),
			.alignment = alignof(skeleton_def_t),
		});
	}
}
