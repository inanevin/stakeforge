// Copyright (c) 2025 Inan Evin

#include "common_resources.hpp"
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
	resource_type_reflection_t::resource_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "resource_type_e",
			.fields =
				{
					{.name = "invalid", .display_name = "Invalid"},
					{.name = "audio", .display_name = "Audio"},
					{.name = "font", .display_name = "Font"},
					{.name = "mesh", .display_name = "Mesh"},
					{.name = "skeleton", .display_name = "Skeleton"},
					{.name = "animation", .display_name = "Animation"},
					{.name = "material", .display_name = "Material"},
					{.name = "shader", .display_name = "Shader"},
					{.name = "texture", .display_name = "Texture"},
					{.name = "texture_sampler", .display_name = "Texture Sampler"},
					{.name = "physical_material", .display_name = "Physical Material"},
					{.name = "prefab", .display_name = "Prefab"},
					{.name = "animation_state_machine", .display_name = "Animation State Machine"},
					{.name = "hdr_skybox", .display_name = "HDR Skybox"},
				},
			.type_id   = type_id_t<resource_type_e>::value,
			.size	   = sizeof(resource_type_e),
			.alignment = alignof(resource_type_e),
			.flags	   = reflected_type_flag_enum,
		});
	}
}
