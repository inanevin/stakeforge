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

#include "reflection_registry_v2.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <algorithm>

namespace sfg
{
#define FLOAT_ROUND_PRECISION_JSON 1'000'000.0f

	namespace
	{
		size_t size_from_field_type(reflected_value_type_e_v2 type)
		{
			switch (type)
			{
			case reflected_value_type_e_v2::boolean:
				return sizeof(bool);
			case reflected_value_type_e_v2::u64:
			case reflected_value_type_e_v2::i64:
				return sizeof(u64);
			case reflected_value_type_e_v2::f32:
				return sizeof(f32);
			case reflected_value_type_e_v2::u32:
			case reflected_value_type_e_v2::i32:
				return sizeof(u32);
			case reflected_value_type_e_v2::u16:
			case reflected_value_type_e_v2::i16:
				return sizeof(i16);
			case reflected_value_type_e_v2::u8:
			case reflected_value_type_e_v2::i8:
				return sizeof(i8);
			default:
				return 0;
			}
		}

		void field_to_stream(const reflected_field_t& field, void* obj, void* user_data, ostream_t& out_stream)
		{
			u8* data = reinterpret_cast<u8*>(obj) + field.offset;

			if (field.value_type == reflected_value_type_e_v2::invalid)
			{
				SFG_ERR("can't write field to stream because value type is invalid!");
				return;
			}

			if (field.custom_serialization.to_stream_fn)
			{
				field.custom_serialization.to_stream_fn(data, user_data, out_stream);
				return;
			}

			if (field.value_type == reflected_value_type_e_v2::object)
			{
				reflection_registry_v2::get().type_to_stream(field.sub_type_id, data, user_data, out_stream);
				return;
			}

			if (field.value_type == reflected_value_type_e_v2::container)
			{
				if (field.container_ops.get_element_ptr_fn == nullptr || field.container_ops.get_element_size_fn == nullptr)
					return;

				const u32 item_size = static_cast<u32>(field.container_ops.get_element_size_fn(data));
				out_stream << item_size;

				for (size_t i = 0; i < item_size; i++)
				{
					u8*						element_data = field.container_ops.get_element_ptr_fn(data, i);
					const reflected_field_t temp_field	 = {
						.sub_type_id = field.container_ops.element_sub_type_id,
						.size		 = size_from_field_type(field.container_ops.element_value_type),
						.value_type	 = field.container_ops.element_value_type,
					};

					field_to_stream(temp_field, element_data, user_data, out_stream);
				}

				return;
			}

			if (field.value_type == reflected_value_type_e_v2::string)
			{
				string_t* str = reinterpret_cast<string_t*>(data);
				out_stream << *str;
				return;
			}

			SFG_ASSERT(field.size != 0);
			const size_t write_size = field.size;
			out_stream.write_raw(data, write_size);
		}

		void field_from_stream(const reflected_field_t& field, void* obj, void* user_data, istream_t& in_stream)
		{
			u8* data = reinterpret_cast<u8*>(obj) + field.offset;

			if (field.value_type == reflected_value_type_e_v2::invalid)
			{
				SFG_ERR("can't write field to stream because value type is invalid!");
				return;
			}

			if (field.custom_serialization.from_stream_fn)
			{
				field.custom_serialization.from_stream_fn(data, user_data, in_stream);
				return;
			}

			if (field.value_type == reflected_value_type_e_v2::object)
			{
				reflection_registry_v2::get().type_from_stream(field.sub_type_id, data, user_data, in_stream);
				return;
			}

			if (field.value_type == reflected_value_type_e_v2::container)
			{
				u32 item_size = 0;
				in_stream >> item_size;

				if (field.container_ops.reset_fn == nullptr || field.container_ops.add_element_ptr_fn == nullptr)
					return;

				field.container_ops.reset_fn(data);

				for (size_t i = 0; i < item_size; i++)
				{
					u8* element_data = field.container_ops.add_element_ptr_fn(data);
					if (element_data == nullptr)
						return;

					const reflected_field_t temp_field = {
						.sub_type_id = field.container_ops.element_sub_type_id,
						.size		 = size_from_field_type(field.container_ops.element_value_type),
						.value_type	 = field.container_ops.element_value_type,
					};

					field_from_stream(temp_field, element_data, user_data, in_stream);
				}

				return;
			}

			if (field.value_type == reflected_value_type_e_v2::string)
			{
				string_t& str = *reinterpret_cast<string_t*>(data);
				in_stream >> str;
				return;
			}

			SFG_ASSERT(field.size != 0);
			const size_t read_size = field.size;
			in_stream.read_to_raw(data, read_size);
		}

