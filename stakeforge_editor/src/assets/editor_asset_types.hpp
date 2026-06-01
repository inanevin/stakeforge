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
	enum class editor_material_type_e : u8
	{
		gbuffer,
		forward,
	};

	struct editor_asset_loader_audio_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static void register_type();
	};

	struct editor_asset_loader_font_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static void register_type();
	};

	struct editor_asset_loader_mesh_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static void register_type();
	};

	struct editor_asset_loader_skeleton_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static void register_type();
	};

	struct editor_asset_loader_animation_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static void register_type();
	};

	struct editor_asset_loader_material_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static bool cook(const editor_asset_t& asset, ostream_t& stream);
		static void register_type();
	};

	struct editor_asset_loader_shader_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static bool cook(const editor_asset_t& asset, ostream_t& stream);
		static void register_type();
	};

	struct editor_asset_loader_texture_t
	{
		static bool							   create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static editor_asset_cook_config_desc_t create_cook_config();
		static bool							   cook(const editor_asset_t& asset, ostream_t& stream);
		static void							   register_type();
	};

	struct editor_asset_loader_texture_sampler_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static bool cook(const editor_asset_t& asset, ostream_t& stream);
		static void register_type();
	};

	struct editor_asset_loader_physical_material_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static bool cook(const editor_asset_t& asset, ostream_t& stream);
		static void register_type();
	};

	struct editor_asset_loader_prefab_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static void register_type();
	};

	struct editor_asset_loader_animation_state_machine_t
	{
		static bool create_default(editor_asset_t& asset, const char* directory, const char* file_name, void* cook_config);
		static void register_type();
	};
}
