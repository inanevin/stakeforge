// Copyright (c) 2025 Inan Evin

#include "common_resources.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/memory/memory.hpp>

#include "animation.hpp"
#include "animation_state_machine.hpp"
#include "audio.hpp"
#include "font.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "particle_properties.hpp"
#include "physical_material.hpp"
#include "prefab.hpp"
#include "shader.hpp"
#include "skeleton.hpp"
#include "texture.hpp"
#include "texture_sampler.hpp"

namespace sfg
{
	void resource_header_t::serialize(ostream_t& stream) const
	{
		stream << magic << version << payload_size << payload_offset << modified_ticks;
	}

	bool resource_header_t::deserialize(istream_t& stream)
	{
		stream >> magic >> version >> payload_size >> payload_offset >> modified_ticks;
		return true;
	}

	void resource_header_t::patch_payload_offset(ostream_t& stream, size_t header_pos) const
	{
		SFG_MEMCPY(stream.get_raw() + header_pos + sizeof(u32) * 3, &payload_offset, sizeof(u32));
	}

	const resource_type_desc_t* const g_resource_type_descs[RESOURCE_TYPE_MAX] = {
		nullptr,
		&audio_resource_desc,
		&font_resource_desc,
		&mesh_resource_desc,
		&skeleton_resource_desc,
		&animation_resource_desc,
		&particle_properties_resource_desc,
		&material_resource_desc,
		&shader_resource_desc,
		&texture_resource_desc,
		&texture_sampler_resource_desc,
		&physical_material_resource_desc,
		&prefab_resource_desc,
		&animation_state_machine_resource_desc,
	};
}
