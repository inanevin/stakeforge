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
#include "engine_config.hpp"
#include "engine_window.hpp"
#include "surface.hpp"
#include "renderer.hpp"
#include "virtual_fs.hpp"
#include "data/atomic.hpp"
#include "memory/pool_allocator_gen.hpp"
#include "memory/dynamic_pool_allocator_gen.hpp"

#include <thread>

namespace sfg
{
	class world_t;

	class engine_t
	{
	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		engine_error_code init(const engine_config_t& config, const virtual_fs_config_t& file_system_config);
		engine_error_code init();
		void			  uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick();

		// -----------------------------------------------------------------------------
		// window
		// -----------------------------------------------------------------------------

		window_handle_t			   create_window(const window_descriptor_t& descriptor);
		void					   destroy_window(window_handle_t handle);
		bool					   is_window_valid(window_handle_t handle) const;
		window_descriptor_t&	   get_window_descriptor(window_handle_t handle);
		const window_descriptor_t& get_window_descriptor(window_handle_t handle) const;
		const window_runtime_t&	   get_window_runtime(window_handle_t handle) const;

		// -----------------------------------------------------------------------------
		// surface
		// -----------------------------------------------------------------------------

		surface_handle_t			create_surface(const surface_descriptor_t& descriptor);
		void						destroy_surface(surface_handle_t handle);
		bool						is_surface_valid(surface_handle_t handle) const;
		surface_descriptor_t&		get_surface_descriptor(surface_handle_t handle);
		const surface_descriptor_t& get_surface_descriptor(surface_handle_t handle) const;
		const surface_runtime_t&	get_surface_runtime(surface_handle_t handle) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline renderer_t& get_renderer()
		{
			return _renderer;
		}

		inline virtual_fs_t& get_vfs()
		{
			return _vfs;
		}

		inline const virtual_fs_t& get_vfs() const
		{
			return _vfs;
		}

	private:
		void ensure_render_thread();
		void end_render();
		void render();

		void		create_surface_render_target(surface_t& surface, const vec2u16_t& size);
		void		destroy_surface_render_target(surface_t& surface);
		static void on_window_event(void* handle, const window_event_t& ev, void* user_data);

	private:
		pool_allocator_gen_t<engine_window_t, engine_id_t, renderer_t::MAX_SWAPCHAINS, engine_window_pool_tag> _windows;
		dynamic_pool_allocator_gen_t<surface_t, engine_id_t, surface_tag>									   _surfaces;
		std::thread																							   _render_thread;
		renderer_t																							   _renderer;
		virtual_fs_t																						   _vfs;
		atomic_t<bool>																						   _is_init				 = false;
		atomic_t<bool>																						   _render_thread_active = false;
		i64																									   _previous_time		 = 0;
		i64																									   _accumulator_ns		 = 0;
		i64																									   _start_time			 = 0;
		i64																									   _fps_main_time		 = 0;
		i64																									   _fps_render_time		 = 0;
		u32																									   _fps_main_frames		 = 0;
		u32																									   _fps_render_frames	 = 0;
	};

}
