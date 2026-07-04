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

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/memory.hpp>
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
		struct read_entity_t
		{
			world_cook_entity_header_t header	= {};
			entity_guid_t			   new_guid = NULL_ENTITY_GUID;
			entity_id_t				   entity	= NULL_ENTITY_ID;
		};

		void fix_entity_guid_references(void* component, reflected_field_span_t fields, const frame_vector_t<read_entity_t>& read_entities)
		{
			for (u32 i = fields.start; i < fields.end; i++)
			{
				const reflected_field_t* field = reflection_registry_t::get().get_field(i);
				if (field == nullptr)
					continue;

				if (field->value_type != reflected_value_type_e::u64 || field->sub_type_id != REFLECTION_SUB_TYPE_IDENTIFIER_ENTITY_GUID)
					continue;

				u64& entity_ref_guid = *reinterpret_cast<u64*>(reinterpret_cast<u8*>(component) + field->offset);
				auto it				 = std::find_if(read_entities.begin(), read_entities.end(), [entity_ref_guid](const read_entity_t& e) -> bool { return e.header.guid == entity_ref_guid; });
				if (it != read_entities.end())
					entity_ref_guid = it->new_guid;
			}
		}
	}

	void world_cooker_t::world_to_stream(const world_t& world, ostream_t& out_stream)
	{
	}

	void world_cooker_t::world_to_json(const world_t& world, nlohmann::json& out_json)
	{
	}

	void world_cooker_t::entity_to_stream(const world_t& world, entity_id_t entity, ostream_t& out_stream, frame_vector_t<resource_handle_t>& out_resources)
	{
		SFG_ASSERT(world.is_alive(entity));

		out_resources.resize(0);

		struct written_entity_t
		{
			entity_id_t	  entity = NULL_ENTITY_ID;
			entity_guid_t guid	 = NULL_ENTITY_GUID;
		};

		frame_vector_t<written_entity_t> written_entities;

		const world_component_table_t* hierarchy_world_table	= world.find_component_table(type_id_t<component_hierarchy_t>::value);
		const ecs_component_table_t&   hierarchy_table			= hierarchy_world_table->table;
		const world_component_table_t* no_serialize_world_table = world.find_component_table(type_id_t<component_no_serialize_t>::value);
		const ecs_component_table_t&   no_serialize_table		= no_serialize_world_table->table;

		const size_t count_offset = out_stream.get_size();
		u32			 total_count  = 0;
		out_stream << total_count;

		const auto write_entity = [&](const auto& self, entity_id_t current) -> void {
			if (ecs_t::table_has(no_serialize_table, current))
				return;

			total_count++;

			const entity_guid_t				 guid	= world.get_entity_guid(current);
			const entity_id_t				 parent = world.get_entity_parent(current);
			const world_cook_entity_header_t header{
				.guid		 = guid,
				.parent_guid = world.get_entity_guid(parent),
				.name		 = world.get_entity_name(current),
				.local_pos	 = world.get_entity_pos_local(current),
				.local_rot	 = world.get_entity_rot_local(current),
				.local_scale = world.get_entity_scale_local(current),
			};
			out_stream << header;
			written_entities.push_back({.entity = current, .guid = guid});

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, current);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, child);
				self(self, child);
				child = child_hierarchy.next_sibling;
			}
		};

		write_entity(write_entity, entity);
		SFG_MEMCPY(out_stream.get_raw() + count_offset, &total_count, sizeof(total_count));

		const vector_t<world_component_table_t>& component_tables		= world.get_component_tables();
		const size_t							 component_count_offset = out_stream.get_size();
		u32										 component_count		= 0;
		out_stream << component_count;

		for (const written_entity_t& written_entity : written_entities)
		{
			for (const world_component_table_t& component_table : component_tables)
			{
				const sid_t				component_type_id = component_table.type_desc.type_id;
				const reflected_type_t* reflected_type	  = reflection_registry_t::get().find_type(component_type_id);
				SFG_ASSERT(reflected_type != nullptr);

				if (reflected_type->flags.is_set(reflected_type_flag_no_serialization))
					continue;

				if (!ecs_t::table_has(component_table.table, written_entity.entity))
					continue;

				ostream_t component_stream;
				void*	  component = ecs_t::table_get(component_table.table, written_entity.entity);
				if (component != nullptr)
				{
					const bool serialized = reflection_registry_t::get().type_to_stream(component_type_id, component, nullptr, component_stream);
					SFG_ASSERT(serialized);
				}

				const u32 component_size = static_cast<u32>(component_stream.get_size());
				component_count++;
				out_stream << component_type_id << component_size << written_entity.guid;
				if (component_size != 0)
					out_stream.write_raw(component_stream.get_raw(), component_size);
			}
		}

		SFG_MEMCPY(out_stream.get_raw() + component_count_offset, &component_count, sizeof(component_count));
	}

	entity_id_t world_cooker_t::entity_from_stream(world_t& world, istream_t& in_stream, bool generate_new_guids, bool sync_hierarchy)
	{
		u32 entity_count = 0;
		in_stream >> entity_count;
		frame_vector_t<read_entity_t> read_entities;
		read_entities.reserve(entity_count);

		for (u32 i = 0; i < entity_count; ++i)
		{
			world_cook_entity_header_t header;
			in_stream >> header;
			const entity_id_t		 entity		= world.create_entity(header.name.c_str(), generate_new_guids ? NULL_ENTITY_GUID : header.guid);
			world_component_table_t* world_guid = world.get_component_table(type_id_t<component_guid_t>::value);
			SFG_ASSERT(world_guid);
			component_guid_t& guid = ecs_helpers_t::table_get_as<component_guid_t>(world_guid->table, entity);
			read_entities.push_back({.header = header, .new_guid = guid.guid, .entity = entity});
		}

		for (const read_entity_t& read_entity : read_entities)
		{
			if (read_entity.header.parent_guid != NULL_ENTITY_GUID)
			{
				const auto parent_it = std::find_if(read_entities.begin(), read_entities.end(), [&](const read_entity_t& other) { return other.header.guid == read_entity.header.parent_guid; });
				SFG_ASSERT(parent_it != read_entities.end());
				world.attach_to(read_entity.entity, parent_it->entity);
			}

			world.set_entity_pos_local(read_entity.entity, read_entity.header.local_pos);
			world.set_entity_rot_local(read_entity.entity, read_entity.header.local_rot);
			world.set_entity_scale_local(read_entity.entity, read_entity.header.local_scale);
		}

		u32 component_count = 0;
		in_stream >> component_count;
		for (u32 i = 0; i < component_count; ++i)
		{
			sid_t		  component_type_id	 = 0;
			u32			  component_size	 = 0;
			entity_guid_t target_entity_guid = NULL_ENTITY_GUID;
			in_stream >> component_type_id >> component_size >> target_entity_guid;

			const reflected_type_t*	 reflected_type	 = reflection_registry_t::get().find_type(component_type_id);
			world_component_table_t* component_table = world.find_component_table(component_type_id);
			const auto				 target_it		 = std::find_if(read_entities.begin(), read_entities.end(), [&](const read_entity_t& other) { return other.header.guid == target_entity_guid; });
			if (reflected_type == nullptr || component_table == nullptr || target_it == read_entities.end())
			{
				in_stream.skip_by(component_size);
				continue;
			}

			const reflected_field_span_t fields = reflected_type->fields;

			void* component = ecs_t::table_add(component_table->table, target_it->entity);
			if (component != nullptr)
			{
				component_table->type_desc.default_init(component);
				istream_t component_stream;
				component_stream.open(in_stream.get_data_current(), component_size);
				const bool deserialized = reflection_registry_t::get().type_from_stream(component_type_id, component, nullptr, component_stream);
				SFG_ASSERT(deserialized);

				if (generate_new_guids)
					fix_entity_guid_references(component, fields, read_entities);
			}

			in_stream.skip_by(component_size);
		}

		const entity_id_t root_entity = read_entities.empty() ? NULL_ENTITY_ID : read_entities[0].entity;
		if (sync_hierarchy && root_entity != NULL_ENTITY_ID)
			world.sync_entity_hierarchy(root_entity);
		return root_entity;
	}

	entity_id_t world_cooker_t::entity_from_json(world_t& world, const nlohmann::json& in_json, bool generate_new_guids, bool sync_hierarchy)
	{
		if (!in_json.is_object())
			return NULL_ENTITY_ID;

		const nlohmann::json entities_json = in_json.value<nlohmann::json>("local_entities", nlohmann::json::array());
		if (!entities_json.is_array())
			return NULL_ENTITY_ID;

		frame_vector_t<read_entity_t> read_entities;
		read_entities.reserve(entities_json.size());

		for (const nlohmann::json& entity_json : entities_json)
		{
			if (!entity_json.is_object())
				return NULL_ENTITY_ID;

			const world_cook_entity_header_t header		= entity_json.get<world_cook_entity_header_t>();
			const entity_id_t				 entity		= world.create_entity(header.name.c_str(), generate_new_guids ? NULL_ENTITY_GUID : header.guid);
			world_component_table_t*		 world_guid = world.get_component_table(type_id_t<component_guid_t>::value);
			SFG_ASSERT(world_guid);
			component_guid_t& guid = ecs_helpers_t::table_get_as<component_guid_t>(world_guid->table, entity);
			read_entities.push_back({.header = header, .new_guid = guid.guid, .entity = entity});
		}

		for (const read_entity_t& read_entity : read_entities)
		{
			if (read_entity.header.parent_guid != NULL_ENTITY_GUID)
			{
				const auto parent_it = std::find_if(read_entities.begin(), read_entities.end(), [&](const read_entity_t& other) { return other.header.guid == read_entity.header.parent_guid; });
				SFG_ASSERT(parent_it != read_entities.end());
				world.attach_to(read_entity.entity, parent_it->entity);
			}

			world.set_entity_pos_local(read_entity.entity, read_entity.header.local_pos);
			world.set_entity_rot_local(read_entity.entity, read_entity.header.local_rot);
			world.set_entity_scale_local(read_entity.entity, read_entity.header.local_scale);
		}

		const nlohmann::json components_json = in_json.value<nlohmann::json>("components", nlohmann::json::array());
		if (components_json.is_array())
		{
			for (const nlohmann::json& component_json : components_json)
			{
				if (!component_json.is_object())
					continue;

				const sid_t			 component_type_id	= component_json.value<sid_t>("type", 0);
				const entity_guid_t	 target_entity_guid = component_json.value<entity_guid_t>("entity", NULL_ENTITY_GUID);
				const nlohmann::json component_data		= component_json.value<nlohmann::json>("data", nlohmann::json::object());

				const reflected_type_t*	 reflected_type	 = reflection_registry_t::get().find_type(component_type_id);
				world_component_table_t* component_table = world.find_component_table(component_type_id);
				const auto				 target_it		 = std::find_if(read_entities.begin(), read_entities.end(), [&](const read_entity_t& other) { return other.header.guid == target_entity_guid; });
				if (reflected_type == nullptr || component_table == nullptr || target_it == read_entities.end())
					continue;

				const reflected_field_span_t fields = reflected_type->fields;

				void* component = ecs_t::table_add(component_table->table, target_it->entity);
				if (component != nullptr)
				{
					component_table->type_desc.default_init(component);
					const bool deserialized = reflection_registry_t::get().type_from_json(component_type_id, component, nullptr, component_data);
					SFG_ASSERT(deserialized);

					if (generate_new_guids)
						fix_entity_guid_references(component, fields, read_entities);
				}
			}
		}

		const entity_id_t root_entity = read_entities.empty() ? NULL_ENTITY_ID : read_entities[0].entity;
		if (sync_hierarchy && root_entity != NULL_ENTITY_ID)
			world.sync_entity_hierarchy(root_entity);
		return root_entity;
	}

	void world_cooker_t::entity_to_json(const world_t& world, entity_id_t entity, nlohmann::json& out_json, frame_vector_t<resource_handle_t>& out_resources)
	{
		SFG_ASSERT(world.is_alive(entity));

		out_resources.resize(0);

		out_json				   = nlohmann::json::object();
		out_json["local_entities"] = nlohmann::json::array();
		out_json["components"]	   = nlohmann::json::array();

		struct written_entity_t
		{
			entity_id_t	  entity = NULL_ENTITY_ID;
			entity_guid_t guid	 = NULL_ENTITY_GUID;
		};

		frame_vector_t<written_entity_t> written_entities;

		const world_component_table_t* hierarchy_world_table	= world.find_component_table(type_id_t<component_hierarchy_t>::value);
		const world_component_table_t* no_serialize_world_table = world.find_component_table(type_id_t<component_no_serialize_t>::value);

		const ecs_component_table_t& hierarchy_table	= hierarchy_world_table->table;
		const ecs_component_table_t& no_serialize_table = no_serialize_world_table->table;

		const auto write_entity = [&](const auto& self, entity_id_t current) -> void {
			if (ecs_t::table_has(no_serialize_table, current))
				return;

			const entity_guid_t				 guid	= world.get_entity_guid(current);
			const entity_id_t				 parent = world.get_entity_parent(current);
			const world_cook_entity_header_t header{
				.guid		 = guid,
				.parent_guid = world.get_entity_guid(parent),
				.name		 = world.get_entity_name(current),
				.local_pos	 = world.get_entity_pos_local(current),
				.local_rot	 = world.get_entity_rot_local(current),
				.local_scale = world.get_entity_scale_local(current),
			};
			out_json["local_entities"].push_back(header);
			written_entities.push_back({.entity = current, .guid = guid});

			const component_hierarchy_t& hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, current);
			for (entity_id_t child = hierarchy.first_child; child != NULL_ENTITY_ID;)
			{
				const component_hierarchy_t& child_hierarchy = ecs_helpers_t::table_get_as_const<component_hierarchy_t>(hierarchy_table, child);
				self(self, child);
				child = child_hierarchy.next_sibling;
			}
		};

		write_entity(write_entity, entity);

		const vector_t<world_component_table_t>& component_tables = world.get_component_tables();
		for (const written_entity_t& written_entity : written_entities)
		{
			for (const world_component_table_t& component_table : component_tables)
			{
				const sid_t				component_type_id = component_table.type_desc.type_id;
				const reflected_type_t* reflected_type	  = reflection_registry_t::get().find_type(component_type_id);
				SFG_ASSERT(reflected_type != nullptr);

				if (reflected_type->flags.is_set(reflected_type_flag_no_serialization))
					continue;

				if (!ecs_t::table_has(component_table.table, written_entity.entity))
					continue;

				nlohmann::json component_data = nlohmann::json::object();
				void*		   component	  = ecs_t::table_get(component_table.table, written_entity.entity);
				if (component != nullptr)
				{
					const bool serialized = reflection_registry_t::get().type_to_json(component_type_id, component, nullptr, component_data);
					SFG_ASSERT(serialized);
				}

				out_json["components"].push_back({
					{"type", component_type_id},
					{"entity", written_entity.guid},
					{"data", component_data},
				});
			}
		}
	}

	void world_cooker_t::entity_to_prefab_json(const world_t& world, entity_id_t entity, nlohmann::json& out_json)
	{
	}
}
