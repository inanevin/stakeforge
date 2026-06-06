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

#include "assets/editor_asset_manager.hpp"
#include "assets/editor_glb_importer.hpp"
#include "editor_directories.hpp"

#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/audio_cook_reflection.hpp>
#include <sfg/runtime/resources/texture_cook_reflection.hpp>

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
			return editor_asset_import_type_e::invalid;
		}

		const editor_asset_import_options_t* find_import_options(const frame_vector_t<editor_asset_import_options_t>& options, editor_asset_import_type_e import_type)
		{
			for (const editor_asset_import_options_t& option : options)
			{
				if (option.type == import_type)
					return &option;
			}
			return nullptr;
		}
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
			return false;

		return editor_asset_util_t::write_asset(asset_path.c_str(), asset);
	}

	bool editor_asset_importer_t::import_asset(editor_asset_node_handle_t directory_node, const char* source_full_path, const frame_vector_t<editor_asset_import_options_t>& options, frame_vector_t<editor_asset_t>& out_assets)
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
			return false;

		const string_t					 extension	 = file_system_t::get_file_extension(source_path);
		const editor_asset_import_type_e import_type = get_import_type_from_extension(extension);
		if (import_type == editor_asset_import_type_e::invalid)
			return false;

		const editor_asset_import_options_t* import_options = find_import_options(options, import_type);
		if (import_options == nullptr)
			return false;

		if (import_type == editor_asset_import_type_e::model)
			return editor_glb_importer_t::import_glb(directory_node, source_path.c_str(), import_options->glb_cook_config, out_assets);

		const string_t asset_name = file_system_t::get_filename_from_path(source_path);
		if (!editor_directories_t::is_valid_asset_name(asset_name.c_str()))
			return false;

		editor_asset_t asset = {};
		switch (import_type)
		{
		case editor_asset_import_type_e::texture: {
			const texture_cook_config_t& texture_config = import_options->texture_cook_config;
			if (!reflection_registry_t::get().serialize_to_json(texture_cook_config_reflection_t::TYPE_ID, &texture_config, asset.cook_options))
				return false;
			if (!make_asset(directory_node, asset_name.c_str(), asset, editor_asset_type_e::texture, editor_asset_source_type_e::file, source_path.c_str()))
				return false;
			break;
		}
		case editor_asset_import_type_e::audio: {
			const audio_cook_config_t& audio_config = import_options->audio_cook_config;
			if (!reflection_registry_t::get().serialize_to_json(audio_cook_config_reflection_t::TYPE_ID, &audio_config, asset.cook_options))
				return false;
			if (!make_asset(directory_node, asset_name.c_str(), asset, editor_asset_type_e::audio, editor_asset_source_type_e::file, source_path.c_str()))
				return false;
			break;
		}
		default:
			return false;
		}

		out_assets.push_back(asset);
		return true;
	}
}
