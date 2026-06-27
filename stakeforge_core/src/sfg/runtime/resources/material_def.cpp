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

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t material_parameter_type_values[] = {
			{
				.name		  = "u32",
				.display_name = "U32",
				.value		  = static_cast<i64>(material_parameter_type_e::u32),
			},
			{
				.name		  = "uint2",
				.display_name = "UInt2",
				.value		  = static_cast<i64>(material_parameter_type_e::uint2),
			},
			{
				.name		  = "uint4",
				.display_name = "UInt4",
				.value		  = static_cast<i64>(material_parameter_type_e::uint4),
			},
			{
				.name		  = "i32",
				.display_name = "I32",
				.value		  = static_cast<i64>(material_parameter_type_e::i32),
			},
			{
				.name		  = "f32",
				.display_name = "F32",
				.value		  = static_cast<i64>(material_parameter_type_e::f32),
			},
			{
				.name		  = "vec2f",
				.display_name = "Vec2F",
				.value		  = static_cast<i64>(material_parameter_type_e::vec2f),
			},
			{
				.name		  = "vec4f",
				.display_name = "Vec4F",
				.value		  = static_cast<i64>(material_parameter_type_e::vec4f),
			},
		};
	}

	material_parameter_type_reflection_t::material_parameter_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<material_parameter_type_e>::value) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = material_parameter_type_values, .size = std::size(material_parameter_type_values)},
			.name		 = "material_parameter_type_e",
			.type_id	 = type_id_t<material_parameter_type_e>::value,
			.size		 = sizeof(material_parameter_type_e),
			.alignment	 = alignof(material_parameter_type_e),
		});
	}

	material_parameter_reflection_t::material_parameter_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<material_parameter_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{
				.name		  = "type",
				.display_name = "Type",
				.type		  = reflected_value_type_e::enum8,
				.sub_type_id  = type_id_t<material_parameter_type_e>::value,
				.offset		  = offsetof(material_parameter_t, type),
				.size		  = sizeof(material_parameter_type_e),
			},
			{
				.name		  = "values",
				.display_name = "Values",
				.type		  = reflected_value_type_e::inplace_vector,
				.sub_type_id  = "f32"_hs,
				.offset		  = offsetof(material_parameter_t, values),
				.size		  = sizeof(inplace_vector_t<f32, 4>),
				.capacity	  = 4,
			},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "material_parameter_t",
			.type_id   = type_id_t<material_parameter_t>::value,
			.size	   = sizeof(material_parameter_t),
			.alignment = alignof(material_parameter_t),
		});
	}

	material_def_reflection_t::material_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<material_def_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{
				.name		  = "pass_flags",
				.display_name = "Pass Flags",
				.type		  = reflected_value_type_e::enum32,
				.sub_type_id  = type_id_t<world_pass_flags_e>::value,
				.offset		  = offsetof(material_def_t, pass_flags),
				.size		  = sizeof(world_pass_flags_e),
				.flags		  = reflected_field_flags_bitmask,
			},
			{
				.name		  = "shader",
				.display_name = "Shader",
				.type		  = reflected_value_type_e::shader_handle,
				.offset		  = offsetof(material_def_t, shader),
				.size		  = sizeof(resource_handle_t),
			},
			{
				.name		  = "sampler",
				.display_name = "Sampler",
				.type		  = reflected_value_type_e::texture_sampler_handle,
				.offset		  = offsetof(material_def_t, sampler),
				.size		  = sizeof(resource_handle_t),
			},
			{
				.name		  = "textures",
				.display_name = "Textures",
				.type		  = reflected_value_type_e::inplace_vector,
				.sub_type_id  = "texture_handle"_hs,
				.offset		  = offsetof(material_def_t, textures),
				.size		  = sizeof(inplace_vector_t<resource_handle_t, MATERIAL_MAX_TEXTURES>),
				.capacity	  = MATERIAL_MAX_TEXTURES,
			},
			{
				.name		  = "parameters",
				.display_name = "Parameters",
				.type		  = reflected_value_type_e::inplace_vector,
				.sub_type_id  = type_id_t<material_parameter_t>::value,
				.offset		  = offsetof(material_def_t, parameters),
				.size		  = sizeof(inplace_vector_t<material_parameter_t, MATERIAL_MAX_PARAMETERS>),
				.capacity	  = MATERIAL_MAX_PARAMETERS,
			},
			{
				.name		  = "double_sided",
				.display_name = "Double Sided",
				.type		  = reflected_value_type_e::bool8,
				.offset		  = offsetof(material_def_t, double_sided),
				.size		  = sizeof(bool),
			},
			{
				.name		  = "use_alpha_cutoff",
				.display_name = "Use Alpha Cutoff",
				.type		  = reflected_value_type_e::bool8,
				.offset		  = offsetof(material_def_t, use_alpha_cutoff),
				.size		  = sizeof(bool),
			},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "material_def_t",
			.type_id   = type_id_t<material_def_t>::value,
			.size	   = sizeof(material_def_t),
			.alignment = alignof(material_def_t),
		});
	}
}
