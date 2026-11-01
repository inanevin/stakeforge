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

#include "editor/editor_exports.hpp"

#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/data/string.hpp>
#include <sfg/core/data/vector.hpp>

namespace sfg
{
	struct thumbnail_lru_t
	{
		sid_t thumbnail_guid = NULL_SID;
		u64	  last_used_tick = 0;
	};

	class SFG_EDITOR_API editor_asset_thumbnail_manager_t final
	{
	public:
		editor_asset_thumbnail_manager_t()													 = default;
		~editor_asset_thumbnail_manager_t()													 = default;
		editor_asset_thumbnail_manager_t(const editor_asset_thumbnail_manager_t&)			 = delete;
		editor_asset_thumbnail_manager_t& operator=(const editor_asset_thumbnail_manager_t&) = delete;

		static inline editor_asset_thumbnail_manager_t& get()
		{
			static editor_asset_thumbnail_manager_t s_instance = {};

			return s_instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static inline constexpr u64 REQUEST_INTERVAL_TICKS = 30;

		void tick();
		void request_thumbnail(sid_t thumbnail_guid);
		void on_thumbnail_regenerated(sid_t thumbnail_guid);
		void request_render(sid_t asset_guid);
		void remove_asset(sid_t asset_guid, sid_t thumbnail_guid);

	private:
		thumbnail_lru_t _thumbnails[384] = {};
		vector_t<sid_t> _render_requests;
		string_t		_cache_path;
	};
}
