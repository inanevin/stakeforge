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

	void editor_asset_thumbnail_manager_t::unload_loaded_thumbnail_resources()
	{
		resource_manager_t& resource_manager = resource_manager_t::get();

		for (auto it = _loaded_thumbnail_resources.rbegin(); it != _loaded_thumbnail_resources.rend(); ++it)
			resource_manager.unload_resource(*it);

		_loaded_thumbnail_resources.resize(0);
	}
}
