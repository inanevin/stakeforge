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

#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class quat_t;
	struct vec3f_t;
	class world_t;

	static inline constexpr u32 SCRIPT_WORLD_QUERY_MAX_COMPONENTS	 = ECS_INNER_JOIN_MAX_TABLES;
	static inline constexpr u32 SCRIPT_WORLD_QUERY_STORAGE_U64_COUNT = 96;

	enum script_world_query_component_flags_e : u8
	{
		script_world_query_component_required = 0,
		script_world_query_component_excluded = ecs_component_table_flags_excluded,
		script_world_query_component_optional = ecs_component_table_flags_optional,
	};

	struct script_world_query_component_t
	{
		sid_t type_id	  = 0;
		u32	  size		  = 0;
		u8	  flags		  = script_world_query_component_required;
		u8	  reserved[3] = {};
	};

	struct script_world_query_t
	{
		u64 storage[SCRIPT_WORLD_QUERY_STORAGE_U64_COUNT] = {};
	};

	struct script_world_query_row_t
	{
		void*		components[SCRIPT_WORLD_QUERY_MAX_COMPONENTS]		  = {};
		sid_t		component_type_ids[SCRIPT_WORLD_QUERY_MAX_COMPONENTS] = {};
		entity_id_t entity												  = NULL_ENTITY_ID;
		u32			component_count										  = 0;
		u32			component_presence_mask								  = 0;
		u32			reserved											  = 0;
	};

	static_assert(sizeof(script_world_query_component_t) == 16);
	static_assert(sizeof(script_world_query_t) == 768);
	static_assert(sizeof(script_world_query_row_t) == 272);

	entity_id_t api_world_create_entity(world_t* world, const char* name);
	u8			api_world_destroy_entity(world_t* world, entity_id_t entity);
	entity_id_t api_world_duplicate_entity(world_t* world, entity_id_t entity);
	u8			api_world_attach_entity(world_t* world, entity_id_t entity, entity_id_t parent);
	u8			api_world_detach_entity(world_t* world, entity_id_t entity);
	u8			api_world_set_entity_pos_local(world_t* world, entity_id_t entity, const vec3f_t* position);
	u8			api_world_set_entity_rot_local(world_t* world, entity_id_t entity, const quat_t* rotation);
	u8			api_world_set_entity_scale_local(world_t* world, entity_id_t entity, const vec3f_t* scale);
	u8			api_world_teleport_entity(world_t* world, entity_id_t entity, const vec3f_t* position, const quat_t* rotation, const vec3f_t* scale);
	u8			api_world_mark_entity_teleported(world_t* world, entity_id_t entity);
	u8			api_world_get_entity_pos_local(const world_t* world, entity_id_t entity, vec3f_t* out_position);
	u8			api_world_get_entity_rot_local(const world_t* world, entity_id_t entity, quat_t* out_rotation);
	u8			api_world_get_entity_scale_local(const world_t* world, entity_id_t entity, vec3f_t* out_scale);
	u8			api_world_get_entity_pos_last_abs(const world_t* world, entity_id_t entity, vec3f_t* out_position);
	u8			api_world_get_entity_rot_last_abs(const world_t* world, entity_id_t entity, quat_t* out_rotation);
	u8			api_world_get_entity_scale_last_abs(const world_t* world, entity_id_t entity, vec3f_t* out_scale);
	u8			api_world_abs_pos_to_local(world_t* world, entity_id_t entity, const vec3f_t* position, vec3f_t* out_position);
	u8			api_world_abs_rot_to_local(world_t* world, entity_id_t entity, const quat_t* rotation, quat_t* out_rotation);
	u8			api_world_abs_scale_to_local(world_t* world, entity_id_t entity, const vec3f_t* scale, vec3f_t* out_scale);
	entity_id_t api_world_get_entity_with_name(const world_t* world, const char* name);
	u32			api_world_get_all_entities_with_name(const world_t* world, const char* name, entity_id_t* out_entities, u32 capacity);
	entity_id_t api_world_get_entity_with_component(const world_t* world, sid_t component_type);
	u32			api_world_get_all_entities_with_component(const world_t* world, sid_t component_type, entity_id_t* out_entities, u32 capacity);
	u8			api_world_is_alive(const world_t* world, entity_id_t entity);
	u8			api_world_has_component(const world_t* world, entity_id_t entity, sid_t component_type);
	u8			api_world_get_component(const world_t* world, entity_id_t entity, sid_t component_type, void* out_component, u32 component_size);
	u8			api_world_add_component(world_t* world, entity_id_t entity, sid_t component_type, const void* component, u32 component_size);
	u8			api_world_set_component(world_t* world, entity_id_t entity, sid_t component_type, const void* component, u32 component_size);
	u8			api_world_remove_component(world_t* world, entity_id_t entity, sid_t component_type);
	entity_id_t api_world_get_entity_with_tag(const world_t* world, u64 tag);
	u32			api_world_get_all_entities_with_tag(const world_t* world, u64 tag, entity_id_t* out_entities, u32 capacity);
	u8			api_world_set_entity_tag(world_t* world, entity_id_t entity, u64 tag, u8 enabled);
	u8			api_world_hide_entity(world_t* world, entity_id_t entity);
	u8			api_world_show_entity(world_t* world, entity_id_t entity);
	entity_id_t api_world_find_entity_by_guid(const world_t* world, entity_guid_t guid);
	entity_id_t api_world_spawn_prefab(world_t* world, resource_handle_t prefab, entity_id_t parent, const vec3f_t* local_position, const quat_t* local_rotation, const vec3f_t* local_scale);
	u8			api_world_query_begin(world_t* world, const script_world_query_component_t* components, u32 component_count, script_world_query_t* out_query);
	u8			api_world_query_next(script_world_query_t* query, script_world_query_row_t* out_row);
	void		api_world_query_end(script_world_query_t* query);

	struct script_api_world_t
	{
		u32													 size							 = 0;
		u32													 version						 = 0;
		decltype(&api_world_create_entity)					 create_entity					 = nullptr;
		decltype(&api_world_destroy_entity)					 destroy_entity					 = nullptr;
		decltype(&api_world_duplicate_entity)				 duplicate_entity				 = nullptr;
		decltype(&api_world_attach_entity)					 attach_entity					 = nullptr;
		decltype(&api_world_detach_entity)					 detach_entity					 = nullptr;
		decltype(&api_world_set_entity_pos_local)			 set_entity_pos_local			 = nullptr;
		decltype(&api_world_set_entity_rot_local)			 set_entity_rot_local			 = nullptr;
		decltype(&api_world_set_entity_scale_local)			 set_entity_scale_local			 = nullptr;
		decltype(&api_world_teleport_entity)				 teleport_entity				 = nullptr;
		decltype(&api_world_mark_entity_teleported)			 mark_entity_teleported			 = nullptr;
		decltype(&api_world_get_entity_pos_local)			 get_entity_pos_local			 = nullptr;
		decltype(&api_world_get_entity_rot_local)			 get_entity_rot_local			 = nullptr;
		decltype(&api_world_get_entity_scale_local)			 get_entity_scale_local			 = nullptr;
		decltype(&api_world_get_entity_pos_last_abs)		 get_entity_pos_last_abs		 = nullptr;
		decltype(&api_world_get_entity_rot_last_abs)		 get_entity_rot_last_abs		 = nullptr;
		decltype(&api_world_get_entity_scale_last_abs)		 get_entity_scale_last_abs		 = nullptr;
		decltype(&api_world_abs_pos_to_local)				 abs_pos_to_local				 = nullptr;
		decltype(&api_world_abs_rot_to_local)				 abs_rot_to_local				 = nullptr;
		decltype(&api_world_abs_scale_to_local)				 abs_scale_to_local				 = nullptr;
		decltype(&api_world_get_entity_with_name)			 get_entity_with_name			 = nullptr;
		decltype(&api_world_get_all_entities_with_name)		 get_all_entities_with_name		 = nullptr;
		decltype(&api_world_get_entity_with_component)		 get_entity_with_component		 = nullptr;
		decltype(&api_world_get_all_entities_with_component) get_all_entities_with_component = nullptr;
		decltype(&api_world_is_alive)						 is_alive						 = nullptr;
		decltype(&api_world_has_component)					 has_component					 = nullptr;
		decltype(&api_world_get_component)					 get_component					 = nullptr;
		decltype(&api_world_add_component)					 add_component					 = nullptr;
		decltype(&api_world_set_component)					 set_component					 = nullptr;
		decltype(&api_world_remove_component)				 remove_component				 = nullptr;
		decltype(&api_world_get_entity_with_tag)			 get_entity_with_tag			 = nullptr;
		decltype(&api_world_get_all_entities_with_tag)		 get_all_entities_with_tag		 = nullptr;
		decltype(&api_world_set_entity_tag)					 set_entity_tag					 = nullptr;
		decltype(&api_world_hide_entity)					 hide_entity					 = nullptr;
		decltype(&api_world_show_entity)					 show_entity					 = nullptr;
		decltype(&api_world_find_entity_by_guid)			 find_entity_by_guid			 = nullptr;
		decltype(&api_world_spawn_prefab)					 spawn_prefab					 = nullptr;
		decltype(&api_world_query_begin)					 query_begin					 = nullptr;
		decltype(&api_world_query_next)						 query_next						 = nullptr;
		decltype(&api_world_query_end)						 query_end						 = nullptr;
	};

	const script_api_world_t& get_script_api_world();
}
