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

#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct editor_asset_t;

	class editor_asset_io_t final
	{
	public:
		editor_asset_io_t()									   = delete;
		~editor_asset_io_t()								   = delete;
		editor_asset_io_t(const editor_asset_io_t&)			   = delete;
		editor_asset_io_t& operator=(const editor_asset_io_t&) = delete;

		static bool			  read_asset(const char* path, editor_asset_t& out_asset);
		static bool			  write_asset(const char* path, const editor_asset_t& asset);
		static nlohmann::json get_embedded_source_json(const editor_asset_t& asset);
		static nlohmann::json get_cook_options_json(const editor_asset_t& asset);
		static void			  set_embedded_source_json(editor_asset_t& asset, const nlohmann::json& source);
		static void			  set_cook_options_json(editor_asset_t& asset, const nlohmann::json& options);
	};
}
