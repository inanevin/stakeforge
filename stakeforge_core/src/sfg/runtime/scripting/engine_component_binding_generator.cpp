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

#include "engine_component_binding_generator.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/resource_type.hpp>
#include <sfg/serialization/serialization.hpp>

namespace sfg
{
	namespace
	{
		string_t get_generated_type_name(const reflected_type_t& type)
		{
			string_t   native_name	= type.name;
			const bool is_component = type.flags.is_set(reflected_type_flag_component) || type.flags.is_set(reflected_type_flag_tag_component);

			if (is_component && native_name.starts_with("component_"))
				native_name.erase(0, 10);

			if (native_name.ends_with("_t") || native_name.ends_with("_e"))
				native_name.resize(native_name.size() - 2);

			string_t name = string_util::to_pascal_case(native_name.c_str());

			if (is_component)
				name += "Component";

			return name;
		}

		const char* get_resource_handle_type(sid_t sub_type_id)
		{
			switch (resource_type_from_reflection_sub_type_id(sub_type_id))
			{
			case resource_type_e::audio:
				return "AudioHandle";
			case resource_type_e::font:
				return "FontHandle";
			case resource_type_e::mesh:
				return "MeshHandle";
			case resource_type_e::skeleton:
				return "SkeletonHandle";
			case resource_type_e::animation:
				return "AnimationHandle";
			case resource_type_e::material:
				return "MaterialHandle";
			case resource_type_e::shader:
				return "ShaderHandle";
			case resource_type_e::texture:
				return "TextureHandle";
			case resource_type_e::texture_sampler:
				return "TextureSamplerHandle";
			case resource_type_e::physical_material:
				return "PhysicalMaterialHandle";
			case resource_type_e::prefab:
				return "PrefabHandle";
			case resource_type_e::animation_graph:
				return "AnimationGraphHandle";
			case resource_type_e::cubemap:
				return "CubemapHandle";
			case resource_type_e::physics_collision_mesh:
				return "PhysicsCollisionMeshHandle";
			case resource_type_e::sprite:
				return "SpriteHandle";
			case resource_type_e::curve:
				return "CurveHandle";
			default:
				return nullptr;
			}
		}

		const char* get_integer_type(reflected_value_type_e value_type)
		{
			switch (value_type)
			{
			case reflected_value_type_e::u64:
				return "ulong";
			case reflected_value_type_e::i64:
				return "long";
			case reflected_value_type_e::u32:
				return "uint";
			case reflected_value_type_e::i32:
				return "int";
			case reflected_value_type_e::u16:
				return "ushort";
			case reflected_value_type_e::i16:
				return "short";
			case reflected_value_type_e::u8:
				return "byte";
			case reflected_value_type_e::i8:
				return "sbyte";
			default:
				return nullptr;
			}
		}

		bool get_field_type(const reflection_registry_t& registry, const reflected_field_t& field, string_t& out_type)
		{
			out_type.resize(0);

			if (field.sub_type_id != 0)
			{
				if (field.sub_type_id == REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID)
				{
					out_type = "EntityGuid";
					return true;
				}

				const char* resource_type = get_resource_handle_type(field.sub_type_id);

				if (resource_type != nullptr)
				{
					out_type = resource_type;
					return true;
				}

				const reflected_type_t* sub_type = registry.find_type(field.sub_type_id);

				if (sub_type != nullptr && sub_type->flags.is_set(reflected_type_flag_enum))
				{
					out_type = get_generated_type_name(*sub_type);
					return true;
				}
			}

			switch (field.value_type)
			{
			case reflected_value_type_e::f32:
				out_type = "float";
				return true;
			case reflected_value_type_e::boolean:
				out_type = "bool";
				return true;
			case reflected_value_type_e::bitmask: {
				const char* integer_type = field.size == 8 ? "ulong" : field.size == 4 ? "uint" : field.size == 2 ? "ushort" : "byte";
				out_type				 = integer_type;
				return true;
			}
			case reflected_value_type_e::object:
				if (field.sub_type_id == "vec2f_t"_hs)
					out_type = "Vector2";
				else if (field.sub_type_id == "vec3f_t"_hs)
					out_type = "Vector3";
				else if (field.sub_type_id == "vec4f_t"_hs)
					out_type = "Vector4";
				else if (field.sub_type_id == "quat_t"_hs)
					out_type = "Quaternion";
				else if (field.sub_type_id == "mat4x3_t"_hs)
					out_type = "Matrix4x3";
				else if (field.sub_type_id == "mat4x4_t"_hs)
					out_type = "Matrix4x4";
				else
				{
					const reflected_type_t* sub_type = registry.find_type(field.sub_type_id);

					if (sub_type == nullptr)
						return false;

					out_type = get_generated_type_name(*sub_type);
				}
				return true;
			default: {
				const char* integer_type = get_integer_type(field.value_type);

				if (integer_type == nullptr)
					return false;

				out_type = integer_type;
				return true;
			}
			}
		}

