/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

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
