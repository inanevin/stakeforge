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

#include "assets/thumbnail/editor_asset_thumbnail_database.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/thumbnail/editor_asset_thumbnailer.hpp"
#include "assets/thumbnail/editor_thumbnail_render_service.hpp"
#include "editor_app.hpp"
#include <sfg/io/log.hpp>

#include <sfg/data/frame_vector.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/resource_type.hpp>

#include <algorithm>

namespace sfg
{
	void editor_asset_thumbnail_database_t::init()
	{
	}

	void editor_asset_thumbnail_database_t::uninit()
	{
		request_t request = {};
		while (_requests.try_dequeue(request))
		{
		}
	}

	void editor_asset_thumbnail_database_t::tick()
	{
		frame_vector_t<pending_t> pending;
		request_t				  request = {};
		while (_requests.try_dequeue(request))
		{
			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(request.asset_guid);
			if (asset == nullptr)
				continue;

			if (asset->thumbnail_guid == NULL_SID)
				continue;

			if (asset->thumbnail_guid == editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset->asset_type))
				continue;

			auto it = std::find_if(pending.begin(), pending.end(), [&](const pending_t& entry) { return entry.thumbnail_guid == asset->thumbnail_guid; });
			if (it == pending.end())
			{
				pending.push_back({
					.asset			= asset,
					.thumbnail_guid = asset->thumbnail_guid,
					.kind			= request.kind,
				});
				continue;
			}

			if (request.kind == request_kind_e::render)
			{
				it->asset = asset;
				it->kind  = request_kind_e::render;
			}
		}

		bool has_render_requests = false;
		for (const pending_t& entry : pending)
			has_render_requests |= entry.kind == request_kind_e::render;

		editor_thumbnail_render_service_t& render_service = editor_thumbnail_render_service_t::get();
		if (has_render_requests || render_service.has_pending_work())
			editor_app_t::get().stop_render();

		for (const pending_t& entry : pending)
		{
			if (entry.kind == request_kind_e::render)
			{
				if (editor_asset_thumbnailer_t::is_renderable_thumbnail(entry.asset->asset_type))
					render_service.request_thumbnail(*entry.asset);
			}
			else
				load_thumbnail(*entry.asset);
		}

		render_service.tick();

		sid_t completed_asset_guid	   = NULL_SID;
		sid_t completed_thumbnail_guid = NULL_SID;
		while (render_service.pop_completed(completed_asset_guid, completed_thumbnail_guid))
		{
			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(completed_asset_guid);
			if (asset != nullptr && asset->thumbnail_guid == completed_thumbnail_guid)
				load_thumbnail(*asset);
		}
	}

	void editor_asset_thumbnail_database_t::load_all_ready()
	{
		resource_manager_t& resource_manager = resource_manager_t::get();
		for (const auto& asset_pair : editor_asset_manager_t::get().get_assets())
		{
			const editor_asset_t& asset = asset_pair.second;
			if (asset.thumbnail_guid == NULL_SID)
				continue;

			if (asset.thumbnail_guid == editor_asset_thumbnailer_t::get_builtin_thumbnail_guid(asset.asset_type))
				continue;

			if (resource_manager.find_entry(asset.thumbnail_guid) != nullptr)
				continue;

			const string_t cache_path = editor_asset_path_t::get_cache_path_for_guid(asset.thumbnail_guid);
			if (file_system_t::exists(cache_path.c_str()))
			{
				if (resource_manager.load_resource(asset.thumbnail_guid, resource_type_e::texture) != resource_state_e::failed)
					track_loaded_thumbnail(asset.thumbnail_guid);
			}
		}
	}

	void editor_asset_thumbnail_database_t::unload_all_thumbnails()
	{
		resource_manager_t& resource_manager = resource_manager_t::get();
		for (auto it = _loaded_thumbnails.rbegin(); it != _loaded_thumbnails.rend(); ++it)
		{
			if (resource_manager.find_entry(*it) != nullptr)
				resource_manager.unload_resource(*it, true);
		}
		_loaded_thumbnails.resize(0);
	}

	void editor_asset_thumbnail_database_t::request_generated(sid_t asset_guid)
	{
		SFG_ASSERT(asset_guid != NULL_SID);
		push_request({.asset_guid = asset_guid, .kind = request_kind_e::generated});
	}

	void editor_asset_thumbnail_database_t::request_render(sid_t asset_guid)
	{
		SFG_ASSERT(asset_guid != NULL_SID);
		push_request({.asset_guid = asset_guid, .kind = request_kind_e::render});
	}

	void editor_asset_thumbnail_database_t::push_request(request_t request)
	{
		_requests.enqueue(request);
	}

	void editor_asset_thumbnail_database_t::load_thumbnail(const editor_asset_t& asset)
	{
		resource_manager_t& resource_manager = resource_manager_t::get();
		if (resource_manager.find_entry(asset.thumbnail_guid) != nullptr)
			resource_manager.unload_resource(asset.thumbnail_guid, true);

		if (resource_manager.load_resource(asset.thumbnail_guid, resource_type_e::texture) != resource_state_e::failed)
			track_loaded_thumbnail(asset.thumbnail_guid);
	}

	void editor_asset_thumbnail_database_t::track_loaded_thumbnail(sid_t thumbnail_guid)
	{
		auto it = std::find(_loaded_thumbnails.begin(), _loaded_thumbnails.end(), thumbnail_guid);
		if (it == _loaded_thumbnails.end())
			_loaded_thumbnails.push_back(thumbnail_guid);
	}
}
