/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class quat_t;
	struct vec2f_t;
	struct vec3f_t;
	class world_t;

	u8 api_animation_get_slot_pos_abs(const world_t* world, entity_id_t entity, sid_t slot_name_hash, vec3f_t* out_position);
	u8 api_animation_get_slot_rot_abs(const world_t* world, entity_id_t entity, sid_t slot_name_hash, quat_t* out_rotation);
	u8 api_animation_set_graph_parameter_f32(world_t* world, entity_id_t entity, sid_t parameter_hash, f32 value);
	u8 api_animation_set_graph_parameter_vec2(world_t* world, entity_id_t entity, sid_t parameter_hash, const vec2f_t* value);
	u8 api_animation_set_graph_parameter_vec3(world_t* world, entity_id_t entity, sid_t parameter_hash, const vec3f_t* value);
	u8 api_animation_set_graph_parameter_quat(world_t* world, entity_id_t entity, sid_t parameter_hash, const quat_t* value);
	u8 api_animation_set_graph_parameter_bool(world_t* world, entity_id_t entity, sid_t parameter_hash, u8 value);
	u8 api_animation_get_graph_parameter_f32(const world_t* world, entity_id_t entity, sid_t parameter_hash, f32* out_value);
	u8 api_animation_get_graph_parameter_vec2(const world_t* world, entity_id_t entity, sid_t parameter_hash, vec2f_t* out_value);
	u8 api_animation_get_graph_parameter_vec3(const world_t* world, entity_id_t entity, sid_t parameter_hash, vec3f_t* out_value);
	u8 api_animation_get_graph_parameter_quat(const world_t* world, entity_id_t entity, sid_t parameter_hash, quat_t* out_value);
	u8 api_animation_get_graph_parameter_bool(const world_t* world, entity_id_t entity, sid_t parameter_hash, u8* out_value);

	struct script_api_animation_t
	{
		u32												  size					   = 0;
		u32												  version				   = 0;
		decltype(&api_animation_get_slot_pos_abs)		  get_slot_pos_abs		   = nullptr;
		decltype(&api_animation_get_slot_rot_abs)		  get_slot_rot_abs		   = nullptr;
		decltype(&api_animation_set_graph_parameter_f32)  set_graph_parameter_f32  = nullptr;
		decltype(&api_animation_set_graph_parameter_vec2) set_graph_parameter_vec2 = nullptr;
		decltype(&api_animation_set_graph_parameter_vec3) set_graph_parameter_vec3 = nullptr;
		decltype(&api_animation_set_graph_parameter_quat) set_graph_parameter_quat = nullptr;
		decltype(&api_animation_set_graph_parameter_bool) set_graph_parameter_bool = nullptr;
		decltype(&api_animation_get_graph_parameter_f32)  get_graph_parameter_f32  = nullptr;
		decltype(&api_animation_get_graph_parameter_vec2) get_graph_parameter_vec2 = nullptr;
		decltype(&api_animation_get_graph_parameter_vec3) get_graph_parameter_vec3 = nullptr;
		decltype(&api_animation_get_graph_parameter_quat) get_graph_parameter_quat = nullptr;
		decltype(&api_animation_get_graph_parameter_bool) get_graph_parameter_bool = nullptr;
	};

	const script_api_animation_t& get_script_api_animation();
}
