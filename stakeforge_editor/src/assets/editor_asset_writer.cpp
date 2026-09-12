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

#include "assets/editor_asset_writer.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/thumbnail/editor_asset_thumbnailer.hpp"
#include "assets/editor_asset_util.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		void resolve_asset_guids(sid_t requested_guid, bool allow_overwrite, const char* asset_path, editor_asset_type_e asset_type, sid_t& out_guid, sid_t& out_thumbnail_guid)
		{
			editor_asset_t existing		= {};
			const bool	   has_existing = allow_overwrite && file_system_t::exists(asset_path) && editor_asset_io_t::read_asset(asset_path, existing);

			out_guid = requested_guid;
			if (out_guid == NULL_SID && has_existing)
				out_guid = existing.guid;
			if (out_guid == NULL_SID)
				out_guid = editor_asset_util_t::generate_unique_asset_guid();

			const sid_t builtin_thumbnail_guid = editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset_type);
			if (builtin_thumbnail_guid != NULL_SID)
				out_thumbnail_guid = builtin_thumbnail_guid;
			else
			{
				out_thumbnail_guid = has_existing ? existing.thumbnail_guid : NULL_SID;
				if (out_thumbnail_guid == NULL_SID || out_thumbnail_guid == out_guid)
					out_thumbnail_guid = editor_asset_thumbnailer_t::make_thumbnail_guid(asset_type, {.data = &out_guid, .size = 1});
			}
		}
	}

	bool editor_asset_writer_t::write_file_asset(const editor_asset_write_file_desc_t& desc, editor_asset_t* out_asset, string_t* out_asset_path)
	{
		if (!editor_directories_t::is_valid_asset_name(desc.name))
			return false;

		const char* source_name = desc.source_name != nullptr ? desc.source_name : desc.name;
		if (!editor_directories_t::is_valid_asset_name(source_name))
			return false;

		const string_t asset_path = editor_asset_path_t::make_asset_path(desc.parent_path, desc.name);
		if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
		{
			SFG_ERR("can't write asset as asset with this name already exists! {0}", asset_path.c_str());
			return false;
		}

		sid_t guid			 = NULL_SID;
		sid_t thumbnail_guid = NULL_SID;
		resolve_asset_guids(desc.guid, desc.allow_overwrite, asset_path.c_str(), desc.asset_type, guid, thumbnail_guid);

		editor_asset_t asset = {};
		asset.version		 = editor_asset_t::VERSION;
		asset.guid			 = guid;
		asset.thumbnail_guid = thumbnail_guid;
		asset.asset_type	 = desc.asset_type;
		asset.source_type	 = editor_asset_source_type_e::file;
		asset.sub_type		 = desc.sub_type;
		if (desc.cook_options != nullptr)
			editor_asset_io_t::set_cook_options_json(asset, *desc.cook_options);
		if (desc.embedded_source != nullptr)
			editor_asset_io_t::set_embedded_source_json(asset, *desc.embedded_source);

		if (desc.source_name != nullptr)
		{
			string_t source_path = editor_asset_path_t::normalize_directory(desc.parent_path);
			source_path += desc.source_name;
			source_path += ".";
			source_path += desc.source_extension;

			if (file_system_t::exists(source_path.c_str()))
				asset.source_relative = editor_asset_path_t::get_source_relative(editor_project_t::get()._runtime.assets_path.c_str(), source_path.c_str());
		}

		if (asset.source_relative.empty())
		{
			string_t template_path = file_system_t::get_running_directory();
			template_path += desc.source_template_relative;
			SFG_ASSERT(file_system_t::exists(template_path.c_str()));

			const string_t source_path = editor_asset_path_t::make_unique_source_path(desc.parent_path, source_name, desc.source_extension);
			if (!file_system_t::copy_file(template_path.c_str(), source_path.c_str()))
			{
				SFG_ERR("failed to copy asset source template {0} to {1}", template_path.c_str(), source_path.c_str());
				return false;
			}

			SFG_ASSERT(file_system_t::exists(source_path.c_str()));
			asset.source_relative = editor_asset_path_t::get_source_relative(editor_project_t::get()._runtime.assets_path.c_str(), source_path.c_str());
			SFG_ASSERT(!asset.source_relative.empty());
		}

		if (!editor_asset_io_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to write file asset {0}", asset_path.c_str());
			return false;
		}

		if (out_asset != nullptr)
			*out_asset = asset;
		if (out_asset_path != nullptr)
			*out_asset_path = asset_path;
		return true;
	}

	bool editor_asset_writer_t::write_existing_file_asset(const editor_asset_write_existing_file_desc_t& desc, editor_asset_t* out_asset, string_t* out_asset_path)
	{
		if (!editor_directories_t::is_valid_asset_name(desc.name))
			return false;

		const string_t asset_path = editor_asset_path_t::make_asset_path(desc.parent_path, desc.name);
		if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
		{
			SFG_ERR("can't write asset as asset with this name already exists! {0}", asset_path.c_str());
			return false;
		}

		sid_t guid			 = NULL_SID;
		sid_t thumbnail_guid = NULL_SID;
		resolve_asset_guids(desc.guid, desc.allow_overwrite, asset_path.c_str(), desc.asset_type, guid, thumbnail_guid);

		editor_asset_t asset = {};
		asset.version		 = editor_asset_t::VERSION;
		asset.guid			 = guid;
		asset.thumbnail_guid = thumbnail_guid;
		asset.asset_type	 = desc.asset_type;
		asset.source_type	 = desc.source_type;
		asset.sub_type		 = desc.sub_type;
		if (desc.cook_options != nullptr)
			editor_asset_io_t::set_cook_options_json(asset, *desc.cook_options);

		if (!editor_asset_path_t::set_source_relative_or_copy(asset, desc.parent_path, desc.name, desc.source_full_path))
		{
			SFG_ERR("failed to set source for asset {0}", desc.name);
			return false;
		}

		if (!editor_asset_io_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to write file asset {0}", asset_path.c_str());
			return false;
		}

		if (out_asset != nullptr)
			*out_asset = asset;
		if (out_asset_path != nullptr)
			*out_asset_path = asset_path;
		return true;
	}

	bool editor_asset_writer_t::write_embedded_asset(const editor_asset_write_embedded_desc_t& desc, editor_asset_t* out_asset, string_t* out_asset_path)
	{
		if (!editor_directories_t::is_valid_asset_name(desc.name))
			return false;

		const string_t asset_path = editor_asset_path_t::make_asset_path(desc.parent_path, desc.name);
		if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
		{
			SFG_ERR("can't write asset as asset with this name already exists! {0}", asset_path.c_str());
			return false;
		}

		sid_t guid			 = NULL_SID;
		sid_t thumbnail_guid = NULL_SID;
		resolve_asset_guids(desc.guid, desc.allow_overwrite, asset_path.c_str(), desc.asset_type, guid, thumbnail_guid);

		editor_asset_t asset = {};
		asset.version		 = editor_asset_t::VERSION;
		asset.guid			 = guid;
		asset.thumbnail_guid = thumbnail_guid;
		asset.asset_type	 = desc.asset_type;
		asset.source_type	 = editor_asset_source_type_e::embedded;
		asset.sub_type		 = desc.sub_type;
		editor_asset_io_t::set_embedded_source_json(asset, *desc.embedded_source);

		if (!editor_asset_io_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to write embedded asset {0}", asset_path.c_str());
			return false;
		}

		if (out_asset != nullptr)
			*out_asset = asset;
		if (out_asset_path != nullptr)
			*out_asset_path = asset_path;
		return true;
	}

	bool editor_asset_writer_t::write_none_source_asset(const editor_asset_write_none_desc_t& desc, editor_asset_t* out_asset, string_t* out_asset_path)
	{
		if (!editor_directories_t::is_valid_asset_name(desc.name))
			return false;

		const string_t asset_path = editor_asset_path_t::make_asset_path(desc.parent_path, desc.name);
		if (!desc.allow_overwrite && file_system_t::exists(asset_path.c_str()))
			return false;

		sid_t guid			 = NULL_SID;
		sid_t thumbnail_guid = NULL_SID;
		resolve_asset_guids(desc.guid, desc.allow_overwrite, asset_path.c_str(), desc.asset_type, guid, thumbnail_guid);

		editor_asset_t asset = {};
		asset.version		 = editor_asset_t::VERSION;
		asset.guid			 = guid;
		asset.thumbnail_guid = thumbnail_guid;
		asset.asset_type	 = desc.asset_type;
		asset.source_type	 = editor_asset_source_type_e::none;
		asset.sub_type		 = desc.sub_type;

		if (!editor_asset_io_t::write_asset(asset_path.c_str(), asset))
		{
			SFG_ERR("failed to write asset {0}", asset_path.c_str());
			return false;
		}

		if (out_asset != nullptr)
			*out_asset = asset;
		if (out_asset_path != nullptr)
			*out_asset_path = asset_path;
		return true;
	}

	bool editor_asset_writer_t::read_embedded_source(const char* asset_relative_path, nlohmann::json& out_embedded_source)
	{
		string_t path = file_system_t::get_running_directory();
		path += asset_relative_path;
		SFG_ASSERT(file_system_t::exists(path.c_str()));

		editor_asset_t asset  = {};
		const bool	   result = editor_asset_io_t::read_asset(path.c_str(), asset);
		SFG_ASSERT(result);
		if (!result)
		{
			SFG_ERR("failed to read embedded source template {0}", path.c_str());
			return false;
		}

		out_embedded_source = editor_asset_io_t::get_embedded_source_json(asset);
		return true;
	}

	bool editor_asset_writer_t::read_cook_options(const char* asset_relative_path, nlohmann::json& out_cook_options)
	{
		string_t path = file_system_t::get_running_directory();
		path += asset_relative_path;
		SFG_ASSERT(file_system_t::exists(path.c_str()));

		editor_asset_t asset  = {};
		const bool	   result = editor_asset_io_t::read_asset(path.c_str(), asset);
		SFG_ASSERT(result);
		if (!result)
		{
			SFG_ERR("failed to read cook options template {0}", path.c_str());
			return false;
		}

		out_cook_options = editor_asset_io_t::get_cook_options_json(asset);
		return true;
	}
}
