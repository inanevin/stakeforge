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

#include "reflection_registry.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec3u.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/math/vec4u.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <cstring>
#include <memory>

namespace sfg
{
#define REFLECTION_REGISTRY_MAX_TYPES		512
#define REFLECTION_REGISTRY_MAX_FIELDS		4096
#define REFLECTION_REGISTRY_MAX_ENUM_VALUES 1024
#define REFLECTION_REGISTRY_TEXT_BYTES		(1024 * 128)

	reflected_value_type_e reflected_value_type_from_sub_type_id(sid_t sub_type_id)
	{
		if (sub_type_id == "f32"_hs)
			return reflected_value_type_e::f32;
		if (sub_type_id == "i32"_hs)
			return reflected_value_type_e::i32;
		if (sub_type_id == "i16"_hs)
			return reflected_value_type_e::i16;
		if (sub_type_id == "i8"_hs)
			return reflected_value_type_e::i8;
		if (sub_type_id == "u32"_hs)
			return reflected_value_type_e::u32;
		if (sub_type_id == "u64"_hs)
			return reflected_value_type_e::u64;
		if (sub_type_id == "u16"_hs)
			return reflected_value_type_e::u16;
		if (sub_type_id == "u8"_hs)
			return reflected_value_type_e::u8;
		if (sub_type_id == "size_t"_hs)
			return reflected_value_type_e::size_t;
		if (sub_type_id == "bool"_hs || sub_type_id == "bool8"_hs)
			return reflected_value_type_e::bool8;
		if (sub_type_id == "audio_handle"_hs)
			return reflected_value_type_e::audio_handle;
		if (sub_type_id == "font_handle"_hs)
			return reflected_value_type_e::font_handle;
		if (sub_type_id == "mesh_handle"_hs)
			return reflected_value_type_e::mesh_handle;
		if (sub_type_id == "skeleton_handle"_hs)
			return reflected_value_type_e::skeleton_handle;
		if (sub_type_id == "animation_handle"_hs)
			return reflected_value_type_e::animation_handle;
		if (sub_type_id == "material_handle"_hs)
			return reflected_value_type_e::material_handle;
		if (sub_type_id == "shader_handle"_hs)
			return reflected_value_type_e::shader_handle;
		if (sub_type_id == "texture_handle"_hs)
			return reflected_value_type_e::texture_handle;
		if (sub_type_id == "texture_sampler_handle"_hs)
			return reflected_value_type_e::texture_sampler_handle;
		if (sub_type_id == "physical_material_handle"_hs)
			return reflected_value_type_e::physical_material_handle;
		if (sub_type_id == "prefab_handle"_hs)
			return reflected_value_type_e::prefab_handle;
		if (sub_type_id == "animation_state_machine_handle"_hs)
			return reflected_value_type_e::animation_state_machine_handle;
		if (sub_type_id == "hdr_skybox_handle"_hs)
			return reflected_value_type_e::hdr_skybox_handle;
		if (sub_type_id == "entity_guid"_hs)
			return reflected_value_type_e::entity_guid;
		if (sub_type_id == "text_id"_hs)
			return reflected_value_type_e::text_id;
		if (sub_type_id == "string"_hs)
			return reflected_value_type_e::string;
		if (sub_type_id == "json"_hs)
			return reflected_value_type_e::json;
		if (sub_type_id == "quat"_hs)
			return reflected_value_type_e::quat;
		if (sub_type_id == "enum8"_hs)
			return reflected_value_type_e::enum8;
		if (sub_type_id == "enum32"_hs || sub_type_id == "enum"_hs)
			return reflected_value_type_e::enum32;
		return reflected_value_type_e::invalid;
	}

	u32 reflected_value_type_size(reflected_value_type_e type)
	{
		if (reflection_registry_t::is_resource_type(type))
			return sizeof(sid_t);

		switch (type)
		{
		case reflected_value_type_e::f32:
			return sizeof(f32);
		case reflected_value_type_e::i32:
			return sizeof(i32);
		case reflected_value_type_e::i16:
			return sizeof(i16);
		case reflected_value_type_e::i8:
			return sizeof(i8);
		case reflected_value_type_e::u32:
			return sizeof(u32);
		case reflected_value_type_e::u64:
			return sizeof(u64);
		case reflected_value_type_e::u16:
			return sizeof(u16);
		case reflected_value_type_e::size_t:
			return sizeof(size_t);
		case reflected_value_type_e::u8:
		case reflected_value_type_e::bool8:
		case reflected_value_type_e::enum8:
			return sizeof(u8);
		case reflected_value_type_e::entity_guid:
			return sizeof(u64);
		case reflected_value_type_e::text_id:
		case reflected_value_type_e::enum32:
			return sizeof(u32);
		case reflected_value_type_e::string:
			return sizeof(string_t);
		case reflected_value_type_e::json:
			return sizeof(nlohmann::json);
		case reflected_value_type_e::quat:
			return sizeof(quat_t);
		case reflected_value_type_e::object:
			return 0;
		default:
			return 0;
		}
	}

	namespace
	{
		bool read_reflected_text(const void* object, const reflected_field_desc_t& field, const char*& value)
		{
			if (field.size == sizeof(string_t))
			{
				value = reinterpret_cast<const string_t*>(static_cast<const u8*>(object) + field.offset)->c_str();
				return true;
			}

			value = reinterpret_cast<const char*>(static_cast<const u8*>(object) + field.offset);
			return true;
		}

		bool write_reflected_text(void* object, const reflected_field_desc_t& field, const char* value)
		{
			const char* src = value != nullptr ? value : "";
			if (field.size == sizeof(string_t))
			{
				*reinterpret_cast<string_t*>(static_cast<u8*>(object) + field.offset) = src;
				return true;
			}

			SFG_ASSERT(field.size > 0);
			char*		 dst	 = reinterpret_cast<char*>(static_cast<u8*>(object) + field.offset);
			const size_t max_len = static_cast<size_t>(field.size - 1);
			const size_t len	 = std::strlen(src) < max_len ? std::strlen(src) : max_len;
			SFG_MEMCPY(dst, src, len);
			dst[len] = '\0';
			return true;
		}

		bool read_reflected_enum(const void* object, const reflected_field_desc_t& field, i64& value)
		{
			const void* ptr = static_cast<const u8*>(object) + field.offset;
			switch (field.size)
			{
			case sizeof(u8):
				value = static_cast<i64>(*static_cast<const u8*>(ptr));
				break;
			case sizeof(u16):
				value = static_cast<i64>(*static_cast<const u16*>(ptr));
				break;
			case sizeof(u64):
				value = static_cast<i64>(*static_cast<const u64*>(ptr));
				break;
			default:
				value = static_cast<i64>(*static_cast<const u32*>(ptr));
				break;
			}
			return true;
		}

		bool write_reflected_enum(void* object, const reflected_field_desc_t& field, i64 value)
		{
			void* ptr = static_cast<u8*>(object) + field.offset;
			switch (field.size)
			{
			case sizeof(u8):
				*static_cast<u8*>(ptr) = static_cast<u8>(value);
				break;
			case sizeof(u16):
				*static_cast<u16*>(ptr) = static_cast<u16>(value);
				break;
			case sizeof(u64):
				*static_cast<u64*>(ptr) = static_cast<u64>(value);
				break;
			default:
				*static_cast<u32*>(ptr) = static_cast<u32>(value);
				break;
			}
			return true;
		}

		i64 read_reflected_enum_value(const void* object, u32 size)
		{
			switch (size)
			{
			case sizeof(u8):
				return static_cast<i64>(*static_cast<const u8*>(object));
			case sizeof(u16):
				return static_cast<i64>(*static_cast<const u16*>(object));
			case sizeof(u64):
				return static_cast<i64>(*static_cast<const u64*>(object));
			default:
				return static_cast<i64>(*static_cast<const u32*>(object));
			}
		}

