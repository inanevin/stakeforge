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
#include <sfg/data/bitmask.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/inplace_vector.hpp>
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
		u64,
		u8,
		bool8,
		audio_handle,
		font_handle,
		mesh_handle,
		skeleton_handle,
		animation_handle,
		material_handle,
		shader_handle,
		texture_handle,
		texture_sampler_handle,
		physical_material_handle,
		prefab_handle,
		animation_state_machine_handle,
		hdr_skybox_handle,
		entity_guid,
		text_id,
		string,
		json,
		quat,
		enum8,
		enum32,
		object,
		vector,
		inplace_vector,
		i8,
		i16,
		u16,
		size_t,
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
		reflected_type_flags_none		  = 0,
		reflected_type_flags_component	  = 1 << 0,
		reflected_type_flags_resource	  = 1 << 1,
		reflected_type_flags_script		  = 1 << 2,
		reflected_type_flags_no_ui		  = 1 << 3,
		reflected_type_flags_no_serialize = 1 << 4,
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
		const char*								  name			= nullptr;
		const char*								  display_name	= nullptr;
		const char*								  tooltip		= nullptr;
		reflected_value_type_e					  type			= reflected_value_type_e::invalid;
		sid_t									  id			= 0;
		sid_t									  value_type_id = 0;
		sid_t									  sub_type_id	= 0;
		u32										  offset		= 0;
		u32										  size			= 0;
		u32										  stride		= 0;
		u32										  capacity		= 0;
		f32										  min			= 0.0f;
		f32										  max			= 0.0f;
		bitmask32								  flags			= reflected_field_flags_none;
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
		bitmask32								  flags		   = reflected_type_flags_none;
	};

	class reflection_registry_t final
	{
	public:
		static reflection_registry_t& get();
		static bool					  is_resource_type(reflected_value_type_e type);

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
		bool serialize_field_to_stream(const void* obj, const reflected_field_desc_t& field, ostream_t& stream) const;
		bool deserialize_field_from_stream(void* obj, const reflected_field_desc_t& field, istream_t& stream) const;
		bool serialize_field_to_stream(sid_t type_id, sid_t field_id, const void* obj, ostream_t& stream) const;
		bool deserialize_field_from_stream(sid_t type_id, sid_t field_id, void* obj, istream_t& stream) const;

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

}
