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

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/color.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>

#include <new>

namespace sfg
{
	struct script_world_query_state_t
	{
		world_t*		   world  = nullptr;
		ecs_query_cursor_t cursor = {};
		bool			   active = false;
	};

	static_assert(sizeof(script_world_query_state_t) <= sizeof(script_world_query_t));
	static_assert(alignof(script_world_query_state_t) <= alignof(script_world_query_t));

	entity_id_t api_world_create_entity(world_t* world, const char* name)
	{
		SFG_ASSERT(world != nullptr);

		if (world->is_component_query_active())
			return NULL_ENTITY_ID;

		return world->create_entity(name);
	}

	u8 api_world_destroy_entity(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (world->is_component_query_active() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		world->destroy_entity_tree(entity);
		return 1;
	}

	entity_id_t api_world_duplicate_entity(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (world->is_component_query_active() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return NULL_ENTITY_ID;

		ostream_t output{};
		world_cooker_t::entity_to_stream(*world, entity, output);

		istream_t input{output.get_raw(), output.get_size()};
		return world_cooker_t::entity_from_stream(*world, input, true);
	}

	u8 api_world_attach_entity(world_t* world, entity_id_t entity, entity_id_t parent)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || parent >= ECS_MAX_ENTITIES || entity == parent || !world->is_alive(entity) || !world->is_alive(parent))
			return 0;

		for (entity_id_t current = parent; current != NULL_ENTITY_ID; current = world->get_entity_parent(current))
		{
			if (current == entity)
				return 0;
		}

		world->attach_to(entity, parent);
		return 1;
	}

	u8 api_world_detach_entity(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		world->detach(entity);
		return 1;
	}

	u8 api_world_set_entity_pos_local(world_t* world, entity_id_t entity, const vec3f_t* position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(position != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		world->set_entity_pos_local(entity, *position);
		return 1;
	}

	u8 api_world_set_entity_rot_local(world_t* world, entity_id_t entity, const quat_t* rotation)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(rotation != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		world->set_entity_rot_local(entity, *rotation);
		return 1;
	}

	u8 api_world_set_entity_scale_local(world_t* world, entity_id_t entity, const vec3f_t* scale)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(scale != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		world->set_entity_scale_local(entity, *scale);
		return 1;
	}

	u8 api_world_teleport_entity(world_t* world, entity_id_t entity, const vec3f_t* position, const quat_t* rotation, const vec3f_t* scale)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(position != nullptr);
		SFG_ASSERT(rotation != nullptr);
		SFG_ASSERT(scale != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		world->teleport_entity(entity, *position, *rotation, *scale);
		return 1;
	}

	u8 api_world_mark_entity_teleported(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		world->mark_entity_teleported(entity);
		return 1;
	}

	u8 api_world_get_entity_pos_local(const world_t* world, entity_id_t entity, vec3f_t* out_position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_position != nullptr);

		*out_position = vec3f_t::zero;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_position = world->get_entity_pos_local(entity);
		return 1;
	}

