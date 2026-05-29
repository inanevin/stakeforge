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

#include <sfg/data/atomic.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/unique.hpp>
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

	class editor_renderer_t
	{
	private:
		struct per_frame_data_t
		{
			semaphore_data_t		  semaphore_frame	 = {};
			semaphore_data_t		  semaphore_transfer = {};
			semaphore_data_t		  semaphore_world	 = {};
			gfx_command_buffer_handle cmd_gfx			 = {};
			gfx_command_buffer_handle cmd_gfx_prepare	 = {};
			gfx_command_buffer_handle cmd_transfer		 = {};
			gfx_resource_handle		  global_buffer		 = {};
			u8*						  mapped_global		 = nullptr;
			u32						  global_index		 = 0;
		};

		struct surface_render_target_t
		{
			gfx_swapchain_handle		swapchain = {};
			ui::ui_context*				ui		  = nullptr;
			unique_t<ui::ui_renderer_t> ui_renderer;
			vec2u16_t					size	  = {};
			bool						minimized = false;
			bool						visible	  = true;
		};

	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init();
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

		gfx_swapchain_handle create_swapchain(void* window_handle, void* platform_handle, f32 dpi_scale, vec2u16_t size, ui::ui_context* ui);
		void				 resize_swapchain(gfx_swapchain_handle swapchain, vec2u16_t size, f32 dpi_scale);
		void				 destroy_swapchain(gfx_swapchain_handle swapchain);
		void				 set_swapchain_minimized(gfx_swapchain_handle handle, bool is_minimized);
		void				 set_swapchain_visible(gfx_swapchain_handle handle, bool visible);

	private:
		void render_loop();

		static constexpr size_t			  RENDER_FRAME_ALLOC_SIZE = 1024ull * 1024ull * 4ull;
		per_frame_data_t				  _pfd[BACK_BUFFER_COUNT] = {};
		vector_t<surface_render_target_t> _render_targets;
		gfx_shader_handle				  _shader_ui_default = {};
		gfx_shader_handle				  _shader_ui_text	 = {};
		gfx_shader_handle				  _shader_ui_sdf	 = {};
		u64								  _frame_counter	 = 0;
		u8								  _frame_index		 = 0;
		editor_world_controller_t*		  _world_controller	 = nullptr;
		std::thread						  _render_thread;
		atomic_t<bool>					  _render_thread_active = false;
	};
}
