// Copyright (c) 2025 Inan Evin
#pragma once

#include "data/span.hpp"
#include "data/vector.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"
#include "math/vec2u16.hpp"
#include "ui/ui_renderer.hpp"

namespace sfg
{
	namespace ui
	{
		class vg_canvas_t;
	}

	struct surface_render_target_t
	{
		gfx_swapchain_handle swapchain = {};
		ui::vg_canvas_t*	 canvas	   = nullptr;
		vec2u16_t			 size	   = {};
	};

	class editor_renderer_t
	{
	private:
		struct per_frame_data_t
		{
			semaphore_data_t		  semaphore_frame = {};
			gfx_command_buffer_handle command_buffer  = {};
		};

	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();
		void render(span_t<const surface_render_target_t> targets);
		void join();

		// -----------------------------------------------------------------------------
		// swapchain
		// -----------------------------------------------------------------------------

		gfx_swapchain_handle create_swapchain(void* window_handle, void* platform_handle, f32 dpi_scale, vec2u16_t size);
		void				 resize_swapchain(gfx_swapchain_handle swapchain, vec2u16_t size, f32 dpi_scale);
		void				 destroy_swapchain(gfx_swapchain_handle swapchain);

	private:
		vector_t<gfx_swapchain_handle>	  _swapchains;
		vector_t<surface_render_target_t> _eligible_targets;
		per_frame_data_t				  _pfd[BACK_BUFFER_COUNT] = {};
		ui::ui_renderer_t				  _ui_renderer			  = {};
		gfx_bind_layout_handle			  _ui_default_layout	  = {};
		gfx_bind_layout_handle			  _ui_text_layout		  = {};
		gfx_bind_layout_handle			  _ui_sdf_layout		  = {};
		u64								  _frame_counter		  = 0;
		u8								  _frame_index			  = 0;
	};
}
