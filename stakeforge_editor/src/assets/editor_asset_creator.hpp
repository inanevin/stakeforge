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

#include "assets/editor_asset.hpp"

namespace sfg
{
	enum class shader_type_e : u8;

	enum class editor_material_type_e : u8
	{
		gbuffer,
		forward,
	};

	enum class editor_texture_sampler_type_e : u8
	{
		linear,
		nearest,
		anisotropic,
	};

	struct editor_asset_create_desc_t
	{
		editor_asset_node_handle_t parent_node	   = {};
		const char*				   name			   = nullptr;
		const char*				   source_name	   = nullptr;
		sid_t					   guid			   = NULL_SID;
		editor_asset_type_e		   asset_type	   = editor_asset_type_e::invalid;
		u8						   sub_type		   = 0;
		bool					   allow_overwrite = false;
	};

	class editor_asset_creator_t final
	{
	public:
		editor_asset_creator_t()										 = delete;
		~editor_asset_creator_t()										 = delete;
		editor_asset_creator_t(const editor_asset_creator_t&)			 = delete;
		editor_asset_creator_t& operator=(const editor_asset_creator_t&) = delete;

		static bool		   create_asset(const editor_asset_create_desc_t& desc, editor_asset_t* out_asset = nullptr);
		static const char* get_material_scaffold_relative(editor_material_type_e material_type);
		static const char* get_physical_material_scaffold_relative();
		static const char* get_shader_scaffold_relative(shader_type_e shader_type);
		static const char* get_texture_sampler_scaffold_relative(editor_texture_sampler_type_e sampler_type = editor_texture_sampler_type_e::linear);
		static string_t	   get_texture_scaffold_relative(const char* texture_name);
		static bool		   scaffold_material_embedded_source(editor_material_type_e material_type, nlohmann::json& out_embedded_source);
		static bool		   scaffold_physical_material_embedded_source(nlohmann::json& out_embedded_source);
		static bool		   scaffold_texture_sampler_embedded_source(editor_texture_sampler_type_e sampler_type, nlohmann::json& out_embedded_source);
		static bool		   scaffold_texture_sampler_embedded_source(nlohmann::json& out_embedded_source);
		static bool		   scaffold_shader_source(editor_asset_t& asset, const char* directory, const char* file_name);
		static bool		   scaffold_texture_source(editor_asset_t& asset, const char* directory, const char* file_name);
		static bool		   scaffold_hdr_skybox_source(editor_asset_t& asset, const char* directory, const char* file_name);
	};
}
