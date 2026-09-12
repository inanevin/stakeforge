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

#include "assets/thumbnail/editor_asset_thumbnail_manager.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/thumbnail/editor_asset_thumbnailer.hpp"
#include "assets/thumbnail/editor_thumbnail_render_service.hpp"
#include "editor_app.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

namespace sfg
{
	void editor_asset_thumbnail_manager_t::init()
	{
		_render_requests.reserve(editor_asset_manager_t::get().get_assets().size());
		_loaded_thumbnail_resources.reserve(editor_asset_manager_t::get().get_assets().size());
	}

	void editor_asset_thumbnail_manager_t::uninit()
	{
		_render_requests.resize(0);
		unload_loaded_thumbnail_resources();
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

	void editor_asset_thumbnail_manager_t::load_all_ready()
	{
		for (const auto& asset_pair : editor_asset_manager_t::get().get_assets())
		{
			const editor_asset_t& asset = asset_pair.second;

			if (asset.thumbnail_guid == NULL_SID)
				continue;

			if (asset.thumbnail_guid == editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset.asset_type))
				continue;

			const string_t cache_path = editor_asset_path_t::get_cache_path_for_guid(asset.thumbnail_guid);

			if (file_system_t::exists(cache_path.c_str()))
				refresh_thumbnail_resource(asset.thumbnail_guid);
		}
	}

	void editor_asset_thumbnail_manager_t::refresh_thumbnail_resource(sid_t thumbnail_guid)
	{
		resource_manager_t& resource_manager = resource_manager_t::get();
		const bool			was_loaded		 = resource_manager.find_entry(thumbnail_guid) != nullptr;
		const auto			loaded_thumbnail = std::find(_loaded_thumbnail_resources.begin(), _loaded_thumbnail_resources.end(), thumbnail_guid);
		const bool			was_tracked		 = loaded_thumbnail != _loaded_thumbnail_resources.end();

		SFG_ASSERT(was_loaded == was_tracked);

		if (was_loaded)
		{
			if (resource_manager.reload_resource(thumbnail_guid) == resource_state_e::failed)
				_loaded_thumbnail_resources.erase(loaded_thumbnail);

			return;
		}

		if (resource_manager.load_resource(thumbnail_guid, resource_type_e::texture) != resource_state_e::failed)
			_loaded_thumbnail_resources.push_back(thumbnail_guid);
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

		const auto loaded_thumbnail = std::find(_loaded_thumbnail_resources.begin(), _loaded_thumbnail_resources.end(), thumbnail_guid);

		if (loaded_thumbnail == _loaded_thumbnail_resources.end())
			return;

		resource_manager_t& resource_manager = resource_manager_t::get();
		SFG_ASSERT(resource_manager.find_entry(thumbnail_guid) != nullptr);

		resource_manager.unload_resource(thumbnail_guid);
		SFG_ASSERT(resource_manager.find_entry(thumbnail_guid) == nullptr);

		_loaded_thumbnail_resources.erase(loaded_thumbnail);
	}

	void editor_asset_thumbnail_manager_t::unload_loaded_thumbnail_resources()
	{
		resource_manager_t& resource_manager = resource_manager_t::get();

		for (auto it = _loaded_thumbnail_resources.rbegin(); it != _loaded_thumbnail_resources.rend(); ++it)
			resource_manager.unload_resource(*it);

		_loaded_thumbnail_resources.resize(0);
	}
}
