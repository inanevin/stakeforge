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

#define REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID	   "reflection_subtype_entity_guid"_hs
#define REFLECTION_SUB_TYPE_IDENTIFIER_DIRECTORY	   "reflection_subtype_directory"_hs
#define REFLECTION_SUB_TYPE_IDENTIFIER_PATH			   "reflection_subtype_path"_hs
#define REFLECTION_SUB_TYPE_IDENTIFIER_COLLISION_LAYER "reflection_subtype_collision_layer"_hs

	enum class reflected_value_type_e : u8
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
		bitmask,
	};

	struct bitmask_option_t
	{
		const char* name  = nullptr;
		u64			value = 0;
	};

	typedef u32 (*fn_bitmask_get_option_count)(void* user_data);
	typedef bitmask_option_t (*fn_bitmask_get_option)(u32 index, void* user_data);
	typedef const char* (*fn_bitmask_build_title)(u64 value, void* user_data);

	struct bitmask_opts_t
	{
		fn_bitmask_get_option_count get_option_count_fn = nullptr;
		fn_bitmask_get_option		get_option_fn		= nullptr;
		fn_bitmask_build_title		build_title_fn		= nullptr;
		void*						user_data			= nullptr;
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
	typedef size_t (*fn_container_get_element_size)(const void* obj);
	typedef void (*fn_container_reset)(void* obj);
	typedef void (*fn_container_remove_index)(void* obj, u32 index);

	struct reflected_field_container_ops_t
	{
		fn_container_add_element_ptr  add_element_ptr_fn  = nullptr;
		fn_container_reset			  reset_fn			  = nullptr;
		fn_container_remove_index	  remove_index_fn	  = nullptr;
		fn_container_get_element_ptr  get_element_ptr_fn  = nullptr;
		fn_container_get_element_size get_element_size_fn = nullptr;
		reflected_value_type_e		  element_value_type  = reflected_value_type_e::invalid;
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
		show_if_not_equal,
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
		reflected_value_type_e				   value_type			= reflected_value_type_e::invalid;
	};

	struct reflected_field_descriptor_t
	{
		reflected_field_container_ops_t		   container_ops		= {};
		reflected_field_custom_serialization_t custom_serialization = {};
		reflected_field_ui_definition_t		   ui_definition		= {};
		const char*							   name					= "";
		const char*							   display_name			= nullptr;
		const char*							   tooltip				= "";
		sid_t								   field_identifier		= 0;
		sid_t								   sub_type_id			= 0;
		size_t								   offset				= 0;
		size_t								   size					= 0;
		bitmask32							   flags				= 0;
		f32									   min_clamp			= 0.0f;
		f32									   max_clamp			= 0.0f;
		f32									   clamp_granularity	= 0.1f;
		reflected_value_type_e				   type					= reflected_value_type_e::invalid;
		reflected_value_type_e				   sub_type				= reflected_value_type_e::invalid;
	};

	enum reflected_type_flags_e
	{
		reflected_type_flag_component		 = 1 << 0,
		reflected_type_flag_resource		 = 1 << 1,
		reflected_type_flag_script			 = 1 << 2,
		reflected_type_flag_no_ui			 = 1 << 3,
		reflected_type_flag_no_serialization = 1 << 4,
		reflected_type_flag_enum			 = 1 << 5,
		reflected_type_flag_system_component = 1 << 6,
		reflected_type_flag_tag_component	 = 1 << 7,
		reflected_type_flag_renderable		 = 1 << 8,
	};

	enum class reflection_owner_e : u8
	{
		engine,
		game_scripts,
	};

	struct reflected_field_span_t
	{
		u32 start = 0;
		u32 end	  = 0;
	};

	typedef void (*fn_default_init)(void* obj);

	struct reflected_type_t
	{
		reflected_field_span_t fields		   = {};
		bitmask_opts_t		   bitmask_opts	   = {};
		const char*			   name			   = nullptr;
		const char*			   display_name	   = nullptr;
		const char*			   category		   = nullptr;
		const char*			   tooltip		   = nullptr;
		fn_default_init		   default_init_fn = nullptr;
		sid_t				   type_id		   = 0;
		size_t				   size			   = 0;
		size_t				   alignment	   = 0;
		bitmask32			   flags		   = 0;
		reflection_owner_e	   owner		   = reflection_owner_e::engine;
	};

	struct reflected_type_descriptor_t
	{
		const char*							   name			   = "";
		const char*							   display_name	   = nullptr;
		const char*							   category		   = nullptr;
		const char*							   tooltip		   = "";
		fn_default_init						   default_init_fn = nullptr;
		vector_t<reflected_field_descriptor_t> fields;
		bitmask_opts_t						   bitmask_opts = {};
		sid_t								   type_id		= 0;
		size_t								   size			= 0;
		size_t								   alignment	= 0;
		bitmask32							   flags		= 0;
		reflection_owner_e					   owner		= reflection_owner_e::engine;
	};

	class reflection_registry_t
	{
	public:
		// size_t reflected as u64 from most users atm.
		static_assert(sizeof(size_t) == sizeof(u64));
		static inline constexpr u32 SCRIPT_TYPE_CAPACITY  = 1024;
		static inline constexpr u32 SCRIPT_FIELD_CAPACITY = 2048;
		static inline constexpr u32 SCRIPT_TEXT_CAPACITY  = 256 * 1024;

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
		void initialize_type(sid_t type_id, void* obj) const;
		void remove_script_types();
		void reserve_script_capacity();
		bool is_script_capacity_valid(size_t type_count, size_t field_count) const;

		const reflected_type_t*	 find_type(sid_t type_id) const;
		const reflected_field_t* get_field(u32 index) const;

		inline const vector_t<reflected_type_t>& get_types() const
		{
			return _types;
		}

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
		text_allocator_t			_text_allocator		   = {};
		text_allocator_t			_script_text_allocator = {};
		size_t						_script_type_begin	   = 0;
		size_t						_script_field_begin	   = 0;
		bool						_script_capacity_ready = false;
	};
}
