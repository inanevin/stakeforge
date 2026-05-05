// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_engine.hpp"
#include <sfg/runtime/render/renderer.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	class engine_runtime_t
	{
	public:
		static void init_globals();
		static void uninit_globals();
		static bool init_backend();
		static void uninit_backend();

		bool init();
		void uninit();
		void simulate(f32 delta_time);
		void render();

		world_handle_t create_world();
		bool		   destroy_world(world_handle_t handle);
		bool		   is_world_valid(world_handle_t handle) const;

		inline renderer_t& get_renderer()
		{
			return _renderer;
		}

		inline const renderer_t& get_renderer() const
		{
			return _renderer;
		}

	private:
		dynamic_gen_pool_t<world_t, u32, world_handle_tag> _worlds;
		renderer_t										   _renderer;
	};
}
