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

#include "assets/editor_asset_manager.hpp"

#include "editor_directories.hpp"
#include "editor_project.hpp"

#include <sfg/data/string_util.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <utility>

namespace sfg
{
	bool editor_asset_manager_t::init(const editor_project_t& project)
	{
		return rescan(project);
	}

	void editor_asset_manager_t::uninit()
	{
		_asset_tree.clear();
		_root_node = {};
		_generation++;
	}

	bool editor_asset_manager_t::rescan(const editor_project_t& project)
	{
		if (!editor_directories_t::ensure_project_assets_directory(project))
			return false;

		const string_t assets_dir = editor_directories_t::get_project_assets_directory(project);
		return build_asset_tree(assets_dir);
	}

	bool editor_asset_manager_t::build_asset_tree(const string_t& assets_dir)
	{
		vector_t<file_system_entry_t> entries;
		file_system_t::get_entries_recursive(assets_dir.c_str(), entries);

		_asset_tree.clear();
		_asset_tree.reserve(static_cast<u32>(entries.size() + 1));

		const string_t root_name = file_system_t::get_last_folder_from_path(assets_dir.c_str());
		_root_node				 = _asset_tree.emplace(editor_asset_node_t{.name = root_name, .type = editor_asset_node_type_e::folder});

		vector_t<string_t> parts;
		for (const file_system_entry_t& entry : entries)
		{
			const string_t relative = file_system_t::get_relative(assets_dir.c_str(), entry.path.c_str());
			parts.clear();
			string_util::split(parts, relative, "/");

			editor_asset_node_handle_t parent			 = _root_node;
			const size_t			   folder_part_count = entry.type == file_system_entry_type_e::directory ? parts.size() : parts.size() - 1;
			for (size_t i = 0; i < folder_part_count; ++i)
				parent = get_or_create_child_folder(parent, parts[i]);

			if (entry.type == file_system_entry_type_e::directory)
				continue;

			editor_asset_t asset = {};
			if (file_system_t::get_file_extension(entry.path) == "sfg_asset")
			{
				if (!read_asset(entry.path.c_str(), asset))
					return false;

				const string_t					 name		= file_system_t::remove_extensions_from_path(parts.back());
				const editor_asset_node_handle_t asset_node = _asset_tree.emplace(editor_asset_node_t{.asset = std::move(asset), .name = name, .type = editor_asset_node_type_e::asset});
				_asset_tree.attach(parent, asset_node);
			}
			else
			{
				asset.source_abs_path					   = entry.path;
				const editor_asset_node_handle_t file_node = _asset_tree.emplace(editor_asset_node_t{.asset = std::move(asset), .name = parts.back(), .type = editor_asset_node_type_e::file});
				_asset_tree.attach(parent, file_node);
			}
		}

		_generation++;
		return true;
	}

	bool editor_asset_manager_t::read_asset(const char* path, editor_asset_t& out_asset) const
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

	editor_asset_node_handle_t editor_asset_manager_t::find_child_folder(editor_asset_node_handle_t parent, const string_t& name) const
	{
		editor_asset_node_handle_t child = _asset_tree.first_child(parent);
		while (!child.is_null())
		{
			const editor_asset_node_t& node = _asset_tree.value(child);
			if (node.type == editor_asset_node_type_e::folder && node.name == name)
				return child;

			child = _asset_tree.next_sibling(child);
		}
		return {};
	}

	editor_asset_node_handle_t editor_asset_manager_t::get_or_create_child_folder(editor_asset_node_handle_t parent, const string_t& name)
	{
		const editor_asset_node_handle_t existing = find_child_folder(parent, name);
		if (!existing.is_null())
			return existing;

		const editor_asset_node_handle_t folder = _asset_tree.emplace(editor_asset_node_t{.name = name, .type = editor_asset_node_type_e::folder});
		_asset_tree.attach(parent, folder);
		return folder;
	}
}
