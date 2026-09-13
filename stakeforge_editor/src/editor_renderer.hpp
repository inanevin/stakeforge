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
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/gfx/common/semaphore_data.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/ui/ui_renderer.hpp>

#include <thread>

namespace sfg
{
	namespace ui
	{
		class ui_context;
	}

	class editor_world_controller_t;

	struct editor_renderer_config_t
	{
		size_t frame_budget_bytes		= 4ull * 1024ull * 1024ull;
		u32	   surface_initial_capacity = 8;
	};

	class editor_renderer_t final
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
			u32				 global_index		= 0;
		};

		struct surface_render_target_t
		{
			gfx_handle_t	   swapchain   = {};
			ui::ui_context*	   ui		   = nullptr;
			ui::ui_renderer_t* ui_renderer = nullptr;
			vec2u16_t		   size		   = {};
			bool			   minimized   = false;
			bool			   visible	   = true;
		};

	public:
		editor_renderer_t()									   = default;
		~editor_renderer_t()								   = default;
		editor_renderer_t(const editor_renderer_t&)			   = delete;
		editor_renderer_t& operator=(const editor_renderer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init(const editor_renderer_config_t& config = {});
		void uninit();
		void render(editor_world_controller_t& world_controller);
		void join();

		// -----------------------------------------------------------------------------
		// threading
		// -----------------------------------------------------------------------------

		void ensure_render(editor_world_controller_t& world_controller);
		void end_render();

		// -----------------------------------------------------------------------------
		// swapchain
		// -----------------------------------------------------------------------------

		gfx_handle_t create_swapchain(void* window_handle, void* platform_handle, f32 dpi_scale, vec2u16_t size, ui::ui_context* ui);
		void		 resize_swapchain(gfx_handle_t swapchain, vec2u16_t size, f32 dpi_scale);
		void		 destroy_swapchain(gfx_handle_t swapchain);
		void		 set_swapchain_minimized(gfx_handle_t handle, bool is_minimized);
		void		 set_swapchain_visible(gfx_handle_t handle, bool visible);

	private:
		void render_loop();

		per_frame_data_t				  _pfd[BACK_BUFFER_COUNT] = {};
		vector_t<surface_render_target_t> _render_targets;
		gfx_handle_t					  _shader_ui_default  = {};
		gfx_handle_t					  _shader_ui_text	  = {};
		gfx_handle_t					  _shader_ui_sdf	  = {};
		u64								  _frame_counter	  = 0;
		size_t							  _frame_budget_bytes = 0;
		u8								  _frame_index		  = 0;
		editor_world_controller_t*		  _world_controller	  = nullptr;
		std::thread						  _render_thread;
		atomic_t<bool>					  _render_thread_active = false;
	};
}
