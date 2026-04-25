// Copyright (c) 2025 Inan Evin
#pragma once

#include "engine/engine_config.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/semaphore_data.hpp"

namespace sfg
{
	class renderer_t
	{
	private:
		struct per_frame_data_t
		{
			semaphore_data_t semaphore_frame = {};
		};

	public:
		engine_runtime_error_code init();
		void					  uninit();
		void					  join();
		void					  render();

		inline gfx_bind_layout_handle get_global_bind_layout() const
		{
			return _global_bind_layout;
		}

		inline gfx_bind_layout_handle get_global_compute_bind_layout() const
		{
			return _global_compute_bind_layout;
		}

		inline u8 get_frame_index() const
		{
			return _frame_index;
		}

	private:
		per_frame_data_t	   _pfd[BACK_BUFFER_COUNT];
		gfx_bind_layout_handle _global_bind_layout		   = {};
		gfx_bind_layout_handle _global_compute_bind_layout = {};
		u64					   _frame_counter			   = 0;
		u8					   _frame_index				   = 0;
	};
}
