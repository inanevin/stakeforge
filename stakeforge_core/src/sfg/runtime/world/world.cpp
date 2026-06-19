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

#include "world.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>

namespace sfg
{
#define WORLD_TEXT_BYTES			  (64 * 1024)
#define WORLD_TEXT_ALLOCATION_RESERVE 1024

	void world_t::init()
	{
		_component_tables.reserve(64);
		_entity_free_list.reserve(1024);
		_text_allocations.reserve(WORLD_TEXT_ALLOCATION_RESERVE);
		_text_allocation_free_list.reserve(WORLD_TEXT_ALLOCATION_RESERVE);
		_text_allocator.init(WORLD_TEXT_BYTES);

		add_component_table(ecs_helpers_t::make_component_desc<component_hierarchy_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_transform_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_name_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_mesh_renderer_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_render_object_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_camera_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_skybox_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_debug_widgets_t>());
		add_component_table(ecs_helpers_t::make_tag_component_desc<component_alive_t>());
		add_component_table(ecs_helpers_t::make_tag_component_desc<component_disabled_t>());
		add_component_table(ecs_helpers_t::make_tag_component_desc<component_no_serialize_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_system_transform_t>());

		_engine_components.hierarchy_table	   = &get_component_table(component_hierarchy_t::TYPE_ID)->table;
		_engine_components.transform_table	   = &get_component_table(component_transform_t::TYPE_ID)->table;
		_engine_components.name_table		   = &get_component_table(component_name_t::TYPE_ID)->table;
		_engine_components.mesh_renderer_table = &get_component_table(component_mesh_renderer_t::TYPE_ID)->table;
		_engine_components.render_object_table = &get_component_table(component_render_object_t::TYPE_ID)->table;
		_engine_components.camera_table		   = &get_component_table(component_camera_t::TYPE_ID)->table;
		_engine_components.skybox_table		   = &get_component_table(component_skybox_t::TYPE_ID)->table;
		_engine_components.debug_widgets_table = &get_component_table(component_debug_widgets_t::TYPE_ID)->table;
		_engine_components.alive_table		   = &get_component_table(component_alive_t::TYPE_ID)->table;
		_engine_components.disabled_table	   = &get_component_table(component_disabled_t::TYPE_ID)->table;
		_engine_components.no_serialize_table  = &get_component_table(component_no_serialize_t::TYPE_ID)->table;
		_system_components.transform_table	   = &get_component_table(component_system_transform_t::TYPE_ID)->table;
	}

	void world_t::uninit()
	{
		for (world_component_table_t& table : _component_tables)
			ecs_t::table_uninit(table.table);

		_component_tables.resize(0);
		_entity_free_list.resize(0);
		_text_allocations.resize(0);
		_text_allocation_free_list.resize(0);
		_text_allocator.uninit();
		_engine_components = {};
		_system_components = {};
		_entity_head	   = 0;
	}

	void world_t::tick(f32)
	{
	}

	entity_id_t world_t::create_entity(const char* name)
	{
		entity_id_t id = NULL_ENTITY_ID;
		if (!_entity_free_list.empty())
		{
			id = _entity_free_list.back();
			_entity_free_list.pop_back();
		}
		else
		{
			SFG_ASSERT(_entity_head < ECS_MAX_ENTITIES);
			id = _entity_head;
			_entity_head++;
		}

		ecs_t::table_add(*_engine_components.alive_table, id);
		ecs_helpers_t::table_add_or_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		ecs_helpers_t::table_add_or_get_as<component_transform_t>(*_engine_components.transform_table, id);
		component_name_t& name_component = ecs_helpers_t::table_add_or_get_as<component_name_t>(*_engine_components.name_table, id);
		if (name != nullptr)
			name_component.text_index = allocate_text(name);
		component_system_transform_t& system_transform = ecs_helpers_t::table_add_or_get_as<component_system_transform_t>(*_system_components.transform_table, id);
		system_transform.snap_interpolation			   = true;

		return id;
	}

