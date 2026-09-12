/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include "assets/editor_asset_type.hpp"
#include "assets/editor_glb_importer.hpp"

#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/cubemap_cook.hpp>
#include <sfg/runtime/resources/sprite_cook.hpp>
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
		sprite,
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
		sprite_cook_config_t	   sprite_cook_config  = {};
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