		void field_to_json(const reflected_field_t& field, void* obj, void* user_data, nlohmann::json& out_json, bool dismiss_name = false)
		{
			u8* data = reinterpret_cast<u8*>(obj) + field.offset;

			if (field.value_type == reflected_value_type_e_v2::invalid)
			{
				SFG_ERR("can't write field to stream because value type is invalid!");
				return;
			}

			if (field.custom_serialization.to_json_fn)
			{
				nlohmann::json child_json = nlohmann::json::object();
				field.custom_serialization.to_json_fn(data, user_data, child_json);

				if (dismiss_name)
					out_json = child_json;
				else
					out_json[field.name] = child_json;

				return;
			}

			if (field.value_type == reflected_value_type_e_v2::container)
			{
				if (field.container_ops.get_element_ptr_fn == nullptr || field.container_ops.get_element_size_fn == nullptr)
					return;

				const size_t item_size = field.container_ops.get_element_size_fn(data);

				nlohmann::json container_json = nlohmann::json::array();

				for (size_t i = 0; i < item_size; i++)
				{
					u8*						element_data = field.container_ops.get_element_ptr_fn(data, i);
					const reflected_field_t temp_field	 = {
						.sub_type_id = field.container_ops.element_sub_type_id,
						.size		 = size_from_field_type(field.container_ops.element_value_type),
						.value_type	 = field.container_ops.element_value_type,
					};

					nlohmann::json elem_json = nlohmann::json::object();
					field_to_json(temp_field, element_data, user_data, elem_json, true);
					container_json.push_back(elem_json);
				}

				out_json[field.name] = container_json;

				return;
			}

			if (field.value_type == reflected_value_type_e_v2::object)
			{
				if (dismiss_name)
					reflection_registry_v2::get().type_to_json(field.sub_type_id, data, user_data, out_json);
				else
				{
					nlohmann::json child_json = nlohmann::json::object();
					reflection_registry_v2::get().type_to_json(field.sub_type_id, data, user_data, child_json);
					out_json[field.name] = child_json;
				}

				return;
			}

			if (field.value_type == reflected_value_type_e_v2::string)
			{
				string_t* str = reinterpret_cast<string_t*>(data);
				if (dismiss_name)
					out_json = *str;
				else
					out_json[field.name] = *str;
				return;
			}

			switch (field.value_type)
			{
			case reflected_value_type_e_v2::boolean:
				if (dismiss_name)
					out_json = *reinterpret_cast<const bool*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const bool*>(data);
				break;
			case reflected_value_type_e_v2::f32: {
				float v = *reinterpret_cast<const float*>(data);
				v		= math::round(v * FLOAT_ROUND_PRECISION_JSON) / FLOAT_ROUND_PRECISION_JSON;
				if (dismiss_name)
					out_json = v;
				else
					out_json[field.name] = v;
				break;
			}

			case reflected_value_type_e_v2::u64:
				if (dismiss_name)
					out_json = *reinterpret_cast<const u64*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const u64*>(data);
				break;

			case reflected_value_type_e_v2::i64:
				if (dismiss_name)
					out_json = *reinterpret_cast<const i64*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const i64*>(data);
				break;

			case reflected_value_type_e_v2::u32:
				if (dismiss_name)
					out_json = *reinterpret_cast<const u32*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const u32*>(data);
				break;

			case reflected_value_type_e_v2::i32:
				if (dismiss_name)
					out_json = *reinterpret_cast<const i32*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const i32*>(data);
				break;

			case reflected_value_type_e_v2::u16:
				if (dismiss_name)
					out_json = *reinterpret_cast<const u16*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const u16*>(data);
				break;

			case reflected_value_type_e_v2::i16:
				if (dismiss_name)
					out_json = *reinterpret_cast<const i16*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const i16*>(data);
				break;

			case reflected_value_type_e_v2::u8:
				if (dismiss_name)
					out_json = *reinterpret_cast<const u8*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const u8*>(data);
				break;

			case reflected_value_type_e_v2::i8:
				if (dismiss_name)
					out_json = *reinterpret_cast<const i8*>(data);
				else
					out_json[field.name] = *reinterpret_cast<const i8*>(data);
				break;

			default:
				SFG_ERR("unsupported reflected value type");
				break;
			}
		}

