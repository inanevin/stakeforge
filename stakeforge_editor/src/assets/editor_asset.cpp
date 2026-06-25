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

#include "assets/editor_asset.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>

#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset.hpp"
#include "editor_project.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/char_util.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/serialization/serialization.hpp>
#include <algorithm>

namespace sfg
{
	namespace
	{
		string_t normalize_directory_path(const char* path)
		{
			const string_t result = file_system_t::get_absolute_path(path);
			return result;
		}

		bool path_in_directory(const string_t& path, const string_t& directory)
		{
			string_t normalized_path	  = path;
			string_t normalized_directory = directory;
			string_util::to_lower(normalized_path);
			string_util::to_lower(normalized_directory);
			return normalized_path.rfind(normalized_directory, 0) == 0;
		}

		bool path_equals(const string_t& lhs, const string_t& rhs)
		{
			string_t normalized_lhs = lhs;
			string_t normalized_rhs = rhs;
			string_util::to_lower(normalized_lhs);
			string_util::to_lower(normalized_rhs);
			return normalized_lhs == normalized_rhs;
		}

		string_t make_source_full_path(const string_t& assets_path, const string_t& source_relative)
		{
			string_t result = assets_path;
			file_system_t::fix_path_end_slash(result);
			result += source_relative;
			file_system_t::fix_path(result);
			return file_system_t::get_absolute_path(result.c_str());
		}

		struct source_file_move_t
		{
			string_t old_path;
			string_t new_path;
		};

		bool is_same_or_descendant_folder(const editor_asset_tree_t& tree, editor_asset_node_handle_t node, editor_asset_node_handle_t ancestor)
		{
			editor_asset_node_handle_t current = node;
			while (!current.is_null())
			{
				if (current == ancestor)
					return true;
				current = tree.parent(current);
			}
			return false;
		}

		bool duplicate_cooked_asset(const editor_asset_t& source_asset, const editor_asset_t& duplicated_asset)
		{
			const string_t source_cache_path = editor_asset_util_t::get_cache_path_for_asset(source_asset);
			if (!file_system_t::exists(source_cache_path.c_str()))
				return true;

			const string_t duplicated_cache_path = editor_asset_util_t::get_cache_path_for_asset(duplicated_asset);
			if (!file_system_t::copy_file(source_cache_path.c_str(), duplicated_cache_path.c_str()))
			{
				SFG_ERR("failed to duplicate cooked asset {0} to {1}", source_asset.guid, duplicated_asset.guid);
				return false;
			}

			return true;
		}

		void remap_source_relative(editor_asset_t& asset, const string_t& assets_path, const string_t& source_folder_path, const string_t& duplicated_folder_path)
		{
			if (asset.source_relative.empty())
				return;

			string_t source_full_path = assets_path;
			source_full_path += asset.source_relative;
			file_system_t::fix_path(source_full_path);
			source_full_path = file_system_t::get_absolute_path(source_full_path.c_str());
			if (!path_in_directory(source_full_path, source_folder_path))
				return;

			string_t duplicated_source_path = duplicated_folder_path;
			duplicated_source_path += source_full_path.substr(source_folder_path.size());
			if (file_system_t::exists(duplicated_source_path.c_str()))
				asset.source_relative = file_system_t::get_relative(assets_path.c_str(), duplicated_source_path.c_str());
		}

		bool remap_asset_sources_in_folder(const string_t& folder_path, const string_t& assets_path, const string_t& source_folder_path, const string_t& target_folder_path)
		{
			vector_t<file_system_entry_t> entries;
			file_system_t::get_entries_recursive(folder_path.c_str(), entries);
			for (const file_system_entry_t& entry : entries)
			{
				if (entry.type != file_system_entry_type_e::file || file_system_t::get_file_extension(entry.path) != "sfg_asset")
					continue;

				editor_asset_t asset = {};
				if (!editor_asset_util_t::read_asset(entry.path.c_str(), asset))
					return false;

				const string_t old_source_relative = asset.source_relative;
				remap_source_relative(asset, assets_path, source_folder_path, target_folder_path);
				if (asset.source_relative != old_source_relative && !editor_asset_util_t::write_asset(entry.path.c_str(), asset))
					return false;
			}

			return true;
		}

