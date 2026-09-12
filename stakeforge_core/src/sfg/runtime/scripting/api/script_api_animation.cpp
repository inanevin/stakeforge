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

#include "script_api_animation.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	u8 api_animation_get_slot_pos_abs(const world_t* world, entity_id_t entity, sid_t slot_name_hash, vec3f_t* out_position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_position != nullptr);

		*out_position = vec3f_t::zero;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().get_slot_pos_abs(entity, slot_name_hash, *out_position) ? 1 : 0;
	}

	u8 api_animation_get_slot_rot_abs(const world_t* world, entity_id_t entity, sid_t slot_name_hash, quat_t* out_rotation)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_rotation != nullptr);

		*out_rotation = quat_t::identity;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().get_slot_rot_abs(entity, slot_name_hash, *out_rotation) ? 1 : 0;
	}

	u8 api_animation_set_graph_parameter_f32(world_t* world, entity_id_t entity, sid_t parameter_hash, f32 value)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().set_graph_parameter_f32(entity, parameter_hash, value) ? 1 : 0;
	}

	u8 api_animation_set_graph_parameter_vec2(world_t* world, entity_id_t entity, sid_t parameter_hash, const vec2f_t* value)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(value != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().set_graph_parameter_vec2(entity, parameter_hash, *value) ? 1 : 0;
	}

	u8 api_animation_set_graph_parameter_vec3(world_t* world, entity_id_t entity, sid_t parameter_hash, const vec3f_t* value)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(value != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().set_graph_parameter_vec3(entity, parameter_hash, *value) ? 1 : 0;
	}

	u8 api_animation_set_graph_parameter_quat(world_t* world, entity_id_t entity, sid_t parameter_hash, const quat_t* value)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(value != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().set_graph_parameter_quat(entity, parameter_hash, *value) ? 1 : 0;
	}

	u8 api_animation_set_graph_parameter_bool(world_t* world, entity_id_t entity, sid_t parameter_hash, u8 value)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().set_graph_parameter_bool(entity, parameter_hash, value != 0) ? 1 : 0;
	}

	u8 api_animation_get_graph_parameter_f32(const world_t* world, entity_id_t entity, sid_t parameter_hash, f32* out_value)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_value != nullptr);

		*out_value = 0.0f;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().get_graph_parameter_f32(entity, parameter_hash, *out_value) ? 1 : 0;
	}

	u8 api_animation_get_graph_parameter_vec2(const world_t* world, entity_id_t entity, sid_t parameter_hash, vec2f_t* out_value)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_value != nullptr);

		*out_value = vec2f_t::zero;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().get_graph_parameter_vec2(entity, parameter_hash, *out_value) ? 1 : 0;
	}

	u8 api_animation_get_graph_parameter_vec3(const world_t* world, entity_id_t entity, sid_t parameter_hash, vec3f_t* out_value)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_value != nullptr);

		*out_value = vec3f_t::zero;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().get_graph_parameter_vec3(entity, parameter_hash, *out_value) ? 1 : 0;
	}

	u8 api_animation_get_graph_parameter_quat(const world_t* world, entity_id_t entity, sid_t parameter_hash, quat_t* out_value)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_value != nullptr);

		*out_value = quat_t::identity;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		return world->get_animation_controller().get_graph_parameter_quat(entity, parameter_hash, *out_value) ? 1 : 0;
	}

	u8 api_animation_get_graph_parameter_bool(const world_t* world, entity_id_t entity, sid_t parameter_hash, u8* out_value)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_value != nullptr);

		bool value = false;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
		{
			*out_value = 0;
			return 0;
		}

		const bool result = world->get_animation_controller().get_graph_parameter_bool(entity, parameter_hash, value);
		*out_value		  = value ? 1 : 0;
		return result ? 1 : 0;
	}

	const script_api_animation_t& get_script_api_animation()
	{
		static const script_api_animation_t api{
			.size					  = static_cast<u32>(sizeof(script_api_animation_t)),
			.version				  = 1,
			.get_slot_pos_abs		  = api_animation_get_slot_pos_abs,
			.get_slot_rot_abs		  = api_animation_get_slot_rot_abs,
			.set_graph_parameter_f32  = api_animation_set_graph_parameter_f32,
			.set_graph_parameter_vec2 = api_animation_set_graph_parameter_vec2,
			.set_graph_parameter_vec3 = api_animation_set_graph_parameter_vec3,
			.set_graph_parameter_quat = api_animation_set_graph_parameter_quat,
			.set_graph_parameter_bool = api_animation_set_graph_parameter_bool,
			.get_graph_parameter_f32  = api_animation_get_graph_parameter_f32,
			.get_graph_parameter_vec2 = api_animation_get_graph_parameter_vec2,
			.get_graph_parameter_vec3 = api_animation_get_graph_parameter_vec3,
			.get_graph_parameter_quat = api_animation_get_graph_parameter_quat,
			.get_graph_parameter_bool = api_animation_get_graph_parameter_bool,
		};

		return api;
	}
}
