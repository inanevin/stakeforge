// Copyright (c) 2025 Inan Evin

#include "common_resources.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/memory/memory.hpp>

#include "animation.hpp"
#include "animation_state_machine.hpp"
#include "audio.hpp"
#include "font.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "physical_material.hpp"
#include "prefab.hpp"
#include "shader.hpp"
#include "skeleton.hpp"
#include "skybox_hdr.hpp"
#include "texture.hpp"
#include "texture_sampler.hpp"

namespace sfg
{
	void resource_header_t::serialize(ostream_t& stream) const
	{
		stream << magic << version;
		stream << source_tick;
	}

	void resource_header_t::deserialize(istream_t& stream)
	{
		stream >> magic >> version;
		stream >> source_tick;
	}

	ostream_t make_resource_stream(const resource_header_t& header, const ostream_t& payload)
	{
		ostream_t stream;
		stream.create(sizeof(header.magic) + sizeof(header.version) + sizeof(header.source_tick) + payload.get_size());
		header.serialize(stream);
		stream.write_raw(payload.get_raw(), payload.get_size());
		return stream;
	}

	const resource_type_desc_t* const g_resource_type_descs[RESOURCE_TYPE_MAX] = {
		nullptr,
		&audio_resource_desc,
		&font_resource_desc,
		&mesh_resource_desc,
		&skeleton_resource_desc,
		&animation_resource_desc,
		&material_resource_desc,
		&shader_resource_desc,
		&texture_resource_desc,
		&texture_sampler_resource_desc,
		&physical_material_resource_desc,
		&prefab_resource_desc,
		&animation_state_machine_resource_desc,
		&skybox_hdr_resource_desc,
	};
}

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t resource_type_values[] = {
			{
				.name		  = "invalid",
				.display_name = "Invalid",
				.value		  = static_cast<i64>(resource_type_e::invalid),
			},
			{
				.name		  = "audio",
				.display_name = "Audio",
				.value		  = static_cast<i64>(resource_type_e::audio),
			},
			{
				.name		  = "font",
				.display_name = "Font",
				.value		  = static_cast<i64>(resource_type_e::font),
			},
			{
				.name		  = "mesh",
				.display_name = "Mesh",
				.value		  = static_cast<i64>(resource_type_e::mesh),
			},
			{
				.name		  = "skeleton",
				.display_name = "Skeleton",
				.value		  = static_cast<i64>(resource_type_e::skeleton),
			},
			{
				.name		  = "animation",
				.display_name = "Animation",
				.value		  = static_cast<i64>(resource_type_e::animation),
			},
			{
				.name		  = "material",
				.display_name = "Material",
				.value		  = static_cast<i64>(resource_type_e::material),
			},
			{
				.name		  = "shader",
				.display_name = "Shader",
				.value		  = static_cast<i64>(resource_type_e::shader),
			},
			{
				.name		  = "texture",
				.display_name = "Texture",
				.value		  = static_cast<i64>(resource_type_e::texture),
			},
			{
				.name		  = "texture_sampler",
				.display_name = "Texture Sampler",
				.value		  = static_cast<i64>(resource_type_e::texture_sampler),
			},
			{
				.name		  = "physical_material",
				.display_name = "Physical Material",
				.value		  = static_cast<i64>(resource_type_e::physical_material),
			},
			{
				.name		  = "prefab",
				.display_name = "Prefab",
				.value		  = static_cast<i64>(resource_type_e::prefab),
			},
			{
				.name		  = "animation_state_machine",
				.display_name = "Animation State Machine",
				.value		  = static_cast<i64>(resource_type_e::animation_state_machine),
			},
			{
				.name		  = "hdr_skybox",
				.display_name = "HDR Skybox",
				.value		  = static_cast<i64>(resource_type_e::hdr_skybox),
			},
		};
	}

	resource_type_reflection_t::resource_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<resource_type_e>::value) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = resource_type_values, .size = std::size(resource_type_values)},
			.name		 = "resource_type_e",
			.type_id	 = type_id_t<resource_type_e>::value,
			.size		 = sizeof(resource_type_e),
			.alignment	 = alignof(resource_type_e),
		});
	}
}
