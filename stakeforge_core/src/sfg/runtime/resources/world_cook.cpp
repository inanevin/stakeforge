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
#include "world_cook_entity_header.hpp"

#include <sfg/data/ostream.hpp>
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
		void add_unique_resource_handle(frame_vector_t<resource_handle_t>& out_resources, resource_handle_t handle)
		{
			if (handle == NULL_RESOURCE_HANDLE)
				return;

			const auto it = std::find(out_resources.begin(), out_resources.end(), handle);
			if (it == out_resources.end())
				out_resources.push_back(handle);
		}

		bool should_serialize_component_table(const world_component_table_t& table)
		{
			const sid_t type_id = table.type_desc.type_id;

			const reflected_type_desc_t* reflected_type = reflection_registry_t::get().find_type(type_id);
			if (reflected_type == nullptr)
				return false;

			if (reflected_type->flags.is_set(reflected_type_flags_no_serialize))
				return false;

			return true;
		}

		size_t get_inplace_vector_size_offset(const reflected_field_desc_t& field)
		{
			const size_t data_size = static_cast<size_t>(field.stride) * field.capacity;
			const size_t alignment = alignof(size_t);
			return (data_size + alignment - 1) & ~(alignment - 1);
		}

		const u8* get_reflected_field_ptr(const void* object, const reflected_field_desc_t& field)
		{
			return static_cast<const u8*>(object) + field.offset;
		}

		const char* get_reflected_text(const world_t& world, u32 text_id)
		{
			const char* text = world.get_text(text_id);
			return text != nullptr ? text : "";
		}

		bool reflected_type_has_text_id(const reflected_type_desc_t& type)
		{
			for (u32 i = 0; i < type.fields.size; i++)
			{
				const reflected_field_desc_t& field = type.fields.data[i];
				if (field.type == reflected_value_type_e::text_id)
					return true;
				if ((field.type == reflected_value_type_e::vector || field.type == reflected_value_type_e::inplace_vector) && reflected_value_type_from_sub_type_id(field.sub_type_id) == reflected_value_type_e::text_id)
					return true;
				if (field.type == reflected_value_type_e::object)
				{
					const reflected_type_desc_t* field_type = reflection_registry_t::get().find_type(field.value_type_id);
					if (field_type != nullptr && reflected_type_has_text_id(*field_type))
						return true;
				}
			}
			return false;
		}

		bool write_reflected_text_vector_to_json(const world_t& world, const void* object, const reflected_field_desc_t& field, nlohmann::json& json)
		{
			json = nlohmann::json::array();

			if (field.type == reflected_value_type_e::vector)
			{
				const vector_t<u32>& text_ids = *reinterpret_cast<const vector_t<u32>*>(get_reflected_field_ptr(object, field));
				for (u32 text_id : text_ids)
					json.push_back(get_reflected_text(world, text_id));
				return true;
			}

			const u8*	 data = get_reflected_field_ptr(object, field);
			const size_t size = *reinterpret_cast<const size_t*>(data + get_inplace_vector_size_offset(field));
			for (size_t i = 0; i < size; ++i)
			{
				const u32 text_id = *reinterpret_cast<const u32*>(data + (i * field.stride));
				json.push_back(get_reflected_text(world, text_id));
			}
			return true;
		}

		bool write_reflected_text_vector_to_stream(const world_t& world, const void* object, const reflected_field_desc_t& field, ostream_t& stream)
		{
			if (field.type == reflected_value_type_e::vector)
			{
				const vector_t<u32>& text_ids = *reinterpret_cast<const vector_t<u32>*>(get_reflected_field_ptr(object, field));
				stream << static_cast<u32>(text_ids.size());
				for (u32 text_id : text_ids)
					stream << string_t(get_reflected_text(world, text_id));
				return true;
			}

			const u8*	 data = get_reflected_field_ptr(object, field);
			const size_t size = *reinterpret_cast<const size_t*>(data + get_inplace_vector_size_offset(field));
			stream << static_cast<u32>(size);
			for (size_t i = 0; i < size; ++i)
			{
				const u32 text_id = *reinterpret_cast<const u32*>(data + (i * field.stride));
				stream << string_t(get_reflected_text(world, text_id));
			}
			return true;
		}

		bool apply_reflected_world_text_to_json(const world_t& world, const void* object, const reflected_type_desc_t& type, nlohmann::json& json)
		{
			for (u32 i = 0; i < type.fields.size; ++i)
			{
				const reflected_field_desc_t& field = type.fields.data[i];
				SFG_ASSERT(field.name != nullptr);

				if (field.type == reflected_value_type_e::text_id)
				{
					const u32 text_id = *reinterpret_cast<const u32*>(get_reflected_field_ptr(object, field));
					json[field.name]  = get_reflected_text(world, text_id);
					continue;
				}

				if (field.type == reflected_value_type_e::object)
				{
					const reflected_type_desc_t* field_type = reflection_registry_t::get().find_type(field.value_type_id);
					if (field_type != nullptr && !apply_reflected_world_text_to_json(world, get_reflected_field_ptr(object, field), *field_type, json[field.name]))
						return false;
					continue;
				}

				if ((field.type == reflected_value_type_e::vector || field.type == reflected_value_type_e::inplace_vector) && reflected_value_type_from_sub_type_id(field.sub_type_id) == reflected_value_type_e::text_id)
				{
					if (!write_reflected_text_vector_to_json(world, object, field, json[field.name]))
						return false;
				}
			}
			return true;
		}

		bool serialize_reflected_type_to_json(const world_t& world, const void* object, const reflected_type_desc_t& type, nlohmann::json& json)
		{
			if (!reflection_registry_t::get().serialize_to_json(type.type_id, object, json))
				return false;

			return apply_reflected_world_text_to_json(world, object, type, json);
		}

		bool serialize_reflected_field_to_stream(const world_t& world, const void* object, const reflected_field_desc_t& field, ostream_t& stream);

		bool serialize_reflected_type_to_stream(const world_t& world, const void* object, const reflected_type_desc_t& type, ostream_t& stream)
		{
			if (type.fields.size == 0)
				return reflection_registry_t::get().serialize_to_stream(type.type_id, object, stream);

			for (u32 i = 0; i < type.fields.size; ++i)
			{
				if (!serialize_reflected_field_to_stream(world, object, type.fields.data[i], stream))
					return false;
			}
			return true;
		}

		bool serialize_reflected_field_to_stream(const world_t& world, const void* object, const reflected_field_desc_t& field, ostream_t& stream)
		{
			if (field.type == reflected_value_type_e::text_id)
			{
				const u32 text_id = *reinterpret_cast<const u32*>(get_reflected_field_ptr(object, field));
				stream << string_t(get_reflected_text(world, text_id));
				return true;
			}

			if (field.type == reflected_value_type_e::object)
			{
				const reflected_type_desc_t* field_type = reflection_registry_t::get().find_type(field.value_type_id);
				if (field_type == nullptr)
					return false;
				return serialize_reflected_type_to_stream(world, get_reflected_field_ptr(object, field), *field_type, stream);
			}

			if ((field.type == reflected_value_type_e::vector || field.type == reflected_value_type_e::inplace_vector) && reflected_value_type_from_sub_type_id(field.sub_type_id) == reflected_value_type_e::text_id)
				return write_reflected_text_vector_to_stream(world, object, field, stream);

			return reflection_registry_t::get().serialize_field_to_stream(object, field, stream);
		}

		world_cook_entity_header_t make_entity_header(const world_t& world, entity_id_t entity, bool ignore_prefab_reference = false)
		{
			const world_component_table_t* transform_table = world.find_component_table(type_id_t<component_transform_t>::value);
			SFG_ASSERT(transform_table != nullptr);
			const world_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
			SFG_ASSERT(hierarchy_table != nullptr);

			const component_transform_t& transform = ecs_helpers_t::table_get_as_const<component_transform_t>(transform_table->table, entity);
			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table->table, entity);
			const char*					 name	   = world.get_entity_name(entity);
			resource_handle_t			 prefab	   = NULL_RESOURCE_HANDLE;
			if (!ignore_prefab_reference)
			{
				const world_component_table_t* prefab_reference_table = world.find_component_table(type_id_t<component_prefab_reference_t>::value);
				SFG_ASSERT(prefab_reference_table != nullptr);
				if (ecs_t::table_has(prefab_reference_table->table, entity))
					prefab = ecs_helpers_t::table_get_as_const<component_prefab_reference_t>(prefab_reference_table->table, entity).prefab;
			}

			return {
				.guid		 = world.get_entity_guid(entity),
				.parent_guid = world.get_entity_guid(hierarchy.parent),
				.name		 = name != nullptr ? name : "",
				.local_pos	 = transform.pos,
				.local_rot	 = transform.rot,
				.local_scale = transform.scale,
				.prefab		 = prefab,
			};
		}

		void collect_resource_handles_from_type(const void* object, const reflected_type_desc_t& type, frame_vector_t<resource_handle_t>& out_resources);

		void collect_resource_handles_from_field(const void* object, const reflected_field_desc_t& field, frame_vector_t<resource_handle_t>& out_resources)
		{
			if (reflected_value_type_is_resource(field.type))
			{
				add_unique_resource_handle(out_resources, *reinterpret_cast<const resource_handle_t*>(static_cast<const u8*>(object) + field.offset));
				return;
			}

			if (field.type == reflected_value_type_e::object)
			{
				const reflected_type_desc_t* type = reflection_registry_t::get().find_type(field.value_type_id);
				if (type != nullptr)
					collect_resource_handles_from_type(static_cast<const u8*>(object) + field.offset, *type, out_resources);
				return;
			}

			if (field.type != reflected_value_type_e::vector && field.type != reflected_value_type_e::inplace_vector)
				return;

			const reflected_value_type_e item_type = reflected_value_type_from_sub_type_id(field.sub_type_id);
			if (!reflected_value_type_is_resource(item_type))
				return;

			if (field.type == reflected_value_type_e::vector)
			{
				const vector_t<resource_handle_t>& handles = *reinterpret_cast<const vector_t<resource_handle_t>*>(static_cast<const u8*>(object) + field.offset);
				for (resource_handle_t handle : handles)
					add_unique_resource_handle(out_resources, handle);
				return;
			}

			const u8*	 data = static_cast<const u8*>(object) + field.offset;
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

		bool is_entity_no_serialize(const world_t& world, entity_id_t entity)
		{
			const world_component_table_t* table = world.find_component_table(type_id_t<component_no_serialize_t>::value);
			SFG_ASSERT(table != nullptr);
			return ecs_t::table_has(table->table, entity);
		}

		u32 get_serialized_component_count(const world_t& world, entity_id_t entity)
		{
			u32 count = 0;
			for (const world_component_table_t& table : world.get_component_tables())
			{
				if (should_serialize_component_table(table) && ecs_t::table_has(table.table, entity))
					count++;
			}
			return count;
		}

		u32 get_serialized_child_count(const world_t& world, const component_hierarchy_t& hierarchy, const world_component_table_t& hierarchy_table)
		{
			u32 count = 0;
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table.table, child);
				if (!is_entity_no_serialize(world, child))
					count++;
				child = child_hierarchy.next_sibling;
			}
			return count;
		}

		bool entity_to_json_impl(const world_t& world, entity_id_t entity, nlohmann::json& out_json, frame_vector_t<resource_handle_t>& out_resources, bool ignore_prefab_reference = false)
		{
			if (is_entity_no_serialize(world, entity))
				return false;

			const world_cook_entity_header_t header = make_entity_header(world, entity, ignore_prefab_reference);
			add_unique_resource_handle(out_resources, header.prefab);

			out_json		   = nlohmann::json::object();
			out_json["header"] = header;

			if (header.prefab != NULL_RESOURCE_HANDLE)
				return true;

			out_json["components"] = nlohmann::json::array();
			out_json["children"]   = nlohmann::json::array();

			for (const world_component_table_t& table : world.get_component_tables())
			{
				if (!should_serialize_component_table(table) || !ecs_t::table_has(table.table, entity))
					continue;

				nlohmann::json component_json = {};
				component_json["type_id"]	  = table.type_desc.type_id;
				component_json["data"]		  = nlohmann::json::object();
				if (!table.type_desc.flags.is_set(ecs_component_type_flags_tag))
				{
					const void*					 component		= ecs_t::table_get(table.table, entity);
					const reflected_type_desc_t* reflected_type = reflection_registry_t::get().find_type(table.type_desc.type_id);
					SFG_ASSERT(reflected_type != nullptr);
					if (!serialize_reflected_type_to_json(world, component, *reflected_type, component_json["data"]))
						SFG_ASSERT(false);

					collect_resource_handles_from_type(component, *reflected_type, out_resources);
				}
				out_json["components"].push_back(component_json);
			}

			const world_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
			SFG_ASSERT(hierarchy_table != nullptr);

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table->table, entity);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table->table, child);
				const entity_id_t			 next_child		 = child_hierarchy.next_sibling;
				nlohmann::json				 child_json		 = {};

				if (entity_to_json_impl(world, child, child_json, out_resources))
					out_json["children"].push_back(child_json);

				child = next_child;
			}

			return true;
		}

		bool entity_to_stream_impl(const world_t& world, entity_id_t entity, ostream_t& out_stream, frame_vector_t<resource_handle_t>& out_resources)
		{
			if (is_entity_no_serialize(world, entity))
				return false;

			const world_cook_entity_header_t header = make_entity_header(world, entity);
			add_unique_resource_handle(out_resources, header.prefab);

			out_stream << header;

			if (header.prefab != NULL_RESOURCE_HANDLE)
				return true;

			out_stream << get_serialized_component_count(world, entity);
			for (const world_component_table_t& table : world.get_component_tables())
			{
				if (!should_serialize_component_table(table) || !ecs_t::table_has(table.table, entity))
					continue;

				out_stream << table.type_desc.type_id;
				if (!table.type_desc.flags.is_set(ecs_component_type_flags_tag))
				{
					const void*					 component		= ecs_t::table_get(table.table, entity);
					const reflected_type_desc_t* reflected_type = reflection_registry_t::get().find_type(table.type_desc.type_id);
					SFG_ASSERT(reflected_type != nullptr);
					const bool serialized =
						reflected_type_has_text_id(*reflected_type) ? serialize_reflected_type_to_stream(world, component, *reflected_type, out_stream) : reflection_registry_t::get().serialize_to_stream(table.type_desc.type_id, component, out_stream);
					if (!serialized)
						SFG_ASSERT(false);

					collect_resource_handles_from_type(component, *reflected_type, out_resources);
				}
			}

			const world_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
			SFG_ASSERT(hierarchy_table != nullptr);

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table->table, entity);
			out_stream << get_serialized_child_count(world, hierarchy, *hierarchy_table);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table->table, child);
				const entity_id_t			 next_child		 = child_hierarchy.next_sibling;

				entity_to_stream_impl(world, child, out_stream, out_resources);
				child = next_child;
			}

			return true;
		}
	}

	void world_cooker_t::world_to_stream(const world_t& world, ostream_t& out_stream)
	{
		const world_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
		SFG_ASSERT(hierarchy_table != nullptr);

		frame_vector_t<entity_id_t>		  roots;
		frame_vector_t<resource_handle_t> resources;
		roots.reserve(64);
		resources.reserve(64);

		const ecs_component_table_ref_t table_refs[] = {
			hierarchy_table->table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = 1}))
		{
			const component_hierarchy_t& hierarchy = *static_cast<const component_hierarchy_t*>(row.components[0]);
			if (hierarchy.parent == NULL_ENTITY_ID && !is_entity_no_serialize(world, row.id))
				roots.push_back(row.id);
		}

		out_stream << static_cast<u32>(roots.size());
		for (entity_id_t root : roots)
			entity_to_stream(world, root, out_stream, resources);

		out_stream << static_cast<u32>(resources.size());
		for (resource_handle_t resource : resources)
			out_stream << resource;
	}

	void world_cooker_t::world_to_json(const world_t& world, nlohmann::json& out_json)
	{
		const world_component_table_t* hierarchy_table = world.find_component_table(type_id_t<component_hierarchy_t>::value);
		SFG_ASSERT(hierarchy_table != nullptr);

		frame_vector_t<resource_handle_t> resources;
		resources.reserve(64);

		out_json			  = nlohmann::json::object();
		out_json["entities"]  = nlohmann::json::array();
		out_json["resources"] = nlohmann::json::array();

		const ecs_component_table_ref_t table_refs[] = {
			hierarchy_table->table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = 1}))
		{
			const component_hierarchy_t& hierarchy = *static_cast<const component_hierarchy_t*>(row.components[0]);
			if (hierarchy.parent != NULL_ENTITY_ID)
				continue;

			nlohmann::json entity_json = {};
			entity_to_json(world, row.id, entity_json, resources);
			if (!entity_json.is_null())
				out_json["entities"].push_back(entity_json);
		}

		for (resource_handle_t resource : resources)
			out_json["resources"].push_back(resource);
	}

	void world_cooker_t::entity_to_stream(const world_t& world, entity_id_t entity, ostream_t& out_stream, frame_vector_t<resource_handle_t>& out_resources)
	{
		entity_to_stream_impl(world, entity, out_stream, out_resources);
	}

	void world_cooker_t::entity_to_json(const world_t& world, entity_id_t entity, nlohmann::json& out_json, frame_vector_t<resource_handle_t>& out_resources)
	{
		if (!entity_to_json_impl(world, entity, out_json, out_resources))
			out_json = nullptr;
	}

	void world_cooker_t::entity_to_prefab_json(const world_t& world, entity_id_t entity, nlohmann::json& out_json)
	{
		frame_vector_t<resource_handle_t> resources;

		if (!entity_to_json_impl(world, entity, out_json, resources, true))
			out_json = nullptr;

		if (out_json.is_null())
			return;

		out_json["resources"] = nlohmann::json::array();
		for (resource_handle_t h : resources)
			out_json["resources"].push_back(h);
	}
}
