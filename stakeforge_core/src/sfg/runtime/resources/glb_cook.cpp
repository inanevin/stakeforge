// Copyright (c) 2025 Inan Evin

#include "glb_cook.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool glb_cooker::cook_from_file(const glb_cook_config_t&, const char*, resource_header_t&, ostream_t&)
	{
		return false;
	}

}

namespace sfg
{
	glb_cook_config_reflection_t::glb_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<glb_cook_config_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "combine_meshes", .display_name = "Combine Meshes", .type = reflected_value_type_e::bool8, .offset = offsetof(glb_cook_config_t, combine_meshes), .size = sizeof(bool)},
			{.name		   = "texture_payload_type",
			 .display_name = "Texture Payload Type",
			 .type		   = reflected_value_type_e::enum8,
			 .sub_type_id  = type_id_t<texture_payload_type_e>::value,
			 .offset	   = offsetof(glb_cook_config_t, texture_payload_type),
			 .size		   = sizeof(texture_payload_type_e)},
			{.name = "generate_mipmaps", .display_name = "Generate Mipmaps", .type = reflected_value_type_e::bool8, .offset = offsetof(glb_cook_config_t, generate_mipmaps), .size = sizeof(bool)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "glb_cook_config_t",
			.type_id   = type_id_t<glb_cook_config_t>::value,
			.size	   = sizeof(glb_cook_config_t),
			.alignment = alignof(glb_cook_config_t),
		});
	}
}