		bool remap_asset_source_file_references(const string_t& assets_path, span_t<const source_file_move_t> moved_sources)
		{
			vector_t<file_system_entry_t> entries;
			file_system_t::get_entries_recursive(assets_path.c_str(), entries);
			for (const file_system_entry_t& entry : entries)
			{
				if (entry.type != file_system_entry_type_e::file || file_system_t::get_file_extension(entry.path) != "sfg_asset")
					continue;

				editor_asset_t asset = {};
				if (!editor_asset_util_t::read_asset(entry.path.c_str(), asset))
					return false;

				if (asset.source_relative.empty())
					continue;

				const string_t			  source_full_path = make_source_full_path(assets_path, asset.source_relative);
				const source_file_move_t* move			   = nullptr;
				for (size_t i = 0; i < moved_sources.size; ++i)
				{
					if (path_equals(source_full_path, moved_sources.data[i].old_path))
					{
						move = moved_sources.data + i;
						break;
					}
				}
				if (move == nullptr)
					continue;

				asset.source_relative = file_system_t::get_relative(assets_path.c_str(), move->new_path.c_str());
				if (!editor_asset_util_t::write_asset(entry.path.c_str(), asset))
					return false;
			}

			return true;
		}
	}

	bool editor_asset_util_t::read_asset(const char* path, editor_asset_t& out_asset)
	{
		const string_t		 json_text = file_system_t::read_file_as_string(path);
		const nlohmann::json doc	   = nlohmann::json::parse(json_text, nullptr, false);
		if (doc.is_discarded())
		{
			SFG_ERR("failed to parse asset {0}", path);
			return false;
		}

		doc.get_to(out_asset);
		return true;
	}

	bool editor_asset_util_t::write_asset(const char* path, const editor_asset_t& asset)
	{
		const nlohmann::json json_data = asset;
		const string_t		 data	   = json_data.dump(4);
		return serializer_t::write_to_file(string_view_t(data.data(), data.size()), path);
	}

	string_t editor_asset_util_t::normalize_directory(const char* directory)
	{
		string_t result = directory != nullptr ? directory : "";
		if (!result.empty() && result.back() != '/')
			result += '/';
		return result;
	}

	string_t editor_asset_util_t::make_asset_path(const char* directory, const char* asset_name)
	{
		string_t result = normalize_directory(directory);
		result += asset_name != nullptr ? asset_name : "";
		result += ".sfg_asset";
		return result;
	}

	string_t editor_asset_util_t::make_blob_path(const char* directory, const char* asset_name)
	{
		string_t result = normalize_directory(directory);
		result += asset_name != nullptr ? asset_name : "";
		result += ".sfg_bin";
		return result;
	}

	string_t editor_asset_util_t::make_source_path(const char* directory, const char* file_name, const char* extension)
	{
		string_t result = normalize_directory(directory);
		result += file_name != nullptr ? file_name : "";

		string_t ext = extension != nullptr ? extension : "";
		if (!ext.empty() && ext[0] != '.')
			ext.insert(ext.begin(), '.');

		result += ext;
		return result;
	}

	string_t editor_asset_util_t::make_unique_source_path(const char* directory, const char* file_name, const char* extension)
	{
		string_t	   result		   = make_source_path(directory, file_name, extension);
		size_t		   insert_position = result.size();
		const string_t ext			   = extension != nullptr ? extension : "";
		if (!ext.empty())
			insert_position -= ext[0] == '.' ? ext.size() : ext.size() + 1;
		while (file_system_t::exists(result.c_str()))
		{
			result.insert(insert_position, " (Copy)");
			insert_position += 7;
		}

		return result;
	}

