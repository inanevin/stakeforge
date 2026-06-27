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

#include "world_cook.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/ecs_component_type.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <algorithm>

namespace sfg
{
	namespace
	{
#define WORLD_COOK_RESOURCE_HANDLE_CASES                                                                                                                                                                                                                           \
	case reflected_value_type_e::resource:                                                                                                                                                                                                                         \
	case reflected_value_type_e::audio_handle:                                                                                                                                                                                                                     \
	case reflected_value_type_e::font_handle:                                                                                                                                                                                                                      \
	case reflected_value_type_e::mesh_handle:                                                                                                                                                                                                                      \
	case reflected_value_type_e::skeleton_handle:                                                                                                                                                                                                                  \
	case reflected_value_type_e::animation_handle:                                                                                                                                                                                                                 \
	case reflected_value_type_e::material_handle:                                                                                                                                                                                                                  \
	case reflected_value_type_e::shader_handle:                                                                                                                                                                                                                    \
	case reflected_value_type_e::texture_handle:                                                                                                                                                                                                                   \
	case reflected_value_type_e::texture_sampler_handle:                                                                                                                                                                                                           \
	case reflected_value_type_e::physical_material_handle:                                                                                                                                                                                                         \
	case reflected_value_type_e::prefab_handle:                                                                                                                                                                                                                    \
	case reflected_value_type_e::animation_state_machine_handle:                                                                                                                                                                                                   \
	case reflected_value_type_e::hdr_skybox_handle

		void add_unique_resource_handle(frame_vector_t<resource_handle_t>& out_resources, resource_handle_t handle)
		{
			if (handle == NULL_RESOURCE_HANDLE)
				return;

			const auto it = std::find(out_resources.begin(), out_resources.end(), handle);
			if (it == out_resources.end())
				out_resources.push_back(handle);
		}

		bool is_resource_reflected_type(reflected_value_type_e type)
		{
			switch (type)
			{
			WORLD_COOK_RESOURCE_HANDLE_CASES:
				return true;
			default:
				return false;
			}
		}

		bool is_reflected_container_ops_valid(const reflected_container_ops_t& ops)
		{
			return ops.get_count != nullptr && ops.get_const_item != nullptr;
		}

		bool is_entity_no_serialize(const world_t& world, entity_id_t entity)
		{
			const world_component_table_t* table = world.find_component_table(component_no_serialize_t::TYPE_ID);
			SFG_ASSERT(table != nullptr);
			return ecs_t::table_has(table->table, entity);
		}

		bool is_prefab_reference_entity(const world_t& world, entity_id_t entity)
		{
			const world_component_table_t* table = world.find_component_table(component_prefab_reference_t::TYPE_ID);
			SFG_ASSERT(table != nullptr);
			return ecs_t::table_has(table->table, entity);
		}

		bool is_guaranteed_prefab_component(sid_t type_id)
		{
			return type_id == component_hierarchy_t::TYPE_ID || type_id == component_guid_t::TYPE_ID || type_id == component_transform_t::TYPE_ID || type_id == component_name_t::TYPE_ID || type_id == component_prefab_reference_t::TYPE_ID;
		}

		bool should_serialize_component_table(const world_component_table_t& table, bool prefab_reference_entity)
		{
			const sid_t type_id = table.type_desc.type_id;
			if (type_id == component_alive_t::TYPE_ID || type_id == component_no_serialize_t::TYPE_ID)
				return false;
			if (prefab_reference_entity && !is_guaranteed_prefab_component(type_id))
				return false;

			const reflected_type_desc_t* reflected_type = reflection_registry_t::get().find_type(type_id);
			if (reflected_type != nullptr && (reflected_type->flags & reflected_type_flags_no_serialize) != 0 && !is_guaranteed_prefab_component(type_id))
				return false;
			return true;
		}

		const void* get_reflected_field_ptr(const void* object, const reflected_field_desc_t& field)
		{
			return static_cast<const u8*>(object) + field.offset;
		}

		resource_handle_t read_resource_handle(const void* object, const reflected_field_desc_t& field)
		{
			resource_handle_t handle = NULL_RESOURCE_HANDLE;
			if (field.get != nullptr)
			{
				field.get(object, field, &handle, field.user_data);
				return handle;
			}
			return *static_cast<const resource_handle_t*>(get_reflected_field_ptr(object, field));
		}

		size_t get_inplace_vector_size_offset(const reflected_field_desc_t& field)
		{
			const size_t data_size = static_cast<size_t>(field.stride) * field.capacity;
			const size_t alignment = alignof(size_t);
			return (data_size + alignment - 1) & ~(alignment - 1);
		}

		void collect_resource_handles_from_type(const void* object, const reflected_type_desc_t& type, frame_vector_t<resource_handle_t>& out_resources);

