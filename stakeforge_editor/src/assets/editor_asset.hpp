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
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	enum class editor_asset_source_type_e : u8
	{
		none,
		file,
		embedded,
	};

	struct editor_asset_t
	{
		static constexpr u32 VERSION = 0;

		nlohmann::json			   source_relative_path = "";
		nlohmann::json			   embedded_source		= nlohmann::json::object();
		nlohmann::json			   cook_options			= nlohmann::json::object();
		u32						   version				= 0;
		sid_t					   guid					= 0;
		editor_asset_type_e		   asset_type			= editor_asset_type_e::invalid;
		editor_asset_source_type_e source_type			= editor_asset_source_type_e::file;
	};

	using editor_asset_create_default_fn = bool (*)(editor_asset_t& asset, const char* directory, const char* file_name);
	using editor_asset_cook_fn			 = bool (*)(const editor_asset_t& asset, const char* cache_dir);

	struct editor_asset_descriptor_t
	{
		editor_asset_create_default_fn create_default = nullptr;
		editor_asset_cook_fn		   cook			  = nullptr;
		editor_asset_type_e			   asset_type	  = editor_asset_type_e::invalid;
	};

	struct editor_asset_loader_audio_t
	{
		static void register_type();
	};

	struct editor_asset_loader_font_t
	{
		static void register_type();
	};

	struct editor_asset_loader_mesh_t
	{
		static void register_type();
	};

	struct editor_asset_loader_skeleton_t
	{
		static void register_type();
	};

	struct editor_asset_loader_animation_t
	{
		static void register_type();
	};

	struct editor_asset_loader_particle_properties_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name);
		static void register_type();
	};

	struct editor_asset_loader_material_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name);
		static void register_type();
	};

	struct editor_asset_loader_shader_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name);
		static void register_type();
	};

	struct editor_asset_loader_texture_t
	{
		static void register_type();
	};

	struct editor_asset_loader_texture_sampler_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name);
		static bool cook(const editor_asset_t& asset, const char* cache_dir);
		static void register_type();
	};

	struct editor_asset_loader_physical_material_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name);
		static void register_type();
	};

	struct editor_asset_loader_prefab_t
	{
		static void register_type();
	};

	struct editor_asset_loader_animation_state_machine_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name);
		static void register_type();
	};

	void to_json(nlohmann::json& j, const editor_asset_source_type_e& t);
	void from_json(const nlohmann::json& j, editor_asset_source_type_e& t);
	void to_json(nlohmann::json& j, const editor_asset_t& asset);
	void from_json(const nlohmann::json& j, editor_asset_t& asset);
}
