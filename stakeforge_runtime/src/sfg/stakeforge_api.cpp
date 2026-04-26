// Copyright (c) 2025 Inan Evin

#include "stakeforge_api.hpp"
#include "engine/engine_runtime.hpp"

#include <new>

namespace
{
	sfg::engine_runtime_t* g_engine = nullptr;

	sfg_api_result_t to_api_result(sfg::engine_runtime_error_code result)
	{
		switch (result)
		{
		case sfg::engine_runtime_error_code::none:
			return sfg_api_result_success;
		case sfg::engine_runtime_error_code::renderer_already_init:
			return sfg_api_result_renderer_already_initialized;
		case sfg::engine_runtime_error_code::backend_failed:
			return sfg_api_result_backend_failed;
		default:
			break;
		}

		return sfg_api_result_engine_init_failed;
	}
}

sfg_api_result_t sfg_engine_init(const sfg::engine_config_t& config)
{
	if (g_engine != nullptr)
		return sfg_api_result_engine_already_initialized;

	if ((config.fixed_framerate_ns <= 0.0 || config.fixed_framerate_max_ticks == 0 || config.frame_allocator_size == 0 || config.resource_allocator_size == 0))
		return sfg_api_result_invalid_argument;

	sfg::engine_runtime_t* engine = new (std::nothrow) sfg::engine_runtime_t();
	if (engine == nullptr)
		return sfg_api_result_engine_init_failed;

	const sfg::engine_runtime_error_code result = engine->init(config);
	if (result != sfg::engine_runtime_error_code::none)
	{
		delete engine;
		return to_api_result(result);
	}

	g_engine = engine;
	return sfg_api_result_success;
}

sfg_api_result_t sfg_engine_uninit(void)
{
	if (g_engine == nullptr)
		return sfg_api_result_engine_not_initialized;

	g_engine->uninit();
	delete g_engine;
	g_engine = nullptr;
	return sfg_api_result_success;
}

sfg_api_result_t sfg_engine_frame(void)
{
	if (g_engine == nullptr)
		return sfg_api_result_engine_not_initialized;

	g_engine->tick();
	return sfg_api_result_success;
}

sfg_api_result_t sfg_world_create(sfg::world_handle_t* out_world)
{
	if (g_engine == nullptr)
		return sfg_api_result_engine_not_initialized;

	if (out_world == nullptr)
		return sfg_api_result_invalid_argument;

	*out_world = g_engine->create_world();
	return sfg_api_result_success;
}

sfg_api_result_t sfg_world_destroy(sfg::world_handle_t world)
{
	if (g_engine == nullptr)
		return sfg_api_result_engine_not_initialized;

	if (!g_engine->destroy_world(world))
		return sfg_api_result_invalid_world_handle;

	return sfg_api_result_success;
}
