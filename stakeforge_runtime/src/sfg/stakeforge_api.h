// Copyright (c) 2025 Inan Evin
#pragma once

#include "stakeforge_api_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

	sfg_api_result_t sfg_engine_init(const engine_config_t* config);
	sfg_api_result_t sfg_engine_uninit(void);
	sfg_api_result_t sfg_engine_frame(void);

	sfg_api_result_t sfg_world_create(world_handle_t* out_world);
	sfg_api_result_t sfg_world_destroy(world_handle_t world);

#ifdef __cplusplus
}
#endif