	void world_t::destroy_entity(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		SFG_ASSERT(hierarchy.first_child == NULL_ENTITY_ID);

		detach(id);

		const component_name_t& name = ecs_helpers_t::table_get_as<component_name_t>(*_engine_components.name_table, id);
		release_text(name.text_index);

		ecs_t::table_remove(*_engine_components.alive_table, id);
		ecs_t::table_remove(*_engine_components.hierarchy_table, id);
		ecs_t::table_remove(*_engine_components.transform_table, id);
		ecs_t::table_remove(*_engine_components.name_table, id);
		ecs_t::table_remove(*_engine_components.render_object_table, id);
		ecs_t::table_remove(*_engine_components.skybox_table, id);
		ecs_t::table_remove(*_engine_components.debug_widgets_table, id);
		ecs_t::table_remove(*_engine_components.disabled_table, id);
		ecs_t::table_remove(*_engine_components.no_serialize_table, id);
		ecs_t::table_remove(*_system_components.transform_table, id);
		_entity_free_list.push_back(id);
	}

	void world_t::destroy_entity_tree(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
			const entity_id_t			 next_child		 = child_hierarchy.next_sibling;
			destroy_entity_tree(child);
			child = next_child;
		}

		destroy_entity(id);
	}

	void world_t::set_entity_name(entity_id_t id, const char* name)
	{
		SFG_ASSERT(is_alive(id));

		component_name_t& name_component = ecs_helpers_t::table_get_as<component_name_t>(*_engine_components.name_table, id);
		release_text(name_component.text_index);
		name_component.text_index = allocate_text(name != nullptr ? name : "");
	}

	void world_t::entity_to_stream(entity_id_t id, ostream_t& stream) const
	{
		SFG_ASSERT(is_alive(id));

		frame_vector_t<entity_id_t> entities;
		frame_vector_t<u32>			entity_to_index;
		entities.reserve(32);
		entity_to_index.resize(_entity_head, ECS_INVALID_INDEX);

		auto collect_entities = [&](auto&& self, entity_id_t entity) -> void {
			if (ecs_t::table_has(*_engine_components.no_serialize_table, entity))
				return;

			const u32 index = static_cast<u32>(entities.size());
			entities.push_back(entity);
			entity_to_index[entity] = index;

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, entity);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
				const entity_id_t			 next_child		 = child_hierarchy.next_sibling;
				self(self, child);
				child = next_child;
			}
		};

		collect_entities(collect_entities, id);

		const u32 entity_count = static_cast<u32>(entities.size());
		stream << entity_count;

		auto get_serialized_entity_index = [&](entity_id_t entity) -> u32 {
			if (entity == NULL_ENTITY_ID)
				return ECS_INVALID_INDEX;

			SFG_ASSERT(entity < entity_to_index.size());
			return entity_to_index[entity];
		};

		auto get_first_serialized_child_index = [&](entity_id_t entity) -> u32 {
			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, entity);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
				const u32					 index			 = get_serialized_entity_index(child);
				if (index != ECS_INVALID_INDEX)
					return index;

				child = child_hierarchy.next_sibling;
			}

			return ECS_INVALID_INDEX;
		};

		auto get_next_serialized_sibling_index = [&](entity_id_t entity) -> u32 {
			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, entity);
			for (entity_id_t sibling = hierarchy.next_sibling; sibling != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& sibling_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, sibling);
				const u32					 index			   = get_serialized_entity_index(sibling);
				if (index != ECS_INVALID_INDEX)
					return index;

				sibling = sibling_hierarchy.next_sibling;
			}

			return ECS_INVALID_INDEX;
		};

		auto get_prev_serialized_sibling_index = [&](entity_id_t entity) -> u32 {
			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, entity);
			for (entity_id_t sibling = hierarchy.prev_sibling; sibling != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& sibling_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, sibling);
				const u32					 index			   = get_serialized_entity_index(sibling);
				if (index != ECS_INVALID_INDEX)
					return index;

				sibling = sibling_hierarchy.prev_sibling;
			}

			return ECS_INVALID_INDEX;
		};

		auto should_serialize_component_table = [&](const world_component_table_t& table) -> bool {
			const sid_t					 type_id		= table.type_desc.type_id;
			const reflected_type_desc_t* reflected_type = reflection_registry_t::get().find_type(type_id);
			const bool					 no_serialize	= reflected_type != nullptr && (reflected_type->flags & reflected_type_flags_no_serialize) != 0;
			return !no_serialize && type_id != component_hierarchy_t::TYPE_ID && type_id != component_name_t::TYPE_ID && type_id != component_alive_t::TYPE_ID;
		};

		for (u32 i = 0; i < entity_count; i++)
		{
			const entity_id_t			 entity		  = entities[i];
			const component_hierarchy_t& hierarchy	  = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, entity);
			const component_name_t&		 name		  = ecs_helpers_t::table_get_as_const<component_name_t>(*_engine_components.name_table, entity);
			const char*					 text		  = get_text(name.text_index);
			const u32					 parent		  = get_serialized_entity_index(hierarchy.parent);
			const u32					 first_child  = get_first_serialized_child_index(entity);
			const u32					 next_sibling = get_next_serialized_sibling_index(entity);
			const u32					 prev_sibling = get_prev_serialized_sibling_index(entity);

			stream << i;
			stream << parent;
			stream << first_child;
			stream << next_sibling;
			stream << prev_sibling;
			stream << string_t(text != nullptr ? text : "");

			u32 component_count = 0;
			for (const world_component_table_t& table : _component_tables)
			{
				if (should_serialize_component_table(table) && ecs_t::table_has(table.table, entity))
					component_count++;
			}

			stream << component_count;
			for (const world_component_table_t& table : _component_tables)
			{
				if (!should_serialize_component_table(table) || !ecs_t::table_has(table.table, entity))
					continue;

				const sid_t type_id = table.type_desc.type_id;
				stream << type_id;
				if (!table.type_desc.flags.is_set(ecs_component_type_flags_tag))
				{
					const void* component  = ecs_t::table_get(table.table, entity);
					const bool	serialized = reflection_registry_t::get().serialize_to_stream(type_id, component, stream);
					if (!serialized)
						SFG_ASSERT(false);
				}
			}
		}
	}

	entity_id_t world_t::entity_from_stream(istream_t& stream)
	{
		struct stream_entity_t
		{
			u32 parent_index	   = ECS_INVALID_INDEX;
			u32 first_child_index  = ECS_INVALID_INDEX;
			u32 next_sibling_index = ECS_INVALID_INDEX;
			u32 prev_sibling_index = ECS_INVALID_INDEX;
		};

		u32 entity_count = 0;
		stream >> entity_count;
		if (entity_count == 0)
			return NULL_ENTITY_ID;

		frame_vector_t<entity_id_t>		entities;
		frame_vector_t<stream_entity_t> stream_entities;
		entities.resize(entity_count, NULL_ENTITY_ID);
		stream_entities.resize(entity_count);

		entity_id_t root	   = NULL_ENTITY_ID;
		u32			root_count = 0;

		for (u32 i = 0; i < entity_count; i++)
		{
			u32		 index = ECS_INVALID_INDEX;
			string_t name;
			stream >> index;
			SFG_ASSERT(index < entity_count);
			SFG_ASSERT(entities[index] == NULL_ENTITY_ID);
			stream >> stream_entities[index].parent_index;
			stream >> stream_entities[index].first_child_index;
			stream >> stream_entities[index].next_sibling_index;
			stream >> stream_entities[index].prev_sibling_index;
			stream >> name;

			entities[index] = create_entity(name.empty() ? nullptr : name.c_str());

			u32 component_count = 0;
			stream >> component_count;
			for (u32 component_index = 0; component_index < component_count; component_index++)
			{
				sid_t type_id = 0;
				stream >> type_id;
				world_component_table_t*	 table			= get_component_table(type_id);
				const reflected_type_desc_t* reflected_type = reflection_registry_t::get().find_type(type_id);
				if (reflected_type != nullptr && (reflected_type->flags & reflected_type_flags_no_serialize) != 0)
					continue;

				void* component = ecs_t::table_add(table->table, entities[index]);
				if (!table->type_desc.flags.is_set(ecs_component_type_flags_tag))
				{
					const bool deserialized = reflection_registry_t::get().deserialize_from_stream(type_id, component, stream);
					if (!deserialized)
						SFG_ASSERT(false);
				}
			}

			if (stream_entities[index].parent_index == ECS_INVALID_INDEX)
			{
				root = entities[index];
				root_count++;
			}
		}
		SFG_ASSERT(root_count == 1);

		auto index_to_entity = [&](u32 index) -> entity_id_t {
			if (index == ECS_INVALID_INDEX)
				return NULL_ENTITY_ID;

			SFG_ASSERT(index < entities.size());
			SFG_ASSERT(entities[index] != NULL_ENTITY_ID);
			return entities[index];
		};

		for (u32 i = 0; i < entity_count; i++)
		{
			component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, entities[i]);
			hierarchy.parent				 = index_to_entity(stream_entities[i].parent_index);
			hierarchy.first_child			 = index_to_entity(stream_entities[i].first_child_index);
			hierarchy.next_sibling			 = index_to_entity(stream_entities[i].next_sibling_index);
			hierarchy.prev_sibling			 = index_to_entity(stream_entities[i].prev_sibling_index);
		}

		sync_entity_hierarchy(root);
		return root;
	}

	entity_id_t world_t::get_entity_parent(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));
		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		return hierarchy.parent;
	}

	void world_t::attach_to(entity_id_t id, entity_id_t parent)
	{
		SFG_ASSERT(id != parent);
		SFG_ASSERT(is_alive(id));
		SFG_ASSERT(is_alive(parent));

		for (entity_id_t current = parent; current != NULL_ENTITY_ID;)
		{
			SFG_ASSERT(current != id);
			const component_hierarchy_t& current_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, current);
			current										   = current_hierarchy.parent;
		}

		component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		if (hierarchy.parent == parent)
			return;

		detach(id);

		component_hierarchy_t& parent_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, parent);
		component_hierarchy_t& child_hierarchy	= ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);

		child_hierarchy.parent		 = parent;
		child_hierarchy.prev_sibling = NULL_ENTITY_ID;
		child_hierarchy.next_sibling = NULL_ENTITY_ID;

		if (parent_hierarchy.first_child == NULL_ENTITY_ID)
		{
			parent_hierarchy.first_child = id;
			return;
		}

		entity_id_t last_child = parent_hierarchy.first_child;
		while (true)
		{
			component_hierarchy_t& last_child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, last_child);
			if (last_child_hierarchy.next_sibling == NULL_ENTITY_ID)
			{
				last_child_hierarchy.next_sibling = id;
				child_hierarchy.prev_sibling	  = last_child;
				break;
			}

			last_child = last_child_hierarchy.next_sibling;
		}
	}

	void world_t::detach(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		component_hierarchy_t& hierarchy	= ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		const entity_id_t	   parent		= hierarchy.parent;
		const entity_id_t	   next_sibling = hierarchy.next_sibling;
		const entity_id_t	   prev_sibling = hierarchy.prev_sibling;

		if (parent != NULL_ENTITY_ID)
		{
			component_hierarchy_t& parent_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, parent);
			if (parent_hierarchy.first_child == id)
				parent_hierarchy.first_child = next_sibling;
		}

		if (prev_sibling != NULL_ENTITY_ID)
		{
			component_hierarchy_t& prev_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, prev_sibling);
			prev_hierarchy.next_sibling			  = next_sibling;
		}

		if (next_sibling != NULL_ENTITY_ID)
		{
			component_hierarchy_t& next_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, next_sibling);
			next_hierarchy.prev_sibling			  = prev_sibling;
		}

		hierarchy.parent	   = NULL_ENTITY_ID;
		hierarchy.next_sibling = NULL_ENTITY_ID;
		hierarchy.prev_sibling = NULL_ENTITY_ID;
	}

	void world_t::set_entity_pos_local(entity_id_t id, const vec3f_t& pos)
	{
		SFG_ASSERT(is_alive(id));

		component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		transform.pos					 = pos;
	}

	void world_t::set_entity_rot_local(entity_id_t id, const quat_t& rot)
	{
		SFG_ASSERT(is_alive(id));

		component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		transform.rot					 = rot;
	}

	void world_t::set_entity_scale_local(entity_id_t id, const vec3f_t& scale)
	{
		SFG_ASSERT(is_alive(id));

		component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		transform.scale					 = scale;
	}

	void world_t::teleport_entity(entity_id_t id, const vec3f_t& pos, const quat_t& rot, const vec3f_t& scale)
	{
		SFG_ASSERT(is_alive(id));

		const vec3f_t local_pos	  = abs_pos_to_local(id, pos);
		const quat_t  local_rot	  = abs_rot_to_local(id, rot);
		const vec3f_t local_scale = abs_scale_to_local(id, scale);

		component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		transform.pos					 = local_pos;
		transform.rot					 = local_rot;
		transform.scale					 = local_scale;

		set_entity_snap_interpolation_recursive(id);
	}

	void world_t::mark_entity_teleported(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));
		set_entity_snap_interpolation_recursive(id);
	}

	const vec3f_t& world_t::get_entity_pos_local(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_transform_t& transform = ecs_helpers_t::table_get_as_const<component_transform_t>(*_engine_components.transform_table, id);
		return transform.pos;
	}

	const quat_t& world_t::get_entity_rot_local(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_transform_t& transform = ecs_helpers_t::table_get_as_const<component_transform_t>(*_engine_components.transform_table, id);
		return transform.rot;
	}

	const vec3f_t& world_t::get_entity_scale_local(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_transform_t& transform = ecs_helpers_t::table_get_as_const<component_transform_t>(*_engine_components.transform_table, id);
		return transform.scale;
	}

	vec3f_t world_t::abs_pos_to_local(entity_id_t id, const vec3f_t& pos)
	{
		SFG_ASSERT(is_alive(id));

		const mat4x3_t parent_abs_mat = calculate_parent_transform_direct(id);
		return parent_abs_mat.inverse() * pos;
	}

	quat_t world_t::abs_rot_to_local(entity_id_t id, const quat_t& rot)
	{
		SFG_ASSERT(is_alive(id));

		vec3f_t parent_abs_pos;
		quat_t	parent_abs_rot;
		vec3f_t parent_abs_scale;
		calculate_parent_transform_direct(id).decompose(parent_abs_pos, parent_abs_rot, parent_abs_scale);
		return parent_abs_rot.inverse() * rot;
	}

	vec3f_t world_t::abs_scale_to_local(entity_id_t id, const vec3f_t& scale)
	{
		SFG_ASSERT(is_alive(id));

		vec3f_t parent_abs_pos;
		quat_t	parent_abs_rot;
		vec3f_t parent_abs_scale;
		calculate_parent_transform_direct(id).decompose(parent_abs_pos, parent_abs_rot, parent_abs_scale);
		return {scale.x / parent_abs_scale.x, scale.y / parent_abs_scale.y, scale.z / parent_abs_scale.z};
	}

	mat4x3_t world_t::calculate_transform_direct(entity_id_t id)
	{
		SFG_ASSERT(is_alive(id));

		const mat4x3_t				  parent_abs_mat   = calculate_parent_transform_direct(id);
		const component_transform_t&  transform		   = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		component_system_transform_t& system_transform = ecs_helpers_t::table_get_as<component_system_transform_t>(*_system_components.transform_table, id);

		system_transform.abs_mat = parent_abs_mat * mat4x3_t::transform(transform.pos, transform.rot, transform.scale);
		system_transform.abs_mat.decompose(system_transform.abs_pos, system_transform.abs_rot, system_transform.abs_scale);
		return system_transform.abs_mat;
	}

	void world_t::update_world_transforms(bool advance_interpolation)
	{
		const ecs_component_table_ref_t table_refs[] = {
			_engine_components.transform_table->ref(),
			_engine_components.hierarchy_table->ref(),
			_system_components.transform_table->ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_hierarchy_t& hierarchy = ecs_helpers_t::row_get<component_hierarchy_t>(row, 1);
			if (hierarchy.parent != NULL_ENTITY_ID)
				continue;

			update_entity_transform(row.id, hierarchy, vec3f_t::zero, quat_t::identity, vec3f_t::one, mat4x3_t::identity, advance_interpolation);
		}
	}

	void world_t::sync_entity_hierarchy(entity_id_t id)
	{
	}

	void world_t::update_entity_transform(entity_id_t id, const component_hierarchy_t& own_hierarchy, const vec3f_t& parent_abs_pos, const quat_t& parent_abs_rot, const vec3f_t& parent_abs_scale, const mat4x3_t& parent_abs_mat, bool advance_interpolation)
	{
		const component_transform_t&  transform		   = ecs_helpers_t::table_get_as<component_transform_t>(*_engine_components.transform_table, id);
		component_system_transform_t& system_transform = ecs_helpers_t::table_get_as<component_system_transform_t>(*_system_components.transform_table, id);

		if (advance_interpolation)
		{
			system_transform.prev_abs_mat	= system_transform.abs_mat;
			system_transform.prev_abs_rot	= system_transform.abs_rot;
			system_transform.prev_abs_pos	= system_transform.abs_pos;
			system_transform.prev_abs_scale = system_transform.abs_scale;
		}

		system_transform.abs_pos   = parent_abs_pos + (parent_abs_rot * (transform.pos * parent_abs_scale));
		system_transform.abs_rot   = parent_abs_rot * transform.rot;
		system_transform.abs_scale = parent_abs_scale * transform.scale;
		system_transform.abs_mat   = parent_abs_mat * mat4x3_t::transform(transform.pos, transform.rot, transform.scale);

		if (system_transform.snap_interpolation)
		{
			system_transform.prev_abs_mat		= system_transform.abs_mat;
			system_transform.prev_abs_rot		= system_transform.abs_rot;
			system_transform.prev_abs_pos		= system_transform.abs_pos;
			system_transform.prev_abs_scale		= system_transform.abs_scale;
			system_transform.snap_interpolation = false;
		}

		for (entity_id_t child = own_hierarchy.first_child; child != NULL_ENTITY_ID;)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
			update_entity_transform(child, child_hierarchy, system_transform.abs_pos, system_transform.abs_rot, system_transform.abs_scale, system_transform.abs_mat, advance_interpolation);
			child = child_hierarchy.next_sibling;
		}
	}

	void world_t::set_entity_snap_interpolation_recursive(entity_id_t id)
	{
		component_system_transform_t& system_transform = ecs_helpers_t::table_get_as<component_system_transform_t>(*_system_components.transform_table, id);
		system_transform.snap_interpolation			   = true;

		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
		{
			const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
			set_entity_snap_interpolation_recursive(child);
			child = child_hierarchy.next_sibling;
		}
	}

	mat4x3_t world_t::calculate_parent_transform_direct(entity_id_t id)
	{
		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		if (hierarchy.parent == NULL_ENTITY_ID)
			return mat4x3_t::identity;

		return calculate_transform_direct(hierarchy.parent);
	}

	world_component_table_t& world_t::add_component_table(const ecs_component_type_desc_t& desc)
	{
		SFG_ASSERT(find_component_table(desc.type_id) == nullptr);

		world_component_table_t& table = _component_tables.emplace_back();
		table.type_desc				   = desc;
		ecs_t::table_init(table.table, desc);
		return table;
	}

	const world_component_table_t* world_t::find_component_table(sid_t type_id) const
	{
		for (const world_component_table_t& table : _component_tables)
		{
			if (table.type_desc.type_id == type_id)
				return &table;
		}

		return nullptr;
	}

	world_component_table_t* world_t::find_component_table(sid_t type_id)
	{
		for (world_component_table_t& table : _component_tables)
		{
			if (table.type_desc.type_id == type_id)
				return &table;
		}

		return nullptr;
	}

	world_component_table_t* world_t::get_component_table(sid_t type_id)
	{
		world_component_table_t* table = find_component_table(type_id);
		SFG_ASSERT(table);
		return table;
	}

	const vector_t<world_component_table_t>& world_t::get_component_tables() const
	{
		return _component_tables;
	}

	const char* world_t::get_entity_name(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));

		const component_name_t& name = ecs_helpers_t::table_get_as_const<component_name_t>(*_engine_components.name_table, id);
		return get_text(name.text_index);
	}

	const char* world_t::get_text(u32 text_index) const
	{
		if (text_index == ECS_INVALID_INDEX)
			return nullptr;

		SFG_ASSERT(text_index < _text_allocations.size());
		return _text_allocations[text_index].allocated;
	}

	bool world_t::is_alive(entity_id_t id) const
	{
		return ecs_t::table_has(*_engine_components.alive_table, id);
	}

	u32 world_t::allocate_text(const char* text)
	{
		const char* allocated = _text_allocator.allocate(text);
		SFG_ASSERT(allocated != nullptr);
		if (allocated == nullptr)
			return ECS_INVALID_INDEX;

		if (!_text_allocation_free_list.empty())
		{
			const u32 text_index = _text_allocation_free_list.back();
			_text_allocation_free_list.pop_back();
			_text_allocations[text_index].allocated = allocated;
			return text_index;
		}

		SFG_ASSERT(_text_allocations.size() < ECS_INVALID_INDEX);
		const u32 text_index = static_cast<u32>(_text_allocations.size());
		_text_allocations.push_back({.allocated = allocated});
		return text_index;
	}

	void world_t::release_text(u32 text_index)
	{
		if (text_index == ECS_INVALID_INDEX)
			return;

		SFG_ASSERT(text_index < _text_allocations.size());
		world_text_allocation_t& allocation = _text_allocations[text_index];
		_text_allocator.deallocate(allocation.allocated);
		allocation.allocated = nullptr;
		_text_allocation_free_list.push_back(text_index);
	}
}
