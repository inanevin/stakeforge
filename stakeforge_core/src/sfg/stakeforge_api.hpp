// Copyright (c) 2025 Inan Evin
#pragma once

#include "stakeforge_api_common.hpp"

sfg_api_result_t sfg_engine_init(void);
sfg_api_result_t sfg_engine_uninit(void);
sfg_api_result_t sfg_engine_simulate(f32 delta_time);
sfg_api_result_t sfg_engine_render(void);

sfg_api_result_t sfg_world_create(sfg::world_handle_t* out_world);
sfg_api_result_t sfg_world_destroy(sfg::world_handle_t world);
