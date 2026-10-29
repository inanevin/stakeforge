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

#include "tools/assets/editor_asset_io.hpp"
#include "tools/assets/editor_asset.hpp"

#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/serialization/serialization.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		string_t json_to_string(const nlohmann::json& json, bool keep_null)
		{
			return !keep_null && json.is_null() ? string_t{} : string_t(json.dump());
		}
	}

	bool editor_asset_io_t::read_asset(const char* path, editor_asset_t& out_asset)
	{
		const string_t		 json_text = file_system_t::read_file_as_string(path);
		const nlohmann::json doc	   = nlohmann::json::parse(json_text, nullptr, false);

		if (doc.is_discarded())
		{
			SFG_ERR("failed to parse asset {0}", path);
			return false;
		}

		editor_asset_t asset = {};

		try
		{
			doc.get_to(asset);
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to deserialize asset {0}: {1}", path, error.what());
			return false;
		}

		out_asset = std::move(asset);

		return true;
	}

	bool editor_asset_io_t::write_asset(const char* path, const editor_asset_t& asset)
	{
		try
		{
			const nlohmann::json json_data = asset;
			const string_t		 data	   = json_data.dump(4);

			return serializer_t::write_to_file(string_view_t(data.data(), data.size()), path);
		}
		catch (const nlohmann::json::exception& error)
		{
			SFG_ERR("failed to serialize asset {0}: {1}", path, error.what());
			return false;
		}
	}

	bool editor_asset_io_t::parse_embedded_source_json(const editor_asset_t& asset, nlohmann::json& out)
	{
		nlohmann::json parsed = asset.embedded_source.empty() ? nlohmann::json() : nlohmann::json::parse(asset.embedded_source, nullptr, false);

		if (parsed.is_discarded())
		{
			SFG_ERR("failed to parse embedded source JSON for asset {0}", asset.guid);
			return false;
		}

		out = std::move(parsed);

		return true;
	}

	bool editor_asset_io_t::parse_cook_options_json(const editor_asset_t& asset, nlohmann::json& out)
	{
		nlohmann::json parsed = asset.cook_options.empty() ? nlohmann::json::object() : nlohmann::json::parse(asset.cook_options, nullptr, false);

		if (parsed.is_discarded())
		{
			SFG_ERR("failed to parse cook options JSON for asset {0}", asset.guid);
			return false;
		}

		out = std::move(parsed);

		return true;
	}

	void editor_asset_io_t::set_embedded_source_json(editor_asset_t& asset, const nlohmann::json& source)
	{
		asset.embedded_source = json_to_string(source, false);
	}

	void editor_asset_io_t::set_cook_options_json(editor_asset_t& asset, const nlohmann::json& options)
	{
		asset.cook_options = json_to_string(options.is_null() ? nlohmann::json::object() : options, true);
	}
}
