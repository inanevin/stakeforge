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
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
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

	void reflection_registry_t::init(const reflection_registry_config_t& config)
	{
		if (_initialized)
			uninit();

		_config = config;
		_text.init(_config.text_bytes);
		_types.reserve(_config.max_types);
		_fields.reserve(_config.max_fields);
		_enum_values.reserve(_config.max_enum_values);
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

		u32 enum_count = 0;
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
		const char*			  name	 = copy_text(desc.name);
		reflected_type_desc_t copied = desc;
		copied.fields				 = field_count != 0 ? span_t<const reflected_field_desc_t>{.data = _fields.data() + field_start, .size = field_count} : span_t<const reflected_field_desc_t>{};
		copied.name					 = name;
		copied.display_name			 = copy_text(desc.display_name != nullptr ? desc.display_name : desc.name);
		copied.category				 = copy_text(desc.category);
		return copied;
	}
}
