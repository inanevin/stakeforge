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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/static_vector.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/text_allocator.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

#include <cstddef>
#include <utility>

namespace sfg
{
	class istream_t;
	class ostream_t;

	enum class reflected_value_type_e : u8
	{
		invalid = 0,
		f32,
		i32,
		u32,
		u8,
		bool8,
		vec2,
		vec3,
		vec4,
		vec2u,
		vec2u16,
		vec3u,
		vec4u,
		color,
		resource,
		entity_id,
		string,
		json,
		quat,
		enum8,
		enum32,
		object,
		vector,
		static_vector,
	};

	reflected_value_type_e reflected_value_type_from_sub_type_id(sid_t sub_type_id);
	u32					   reflected_value_type_size(reflected_value_type_e type);

	enum reflected_field_flags_e : u32
	{
		reflected_field_flags_none		= 0,
		reflected_field_flags_read_only = 1 << 0,
		reflected_field_flags_no_ui		= 1 << 1,
		reflected_field_flags_clamped	= 1 << 2,
		reflected_field_flags_transient = 1 << 3,
		reflected_field_flags_bitmask	= 1 << 4,
	};

	enum reflected_type_flags_e : u32
	{
		reflected_type_flags_none	   = 0,
		reflected_type_flags_component = 1 << 0,
		reflected_type_flags_resource  = 1 << 1,
		reflected_type_flags_script	   = 1 << 2,
	};

	struct reflected_field_desc_t;

	using reflected_get_fn						= bool (*)(const void* object, const reflected_field_desc_t& field, void* out_value, void* user_data);
	using reflected_set_fn						= bool (*)(void* object, const reflected_field_desc_t& field, const void* value, void* user_data);
	using reflected_container_get_count_fn		= u32 (*)(const void* object, const reflected_field_desc_t& field);
	using reflected_container_get_item_fn		= void* (*)(void* object, const reflected_field_desc_t& field, u32 index);
	using reflected_container_get_const_item_fn = const void* (*)(const void* object, const reflected_field_desc_t& field, u32 index);
	using reflected_container_clear_fn			= void (*)(void* object, const reflected_field_desc_t& field);
	using reflected_container_resize_fn			= bool (*)(void* object, const reflected_field_desc_t& field, u32 size);
	using reflected_container_add_fn			= bool (*)(void* object, const reflected_field_desc_t& field);
	using reflected_container_remove_fn			= bool (*)(void* object, const reflected_field_desc_t& field, u32 index);

	struct reflected_container_ops_t
	{
		reflected_container_get_count_fn	  get_count		 = nullptr;
		reflected_container_get_item_fn		  get_item		 = nullptr;
		reflected_container_get_const_item_fn get_const_item = nullptr;
		reflected_container_clear_fn		  clear			 = nullptr;
		reflected_container_resize_fn		  resize		 = nullptr;
		reflected_container_add_fn			  add			 = nullptr;
		reflected_container_remove_fn		  remove		 = nullptr;
	};

	struct reflected_enum_value_desc_t
	{
		const char* name		 = nullptr;
		const char* display_name = nullptr;
		sid_t		id			 = 0;
		i64			value		 = 0;
	};

	struct reflected_field_desc_t
	{
		span_t<const reflected_enum_value_desc_t> enum_values	= {};
		reflected_get_fn						  get			= nullptr;
		reflected_set_fn						  set			= nullptr;
		void*									  user_data		= nullptr;
		const char*								  name			= nullptr;
		const char*								  display_name	= nullptr;
		reflected_value_type_e					  type			= reflected_value_type_e::invalid;
		sid_t									  id			= 0;
		sid_t									  value_type_id = 0;
		sid_t									  sub_type_id	= 0;
		reflected_container_ops_t				  container_ops = {};
		u32										  offset		= 0;
		u32										  size			= 0;
		u32										  stride		= 0;
		u32										  capacity		= 0;
		f32										  min			= 0.0f;
		f32										  max			= 0.0f;
		u32										  flags			= reflected_field_flags_none;
	};

