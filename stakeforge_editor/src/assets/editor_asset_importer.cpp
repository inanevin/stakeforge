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

#include "assets/editor_asset_importer.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_glb_importer.hpp"
#include "editor_directories.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/skybox_hdr_cook.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		editor_asset_import_type_e get_import_type_from_extension(string_t extension)
		{
			string_util::to_lower(extension);
			if (extension == "mp3")
				return editor_asset_import_type_e::audio;
			if (extension == "png" || extension == "jpg" || extension == "jpeg")
				return editor_asset_import_type_e::texture;
			if (extension == "glb")
				return editor_asset_import_type_e::model;
			if (extension == "hdr")
				return editor_asset_import_type_e::hdr_skybox;
			if (extension == "ttf")
				return editor_asset_import_type_e::font;
			return editor_asset_import_type_e::invalid;
		}

		const editor_asset_import_options_t* find_import_options(span_t<const editor_asset_import_options_t> options, editor_asset_import_type_e import_type)
		{
			for (size_t i = 0; i < options.size; ++i)
			{
				const editor_asset_import_options_t& option = options.data[i];
				if (option.type == import_type)
					return &option;
			}
			return nullptr;
		}
	}

	void editor_asset_import_context_t::report_status(const char* text) const
	{
		if (set_status != nullptr)
			set_status(user_data, text);
	}

	bool editor_asset_importer_t::make_import_options(editor_asset_import_options_t& out_options, const char* asset_name)
	{
		SFG_ASSERT(asset_name != nullptr);
		SFG_ASSERT(asset_name[0] != '\0');

		const editor_asset_import_type_e import_type = get_import_type_from_extension(file_system_t::get_file_extension(asset_name));
		out_options									 = {};
		out_options.type							 = import_type;
		switch (import_type)
		{
		case editor_asset_import_type_e::texture:
			out_options.texture_cook_config = {};
			return true;
		case editor_asset_import_type_e::audio:
			out_options.audio_cook_config = {};
			return true;
		case editor_asset_import_type_e::model:
			out_options.glb_cook_config = {};
			return true;
		case editor_asset_import_type_e::hdr_skybox:
			out_options.skybox_cook_config = {};
			return true;
		case editor_asset_import_type_e::font:
			return true;
		default:
			return false;
		}
	}

	bool editor_asset_importer_t::make_asset(editor_asset_node_handle_t directory_node, const char* asset_name, editor_asset_t& asset, editor_asset_type_e asset_type, editor_asset_source_type_e source_type, const char* source_full_path)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!directory_node.is_null());
		SFG_ASSERT(tree.is_valid(directory_node));
		const editor_asset_node_t& parent_node = tree.value(directory_node);
		SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!parent_node.full_path.empty());
		SFG_ASSERT(asset_type != editor_asset_type_e::invalid);
		SFG_ASSERT(asset_type != editor_asset_type_e::count);

		if (!editor_directories_t::is_valid_asset_name(asset_name))
			return false;

		const string_t asset_path	 = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), asset_name);
		const sid_t	   existing_guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());

		asset.version	  = editor_asset_t::VERSION;
		asset.guid		  = existing_guid != NULL_SID ? existing_guid : editor_asset_util_t::generate_unique_asset_guid();
		asset.asset_type  = asset_type;
		asset.source_type = source_type;

		if (source_full_path != nullptr && source_full_path[0] != '\0' && !editor_asset_util_t::set_source_relative_or_copy(asset, parent_node.full_path.c_str(), asset_name, source_full_path))
		{
			SFG_ERR("failed to set source for imported asset {0}", asset_name);
			return false;
		}

		if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to write imported asset {0}", asset_path.c_str());
			return false;
		}

		return true;
	}

	bool editor_asset_importer_t::import_asset(editor_asset_node_handle_t directory_node, const char* source_full_path, span_t<const editor_asset_import_options_t> options, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!directory_node.is_null());
		SFG_ASSERT(tree.is_valid(directory_node));

		const editor_asset_node_t& parent_node = tree.value(directory_node);
		SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!parent_node.full_path.empty());
		SFG_ASSERT(source_full_path != nullptr);
		SFG_ASSERT(source_full_path[0] != '\0');

		const string_t source_path = file_system_t::get_absolute_path(source_full_path);
		if (!file_system_t::exists(source_path.c_str()))
		{
			SFG_ERR("import source does not exist: {0}", source_path.c_str());
			return false;
		}

		const string_t					 extension	 = file_system_t::get_file_extension(source_path);
		const editor_asset_import_type_e import_type = get_import_type_from_extension(extension);
		if (import_type == editor_asset_import_type_e::invalid)
		{
			SFG_ERR("unsupported import extension: {0}", extension.c_str());
			return false;
		}

		const editor_asset_import_options_t* import_options = find_import_options(options, import_type);
		if (import_options == nullptr)
		{
			SFG_ERR("missing import options for {0}", source_path.c_str());
			return false;
		}

		if (import_type == editor_asset_import_type_e::model)
			return editor_glb_importer_t::import_glb(directory_node, source_path.c_str(), import_options->glb_cook_config, context, out_assets);

		const string_t asset_name = file_system_t::get_filename_from_path(source_path);
		if (!editor_directories_t::is_valid_asset_name(asset_name.c_str()))
		{
			SFG_ERR("invalid imported asset name: {0}", asset_name.c_str());
			return false;
		}

		editor_asset_t asset = {};
		switch (import_type)
		{
		case editor_asset_import_type_e::texture: {
			string_t status = "Importing texture: ";
			status += asset_name;
			context.report_status(status.c_str());
			const texture_cook_config_t& texture_config = import_options->texture_cook_config;
			nlohmann::json				 cook_options	= nlohmann::json::object();
			if (!reflection_registry_t::get().type_to_json(type_id_t<texture_cook_config_t>::value, const_cast<texture_cook_config_t*>(&texture_config), nullptr, cook_options))
			{
				SFG_ERR("failed to serialize texture import options for {0}", source_path.c_str());
				return false;
			}
			editor_asset_util_t::set_cook_options_json(asset, cook_options);
			if (!make_asset(directory_node, asset_name.c_str(), asset, editor_asset_type_e::texture, editor_asset_source_type_e::file, source_path.c_str()))
			{
				SFG_ERR("failed to create imported texture asset {0}", asset_name.c_str());
				return false;
			}
			if (!editor_asset_cooker_t::cook_texture(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported texture asset {0}", asset.guid);
				return false;
			}
			break;
		}
		case editor_asset_import_type_e::font: {
			string_t status = "Importing font: ";
			status += asset_name;
			context.report_status(status.c_str());
			if (!make_asset(directory_node, asset_name.c_str(), asset, editor_asset_type_e::font, editor_asset_source_type_e::file, source_path.c_str()))
			{
				SFG_ERR("failed to create imported font asset {0}", asset_name.c_str());
				return false;
			}
			if (!editor_asset_cooker_t::cook_font(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported font asset {0}", asset.guid);
				return false;
			}
			break;
		}
		case editor_asset_import_type_e::audio: {
			string_t status = "Importing audio: ";
			status += asset_name;
			context.report_status(status.c_str());
			const audio_cook_config_t& audio_config = import_options->audio_cook_config;
			nlohmann::json			   cook_options = nlohmann::json::object();
			if (!reflection_registry_t::get().type_to_json(type_id_t<audio_cook_config_t>::value, const_cast<audio_cook_config_t*>(&audio_config), nullptr, cook_options))
			{
				SFG_ERR("failed to serialize audio import options for {0}", source_path.c_str());
				return false;
			}
			editor_asset_util_t::set_cook_options_json(asset, cook_options);
			if (!make_asset(directory_node, asset_name.c_str(), asset, editor_asset_type_e::audio, editor_asset_source_type_e::file, source_path.c_str()))
			{
				SFG_ERR("failed to create imported audio asset {0}", asset_name.c_str());
				return false;
			}
			if (!editor_asset_cooker_t::cook_audio(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported audio asset {0}", asset.guid);
				return false;
			}
			break;
		}
		case editor_asset_import_type_e::hdr_skybox: {
			string_t status = "Importing HDR skybox: ";
			status += asset_name;
			context.report_status(status.c_str());
			const skybox_hdr_cook_config_t& skybox_config = import_options->skybox_cook_config;
			nlohmann::json					cook_options  = nlohmann::json::object();
			if (!reflection_registry_t::get().type_to_json(type_id_t<skybox_hdr_cook_config_t>::value, const_cast<skybox_hdr_cook_config_t*>(&skybox_config), nullptr, cook_options))
			{
				SFG_ERR("failed to serialize HDR skybox import options for {0}", source_path.c_str());
				return false;
			}
			editor_asset_util_t::set_cook_options_json(asset, cook_options);
			if (!make_asset(directory_node, asset_name.c_str(), asset, editor_asset_type_e::hdr_skybox, editor_asset_source_type_e::file, source_path.c_str()))
			{
				SFG_ERR("failed to create imported HDR skybox asset {0}", asset_name.c_str());
				return false;
			}
			if (!editor_asset_cooker_t::cook_hdr_skybox(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported HDR skybox asset {0}", asset.guid);
				return false;
			}
			break;
		}
		default:
			SFG_ERR("unsupported import type: {0}", static_cast<u8>(import_type));
			return false;
		}

		out_assets.push_back(asset);
		return true;
	}
}
