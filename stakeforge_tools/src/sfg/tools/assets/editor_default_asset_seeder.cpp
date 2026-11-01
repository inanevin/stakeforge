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

#include "tools/assets/editor_default_asset_seeder.hpp"
#include "tools/assets/editor_asset.hpp"
#include "tools/assets/editor_asset_cooker.hpp"
#include "tools/assets/editor_asset_io.hpp"
#include "tools/assets/editor_asset_path.hpp"
#include "tools/assets/editor_asset_writer.hpp"
#include "tools/editor_directories.hpp"
#include "tools/editor_project.hpp"

#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/runtime/resources/shader_data_definition.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		struct project_default_asset_t
		{
			editor_asset_t asset		   = {};
			string_t	   name			   = {};
			nlohmann::json embedded_source = {};
			nlohmann::json cook_options	   = {};
		};

		bool is_default_asset_ready(const char* default_assets_dir, const project_default_asset_t& entry)
		{
			const string_t asset_path = editor_asset_path_t::make_asset_path(default_assets_dir, entry.name.c_str());
			editor_asset_t asset	  = {};

			if (!file_system_t::exists(asset_path.c_str()))
				return false;

			if (!editor_asset_io_t::read_asset(asset_path.c_str(), asset))
				return false;

			if (asset.guid != entry.asset.guid || asset.asset_type != entry.asset.asset_type || asset.sub_type != entry.asset.sub_type || asset.source_type != entry.asset.source_type)
				return false;

			nlohmann::json cook_options = {};

			if (!editor_asset_io_t::parse_cook_options_json(asset, cook_options) || cook_options != entry.cook_options)
				return false;

			if (asset.source_type == editor_asset_source_type_e::embedded)
			{
				if (asset.embedded_source.empty())
					return false;

				nlohmann::json embedded_source = {};

				if (!editor_asset_io_t::parse_embedded_source_json(asset, embedded_source))
					return false;

				if (embedded_source != entry.embedded_source)
					return false;
			}
			else if (asset.source_type == editor_asset_source_type_e::file || asset.source_type == editor_asset_source_type_e::file_blob)
			{
				if (asset.source_relative.empty())
					return false;

				if (asset.asset_type == editor_asset_type_e::shader)
				{
					if (asset.embedded_source.empty())
						return false;

					nlohmann::json embedded_source = {};

					if (!editor_asset_io_t::parse_embedded_source_json(asset, embedded_source) || !embedded_source.is_object() || !embedded_source.contains("textures") || !embedded_source.contains("samplers") || !embedded_source.contains("parameters"))
						return false;
				}

				const string_t source_path = editor_asset_path_t::get_source_full_path(editor_project_t::get()._runtime.assets_path.c_str(), asset);

				if (!file_system_t::exists(source_path.c_str()))
					return false;
			}

			return editor_asset_cooker_t::is_asset_cooked(asset);
		}

	}

	bool editor_default_asset_seeder_t::ensure(const char* default_assets_dir)
	{
		const string_t		 manifest_path = editor_directories_t::get_editor_assets() + "project_assets_manifest.sfg_asset";
		const string_t		 json_text	   = file_system_t::read_file_as_string(manifest_path.c_str());
		const nlohmann::json document	   = nlohmann::json::parse(json_text, nullptr, false);

		if (document.is_discarded())
		{
			SFG_ERR("failed to parse project assets manifest {0}", manifest_path);
			return false;
		}

		vector_t<project_default_asset_t> defaults = {};

		try
		{
			const nlohmann::json entries = document.at("assets");

			if (!entries.is_array())
			{
				SFG_ERR("invalid project assets list in {0}", manifest_path);
				return false;
			}

			defaults.reserve(entries.size());

			for (const nlohmann::json& entry : entries)
			{
				project_default_asset_t value	 = {};
				value.name						 = entry.value<string_t>("name", "");
				const string_t template_relative = entry.value<string_t>("template", "");
				const string_t template_path	 = editor_directories_t::get_editor_assets() + template_relative;

				if (!editor_directories_t::is_valid_asset_name(value.name.c_str()) || template_relative.empty())
				{
					SFG_ERR("invalid project asset entry in {0}", manifest_path);
					return false;
				}

				if (!editor_asset_io_t::read_asset(template_path.c_str(), value.asset))
					return false;

				if (value.asset.guid == NULL_SID || !editor_asset_cooker_t::is_cookable(value.asset.asset_type) || (value.asset.source_type != editor_asset_source_type_e::file && value.asset.source_type != editor_asset_source_type_e::embedded))
				{
					SFG_ERR("invalid project asset template {0}", template_path);
					return false;
				}

				if (std::find_if(defaults.begin(), defaults.end(), [&](const project_default_asset_t& other) { return other.name == value.name || other.asset.guid == value.asset.guid; }) != defaults.end())
				{
					SFG_ERR("duplicate project asset name or GUID in {0}: {1}", manifest_path, value.name);
					return false;
				}

				if (!editor_asset_io_t::parse_embedded_source_json(value.asset, value.embedded_source) || !editor_asset_io_t::parse_cook_options_json(value.asset, value.cook_options))
					return false;

				if (value.asset.source_type == editor_asset_source_type_e::file)
				{
					const string_t source_path = editor_directories_t::get_editor_assets() + value.asset.source_relative;

					if (value.asset.source_relative.empty() || !file_system_t::exists(source_path.c_str()))
					{
						SFG_ERR("missing project asset source {0}", source_path);
						return false;
					}
				}
				else if (!value.embedded_source.is_object())
				{
					SFG_ERR("invalid embedded project asset template {0}", template_path);
					return false;
				}

				defaults.push_back(std::move(value));
			}
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to deserialize project assets manifest {0}: {1}", manifest_path, error.what());
			return false;
		}

		for (const project_default_asset_t& entry : defaults)
		{
			const editor_asset_t& asset_template = entry.asset;
			const bool			  embedded		 = asset_template.source_type == editor_asset_source_type_e::embedded;

			if (is_default_asset_ready(default_assets_dir, entry))
				continue;

			editor_asset_t asset	  = {};
			string_t	   asset_path = {};
			bool		   created	  = false;

			if (embedded)
			{
				created = editor_asset_writer_t::write_embedded_asset(
					{
						.embedded_source = &entry.embedded_source,
						.parent_path	 = default_assets_dir,
						.name			 = entry.name.c_str(),
						.guid			 = asset_template.guid,
						.asset_type		 = asset_template.asset_type,
						.sub_type		 = asset_template.sub_type,
						.allow_overwrite = true,
					},
					&asset,
					&asset_path);
			}
			else
			{
				const string_t extension   = file_system_t::get_file_extension(asset_template.source_relative);
				const string_t source_path = editor_directories_t::get_editor_assets() + asset_template.source_relative;
				const string_t destination = string_t(default_assets_dir) + entry.name + "." + extension;

				if (!file_system_t::copy_file(source_path.c_str(), destination.c_str()))
				{
					SFG_ERR("failed to copy default project asset {0} to {1}", source_path, destination);
					return false;
				}

				created = editor_asset_writer_t::write_file_asset(
					{
						.cook_options			  = &entry.cook_options,
						.parent_path			  = default_assets_dir,
						.name					  = entry.name.c_str(),
						.source_name			  = entry.name.c_str(),
						.source_extension		  = extension.c_str(),
						.source_template_relative = asset_template.source_relative.c_str(),
						.guid					  = asset_template.guid,
						.asset_type				  = asset_template.asset_type,
						.sub_type				  = asset_template.sub_type,
						.allow_overwrite		  = true,
					},
					&asset,
					&asset_path);
			}

			if (created && asset.asset_type == editor_asset_type_e::shader)
			{
				shader_data_definition_t definition = {};
				created								= editor_asset_cooker_t::cook_shader(asset, entry.name.c_str(), &definition);

				if (created)
				{
					editor_asset_io_t::set_embedded_source_json(asset, definition);
					created = editor_asset_io_t::write_asset(asset_path.c_str(), asset);
				}
			}
			else if (created)
				created = editor_asset_cooker_t::cook_asset(asset, entry.name.c_str());

			if (!created)
			{
				SFG_ERR("failed to ensure project asset {0} from {1}", entry.name, manifest_path);
				return false;
			}
		}

		return true;
	}
}