		void field_from_json(const reflected_field_t& field, void* obj, void* user_data, const nlohmann::json& in_json, bool dismiss_name = false)
		{
			u8* data = reinterpret_cast<u8*>(obj) + field.offset;

			if (field.value_type == reflected_value_type_e_v2::invalid)
			{
				SFG_ERR("can't write field to stream because value type is invalid!");
				return;
			}

			if (field.custom_serialization.from_json_fn)
			{
				if (dismiss_name)
					field.custom_serialization.from_json_fn(data, user_data, in_json);
				else
				{
					const nlohmann::json child_json = in_json.value<nlohmann::json>(field.name, {});
					field.custom_serialization.from_json_fn(data, user_data, child_json);
				}

				return;
			}

			if (field.value_type == reflected_value_type_e_v2::container)
			{
				if (field.container_ops.reset_fn == nullptr || field.container_ops.add_element_ptr_fn == nullptr)
					return;

				if (!in_json.contains(field.name))
					return;

				const nlohmann::json container_json = in_json.value<nlohmann::json>(field.name, nlohmann::json::array());
				if (!container_json.is_array())
					return;

				const size_t sz = container_json.size();
				field.container_ops.reset_fn(data);

				for (size_t i = 0; i < sz; i++)
				{
					u8* element_data = field.container_ops.add_element_ptr_fn(data);
					if (element_data == nullptr)
						return;

					const reflected_field_t temp_field = {
						.sub_type_id = field.container_ops.element_sub_type_id,
						.size		 = size_from_field_type(field.container_ops.element_value_type),
						.value_type	 = field.container_ops.element_value_type,
					};
					field_from_json(temp_field, element_data, user_data, container_json[i], true);
				}

				return;
			}

			if (field.value_type == reflected_value_type_e_v2::object)
			{
				if (dismiss_name)
					reflection_registry_v2::get().type_from_json(field.sub_type_id, data, user_data, in_json);
				else
				{
					if (!in_json.contains(field.name))
						return;
					const nlohmann::json child_json = in_json.value<nlohmann::json>(field.name, {});
					reflection_registry_v2::get().type_from_json(field.sub_type_id, data, user_data, child_json);
				}

				return;
			}

			if (field.value_type == reflected_value_type_e_v2::string)
			{
				string_t& str = *reinterpret_cast<string_t*>(data);
				if (dismiss_name)
					str = in_json;
				else
				{
					if (!in_json.contains(field.name))
						return;
					str = in_json.value<string_t>(field.name, "");
				}
				return;
			}

			if (!dismiss_name && !in_json.contains(field.name))
				return;

			switch (field.value_type)
			{
			case reflected_value_type_e_v2::boolean:
				if (dismiss_name)
					*reinterpret_cast<bool*>(data) = in_json;
				else
					*reinterpret_cast<bool*>(data) = in_json.value<bool>(field.name, false);
				break;
			case reflected_value_type_e_v2::f32:
				if (dismiss_name)
					*reinterpret_cast<f32*>(data) = in_json;
				else
					*reinterpret_cast<f32*>(data) = in_json.value<f32>(field.name, 0.0f);
				break;

			case reflected_value_type_e_v2::u64:
				if (dismiss_name)
					*reinterpret_cast<u64*>(data) = in_json;
				else
					*reinterpret_cast<u64*>(data) = in_json.value<u64>(field.name, 0);
				break;

			case reflected_value_type_e_v2::i64:
				if (dismiss_name)
					*reinterpret_cast<i64*>(data) = in_json;
				else
					*reinterpret_cast<i64*>(data) = in_json.value<i64>(field.name, 0);
				break;

			case reflected_value_type_e_v2::u32:
				if (dismiss_name)
					*reinterpret_cast<u32*>(data) = in_json;
				else
					*reinterpret_cast<u32*>(data) = in_json.value<u32>(field.name, 0);
				break;

			case reflected_value_type_e_v2::i32:
				if (dismiss_name)
					*reinterpret_cast<i32*>(data) = in_json;
				else
					*reinterpret_cast<i32*>(data) = in_json.value<i32>(field.name, 0);
				break;

			case reflected_value_type_e_v2::u16:
				if (dismiss_name)
					*reinterpret_cast<u16*>(data) = in_json;
				else
					*reinterpret_cast<u16*>(data) = in_json.value<u16>(field.name, 0);
				break;

			case reflected_value_type_e_v2::i16:
				if (dismiss_name)
					*reinterpret_cast<i16*>(data) = in_json;
				else
					*reinterpret_cast<i16*>(data) = in_json.value<i16>(field.name, 0);
				break;

			case reflected_value_type_e_v2::u8:
				if (dismiss_name)
					*reinterpret_cast<u8*>(data) = in_json;
				else
					*reinterpret_cast<u8*>(data) = in_json.value<u8>(field.name, 0);
				break;

			case reflected_value_type_e_v2::i8:
				if (dismiss_name)
					*reinterpret_cast<i8*>(data) = in_json;
				else
					*reinterpret_cast<i8*>(data) = in_json.value<i8>(field.name, 0);
				break;

			default:
				SFG_ERR("unsupported reflected value type");
				break;
			}
		}
	}

