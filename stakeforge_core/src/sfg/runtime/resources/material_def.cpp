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

#include "material_def.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry_v2.hpp>

#include <cstddef>

namespace sfg
{
	material_parameter_type_reflection_t::material_parameter_type_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "material_parameter_type_e",
			.fields =
				{
					{.name = "u32", .display_name = "U32"},
					{.name = "uint2", .display_name = "UInt2"},
					{.name = "uint4", .display_name = "UInt4"},
					{.name = "i32", .display_name = "I32"},
					{.name = "f32", .display_name = "F32"},
					{.name = "vec2f", .display_name = "Vec2F"},
					{.name = "vec4f", .display_name = "Vec4F"},
				},
			.type_id   = type_id_t<material_parameter_type_e>::value,
			.size	   = sizeof(material_parameter_type_e),
			.alignment = alignof(material_parameter_type_e),
			.flags	   = reflected_type_flag_enum,
		});
	}

	material_parameter_reflection_t::material_parameter_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "material_parameter_t",
			.fields =
				{
					{.name = "type", .display_name = "Type", .sub_type_id = type_id_t<material_parameter_type_e>::value, .offset = offsetof(material_parameter_t, type), .size = sizeof(material_parameter_type_e), .type = reflected_value_type_e_v2::u8},
					{.container_ops = reflection_container_ops_t::inplace_vector_ops<f32, 4>(reflected_value_type_e_v2::f32),
					 .name			= "values",
					 .display_name	= "Values",
					 .offset		= offsetof(material_parameter_t, values),
					 .size			= sizeof(inplace_vector_t<f32, 4>),
					 .type			= reflected_value_type_e_v2::container},
				},
			.type_id   = type_id_t<material_parameter_t>::value,
			.size	   = sizeof(material_parameter_t),
			.alignment = alignof(material_parameter_t),
		});
	}

	material_def_reflection_t::material_def_reflection_t()
	{
		reflection_registry_v2& registry = reflection_registry_v2::get();

		registry.register_type({
			.name = "material_def_t",
			.fields =
				{
					{.name = "pass_flags", .display_name = "Pass Flags", .sub_type_id = type_id_t<world_pass_flags_e>::value, .offset = offsetof(material_def_t, pass_flags), .size = sizeof(bitmask_t<u32>), .type = reflected_value_type_e_v2::u32},
					{.name = "shader", .display_name = "Shader", .sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID, .offset = offsetof(material_def_t, shader), .size = sizeof(resource_handle_t), .type = reflected_value_type_e_v2::u64},
					{.name = "sampler", .display_name = "Sampler", .sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID, .offset = offsetof(material_def_t, sampler), .size = sizeof(resource_handle_t), .type = reflected_value_type_e_v2::u64},
					{.container_ops = reflection_container_ops_t::inplace_vector_ops<resource_handle_t, MATERIAL_MAX_TEXTURES>(reflected_value_type_e_v2::u64, REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID),
					 .name			= "textures",
					 .display_name	= "Textures",
					 .offset		= offsetof(material_def_t, textures),
					 .size			= sizeof(inplace_vector_t<resource_handle_t, MATERIAL_MAX_TEXTURES>),
					 .type			= reflected_value_type_e_v2::container},
					{.container_ops = reflection_container_ops_t::inplace_vector_ops<material_parameter_t, MATERIAL_MAX_PARAMETERS>(reflected_value_type_e_v2::object, type_id_t<material_parameter_t>::value),
					 .name			= "parameters",
					 .display_name	= "Parameters",
					 .offset		= offsetof(material_def_t, parameters),
					 .size			= sizeof(inplace_vector_t<material_parameter_t, MATERIAL_MAX_PARAMETERS>),
					 .type			= reflected_value_type_e_v2::container},
					{.name = "double_sided", .display_name = "Double Sided", .offset = offsetof(material_def_t, double_sided), .size = sizeof(bool), .type = reflected_value_type_e_v2::boolean},
					{.name = "use_alpha_cutoff", .display_name = "Use Alpha Cutoff", .offset = offsetof(material_def_t, use_alpha_cutoff), .size = sizeof(bool), .type = reflected_value_type_e_v2::boolean},
				},
			.type_id   = type_id_t<material_def_t>::value,
			.size	   = sizeof(material_def_t),
			.alignment = alignof(material_def_t),
		});
	}
}
