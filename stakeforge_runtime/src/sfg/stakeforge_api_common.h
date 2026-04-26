// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "engine/common_engine.hpp"
#include "engine/engine_config.hpp"

typedef enum sfg_api_result_t
{
	sfg_api_result_success = 0,
	sfg_api_result_invalid_argument,
	sfg_api_result_engine_already_initialized,
	sfg_api_result_engine_not_initialized,
	sfg_api_result_engine_init_failed,
	sfg_api_result_renderer_already_initialized,
	sfg_api_result_backend_failed,
	sfg_api_result_invalid_world_handle,
} sfg_api_result_t;