	struct reflected_type_desc_t
	{
		span_t<const reflected_field_desc_t>	  fields	   = {};
		span_t<const reflected_enum_value_desc_t> enum_values  = {};
		void*									  user_data	   = nullptr;
		const char*								  name		   = nullptr;
		const char*								  display_name = nullptr;
		const char*								  category	   = nullptr;
		sid_t									  type_id	   = 0;
		u32										  size		   = 0;
		u32										  alignment	   = 0;
		u32										  flags		   = reflected_type_flags_none;
	};

	class reflection_registry_t final
	{
	public:
		static reflection_registry_t& get();

		reflection_registry_t();
		~reflection_registry_t();
		reflection_registry_t(const reflection_registry_t&)			   = delete;
		reflection_registry_t& operator=(const reflection_registry_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void reset();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		bool register_type(const reflected_type_desc_t& desc);
		bool serialize_to_json(sid_t type_id, const void* obj, nlohmann::json& j) const;
		bool serialize_to_stream(sid_t type_id, const void* obj, ostream_t& stream) const;
		bool deserialize_from_json(sid_t type_id, void* obj, const nlohmann::json& j) const;
		bool deserialize_from_stream(sid_t type_id, void* obj, istream_t& stream) const;

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		const reflected_type_desc_t*  find_type(sid_t type_id) const;
		const reflected_type_desc_t&  get_type(sid_t type_id) const;
		const reflected_field_desc_t* find_field(sid_t type_id, sid_t field_id) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		span_t<const reflected_type_desc_t> get_types() const;
		bool								is_initialized() const;

	private:
		u32							find_type_index(sid_t type_id) const;
		const char*					copy_text(const char* text);
		sid_t						resolve_id(sid_t id, const char* name) const;
		reflected_enum_value_desc_t copy_enum_value(const reflected_enum_value_desc_t& desc);
		reflected_field_desc_t		copy_field(const reflected_field_desc_t& desc);
		reflected_type_desc_t		copy_type(const reflected_type_desc_t& desc, u32 field_start, u32 field_count);

	private:
		text_allocator_t					  _text;
		vector_t<reflected_type_desc_t>		  _types;
		vector_t<reflected_field_desc_t>	  _fields;
		vector_t<reflected_enum_value_desc_t> _enum_values;
		bool								  _initialized = false;
	};

	inline const void* get_reflected_field_data_ptr(const void* object, const reflected_field_desc_t& field)
	{
		return static_cast<const u8*>(object) + field.offset;
	}

	inline void* get_reflected_field_data_ptr(void* object, const reflected_field_desc_t& field)
	{
		return static_cast<u8*>(object) + field.offset;
	}

	template <typename T> vector_t<T>& get_reflected_vector_field(void* object, const reflected_field_desc_t& field)
	{
		return *static_cast<vector_t<T>*>(get_reflected_field_data_ptr(object, field));
	}

	template <typename T> const vector_t<T>& get_reflected_vector_field(const void* object, const reflected_field_desc_t& field)
	{
		return *static_cast<const vector_t<T>*>(get_reflected_field_data_ptr(object, field));
	}

	template <typename T> u32 reflected_vector_get_count(const void* object, const reflected_field_desc_t& field)
	{
		return static_cast<u32>(get_reflected_vector_field<T>(object, field).size());
	}

	template <typename T> void* reflected_vector_get_item(void* object, const reflected_field_desc_t& field, u32 index)
	{
		vector_t<T>& values = get_reflected_vector_field<T>(object, field);
		return values.data() + index;
	}

	template <typename T> const void* reflected_vector_get_const_item(const void* object, const reflected_field_desc_t& field, u32 index)
	{
		const vector_t<T>& values = get_reflected_vector_field<T>(object, field);
		return values.data() + index;
	}

	template <typename T> void reflected_vector_clear(void* object, const reflected_field_desc_t& field)
	{
		get_reflected_vector_field<T>(object, field).resize(0);
	}

	template <typename T> bool reflected_vector_resize(void* object, const reflected_field_desc_t& field, u32 size)
	{
		get_reflected_vector_field<T>(object, field).resize(size);
		return true;
	}