	string_t editor_asset_util_t::get_cache_path_for_asset(const editor_asset_t& asset)
	{
		string_t result = editor_project_t::get()._runtime.cache_path;

		char  guid_text[32] = {};
		char* guid_text_cur = guid_text;
		if (!char_util::append_u64(guid_text_cur, guid_text + sizeof(guid_text), asset.guid))
			SFG_ASSERT(false);

		result += guid_text;
		result += ".sfg_bin";
		return result;
	}

	string_t editor_asset_util_t::get_source_full_path(const char* assets_path, const editor_asset_t& asset)
	{
		SFG_ASSERT(!asset.source_relative.empty());
		string_t result = file_system_t::get_absolute_path(assets_path);
		result += asset.source_relative;
		SFG_ASSERT(file_system_t::exists(result.c_str()));
		return result;
	}

	string_t editor_asset_util_t::get_source_relative(const char* assets_path, const char* source_full_path)
	{
		if (source_full_path == nullptr || source_full_path[0] == '\0')
			return {};

		string_t normalized_assets_path		  = file_system_t::get_absolute_path(assets_path);
		string_t normalized_assets_path_lower = normalized_assets_path;
		string_util::to_lower(normalized_assets_path_lower);

		const string_t normalized_source_path		= file_system_t::get_absolute_path(source_full_path);
		string_t	   normalized_source_path_lower = normalized_source_path;
		string_util::to_lower(normalized_source_path_lower);
		if (normalized_source_path_lower.rfind(normalized_assets_path_lower, 0) != 0)
			return {};

		return file_system_t::get_relative(normalized_assets_path.c_str(), normalized_source_path.c_str());
	}

	bool editor_asset_util_t::set_source_relative_or_copy(editor_asset_t& asset, const char* asset_directory, const char* asset_name, const char* source_full_path)
	{
		SFG_ASSERT(asset_directory != nullptr);
		SFG_ASSERT(asset_directory[0] != '\0');
		SFG_ASSERT(asset_name != nullptr);
		SFG_ASSERT(asset_name[0] != '\0');
		SFG_ASSERT(source_full_path != nullptr);
		SFG_ASSERT(source_full_path[0] != '\0');

		const string_t source_path = file_system_t::get_absolute_path(source_full_path);
		SFG_ASSERT(file_system_t::exists(source_path.c_str()));

		const string_t assets_path = editor_project_t::get()._runtime.assets_path;
		asset.source_relative	   = get_source_relative(assets_path.c_str(), source_path.c_str());
		if (!asset.source_relative.empty())
			return true;

		const string_t source_extension	  = file_system_t::get_file_extension(source_path);
		const string_t target_source_path = make_source_path(asset_directory, asset_name, source_extension.c_str());
		if (!file_system_t::copy_file(source_path.c_str(), target_source_path.c_str()))
			return false;

		SFG_ASSERT(file_system_t::exists(target_source_path.c_str()));
		asset.source_relative = get_source_relative(assets_path.c_str(), target_source_path.c_str());
		SFG_ASSERT(!asset.source_relative.empty());
		return true;
	}

	bool editor_asset_util_t::is_source_inside_assets(const char* assets_path, const char* source_full_path)
	{
		return !get_source_relative(assets_path, source_full_path).empty();
	}

	void editor_asset_util_t::fetch_dependencies(const editor_asset_t& asset, frame_vector_t<sid_t>& out_dependencies)
	{
		const auto push_dependency = [&](sid_t dependency) {
			if (dependency != NULL_SID)
				out_dependencies.push_back(dependency);
		};

		switch (asset.asset_type)
		{
		case editor_asset_type_e::mesh:
		case editor_asset_type_e::material: {
			if (!asset.embedded_source.is_object())
				break;

			push_dependency(asset.embedded_source.value<sid_t>("shader", NULL_SID));
			push_dependency(asset.embedded_source.value<sid_t>("sampler", NULL_SID));
			const vector_t<sid_t> textures = asset.embedded_source.value<vector_t<sid_t>>("textures", {});
			out_dependencies.reserve(out_dependencies.size() + textures.size());
			for (const sid_t texture : textures)
				push_dependency(texture);
			break;
		}
		default:
			break;
		}
	}

