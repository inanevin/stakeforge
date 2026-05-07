// Copyright (c) 2025 Inan Evin

#include "resource_manifest.hpp"

namespace sfg
{
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

	void from_json(const nlohmann::json& j, resource_manifest_entry_t& e)
	{
		e.path	 = j.value<string_t>("path", "");
		e.name	 = j.value<string_t>("name", "");
		e.type	 = j.value<resource_type_e>("type", resource_type_e::invalid);
		e.config = j.value<nlohmann::json>("config", nlohmann::json::object());
	}

	void from_json(const nlohmann::json& j, resource_manifest_t& m)
	{
		m.resources = j.value<vector_t<resource_manifest_entry_t>>("resources", {});
	}
}
