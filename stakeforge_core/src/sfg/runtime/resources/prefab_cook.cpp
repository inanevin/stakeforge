// Copyright (c) 2025 Inan Evin

#include "prefab_cook.hpp"
#include "world_cook_entity_header.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/serialization/compression.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		size_t align_size(size_t size, size_t alignment)
		{
			return (size + alignment - 1) & ~(alignment - 1);
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

		void scrub_reflected_text_id_json(const reflected_type_desc_t& type, const nlohmann::json& src, nlohmann::json& dst)
		{
			dst = src.is_object() ? src : nlohmann::json::object();
			for (u32 i = 0; i < type.fields.size; i++)
			{
				const reflected_field_desc_t& field = type.fields.data[i];
				if (field.name == nullptr || !dst.contains(field.name))
					continue;

				if (field.type == reflected_value_type_e::text_id)
				{
					dst[field.name] = 0u;
					continue;
				}

				if ((field.type == reflected_value_type_e::vector || field.type == reflected_value_type_e::inplace_vector) && reflected_value_type_from_sub_type_id(field.sub_type_id) == reflected_value_type_e::text_id)
				{
					const nlohmann::json values = dst.value(field.name, nlohmann::json::array());
					dst[field.name]				= nlohmann::json::array();
					if (values.is_array())
					{
						for (size_t value_index = 0; value_index < values.size(); value_index++)
							dst[field.name].push_back(0u);
					}
					continue;
				}

				if (field.type == reflected_value_type_e::object)
				{
					const reflected_type_desc_t* field_type = reflection_registry_t::get().find_type(field.value_type_id);
					if (field_type != nullptr)
					{
						nlohmann::json child = dst[field.name];
						scrub_reflected_text_id_json(*field_type, child, dst[field.name]);
					}
				}
			}
		}

		bool write_text_id_json_to_stream(const nlohmann::json& json, ostream_t& stream)
		{
			stream << string_t(json.is_string() ? json.get<string_t>().c_str() : "");
			return true;
		}

		bool write_text_id_vector_json_to_stream(const nlohmann::json& json, ostream_t& stream)
		{
			if (!json.is_array())
			{
				stream << 0u;
				return true;
			}

			stream << static_cast<u32>(json.size());
			for (const nlohmann::json& value : json)
			{
				if (!write_text_id_json_to_stream(value, stream))
					return false;
			}
			return true;
		}

		bool write_reflected_type_to_stream_with_text_ids(const reflected_type_desc_t& type, const void* data, const nlohmann::json& json, ostream_t& stream)
		{
			for (u32 i = 0; i < type.fields.size; i++)
			{
				const reflected_field_desc_t& field = type.fields.data[i];
				SFG_ASSERT(field.name != nullptr);
				const nlohmann::json field_json = json.value(field.name, nlohmann::json{});

				if (field.type == reflected_value_type_e::text_id)
				{
					if (!write_text_id_json_to_stream(field_json, stream))
						return false;
					continue;
				}

				if ((field.type == reflected_value_type_e::vector || field.type == reflected_value_type_e::inplace_vector) && reflected_value_type_from_sub_type_id(field.sub_type_id) == reflected_value_type_e::text_id)
				{
					if (!write_text_id_vector_json_to_stream(field_json, stream))
						return false;
					continue;
				}

				if (field.type == reflected_value_type_e::object)
				{
					const reflected_type_desc_t* field_type = reflection_registry_t::get().find_type(field.value_type_id);
					if (field_type != nullptr && reflected_type_has_text_id(*field_type))
					{
						if (!write_reflected_type_to_stream_with_text_ids(*field_type, static_cast<const u8*>(data) + field.offset, field_json, stream))
							return false;
						continue;
					}
				}

				if (!reflection_registry_t::get().serialize_field_to_stream(data, field, stream))
					return false;
			}
			return true;
		}

		bool write_component_to_stream(const nlohmann::json& component, ostream_t& stream)
		{
			const sid_t type_id = component.value<sid_t>("type_id", 0);
			if (type_id == 0)
			{
				SFG_ERR("prefab component has invalid type id");
				return false;
			}

			const reflected_type_desc_t* type = reflection_registry_t::get().find_type(type_id);
			if (type == nullptr)
			{
				SFG_ERR("prefab component has unregistered type: {0}", type_id);
				return false;
			}

			stream << type_id;
			if (type->size == 0)
				return true;

			const size_t allocation_size = align_size(type->size, type->alignment);
			void*		 data			 = SFG_ALIGNED_MALLOC(type->alignment, allocation_size);
			SFG_MEMSET(data, 0, allocation_size);
			const nlohmann::json component_data = component.value("data", nlohmann::json::object());
			nlohmann::json		 scrubbed_data	= {};
			const bool			 has_text_ids	= reflected_type_has_text_id(*type);
			if (has_text_ids)
				scrub_reflected_text_id_json(*type, component_data, scrubbed_data);

			if (!reflection_registry_t::get().deserialize_from_json(type_id, data, has_text_ids ? scrubbed_data : component_data))
			{
				SFG_ALIGNED_FREE(data);
				SFG_ERR("failed to deserialize prefab component: {0}", type_id);
				return false;
			}
			if (has_text_ids ? !write_reflected_type_to_stream_with_text_ids(*type, data, component_data, stream) : !reflection_registry_t::get().serialize_to_stream(type_id, data, stream))
			{
				SFG_ALIGNED_FREE(data);
				SFG_ERR("failed to serialize prefab component: {0}", type_id);
				return false;
			}

			SFG_ALIGNED_FREE(data);
			return true;
		}

		bool write_entity_to_stream(const nlohmann::json& entity, ostream_t& stream)
		{
			const world_cook_entity_header_t header = entity.value("header", world_cook_entity_header_t{});
			stream << header;

			if (header.prefab != NULL_RESOURCE_HANDLE)
				return true;

			const nlohmann::json components = entity.value("components", nlohmann::json::array());
			if (!components.is_array())
			{
				SFG_ERR("prefab entity components must be an array");
				return false;
			}

			stream << static_cast<u32>(components.size());
			for (const nlohmann::json& component : components)
			{
				if (!write_component_to_stream(component, stream))
					return false;
			}

			const nlohmann::json children = entity.value("children", nlohmann::json::array());
			if (!children.is_array())
			{
				SFG_ERR("prefab entity children must be an array");
				return false;
			}

			stream << static_cast<u32>(children.size());
			for (const nlohmann::json& child : children)
			{
				if (!write_entity_to_stream(child, stream))
					return false;
			}

			return true;
		}
	}

	bool prefab_cooker::cook_from_file(const char* full_path, resource_header_t& out_header, ostream_t& stream)
	{
		const string_t		 text = file_system_t::read_file_as_string(full_path);
		const nlohmann::json json = nlohmann::json::parse(text, nullptr, false);
		if (json.is_discarded())
		{
			SFG_ERR("failed to parse prefab json: {0}", full_path);
			return false;
		}

		return cook_from_json(json, out_header, stream);
	}

	bool prefab_cooker::cook_from_json(const nlohmann::json& json, resource_header_t& out_header, ostream_t& stream)
	{
		ostream_t			 prefab_stream;
		const nlohmann::json resources = json.value("resources", nlohmann::json::array());
		if (!resources.is_array())
		{
			SFG_ERR("prefab resources must be an array");
			return false;
		}

		prefab_stream << static_cast<u32>(resources.size());
		for (const nlohmann::json& resource : resources)
			prefab_stream << resource.get<resource_handle_t>();

		if (!write_entity_to_stream(json, prefab_stream))
			return false;

		out_header = {
			.magic		 = prefab_loader_t::WIRE_MAGIC,
			.version	 = prefab_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(prefab_stream.get_raw(), prefab_stream.get_size()),
		};

		stream = compressor_t::compress(prefab_stream);
		if (stream.get_size() == 0)
		{
			SFG_ERR("failed to compress prefab payload");
			return false;
		}

		return true;
	}
}
