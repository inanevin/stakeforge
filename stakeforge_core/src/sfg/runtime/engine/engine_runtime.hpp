// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_engine.hpp"
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace ui
	{
		struct glyph_atlas_config_t;
	}

	class engine_runtime_t
	{
	public:
		static void init_globals(size_t resource_manager_memory = 64ull * 1024ull * 1024ull);
		static void uninit_globals();
		static bool init_backend(const ui::glyph_atlas_config_t& glyph_atlas_config);
		static void uninit_backend();

		bool init();
		void uninit();
		void simulate(f32 delta_time);
		void render();

		world_handle_t create_world();
		bool		   destroy_world(world_handle_t handle);
		bool		   is_world_valid(world_handle_t handle) const;
		world_t&	   get_world(world_handle_t handle);
		const world_t& get_world(world_handle_t handle) const;

	private:
		dynamic_gen_pool_t<world_t, u32, world_handle_tag> _worlds;
	};
}