	reflection_registry_v2::reflection_registry_v2()
	{
		init();
	}

	reflection_registry_v2::~reflection_registry_v2()
	{
		uninit();
	}

	void reflection_registry_v2::init()
	{
		_types.reserve(512);
		_fields.reserve(1024);
		_text_allocator.init(16 * 1024);
	}

	void reflection_registry_v2::uninit()
	{
		_types.clear();
		_fields.clear();
		_text_allocator.uninit();
	}

	void reflection_registry_v2::type_field_to_stream(sid_t type_id, sid_t field_id, void* obj, void* user_data, ostream_t& out_stream)
	{
		reflected_type_t* type = find_type(type_id);
		if (type == nullptr)
		{
			SFG_ERR("type id could not be found! {0}", type_id);
			return;
		}

		for (size_t i = type->fields.start; i < type->fields.end; i++)
		{
			const reflected_field_t& field = _fields[i];
			if (field.field_identifier == field_id)
			{
				if (field.flags.is_set(reflected_field_flags_e::reflected_field_flag_no_serialization))
					return;

				field_to_stream(field, obj, user_data, out_stream);
				return;
			}
		}

		SFG_ERR("field id could not be found! field: {0}, type: {1}", field_id, type_id);
	}

	void reflection_registry_v2::type_field_to_json(sid_t type_id, sid_t field_id, void* obj, void* user_data, nlohmann::json& out_json)
	{
		reflected_type_t* type = find_type(type_id);
		if (type == nullptr)
		{
			SFG_ERR("type id could not be found! {0}", type_id);
			return;
		}

		for (size_t i = type->fields.start; i < type->fields.end; i++)
		{
			const reflected_field_t& field = _fields[i];
			if (field.field_identifier == field_id)
			{
				if (field.flags.is_set(reflected_field_flags_e::reflected_field_flag_no_serialization))
					return;

				field_to_json(field, obj, user_data, out_json);
				return;
			}
		}

		SFG_ERR("field id could not be found! field: {0}, type: {1}", field_id, type_id);
	}

	void reflection_registry_v2::type_field_from_stream(sid_t type_id, sid_t field_id, void* obj, void* user_data, istream_t& in_stream)
	{
		reflected_type_t* type = find_type(type_id);
		if (type == nullptr)
		{
			SFG_ERR("type id could not be found! {0}", type_id);
			return;
		}
		for (size_t i = type->fields.start; i < type->fields.end; i++)
		{
			const reflected_field_t& field = _fields[i];
			if (field.field_identifier == field_id)
			{
				if (field.flags.is_set(reflected_field_flags_e::reflected_field_flag_no_serialization))
					return;

				field_from_stream(field, obj, user_data, in_stream);
				return;
			}
		}

		SFG_ERR("field id could not be found! field: {0}, type: {1}", field_id, type_id);
	}