		void write_reflected_enum_value(void* object, u32 size, i64 value)
		{
			switch (size)
			{
			case sizeof(u8):
				*static_cast<u8*>(object) = static_cast<u8>(value);
				break;
			case sizeof(u16):
				*static_cast<u16*>(object) = static_cast<u16>(value);
				break;
			case sizeof(u64):
				*static_cast<u64*>(object) = static_cast<u64>(value);
				break;
			default:
				*static_cast<u32*>(object) = static_cast<u32>(value);
				break;
			}
		}

		void stream_reflected_enum_value(ostream_t& stream, u32 size, i64 value)
		{
			switch (size)
			{
			case sizeof(u8):
				stream << static_cast<u8>(value);
				break;
			case sizeof(u16):
				stream << static_cast<u16>(value);
				break;
			case sizeof(u64):
				stream << static_cast<u64>(value);
				break;
			default:
				stream << static_cast<u32>(value);
				break;
			}
		}

		i64 stream_read_reflected_enum_value(istream_t& stream, u32 size)
		{
			switch (size)
			{
			case sizeof(u8): {
				u8 value = 0;
				stream >> value;
				return static_cast<i64>(value);
			}
			case sizeof(u16): {
				u16 value = 0;
				stream >> value;
				return static_cast<i64>(value);
			}
			case sizeof(u64): {
				u64 value = 0;
				stream >> value;
				return static_cast<i64>(value);
			}
			default: {
				u32 value = 0;
				stream >> value;
				return static_cast<i64>(value);
			}
			}
		}

		template <typename T> size_t get_inplace_vector_head_offset(const reflected_field_desc_t& field)
		{
			const size_t data_size = sizeof(T) * field.capacity;
			const size_t alignment = alignof(size_t);
			return (data_size + alignment - 1) & ~(alignment - 1);
		}

		template <typename T> void clear_reflected_inplace_vector(void* object, const reflected_field_desc_t& field)
		{
			T*		data = std::launder(reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset));
			size_t& size = *reinterpret_cast<size_t*>(static_cast<u8*>(object) + field.offset + get_inplace_vector_head_offset<T>(field));
			while (size > 0)
			{
				--size;
				std::destroy_at(data + size);
			}
		}

