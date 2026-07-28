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

#include "script_api_world.hpp"

namespace sfg
{
	entity_id_t api_world_create_entity(world_t* world, const char* name)
	{
		return NULL_ENTITY_ID;
	}

	u8 api_world_destroy_entity(world_t* world, entity_id_t entity)
	{
		return 0;
	}

	entity_id_t api_world_duplicate_entity(world_t* world, entity_id_t entity)
	{
		return NULL_ENTITY_ID;
	}

	u8 api_world_attach_entity(world_t* world, entity_id_t entity, entity_id_t parent)
	{
		return 0;
	}

	u8 api_world_detach_entity(world_t* world, entity_id_t entity)
	{
		return 0;
	}

	u8 api_world_set_entity_pos_local(world_t* world, entity_id_t entity, const vec3f_t* position)
	{
		return 0;
	}

	u8 api_world_set_entity_rot_local(world_t* world, entity_id_t entity, const quat_t* rotation)
	{
		return 0;
	}

	u8 api_world_set_entity_scale_local(world_t* world, entity_id_t entity, const vec3f_t* scale)
	{
		return 0;
	}

	u8 api_world_teleport_entity(world_t* world, entity_id_t entity, const vec3f_t* position, const quat_t* rotation, const vec3f_t* scale)
	{
		return 0;
	}

	u8 api_world_mark_entity_teleported(world_t* world, entity_id_t entity)
	{
		return 0;
	}

	u8 api_world_get_entity_pos_local(const world_t* world, entity_id_t entity, vec3f_t* out_position)
	{
		return 0;
	}

	u8 api_world_get_entity_rot_local(const world_t* world, entity_id_t entity, quat_t* out_rotation)
	{
		return 0;
	}

	u8 api_world_get_entity_scale_local(const world_t* world, entity_id_t entity, vec3f_t* out_scale)
	{
		return 0;
	}

	u8 api_world_get_entity_pos_last_abs(const world_t* world, entity_id_t entity, vec3f_t* out_position)
	{
		return 0;
	}

	u8 api_world_get_entity_rot_last_abs(const world_t* world, entity_id_t entity, quat_t* out_rotation)
	{
		return 0;
	}

	u8 api_world_get_entity_scale_last_abs(const world_t* world, entity_id_t entity, vec3f_t* out_scale)
	{
		return 0;
	}

	u8 api_world_abs_pos_to_local(world_t* world, entity_id_t entity, const vec3f_t* position, vec3f_t* out_position)
	{
		return 0;
	}

	u8 api_world_abs_rot_to_local(world_t* world, entity_id_t entity, const quat_t* rotation, quat_t* out_rotation)
	{
		return 0;
	}

	u8 api_world_abs_scale_to_local(world_t* world, entity_id_t entity, const vec3f_t* scale, vec3f_t* out_scale)
	{
		return 0;
	}

	entity_id_t api_world_get_entity_with_name(const world_t* world, const char* name)
	{
		return NULL_ENTITY_ID;
	}

	u32 api_world_get_all_entities_with_name(const world_t* world, const char* name, entity_id_t* out_entities, u32 capacity)
	{
		return 0;
	}

	entity_id_t api_world_get_entity_with_component(const world_t* world, sid_t component_type)
	{
		return NULL_ENTITY_ID;
	}

	u32 api_world_get_all_entities_with_component(const world_t* world, sid_t component_type, entity_id_t* out_entities, u32 capacity)
	{
		return 0;
	}

	u8 api_world_is_alive(const world_t* world, entity_id_t entity)
	{
		return 0;
	}

	u8 api_world_has_component(const world_t* world, entity_id_t entity, sid_t component_type)
	{
		return 0;
	}

	u8 api_world_get_component(const world_t* world, entity_id_t entity, sid_t component_type, void* out_component, u32 component_size)
	{
		return 0;
	}

	u8 api_world_add_component(world_t* world, entity_id_t entity, sid_t component_type, const void* component, u32 component_size)
	{
		return 0;
	}

	u8 api_world_set_component(world_t* world, entity_id_t entity, sid_t component_type, const void* component, u32 component_size)
	{
		return 0;
	}

	u8 api_world_remove_component(world_t* world, entity_id_t entity, sid_t component_type)
	{
		return 0;
	}

	entity_id_t api_world_get_entity_with_tag(const world_t* world, u64 tag)
	{
		return NULL_ENTITY_ID;
	}

	u32 api_world_get_all_entities_with_tag(const world_t* world, u64 tag, entity_id_t* out_entities, u32 capacity)
	{
		return 0;
	}

	u8 api_world_set_entity_tag(world_t* world, entity_id_t entity, u64 tag, u8 enabled)
	{
		return 0;
	}

	const script_api_world_t& get_script_api_world()
	{
		static const script_api_world_t api{
			.size							 = static_cast<u32>(sizeof(script_api_world_t)),
			.version						 = 1,
			.create_entity					 = api_world_create_entity,
			.destroy_entity					 = api_world_destroy_entity,
			.duplicate_entity				 = api_world_duplicate_entity,
			.attach_entity					 = api_world_attach_entity,
			.detach_entity					 = api_world_detach_entity,
			.set_entity_pos_local			 = api_world_set_entity_pos_local,
			.set_entity_rot_local			 = api_world_set_entity_rot_local,
			.set_entity_scale_local			 = api_world_set_entity_scale_local,
			.teleport_entity				 = api_world_teleport_entity,
			.mark_entity_teleported			 = api_world_mark_entity_teleported,
			.get_entity_pos_local			 = api_world_get_entity_pos_local,
			.get_entity_rot_local			 = api_world_get_entity_rot_local,
			.get_entity_scale_local			 = api_world_get_entity_scale_local,
			.get_entity_pos_last_abs		 = api_world_get_entity_pos_last_abs,
			.get_entity_rot_last_abs		 = api_world_get_entity_rot_last_abs,
			.get_entity_scale_last_abs		 = api_world_get_entity_scale_last_abs,
			.abs_pos_to_local				 = api_world_abs_pos_to_local,
			.abs_rot_to_local				 = api_world_abs_rot_to_local,
			.abs_scale_to_local				 = api_world_abs_scale_to_local,
			.get_entity_with_name			 = api_world_get_entity_with_name,
			.get_all_entities_with_name		 = api_world_get_all_entities_with_name,
			.get_entity_with_component		 = api_world_get_entity_with_component,
			.get_all_entities_with_component = api_world_get_all_entities_with_component,
			.is_alive						 = api_world_is_alive,
			.has_component					 = api_world_has_component,
			.get_component					 = api_world_get_component,
			.add_component					 = api_world_add_component,
			.set_component					 = api_world_set_component,
			.remove_component				 = api_world_remove_component,
			.get_entity_with_tag			 = api_world_get_entity_with_tag,
			.get_all_entities_with_tag		 = api_world_get_all_entities_with_tag,
			.set_entity_tag					 = api_world_set_entity_tag,
		};

		return api;
	}
}
