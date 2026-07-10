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

#include "assets/editor_asset_type.hpp"

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/bitmask.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/semaphore_data.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_init_config.hpp>

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

		void request_thumbnail(const editor_asset_t& asset);
		void tick();
		bool pop_completed(sid_t& out_asset_guid, sid_t& out_thumbnail_guid);
		bool has_pending_work() const;

	private:
		struct thumbnail_request_t
		{
			sid_t				asset_guid	   = NULL_SID;
			sid_t				thumbnail_guid = NULL_SID;
			editor_asset_type_e asset_type	   = editor_asset_type_e::invalid;
		};

		struct thumbnail_world_t
		{
			world_t*					world			   = nullptr;
			vector_t<resource_handle_t> texture_resources  = {};
			entity_id_t					environment_entity = NULL_ENTITY_ID;
			entity_id_t					camera_entity	   = NULL_ENTITY_ID;
			entity_id_t					display_entity	   = NULL_ENTITY_ID;
		};

		struct pending_render_t
		{
			thumbnail_request_t request		= {};
			u32					world_index = 0;
		};

		struct completed_render_t
		{
			sid_t asset_guid	 = NULL_SID;
			sid_t thumbnail_guid = NULL_SID;
		};

	private:
		u32	 acquire_world();
		void release_world(u32 world_index);
		void grow_world_pool(u32 count);
		void setup_base_world(thumbnail_world_t& thumbnail_world);
		void setup_camera_for_asset(thumbnail_world_t& thumbnail_world);
		void setup_world_for_asset(thumbnail_world_t& thumbnail_world, const thumbnail_request_t& request);
		void setup_world_for_prefab(thumbnail_world_t& thumbnail_world, const thumbnail_request_t& request);
		void setup_world_for_material(thumbnail_world_t& thumbnail_world, const thumbnail_request_t& request);
		void setup_world_for_mesh(thumbnail_world_t& thumbnail_world, const thumbnail_request_t& request);
		void setup_world_for_skybox(thumbnail_world_t& thumbnail_world, const thumbnail_request_t& request);
		void setup_world_for_animation(thumbnail_world_t& thumbnail_world, const thumbnail_request_t& request);
		void collect_texture_resources(pending_render_t& pending_render);
		void prepare_request(const thumbnail_request_t& request);
		bool is_ready_to_render(const pending_render_t& pending_render) const;
		void produce_snapshot(thumbnail_world_t& thumbnail_world);
		void render_world();
		void resolve_world_to_thumbnail_texture();
		void readback_thumbnail_texture();
		bool save_rendered_thumbnail(const thumbnail_request_t& request);

	private:
		world_init_config_t			  _world_config;
		world_render_context_t		  _render_context;
		world_render_snapshot_t		  _snapshot;
		vector_t<thumbnail_world_t>	  _worlds;
		vector_t<pending_render_t>	  _pending_renders;
		vector_t<thumbnail_request_t> _requests;
		vector_t<completed_render_t>  _completed_renders;
		vector_t<u32>				  _available_worlds;
		semaphore_data_t			  _semaphore_frame	  = {};
		semaphore_data_t			  _semaphore_transfer = {};
		semaphore_data_t			  _semaphore_readback = {};
		gfx_handle_t				  _cmd_prepare		  = {};
		gfx_handle_t				  _cmd_transfer		  = {};
		gfx_handle_t				  _cmd_transit		  = {};
		gfx_handle_t				  _cmd_resolve		  = {};
		gfx_handle_t				  _global_buffer	  = {};
		gfx_handle_t				  _thumbnail_texture  = {};
		gfx_handle_t				  _thumbnail_readback = {};
		gfx_handle_t				  _thumbnail_shader	  = {};
		vector_t<u8>				  _readback_pixels;
		gpu_index_t					  _global_index	   = NULL_GPU_INDEX;
		u8*							  _mapped_global   = nullptr;
		u8*							  _mapped_readback = nullptr;
		bool						  _initialized	   = false;
	};
}