	u8 api_world_get_entity_rot_local(const world_t* world, entity_id_t entity, quat_t* out_rotation)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_rotation != nullptr);

		*out_rotation = quat_t::identity;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_rotation = world->get_entity_rot_local(entity);
		return 1;
	}

	u8 api_world_get_entity_scale_local(const world_t* world, entity_id_t entity, vec3f_t* out_scale)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_scale != nullptr);

		*out_scale = vec3f_t::one;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_scale = world->get_entity_scale_local(entity);
		return 1;
	}

	u8 api_world_get_entity_pos_last_abs(const world_t* world, entity_id_t entity, vec3f_t* out_position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_position != nullptr);

		*out_position = vec3f_t::zero;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_position = world->get_entity_pos_last_abs(entity);
		return 1;
	}

	u8 api_world_get_entity_rot_last_abs(const world_t* world, entity_id_t entity, quat_t* out_rotation)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_rotation != nullptr);

		*out_rotation = quat_t::identity;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_rotation = world->get_entity_rot_last_abs(entity);
		return 1;
	}

	u8 api_world_get_entity_scale_last_abs(const world_t* world, entity_id_t entity, vec3f_t* out_scale)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_scale != nullptr);

		*out_scale = vec3f_t::one;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_scale = world->get_entity_scale_last_abs(entity);
		return 1;
	}

	u8 api_world_abs_pos_to_local(world_t* world, entity_id_t entity, const vec3f_t* position, vec3f_t* out_position)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(position != nullptr);
		SFG_ASSERT(out_position != nullptr);

		*out_position = vec3f_t::zero;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_position = world->abs_pos_to_local(entity, *position);
		return 1;
	}

	u8 api_world_abs_rot_to_local(world_t* world, entity_id_t entity, const quat_t* rotation, quat_t* out_rotation)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(rotation != nullptr);
		SFG_ASSERT(out_rotation != nullptr);

		*out_rotation = quat_t::identity;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_rotation = world->abs_rot_to_local(entity, *rotation);
		return 1;
	}

	u8 api_world_abs_scale_to_local(world_t* world, entity_id_t entity, const vec3f_t* scale, vec3f_t* out_scale)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(scale != nullptr);
		SFG_ASSERT(out_scale != nullptr);

		*out_scale = vec3f_t::one;

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		*out_scale = world->abs_scale_to_local(entity, *scale);
		return 1;
	}

	entity_id_t api_world_get_entity_with_name(const world_t* world, const char* name)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(name != nullptr);

		const ecs_component_table_t&	name_table = world->get_component_table(type_id_t<component_name_t>::value);
		const ecs_component_table_ref_t table_ref  = name_table.ref();

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = &table_ref, .size = 1}))
		{
			const component_name_t& entity_name = ecs_helpers_t::row_get<component_name_t>(row, 0);

			if (std::strcmp(entity_name.text, name) == 0)
				return row.id;
		}

		return NULL_ENTITY_ID;
	}

	u32 api_world_get_all_entities_with_name(const world_t* world, const char* name, entity_id_t* out_entities, u32 capacity)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(name != nullptr);
		SFG_ASSERT(out_entities != nullptr || capacity == 0);

		if (capacity == 0)
			return 0;

		const ecs_component_table_t&	name_table = world->get_component_table(type_id_t<component_name_t>::value);
		const ecs_component_table_ref_t table_ref  = name_table.ref();
		u32								count	   = 0;

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = &table_ref, .size = 1}))
		{
			const component_name_t& entity_name = ecs_helpers_t::row_get<component_name_t>(row, 0);

			if (std::strcmp(entity_name.text, name) != 0)
				continue;

			out_entities[count] = row.id;
			count++;

			if (count == capacity)
				break;
		}

		return count;
	}

	entity_id_t api_world_get_entity_with_component(const world_t* world, sid_t component_type)
	{
		SFG_ASSERT(world != nullptr);

		const ecs_component_table_t* table = world->find_component_table(component_type);

		if (table == nullptr)
			return NULL_ENTITY_ID;

		const ecs_component_table_ref_t table_ref = table->ref();

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = &table_ref, .size = 1}))
			return row.id;

		return NULL_ENTITY_ID;
	}

	u32 api_world_get_all_entities_with_component(const world_t* world, sid_t component_type, entity_id_t* out_entities, u32 capacity)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_entities != nullptr || capacity == 0);

		const ecs_component_table_t* table = world->find_component_table(component_type);

		if (table == nullptr || capacity == 0)
			return 0;

		const ecs_component_table_ref_t table_ref = table->ref();
		u32								count	  = 0;

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = &table_ref, .size = 1}))
		{
			out_entities[count] = row.id;
			count++;

			if (count == capacity)
				break;
		}

		return count;
	}

	u8 api_world_is_alive(const world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES)
			return 0;

		return world->is_alive(entity) ? 1 : 0;
	}

	u8 api_world_has_component(const world_t* world, entity_id_t entity, sid_t component_type)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		const ecs_component_table_t* table = world->find_component_table(component_type);

		if (table == nullptr)
			return 0;

		return ecs_t::table_has(*table, entity) ? 1 : 0;
	}

	u8 api_world_get_component(const world_t* world, entity_id_t entity, sid_t component_type, void* out_component, u32 component_size)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		const ecs_component_table_t* table = world->find_component_table(component_type);

		if (table == nullptr || table->type_desc.size != component_size || !ecs_t::table_has(*table, entity))
			return 0;

		if (component_size == 0)
			return 1;

		SFG_ASSERT(out_component != nullptr);
		const void* component = ecs_t::table_get(*table, entity);

		SFG_ASSERT(component != nullptr);
		SFG_MEMCPY(out_component, component, component_size);
		return 1;
	}

	u8 api_world_add_component(world_t* world, entity_id_t entity, sid_t component_type, const void* component, u32 component_size)
	{
		SFG_ASSERT(world != nullptr);

		if (world->is_component_query_active() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		ecs_component_table_t* table = world->find_component_table(component_type);

		if (table == nullptr || table->type_desc.size != component_size || ecs_t::table_has(*table, entity))
			return 0;

		void* added_component = ecs_t::table_add(*table, entity);

		if (component_size != 0)
		{
			SFG_ASSERT(component != nullptr);
			SFG_ASSERT(added_component != nullptr);
			SFG_MEMCPY(added_component, component, component_size);
		}

		return 1;
	}

	u8 api_world_set_component(world_t* world, entity_id_t entity, sid_t component_type, const void* component, u32 component_size)
	{
		SFG_ASSERT(world != nullptr);

		if (entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		ecs_component_table_t* table = world->find_component_table(component_type);

		if (table == nullptr || table->type_desc.size != component_size || !ecs_t::table_has(*table, entity))
			return 0;

		if (component_size == 0)
			return 1;

		SFG_ASSERT(component != nullptr);
		void* stored_component = ecs_t::table_get(*table, entity);

		SFG_ASSERT(stored_component != nullptr);
		SFG_MEMCPY(stored_component, component, component_size);
		return 1;
	}

	u8 api_world_remove_component(world_t* world, entity_id_t entity, sid_t component_type)
	{
		SFG_ASSERT(world != nullptr);

		if (world->is_component_query_active() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		ecs_component_table_t* table = world->find_component_table(component_type);

		if (table == nullptr || !ecs_t::table_has(*table, entity))
			return 0;

		ecs_t::table_remove(*table, entity);
		return 1;
	}

	entity_id_t api_world_get_entity_with_tag(const world_t* world, u64 tag)
	{
		SFG_ASSERT(world != nullptr);

		if (tag == 0)
			return NULL_ENTITY_ID;

		const ecs_component_table_t&	tags_table = world->get_component_table(type_id_t<component_entity_tags_t>::value);
		const ecs_component_table_ref_t table_ref  = tags_table.ref();

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = &table_ref, .size = 1}))
		{
			const component_entity_tags_t& entity_tags = ecs_helpers_t::row_get<component_entity_tags_t>(row, 0);

			if ((entity_tags.tags & tag) == tag)
				return row.id;
		}

		return NULL_ENTITY_ID;
	}

	u32 api_world_get_all_entities_with_tag(const world_t* world, u64 tag, entity_id_t* out_entities, u32 capacity)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(out_entities != nullptr || capacity == 0);

		if (tag == 0 || capacity == 0)
			return 0;

		const ecs_component_table_t&	tags_table = world->get_component_table(type_id_t<component_entity_tags_t>::value);
		const ecs_component_table_ref_t table_ref  = tags_table.ref();
		u32								count	   = 0;

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = &table_ref, .size = 1}))
		{
			const component_entity_tags_t& entity_tags = ecs_helpers_t::row_get<component_entity_tags_t>(row, 0);

			if ((entity_tags.tags & tag) != tag)
				continue;

			out_entities[count] = row.id;
			count++;

			if (count == capacity)
				break;
		}

		return count;
	}

	u8 api_world_set_entity_tag(world_t* world, entity_id_t entity, u64 tag, u8 enabled)
	{
		SFG_ASSERT(world != nullptr);

		if (world->is_component_query_active() || tag == 0 || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		ecs_component_table_t&	 tags_table	 = world->get_component_table(type_id_t<component_entity_tags_t>::value);
		component_entity_tags_t* entity_tags = ecs_helpers_t::table_find_as<component_entity_tags_t>(tags_table, entity);

		if (enabled != 0)
		{
			if (entity_tags == nullptr)
				entity_tags = &ecs_helpers_t::table_add_or_get_as<component_entity_tags_t>(tags_table, entity);

			entity_tags->tags |= tag;
			return 1;
		}

		if (entity_tags == nullptr)
			return 1;

		entity_tags->tags &= ~tag;

		if (entity_tags->tags == 0)
			ecs_t::table_remove(tags_table, entity);

		return 1;
	}

	u8 api_world_hide_entity(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (world->is_component_query_active() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		ecs_component_table_t& disabled_table = world->get_component_table(type_id_t<component_disabled_t>::value);

		if (!ecs_t::table_has(disabled_table, entity))
			ecs_t::table_add(disabled_table, entity);

		return 1;
	}

	u8 api_world_show_entity(world_t* world, entity_id_t entity)
	{
		SFG_ASSERT(world != nullptr);

		if (world->is_component_query_active() || entity >= ECS_MAX_ENTITIES || !world->is_alive(entity))
			return 0;

		ecs_component_table_t& disabled_table = world->get_component_table(type_id_t<component_disabled_t>::value);

		if (ecs_t::table_has(disabled_table, entity))
			ecs_t::table_remove(disabled_table, entity);

		return 1;
	}

	entity_id_t api_world_find_entity_by_guid(const world_t* world, entity_guid_t guid)
	{
		SFG_ASSERT(world != nullptr);

		if (guid == NULL_ENTITY_GUID)
			return NULL_ENTITY_ID;

		return world->find_by_guid(guid);
	}

	entity_id_t api_world_spawn_prefab(world_t* world, resource_handle_t prefab, entity_id_t parent, const vec3f_t* local_position, const quat_t* local_rotation, const vec3f_t* local_scale)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(local_position != nullptr);
		SFG_ASSERT(local_rotation != nullptr);
		SFG_ASSERT(local_scale != nullptr);

		if (world->is_component_query_active() || prefab == NULL_RESOURCE_HANDLE || (parent != NULL_ENTITY_ID && (parent >= ECS_MAX_ENTITIES || !world->is_alive(parent))))
			return NULL_ENTITY_ID;

		const entity_id_t root = world_cooker_t::spawn_prefab(*world, prefab, {});

		if (root == NULL_ENTITY_ID)
			return NULL_ENTITY_ID;

		world_cooker_t::make_prefab_chain(*world, root, prefab);

		if (parent != NULL_ENTITY_ID)
			world->attach_to(root, parent);

		world->set_entity_pos_local(root, *local_position);
		world->set_entity_rot_local(root, *local_rotation);
		world->set_entity_scale_local(root, *local_scale);
		world->mark_entity_teleported(root);
		return root;
	}

	u8 api_world_query_begin(world_t* world, const script_world_query_component_t* components, u32 component_count, script_world_query_t* out_query)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(components != nullptr || component_count == 0);
		SFG_ASSERT(out_query != nullptr);

		*out_query = {};

		if (component_count == 0 || component_count > SCRIPT_WORLD_QUERY_MAX_COMPONENTS)
			return 0;

		ecs_component_table_ref_t table_refs[SCRIPT_WORLD_QUERY_MAX_COMPONENTS] = {};
		u32						  table_count									= 0;
		u32						  required_count								= 0;
		bool					  missing_required								= false;

		for (u32 component_index = 0; component_index < component_count; ++component_index)
		{
			const script_world_query_component_t& component = components[component_index];

			// verify flags
			if (component.type_id == 0 || (component.flags != script_world_query_component_required && component.flags != script_world_query_component_optional && component.flags != script_world_query_component_excluded))
				return 0;

			// verify prev
			for (u32 previous_index = 0; previous_index < component_index; ++previous_index)
			{
				if (components[previous_index].type_id == component.type_id)
					return 0;
			}

			const bool required = component.flags == script_world_query_component_required;

			if (required)
				required_count++;

			const ecs_component_table_t* table = world->find_component_table(component.type_id);

			if (table == nullptr)
			{
				missing_required |= required;
				continue;
			}

			if (table->type_desc.size != component.size)
				return 0;

			table_refs[table_count] = {
				.table = table,
				.flags = component.flags,
			};
			table_count++;
		}

		if (required_count == 0)
			return 0;

		script_world_query_state_t* state = new (out_query->storage) script_world_query_state_t{};
		state->world					  = world;

		if (missing_required)
			return 1;

		ecs_t::inner_join_init(state->cursor, {.data = table_refs, .size = table_count});
		world->begin_component_query();
		state->active = true;
		return 1;
	}

	u8 api_world_query_next(script_world_query_t* query, script_world_query_row_t* out_row)
	{
		SFG_ASSERT(query != nullptr);
		SFG_ASSERT(out_row != nullptr);

		*out_row = {};

		script_world_query_state_t* state = reinterpret_cast<script_world_query_state_t*>(query->storage);

		if (!state->active)
			return 0;

		if (!ecs_t::inner_join_next(state->cursor))
		{
			state->world->end_component_query();
			state->active = false;
			return 0;
		}

		const ecs_query_row_t& row		 = state->cursor.current;
		out_row->entity					 = row.id;
		out_row->component_count		 = row.component_count;
		out_row->component_presence_mask = row.component_presence_mask;

		for (u32 component_index = 0; component_index < row.component_count; ++component_index)
		{
			out_row->components[component_index]		 = row.components[component_index];
			out_row->component_type_ids[component_index] = row.component_type_ids[component_index];
		}

		return 1;
	}

	void api_world_query_end(script_world_query_t* query)
	{
		SFG_ASSERT(query != nullptr);

		script_world_query_state_t* state = reinterpret_cast<script_world_query_state_t*>(query->storage);

		if (!state->active)
			return;

		state->world->end_component_query();
		state->active = false;
	}

	void api_world_debug_draw_line(world_t* world, const vec3f_t* from, const vec3f_t* to, const color_t* color, f32 thickness_px, debug_draw_depth_e depth)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(from != nullptr);
		SFG_ASSERT(to != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_line(*from, *to, *color, thickness_px, depth);
	}

	void api_world_debug_draw_arrow(world_t* world, const vec3f_t* from, const vec3f_t* to, const color_t* color, f32 head_length, f32 head_radius, f32 thickness_px, debug_draw_depth_e depth)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(from != nullptr);
		SFG_ASSERT(to != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_arrow(*from, *to, *color, head_length, head_radius, thickness_px, depth);
	}

	void api_world_debug_draw_triangle(world_t* world, const vec3f_t* p0, const vec3f_t* p1, const vec3f_t* p2, const color_t* color)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(p0 != nullptr);
		SFG_ASSERT(p1 != nullptr);
		SFG_ASSERT(p2 != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_triangle(*p0, *p1, *p2, *color);
	}

	void api_world_debug_draw_polyline(world_t* world, const vec3f_t* points, u32 point_count, const color_t* color, f32 thickness_px, debug_draw_depth_e depth, u8 closed)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(points != nullptr || point_count == 0);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_polyline({.data = points, .size = point_count}, *color, thickness_px, depth, closed != 0);
	}

	void api_world_debug_draw_aabb(world_t* world, const vec3f_t* bounds_min, const vec3f_t* bounds_max, const color_t* color, f32 thickness_px, debug_draw_depth_e depth)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(bounds_min != nullptr);
		SFG_ASSERT(bounds_max != nullptr);
		SFG_ASSERT(color != nullptr);

		const aabb_t bounds(*bounds_min, *bounds_max);

		world->get_debug_draw().draw_aabb(bounds, *color, thickness_px, depth);
	}

	void api_world_debug_draw_box(world_t* world, const mat4x3_t* transform, const vec3f_t* half_extents, const color_t* color, f32 thickness_px, debug_draw_depth_e depth)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(transform != nullptr);
		SFG_ASSERT(half_extents != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_box(*transform, *half_extents, *color, thickness_px, depth);
	}

	void api_world_debug_draw_rectangle(world_t* world, const vec3f_t* center, const vec3f_t* right, const vec3f_t* up, const vec2f_t* size, const color_t* color, f32 thickness_px, debug_draw_depth_e depth)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(center != nullptr);
		SFG_ASSERT(right != nullptr);
		SFG_ASSERT(up != nullptr);
		SFG_ASSERT(size != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_rectangle(*center, *right, *up, *size, *color, thickness_px, depth);
	}

	void api_world_debug_draw_arc(world_t* world, const vec3f_t* center, const vec3f_t* normal, const vec3f_t* start_direction, f32 radius, f32 angle_radians, const color_t* color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(center != nullptr);
		SFG_ASSERT(normal != nullptr);
		SFG_ASSERT(start_direction != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_arc(*center, *normal, *start_direction, radius, angle_radians, *color, thickness_px, depth, segments);
	}

	void api_world_debug_draw_circle(world_t* world, const vec3f_t* center, f32 radius, const vec3f_t* normal, const color_t* color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(center != nullptr);
		SFG_ASSERT(normal != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_circle(*center, radius, *normal, *color, thickness_px, depth, segments);
	}

	void api_world_debug_draw_sphere(world_t* world, const vec3f_t* center, f32 radius, const color_t* color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(center != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_sphere(*center, radius, *color, thickness_px, depth, segments);
	}

	void api_world_debug_draw_capsule(world_t* world, const vec3f_t* center, f32 radius, f32 half_height, const vec3f_t* direction, const color_t* color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(center != nullptr);
		SFG_ASSERT(direction != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_capsule(*center, radius, half_height, *direction, *color, thickness_px, depth, segments);
	}

	void api_world_debug_draw_cylinder(world_t* world, const vec3f_t* center, f32 radius, f32 half_height, const vec3f_t* direction, const color_t* color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(center != nullptr);
		SFG_ASSERT(direction != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_cylinder(*center, radius, half_height, *direction, *color, thickness_px, depth, segments);
	}

	void api_world_debug_draw_cone(world_t* world, const vec3f_t* origin, const vec3f_t* direction, f32 length, f32 half_angle_radians, const color_t* color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(origin != nullptr);
		SFG_ASSERT(direction != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_cone(*origin, *direction, length, half_angle_radians, *color, thickness_px, depth, segments);
	}

	void api_world_debug_draw_text_2d(world_t* world, const vec2f_t* position, const char* text, const color_t* color, f32 size_px, debug_draw_text_alignment_e alignment, resource_handle_t font)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(position != nullptr);
		SFG_ASSERT(text != nullptr);
		SFG_ASSERT(color != nullptr);

		world->get_debug_draw().draw_text_2d(*position, text, *color, size_px, alignment, font);
	}

	void api_world_debug_draw_text_3d(world_t* world, const vec3f_t* position, const char* text, const color_t* color, f32 size_px, debug_draw_depth_e depth, debug_draw_text_alignment_e alignment, const vec2f_t* screen_offset, resource_handle_t font)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(position != nullptr);
		SFG_ASSERT(text != nullptr);
		SFG_ASSERT(color != nullptr);
		SFG_ASSERT(screen_offset != nullptr);

		world->get_debug_draw().draw_text_3d(*position, text, *color, size_px, depth, alignment, *screen_offset, font);
	}

	void api_world_debug_draw_texture_3d(world_t* world, const vec3f_t* position, resource_handle_t texture, const vec2f_t* size_px, const color_t* color, entity_id_t entity, debug_draw_depth_e depth, const vec2f_t* screen_offset, u8 linear_sample)
	{
		SFG_ASSERT(world != nullptr);
		SFG_ASSERT(position != nullptr);
		SFG_ASSERT(size_px != nullptr);
		SFG_ASSERT(color != nullptr);
		SFG_ASSERT(screen_offset != nullptr);

		world->get_debug_draw().draw_texture_3d(*position, texture, *size_px, *color, entity, depth, *screen_offset, linear_sample != 0);
	}

	void api_world_set_time_scale(world_t* world, f32 time_scale)
	{
		SFG_ASSERT(world != nullptr);

		world->set_time_scale(time_scale);
	}

	f32 api_world_get_time_scale(const world_t* world)
	{
		SFG_ASSERT(world != nullptr);

		return world->get_time_scale();
	}

	f32 api_world_get_elapsed_time(const world_t* world)
	{
		SFG_ASSERT(world != nullptr);

		return world->get_elapsed_time();
	}

	f32 api_world_get_real_elapsed_time(const world_t* world)
	{
		SFG_ASSERT(world != nullptr);

		return world->get_real_elapsed_time();
	}

	const script_api_world_t& get_script_api_world()
	{
		static const script_api_world_t api{
			.size							 = static_cast<u32>(sizeof(script_api_world_t)),
			.version						 = 5,
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
			.hide_entity					 = api_world_hide_entity,
			.show_entity					 = api_world_show_entity,
			.find_entity_by_guid			 = api_world_find_entity_by_guid,
			.spawn_prefab					 = api_world_spawn_prefab,
			.query_begin					 = api_world_query_begin,
			.query_next						 = api_world_query_next,
			.query_end						 = api_world_query_end,
			.debug_draw_line				 = api_world_debug_draw_line,
			.debug_draw_arrow				 = api_world_debug_draw_arrow,
			.debug_draw_triangle			 = api_world_debug_draw_triangle,
			.debug_draw_polyline			 = api_world_debug_draw_polyline,
			.debug_draw_aabb				 = api_world_debug_draw_aabb,
			.debug_draw_box					 = api_world_debug_draw_box,
			.debug_draw_rectangle			 = api_world_debug_draw_rectangle,
			.debug_draw_arc					 = api_world_debug_draw_arc,
			.debug_draw_circle				 = api_world_debug_draw_circle,
			.debug_draw_sphere				 = api_world_debug_draw_sphere,
			.debug_draw_capsule				 = api_world_debug_draw_capsule,
			.debug_draw_cylinder			 = api_world_debug_draw_cylinder,
			.debug_draw_cone				 = api_world_debug_draw_cone,
			.debug_draw_text_2d				 = api_world_debug_draw_text_2d,
			.debug_draw_text_3d				 = api_world_debug_draw_text_3d,
			.debug_draw_texture_3d			 = api_world_debug_draw_texture_3d,
			.set_time_scale					 = api_world_set_time_scale,
			.get_time_scale					 = api_world_get_time_scale,
			.get_elapsed_time				 = api_world_get_elapsed_time,
			.get_real_elapsed_time			 = api_world_get_real_elapsed_time,
		};

		return api;
	}
}
