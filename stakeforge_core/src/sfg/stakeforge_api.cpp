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
