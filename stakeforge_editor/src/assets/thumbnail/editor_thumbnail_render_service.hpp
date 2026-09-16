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
#include "assets/thumbnail/editor_thumbnail_render_util.hpp"

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/bitmask.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/semaphore_data.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/physics/physics_config.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/runtime/world/world_particle_simulation_config.hpp>

namespace sfg
{
	struct editor_asset_t;

	struct editor_thumbnail_render_service_config_t
	{
		physics_runtime_config_t		   physics							 = {};
		world_render_context_config_t	   render_context					 = {};
		world_debug_draw_config_t		   debug_draw						 = {};
		world_particle_simulation_config_t particle_simulation				 = {};
		world_init_config_t				   world							 = {};
		vec2u16_t						   render_resolution				 = vec2u16_t(256, 256);
		u32								   world_pool_initial_capacity		 = 16;
		u32								   world_pool_max_count				 = 64;
		u32								   request_initial_capacity			 = 256;
		u32								   texture_resource_initial_capacity = 32;
		u8								   pixel_bytes						 = 4;

		static editor_thumbnail_render_service_config_t make_default();
	};

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
		void init(const editor_thumbnail_render_service_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void request_thumbnail(const editor_asset_t& asset);
		void cancel_asset(sid_t asset_guid);
		void tick();
		bool has_pending_work() const;

	private:
		struct thumbnail_request_t
		{
			sid_t				asset_guid	   = NULL_SID;
			sid_t				thumbnail_guid = NULL_SID;
			editor_asset_type_e asset_type	   = editor_asset_type_e::invalid;
		};

		struct pending_render_t
		{
			thumbnail_request_t request		= {};
			u32					world_index = 0;
		};

	private:
		u32	 acquire_world();
		void release_world(u32 world_index);
		void grow_world_pool(u32 count);
		void prepare_request(const thumbnail_request_t& request);
		void produce_snapshot(editor_thumbnail_world_t& thumbnail_world);
		void render_world();
		void resolve_world_to_thumbnail_texture();
		void readback_thumbnail_texture();
		bool save_rendered_thumbnail(const thumbnail_request_t& request);

	private:
		editor_thumbnail_render_service_config_t _config = {};
		world_render_context_t					 _render_context;
		world_render_snapshot_t					 _snapshot;
		vector_t<editor_thumbnail_world_t>		 _worlds;
		vector_t<pending_render_t>				 _pending_renders;
		vector_t<thumbnail_request_t>			 _requests;
		vector_t<u32>							 _available_worlds;
		semaphore_data_t						 _semaphore_frame		= {};
		semaphore_data_t						 _semaphore_transfer	= {};
		semaphore_data_t						 _semaphore_readback	= {};
		gfx_handle_t							 _cmd_prepare			= {};
		gfx_handle_t							 _cmd_transfer			= {};
		gfx_handle_t							 _cmd_transit			= {};
		gfx_handle_t							 _cmd_resolve			= {};
		gfx_handle_t							 _global_buffer			= {};
		gfx_handle_t							 _thumbnail_texture		= {};
		gfx_handle_t							 _thumbnail_readback	= {};
		gfx_handle_t							 _thumbnail_shader		= {};
		gfx_handle_t							 _debug_triangle_shader = {};
		vector_t<u8>							 _readback_pixels;
		gpu_index_t								 _global_index	  = NULL_GPU_INDEX;
		u8*										 _mapped_global	  = nullptr;
		u8*										 _mapped_readback = nullptr;
	};
}