	template <typename T> bool reflected_vector_add(void* object, const reflected_field_desc_t& field)
	{
		get_reflected_vector_field<T>(object, field).push_back(T{});
		return true;
	}

	template <typename T> bool reflected_vector_remove(void* object, const reflected_field_desc_t& field, u32 index)
	{
		vector_t<T>& values = get_reflected_vector_field<T>(object, field);
		if (index >= values.size())
			return false;
		values.erase(values.begin() + index);
		return true;
	}

	template <typename T> reflected_container_ops_t reflected_vector_ops()
	{
		return {
			.get_count		= reflected_vector_get_count<T>,
			.get_item		= reflected_vector_get_item<T>,
			.get_const_item = reflected_vector_get_const_item<T>,
			.clear			= reflected_vector_clear<T>,
			.resize			= reflected_vector_resize<T>,
			.add			= reflected_vector_add<T>,
			.remove			= reflected_vector_remove<T>,
		};
	}

	template <typename T, int N> static_vector_t<T, N>& get_reflected_static_vector_field(void* object, const reflected_field_desc_t& field)
	{
		return *static_cast<static_vector_t<T, N>*>(get_reflected_field_data_ptr(object, field));
	}

	template <typename T, int N> const static_vector_t<T, N>& get_reflected_static_vector_field(const void* object, const reflected_field_desc_t& field)
	{
		return *static_cast<const static_vector_t<T, N>*>(get_reflected_field_data_ptr(object, field));
	}

	template <typename T, int N> u32 reflected_static_vector_get_count(const void* object, const reflected_field_desc_t& field)
	{
		return static_cast<u32>(get_reflected_static_vector_field<T, N>(object, field).size());
	}

	template <typename T, int N> void* reflected_static_vector_get_item(void* object, const reflected_field_desc_t& field, u32 index)
	{
		static_vector_t<T, N>& values = get_reflected_static_vector_field<T, N>(object, field);
		return values.data() + index;
	}

	template <typename T, int N> const void* reflected_static_vector_get_const_item(const void* object, const reflected_field_desc_t& field, u32 index)
	{
		const static_vector_t<T, N>& values = get_reflected_static_vector_field<T, N>(object, field);
		return values.data() + index;
	}

	template <typename T, int N> void reflected_static_vector_clear(void* object, const reflected_field_desc_t& field)
	{
		get_reflected_static_vector_field<T, N>(object, field).clear();
	}

	template <typename T, int N> bool reflected_static_vector_resize(void* object, const reflected_field_desc_t& field, u32 size)
	{
		if (size > N)
			return false;
		get_reflected_static_vector_field<T, N>(object, field).resize(size);
		return true;
	}

	template <typename T, int N> bool reflected_static_vector_add(void* object, const reflected_field_desc_t& field)
	{
		static_vector_t<T, N>& values = get_reflected_static_vector_field<T, N>(object, field);
		if (values.full())
			return false;
		values.emplace_back();
		return true;
	}

	template <typename T, int N> bool reflected_static_vector_remove(void* object, const reflected_field_desc_t& field, u32 index)
	{
		static_vector_t<T, N>& values = get_reflected_static_vector_field<T, N>(object, field);
		if (index >= values.size())
			return false;
		for (size_t i = index; i + 1 < values.size(); ++i)
			values[i] = std::move(values[i + 1]);
		values.pop_back();
		return true;
	}

	template <typename T, int N> reflected_container_ops_t reflected_static_vector_ops()
	{
		return {
			.get_count		= reflected_static_vector_get_count<T, N>,
			.get_item		= reflected_static_vector_get_item<T, N>,
			.get_const_item = reflected_static_vector_get_const_item<T, N>,
			.clear			= reflected_static_vector_clear<T, N>,
			.resize			= reflected_static_vector_resize<T, N>,
			.add			= reflected_static_vector_add<T, N>,
			.remove			= reflected_static_vector_remove<T, N>,
		};
	}
}
