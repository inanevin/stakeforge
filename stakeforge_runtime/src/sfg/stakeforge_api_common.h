// Copyright (c) 2025 Inan Evin
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

	typedef struct engine_config_t
	{
		double	 fixed_framerate_ns;
		uint32_t fixed_framerate_max_ticks;
		uint64_t frame_allocator_size;
	} engine_config_t;

	typedef struct world_handle_t
	{
		uint32_t generation;
		uint32_t index;
	} world_handle_t;

#ifdef __cplusplus
}
#endif