	sid_t editor_asset_util_t::generate_unique_asset_guid(span_t<const sid_t> pending_guids)
	{
		sid_t guid			= NULL_SID;
		bool  found_pending = false;
		do
		{
			guid		  = hashing_t::generate_guid64();
			found_pending = false;
			for (size_t i = 0; i < pending_guids.size; ++i)
			{
				if (pending_guids.data[i] == guid)
				{
					found_pending = true;
					break;
				}
			}
		} while (guid == NULL_SID || editor_asset_manager_t::get().find_asset(guid) != nullptr || found_pending);
		return guid;
	}

	sid_t editor_asset_util_t::try_read_existing_guid(const char* path)
	{
		editor_asset_t asset = {};
		if (!file_system_t::exists(path))
			return NULL_SID;
		return read_asset(path, asset) ? asset.guid : NULL_SID;
	}

	editor_asset_type_e editor_asset_util_t::reflected_value_type_to_asset_type(reflected_value_type_e type)
	{
		switch (type)
		{
		case reflected_value_type_e::audio_handle:
			return editor_asset_type_e::audio;
		case reflected_value_type_e::font_handle:
			return editor_asset_type_e::font;
		case reflected_value_type_e::mesh_handle:
			return editor_asset_type_e::mesh;
		case reflected_value_type_e::skeleton_handle:
			return editor_asset_type_e::skeleton;
		case reflected_value_type_e::animation_handle:
			return editor_asset_type_e::animation;
		case reflected_value_type_e::material_handle:
			return editor_asset_type_e::material;
		case reflected_value_type_e::shader_handle:
			return editor_asset_type_e::shader;
		case reflected_value_type_e::texture_handle:
			return editor_asset_type_e::texture;
		case reflected_value_type_e::texture_sampler_handle:
			return editor_asset_type_e::texture_sampler;
		case reflected_value_type_e::physical_material_handle:
			return editor_asset_type_e::physical_material;
		case reflected_value_type_e::prefab_handle:
			return editor_asset_type_e::prefab;
		case reflected_value_type_e::animation_state_machine_handle:
			return editor_asset_type_e::animation_state_machine;
		case reflected_value_type_e::hdr_skybox_handle:
			return editor_asset_type_e::hdr_skybox;
		default:
			return editor_asset_type_e::invalid;
		}
	}

	const char* editor_asset_util_t::find_asset_display_name(sid_t guid)
	{
		if (guid == NULL_SID)
			return nullptr;

		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		for (auto it = tree.begin_handle(); it != tree.end_handle(); ++it)
		{
			const editor_asset_node_t& node = tree.value(*it);
			if (node.type == editor_asset_node_type_e::asset && node.asset_id == guid)
				return node.name.c_str();
		}
		return nullptr;
	}