	void reflection_registry_v2::type_field_from_json(sid_t type_id, sid_t field_id, void* obj, void* user_data, const nlohmann::json& in_json)
	{
		reflected_type_t* type = find_type(type_id);
		if (type == nullptr)
		{
			SFG_ERR("type id could not be found! {0}", type_id);
			return;
		}

		for (size_t i = type->fields.start; i < type->fields.end; i++)
		{
			const reflected_field_t& field = _fields[i];
			if (field.field_identifier == field_id)
			{
				if (field.flags.is_set(reflected_field_flags_e::reflected_field_flag_no_serialization))
					return;

				field_from_json(field, obj, user_data, in_json);
				return;
			}
		}

		SFG_ERR("field id could not be found! field: {0}, type: {1}", field_id, type_id);
	}

	void reflection_registry_v2::type_to_stream(sid_t type_id, void* obj, void* user_data, ostream_t& out_stream)
	{
		reflected_type_t* type = find_type(type_id);
		if (type == nullptr)
		{
			SFG_ERR("type id could not be found! {0}", type_id);
			return;
		}

		if (type->flags.is_set(reflected_type_flags_e::reflected_type_flag_no_serialization))
			return;

		if (type->flags.is_set(reflected_type_flags_e::reflected_type_flag_enum))
		{
			out_stream.write_raw(reinterpret_cast<u8*>(obj), type->size);
			return;
		}

		for (size_t i = type->fields.start; i < type->fields.end; i++)
		{
			const reflected_field_t& field = _fields[i];
			field_to_stream(field, obj, user_data, out_stream);
		}
	}

	void reflection_registry_v2::type_to_json(sid_t type_id, void* obj, void* user_data, nlohmann::json& out_json)
	{
		reflected_type_t* type = find_type(type_id);
		if (type == nullptr)
		{
			SFG_ERR("type id could not be found! {0}", type_id);
			return;
		}

		if (type->flags.is_set(reflected_type_flags_e::reflected_type_flag_no_serialization))
			return;

		if (type->flags.is_set(reflected_type_flags_e::reflected_type_flag_enum))
		{
			if (type->size == sizeof(u8))
			{
				out_json[type->name] = *reinterpret_cast<u8*>(obj);
			}
			else if (type->size == sizeof(u16))
			{
				out_json[type->name] = *reinterpret_cast<u16*>(obj);
			}
			else if (type->size == sizeof(u32))
			{
				out_json[type->name] = *reinterpret_cast<u32*>(obj);
			}
			else
			{
				SFG_ERR("enum type size mismatch!");
			}
			return;
		}

		nlohmann::json type_json = nlohmann::json::object();

		for (size_t i = type->fields.start; i < type->fields.end; i++)
		{
			const reflected_field_t& field = _fields[i];
			field_to_json(field, obj, user_data, type_json);
		}

		out_json[type->name] = type_json;
	}

	void reflection_registry_v2::type_from_stream(sid_t type_id, void* obj, void* user_data, istream_t& in_stream)
	{
		reflected_type_t* type = find_type(type_id);
		if (type == nullptr)
		{
			SFG_ERR("type id could not be found! {0}", type_id);
			return;
		}

		if (type->flags.is_set(reflected_type_flags_e::reflected_type_flag_no_serialization))
			return;

		if (type->flags.is_set(reflected_type_flags_e::reflected_type_flag_enum))
		{
			u8* ptr = reinterpret_cast<u8*>(obj);
			in_stream.read_to_raw(ptr, type->size);
			return;
		}

		for (size_t i = type->fields.start; i < type->fields.end; i++)
		{
			const reflected_field_t& field = _fields[i];
			field_from_stream(field, obj, user_data, in_stream);
		}
	}

	void reflection_registry_v2::type_from_json(sid_t type_id, void* obj, void* user_data, const nlohmann::json& in_json)
	{
		reflected_type_t* type = find_type(type_id);
		if (type == nullptr)
		{
			SFG_ERR("type id could not be found! {0}", type_id);
			return;
		}

		if (type->flags.is_set(reflected_type_flags_e::reflected_type_flag_no_serialization))
			return;

		if (!in_json.contains(type->name))
			return;

		if (type->flags.is_set(reflected_type_flags_e::reflected_type_flag_enum))
		{
			if (type->size == sizeof(u8))
			{
				*reinterpret_cast<u8*>(obj) = in_json.value<u8>(type->name, 0);
			}
			else if (type->size == sizeof(u16))
			{
				*reinterpret_cast<u16*>(obj) = in_json.value<u16>(type->name, 0);
			}
			else if (type->size == sizeof(u32))
			{
				*reinterpret_cast<u32*>(obj) = in_json.value<u32>(type->name, 0);
			}
			else
			{
				SFG_ERR("enum type size mismatch!");
			}
			return;
		}

		nlohmann::json type_json = in_json.value<nlohmann::json>(type->name, {});

		for (size_t i = type->fields.start; i < type->fields.end; i++)
		{
			const reflected_field_t& field = _fields[i];
			field_from_json(field, obj, user_data, type_json);
		}
	}

