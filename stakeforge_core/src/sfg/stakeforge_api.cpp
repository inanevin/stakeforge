// Copyright (c) 2025 Inan Evin

#include "stakeforge_api.hpp"
#include <sfg/runtime/engine/engine_runtime.hpp>

namespace
{
	bool g_engine_initialized = false;
}

sfg_api_result_t sfg_engine_init(void)
{
	if (g_engine_initialized)
		return sfg_api_result_engine_already_initialized;

	if (!sfg::engine_runtime_t::get().init())
		return sfg_api_result_engine_init_failed;

	g_engine_initialized = true;
	return sfg_api_result_success;
}

sfg_api_result_t sfg_engine_uninit(void)
{
	if (!g_engine_initialized)
		return sfg_api_result_engine_not_initialized;

	sfg::engine_runtime_t::get().uninit();
	g_engine_initialized = false;
	return sfg_api_result_success;
}
