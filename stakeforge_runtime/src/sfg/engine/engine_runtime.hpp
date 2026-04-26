// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_engine.hpp"
#include "engine_config.hpp"
#include "render/renderer.hpp"
#include "data/atomic.hpp"
#include "memory/dynamic_pool_allocator_gen.hpp"
#include "world/world.hpp"

#include <thread>

namespace sfg
{
	class engine_runtime_t
	{
	public:
		engine_runtime_error_code init(const engine_config_t& config);
		void					  uninit();
		void					  tick();
		world_handle_t			  create_world();
		bool					  destroy_world(world_handle_t handle);
		bool					  is_world_valid(world_handle_t handle) const;

		inline renderer_t& get_renderer()
		{
			return _renderer;
		}

		inline const renderer_t& get_renderer() const
		{
			return _renderer;
		}

	private:
		void ensure_render_thread();
		void end_render();
		void render();
		void tick_worlds(f32 delta_time);

	private:
		dynamic_pool_allocator_gen_t<world_t, u32, world_handle_tag> _worlds;
		renderer_t													 _renderer;
		std::thread													 _render_thread;
		engine_config_t												 _config			   = {};
		i64															 _previous_time		   = 0;
		i64															 _accumulator_ns	   = 0;
		i64															 _start_time		   = 0;
		i64															 _fps_main_time		   = 0;
		i64															 _fps_render_time	   = 0;
		u32															 _fps_main_frames	   = 0;
		u32															 _fps_render_frames	   = 0;
		atomic_t<bool>												 _is_init			   = false;
		atomic_t<bool>												 _render_thread_active = false;
	};
}
