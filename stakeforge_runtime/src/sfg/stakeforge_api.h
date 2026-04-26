// Copyright (c) 2025 Inan Evin
#pragma once

#include "stakeforge_api_common.h"

sfg_api_result_t sfg_engine_init(const sfg::engine_config_t& config);
sfg_api_result_t sfg_engine_uninit(void);
sfg_api_result_t sfg_engine_frame(void);

sfg_api_result_t sfg_world_create(sfg::world_handle_t* out_world);
sfg_api_result_t sfg_world_destroy(sfg::world_handle_t world);