		const char* get_enum_underlying_type(size_t size)
		{
			if (size == 1)
				return "byte";
			if (size == 2)
				return "ushort";
			if (size == 8)
				return "ulong";

			return "uint";
		}

		void append_enum(const reflection_registry_t& registry, const reflected_type_t& type, string_t& output)
		{
			output += "public enum ";
			output += get_generated_type_name(type);
			output += " : ";
			output += get_enum_underlying_type(type.size);
			output += "\n{\n";

			u32 value = 0;

			for (u32 field_index = type.fields.start; field_index < type.fields.end; ++field_index)
			{
				const reflected_field_t* field = registry.get_field(field_index);
				SFG_ASSERT(field != nullptr);

				output += "    ";
				output += string_util::to_pascal_case(field->name);
				output += " = ";
				output += std::to_string(value);
				output += ",\n";
				value++;
			}

			output += "}\n\n";
		}

		void append_struct(const reflection_registry_t& registry, const reflected_type_t& type, string_t& output)
		{
			const bool	 is_component = type.flags.is_set(reflected_type_flag_component) || type.flags.is_set(reflected_type_flag_tag_component);
			const bool	 is_tag		  = type.flags.is_set(reflected_type_flag_tag_component);
			const size_t layout_size  = is_tag ? 1 : type.size;

			output += "[StructLayout(LayoutKind.Explicit, Size = ";
			output += std::to_string(layout_size);
			output += ")]\n";

			if (is_component)
			{
				output += "[Component(";
				output += std::to_string(type.type_id);
				output += "UL, ";
				output += std::to_string(is_tag ? 0 : type.size);
				output += "U, true)]\n";
			}

			output += "public struct ";
			output += get_generated_type_name(type);
			output += "\n{\n";

			for (u32 field_index = type.fields.start; field_index < type.fields.end; ++field_index)
			{
				const reflected_field_t* field = registry.get_field(field_index);
				SFG_ASSERT(field != nullptr);

				string_t field_type = {};

				if (!get_field_type(registry, *field, field_type))
					continue;

				output += "    [FieldOffset(";
				output += std::to_string(field->offset);
				output += ")]\n";

				if (field->value_type == reflected_value_type_e::boolean)
					output += "    [MarshalAs(UnmanagedType.U1)]\n";

				output += "    public ";
				output += field_type;
				output += " ";
				output += string_util::to_pascal_case(field->name);
				output += ";\n\n";
			}

			output += "}\n\n";
		}
	}

	bool engine_component_binding_generator_t::generate(const char* output_path)
	{
		SFG_ASSERT(output_path != nullptr);

		const reflection_registry_t&	  registry		  = reflection_registry_t::get();
		const vector_t<reflected_type_t>& reflected_types = registry.get_types();
		vector_t<const reflected_type_t*> generated_types = {};
		vector_t<const reflected_type_t*> generated_enums = {};
		generated_types.reserve(reflected_types.size());
		generated_enums.reserve(reflected_types.size());

		for (const reflected_type_t& type : reflected_types)
		{
			const bool is_component = type.flags.is_set(reflected_type_flag_component) || type.flags.is_set(reflected_type_flag_tag_component);

			if (type.owner == reflection_owner_e::engine && is_component)
				generated_types.push_back(&type);
		}

		for (size_t type_index = 0; type_index < generated_types.size(); ++type_index)
		{
			const reflected_type_t& type = *generated_types[type_index];

			for (u32 field_index = type.fields.start; field_index < type.fields.end; ++field_index)
			{
				const reflected_field_t* field = registry.get_field(field_index);
				SFG_ASSERT(field != nullptr);

				if (field->sub_type_id == 0)
					continue;

				const reflected_type_t* sub_type = registry.find_type(field->sub_type_id);

				if (sub_type == nullptr || sub_type->owner != reflection_owner_e::engine)
					continue;

				if (sub_type->flags.is_set(reflected_type_flag_enum))
				{
					if (std::find(generated_enums.begin(), generated_enums.end(), sub_type) == generated_enums.end())
						generated_enums.push_back(sub_type);
				}
				else if (field->value_type == reflected_value_type_e::object && field->sub_type_id != "vec2f_t"_hs && field->sub_type_id != "vec3f_t"_hs && field->sub_type_id != "vec4f_t"_hs && field->sub_type_id != "quat_t"_hs &&
						 field->sub_type_id != "mat4x3_t"_hs && field->sub_type_id != "mat4x4_t"_hs)
				{
					if (std::find(generated_types.begin(), generated_types.end(), sub_type) == generated_types.end())
						generated_types.push_back(sub_type);
				}
			}
		}

		string_t output = "using System.Runtime.InteropServices;\n\nnamespace SFG.Components;\n\n";

		for (const reflected_type_t* type : generated_enums)
			append_enum(registry, *type, output);

		for (const reflected_type_t* type : generated_types)
			append_struct(registry, *type, output);

		if (file_system_t::exists(output_path))
		{
			const string_t existing = file_system_t::read_file_as_string(output_path);

			if (existing == output)
				return true;
		}

		return serializer_t::write_to_file(output, output_path);
	}
}
