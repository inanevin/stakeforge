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

#include "tools/assets/thumbnail/editor_asset_thumbnail_manager.hpp"
#include "tools/assets/editor_asset.hpp"
#include "tools/assets/editor_asset_manager.hpp"
#include "tools/assets/editor_asset_path.hpp"
#include "tools/assets/thumbnail/editor_asset_thumbnailer.hpp"
#include "tools/assets/thumbnail/editor_thumbnail_render_service.hpp"
#include "tools/editor_app.hpp"
#include <sfg/core/io/file_system.hpp>
#include <sfg/core/runtime/resources/resource_manager.hpp>

namespace sfg
{
	void editor_asset_thumbnail_manager_t::init()
	{
		_render_requests.reserve(editor_asset_manager_t::get().get_assets().size());
	}

	void editor_asset_thumbnail_manager_t::uninit()
	{
		_render_requests.resize(0);

		for (thumbnail_lru_t& thumbnail : _thumbnails)
		{
			if (thumbnail.thumbnail_guid != NULL_SID)
				resource_manager_t::get().unload_resource(thumbnail.thumbnail_guid);

			thumbnail = {};
		}
	}

	void editor_asset_thumbnail_manager_t::tick()
	{
		editor_thumbnail_render_service_t& render_service = editor_thumbnail_render_service_t::get();

		for (const sid_t asset_guid : _render_requests)
		{
			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(asset_guid);

			if (asset == nullptr)
				continue;

			if (asset->thumbnail_guid == NULL_SID)
				continue;

			if (asset->thumbnail_guid == editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset->asset_type))
				continue;

			if (editor_asset_thumbnailer_t::is_renderable_thumbnail(asset->asset_type))
				render_service.request_thumbnail(*asset);
		}

		_render_requests.resize(0);

		if (render_service.has_pending_work())
			editor_app_t::get().stop_render();

		render_service.tick();
	}

	void editor_asset_thumbnail_manager_t::request_thumbnail(sid_t thumbnail_guid)
	{
		if (thumbnail_guid == NULL_SID || editor_asset_thumbnailer_t::is_builtin_thumbnail(thumbnail_guid))
			return;

		const u64		 tick		= editor_app_t::get().get_tick_count();
		thumbnail_lru_t* candidate	= nullptr;
		u64				 oldest_age = 0;

		for (thumbnail_lru_t& thumbnail : _thumbnails)
		{
			if (thumbnail.thumbnail_guid == thumbnail_guid)
			{
				thumbnail.last_used_tick = tick;
				return;
			}

			if (thumbnail.thumbnail_guid == NULL_SID)
			{
				if (candidate == nullptr || candidate->thumbnail_guid != NULL_SID)
					candidate = &thumbnail;

				continue;
			}

			const u64 age = tick - thumbnail.last_used_tick;

			if (age > 2 * REQUEST_INTERVAL_TICKS && (candidate == nullptr || (candidate->thumbnail_guid != NULL_SID && age > oldest_age)))
			{
				candidate  = &thumbnail;
				oldest_age = age;
			}
		}

		if (candidate == nullptr)
			return;

		editor_asset_path_t::get_cache_path_for_guid(thumbnail_guid, _cache_path);

		if (!file_system_t::exists(_cache_path.c_str()))
			return;

		resource_manager_t& resource_manager = resource_manager_t::get();

		if (candidate->thumbnail_guid != NULL_SID)
			resource_manager.unload_resource(candidate->thumbnail_guid);

		*candidate = {};

		if (resource_manager.load_resource(thumbnail_guid, resource_type_e::texture) != resource_state_e::failed)
			*candidate = {.thumbnail_guid = thumbnail_guid, .last_used_tick = tick};
	}

	void editor_asset_thumbnail_manager_t::on_thumbnail_regenerated(sid_t thumbnail_guid)
	{
		for (thumbnail_lru_t& thumbnail : _thumbnails)
		{
			if (thumbnail.thumbnail_guid != thumbnail_guid || thumbnail_guid == NULL_SID)
				continue;

			if (resource_manager_t::get().reload_resource(thumbnail_guid) == resource_state_e::failed)
				thumbnail = {};

			return;
		}
	}

	void editor_asset_thumbnail_manager_t::request_render(sid_t asset_guid)
	{
		_render_requests.push_back(asset_guid);
	}

	void editor_asset_thumbnail_manager_t::remove_asset(sid_t asset_guid, sid_t thumbnail_guid)
	{
		const auto request_end = std::remove(_render_requests.begin(), _render_requests.end(), asset_guid);
		_render_requests.erase(request_end, _render_requests.end());

		editor_thumbnail_render_service_t::get().cancel_asset(asset_guid);

		for (thumbnail_lru_t& thumbnail : _thumbnails)
		{
			if (thumbnail.thumbnail_guid != thumbnail_guid || thumbnail_guid == NULL_SID)
				continue;

			resource_manager_t::get().unload_resource(thumbnail_guid);
			thumbnail = {};
			return;
		}
	}
}
