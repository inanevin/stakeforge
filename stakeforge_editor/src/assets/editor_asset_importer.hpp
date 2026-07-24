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
#include "assets/editor_glb_importer.hpp"

#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/cubemap_cook.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>

namespace sfg
{
	struct editor_asset_t;
	enum class editor_asset_source_type_e : u8;

	enum class editor_asset_import_type_e : u8
	{
		invalid,
		audio,
		font,
		texture,
		model,
		cubemap,
		orm_texture,
	};

	struct editor_texture_orm_import_sources_t
	{
		string_t occlusion = {};
		string_t roughness = {};
		string_t metallic  = {};
	};

	SFG_DEFINE_TYPE_ID(editor_texture_orm_import_sources_t);

	struct editor_texture_orm_import_sources_reflection_t
	{
		editor_texture_orm_import_sources_reflection_t();
	};

	inline editor_texture_orm_import_sources_reflection_t g_reflect_editor_texture_orm_import_sources;

	struct editor_asset_import_options_t
	{
		texture_cook_config_t	   texture_cook_config = {};
		audio_cook_config_t		   audio_cook_config   = {};
		cubemap_cook_config_t	   cubemap_cook_config = {};
		glb_cook_config_t		   glb_cook_config	   = {};
		editor_asset_import_type_e type				   = editor_asset_import_type_e::invalid;
	};

	struct editor_asset_import_context_t
	{
		void* user_data									 = nullptr;
		void (*set_status)(void* user_data, const char*) = nullptr;

		void report_status(const char* text) const;
	};

	class editor_asset_importer_t final
	{
	public:
		editor_asset_importer_t()										   = delete;
		~editor_asset_importer_t()										   = delete;
		editor_asset_importer_t(const editor_asset_importer_t&)			   = delete;
		editor_asset_importer_t& operator=(const editor_asset_importer_t&) = delete;

		static bool make_import_options(editor_asset_import_options_t& out_options, const char* asset_name);
		static bool import_asset(
			const char* target_directory, const char* source_full_path, span_t<const editor_asset_import_options_t> options, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets, vector_t<string_t>& out_asset_paths);
		static bool import_texture_orm(
			const char* target_directory, span_t<const string_t> source_paths, const texture_cook_config_t& texture_config, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets, vector_t<string_t>& out_asset_paths);
	};
}
