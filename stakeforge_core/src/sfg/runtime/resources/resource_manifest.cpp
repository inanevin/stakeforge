// Copyright (c) 2025 Inan Evin

#include "resource_manifest.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const resource_type_e& t)
	{
		switch (t)
		{
		case resource_type_e::audio:
			j = "audio";
			return;
		case resource_type_e::font:
			j = "font";
			return;
		case resource_type_e::mesh:
			j = "mesh";
			return;
		case resource_type_e::skeleton:
			j = "skeleton";
			return;
		case resource_type_e::animation:
			j = "animation";
			return;
		case resource_type_e::particle_properties:
			j = "particle_properties";
			return;
		case resource_type_e::material:
			j = "material";
			return;
		case resource_type_e::shader:
			j = "shader";
			return;
		case resource_type_e::texture:
			j = "texture";
			return;
		case resource_type_e::texture_sampler:
			j = "texture_sampler";
			return;
		case resource_type_e::physical_material:
			j = "physical_material";
			return;
		case resource_type_e::prefab:
			j = "prefab";
			return;
		case resource_type_e::animation_state_machine:
			j = "animation_state_machine";
			return;
		default:
			break;
		}

		j = "invalid";
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

	void to_json(nlohmann::json& j, const resource_manifest_entry_t& e)
	{
		j["name"] = e.name;
		j["path"] = e.path;
		j["type"] = e.type;
	}

	void from_json(const nlohmann::json& j, resource_manifest_entry_t& e)
	{
		e.path = j.value<string_t>("path", "");
		e.name = j.value<string_t>("name", "");
		e.type = j.value<resource_type_e>("path", resource_type_e::invalid);
	}

	void to_json(nlohmann::json& j, const resource_manifest_t& m)
	{
		j["resources"] = m.resources;
	}

	void from_json(const nlohmann::json& j, resource_manifest_t& m)
	{
		m.resources = j.value<vector_t<resource_manifest_entry_t>>("resources", {});
	}
}