		template <typename T> void resize_reflected_inplace_vector(void* object, const reflected_field_desc_t& field, u32 size)
		{
			clear_reflected_inplace_vector<T>(object, field);
			T*		data = std::launder(reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset));
			size_t& head = *reinterpret_cast<size_t*>(static_cast<u8*>(object) + field.offset + get_inplace_vector_head_offset<T>(field));
			for (u32 i = 0; i < size; ++i)
			{
				std::construct_at(data + i);
				++head;
			}
		}

		const char* find_enum_name(span_t<const reflected_enum_value_desc_t> enum_values, i64 value)
		{
			for (u32 i = 0; i < enum_values.size; ++i)
			{
				if (enum_values.data[i].value == value)
					return enum_values.data[i].name;
			}
			return nullptr;
		}

		bool find_enum_value(span_t<const reflected_enum_value_desc_t> enum_values, const char* name, i64& out_value)
		{
			for (u32 i = 0; i < enum_values.size; ++i)
			{
				const reflected_enum_value_desc_t& value = enum_values.data[i];
				if ((value.name != nullptr && std::strcmp(value.name, name) == 0) || (value.display_name != nullptr && std::strcmp(value.display_name, name) == 0))
				{
					out_value = value.value;
					return true;
				}
			}
			return false;
		}

		span_t<const reflected_enum_value_desc_t> get_reflected_field_enum_values(const reflected_field_desc_t& field)
		{
			if (field.enum_values.size != 0)
				return field.enum_values;

			sid_t type_id = field.sub_type_id;
			if (type_id == 0 || reflected_value_type_from_sub_type_id(type_id) != reflected_value_type_e::invalid)
				type_id = field.value_type_id;
			if (type_id == 0)
				return {};

			const reflected_type_desc_t* type = reflection_registry_t::get().find_type(type_id);
			return type != nullptr ? type->enum_values : span_t<const reflected_enum_value_desc_t>{};
		}

		bool json_to_vec2(const nlohmann::json& j, vec2f_t& value)
		{
			if (!j.is_array() || j.size() < 2)
				return false;
			value = {j.at(0).get<f32>(), j.at(1).get<f32>()};
			return true;
		}

		bool json_to_vec3(const nlohmann::json& j, vec3f_t& value)
		{
			if (!j.is_array() || j.size() < 3)
				return false;
			value = {j.at(0).get<f32>(), j.at(1).get<f32>(), j.at(2).get<f32>()};
			return true;
		}

		bool json_to_vec4(const nlohmann::json& j, vec4f_t& value)
		{
			if (!j.is_array() || j.size() < 4)
				return false;
			value = {j.at(0).get<f32>(), j.at(1).get<f32>(), j.at(2).get<f32>(), j.at(3).get<f32>()};
			return true;
		}

		bool json_to_vec2u(const nlohmann::json& j, vec2u_t& value)
		{
			if (!j.is_array() || j.size() < 2)
				return false;
			value = {j.at(0).get<u32>(), j.at(1).get<u32>()};
			return true;
		}

		bool json_to_vec2u16(const nlohmann::json& j, vec2u16_t& value)
		{
			if (!j.is_array() || j.size() < 2)
				return false;
			value = {j.at(0).get<u16>(), j.at(1).get<u16>()};
			return true;
		}

		bool json_to_vec3u(const nlohmann::json& j, vec3u_t& value)
		{
			if (!j.is_array() || j.size() < 3)
				return false;
			value = {j.at(0).get<u32>(), j.at(1).get<u32>(), j.at(2).get<u32>()};
			return true;
		}

		bool json_to_vec4u(const nlohmann::json& j, vec4u_t& value)
		{
			if (!j.is_array() || j.size() < 4)
				return false;
			value = {j.at(0).get<u32>(), j.at(1).get<u32>(), j.at(2).get<u32>(), j.at(3).get<u32>()};
			return true;
		}

		bool reflected_math_type_to_json(sid_t type_id, const void* obj, nlohmann::json& j)
		{
			if (type_id == type_id_t<vec2f_t>::value)
			{
				const vec2f_t& value = *static_cast<const vec2f_t*>(obj);
				j					 = nlohmann::json::array_t({value.x, value.y});
				return true;
			}
			if (type_id == type_id_t<vec3f_t>::value)
			{
				const vec3f_t& value = *static_cast<const vec3f_t*>(obj);
				j					 = nlohmann::json::array_t({value.x, value.y, value.z});
				return true;
			}
			if (type_id == type_id_t<vec4f_t>::value)
			{
				const vec4f_t& value = *static_cast<const vec4f_t*>(obj);
				j					 = nlohmann::json::array_t({value.x, value.y, value.z, value.w});
				return true;
			}
			if (type_id == type_id_t<vec2u_t>::value)
			{
				const vec2u_t& value = *static_cast<const vec2u_t*>(obj);
				j					 = nlohmann::json::array_t({value.x, value.y});
				return true;
			}
			if (type_id == type_id_t<vec2u16_t>::value)
			{
				const vec2u16_t& value = *static_cast<const vec2u16_t*>(obj);
				j					   = nlohmann::json::array_t({value.x, value.y});
				return true;
			}
			if (type_id == type_id_t<vec3u_t>::value)
			{
				const vec3u_t& value = *static_cast<const vec3u_t*>(obj);
				j					 = nlohmann::json::array_t({value.x, value.y, value.z});
				return true;
			}
			if (type_id == type_id_t<vec4u_t>::value)
			{
				const vec4u_t& value = *static_cast<const vec4u_t*>(obj);
				j					 = nlohmann::json::array_t({value.x, value.y, value.z, value.w});
				return true;
			}
			return false;
		}

		bool reflected_math_type_from_json(sid_t type_id, void* obj, const nlohmann::json& j)
		{
			if (type_id == type_id_t<vec2f_t>::value)
				return json_to_vec2(j, *static_cast<vec2f_t*>(obj));
			if (type_id == type_id_t<vec3f_t>::value)
				return json_to_vec3(j, *static_cast<vec3f_t*>(obj));
			if (type_id == type_id_t<vec4f_t>::value)
				return json_to_vec4(j, *static_cast<vec4f_t*>(obj));
			if (type_id == type_id_t<vec2u_t>::value)
				return json_to_vec2u(j, *static_cast<vec2u_t*>(obj));
			if (type_id == type_id_t<vec2u16_t>::value)
				return json_to_vec2u16(j, *static_cast<vec2u16_t*>(obj));
			if (type_id == type_id_t<vec3u_t>::value)
				return json_to_vec3u(j, *static_cast<vec3u_t*>(obj));
			if (type_id == type_id_t<vec4u_t>::value)
				return json_to_vec4u(j, *static_cast<vec4u_t*>(obj));
			return false;
		}

		bitmask32 get_reflected_container_item_flags(const reflected_field_desc_t& field)
		{
			bitmask32 flags = reflected_field_flags_none;
			flags.set(reflected_field_flags_clamped, field.flags.is_set(reflected_field_flags_clamped));
			return flags;
		}

		reflected_field_desc_t reflected_container_item_field(const reflected_field_desc_t& field)
		{
			const reflected_value_type_e type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (type != reflected_value_type_e::invalid)
			{
				return {
					.name		   = "value",
					.display_name  = "Value",
					.type		   = type,
					.value_type_id = field.value_type_id,
					.size		   = reflected_value_type_size(type),
					.min		   = field.min,
					.max		   = field.max,
					.flags		   = get_reflected_container_item_flags(field),
				};
			}

			const reflected_type_desc_t* sub_type = reflection_registry_t::get().find_type(field.sub_type_id);
			if (sub_type != nullptr && sub_type->fields.size == 0 && sub_type->enum_values.size != 0)
			{
				return {
					.name		   = "value",
					.display_name  = "Value",
					.type		   = sub_type->size == sizeof(u8) ? reflected_value_type_e::enum8 : reflected_value_type_e::enum32,
					.value_type_id = sub_type->type_id,
					.sub_type_id   = sub_type->type_id,
					.size		   = sub_type->size,
					.min		   = field.min,
					.max		   = field.max,
					.flags		   = get_reflected_container_item_flags(field),
				};
			}

			return {
				.name		   = "value",
				.display_name  = "Value",
				.type		   = sub_type != nullptr ? reflected_value_type_e::object : reflected_value_type_e::invalid,
				.value_type_id = sub_type != nullptr ? sub_type->type_id : 0,
				.size		   = sub_type != nullptr ? sub_type->size : 0,
				.min		   = field.min,
				.max		   = field.max,
				.flags		   = get_reflected_container_item_flags(field),
			};
		}

		bool reflected_field_to_json(const void* object, const reflected_field_desc_t& field, nlohmann::json& j);
		bool reflected_field_from_json(void* object, const reflected_field_desc_t& field, const nlohmann::json& j);
		bool reflected_field_to_stream(const void* object, const reflected_field_desc_t& field, ostream_t& stream);
		bool reflected_field_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream);

		template <typename T> bool reflected_vector_to_json(const vector_t<T>& values, const reflected_field_desc_t& field, nlohmann::json& out)
		{
			const reflected_field_desc_t item_field = reflected_container_item_field(field);
			out										= nlohmann::json::array();
			for (const T& value : values)
			{
				nlohmann::json item = {};
				if (!reflected_field_to_json(&value, item_field, item))
					return false;
				out.push_back(item);
			}
			return true;
		}

		template <typename T> bool reflected_inplace_vector_to_json(const void* object, const reflected_field_desc_t& field, nlohmann::json& out)
		{
			const reflected_field_desc_t item_field = reflected_container_item_field(field);
			out										= nlohmann::json::array();
			const T*	 data						= std::launder(reinterpret_cast<const T*>(static_cast<const u8*>(object) + field.offset));
			const size_t size						= *reinterpret_cast<const size_t*>(static_cast<const u8*>(object) + field.offset + get_inplace_vector_head_offset<T>(field));
			for (size_t i = 0; i < size; ++i)
			{
				nlohmann::json item = {};
				if (!reflected_field_to_json(data + i, item_field, item))
					return false;
				out.push_back(item);
			}
			return true;
		}

		template <typename T> bool reflected_vector_from_json(void* object, const reflected_field_desc_t& field, const nlohmann::json& j)
		{
			if (!j.is_array())
				return true;

			const reflected_field_desc_t item_field = reflected_container_item_field(field);
			vector_t<T>&				 values		= *reinterpret_cast<vector_t<T>*>(static_cast<u8*>(object) + field.offset);
			values.resize(0);
			values.reserve(j.size());
			for (const nlohmann::json& item : j)
			{
				T value = {};
				if (!reflected_field_from_json(&value, item_field, item))
					return false;
				values.push_back(value);
			}
			return true;
		}

		template <typename T> bool reflected_inplace_vector_from_json(void* object, const reflected_field_desc_t& field, const nlohmann::json& j)
		{
			if (!j.is_array())
				return true;
			if (j.size() > field.capacity)
				return false;

			const reflected_field_desc_t item_field = reflected_container_item_field(field);
			resize_reflected_inplace_vector<T>(object, field, static_cast<u32>(j.size()));
			T* data = std::launder(reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset));
			for (u32 i = 0; i < j.size(); ++i)
			{
				if (!reflected_field_from_json(data + i, item_field, j.at(i)))
					return false;
			}
			return true;
		}

		template <typename T> bool reflected_vector_to_stream(const vector_t<T>& values, const reflected_field_desc_t& field, ostream_t& stream)
		{
			const reflected_field_desc_t item_field = reflected_container_item_field(field);
			const u32					 size		= static_cast<u32>(values.size());
			stream << size;
			for (const T& value : values)
			{
				if (!reflected_field_to_stream(&value, item_field, stream))
					return false;
			}
			return true;
		}

		template <typename T> bool reflected_inplace_vector_to_stream(const void* object, const reflected_field_desc_t& field, ostream_t& stream)
		{
			const reflected_field_desc_t item_field = reflected_container_item_field(field);
			const T*					 data		= std::launder(reinterpret_cast<const T*>(static_cast<const u8*>(object) + field.offset));
			const size_t				 size		= *reinterpret_cast<const size_t*>(static_cast<const u8*>(object) + field.offset + get_inplace_vector_head_offset<T>(field));
			stream << static_cast<u32>(size);
			for (size_t i = 0; i < size; ++i)
			{
				if (!reflected_field_to_stream(data + i, item_field, stream))
					return false;
			}
			return true;
		}

		template <typename T> bool reflected_vector_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream)
		{
			u32 size = 0;
			stream >> size;
			const reflected_field_desc_t item_field = reflected_container_item_field(field);
			vector_t<T>&				 values		= *reinterpret_cast<vector_t<T>*>(static_cast<u8*>(object) + field.offset);
			values.resize(size);
			for (T& value : values)
			{
				if (!reflected_field_from_stream(&value, item_field, stream))
					return false;
			}
			return true;
		}

		template <typename T> bool reflected_inplace_vector_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream)
		{
			u32 size = 0;
			stream >> size;
			if (size > field.capacity)
				return false;

			const reflected_field_desc_t item_field = reflected_container_item_field(field);
			resize_reflected_inplace_vector<T>(object, field, size);
			T* data = std::launder(reinterpret_cast<T*>(static_cast<u8*>(object) + field.offset));
			for (u32 i = 0; i < size; ++i)
			{
				if (!reflected_field_from_stream(data + i, item_field, stream))
					return false;
			}
			return true;
		}

		bool reflected_vector_to_json_by_type(const void* object, const reflected_field_desc_t& field, nlohmann::json& j)
		{
			const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (reflection_registry_t::is_resource_type(value_type))
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<sid_t>*>(static_cast<const u8*>(object) + field.offset), field, j);

			switch (value_type)
			{
			case reflected_value_type_e::f32:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<f32>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::i32:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<i32>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::i16:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<i16>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::i8:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<i8>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
			case reflected_value_type_e::enum32:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<u32>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::u64:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<u64>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::u16:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<u16>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::size_t:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<size_t>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::entity_guid:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<u64>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::u8:
			case reflected_value_type_e::bool8:
			case reflected_value_type_e::enum8:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<u8>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::string:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<string_t>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::json:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<nlohmann::json>*>(static_cast<const u8*>(object) + field.offset), field, j);
			case reflected_value_type_e::quat:
				return reflected_vector_to_json(*reinterpret_cast<const vector_t<quat_t>*>(static_cast<const u8*>(object) + field.offset), field, j);
			default:
				return false;
			}
		}

		bool reflected_inplace_vector_to_json_by_type(const void* object, const reflected_field_desc_t& field, nlohmann::json& j)
		{
			const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (reflection_registry_t::is_resource_type(value_type))
				return reflected_inplace_vector_to_json<sid_t>(object, field, j);

			switch (value_type)
			{
			case reflected_value_type_e::f32:
				return reflected_inplace_vector_to_json<f32>(object, field, j);
			case reflected_value_type_e::i32:
				return reflected_inplace_vector_to_json<i32>(object, field, j);
			case reflected_value_type_e::i16:
				return reflected_inplace_vector_to_json<i16>(object, field, j);
			case reflected_value_type_e::i8:
				return reflected_inplace_vector_to_json<i8>(object, field, j);
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
			case reflected_value_type_e::enum32:
				return reflected_inplace_vector_to_json<u32>(object, field, j);
			case reflected_value_type_e::u64:
				return reflected_inplace_vector_to_json<u64>(object, field, j);
			case reflected_value_type_e::u16:
				return reflected_inplace_vector_to_json<u16>(object, field, j);
			case reflected_value_type_e::size_t:
				return reflected_inplace_vector_to_json<size_t>(object, field, j);
			case reflected_value_type_e::entity_guid:
				return reflected_inplace_vector_to_json<u64>(object, field, j);
			case reflected_value_type_e::u8:
			case reflected_value_type_e::bool8:
			case reflected_value_type_e::enum8:
				return reflected_inplace_vector_to_json<u8>(object, field, j);
			case reflected_value_type_e::string:
				return reflected_inplace_vector_to_json<string_t>(object, field, j);
			case reflected_value_type_e::json:
				return reflected_inplace_vector_to_json<nlohmann::json>(object, field, j);
			case reflected_value_type_e::quat:
				return reflected_inplace_vector_to_json<quat_t>(object, field, j);
			default:
				return false;
			}
		}

		bool reflected_vector_from_json_by_type(void* object, const reflected_field_desc_t& field, const nlohmann::json& j)
		{
			const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (reflection_registry_t::is_resource_type(value_type))
				return reflected_vector_from_json<sid_t>(object, field, j);

			switch (value_type)
			{
			case reflected_value_type_e::f32:
				return reflected_vector_from_json<f32>(object, field, j);
			case reflected_value_type_e::i32:
				return reflected_vector_from_json<i32>(object, field, j);
			case reflected_value_type_e::i16:
				return reflected_vector_from_json<i16>(object, field, j);
			case reflected_value_type_e::i8:
				return reflected_vector_from_json<i8>(object, field, j);
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
			case reflected_value_type_e::enum32:
				return reflected_vector_from_json<u32>(object, field, j);
			case reflected_value_type_e::u64:
				return reflected_vector_from_json<u64>(object, field, j);
			case reflected_value_type_e::u16:
				return reflected_vector_from_json<u16>(object, field, j);
			case reflected_value_type_e::size_t:
				return reflected_vector_from_json<size_t>(object, field, j);
			case reflected_value_type_e::entity_guid:
				return reflected_vector_from_json<u64>(object, field, j);
			case reflected_value_type_e::u8:
			case reflected_value_type_e::bool8:
			case reflected_value_type_e::enum8:
				return reflected_vector_from_json<u8>(object, field, j);
			case reflected_value_type_e::string:
				return reflected_vector_from_json<string_t>(object, field, j);
			case reflected_value_type_e::json:
				return reflected_vector_from_json<nlohmann::json>(object, field, j);
			case reflected_value_type_e::quat:
				return reflected_vector_from_json<quat_t>(object, field, j);
			default:
				return false;
			}
		}

		bool reflected_inplace_vector_from_json_by_type(void* object, const reflected_field_desc_t& field, const nlohmann::json& j)
		{
			const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (reflection_registry_t::is_resource_type(value_type))
				return reflected_inplace_vector_from_json<sid_t>(object, field, j);

			switch (value_type)
			{
			case reflected_value_type_e::f32:
				return reflected_inplace_vector_from_json<f32>(object, field, j);
			case reflected_value_type_e::i32:
				return reflected_inplace_vector_from_json<i32>(object, field, j);
			case reflected_value_type_e::i16:
				return reflected_inplace_vector_from_json<i16>(object, field, j);
			case reflected_value_type_e::i8:
				return reflected_inplace_vector_from_json<i8>(object, field, j);
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
			case reflected_value_type_e::enum32:
				return reflected_inplace_vector_from_json<u32>(object, field, j);
			case reflected_value_type_e::u64:
				return reflected_inplace_vector_from_json<u64>(object, field, j);
			case reflected_value_type_e::u16:
				return reflected_inplace_vector_from_json<u16>(object, field, j);
			case reflected_value_type_e::size_t:
				return reflected_inplace_vector_from_json<size_t>(object, field, j);
			case reflected_value_type_e::entity_guid:
				return reflected_inplace_vector_from_json<u64>(object, field, j);
			case reflected_value_type_e::u8:
			case reflected_value_type_e::bool8:
			case reflected_value_type_e::enum8:
				return reflected_inplace_vector_from_json<u8>(object, field, j);
			case reflected_value_type_e::string:
				return reflected_inplace_vector_from_json<string_t>(object, field, j);
			case reflected_value_type_e::json:
				return reflected_inplace_vector_from_json<nlohmann::json>(object, field, j);
			case reflected_value_type_e::quat:
				return reflected_inplace_vector_from_json<quat_t>(object, field, j);
			default:
				return false;
			}
		}

		bool reflected_vector_to_stream_by_type(const void* object, const reflected_field_desc_t& field, ostream_t& stream)
		{
			const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (reflection_registry_t::is_resource_type(value_type))
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<sid_t>*>(static_cast<const u8*>(object) + field.offset), field, stream);

			switch (value_type)
			{
			case reflected_value_type_e::f32:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<f32>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::i32:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<i32>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::i16:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<i16>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::i8:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<i8>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
			case reflected_value_type_e::enum32:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<u32>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::u64:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<u64>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::u16:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<u16>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::size_t:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<size_t>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::entity_guid:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<u64>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::u8:
			case reflected_value_type_e::bool8:
			case reflected_value_type_e::enum8:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<u8>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::string:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<string_t>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::json:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<nlohmann::json>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			case reflected_value_type_e::quat:
				return reflected_vector_to_stream(*reinterpret_cast<const vector_t<quat_t>*>(static_cast<const u8*>(object) + field.offset), field, stream);
			default:
				return false;
			}
		}

		bool reflected_inplace_vector_to_stream(const void* object, const reflected_field_desc_t& field, ostream_t& stream)
		{
			const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (reflection_registry_t::is_resource_type(value_type))
				return reflected_inplace_vector_to_stream<sid_t>(object, field, stream);

			switch (value_type)
			{
			case reflected_value_type_e::f32:
				return reflected_inplace_vector_to_stream<f32>(object, field, stream);
			case reflected_value_type_e::i32:
				return reflected_inplace_vector_to_stream<i32>(object, field, stream);
			case reflected_value_type_e::i16:
				return reflected_inplace_vector_to_stream<i16>(object, field, stream);
			case reflected_value_type_e::i8:
				return reflected_inplace_vector_to_stream<i8>(object, field, stream);
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
			case reflected_value_type_e::enum32:
				return reflected_inplace_vector_to_stream<u32>(object, field, stream);
			case reflected_value_type_e::u64:
				return reflected_inplace_vector_to_stream<u64>(object, field, stream);
			case reflected_value_type_e::u16:
				return reflected_inplace_vector_to_stream<u16>(object, field, stream);
			case reflected_value_type_e::size_t:
				return reflected_inplace_vector_to_stream<size_t>(object, field, stream);
			case reflected_value_type_e::entity_guid:
				return reflected_inplace_vector_to_stream<u64>(object, field, stream);
			case reflected_value_type_e::u8:
			case reflected_value_type_e::bool8:
			case reflected_value_type_e::enum8:
				return reflected_inplace_vector_to_stream<u8>(object, field, stream);
			case reflected_value_type_e::string:
				return reflected_inplace_vector_to_stream<string_t>(object, field, stream);
			case reflected_value_type_e::json:
				return reflected_inplace_vector_to_stream<nlohmann::json>(object, field, stream);
			case reflected_value_type_e::quat:
				return reflected_inplace_vector_to_stream<quat_t>(object, field, stream);
			default:
				return false;
			}
		}

		bool reflected_vector_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream)
		{
			const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (reflection_registry_t::is_resource_type(value_type))
				return reflected_vector_from_stream<sid_t>(object, field, stream);

			switch (value_type)
			{
			case reflected_value_type_e::f32:
				return reflected_vector_from_stream<f32>(object, field, stream);
			case reflected_value_type_e::i32:
				return reflected_vector_from_stream<i32>(object, field, stream);
			case reflected_value_type_e::i16:
				return reflected_vector_from_stream<i16>(object, field, stream);
			case reflected_value_type_e::i8:
				return reflected_vector_from_stream<i8>(object, field, stream);
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
			case reflected_value_type_e::enum32:
				return reflected_vector_from_stream<u32>(object, field, stream);
			case reflected_value_type_e::u64:
				return reflected_vector_from_stream<u64>(object, field, stream);
			case reflected_value_type_e::u16:
				return reflected_vector_from_stream<u16>(object, field, stream);
			case reflected_value_type_e::size_t:
				return reflected_vector_from_stream<size_t>(object, field, stream);
			case reflected_value_type_e::entity_guid:
				return reflected_vector_from_stream<u64>(object, field, stream);
			case reflected_value_type_e::u8:
			case reflected_value_type_e::bool8:
			case reflected_value_type_e::enum8:
				return reflected_vector_from_stream<u8>(object, field, stream);
			case reflected_value_type_e::string:
				return reflected_vector_from_stream<string_t>(object, field, stream);
			case reflected_value_type_e::json:
				return reflected_vector_from_stream<nlohmann::json>(object, field, stream);
			case reflected_value_type_e::quat:
				return reflected_vector_from_stream<quat_t>(object, field, stream);
			default:
				return false;
			}
		}

		bool reflected_inplace_vector_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream)
		{
			const reflected_value_type_e value_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (reflection_registry_t::is_resource_type(value_type))
				return reflected_inplace_vector_from_stream<sid_t>(object, field, stream);

			switch (value_type)
			{
			case reflected_value_type_e::f32:
				return reflected_inplace_vector_from_stream<f32>(object, field, stream);
			case reflected_value_type_e::i32:
				return reflected_inplace_vector_from_stream<i32>(object, field, stream);
			case reflected_value_type_e::i16:
				return reflected_inplace_vector_from_stream<i16>(object, field, stream);
			case reflected_value_type_e::i8:
				return reflected_inplace_vector_from_stream<i8>(object, field, stream);
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
			case reflected_value_type_e::enum32:
				return reflected_inplace_vector_from_stream<u32>(object, field, stream);
			case reflected_value_type_e::u64:
				return reflected_inplace_vector_from_stream<u64>(object, field, stream);
			case reflected_value_type_e::u16:
				return reflected_inplace_vector_from_stream<u16>(object, field, stream);
			case reflected_value_type_e::size_t:
				return reflected_inplace_vector_from_stream<size_t>(object, field, stream);
			case reflected_value_type_e::entity_guid:
				return reflected_inplace_vector_from_stream<u64>(object, field, stream);
			case reflected_value_type_e::u8:
			case reflected_value_type_e::bool8:
			case reflected_value_type_e::enum8:
				return reflected_inplace_vector_from_stream<u8>(object, field, stream);
			case reflected_value_type_e::string:
				return reflected_inplace_vector_from_stream<string_t>(object, field, stream);
			case reflected_value_type_e::json:
				return reflected_inplace_vector_from_stream<nlohmann::json>(object, field, stream);
			case reflected_value_type_e::quat:
				return reflected_inplace_vector_from_stream<quat_t>(object, field, stream);
			default:
				return false;
			}
		}

		bool reflected_field_to_json(const void* object, const reflected_field_desc_t& field, nlohmann::json& j)
		{
			const u8* field_ptr = static_cast<const u8*>(object) + field.offset;
			if (reflection_registry_t::is_resource_type(field.type))
			{
				const sid_t value = *reinterpret_cast<const sid_t*>(field_ptr);
				j				  = value;
				return true;
			}

			switch (field.type)
			{
			case reflected_value_type_e::f32: {
				const f32 value = *reinterpret_cast<const f32*>(field_ptr);
				j				= value;
				return true;
			}
			case reflected_value_type_e::i32: {
				const i32 value = *reinterpret_cast<const i32*>(field_ptr);
				j				= value;
				return true;
			}
			case reflected_value_type_e::i16: {
				const i16 value = *reinterpret_cast<const i16*>(field_ptr);
				j				= value;
				return true;
			}
			case reflected_value_type_e::i8: {
				const i8 value = *reinterpret_cast<const i8*>(field_ptr);
				j			   = value;
				return true;
			}
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id: {
				const u32 value = *reinterpret_cast<const u32*>(field_ptr);
				j				= value;
				return true;
			}
			case reflected_value_type_e::u64: {
				const u64 value = *reinterpret_cast<const u64*>(field_ptr);
				j				= value;
				return true;
			}
			case reflected_value_type_e::u16: {
				const u16 value = *reinterpret_cast<const u16*>(field_ptr);
				j				= value;
				return true;
			}
			case reflected_value_type_e::size_t: {
				const size_t value = *reinterpret_cast<const size_t*>(field_ptr);
				j				   = value;
				return true;
			}
			case reflected_value_type_e::u8: {
				const u8 value = *reinterpret_cast<const u8*>(field_ptr);
				j			   = value;
				return true;
			}
			case reflected_value_type_e::bool8: {
				const bool value = *field_ptr != 0;
				j				 = value;
				return true;
			}
			case reflected_value_type_e::entity_guid: {
				const u64 value = *reinterpret_cast<const u64*>(field_ptr);
				j				= value;
				return true;
			}
			case reflected_value_type_e::string: {
				const char* value = nullptr;
				if (!read_reflected_text(object, field, value))
					return false;
				j = value != nullptr ? value : "";
				return true;
			}
			case reflected_value_type_e::json: {
				const nlohmann::json& value = *reinterpret_cast<const nlohmann::json*>(field_ptr);
				j							= value;
				return true;
			}
			case reflected_value_type_e::quat: {
				const quat_t value = *reinterpret_cast<const quat_t*>(field_ptr);
				j				   = nlohmann::json::array_t({value.x, value.y, value.z, value.w});
				return true;
			}
			case reflected_value_type_e::enum8:
			case reflected_value_type_e::enum32: {
				i64 value = 0;
				if (!read_reflected_enum(object, field, value))
					return false;
				if (field.flags.is_set(reflected_field_flags_bitmask))
				{
					j															= nlohmann::json::array();
					const span_t<const reflected_enum_value_desc_t> enum_values = get_reflected_field_enum_values(field);
					for (u32 i = 0; i < enum_values.size; ++i)
					{
						const reflected_enum_value_desc_t& enum_value = enum_values.data[i];
						if (enum_value.value != 0 && (value & enum_value.value) != 0)
							j.push_back(enum_value.name);
					}
					return true;
				}
				const char* name = find_enum_name(get_reflected_field_enum_values(field), value);
				j				 = name != nullptr ? nlohmann::json(name) : nlohmann::json(value);
				return true;
			}
			case reflected_value_type_e::object:
				return reflection_registry_t::get().serialize_to_json(field.value_type_id, field_ptr, j);
			case reflected_value_type_e::vector:
				return reflected_vector_to_json_by_type(object, field, j);
			case reflected_value_type_e::inplace_vector:
				return reflected_inplace_vector_to_json_by_type(object, field, j);
			default:
				return false;
			}
		}

		bool reflected_field_from_json(void* object, const reflected_field_desc_t& field, const nlohmann::json& j)
		{
			u8* field_ptr = static_cast<u8*>(object) + field.offset;
			if (reflection_registry_t::is_resource_type(field.type))
			{
				*reinterpret_cast<sid_t*>(field_ptr) = j.get<sid_t>();
				return true;
			}

			switch (field.type)
			{
			case reflected_value_type_e::f32:
				*reinterpret_cast<f32*>(field_ptr) = j.get<f32>();
				return true;
			case reflected_value_type_e::i32:
				*reinterpret_cast<i32*>(field_ptr) = j.get<i32>();
				return true;
			case reflected_value_type_e::i16:
				*reinterpret_cast<i16*>(field_ptr) = j.get<i16>();
				return true;
			case reflected_value_type_e::i8:
				*reinterpret_cast<i8*>(field_ptr) = j.get<i8>();
				return true;
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id:
				*reinterpret_cast<u32*>(field_ptr) = j.get<u32>();
				return true;
			case reflected_value_type_e::u64:
				*reinterpret_cast<u64*>(field_ptr) = j.get<u64>();
				return true;
			case reflected_value_type_e::u16:
				*reinterpret_cast<u16*>(field_ptr) = j.get<u16>();
				return true;
			case reflected_value_type_e::size_t:
				*reinterpret_cast<size_t*>(field_ptr) = j.get<size_t>();
				return true;
			case reflected_value_type_e::u8:
				*reinterpret_cast<u8*>(field_ptr) = j.get<u8>();
				return true;
			case reflected_value_type_e::bool8:
				*field_ptr = j.get<bool>() ? 1 : 0;
				return true;
			case reflected_value_type_e::entity_guid:
				*reinterpret_cast<u64*>(field_ptr) = j.get<u64>();
				return true;
			case reflected_value_type_e::string: {
				const string_t value = j.get<string_t>();
				return write_reflected_text(object, field, value.c_str());
			}
			case reflected_value_type_e::json:
				*reinterpret_cast<nlohmann::json*>(field_ptr) = j;
				return true;
			case reflected_value_type_e::quat: {
				vec4f_t value = {};
				if (!json_to_vec4(j, value))
					return false;
				*reinterpret_cast<quat_t*>(field_ptr) = quat_t{value.x, value.y, value.z, value.w};
				return true;
			}
			case reflected_value_type_e::enum8:
			case reflected_value_type_e::enum32: {
				i64 value = 0;
				if (field.flags.is_set(reflected_field_flags_bitmask) && j.is_array())
				{
					for (const nlohmann::json& item : j)
					{
						i64 flag = 0;
						if (item.is_string())
						{
							const string_t name = item.get<string_t>();
							if (!find_enum_value(get_reflected_field_enum_values(field), name.c_str(), flag))
								return false;
						}
						else
						{
							flag = item.get<i64>();
						}
						value |= flag;
					}
				}
				else if (j.is_string())
				{
					const string_t name = j.get<string_t>();
					if (!find_enum_value(get_reflected_field_enum_values(field), name.c_str(), value))
						return false;
				}
				else
				{
					value = j.get<i64>();
				}
				return write_reflected_enum(object, field, value);
			}
			case reflected_value_type_e::object:
				return reflection_registry_t::get().deserialize_from_json(field.value_type_id, field_ptr, j);
			case reflected_value_type_e::vector:
				return reflected_vector_from_json_by_type(object, field, j);
			case reflected_value_type_e::inplace_vector:
				return reflected_inplace_vector_from_json_by_type(object, field, j);
			default:
				return false;
			}
		}

		bool reflected_field_to_stream(const void* object, const reflected_field_desc_t& field, ostream_t& stream)
		{
			const u8* field_ptr = static_cast<const u8*>(object) + field.offset;
			if (reflection_registry_t::is_resource_type(field.type))
			{
				const sid_t value = *reinterpret_cast<const sid_t*>(field_ptr);
				stream << value;
				return true;
			}

			switch (field.type)
			{
			case reflected_value_type_e::f32: {
				const f32 value = *reinterpret_cast<const f32*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::i32: {
				const i32 value = *reinterpret_cast<const i32*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::i16: {
				const i16 value = *reinterpret_cast<const i16*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::i8: {
				const i8 value = *reinterpret_cast<const i8*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id: {
				const u32 value = *reinterpret_cast<const u32*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::u64: {
				const u64 value = *reinterpret_cast<const u64*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::u16: {
				const u16 value = *reinterpret_cast<const u16*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::size_t: {
				const size_t value = *reinterpret_cast<const size_t*>(field_ptr);
				stream << static_cast<u32>(value);
				return true;
			}
			case reflected_value_type_e::u8: {
				const u8 value = *reinterpret_cast<const u8*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::bool8: {
				const bool value = *field_ptr != 0;
				stream << static_cast<u8>(value ? 1 : 0);
				return true;
			}
			case reflected_value_type_e::entity_guid: {
				const u64 value = *reinterpret_cast<const u64*>(field_ptr);
				stream << value;
				return true;
			}
			case reflected_value_type_e::string: {
				const char* value = nullptr;
				if (!read_reflected_text(object, field, value))
					return false;
				stream << string_t(value != nullptr ? value : "");
				return true;
			}
			case reflected_value_type_e::json: {
				const nlohmann::json& value = *reinterpret_cast<const nlohmann::json*>(field_ptr);
				stream << string_t(value.dump().c_str());
				return true;
			}
			case reflected_value_type_e::quat: {
				const quat_t value = *reinterpret_cast<const quat_t*>(field_ptr);
				stream << value.x << value.y << value.z << value.w;
				return true;
			}
			case reflected_value_type_e::enum8: {
				i64 value = 0;
				if (!read_reflected_enum(object, field, value))
					return false;
				stream << static_cast<u8>(value);
				return true;
			}
			case reflected_value_type_e::enum32: {
				i64 value = 0;
				if (!read_reflected_enum(object, field, value))
					return false;
				stream << static_cast<u32>(value);
				return true;
			}
			case reflected_value_type_e::object:
				return reflection_registry_t::get().serialize_to_stream(field.value_type_id, field_ptr, stream);
			case reflected_value_type_e::vector:
				return reflected_vector_to_stream_by_type(object, field, stream);
			case reflected_value_type_e::inplace_vector:
				return reflected_inplace_vector_to_stream(object, field, stream);
			default:
				return false;
			}
		}

		bool reflected_field_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream)
		{
			u8* field_ptr = static_cast<u8*>(object) + field.offset;
			if (reflection_registry_t::is_resource_type(field.type))
			{
				sid_t value = 0;
				stream >> value;
				*reinterpret_cast<sid_t*>(field_ptr) = value;
				return true;
			}

			switch (field.type)
			{
			case reflected_value_type_e::f32: {
				f32 value = 0.0f;
				stream >> value;
				*reinterpret_cast<f32*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::i32: {
				i32 value = 0;
				stream >> value;
				*reinterpret_cast<i32*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::i16: {
				i16 value = 0;
				stream >> value;
				*reinterpret_cast<i16*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::i8: {
				i8 value = 0;
				stream >> value;
				*reinterpret_cast<i8*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::u32:
			case reflected_value_type_e::text_id: {
				u32 value = 0;
				stream >> value;
				*reinterpret_cast<u32*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::u64: {
				u64 value = 0;
				stream >> value;
				*reinterpret_cast<u64*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::u16: {
				u16 value = 0;
				stream >> value;
				*reinterpret_cast<u16*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::size_t: {
				u32 value = 0;
				stream >> value;
				*reinterpret_cast<size_t*>(field_ptr) = static_cast<size_t>(value);
				return true;
			}
			case reflected_value_type_e::u8: {
				u8 value = 0;
				stream >> value;
				*reinterpret_cast<u8*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::bool8: {
				u8 value = 0;
				stream >> value;
				*field_ptr = value != 0 ? 1 : 0;
				return true;
			}
			case reflected_value_type_e::entity_guid: {
				u64 value = 0;
				stream >> value;
				*reinterpret_cast<u64*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::string: {
				string_t value;
				stream >> value;
				return write_reflected_text(object, field, value.c_str());
			}
			case reflected_value_type_e::json: {
				string_t value;
				stream >> value;
				const nlohmann::json j = nlohmann::json::parse(value, nullptr, false);
				if (j.is_discarded())
					return false;
				*reinterpret_cast<nlohmann::json*>(field_ptr) = j;
				return true;
			}
			case reflected_value_type_e::quat: {
				quat_t value;
				stream >> value.x >> value.y >> value.z >> value.w;
				*reinterpret_cast<quat_t*>(field_ptr) = value;
				return true;
			}
			case reflected_value_type_e::enum8: {
				u8 value = 0;
				stream >> value;
				return write_reflected_enum(object, field, static_cast<i64>(value));
			}
			case reflected_value_type_e::enum32: {
				u32 value = 0;
				stream >> value;
				return write_reflected_enum(object, field, static_cast<i64>(value));
			}
			case reflected_value_type_e::object:
				return reflection_registry_t::get().deserialize_from_stream(field.value_type_id, field_ptr, stream);
			case reflected_value_type_e::vector:
				return reflected_vector_from_stream(object, field, stream);
			case reflected_value_type_e::inplace_vector:
				return reflected_inplace_vector_from_stream(object, field, stream);
			default:
				return false;
			}
		}
	}

	reflection_registry_t& reflection_registry_t::get()
	{
		static reflection_registry_t registry;
		return registry;
	}

	bool reflection_registry_t::is_resource_type(reflected_value_type_e type)
	{
		const u8 value = static_cast<u8>(type);
		return value >= static_cast<u8>(reflected_value_type_e::audio_handle) && value <= static_cast<u8>(reflected_value_type_e::hdr_skybox_handle);
	}

	reflection_registry_t::reflection_registry_t()
	{
		init();
	}

	reflection_registry_t::~reflection_registry_t()
	{
		uninit();
	}

	void reflection_registry_t::init()
	{
		if (_initialized)
			uninit();

		_text.init(REFLECTION_REGISTRY_TEXT_BYTES);
		_types.reserve(REFLECTION_REGISTRY_MAX_TYPES);
		_fields.reserve(REFLECTION_REGISTRY_MAX_FIELDS);
		_enum_values.reserve(REFLECTION_REGISTRY_MAX_ENUM_VALUES);
		_initialized = true;
	}

	void reflection_registry_t::uninit()
	{
		if (!_initialized)
			return;

		_types.resize(0);
		_fields.resize(0);
		_enum_values.resize(0);
		_text.uninit();
		_initialized = false;
	}

	void reflection_registry_t::reset()
	{
		SFG_ASSERT(_initialized);

		_types.resize(0);
		_fields.resize(0);
		_enum_values.resize(0);
		_text.reset();
	}

	bool reflection_registry_t::register_type(const reflected_type_desc_t& desc)
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(desc.type_id != 0);

		if (!_initialized || desc.type_id == 0)
		{
			SFG_ERR("invalid reflected type registration");
			return false;
		}

		const u32 field_count = static_cast<u32>(desc.fields.size);
		if (field_count != 0 && desc.fields.data == nullptr)
		{
			SFG_ERR("reflected type has field count but no field data");
			return false;
		}

		SFG_ASSERT(_fields.size() + field_count <= _fields.capacity());
		if (_fields.size() + field_count > _fields.capacity())
		{
			SFG_ERR("reflection field capacity exceeded");
			return false;
		}

		if (desc.enum_values.size != 0 && desc.enum_values.data == nullptr)
		{
			SFG_ERR("reflected type has enum value count but no enum value data");
			return false;
		}

		u32 enum_count = static_cast<u32>(desc.enum_values.size);
		for (u32 i = 0; i < field_count; ++i)
		{
			if (desc.fields.data[i].enum_values.size != 0 && desc.fields.data[i].enum_values.data == nullptr)
			{
				SFG_ERR("reflected field has enum value count but no enum value data");
				return false;
			}

			enum_count += static_cast<u32>(desc.fields.data[i].enum_values.size);
		}

		SFG_ASSERT(_enum_values.size() + enum_count <= _enum_values.capacity());
		if (_enum_values.size() + enum_count > _enum_values.capacity())
		{
			SFG_ERR("reflection enum value capacity exceeded");
			return false;
		}

		const u32 field_start = static_cast<u32>(_fields.size());
		for (u32 i = 0; i < field_count; ++i)
			_fields.push_back(copy_field(desc.fields.data[i]));

		reflected_type_desc_t copied		 = copy_type(desc, field_start, field_count);
		const u32			  existing_index = find_type_index(desc.type_id);
		if (existing_index != static_cast<u32>(_types.size()))
		{
			_types[existing_index] = copied;
			return true;
		}

		SFG_ASSERT(_types.size() < _types.capacity());
		if (_types.size() >= _types.capacity())
		{
			SFG_ERR("reflection type capacity exceeded");
			return false;
		}
		_types.push_back(copied);
		return true;
	}

	bool reflection_registry_t::serialize_to_json(sid_t type_id, const void* obj, nlohmann::json& j) const
	{
		SFG_ASSERT(obj != nullptr);

		const reflected_type_desc_t* type = find_type(type_id);
		if (type == nullptr)
			return false;

		if (type->fields.size == 0 && type->enum_values.size != 0)
		{
			const i64	value = read_reflected_enum_value(obj, type->size);
			const char* name  = find_enum_name(type->enum_values, value);
			j				  = name != nullptr ? nlohmann::json(name) : nlohmann::json(value);
			return true;
		}

		if (reflected_math_type_to_json(type_id, obj, j))
			return true;

		j = nlohmann::json::object();
		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];

			SFG_ASSERT(field.name != nullptr);
			nlohmann::json value = {};
			if (!reflected_field_to_json(obj, field, value))
				return false;

			j[field.name] = value;
		}

		return true;
	}

	bool reflection_registry_t::serialize_to_stream(sid_t type_id, const void* obj, ostream_t& stream) const
	{
		SFG_ASSERT(obj != nullptr);

		const reflected_type_desc_t* type = find_type(type_id);
		if (type == nullptr)
			return false;

		if (type->fields.size == 0 && type->enum_values.size != 0)
		{
			stream_reflected_enum_value(stream, type->size, read_reflected_enum_value(obj, type->size));
			return true;
		}

		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			if (!reflected_field_to_stream(obj, field, stream))
				return false;
		}

		return true;
	}

	bool reflection_registry_t::deserialize_from_json(sid_t type_id, void* obj, const nlohmann::json& j) const
	{
		SFG_ASSERT(obj != nullptr);

		const reflected_type_desc_t* type = find_type(type_id);
		if (type == nullptr)
			return false;

		if (type->fields.size == 0 && type->enum_values.size != 0)
		{
			i64 value = 0;
			if (j.is_string())
			{
				const string_t name = j.get<string_t>();
				if (!find_enum_value(type->enum_values, name.c_str(), value))
					return false;
			}
			else
			{
				value = j.get<i64>();
			}

			write_reflected_enum_value(obj, type->size, value);
			return true;
		}

		if (reflected_math_type_from_json(type_id, obj, j))
			return true;

		if (!j.is_object())
			return false;

		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];

			SFG_ASSERT(field.name != nullptr);
			if (!j.contains(field.name))
				continue;
			if (!reflected_field_from_json(obj, field, j.at(field.name)))
				return false;
		}

		return true;
	}

	bool reflection_registry_t::deserialize_from_stream(sid_t type_id, void* obj, istream_t& stream) const
	{
		SFG_ASSERT(obj != nullptr);

		const reflected_type_desc_t* type = find_type(type_id);
		if (type == nullptr)
			return false;

		if (type->fields.size == 0 && type->enum_values.size != 0)
		{
			write_reflected_enum_value(obj, type->size, stream_read_reflected_enum_value(stream, type->size));
			return true;
		}

		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			if (!reflected_field_from_stream(obj, field, stream))
				return false;
		}

		return true;
	}

	bool reflection_registry_t::serialize_field_to_stream(const void* obj, const reflected_field_desc_t& field, ostream_t& stream) const
	{
		SFG_ASSERT(obj != nullptr);
		return reflected_field_to_stream(obj, field, stream);
	}

	bool reflection_registry_t::deserialize_field_from_stream(void* obj, const reflected_field_desc_t& field, istream_t& stream) const
	{
		SFG_ASSERT(obj != nullptr);
		return reflected_field_from_stream(obj, field, stream);
	}

	bool reflection_registry_t::serialize_field_to_stream(sid_t type_id, sid_t field_id, const void* obj, ostream_t& stream) const
	{
		const reflected_field_desc_t* field = find_field(type_id, field_id);
		if (field == nullptr)
			return false;
		return serialize_field_to_stream(obj, *field, stream);
	}

	bool reflection_registry_t::deserialize_field_from_stream(sid_t type_id, sid_t field_id, void* obj, istream_t& stream) const
	{
		const reflected_field_desc_t* field = find_field(type_id, field_id);
		if (field == nullptr)
			return false;
		return deserialize_field_from_stream(obj, *field, stream);
	}

	const reflected_type_desc_t* reflection_registry_t::find_type(sid_t type_id) const
	{
		const u32 index = find_type_index(type_id);
		return index != static_cast<u32>(_types.size()) ? &_types[index] : nullptr;
	}

	const reflected_type_desc_t& reflection_registry_t::get_type(sid_t type_id) const
	{
		const reflected_type_desc_t* type = find_type(type_id);
		SFG_ASSERT(type != nullptr);
		return *type;
	}

	const reflected_field_desc_t* reflection_registry_t::find_field(sid_t type_id, sid_t field_id) const
	{
		const reflected_type_desc_t* type = find_type(type_id);
		if (type == nullptr)
			return nullptr;

		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			if (field.id == field_id)
				return &field;
		}

		return nullptr;
	}

	span_t<const reflected_type_desc_t> reflection_registry_t::get_types() const
	{
		return {.data = _types.data(), .size = _types.size()};
	}

	bool reflection_registry_t::is_initialized() const
	{
		return _initialized;
	}

	u32 reflection_registry_t::find_type_index(sid_t type_id) const
	{
		for (u32 i = 0; i < _types.size(); ++i)
		{
			if (_types[i].type_id == type_id)
				return i;
		}

		return static_cast<u32>(_types.size());
	}

	const char* reflection_registry_t::copy_text(const char* text)
	{
		if (text == nullptr || text[0] == '\0')
			return nullptr;

		const char* copied = _text.allocate(text);
		SFG_ASSERT(copied != nullptr);
		return copied;
	}

	sid_t reflection_registry_t::resolve_id(sid_t id, const char* name) const
	{
		if (id != 0)
			return id;

		SFG_ASSERT(name != nullptr);
		return hashing_t::to_sid(name);
	}

	reflected_enum_value_desc_t reflection_registry_t::copy_enum_value(const reflected_enum_value_desc_t& desc)
	{
		const char* name = copy_text(desc.name);
		return {
			.name		  = name,
			.display_name = copy_text(desc.display_name != nullptr ? desc.display_name : desc.name),
			.id			  = resolve_id(desc.id, desc.name),
			.value		  = desc.value,
		};
	}

	reflected_field_desc_t reflection_registry_t::copy_field(const reflected_field_desc_t& desc)
	{
		const u32 enum_value_start = static_cast<u32>(_enum_values.size());
		const u32 enum_value_count = static_cast<u32>(desc.enum_values.size);

		for (u32 i = 0; i < enum_value_count; ++i)
			_enum_values.push_back(copy_enum_value(desc.enum_values.data[i]));

		const char*			   name	  = copy_text(desc.name);
		reflected_field_desc_t copied = desc;
		copied.enum_values			  = enum_value_count != 0 ? span_t<const reflected_enum_value_desc_t>{.data = _enum_values.data() + enum_value_start, .size = enum_value_count} : span_t<const reflected_enum_value_desc_t>{};
		copied.name					  = name;
		copied.display_name			  = copy_text(desc.display_name != nullptr ? desc.display_name : desc.name);
		copied.tooltip				  = copy_text(desc.tooltip);
		copied.id					  = resolve_id(desc.id, desc.name);
		return copied;
	}

	reflected_type_desc_t reflection_registry_t::copy_type(const reflected_type_desc_t& desc, u32 field_start, u32 field_count)
	{
		const u32 enum_value_start = static_cast<u32>(_enum_values.size());
		const u32 enum_value_count = static_cast<u32>(desc.enum_values.size);

		for (u32 i = 0; i < enum_value_count; ++i)
			_enum_values.push_back(copy_enum_value(desc.enum_values.data[i]));

		const char*			  name	 = copy_text(desc.name);
		reflected_type_desc_t copied = desc;
		copied.fields				 = field_count != 0 ? span_t<const reflected_field_desc_t>{.data = _fields.data() + field_start, .size = field_count} : span_t<const reflected_field_desc_t>{};
		copied.enum_values			 = enum_value_count != 0 ? span_t<const reflected_enum_value_desc_t>{.data = _enum_values.data() + enum_value_start, .size = enum_value_count} : span_t<const reflected_enum_value_desc_t>{};
		copied.name					 = name;
		copied.display_name			 = copy_text(desc.display_name != nullptr ? desc.display_name : desc.name);
		copied.category				 = copy_text(desc.category);
		return copied;
	}
}
