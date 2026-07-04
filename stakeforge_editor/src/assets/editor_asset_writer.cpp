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

#include "assets/editor_asset_writer.hpp"

#include "assets/editor_asset_manager.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	const editor_asset_node_t& editor_asset_writer_t::get_parent_folder(editor_asset_node_handle_t parent_node)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!parent_node.is_null());
		SFG_ASSERT(tree.is_valid(parent_node));
		const editor_asset_node_t& node = tree.value(parent_node);
		SFG_ASSERT(node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!node.full_path.empty());
		return node;
	}

	bool editor_asset_writer_t::write_file_asset(const editor_asset_write_file_desc_t& desc, editor_asset_t* out_asset)
	{
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::invalid);
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::count);
		SFG_ASSERT(desc.source_extension != nullptr);
		SFG_ASSERT(desc.source_extension[0] != '\0');
		SFG_ASSERT(desc.source_template_relative != nullptr);
		SFG_ASSERT(desc.source_template_relative[0] != '\0');

		const editor_asset_node_t& parent_node = get_parent_folder(desc.parent_node);

		if (!editor_directories_t::is_valid_asset_name(desc.name))
			return false;

		const char* source_name = desc.source_name != nullptr ? desc.source_name : desc.name;
		if (!editor_directories_t::is_valid_asset_name(source_name))
			return false;

		const string_t asset_path = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), desc.name);
		if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
			return false;

		sid_t guid = desc.guid;
		if (guid == NULL_SID && desc.allow_overwrite)
			guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());
		if (guid == NULL_SID)
			guid = editor_asset_util_t::generate_unique_asset_guid();

		editor_asset_t asset = {};
		asset.version		 = editor_asset_t::VERSION;
		asset.guid			 = guid;
		asset.asset_type	 = desc.asset_type;
		asset.source_type	 = editor_asset_source_type_e::file;
		asset.sub_type		 = desc.sub_type;
		if (desc.cook_options != nullptr)
			asset.cook_options = *desc.cook_options;

		if (desc.source_name != nullptr)
		{
			string_t source_path = editor_asset_util_t::normalize_directory(parent_node.full_path.c_str());
			source_path += desc.source_name;
			source_path += ".";
			source_path += desc.source_extension;
			if (file_system_t::exists(source_path.c_str()))
				asset.source_relative = editor_asset_util_t::get_source_relative(editor_project_t::get()._runtime.assets_path.c_str(), source_path.c_str());
		}

		if (asset.source_relative.empty())
		{
			string_t template_path = file_system_t::get_running_directory();
			template_path += desc.source_template_relative;
			SFG_ASSERT(file_system_t::exists(template_path.c_str()));

			const string_t source_path = editor_asset_util_t::make_unique_source_path(parent_node.full_path.c_str(), source_name, desc.source_extension);
			if (!file_system_t::copy_file(template_path.c_str(), source_path.c_str()))
			{
				SFG_ERR("failed to copy asset source template {0} to {1}", template_path.c_str(), source_path.c_str());
				return false;
			}

			SFG_ASSERT(file_system_t::exists(source_path.c_str()));
			asset.source_relative = editor_asset_util_t::get_source_relative(editor_project_t::get()._runtime.assets_path.c_str(), source_path.c_str());
			SFG_ASSERT(!asset.source_relative.empty());
		}

		if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to write file asset {0}", asset_path.c_str());
			return false;
		}

		if (out_asset != nullptr)
			*out_asset = asset;
		return true;
	}

	bool editor_asset_writer_t::write_embedded_asset(const editor_asset_write_embedded_desc_t& desc, editor_asset_t* out_asset)
	{
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::invalid);
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::count);
		SFG_ASSERT(desc.embedded_source != nullptr);

		const editor_asset_node_t& parent_node = get_parent_folder(desc.parent_node);

		if (!editor_directories_t::is_valid_asset_name(desc.name))
			return false;

		const string_t asset_path = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), desc.name);
		if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
			return false;

		sid_t guid = desc.guid;
		if (guid == NULL_SID && desc.allow_overwrite)
			guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());
		if (guid == NULL_SID)
			guid = editor_asset_util_t::generate_unique_asset_guid();

		editor_asset_t asset  = {};
		asset.version		  = editor_asset_t::VERSION;
		asset.guid			  = guid;
		asset.asset_type	  = desc.asset_type;
		asset.source_type	  = editor_asset_source_type_e::embedded;
		asset.sub_type		  = desc.sub_type;
		asset.embedded_source = *desc.embedded_source;

		if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to write embedded asset {0}", asset_path.c_str());
			return false;
		}

		if (out_asset != nullptr)
			*out_asset = asset;
		return true;
	}

	bool editor_asset_writer_t::write_none_source_asset(const editor_asset_write_none_desc_t& desc, editor_asset_t* out_asset)
	{
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::invalid);
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::count);

		const editor_asset_node_t& parent_node = get_parent_folder(desc.parent_node);

		if (!editor_directories_t::is_valid_asset_name(desc.name))
			return false;

		const string_t asset_path = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), desc.name);
		if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
			return false;

		sid_t guid = desc.guid;
		if (guid == NULL_SID && desc.allow_overwrite)
			guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());
		if (guid == NULL_SID)
			guid = editor_asset_util_t::generate_unique_asset_guid();

		editor_asset_t asset = {};
		asset.version		 = editor_asset_t::VERSION;
		asset.guid			 = guid;
		asset.asset_type	 = desc.asset_type;
		asset.source_type	 = editor_asset_source_type_e::none;
		asset.sub_type		 = desc.sub_type;

		if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to write asset {0}", asset_path.c_str());
			return false;
		}

		if (out_asset != nullptr)
			*out_asset = asset;
		return true;
	}

	bool editor_asset_writer_t::read_embedded_source(const char* asset_relative_path, nlohmann::json& out_embedded_source)
	{
		SFG_ASSERT(asset_relative_path != nullptr);
		SFG_ASSERT(asset_relative_path[0] != '\0');

		string_t path = file_system_t::get_running_directory();
		path += asset_relative_path;
		SFG_ASSERT(file_system_t::exists(path.c_str()));

		editor_asset_t asset  = {};
		const bool	   result = editor_asset_util_t::read_asset(path.c_str(), asset);
		SFG_ASSERT(result);
		if (!result)
		{
			SFG_ERR("failed to read embedded source template {0}", path.c_str());
			return false;
		}

		out_embedded_source = asset.embedded_source;
		return true;
	}

	bool editor_asset_writer_t::read_cook_options(const char* asset_relative_path, nlohmann::json& out_cook_options)
	{
		SFG_ASSERT(asset_relative_path != nullptr);
		SFG_ASSERT(asset_relative_path[0] != '\0');

		string_t path = file_system_t::get_running_directory();
		path += asset_relative_path;
		SFG_ASSERT(file_system_t::exists(path.c_str()));

		editor_asset_t asset  = {};
		const bool	   result = editor_asset_util_t::read_asset(path.c_str(), asset);
		SFG_ASSERT(result);
		if (!result)
		{
			SFG_ERR("failed to read cook options template {0}", path.c_str());
			return false;
		}

		out_cook_options = asset.cook_options;
		return true;
	}
}
