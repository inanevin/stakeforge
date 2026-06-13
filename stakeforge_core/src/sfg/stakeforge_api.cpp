// Copyright (c) 2025 Inan Evin

#include "stakeforge_api.hpp"
#include <sfg/runtime/engine/engine_runtime.hpp>

#include <new>

namespace
{
	sfg::engine_runtime_t* g_engine = nullptr;
}

sfg_api_result_t sfg_engine_init(void)
{
	if (g_engine != nullptr)
		return sfg_api_result_engine_already_initialized;

	sfg::engine_runtime_t* engine = new (std::nothrow) sfg::engine_runtime_t();
	if (engine == nullptr)
		return sfg_api_result_engine_init_failed;

	if (!engine->init())
	{
		delete engine;
		return sfg_api_result_engine_init_failed;
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
