/*
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

#include "assets/editor_asset_type.hpp"

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define DEFAULT_GBUFFER_SHADER_ASSET_GUID	   1000
#define DEFAULT_FORWARD_SHADER_ASSET_GUID	   1001
#define DEFAULT_ALBEDO_TEXTURE_ASSET_GUID	   1002
#define DEFAULT_ORM_TEXTURE_ASSET_GUID		   1003
#define DEFAULT_NORMAL_TEXTURE_ASSET_GUID	   1004
#define DEFAULT_EMISSIVE_TEXTURE_ASSET_GUID	   1005
#define DEFAULT_GBUFFER_MATERIAL_ASSET_GUID	   1006
#define DEFAULT_FORWARD_MATERIAL_ASSET_GUID	   1007
#define DEFAULT_PHYSICAL_MATERIAL_ASSET_GUID   1008
#define DEFAULT_LINEAR_SAMPLER_ASSET_GUID	   1009
#define DEFAULT_NEAREST_SAMPLER_ASSET_GUID	   1010
#define DEFAULT_ANISOTROPIC_SAMPLER_ASSET_GUID 1011

	class ostream_t;

	enum class editor_asset_source_type_e : u8
	{
		none,
		file,
		embedded,
		data,
	};

	enum class editor_asset_status_e : u8
	{
		ok,
		missing_dependency,
		missing_embedded_data,
		missing_file_source,
	};

	struct editor_asset_t
	{
		static constexpr u32 VERSION = 0;

		nlohmann::json			   embedded_source = nlohmann::json();
		nlohmann::json			   cook_options	   = nlohmann::json::object();
		string_t				   source_relative = {};
		span_t<u8>				   _transient_data = {};
		u32						   version		   = 0;
		sid_t					   guid			   = NULL_SID;
		editor_asset_type_e		   asset_type	   = editor_asset_type_e::invalid;
		editor_asset_source_type_e source_type	   = editor_asset_source_type_e::file;
		editor_asset_status_e	   status		   = editor_asset_status_e::ok;
		u8						   sub_type		   = 0;
	};

	using editor_asset_create_default_fn	  = bool (*)(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
	using editor_asset_cook_fn				  = bool (*)(const editor_asset_t& asset, ostream_t& stream);
	using editor_asset_destroy_cook_config_fn = void (*)(void* object);

	struct editor_asset_cook_config_desc_t
	{
		void*								object	= nullptr;
		const char*							title	= nullptr;
		editor_asset_destroy_cook_config_fn destroy = nullptr;
		sid_t								type_id = 0;
	};

	using editor_asset_create_cook_config_fn = editor_asset_cook_config_desc_t (*)();

	struct editor_asset_descriptor_t
	{
		editor_asset_create_default_fn	   create_default	  = nullptr;
		editor_asset_create_cook_config_fn create_cook_config = nullptr;
		editor_asset_cook_fn			   cook				  = nullptr;
		vector_t<string_t>				   extensions		  = {};
		string_t						   display_name		  = {};
		vec4f_t							   color			  = {};
		editor_asset_type_e				   asset_type		  = editor_asset_type_e::invalid;
	};

	class editor_asset_util_t final
	{
	public:
		editor_asset_util_t()									   = delete;
		~editor_asset_util_t()									   = delete;
		editor_asset_util_t(const editor_asset_util_t&)			   = delete;
		editor_asset_util_t& operator=(const editor_asset_util_t&) = delete;

		static bool		read_asset(const char* path, editor_asset_t& out_asset);
		static bool		write_asset(const char* path, const editor_asset_t& asset);
		static string_t normalize_directory(const char* directory);
		static string_t make_asset_path(const char* directory, const char* asset_name);
		static string_t make_unique_source_path(const char* directory, const char* file_name, const char* extension);
		static string_t get_cache_path_for_asset(const editor_asset_t& asset);
		static string_t get_source_full_path(const char* assets_path, const editor_asset_t& asset);
		static string_t get_source_relative(const char* assets_path, const char* source_full_path);
		static bool		is_source_inside_assets(const char* assets_path, const char* source_full_path);
		static void		fetch_dependencies(const editor_asset_t& asset, frame_vector_t<sid_t>& out_dependencies);
		static sid_t	try_read_existing_guid(const char* path);
	};

}
