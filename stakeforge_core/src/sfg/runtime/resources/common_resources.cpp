// Copyright (c) 2025 Inan Evin

#include "common_resources.hpp"
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/memory/memory.hpp>
#include <algorithm>
#include <cstring>

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
		stream.write_raw(reinterpret_cast<const u8*>(debug_name), sizeof(debug_name));
		stream << magic << version;
		stream << source_tick;
		stream << dependency_count;
		for (u32 i = 0; i < MAX_DEPENDENCIES; i++)
			stream << dependencies[i];
	}

	void resource_header_t::deserialize(istream_t& stream)
	{
		stream.read_to_raw(reinterpret_cast<u8*>(debug_name), sizeof(debug_name));
		stream >> magic >> version;
		stream >> source_tick;
		stream >> dependency_count;
		for (u32 i = 0; i < MAX_DEPENDENCIES; i++)
			stream >> dependencies[i];
	}

	void resource_header_t::set_debug_name(const char* full_path)
	{
		SFG_ASSERT(full_path != nullptr);

		string_t path = full_path;
		file_system_t::fix_path(path);

		const string_t file_name  = file_system_t::get_filename_and_extension_from_path(path);
		const char*	   debug_name = !file_name.empty() ? file_name.c_str() : path.c_str();
		const size_t   copy_size  = std::min(std::strlen(debug_name), sizeof(this->debug_name) - 1);

		SFG_MEMSET(this->debug_name, 0, sizeof(this->debug_name));
		SFG_MEMCPY(this->debug_name, debug_name, copy_size);
	}

	ostream_t resource_header_t::make_stream(const ostream_t& payload) const
	{
		ostream_t stream;
		stream.create(sizeof(resource_header_t) + payload.get_size());
		serialize(stream);
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

	void resource_dependency_t::serialize(ostream_t& stream) const
	{
		stream << type;
		stream << handle;
	}

	void resource_dependency_t::deserialize(istream_t& stream)
	{
		stream >> type;
		stream >> handle;
	}

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
