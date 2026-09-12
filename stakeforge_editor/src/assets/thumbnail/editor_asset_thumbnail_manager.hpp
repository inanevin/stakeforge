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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	class editor_asset_thumbnail_manager_t final
	{
	public:
		editor_asset_thumbnail_manager_t()													 = default;
		~editor_asset_thumbnail_manager_t()													 = default;
		editor_asset_thumbnail_manager_t(const editor_asset_thumbnail_manager_t&)			 = delete;
		editor_asset_thumbnail_manager_t& operator=(const editor_asset_thumbnail_manager_t&) = delete;

		static inline editor_asset_thumbnail_manager_t& get()
		{
			static editor_asset_thumbnail_manager_t s_instance;
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

		void tick();
		void load_all_ready();
		void refresh_thumbnail_resource(sid_t thumbnail_guid);
		void request_render(sid_t asset_guid);
		void remove_asset(sid_t asset_guid, sid_t thumbnail_guid);

	private:
		void unload_loaded_thumbnail_resources();

	private:
		vector_t<sid_t> _render_requests;
		vector_t<sid_t> _loaded_thumbnail_resources;
	};
}
