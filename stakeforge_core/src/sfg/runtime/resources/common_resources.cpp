// Copyright (c) 2025 Inan Evin

#include "common_resources.hpp"
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
	const resource_type_desc_t* const g_resource_type_descs[resource_type_max] = {
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

	resource_type_e resolve_resource_type(const string_t& s)
	{
		if (s == "audio")
			return resource_type_e::audio;
		if (s == "font")
			return resource_type_e::font;
		if (s == "mesh")
			return resource_type_e::mesh;
		if (s == "skeleton")
			return resource_type_e::skeleton;
		if (s == "animation")
			return resource_type_e::animation;
		if (s == "particle_properties")
			return resource_type_e::particle_properties;
		if (s == "material")
			return resource_type_e::material;
		if (s == "shader")
			return resource_type_e::shader;
		if (s == "texture")
			return resource_type_e::texture;
		if (s == "texture_sampler")
			return resource_type_e::texture_sampler;
		if (s == "physical_material")
			return resource_type_e::physical_material;
		if (s == "prefab")
			return resource_type_e::prefab;
		if (s == "animation_state_machine")
			return resource_type_e::animation_state_machine;
		return resource_type_e::invalid;
	}
}
