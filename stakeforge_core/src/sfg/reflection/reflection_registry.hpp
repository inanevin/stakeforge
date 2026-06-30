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
#include <sfg/common/type_id.hpp>
#include <sfg/data/bitmask.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/text_allocator.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;

#define REFLECTION_SUB_TYPE_IDENTIFIER_WORLD_TEXT_ID "reflection_subtype_world_text_id"_hs
#define REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID	 "reflection_subtype_entity_guid"_hs
#define REFLECTION_SUB_TYPE_IDENTIFIER_RESOURCE_GUID "reflection_subtype_resource_guid"_hs

	enum class reflected_value_type_e_v2 : u8
	{
		invalid,
		f32,
		u64,
		i64,
		u32,
		i32,
		u16,
		i16,
		u8,
		i8,
		boolean,
		string,
		object,
		container,
		char_array,
	};

	enum reflected_field_flags_e
	{
		reflected_field_flag_none			  = 1 << 0,
		reflected_field_flag_no_ui			  = 1 << 1,
		reflected_field_flag_no_serialization = 1 << 2,
		reflected_field_flag_clamped		  = 1 << 3,
	};

	typedef u8* (*fn_container_get_element_ptr)(void* obj, u32 index);
	typedef u8* (*fn_container_add_element_ptr)(void* obj);
	typedef size_t (*fn_container_get_element_size)(void* obj);
	typedef void (*fn_container_reset)(void* obj);

	struct reflected_field_container_ops_t
	{
		fn_container_add_element_ptr  add_element_ptr_fn  = nullptr;
		fn_container_reset			  reset_fn			  = nullptr;
		fn_container_get_element_ptr  get_element_ptr_fn  = nullptr;
		fn_container_get_element_size get_element_size_fn = nullptr;
		reflected_value_type_e_v2	  element_value_type  = reflected_value_type_e_v2::invalid;
		sid_t						  element_sub_type_id = 0;
		size_t						  element_value_size  = 0;
	};

	typedef void (*fn_field_custom_to_stream)(void* obj, void* user_data, ostream_t& out_stream);
	typedef void (*fn_field_custom_to_json)(void* obj, void* user_data, nlohmann::json& out_json);
	typedef void (*fn_field_custom_from_stream)(void* obj, void* user_data, istream_t& in_stream);
	typedef void (*fn_field_custom_from_json)(void* obj, void* user_data, const nlohmann::json& in_json);

	struct reflected_field_custom_serialization_t
	{
		fn_field_custom_to_stream	to_stream_fn   = nullptr;
		fn_field_custom_to_json		to_json_fn	   = nullptr;
		fn_field_custom_from_stream from_stream_fn = nullptr;
		fn_field_custom_from_json	from_json_fn   = nullptr;
	};

	enum class reflected_field_dependency_type_e : u8
	{
		show_if_equals,
	};

	struct reflected_field_ui_definition_t
	{
		f32								  min_clamp			= 0.0f;
		f32								  max_clamp			= 0.0f;
		f32								  clamp_granularity = 0.1f;
		sid_t							  dependency_field	= 0;
		u32								  dependency_value	= 0;
		reflected_field_dependency_type_e dependency_type	= reflected_field_dependency_type_e::show_if_equals;
	};

	struct reflected_field_t
	{
		reflected_field_container_ops_t		   container_ops		= {};
		reflected_field_custom_serialization_t custom_serialization = {};
		reflected_field_ui_definition_t		   ui_definition		= {};
		const char*							   name					= nullptr;
		const char*							   display_name			= nullptr;
		const char*							   tooltip				= nullptr;
		sid_t								   field_identifier		= 0;
		sid_t								   sub_type_id			= 0;
		size_t								   offset				= 0;
		size_t								   size					= 0;
		bitmask32							   flags				= 0;
		reflected_value_type_e_v2			   value_type			= reflected_value_type_e_v2::invalid;
	};

	struct reflected_field_descriptor_t
	{
		reflected_field_container_ops_t		   container_ops		= {};
		reflected_field_custom_serialization_t custom_serialization = {};
		reflected_field_ui_definition_t		   ui_definition		= {};
		const char*							   name					= "";
		const char*							   display_name			= nullptr;
		const char*							   tooltip				= "";
		sid_t								   sub_type_id			= 0;
		size_t								   offset				= 0;
		size_t								   size					= 0;
		bitmask32							   flags				= 0;
		f32									   min_clamp			= 0.0f;
		f32									   max_clamp			= 0.0f;
		f32									   clamp_granularity	= 0.1f;
		reflected_value_type_e_v2			   type					= reflected_value_type_e_v2::invalid;
		reflected_value_type_e_v2			   sub_type				= reflected_value_type_e_v2::invalid;
	};

	enum reflected_type_flags_e
	{
		reflected_type_flag_component		 = 1 << 0,
		reflected_type_flag_resource		 = 1 << 1,
		reflected_type_flag_script			 = 1 << 2,
		reflected_type_flag_no_ui			 = 1 << 3,
		reflected_type_flag_no_serialization = 1 << 4,
		reflected_type_flag_enum			 = 1 << 5,
	};

	struct reflected_field_span_t
	{
		u32 start = 0;
		u32 end	  = 0;
	};

	struct reflected_type_t
	{
		reflected_field_span_t fields		= {};
		reflected_field_span_t enum_fields	= {};
		const char*			   name			= nullptr;
		const char*			   display_name = nullptr;
		const char*			   tooltip		= nullptr;
		sid_t				   type_id		= 0;
		size_t				   size			= 0;
		size_t				   alignment	= 0;
		bitmask32			   flags		= 0;
	};

	struct reflected_type_descriptor_t
	{
		const char*							   name			= "";
		const char*							   display_name = nullptr;
		const char*							   tooltip		= "";
		vector_t<reflected_field_descriptor_t> fields;
		sid_t								   type_id	 = 0;
		size_t								   size		 = 0;
		size_t								   alignment = 0;
		bitmask32							   flags	 = 0;
	};

	class reflection_registry_t
	{
	public:
		// size_t reflected as u64 from most users atm.
		static_assert(sizeof(size_t) == sizeof(u64));

		static inline reflection_registry_t& get()
		{
			static reflection_registry_t instance;
			return instance;
		}

		reflection_registry_t();
		~reflection_registry_t();
		void init();
		void uninit();

		void type_field_to_stream(sid_t type_id, sid_t field_id, void* obj, void* user_data, ostream_t& out_stream);
		void type_field_to_json(sid_t type_id, sid_t field_id, void* obj, void* user_data, nlohmann::json& out_json);
		void type_field_from_stream(sid_t type_id, sid_t field_id, void* obj, void* user_data, istream_t& in_stream);
		void type_field_from_json(sid_t type_id, sid_t field_id, void* obj, void* user_data, const nlohmann::json& in_json);
		bool type_to_stream(sid_t type_id, void* obj, void* user_data, ostream_t& out_stream);
		bool type_to_json(sid_t type_id, void* obj, void* user_data, nlohmann::json& out_json);
		bool type_from_stream(sid_t type_id, void* obj, void* user_data, istream_t& in_stream);
		bool type_from_json(sid_t type_id, void* obj, void* user_data, const nlohmann::json& in_json);

		reflected_type_t* find_type(sid_t type_id);

		void register_type(const reflected_type_descriptor_t& descriptor);

		template <typename T> void register_type(const char* name, const char* tooltip, bitmask32 flags, const vector_t<reflected_field_descriptor_t>& fields)
		{
			const reflected_type_descriptor_t desc = {
				.name	   = name,
				.tooltip   = tooltip,
				.fields	   = fields,
				.type_id   = type_id_t<T>::value,
				.size	   = sizeof(T),
				.alignment = alignof(T),
				.flags	   = flags,
			};

			register_type(desc);
		}

	private:
		vector_t<reflected_type_t>	_types;
		vector_t<reflected_field_t> _fields;
		text_allocator_t			_text_allocator = {};
	};
}
