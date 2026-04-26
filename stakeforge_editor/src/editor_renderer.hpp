// Copyright (c) 2025 Inan Evin
#pragma once

#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"
#include "data/vector.hpp"

namespace sfg
{
	struct vec2u16_t;

	class editor_renderer_t
	{
	private:
		struct per_frame_data_t
		{
			semaphore_data_t		  semaphore_frame = {};
			gfx_command_buffer_handle command_buffer  = {};
		};

	public:
		bool init();
		void uninit();
		void render();
		void join();

		gfx_swapchain_handle create_swapchain(void* window_handle, void* platform_handle, f32 dpi_scale, vec2u16_t size);
		void				 resize_swapchain(gfx_swapchain_handle swapchain, vec2u16_t size, f32 dpi_scale);
		void				 destroy_swapchain(gfx_swapchain_handle swapchain);

	private:
		vector_t<gfx_swapchain_handle> _swapchains;
		vector_t<gfx_swapchain_handle> _eligible_swapchains;
		per_frame_data_t			   _pfd[BACK_BUFFER_COUNT] = {};
		u64							   _frame_counter		   = 0;
		u8							   _frame_index			   = 0;
	};
}