	bool editor_asset_util_t::delete_folder(editor_asset_node_handle_t folder_node)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!folder_node.is_null());
		SFG_ASSERT(tree.is_valid(folder_node));

		const editor_asset_node_t& node = tree.value(folder_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!node.full_path.empty());
		return file_system_t::delete_directory(node.full_path.c_str());
	}

	bool editor_asset_util_t::duplicate_folder(editor_asset_node_handle_t folder_node, string_t* out_duplicated_path)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!folder_node.is_null());
		SFG_ASSERT(tree.is_valid(folder_node));

		const editor_asset_node_t& node = tree.value(folder_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!node.full_path.empty());

		const string_t duplicated_folder = file_system_t::duplicate(node.full_path.c_str());
		if (duplicated_folder.empty())
		{
			SFG_ERR("failed to duplicate folder {0}", node.full_path.c_str());
			return false;
		}

		const string_t source_folder_path	  = normalize_directory_path(node.full_path.c_str());
		const string_t duplicated_folder_path = normalize_directory_path(duplicated_folder.c_str());
		const string_t assets_path			  = normalize_directory_path(editor_project_t::get()._runtime.assets_path.c_str());

		vector_t<file_system_entry_t> entries;
		file_system_t::get_entries_recursive(duplicated_folder_path.c_str(), entries);

		vector_t<sid_t> duplicated_guids;
		duplicated_guids.reserve(entries.size());

		for (const file_system_entry_t& entry : entries)
		{
			if (entry.type != file_system_entry_type_e::file || file_system_t::get_file_extension(entry.path) != "sfg_asset")
				continue;

			editor_asset_t duplicated_asset = {};
			if (!read_asset(entry.path.c_str(), duplicated_asset))
				return false;

			editor_asset_t source_asset = duplicated_asset;
			duplicated_asset.guid		= generate_unique_asset_guid({.data = duplicated_guids.data(), .size = duplicated_guids.size()});
			duplicated_guids.push_back(duplicated_asset.guid);
			remap_source_relative(duplicated_asset, assets_path, source_folder_path, duplicated_folder_path);

			if (!write_asset(entry.path.c_str(), duplicated_asset))
				return false;

			if (!duplicate_cooked_asset(source_asset, duplicated_asset))
				return false;
		}

		if (out_duplicated_path != nullptr)
			*out_duplicated_path = duplicated_folder;
		return true;
	}

	bool editor_asset_util_t::rename_folder(editor_asset_node_handle_t folder_node, const char* new_path)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!folder_node.is_null());
		SFG_ASSERT(tree.is_valid(folder_node));
		SFG_ASSERT(new_path != nullptr);
		SFG_ASSERT(new_path[0] != '\0');

		const editor_asset_node_t& node = tree.value(folder_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!node.full_path.empty());

		const string_t old_folder_path = normalize_directory_path(node.full_path.c_str());
		string_t	   renamed_path	   = new_path;
		if (!file_system_t::change_directory_name(node.full_path.c_str(), renamed_path.c_str()))
		{
			SFG_ERR("failed to rename folder {0} to {1}", node.full_path.c_str(), renamed_path.c_str());
			return false;
		}

		const string_t renamed_folder_path = normalize_directory_path(renamed_path.c_str());
		const string_t assets_path		   = normalize_directory_path(editor_project_t::get()._runtime.assets_path.c_str());
		if (!remap_asset_sources_in_folder(renamed_folder_path, assets_path, old_folder_path, renamed_folder_path))
		{
			SFG_ERR("failed to remap asset sources in renamed folder {0}", renamed_folder_path.c_str());
			return false;
		}

		return true;
	}

	bool editor_asset_util_t::move_folder(editor_asset_node_handle_t folder_node, editor_asset_node_handle_t target_folder_node)
	{
		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  tree			= asset_manager.get_asset_tree();
		SFG_ASSERT(!folder_node.is_null());
		SFG_ASSERT(!target_folder_node.is_null());
		SFG_ASSERT(tree.is_valid(folder_node));
		SFG_ASSERT(tree.is_valid(target_folder_node));

		const editor_asset_node_t& node		   = tree.value(folder_node);
		const editor_asset_node_t& target_node = tree.value(target_folder_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(target_node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!node.full_path.empty());
		SFG_ASSERT(!target_node.full_path.empty());

		if (folder_node == asset_manager.get_root_node() || (node.flags & editor_asset_node_flag_promoted) != 0)
			return false;
		if (is_same_or_descendant_folder(tree, target_folder_node, folder_node))
			return false;

		const editor_asset_node_handle_t old_parent = tree.parent(folder_node);
		if (old_parent == target_folder_node)
			return true;

		const string_t old_folder_path = normalize_directory_path(node.full_path.c_str());
		string_t	   old_folder_dir  = old_folder_path;
		file_system_t::fix_path_end_slash(old_folder_dir);
		const string_t target_directory	   = normalize_directory_path(target_node.full_path.c_str());
		string_t	   new_folder_path	   = normalize_directory(target_directory.c_str()) + node.name;
		const string_t new_folder_abs_path = file_system_t::get_absolute_path(new_folder_path.c_str());
		if (file_system_t::exists(new_folder_abs_path.c_str()))
		{
			SFG_ERR("folder move target already exists {0}", new_folder_abs_path.c_str());
			return false;
		}

		string_t assets_path = normalize_directory_path(editor_project_t::get()._runtime.assets_path.c_str());
		file_system_t::fix_path_end_slash(assets_path);

		vector_t<file_system_entry_t> entries;
		file_system_t::get_entries_recursive(old_folder_dir.c_str(), entries);

		vector_t<source_file_move_t> moved_sources;
		moved_sources.reserve(entries.size());
		string_t new_folder_dir = new_folder_abs_path;
		file_system_t::fix_path_end_slash(new_folder_dir);
		for (const file_system_entry_t& entry : entries)
		{
			if (entry.type != file_system_entry_type_e::file || file_system_t::get_file_extension(entry.path) == "sfg_asset")
				continue;

			const string_t old_source_path = file_system_t::get_absolute_path(entry.path.c_str());
			string_t	   new_source_path = new_folder_dir;
			new_source_path += old_source_path.substr(old_folder_dir.size());
			moved_sources.push_back({.old_path = old_source_path, .new_path = new_source_path});
		}

		if (!file_system_t::change_directory_name(old_folder_path.c_str(), new_folder_abs_path.c_str()))
		{
			SFG_ERR("failed to move folder {0} to {1}", old_folder_path.c_str(), new_folder_abs_path.c_str());
			return false;
		}

		if (!remap_asset_source_file_references(assets_path, {.data = moved_sources.data(), .size = moved_sources.size()}))
		{
			SFG_ERR("failed to remap asset source references after moving folder {0}", new_folder_abs_path.c_str());
			return false;
		}

		return true;
	}

	bool editor_asset_util_t::delete_file(editor_asset_node_handle_t file_node)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!file_node.is_null());
		SFG_ASSERT(tree.is_valid(file_node));

		const editor_asset_node_t& node = tree.value(file_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::file);
		SFG_ASSERT(!node.full_path.empty());
		const bool deleted = !file_system_t::delete_file(node.full_path.c_str());
		if (!deleted)
			SFG_ERR("failed to delete file {0}", node.full_path.c_str());
		return deleted;
	}

	bool editor_asset_util_t::rename_file(editor_asset_node_handle_t file_node, const char* new_path)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!file_node.is_null());
		SFG_ASSERT(tree.is_valid(file_node));
		SFG_ASSERT(new_path != nullptr);
		SFG_ASSERT(new_path[0] != '\0');

		const editor_asset_node_t& node = tree.value(file_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::file);
		SFG_ASSERT(!node.full_path.empty());

		const string_t old_source_path = file_system_t::get_absolute_path(node.full_path.c_str());
		string_t	   renamed_path	   = new_path;
		if (!file_system_t::change_directory_name(node.full_path.c_str(), renamed_path.c_str()))
		{
			SFG_ERR("failed to rename file {0} to {1}", node.full_path.c_str(), renamed_path.c_str());
			return false;
		}

		const string_t new_source_path = file_system_t::get_absolute_path(renamed_path.c_str());
		string_t	   assets_path	   = normalize_directory_path(editor_project_t::get()._runtime.assets_path.c_str());
		file_system_t::fix_path_end_slash(assets_path);
		const source_file_move_t moved_source{.old_path = old_source_path, .new_path = new_source_path};
		if (!remap_asset_source_file_references(assets_path, {.data = &moved_source, .size = 1}))
		{
			SFG_ERR("failed to remap asset source references after renaming file {0}", new_source_path.c_str());
			return false;
		}

		return true;
	}

	bool editor_asset_util_t::delete_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!asset_node.is_null());
		SFG_ASSERT(tree.is_valid(asset_node));

		const editor_asset_node_t& node = tree.value(asset_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::asset);
		SFG_ASSERT(node.asset_id == asset.guid);
		SFG_ASSERT(!node.full_path.empty());
		const bool deleted = !file_system_t::delete_file(node.full_path.c_str());
		if (!deleted)
			SFG_ERR("failed to delete asset {0}", node.full_path.c_str());
		return deleted;
	}

	bool editor_asset_util_t::duplicate_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, string_t* out_duplicated_path)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!asset_node.is_null());
		SFG_ASSERT(tree.is_valid(asset_node));

		const editor_asset_node_t& node = tree.value(asset_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::asset);
		SFG_ASSERT(node.asset_id == asset.guid);
		SFG_ASSERT(!node.full_path.empty());

		const string_t duplicated_path = file_system_t::duplicate(node.full_path.c_str());
		if (duplicated_path.empty())
		{
			SFG_ERR("failed to duplicate asset {0}", node.full_path.c_str());
			return false;
		}

		editor_asset_t duplicated_asset = asset;
		duplicated_asset.guid			= generate_unique_asset_guid();

		if (!write_asset(duplicated_path.c_str(), duplicated_asset))
			return false;

		if (!duplicate_cooked_asset(asset, duplicated_asset))
			return false;
		if (out_duplicated_path != nullptr)
			*out_duplicated_path = duplicated_path;
		return true;
	}

	bool editor_asset_util_t::rename_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, const char* new_path)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!asset_node.is_null());
		SFG_ASSERT(tree.is_valid(asset_node));
		SFG_ASSERT(new_path != nullptr);
		SFG_ASSERT(new_path[0] != '\0');

		const editor_asset_node_t& node = tree.value(asset_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::asset);
		SFG_ASSERT(node.asset_id == asset.guid);
		SFG_ASSERT(!node.full_path.empty());

		const string_t renamed_path = new_path;
		if (!file_system_t::change_directory_name(node.full_path.c_str(), renamed_path.c_str()))
		{
			SFG_ERR("failed to rename asset {0} to {1}", node.full_path.c_str(), renamed_path.c_str());
			return false;
		}

		return true;
	}

	bool editor_asset_util_t::move_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, editor_asset_node_handle_t target_folder_node)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!asset_node.is_null());
		SFG_ASSERT(!target_folder_node.is_null());
		SFG_ASSERT(tree.is_valid(asset_node));
		SFG_ASSERT(tree.is_valid(target_folder_node));

		const editor_asset_node_t& node		   = tree.value(asset_node);
		const editor_asset_node_t& target_node = tree.value(target_folder_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::asset);
		SFG_ASSERT(target_node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(node.asset_id == asset.guid);
		SFG_ASSERT(!node.full_path.empty());
		SFG_ASSERT(!target_node.full_path.empty());

		const string_t old_asset_path		 = file_system_t::get_absolute_path(node.full_path.c_str());
		const string_t old_asset_directory	 = normalize_directory_path(file_system_t::get_directory_of_file(old_asset_path.c_str()).c_str());
		const string_t target_directory		 = normalize_directory_path(target_node.full_path.c_str());
		const string_t asset_file_name		 = file_system_t::get_filename_and_extension_from_path(old_asset_path);
		const string_t target_asset_path	 = normalize_directory(target_directory.c_str()) + asset_file_name;
		const string_t target_asset_abs_path = file_system_t::get_absolute_path(target_asset_path.c_str());
		if (path_equals(old_asset_directory, target_directory))
			return true;

		if (file_system_t::exists(target_asset_abs_path.c_str()))
		{
			SFG_ERR("asset move target already exists {0}", target_asset_abs_path.c_str());
			return false;
		}

		editor_asset_t moved_asset = asset;
		bool		   move_source = false;
		string_t	   source_full_path;
		string_t	   target_source_path;
		if (!asset.source_relative.empty())
		{
			string_t assets_path = normalize_directory_path(editor_project_t::get()._runtime.assets_path.c_str());
			file_system_t::fix_path_end_slash(assets_path);
			source_full_path = make_source_full_path(assets_path, asset.source_relative);
			if (file_system_t::exists(source_full_path.c_str()))
			{
				const string_t source_directory = normalize_directory_path(file_system_t::get_directory_of_file(source_full_path.c_str()).c_str());
				move_source						= path_equals(source_directory, old_asset_directory);
				if (move_source)
				{
					const string_t source_file_name = file_system_t::get_filename_and_extension_from_path(source_full_path);
					target_source_path				= normalize_directory(target_directory.c_str()) + source_file_name;
					if (file_system_t::exists(target_source_path.c_str()))
					{
						SFG_ERR("asset source move target already exists {0}", target_source_path.c_str());
						return false;
					}
					moved_asset.source_relative = file_system_t::get_relative(assets_path.c_str(), target_source_path.c_str());
				}
			}
		}

		if (move_source && !file_system_t::change_directory_name(source_full_path.c_str(), target_source_path.c_str()))
		{
			SFG_ERR("failed to move asset source {0} to {1}", source_full_path.c_str(), target_source_path.c_str());
			return false;
		}

		if (move_source && !write_asset(old_asset_path.c_str(), moved_asset))
		{
			SFG_ERR("failed to update moved asset source reference {0}", old_asset_path.c_str());
			return false;
		}

		if (!file_system_t::change_directory_name(old_asset_path.c_str(), target_asset_abs_path.c_str()))
		{
			SFG_ERR("failed to move asset {0} to {1}", old_asset_path.c_str(), target_asset_abs_path.c_str());
			return false;
		}

		return true;
	}

}