		void collect_resource_handles_from_field(const void* object, const reflected_field_desc_t& field, frame_vector_t<resource_handle_t>& out_resources)
		{
			if ((field.flags & reflected_field_flags_transient) != 0)
				return;

			if (is_resource_reflected_type(field.type))
			{
				add_unique_resource_handle(out_resources, read_resource_handle(object, field));
				return;
			}

			if (field.type == reflected_value_type_e::object)
			{
				const reflected_type_desc_t* type = reflection_registry_t::get().find_type(field.value_type_id);
				if (type != nullptr)
					collect_resource_handles_from_type(get_reflected_field_ptr(object, field), *type, out_resources);
				return;
			}

			if (field.type != reflected_value_type_e::vector && field.type != reflected_value_type_e::inplace_vector)
				return;

			const reflected_value_type_e item_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (is_reflected_container_ops_valid(field.container_ops))
			{
				const u32 count = field.container_ops.get_count(object, field);
				for (u32 i = 0; i < count; ++i)
				{
					const void* item = field.container_ops.get_const_item(object, field, i);
					if (is_resource_reflected_type(item_type))
						add_unique_resource_handle(out_resources, *static_cast<const resource_handle_t*>(item));
					else if (item_type == reflected_value_type_e::invalid)
					{
						const reflected_type_desc_t* item_reflected_type = reflection_registry_t::get().find_type(field.sub_type_id);
						if (item_reflected_type != nullptr)
							collect_resource_handles_from_type(item, *item_reflected_type, out_resources);
					}
				}
				return;
			}

			if (!is_resource_reflected_type(item_type))
				return;

			if (field.type == reflected_value_type_e::vector)
			{
				const vector_t<resource_handle_t>& handles = get_reflected_vector_field<resource_handle_t>(object, field);
				for (resource_handle_t handle : handles)
					add_unique_resource_handle(out_resources, handle);
				return;
			}

			const u8*	 data = static_cast<const u8*>(get_reflected_field_ptr(object, field));
			const size_t size = *reinterpret_cast<const size_t*>(data + get_inplace_vector_size_offset(field));
			for (size_t i = 0; i < size; ++i)
			{
				const resource_handle_t handle = *reinterpret_cast<const resource_handle_t*>(data + (i * field.stride));
				add_unique_resource_handle(out_resources, handle);
			}
		}

		void collect_resource_handles_from_type(const void* object, const reflected_type_desc_t& type, frame_vector_t<resource_handle_t>& out_resources)
		{
			for (u32 i = 0; i < type.fields.size; ++i)
				collect_resource_handles_from_field(object, type.fields.data[i], out_resources);
		}

		void collect_resource_handles_from_component(sid_t type_id, const void* component, frame_vector_t<resource_handle_t>& out_resources)
		{
			const reflected_type_desc_t* reflected_type = reflection_registry_t::get().find_type(type_id);
			if (reflected_type != nullptr)
				collect_resource_handles_from_type(component, *reflected_type, out_resources);
		}

		bool entity_to_json_impl(const world_t& world, entity_id_t entity, nlohmann::json& out_json, frame_vector_t<resource_handle_t>& out_resources)
		{
			if (is_entity_no_serialize(world, entity))
				return false;

			const bool prefab_reference_entity = is_prefab_reference_entity(world, entity);
			out_json						   = nlohmann::json::object();
			out_json["guid"]				   = world.get_entity_guid(entity);
			const char* name				   = world.get_entity_name(entity);
			out_json["name"]				   = name != nullptr ? name : "";
			out_json["components"]			   = nlohmann::json::array();
			out_json["children"]			   = nlohmann::json::array();

			for (const world_component_table_t& table : world.get_component_tables())
			{
				if (!should_serialize_component_table(table, prefab_reference_entity) || !ecs_t::table_has(table.table, entity))
					continue;

				nlohmann::json component_json = {};
				component_json["type_id"]	  = table.type_desc.type_id;
				component_json["data"]		  = nlohmann::json::object();
				if (!table.type_desc.flags.is_set(ecs_component_type_flags_tag))
				{
					const void* component = ecs_t::table_get(table.table, entity);
					if (!reflection_registry_t::get().serialize_to_json(table.type_desc.type_id, component, component_json["data"]))
						SFG_ASSERT(false);
					collect_resource_handles_from_component(table.type_desc.type_id, component, out_resources);
				}
				out_json["components"].push_back(component_json);
			}

			if (prefab_reference_entity)
				return true;

			const world_component_table_t* hierarchy_table = world.find_component_table(component_hierarchy_t::TYPE_ID);
			SFG_ASSERT(hierarchy_table != nullptr);
			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table->table, entity);
			for (entity_guid_t child_guid = hierarchy.first_child; child_guid != NULL_ENTITY_GUID;)
			{
				const entity_id_t			 child			 = world.get_entity_from_guid(child_guid);
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table->table, child);
				const entity_guid_t			 next_child_guid = child_hierarchy.next_sibling;
				nlohmann::json				 child_json		 = {};
				if (entity_to_json_impl(world, child, child_json, out_resources))
					out_json["children"].push_back(child_json);
				child_guid = next_child_guid;
			}

			return true;
		}
	}

	void world_cooker_t::world_to_stream(const world_t&, ostream_t&)
	{
	}

	void world_cooker_t::world_to_json(const world_t&, nlohmann::json&)
	{
	}

	void world_cooker_t::entity_to_stream(const world_t&, entity_id_t, ostream_t&)
	{
	}

	void world_cooker_t::entity_to_json(const world_t& world, entity_id_t entity, nlohmann::json& out_json, frame_vector_t<resource_handle_t>& out_resources)
	{
		if (!entity_to_json_impl(world, entity, out_json, out_resources))
			out_json = nullptr;
	}
}
