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

#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset.hpp"

#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		nlohmann::json parse_json_string(const string_t& text, const nlohmann::json& fallback)
		{
			if (text.empty())
				return fallback;

			const nlohmann::json parsed = nlohmann::json::parse(text, nullptr, false);
			return parsed.is_discarded() ? fallback : parsed;
		}

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

		doc.get_to(out_asset);
		return true;
	}

	bool editor_asset_io_t::write_asset(const char* path, const editor_asset_t& asset)
	{
		const nlohmann::json json_data = asset;
		const string_t		 data	   = json_data.dump(4);
		return serializer_t::write_to_file(string_view_t(data.data(), data.size()), path);
	}

	nlohmann::json editor_asset_io_t::get_embedded_source_json(const editor_asset_t& asset)
	{
		return parse_json_string(asset.embedded_source, nlohmann::json());
	}

	nlohmann::json editor_asset_io_t::get_cook_options_json(const editor_asset_t& asset)
	{
		return parse_json_string(asset.cook_options, nlohmann::json::object());
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