namespace sfg
{
	void from_json(const nlohmann::json& j, editor_asset_source_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "none")
			t = editor_asset_source_type_e::none;
		else if (s == "file_blob")
			t = editor_asset_source_type_e::file_blob;
		else if (s == "embedded")
			t = editor_asset_source_type_e::embedded;
		else
			t = editor_asset_source_type_e::file;
	}

	void from_json(const nlohmann::json& j, editor_asset_t& asset)
	{
		asset.version		  = j.value<u32>("version", editor_asset_t::VERSION);
		asset.guid			  = j.value<sid_t>("guid", NULL_SID);
		asset.asset_type	  = j.value<editor_asset_type_e>("asset_type", j.value<editor_asset_type_e>("resource_type", j.value<editor_asset_type_e>("type", editor_asset_type_e::invalid)));
		asset.sub_type		  = j.value<u8>("sub_type", 0);
		asset.embedded_source = j.value<nlohmann::json>("embedded_source", nlohmann::json());
		asset.cook_options	  = j.value<nlohmann::json>("cook_options", nlohmann::json::object());
		asset.source_relative = j.value<string_t>("source_relative", {});
		asset.source_type	  = j.value<editor_asset_source_type_e>("source_type", editor_asset_source_type_e::file);
		asset.status		  = editor_asset_status_e::ok;
	}

	void to_json(nlohmann::json& j, const editor_asset_source_type_e& t)
	{
		switch (t)
		{
		case editor_asset_source_type_e::none:
			j = "none";
			break;
		case editor_asset_source_type_e::file_blob:
			j = "file_blob";
			break;
		case editor_asset_source_type_e::embedded:
			j = "embedded";
			break;
		default:
			j = "file";
			break;
		}
	}

	void to_json(nlohmann::json& j, const editor_asset_t& asset)
	{
		j["version"]		 = asset.version;
		j["guid"]			 = asset.guid;
		j["asset_type"]		 = asset.asset_type;
		j["sub_type"]		 = asset.sub_type;
		j["embedded_source"] = asset.embedded_source;
		j["cook_options"]	 = asset.cook_options;
		j["source_relative"] = asset.source_relative;
		j["source_type"]	 = asset.source_type;
	}
}
