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

	namespace
	{
		const void* get_reflected_field_ptr(const void* object, const reflected_field_desc_t& field)
		{
			return static_cast<const u8*>(object) + field.offset;
		}

		void* get_reflected_field_ptr(void* object, const reflected_field_desc_t& field)
		{
			return static_cast<u8*>(object) + field.offset;
		}

		template <typename T> bool read_reflected_value(const void* object, const reflected_field_desc_t& field, T& value)
		{
			if (field.get != nullptr)
				return field.get(object, field, &value, field.user_data);

			value = *static_cast<const T*>(get_reflected_field_ptr(object, field));
			return true;
		}

		template <typename T> bool write_reflected_value(void* object, const reflected_field_desc_t& field, const T& value)
		{
			if (field.set != nullptr)
				return field.set(object, field, &value, field.user_data);

			*static_cast<T*>(get_reflected_field_ptr(object, field)) = value;
			return true;
		}

		bool read_reflected_bool(const void* object, const reflected_field_desc_t& field, bool& value)
		{
			if (field.get != nullptr)
				return field.get(object, field, &value, field.user_data);

			value = *static_cast<const u8*>(get_reflected_field_ptr(object, field)) != 0;
			return true;
		}

		bool write_reflected_bool(void* object, const reflected_field_desc_t& field, bool value)
		{
			if (field.set != nullptr)
				return field.set(object, field, &value, field.user_data);

			*static_cast<u8*>(get_reflected_field_ptr(object, field)) = value ? 1 : 0;
			return true;
		}

		bool read_reflected_text(const void* object, const reflected_field_desc_t& field, const char*& value)
		{
			if (field.get != nullptr)
				return field.get(object, field, &value, field.user_data);

			if (field.size == sizeof(string_t))
			{
				value = static_cast<const string_t*>(get_reflected_field_ptr(object, field))->c_str();
				return true;
			}

			value = static_cast<const char*>(get_reflected_field_ptr(object, field));
			return true;
		}

		bool write_reflected_text(void* object, const reflected_field_desc_t& field, const char* value)
		{
			if (field.set != nullptr)
				return field.set(object, field, value, field.user_data);

			const char* src = value != nullptr ? value : "";
			if (field.size == sizeof(string_t))
			{
				*static_cast<string_t*>(get_reflected_field_ptr(object, field)) = src;
				return true;
			}

			SFG_ASSERT(field.size > 0);
			char*		 dst	 = static_cast<char*>(get_reflected_field_ptr(object, field));
			const size_t max_len = static_cast<size_t>(field.size - 1);
			const size_t len	 = std::strlen(src) < max_len ? std::strlen(src) : max_len;
			std::memcpy(dst, src, len);
			dst[len] = '\0';
			return true;
		}

		bool read_reflected_enum(const void* object, const reflected_field_desc_t& field, i64& value)
		{
			if (field.get != nullptr)
			{
				if (field.type == reflected_value_type_e::enum8)
				{
					u8 raw = 0;
					if (!field.get(object, field, &raw, field.user_data))
						return false;
					value = static_cast<i64>(raw);
					return true;
				}

				u32 raw = 0;
				if (!field.get(object, field, &raw, field.user_data))
					return false;
				value = static_cast<i64>(raw);
				return true;
			}

			const void* ptr = get_reflected_field_ptr(object, field);
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
			if (field.set != nullptr)
			{
				if (field.type == reflected_value_type_e::enum8)
				{
					const u8 raw = static_cast<u8>(value);
					return field.set(object, field, &raw, field.user_data);
				}

				const u32 raw = static_cast<u32>(value);
				return field.set(object, field, &raw, field.user_data);
			}

			void* ptr = get_reflected_field_ptr(object, field);
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

		template <typename T> vector_t<T>& get_reflected_vector(void* object, const reflected_field_desc_t& field)
		{
			return *static_cast<vector_t<T>*>(get_reflected_field_ptr(object, field));
		}

		template <typename T> const vector_t<T>& get_reflected_vector(const void* object, const reflected_field_desc_t& field)
		{
			return *static_cast<const vector_t<T>*>(get_reflected_field_ptr(object, field));
		}

		template <typename T> size_t get_static_vector_head_offset(const reflected_field_desc_t& field)
		{
			const size_t data_size = sizeof(T) * field.capacity;
			const size_t alignment = alignof(size_t);
			return (data_size + alignment - 1) & ~(alignment - 1);
		}

		template <typename T> T* get_reflected_static_vector_data(void* object, const reflected_field_desc_t& field)
		{
			return std::launder(reinterpret_cast<T*>(get_reflected_field_ptr(object, field)));
		}

		template <typename T> const T* get_reflected_static_vector_data(const void* object, const reflected_field_desc_t& field)
		{
			return std::launder(reinterpret_cast<const T*>(get_reflected_field_ptr(object, field)));
		}

		template <typename T> size_t& get_reflected_static_vector_size(void* object, const reflected_field_desc_t& field)
		{
			return *reinterpret_cast<size_t*>(static_cast<u8*>(get_reflected_field_ptr(object, field)) + get_static_vector_head_offset<T>(field));
		}

		template <typename T> const size_t& get_reflected_static_vector_size(const void* object, const reflected_field_desc_t& field)
		{
			return *reinterpret_cast<const size_t*>(static_cast<const u8*>(get_reflected_field_ptr(object, field)) + get_static_vector_head_offset<T>(field));
		}

		template <typename T> void clear_reflected_static_vector(void* object, const reflected_field_desc_t& field)
		{
			T*		data = get_reflected_static_vector_data<T>(object, field);
			size_t& size = get_reflected_static_vector_size<T>(object, field);
			while (size > 0)
			{
				--size;
				std::destroy_at(data + size);
			}
		}

		template <typename T> void resize_reflected_static_vector(void* object, const reflected_field_desc_t& field, u32 size)
		{
			clear_reflected_static_vector<T>(object, field);
			T*		data = get_reflected_static_vector_data<T>(object, field);
			size_t& head = get_reflected_static_vector_size<T>(object, field);
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
			if (field.value_type_id == 0)
				return {};

			const reflected_type_desc_t* type = reflection_registry_t::get().find_type(field.value_type_id);
			return type != nullptr ? type->enum_values : span_t<const reflected_enum_value_desc_t>{};
		}

		const char* find_enum_name(const reflected_field_desc_t& field, i64 value)
		{
			return find_enum_name(get_reflected_field_enum_values(field), value);
		}

		bool find_enum_value(const reflected_field_desc_t& field, const char* name, i64& out_value)
		{
			return find_enum_value(get_reflected_field_enum_values(field), name, out_value);
		}

		nlohmann::json vec2_to_json(const vec2f_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y});
		}

		nlohmann::json vec3_to_json(const vec3f_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y, value.z});
		}

		nlohmann::json vec4_to_json(const vec4f_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y, value.z, value.w});
		}

		nlohmann::json vec2u_to_json(const vec2u_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y});
		}

		nlohmann::json vec2u16_to_json(const vec2u16_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y});
		}

		nlohmann::json vec3u_to_json(const vec3u_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y, value.z});
		}

		nlohmann::json vec4u_to_json(const vec4u_t& value)
		{
			return nlohmann::json::array_t({value.x, value.y, value.z, value.w});
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

		template <typename T> nlohmann::json reflected_vector_to_json(const vector_t<T>& values)
		{
			nlohmann::json out = nlohmann::json::array();
			for (const T& value : values)
				out.push_back(value);
			return out;
		}

		template <typename T> nlohmann::json reflected_static_vector_to_json(const void* object, const reflected_field_desc_t& field)
		{
			nlohmann::json out	= nlohmann::json::array();
			const T*	   data = get_reflected_static_vector_data<T>(object, field);
			const size_t   size = get_reflected_static_vector_size<T>(object, field);
			for (size_t i = 0; i < size; ++i)
				out.push_back(data[i]);
			return out;
		}

		template <typename T> bool reflected_vector_from_json(void* object, const reflected_field_desc_t& field, const nlohmann::json& j)
		{
			if (!j.is_array())
				return true;

			vector_t<T>& values = get_reflected_vector<T>(object, field);
			values.resize(0);
			values.reserve(j.size());
			for (const nlohmann::json& item : j)
				values.push_back(item.get<T>());
			return true;
		}

		template <typename T> bool reflected_static_vector_from_json(void* object, const reflected_field_desc_t& field, const nlohmann::json& j)
		{
			if (!j.is_array())
				return true;
			if (j.size() > field.capacity)
				return false;

			resize_reflected_static_vector<T>(object, field, static_cast<u32>(j.size()));
			T* data = get_reflected_static_vector_data<T>(object, field);
			for (u32 i = 0; i < j.size(); ++i)
				data[i] = j.at(i).get<T>();
			return true;
		}

		template <typename T> void reflected_vector_to_stream(const vector_t<T>& values, ostream_t& stream)
		{
			const u32 size = static_cast<u32>(values.size());
			stream << size;
			for (const T& value : values)
				stream << value;
		}

		template <typename T> void reflected_static_vector_to_stream(const void* object, const reflected_field_desc_t& field, ostream_t& stream)
		{
			const T*	 data = get_reflected_static_vector_data<T>(object, field);
			const size_t size = get_reflected_static_vector_size<T>(object, field);
			stream << static_cast<u32>(size);
			for (size_t i = 0; i < size; ++i)
				stream << data[i];
		}

		template <typename T> void reflected_vector_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream)
		{
			u32 size = 0;
			stream >> size;
			vector_t<T>& values = get_reflected_vector<T>(object, field);
			values.resize(size);
			for (T& value : values)
				stream >> value;
		}

		template <typename T> bool reflected_static_vector_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream)
		{
			u32 size = 0;
			stream >> size;
			if (size > field.capacity)
				return false;

			resize_reflected_static_vector<T>(object, field, size);
			T* data = get_reflected_static_vector_data<T>(object, field);
			for (u32 i = 0; i < size; ++i)
				stream >> data[i];
			return true;
		}

		bool reflected_field_to_json(const void* object, const reflected_field_desc_t& field, nlohmann::json& j)
		{
			switch (field.type)
			{
			case reflected_value_type_e::f32: {
				f32 value = 0.0f;
				if (!read_reflected_value(object, field, value))
					return false;
				j = value;
				return true;
			}
			case reflected_value_type_e::i32: {
				i32 value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				j = value;
				return true;
			}
			case reflected_value_type_e::u32: {
				u32 value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				j = value;
				return true;
			}
			case reflected_value_type_e::u8: {
				u8 value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				j = value;
				return true;
			}
			case reflected_value_type_e::bool8: {
				bool value = false;
				if (!read_reflected_bool(object, field, value))
					return false;
				j = value;
				return true;
			}
			case reflected_value_type_e::vec2: {
				vec2f_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = vec2_to_json(value);
				return true;
			}
			case reflected_value_type_e::vec3: {
				vec3f_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = vec3_to_json(value);
				return true;
			}
			case reflected_value_type_e::vec4:
			case reflected_value_type_e::color: {
				vec4f_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = vec4_to_json(value);
				return true;
			}
			case reflected_value_type_e::vec2u: {
				vec2u_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = vec2u_to_json(value);
				return true;
			}
			case reflected_value_type_e::vec2u16: {
				vec2u16_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = vec2u16_to_json(value);
				return true;
			}
			case reflected_value_type_e::vec3u: {
				vec3u_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = vec3u_to_json(value);
				return true;
			}
			case reflected_value_type_e::vec4u: {
				vec4u_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = vec4u_to_json(value);
				return true;
			}
			case reflected_value_type_e::resource: {
				sid_t value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				j = value;
				return true;
			}
			case reflected_value_type_e::entity_id: {
				u32 value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				j = value;
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
				nlohmann::json value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = value;
				return true;
			}
			case reflected_value_type_e::quat: {
				quat_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				j = nlohmann::json::array_t({value.x, value.y, value.z, value.w});
				return true;
			}
			case reflected_value_type_e::enum8:
			case reflected_value_type_e::enum32: {
				i64 value = 0;
				if (!read_reflected_enum(object, field, value))
					return false;
				const char* name = find_enum_name(field, value);
				j				 = name != nullptr ? nlohmann::json(name) : nlohmann::json(value);
				return true;
			}
			case reflected_value_type_e::vector:
				if (field.sub_type_id == "f32"_hs)
					j = reflected_vector_to_json(get_reflected_vector<f32>(object, field));
				else if (field.sub_type_id == "string"_hs)
					j = reflected_vector_to_json(get_reflected_vector<string_t>(object, field));
				else
					return false;
				return true;
			case reflected_value_type_e::static_vector:
				if (field.sub_type_id == "f32"_hs)
					j = reflected_static_vector_to_json<f32>(object, field);
				else if (field.sub_type_id == "string"_hs)
					j = reflected_static_vector_to_json<string_t>(object, field);
				else
					return false;
				return true;
			default:
				return false;
			}
		}

		bool reflected_field_from_json(void* object, const reflected_field_desc_t& field, const nlohmann::json& j)
		{
			switch (field.type)
			{
			case reflected_value_type_e::f32:
				return write_reflected_value(object, field, j.get<f32>());
			case reflected_value_type_e::i32:
				return write_reflected_value(object, field, j.get<i32>());
			case reflected_value_type_e::u32:
				return write_reflected_value(object, field, j.get<u32>());
			case reflected_value_type_e::u8:
				return write_reflected_value(object, field, j.get<u8>());
			case reflected_value_type_e::bool8:
				return write_reflected_bool(object, field, j.get<bool>());
			case reflected_value_type_e::vec2: {
				vec2f_t value = {};
				return json_to_vec2(j, value) && write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec3: {
				vec3f_t value = {};
				return json_to_vec3(j, value) && write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec4:
			case reflected_value_type_e::color: {
				vec4f_t value = {};
				return json_to_vec4(j, value) && write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec2u: {
				vec2u_t value = {};
				return json_to_vec2u(j, value) && write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec2u16: {
				vec2u16_t value = {};
				return json_to_vec2u16(j, value) && write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec3u: {
				vec3u_t value = {};
				return json_to_vec3u(j, value) && write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec4u: {
				vec4u_t value = {};
				return json_to_vec4u(j, value) && write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::resource:
				return write_reflected_value(object, field, j.get<sid_t>());
			case reflected_value_type_e::entity_id:
				return write_reflected_value(object, field, j.get<u32>());
			case reflected_value_type_e::string: {
				const string_t value = j.get<string_t>();
				return write_reflected_text(object, field, value.c_str());
			}
			case reflected_value_type_e::json:
				return write_reflected_value(object, field, j);
			case reflected_value_type_e::quat: {
				vec4f_t value = {};
				if (!json_to_vec4(j, value))
					return false;
				return write_reflected_value(object, field, quat_t{value.x, value.y, value.z, value.w});
			}
			case reflected_value_type_e::enum8:
			case reflected_value_type_e::enum32: {
				i64 value = 0;
				if (j.is_string())
				{
					const string_t name = j.get<string_t>();
					if (!find_enum_value(field, name.c_str(), value))
						return false;
				}
				else
				{
					value = j.get<i64>();
				}
				return write_reflected_enum(object, field, value);
			}
			case reflected_value_type_e::vector:
				if (field.sub_type_id == "f32"_hs)
					return reflected_vector_from_json<f32>(object, field, j);
				if (field.sub_type_id == "string"_hs)
					return reflected_vector_from_json<string_t>(object, field, j);
				return false;
			case reflected_value_type_e::static_vector:
				if (field.sub_type_id == "f32"_hs)
					return reflected_static_vector_from_json<f32>(object, field, j);
				if (field.sub_type_id == "string"_hs)
					return reflected_static_vector_from_json<string_t>(object, field, j);
				return false;
			default:
				return false;
			}
		}

		bool reflected_field_to_stream(const void* object, const reflected_field_desc_t& field, ostream_t& stream)
		{
			switch (field.type)
			{
			case reflected_value_type_e::f32: {
				f32 value = 0.0f;
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value;
				return true;
			}
			case reflected_value_type_e::i32: {
				i32 value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value;
				return true;
			}
			case reflected_value_type_e::u32: {
				u32 value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value;
				return true;
			}
			case reflected_value_type_e::u8: {
				u8 value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value;
				return true;
			}
			case reflected_value_type_e::bool8: {
				bool value = false;
				if (!read_reflected_bool(object, field, value))
					return false;
				stream << static_cast<u8>(value ? 1 : 0);
				return true;
			}
			case reflected_value_type_e::vec2: {
				vec2f_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value.x << value.y;
				return true;
			}
			case reflected_value_type_e::vec3: {
				vec3f_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value.x << value.y << value.z;
				return true;
			}
			case reflected_value_type_e::vec4:
			case reflected_value_type_e::color: {
				vec4f_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value.x << value.y << value.z << value.w;
				return true;
			}
			case reflected_value_type_e::vec2u: {
				vec2u_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value.x << value.y;
				return true;
			}
			case reflected_value_type_e::vec2u16: {
				vec2u16_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value.x << value.y;
				return true;
			}
			case reflected_value_type_e::vec3u: {
				vec3u_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value.x << value.y << value.z;
				return true;
			}
			case reflected_value_type_e::vec4u: {
				vec4u_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value.x << value.y << value.z << value.w;
				return true;
			}
			case reflected_value_type_e::resource: {
				sid_t value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
				stream << value;
				return true;
			}
			case reflected_value_type_e::entity_id: {
				u32 value = 0;
				if (!read_reflected_value(object, field, value))
					return false;
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
				nlohmann::json value = {};
				if (!read_reflected_value(object, field, value))
					return false;
				stream << string_t(value.dump().c_str());
				return true;
			}
			case reflected_value_type_e::quat: {
				quat_t value = {};
				if (!read_reflected_value(object, field, value))
					return false;
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
			case reflected_value_type_e::vector:
				if (field.sub_type_id == "f32"_hs)
					reflected_vector_to_stream(get_reflected_vector<f32>(object, field), stream);
				else if (field.sub_type_id == "string"_hs)
					reflected_vector_to_stream(get_reflected_vector<string_t>(object, field), stream);
				else
					return false;
				return true;
			case reflected_value_type_e::static_vector:
				if (field.sub_type_id == "f32"_hs)
					reflected_static_vector_to_stream<f32>(object, field, stream);
				else if (field.sub_type_id == "string"_hs)
					reflected_static_vector_to_stream<string_t>(object, field, stream);
				else
					return false;
				return true;
			default:
				return false;
			}
		}

		bool reflected_field_from_stream(void* object, const reflected_field_desc_t& field, istream_t& stream)
		{
			switch (field.type)
			{
			case reflected_value_type_e::f32: {
				f32 value = 0.0f;
				stream >> value;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::i32: {
				i32 value = 0;
				stream >> value;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::u32: {
				u32 value = 0;
				stream >> value;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::u8: {
				u8 value = 0;
				stream >> value;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::bool8: {
				u8 value = 0;
				stream >> value;
				return write_reflected_bool(object, field, value != 0);
			}
			case reflected_value_type_e::vec2: {
				vec2f_t value = {};
				stream >> value.x >> value.y;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec3: {
				vec3f_t value = {};
				stream >> value.x >> value.y >> value.z;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec4:
			case reflected_value_type_e::color: {
				vec4f_t value = {};
				stream >> value.x >> value.y >> value.z >> value.w;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec2u: {
				vec2u_t value = {};
				stream >> value.x >> value.y;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec2u16: {
				vec2u16_t value = {};
				stream >> value.x >> value.y;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec3u: {
				vec3u_t value = {};
				stream >> value.x >> value.y >> value.z;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::vec4u: {
				vec4u_t value = {};
				stream >> value.x >> value.y >> value.z >> value.w;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::resource: {
				sid_t value = 0;
				stream >> value;
				return write_reflected_value(object, field, value);
			}
			case reflected_value_type_e::entity_id: {
				u32 value = 0;
				stream >> value;
				return write_reflected_value(object, field, value);
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
				return write_reflected_value(object, field, j);
			}
			case reflected_value_type_e::quat: {
				quat_t value;
				stream >> value.x >> value.y >> value.z >> value.w;
				return write_reflected_value(object, field, value);
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
			case reflected_value_type_e::vector:
				if (field.sub_type_id == "f32"_hs)
					reflected_vector_from_stream<f32>(object, field, stream);
				else if (field.sub_type_id == "string"_hs)
					reflected_vector_from_stream<string_t>(object, field, stream);
				else
					return false;
				return true;
			case reflected_value_type_e::static_vector:
				if (field.sub_type_id == "f32"_hs)
					return reflected_static_vector_from_stream<f32>(object, field, stream);
				if (field.sub_type_id == "string"_hs)
					return reflected_static_vector_from_stream<string_t>(object, field, stream);
				return false;
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

		j = nlohmann::json::object();
		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			if ((field.flags & reflected_field_flags_transient) != 0)
				continue;

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
			if ((field.flags & reflected_field_flags_transient) != 0)
				continue;
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

		if (!j.is_object())
			return false;

		for (u32 i = 0; i < type->fields.size; ++i)
		{
			const reflected_field_desc_t& field = type->fields.data[i];
			if ((field.flags & reflected_field_flags_transient) != 0)
				continue;

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
			if ((field.flags & reflected_field_flags_transient) != 0)
				continue;
			if (!reflected_field_from_stream(obj, field, stream))
				return false;
		}

		return true;
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
