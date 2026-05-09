// Copyright (c) 2025 Inan Evin
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

	class editor_renderer_t
	{
	private:
		struct per_frame_data_t
		{
			semaphore_data_t		  semaphore_frame	  = {};
			semaphore_data_t		  semaphore_transfer  = {};
			gfx_command_buffer_handle cmd_gfx	  = {};
			gfx_command_buffer_handle cmd_transfer = {};
			gfx_resource_handle		  global_buffer		  = {};
			u8*						  mapped_global		  = nullptr;
			u32						  global_index		  = 0;
		};

		struct surface_render_target_t
		{
			gfx_swapchain_handle swapchain = {};
			ui::ui_context*		 ui		   = nullptr;
			vec2u16_t			 size	   = {};
			bool				 minimized = false;
		};

	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();
		void render();
		void join();

		// -----------------------------------------------------------------------------
		// threading
		// -----------------------------------------------------------------------------

		void ensure_render();
		void end_render();

		// -----------------------------------------------------------------------------
		// swapchain
		// -----------------------------------------------------------------------------

		gfx_swapchain_handle create_swapchain(void* window_handle, void* platform_handle, f32 dpi_scale, vec2u16_t size, ui::ui_context* ui);
		void				 resize_swapchain(gfx_swapchain_handle swapchain, vec2u16_t size, f32 dpi_scale);
		void				 destroy_swapchain(gfx_swapchain_handle swapchain);
		void				 set_swapchain_minimized(gfx_swapchain_handle handle, bool is_minimized);

	private:
		void render_loop();

		static constexpr size_t			  RENDER_FRAME_ALLOC_SIZE = 1024ull * 1024ull * 4ull;
		ui::ui_renderer_t				  _ui_renderer			  = {};
		per_frame_data_t				  _pfd[BACK_BUFFER_COUNT] = {};
		vector_t<surface_render_target_t> _render_targets;
		gfx_shader_handle				  _shader_ui_default = {};
		gfx_shader_handle				  _shader_ui_text	 = {};
		gfx_shader_handle				  _shader_ui_sdf	 = {};
		u64								  _frame_counter	 = 0;
		u8								  _frame_index		 = 0;
		std::thread						  _render_thread;
		atomic_t<bool>					  _render_thread_active = false;
	};
}
