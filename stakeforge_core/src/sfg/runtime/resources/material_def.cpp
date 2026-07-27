/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "material_def.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		void copy_name(const char* src, string_t& out)
		{
			out = src != nullptr ? src : "";
		}
	}

	void material_def_t::serialize(ostream_t& stream) const
	{
		stream << shader;
		stream << write_shadows;
		stream << write_reflections;
		stream << is_transparent;
		stream << double_sided;
		stream << use_alpha_cutoff;

		const u8 texture_count = static_cast<u8>(textures.size());
		stream << texture_count;
		for (const material_texture_value_t& texture : textures)
		{
			const sid_t name_hash = texture.name.empty() ? texture.name_hash : hashing_t::to_sid(texture.name);
			stream << name_hash;
			stream << texture.texture;
			stream << texture.type;
		}

		const u8 sampler_count = static_cast<u8>(samplers.size());
		stream << sampler_count;
		for (const material_sampler_value_t& sampler : samplers)
		{
			const sid_t name_hash = sampler.name.empty() ? sampler.name_hash : hashing_t::to_sid(sampler.name);
			stream << name_hash;
			stream << sampler.sampler;
		}

		const u8 parameter_count = static_cast<u8>(parameters.size());
		stream << parameter_count;
		for (const material_param_value_t& parameter : parameters)
		{
			const sid_t name_hash = parameter.name.empty() ? parameter.name_hash : hashing_t::to_sid(parameter.name);
			stream << name_hash;
			stream << parameter.type;
			stream << parameter.hint;

			for (u8 i = 0; i < 4; ++i)
			{
				if (parameter.type == shader_param_type_e::u32)
					stream << parameter.value_u32[i];
				else
					stream << parameter.value[i];
			}
		}
	}

	void material_def_t::deserialize(istream_t& stream)
	{
		*this = {};

		stream >> shader;
		stream >> write_shadows;
		stream >> write_reflections;
		stream >> is_transparent;
		stream >> double_sided;
		stream >> use_alpha_cutoff;

		u8 texture_count = 0;
		stream >> texture_count;
		textures.resize(texture_count);
		for (material_texture_value_t& texture : textures)
		{
			stream >> texture.name_hash;
			stream >> texture.texture;
			stream >> texture.type;
		}

		u8 sampler_count = 0;
		stream >> sampler_count;
		samplers.resize(sampler_count);
		for (material_sampler_value_t& sampler : samplers)
		{
			stream >> sampler.name_hash;
			stream >> sampler.sampler;
		}

		u8 parameter_count = 0;
		stream >> parameter_count;
		parameters.resize(parameter_count);
		for (material_param_value_t& parameter : parameters)
		{
			stream >> parameter.name_hash;
			stream >> parameter.type;
			stream >> parameter.hint;

			for (u8 i = 0; i < 4; ++i)
			{
				if (parameter.type == shader_param_type_e::u32)
					stream >> parameter.value_u32[i];
				else
					stream >> parameter.value[i];
			}
		}
	}

	material_def_t material_def_from_shader_def(const shader_data_definition_t& shader_def, resource_handle_t shader)
	{
		material_def_t out = {};
		out.shader		   = shader;

		for (const shader_texture_definition_t& texture : shader_def.textures)
		{
			material_texture_value_t& value = out.textures.emplace_back();
			value.name						= texture.texture_name != nullptr ? texture.texture_name : "";
			value.name_hash					= hashing_t::to_sid(value.name);
			value.type						= texture.type;
		}

		for (const shader_sampler_definition_t& sampler : shader_def.samplers)
		{
			material_sampler_value_t& value = out.samplers.emplace_back();
			value.name						= sampler.sampler_name != nullptr ? sampler.sampler_name : "";
			value.name_hash					= hashing_t::to_sid(value.name);
		}

		for (const shader_param_definition_t& parameter : shader_def.parameters)
		{
			material_param_value_t& value = out.parameters.emplace_back();
			value.name					  = parameter.param_name != nullptr ? parameter.param_name : "";
			value.name_hash				  = hashing_t::to_sid(value.name);
			value.type					  = parameter.type;
			value.hint					  = parameter.hint;

			for (u8 i = 0; i < 4; ++i)
			{
				if (parameter.type == shader_param_type_e::u32)
					value.value_u32[i] = parameter.default_value_u32[i];
				else
					value.value[i] = parameter.default_value[i];
			}
		}

		return out;
	}

	void to_json(nlohmann::json& j, const material_texture_value_t& value)
	{
		j["name"]	 = value.name;
		j["texture"] = value.texture;
		j["type"]	 = shader_texture_type_to_string(value.type);
	}

	void from_json(const nlohmann::json& j, material_texture_value_t& value)
	{
		value = {};
		if (j.is_number_unsigned() || j.is_number_integer())
		{
			value.texture = j.get<resource_handle_t>();
			return;
		}

		value.name		= j.value<string_t>("name", {});
		value.name_hash = hashing_t::to_sid(value.name);
		value.texture	= j.value<resource_handle_t>("texture", NULL_RESOURCE_HANDLE);
		value.type		= shader_texture_type_from_string(j.value<string_t>("type", "texture2d"));
	}

	void to_json(nlohmann::json& j, const material_sampler_value_t& value)
	{
		j["name"]	 = value.name;
		j["sampler"] = value.sampler;
	}

	void from_json(const nlohmann::json& j, material_sampler_value_t& value)
	{
		value = {};
		if (j.is_number_unsigned() || j.is_number_integer())
		{
			value.sampler = j.get<resource_handle_t>();
			return;
		}

		value.name		= j.value<string_t>("name", {});
		value.name_hash = hashing_t::to_sid(value.name);
		value.sampler	= j.value<resource_handle_t>("sampler", NULL_RESOURCE_HANDLE);
	}

	void to_json(nlohmann::json& j, const material_param_value_t& value)
	{
		j["name"] = value.name;
		j["type"] = shader_param_type_to_string(value.type);
		j["hint"] = shader_param_hint_to_string(value.hint);
		if (value.type == shader_param_type_e::u32)
			j["value"] = {value.value_u32[0], value.value_u32[1], value.value_u32[2], value.value_u32[3]};
		else
			j["value"] = {value.value[0], value.value[1], value.value[2], value.value[3]};
	}

	void from_json(const nlohmann::json& j, material_param_value_t& value)
	{
		value			= {};
		value.name		= j.value<string_t>("name", {});
		value.name_hash = hashing_t::to_sid(value.name);
		value.type		= shader_param_type_from_string(j.value<string_t>("type", "invalid"));
		value.hint		= shader_param_hint_from_string(j.value<string_t>("hint", "none"));

		const nlohmann::json values = j.value("value", nlohmann::json::array());
		if (values.is_array())
		{
			const size_t count = values.size() < 4 ? values.size() : 4;
			for (size_t i = 0; i < count; ++i)
			{
				if (value.type == shader_param_type_e::u32)
					value.value_u32[i] = values[i].get<u32>();
				else
					value.value[i] = values[i].get<f32>();
			}
		}
	}

	void to_json(nlohmann::json& j, const material_def_t& value)
	{
		j["shader"]			   = value.shader;
		j["write_shadows"]	   = value.write_shadows;
		j["write_reflections"] = value.write_reflections;
		j["is_transparent"]	   = value.is_transparent;
		j["double_sided"]	   = value.double_sided;
		j["use_alpha_cutoff"]  = value.use_alpha_cutoff;
		j["textures"]		   = nlohmann::json::array();
		j["samplers"]		   = nlohmann::json::array();
		j["parameters"]		   = nlohmann::json::array();

		for (const material_texture_value_t& texture : value.textures)
			j["textures"].push_back(texture);

		for (const material_sampler_value_t& sampler : value.samplers)
			j["samplers"].push_back(sampler);

		for (const material_param_value_t& parameter : value.parameters)
			j["parameters"].push_back(parameter);
	}

	void from_json(const nlohmann::json& j, material_def_t& value)
	{
		value					= {};
		value.shader			= j.value<resource_handle_t>("shader", NULL_RESOURCE_HANDLE);
		value.write_shadows		= j.value<bool>("write_shadows", false);
		value.write_reflections = j.value<bool>("write_reflections", false);
		value.is_transparent	= j.value<bool>("is_transparent", false);
		value.double_sided		= j.value<bool>("double_sided", false);
		value.use_alpha_cutoff	= j.value<bool>("use_alpha_cutoff", false);

		const nlohmann::json textures = j.value("textures", nlohmann::json::array());

		for (const nlohmann::json& item : textures)
		{
			if (value.textures.full())
				break;
			value.textures.push_back(item.get<material_texture_value_t>());
		}

		const nlohmann::json samplers = j.value("samplers", nlohmann::json::array());

		for (const nlohmann::json& item : samplers)
		{
			if (value.samplers.full())
				break;
			value.samplers.push_back(item.get<material_sampler_value_t>());
		}

		const nlohmann::json parameters = j.value("parameters", nlohmann::json::array());

		for (const nlohmann::json& item : parameters)
		{
			if (value.parameters.full())
				break;
			value.parameters.push_back(item.get<material_param_value_t>());
		}
	}
}
