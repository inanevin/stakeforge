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

#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/prefab.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/world_cook.hpp>
#include <sfg/runtime/resources/world_cook_entity_header.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <new>

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
		_used_resources.reserve(512);

		add_component_table(ecs_helpers_t::make_component_desc<component_hierarchy_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_guid_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_transform_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_name_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_mesh_renderer_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_render_object_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_camera_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_skybox_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_prefab_reference_t>());
		add_component_table({
			.default_init = [](void* ptr) { new (ptr) component_debug_widgets_t{}; },
			.type_id	  = type_id_t<component_debug_widgets_t>::value,
			.size		  = sizeof(component_debug_widgets_t),
			.alignment	  = alignof(component_debug_widgets_t),
			.flags		  = ecs_component_type_flags_none,
			.debug_name	  = "debug_widgets_component",
		});
		add_component_table(ecs_helpers_t::make_tag_component_desc<component_alive_t>());
		add_component_table(ecs_helpers_t::make_tag_component_desc<component_disabled_t>());
		add_component_table(ecs_helpers_t::make_tag_component_desc<component_no_serialize_t>());
		add_component_table(ecs_helpers_t::make_component_desc<component_system_transform_t>());

		_engine_components.hierarchy_table = &get_component_table(type_id_t<component_hierarchy_t>::value)->table;
		_engine_components.guid_table	   = &get_component_table(type_id_t<component_guid_t>::value)->table;
		_engine_components.transform_table = &get_component_table(type_id_t<component_transform_t>::value)->table;
		_engine_components.name_table	   = &get_component_table(type_id_t<component_name_t>::value)->table;
		_engine_components.alive_table	   = &get_component_table(type_id_t<component_alive_t>::value)->table;
		_engine_components.prefab_table	   = &get_component_table(type_id_t<component_prefab_reference_t>::value)->table;
		_system_components.transform_table = &get_component_table(type_id_t<component_system_transform_t>::value)->table;
	}

	void world_t::uninit()
	{
		for (world_component_table_t& table : _component_tables)
			ecs_t::table_uninit(table.table);

		_used_resources.resize(0);
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

	entity_id_t world_t::create_entity(const char* name, entity_guid_t guid)
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

		if (guid == NULL_ENTITY_GUID)
		{
			do
			{
				guid = hashing_t::generate_guid64();
			} while (guid == NULL_ENTITY_GUID || get_entity_from_guid(guid) != NULL_ENTITY_ID);
		}
		else
		{
			SFG_ASSERT(get_entity_from_guid(guid) == NULL_ENTITY_ID);
		}

		ecs_t::table_add(*_engine_components.alive_table, id);
		ecs_helpers_t::table_add_or_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, id);

		component_guid_t& guid_component = ecs_helpers_t::table_add_or_get_as<component_guid_t>(*_engine_components.guid_table, id);
		guid_component.guid				 = guid;

		ecs_helpers_t::table_add_or_get_as<component_transform_t>(*_engine_components.transform_table, id);
		component_name_t& name_component = ecs_helpers_t::table_add_or_get_as<component_name_t>(*_engine_components.name_table, id);

		if (name == nullptr)
			name_component.text[0] = '\0';
		else
		{
			const size_t name_len  = std::strlen(name);
			const size_t text_size = sizeof(name_component.text);
			const size_t copy_len  = name_len < text_size ? name_len : text_size - 1;
			SFG_MEMCPY((void*)name_component.text, name, copy_len);
			name_component.text[copy_len] = '\0';
		}

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

		for (world_component_table_t& t : _component_tables)
		{
			ecs_t::table_remove(t.table, id);
		}

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

		if (name == nullptr)
			name_component.text[0] = '\0';
		else
		{
			const size_t name_len  = std::strlen(name);
			const size_t text_size = sizeof(name_component.text);
			const size_t copy_len  = name_len < text_size ? name_len : text_size - 1;
			SFG_MEMCPY((void*)name_component.text, name, copy_len);
			name_component.text[copy_len] = '\0';
		}
	}

	entity_id_t world_t::spawn_prefab(resource_handle_t handle, const prefab_spawn_params_t& params)
	{
		resource_manager_t::get().load_resource(handle, resource_type_e::prefab, false, true);

		const prefab_internals_t* prefab_data = resource_manager_t::get().find_internals<prefab_internals_t>(handle);
		if (prefab_data == nullptr)
			return NULL_ENTITY_ID;

		return spawn_prefab(handle, *prefab_data, params);
	}

	entity_id_t world_t::spawn_prefab(resource_handle_t handle, const prefab_internals_t& prefab_data, const prefab_spawn_params_t& params)
	{
		const char*			 prefab_source = resource_manager_t::get().get_memory().get_text(prefab_data.source);
		const nlohmann::json prefab_json   = nlohmann::json::parse(prefab_source, nullptr, false);
		if (prefab_json.is_discarded())
			return NULL_ENTITY_ID;

		const entity_id_t root = world_cooker_t::entity_from_json(*this, prefab_json, true, false);
		if (root == NULL_ENTITY_ID)
			return NULL_ENTITY_ID;

		if (params.parent != NULL_ENTITY_ID)
			attach_to(root, params.parent);
		set_entity_pos_local(root, params.local_pos);
		set_entity_rot_local(root, params.local_rot);
		set_entity_scale_local(root, params.local_scale);
		sync_entity_hierarchy(root);
		scan_for_resources(root);

		make_prefab_chain(root, handle);

		return root;
	}

	void world_t::make_prefab_chain(entity_id_t root, resource_handle_t handle)
	{
		SFG_ASSERT(is_alive(root));
		SFG_ASSERT(handle != NULL_RESOURCE_HANDLE);

		const auto scan = [&](const auto& self, entity_id_t current) -> void {
			component_prefab_reference_t& ref = ecs_helpers_t::table_add_or_get_as<component_prefab_reference_t>(*_engine_components.prefab_table, current);
			ref.is_root						  = current == root;
			ref.prefab						  = handle;

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, current);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
				const entity_id_t			 next_child		 = child_hierarchy.next_sibling;
				self(self, child);
				child = next_child;
			}
		};
		scan(scan, root);

		if (add_resource(resource_type_e::prefab, handle))
			load_all_used_resources();
	}

	void world_t::break_prefab_chain(entity_id_t root)
	{
		SFG_ASSERT(is_alive(root));

		const auto scan = [&](const auto& self, entity_id_t current) -> void {
			if (ecs_t::table_has(*_engine_components.prefab_table, current))
				ecs_t::table_remove(*_engine_components.prefab_table, current);

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, current);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
				const entity_id_t			 next_child		 = child_hierarchy.next_sibling;
				self(self, child);
				child = next_child;
			}
		};
		scan(scan, root);
	}

	entity_id_t world_t::get_entity_parent(entity_id_t id) const
	{
		SFG_ASSERT(is_alive(id));
		const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(*_engine_components.hierarchy_table, id);
		return hierarchy.parent;
	}

	entity_guid_t world_t::get_entity_guid(entity_id_t id) const
	{
		if (id == NULL_ENTITY_ID)
			return NULL_ENTITY_GUID;

		SFG_ASSERT(is_alive(id));
		const component_guid_t& guid = ecs_helpers_t::table_get_as_const<component_guid_t>(*_engine_components.guid_table, id);
		return guid.guid;
	}

	entity_id_t world_t::get_entity_from_guid(entity_guid_t guid) const
	{
		return find_by_guid(guid);
	}

	entity_id_t world_t::find_by_guid(entity_guid_t guid) const
	{
		if (guid == NULL_ENTITY_GUID)
			return NULL_ENTITY_ID;

		const ecs_component_table_ref_t table_refs[] = {
			_engine_components.alive_table->ref(),
			_engine_components.guid_table->ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_guid_t& g = ecs_helpers_t::row_get<component_guid_t>(row, 1);
			if (g.guid == guid)
				return row.id;
		}

		return NULL_ENTITY_ID;
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

	bool world_t::add_resource(resource_type_e type, resource_handle_t handle)
	{
		auto it = std::find_if(_used_resources.begin(), _used_resources.end(), [handle](const world_resource_t& r) -> bool { return r.handle == handle; });
		if (it != _used_resources.end())
			return false;
		_used_resources.push_back({.handle = handle, .type = type});
		return true;
	}

	void world_t::scan_for_resources(entity_id_t entity)
	{
		SFG_ASSERT(is_alive(entity));

		bool	   added = false;
		const auto scan	 = [&](const auto& self, entity_id_t current) -> void {
			 reflection_registry_t& registry = reflection_registry_t::get();
			 for (world_component_table_t& component_table : _component_tables)
			 {
				 if (!ecs_t::table_has(component_table.table, current))
					 continue;

				 const reflected_type_t* type = registry.find_type(component_table.table.component_type_id);
				 if (type == nullptr || type->fields.start == type->fields.end)
					 continue;

				 void* component = ecs_t::table_get(component_table.table, current);
				 SFG_ASSERT(component != nullptr);

				 for (u32 i = type->fields.start; i < type->fields.end; ++i)
				 {
					 const reflected_field_t* field = registry.get_field(i);
					 SFG_ASSERT(field != nullptr);
					 if (field->value_type != reflected_value_type_e::u64)
						 continue;

					 const resource_type_e resource_type = resource_type_from_reflection_sub_type_id(field->sub_type_id);
					 if (resource_type == resource_type_e::invalid)
						 continue;

					 const resource_handle_t handle = *reinterpret_cast<const resource_handle_t*>(static_cast<const u8*>(component) + field->offset);
					 if (handle != NULL_RESOURCE_HANDLE)
						 added = add_resource(resource_type, handle) || added;
				 }
			 }

			 const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, current);
			 for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			 {
				 const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as<component_hierarchy_t>(*_engine_components.hierarchy_table, child);
				 const entity_id_t			  next_child	  = child_hierarchy.next_sibling;
				 self(self, child);
				 child = next_child;
			 }
		};
		scan(scan, entity);

		if (added)
			load_all_used_resources();
	}

	void world_t::load_all_used_resources()
	{
		std::stable_sort(_used_resources.begin(), _used_resources.end(), [](const world_resource_t& a, const world_resource_t& b) {
			const auto get_load_priority = [](resource_type_e type) -> u8 {
				switch (type)
				{
				case resource_type_e::shader:
					return 0;
				case resource_type_e::texture:
					return 1;
				case resource_type_e::texture_sampler:
					return 2;
				default:
					return 3;
				}
			};
			return get_load_priority(a.type) < get_load_priority(b.type);
		});

		resource_manager_t& rm = resource_manager_t::get();
		for (world_resource_t& res : _used_resources)
		{
			if (res.loaded)
				continue;

			rm.load_resource(res.handle, res.type);
			res.loaded = rm.find_entry(res.handle) != nullptr;
		}
	}

	void world_t::unload_all_used_resources()
	{
		resource_manager_t& rm = resource_manager_t::get();
		for (auto it = _used_resources.rbegin(); it != _used_resources.rend(); ++it)
		{
			if (!it->loaded)
				continue;

			rm.unload_resource(it->handle);
			it->loaded = false;
		}
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
		return name.text;
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
