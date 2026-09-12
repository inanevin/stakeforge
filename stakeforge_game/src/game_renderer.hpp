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

#include <sfg/data/atomic.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/semaphore_data.hpp>
#include <sfg/math/vec2u16.hpp>

#include <thread>

namespace sfg
{
	class game_world_controller_t;
	struct window_runtime_t;

	struct game_renderer_config_t
	{
		size_t frame_budget_bytes = 4ull * 1024ull * 1024ull;
		bool   is_fullscreen	  = false;
	};

	class game_renderer_t final
	{
	private:
		struct per_frame_data_t
		{
			semaphore_data_t semaphore_frame	= {};
			semaphore_data_t semaphore_transfer = {};
			semaphore_data_t semaphore_world	= {};
			gfx_handle_t	 cmd_gfx			= {};
			gfx_handle_t	 cmd_gfx_prepare	= {};
			gfx_handle_t	 cmd_gfx_transit	= {};
			gfx_handle_t	 cmd_transfer		= {};
			gfx_handle_t	 global_buffer		= {};
			u8*				 mapped_global		= nullptr;
			gpu_index_t		 global_index		= NULL_GPU_INDEX;
		};

	public:
		game_renderer_t()								   = default;
		~game_renderer_t()								   = default;
		game_renderer_t(const game_renderer_t&)			   = delete;
		game_renderer_t& operator=(const game_renderer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(window_runtime_t& window, game_world_controller_t& world_controller, const game_renderer_config_t& config = {});
		void uninit();
		void start();
		void end_render();
		void join();

		// -----------------------------------------------------------------------------
		// swapchain
		// -----------------------------------------------------------------------------

		void resize(vec2u16_t size, f32 dpi_scale, bool minimized);

	private:
		void render();
		void render_loop();

		per_frame_data_t		 _pfd[BACK_BUFFER_COUNT] = {};
		std::thread				 _render_thread;
		game_world_controller_t* _world_controller	   = nullptr;
		gfx_handle_t			 _swapchain			   = {};
		gfx_handle_t			 _blit_shader		   = {};
		vec2u16_t				 _size				   = vec2u16_t::zero;
		size_t					 _frame_budget_bytes   = 0;
		i64						 _previous_time_us	   = 0;
		u64						 _frame_counter		   = 0;
		f32						 _elapsed_time		   = 0.0f;
		atomic_t<bool>			 _render_thread_active = false;
		bool					 _is_fullscreen		   = false;
		bool					 _minimized			   = false;
	};
}