	reflected_type_t* reflection_registry_v2::find_type(sid_t type_id)
	{
		auto it = std::find_if(_types.begin(), _types.end(), [type_id](const reflected_type_t& t) -> bool { return t.type_id == type_id; });
		if (it == _types.end())
			return nullptr;
		return &*it;
	}

	void reflection_registry_v2::register_type(const reflected_type_descriptor_t& descriptor)
	{
		SFG_ASSERT(descriptor.name != nullptr);
		SFG_ASSERT(descriptor.type_id != 0);
		SFG_ASSERT(descriptor.size != 0);

		reflected_type_t* existing_type = find_type(descriptor.type_id);
		if (existing_type != nullptr)
		{
			SFG_WARN("reflection type already exists! {0}", existing_type->name);
			return;
		}

		reflected_type_t type = {};

		type.name = _text_allocator.allocate(descriptor.name);

		if (descriptor.tooltip != nullptr)
			type.tooltip = _text_allocator.allocate(descriptor.tooltip);

		if (descriptor.display_name != nullptr)
			type.display_name = _text_allocator.allocate(descriptor.display_name);

		type.type_id   = descriptor.type_id;
		type.size	   = descriptor.size;
		type.alignment = descriptor.alignment;
		type.flags	   = descriptor.flags;

		if (descriptor.flags.is_set(reflected_type_flags_e::reflected_type_flag_enum))
		{
			SFG_ASSERT(descriptor.size == sizeof(u8) || descriptor.size == sizeof(u16) || descriptor.size == sizeof(u32));
		}

		type.fields.start = static_cast<u32>(_fields.size());

		for (const reflected_field_descriptor_t& field_desc : descriptor.fields)
		{
			reflected_field_t field = {};
			SFG_ASSERT(field_desc.name != nullptr);
			SFG_ASSERT(field_desc.type != reflected_value_type_e_v2::object || field_desc.sub_type_id != 0);
			SFG_ASSERT(field_desc.type != reflected_value_type_e_v2::container || field_desc.container_ops.get_element_ptr_fn != nullptr);
			SFG_ASSERT(field_desc.type != reflected_value_type_e_v2::container || field_desc.container_ops.get_element_size_fn != nullptr);
			SFG_ASSERT(field_desc.type != reflected_value_type_e_v2::container || field_desc.container_ops.add_element_ptr_fn != nullptr);
			SFG_ASSERT(field_desc.type != reflected_value_type_e_v2::container || field_desc.container_ops.reset_fn != nullptr);
			SFG_ASSERT(field_desc.type != reflected_value_type_e_v2::container || (field_desc.container_ops.element_value_type != reflected_value_type_e_v2::invalid && field_desc.container_ops.element_value_type != reflected_value_type_e_v2::container));
			SFG_ASSERT(field_desc.type != reflected_value_type_e_v2::container || field_desc.container_ops.element_value_type != reflected_value_type_e_v2::object || field_desc.container_ops.element_sub_type_id != 0);

			field.name			   = _text_allocator.allocate(field_desc.name);
			field.field_identifier = TO_SID(field.name);

			if (field_desc.tooltip != nullptr)
				field.tooltip = _text_allocator.allocate(field_desc.tooltip);

			if (field_desc.display_name != nullptr)
				field.display_name = _text_allocator.allocate(field_desc.display_name);

			field.value_type  = field_desc.type;
			field.sub_type_id = field_desc.sub_type_id;
			field.flags		  = field_desc.flags;
			field.offset	  = field_desc.offset;
			field.size		  = size_from_field_type(field_desc.type);

			field.ui_definition		   = field_desc.ui_definition;
			field.container_ops		   = field_desc.container_ops;
			field.custom_serialization = field_desc.custom_serialization;

			_fields.push_back(field);
		}

		type.fields.end = static_cast<u32>(_fields.size());
		_types.push_back(type);
	}
}
