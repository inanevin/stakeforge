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

#include "common/size_definitions.hpp"

// gfx
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/buffer.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/proxy/proxy_manager.hpp"

// misc
#include "memory/bump_allocator.hpp"

namespace SFG
{
	struct window_event;
	class vector2ui;
	class window;
	class render_data;
	class game_world_renderer;
	class world;
	class render_event_stream;
	class editor;

	class renderer
	{
	public:
		renderer(window& win, world& w, render_event_stream& event_stream, void* editor);

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		bool init(const vector2ui16& game_resolution);
		void uninit();
		void wait_backend();
		void tick();
		void render();

		// -----------------------------------------------------------------------------
		// external control
		// -----------------------------------------------------------------------------

		bool		on_window_event(const window_event& ev);
		void		on_window_resize(const vector2ui16& size);
		void		on_swapchain_flags(u8 flags);
		void		on_world_resolution(const vector2ui16& size);
		static void create_bind_layout_global();
		static void destroy_bind_layout_global();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline static gfx_id get_bind_layout_global()
		{
			return s_bind_layout_global;
		}

		inline static gfx_id get_bind_layout_global_compute()
		{
			return s_bind_layout_global_compute;
		}

		inline game_world_renderer* get_world_renderer() const
		{
			return _world_renderer;
		}

		inline u8 get_frame_index() const
		{
			return _gfx_data.frame_index;
		}

		inline texture_queue* get_texture_queue()
		{
			return &_texture_queue;
		}

		inline proxy_manager& get_proxy_manager()
		{
			return _proxy_manager;
		}

	private:
		struct buf_engine_global
		{
			f32 delta	= 0.0f;
			f32 elapsed = 0.0;
		};

		struct per_frame_data
		{
			buffer_gpu	   buf_engine_global = {};
			semaphore_data sem_frame		 = {};
			semaphore_data sem_copy			 = {};
			gfx_id		   cmd_gfx			 = 0;
			gfx_id		   cmd_copy			 = 0;
			gfx_id		   bind_group_global = 0;
		};

		struct gfx_data
		{
			gfx_id swapchain   = 0;
			u8	   frame_index = 0;
		};

		struct shader_data
		{
			gfx_id swapchain = 0;
		};

	private:
		world&				 _world;
		render_event_stream& _event_stream;
		window&				 _main_window;
		game_world_renderer* _world_renderer = nullptr;

#ifdef SFG_TOOLMODE
		editor* _editor = nullptr;
#endif

		vector<barrier> _reuse_upload_barriers = {};
		gfx_data		_gfx_data			   = {};
		shader_data		_shaders			   = {};
		per_frame_data	_pfd[BACK_BUFFER_COUNT];
		bump_allocator	_frame_allocator[BACK_BUFFER_COUNT] = {};
		buffer_queue	_buffer_queue						= {};
		texture_queue	_texture_queue						= {};
		proxy_manager	_proxy_manager;
		vector2ui16		_base_size		 = {};
		vector2ui16		_world_size		 = {};
		u8				_swapchain_flags = 0;

		static gfx_id s_bind_layout_global;
		static gfx_id s_bind_layout_global_compute;
		static gfx_id s_bind_group_global[BACK_BUFFER_COUNT];
	};
}
