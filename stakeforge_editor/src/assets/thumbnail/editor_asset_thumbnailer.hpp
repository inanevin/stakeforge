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

#include "assets/editor_asset_type.hpp"

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>

namespace sfg
{
	struct editor_asset_t;

	enum class editor_asset_thumbnail_source_e : u8
	{
		builtin,
		generated,
	};

	class editor_asset_thumbnailer_t final
	{
	public:
		editor_asset_thumbnailer_t()											 = delete;
		~editor_asset_thumbnailer_t()											 = delete;
		editor_asset_thumbnailer_t(const editor_asset_thumbnailer_t&)			 = delete;
		editor_asset_thumbnailer_t& operator=(const editor_asset_thumbnailer_t&) = delete;

		static void	 generate_thumbnail(const editor_asset_t& asset, const char* display_name = nullptr);
		static bool	 save_thumbnail(const editor_asset_t& asset, span_t<u8> pixels, const char* display_name);
		static bool	 is_renderable_thumbnail(editor_asset_type_e asset_type);
		static sid_t make_thumbnail_guid(editor_asset_type_e asset_type, span_t<const sid_t> pending_guids = {});
		static sid_t get_builtin_thumbnail_guid(editor_asset_type_e asset_type);
	};
}
