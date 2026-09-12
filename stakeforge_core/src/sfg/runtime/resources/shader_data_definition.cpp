/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "shader_data_definition.hpp"
#include "resource_type.hpp"
#include <sfg/data/string.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		void copy_definition_name_from_string(const string_t& name, char* out, size_t out_size)
		{
			SFG_MEMSET(out, 0, out_size);
			if (name.size() < out_size)
				SFG_MEMCPY(out, name.data(), name.size());
		}
	}

	const char* shader_texture_type_to_string(shader_texture_type_e type)
	{
		switch (type)
		{
		case shader_texture_type_e::texture2d:
			return "texture2d";
		case shader_texture_type_e::texture_cube:
			return "texture_cube";
		case shader_texture_type_e::sprite:
			return "sprite";
		default:
			return "invalid";
		}
	}

	resource_type_e shader_texture_type_to_resource_type(shader_texture_type_e type)
	{
		switch (type)
		{
		case shader_texture_type_e::texture2d:
			return resource_type_e::texture;
		case shader_texture_type_e::texture_cube:
			return resource_type_e::cubemap;
		case shader_texture_type_e::sprite:
			return resource_type_e::sprite;
		default:
			return resource_type_e::invalid;
		}
	}

	const char* shader_param_type_to_string(shader_param_type_e type)
	{
		switch (type)
		{
		case shader_param_type_e::f32:
			return "f32";
		case shader_param_type_e::vec2:
			return "vec2";
		case shader_param_type_e::vec4:
			return "vec4";
		case shader_param_type_e::u32:
			return "u32";
		default:
			return "invalid";
		}
	}

	const char* shader_param_hint_to_string(shader_param_hint_e hint)
	{
		switch (hint)
		{
		case shader_param_hint_e::color:
			return "color";
		case shader_param_hint_e::color_hdr:
			return "color_hdr";
		case shader_param_hint_e::pack_uint2:
			return "pack_uint2";
		case shader_param_hint_e::toggle:
			return "toggle";
		default:
			return "none";
		}
	}

	shader_texture_type_e shader_texture_type_from_string(const string_t& value)
	{
		if (value == "texture2d")
			return shader_texture_type_e::texture2d;
		if (value == "texture_cube")
			return shader_texture_type_e::texture_cube;
		if (value == "sprite")
			return shader_texture_type_e::sprite;

		return shader_texture_type_e::invalid;
	}

	shader_param_type_e shader_param_type_from_string(const string_t& value)
	{
		if (value == "f32")
			return shader_param_type_e::f32;
		if (value == "vec2")
			return shader_param_type_e::vec2;
		if (value == "vec4")
			return shader_param_type_e::vec4;
		if (value == "u32")
			return shader_param_type_e::u32;
		return shader_param_type_e::invalid;
	}

	shader_param_hint_e shader_param_hint_from_string(const string_t& value)
	{
		if (value == "color")
			return shader_param_hint_e::color;
		if (value == "color_hdr")
			return shader_param_hint_e::color_hdr;
		if (value == "pack_uint2")
			return shader_param_hint_e::pack_uint2;
		if (value == "toggle")
			return shader_param_hint_e::toggle;
		return shader_param_hint_e::none;
	}

	void to_json(nlohmann::json& j, const shader_texture_definition_t& definition)
	{
		j["name"] = definition.texture_name;
		j["type"] = shader_texture_type_to_string(definition.type);
	}

	void from_json(const nlohmann::json& j, shader_texture_definition_t& definition)
	{
		definition		= {};
		definition.type = shader_texture_type_from_string(j.value<string_t>("type", "invalid"));
		copy_definition_name_from_string(j.value<string_t>("name", {}), definition.texture_name, sizeof(definition.texture_name));
	}

	void to_json(nlohmann::json& j, const shader_sampler_definition_t& definition)
	{
		j["name"] = definition.sampler_name;
	}

	void from_json(const nlohmann::json& j, shader_sampler_definition_t& definition)
	{
		definition = {};
		copy_definition_name_from_string(j.value<string_t>("name", {}), definition.sampler_name, sizeof(definition.sampler_name));
	}

	void to_json(nlohmann::json& j, const shader_param_definition_t& definition)
	{
		j["name"] = definition.param_name;
		j["type"] = shader_param_type_to_string(definition.type);
		j["hint"] = shader_param_hint_to_string(definition.hint);
		if (definition.type == shader_param_type_e::f32)
		{
			j["default"] = definition.default_value[0];
			j["min"]	 = definition.min_value[0];
			j["max"]	 = definition.max_value[0];
		}
		else if (definition.type == shader_param_type_e::u32)
		{
			j["default"] = definition.default_value_u32[0];
			j["min"]	 = definition.min_value_u32[0];
			j["max"]	 = definition.max_value_u32[0];
		}
	}

	void from_json(const nlohmann::json& j, shader_param_definition_t& definition)
	{
		definition		= {};
		definition.type = shader_param_type_from_string(j.value<string_t>("type", "invalid"));
		definition.hint = shader_param_hint_from_string(j.value<string_t>("hint", "none"));
		copy_definition_name_from_string(j.value<string_t>("name", {}), definition.param_name, sizeof(definition.param_name));
		if (definition.type == shader_param_type_e::f32)
		{
			definition.default_value[0] = j.value<f32>("default", 0.0f);
			definition.min_value[0]		= j.value<f32>("min", 0.0f);
			definition.max_value[0]		= j.value<f32>("max", 0.0f);
		}
		else if (definition.type == shader_param_type_e::u32)
		{
			definition.default_value_u32[0] = j.value<u32>("default", 0);
			definition.min_value_u32[0]		= j.value<u32>("min", 0);
			definition.max_value_u32[0]		= j.value<u32>("max", 0);
		}
	}

	void to_json(nlohmann::json& j, const shader_data_definition_t& definition)
	{
		j["textures"]	= nlohmann::json::array();
		j["samplers"]	= nlohmann::json::array();
		j["parameters"] = nlohmann::json::array();
		for (const shader_texture_definition_t& texture : definition.textures)
			j["textures"].push_back(texture);
		for (const shader_sampler_definition_t& sampler : definition.samplers)
			j["samplers"].push_back(sampler);
		for (const shader_param_definition_t& parameter : definition.parameters)
			j["parameters"].push_back(parameter);
	}

	void from_json(const nlohmann::json& j, shader_data_definition_t& definition)
	{
		definition = {};

		const nlohmann::json textures = j.value("textures", nlohmann::json::array());
		for (const nlohmann::json& item : textures)
		{
			if (definition.textures.full())
				break;
			definition.textures.push_back(item.get<shader_texture_definition_t>());
		}

		const nlohmann::json samplers = j.value("samplers", nlohmann::json::array());
		for (const nlohmann::json& item : samplers)
		{
			if (definition.samplers.full())
				break;
			definition.samplers.push_back(item.get<shader_sampler_definition_t>());
		}

		const nlohmann::json parameters = j.value("parameters", nlohmann::json::array());
		for (const nlohmann::json& item : parameters)
		{
			if (definition.parameters.full())
				break;
			definition.parameters.push_back(item.get<shader_param_definition_t>());
		}
	}
}
