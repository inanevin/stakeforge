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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/bitmask.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/semaphore_data.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	struct editor_asset_t;

	class editor_thumbnail_render_service_t final
	{
	public:
		editor_thumbnail_render_service_t()													   = default;
		~editor_thumbnail_render_service_t()												   = default;
		editor_thumbnail_render_service_t(const editor_thumbnail_render_service_t&)			   = delete;
		editor_thumbnail_render_service_t& operator=(const editor_thumbnail_render_service_t&) = delete;

		static inline editor_thumbnail_render_service_t& get()
		{
			static editor_thumbnail_render_service_t s_instance;
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

		bool render_thumbnail(const editor_asset_t& asset);

	private:
		void setup_base_world();
		void clear_world_for_setup();
		void setup_camera_for_asset();
		void setup_world_for_prefab(const editor_asset_t& asset);
		void setup_world_for_material(const editor_asset_t& asset);
		void setup_world_for_mesh(const editor_asset_t& asset);
		void setup_world_for_skybox(const editor_asset_t& asset);
		void setup_world_for_animation(const editor_asset_t& asset);
		void produce_snapshot();
		void render_world();
		void resolve_world_to_thumbnail_texture();
		void readback_thumbnail_texture();
		bool save_rendered_thumbnail(const editor_asset_t& asset);

	private:
		world_t					_world;
		world_render_context_t	_render_context;
		world_render_snapshot_t _snapshot;
		semaphore_data_t		_semaphore_frame	= {};
		semaphore_data_t		_semaphore_transfer = {};
		semaphore_data_t		_semaphore_readback = {};
		gfx_handle_t			_cmd_prepare		= {};
		gfx_handle_t			_cmd_transfer		= {};
		gfx_handle_t			_cmd_transit		= {};
		gfx_handle_t			_cmd_resolve		= {};
		gfx_handle_t			_global_buffer		= {};
		gfx_handle_t			_thumbnail_texture	= {};
		gfx_handle_t			_thumbnail_readback = {};
		gfx_handle_t			_thumbnail_shader	= {};
		vector_t<u8>			_readback_pixels;
		gpu_index_t				_global_index		= NULL_GPU_INDEX;
		u8*						_mapped_global		= nullptr;
		u8*						_mapped_readback	= nullptr;
		entity_id_t				_environment_entity = NULL_ENTITY_ID;
		entity_id_t				_camera_entity		= NULL_ENTITY_ID;
		entity_id_t				_display_entity		= NULL_ENTITY_ID;
		bool					_initialized		= false;
	};
}
