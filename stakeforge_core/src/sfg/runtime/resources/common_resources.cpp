// Copyright (c) 2025 Inan Evin

#include "common_resources.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/istream_vector.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/ostream_vector.hpp>
#include <sfg/data/string.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

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
		stream << magic << version;
		stream << source_ticks;
	}

	void resource_header_t::deserialize(istream_t& stream)
	{
		stream >> magic >> version;
		stream >> source_ticks;
	}

	void to_json(nlohmann::json& j, const resource_type_e& t)
	{
		switch (t)
		{
		case resource_type_e::audio:
			j = "audio";
			break;
		case resource_type_e::font:
			j = "font";
			break;
		case resource_type_e::mesh:
			j = "mesh";
			break;
		case resource_type_e::skeleton:
			j = "skeleton";
			break;
		case resource_type_e::animation:
			j = "animation";
			break;
		case resource_type_e::particle_properties:
			j = "particle_properties";
			break;
		case resource_type_e::material:
			j = "material";
			break;
		case resource_type_e::shader:
			j = "shader";
			break;
		case resource_type_e::texture:
			j = "texture";
			break;
		case resource_type_e::texture_sampler:
			j = "texture_sampler";
			break;
		case resource_type_e::physical_material:
			j = "physical_material";
			break;
		case resource_type_e::prefab:
			j = "prefab";
			break;
		case resource_type_e::animation_state_machine:
			j = "animation_state_machine";
			break;
		default:
			j = "invalid";
			break;
		}
	}

	void from_json(const nlohmann::json& j, resource_type_e& t)
	{
		const string_t s = j.get<string_t>();

		if (s == "audio")
			t = resource_type_e::audio;
		else if (s == "font")
			t = resource_type_e::font;
		else if (s == "mesh")
			t = resource_type_e::mesh;
		else if (s == "skeleton")
			t = resource_type_e::skeleton;
		else if (s == "animation")
			t = resource_type_e::animation;
		else if (s == "particle_properties")
			t = resource_type_e::particle_properties;
		else if (s == "material")
			t = resource_type_e::material;
		else if (s == "shader")
			t = resource_type_e::shader;
		else if (s == "texture")
			t = resource_type_e::texture;
		else if (s == "texture_sampler")
			t = resource_type_e::texture_sampler;
		else if (s == "physical_material")
			t = resource_type_e::physical_material;
		else if (s == "prefab")
			t = resource_type_e::prefab;
		else if (s == "animation_state_machine")
			t = resource_type_e::animation_state_machine;
		else
			t = resource_type_e::invalid;
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
