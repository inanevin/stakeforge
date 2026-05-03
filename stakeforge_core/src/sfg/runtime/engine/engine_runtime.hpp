// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_engine.hpp"
#include "engine_config.hpp"
#include <sfg/runtime/render/renderer.hpp>
#include <sfg/data/atomic.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/world/world.hpp>

#include <thread>

namespace sfg
{
	class engine_runtime_t
	{
	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		static void init_globals();
		static void uninit_globals();
		static bool init_backend();
		static void uninit_backend();
		bool		init(const engine_config_t& config);
		void		uninit();
		void		tick();

		// -----------------------------------------------------------------------------
		// world
		// -----------------------------------------------------------------------------

		world_handle_t create_world();
		bool		   destroy_world(world_handle_t handle);
		bool		   is_world_valid(world_handle_t handle) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline renderer_t& get_renderer()
		{
			return _renderer;
		}

		inline const renderer_t& get_renderer() const
		{
			return _renderer;
		}

		inline resource_manager_t& get_resource_manager()
		{
			return _resource_manager;
		}

		inline const resource_manager_t& get_resource_manager() const
		{
			return _resource_manager;
		}

	private:
		void ensure_render_thread();
		void end_render();
		void render();
		void tick_worlds(f32 delta_time);

	private:
		dynamic_gen_pool_t<world_t, u32, world_handle_tag> _worlds;
		renderer_t										   _renderer;
		resource_manager_t								   _resource_manager;
		engine_config_t									   _config = {};
		std::thread										   _render_thread;
		i64												   _previous_time		 = 0;
		i64												   _accumulator_ns		 = 0;
		i64												   _start_time			 = 0;
		i64												   _fps_main_time		 = 0;
		i64												   _fps_render_time		 = 0;
		u32												   _fps_main_frames		 = 0;
		u32												   _fps_render_frames	 = 0;
		atomic_t<bool>									   _render_thread_active = false;
	};
}
