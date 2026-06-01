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

#include "material_json.hpp"

#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void material_parameter_t::serialize(ostream_t& stream) const
	{
		stream << type;
		for (u8 i = 0; i < 4; ++i)
			stream << values[i];
	}

	void material_parameter_t::deserialize(istream_t& stream)
	{
		stream >> type;
		for (u8 i = 0; i < 4; ++i)
			stream >> values[i];
	}

	void material_json_t::serialize(ostream_t& stream) const
	{
		stream << static_cast<u32>(pass_flags);
		shader.serialize(stream);

		stream << static_cast<u32>(textures.size());
		for (const resource_json_ref_t& texture : textures)
			texture.serialize(stream);

		stream << static_cast<u32>(parameters.size());
		for (const material_parameter_t& parameter : parameters)
			parameter.serialize(stream);

		stream << double_sided;
		stream << use_alpha_cutoff;
		sampler_definition.serialize(stream);
	}

	void material_json_t::deserialize(istream_t& stream)
	{
		u32 flags = 0;
		stream >> flags;
		pass_flags = static_cast<world_pass_flags>(flags);
		shader.deserialize(stream);

		u32 texture_count = 0;
		stream >> texture_count;
		textures.resize(texture_count);
		for (resource_json_ref_t& texture : textures)
			texture.deserialize(stream);

		u32 parameter_count = 0;
		stream >> parameter_count;
		parameters.resize(parameter_count);
		for (material_parameter_t& parameter : parameters)
			parameter.deserialize(stream);

		stream >> double_sided;
		stream >> use_alpha_cutoff;
		sampler_definition.deserialize(stream);
	}

	void to_json(nlohmann::json& j, const material_parameter_type_e& t)
	{
		switch (t)
		{
		case material_parameter_type_e::u32:
			j = "u32";
			break;
		case material_parameter_type_e::uint2:
			j = "uint2";
			break;
		case material_parameter_type_e::uint4:
			j = "uint4";
			break;
		case material_parameter_type_e::i32:
			j = "i32";
			break;
		case material_parameter_type_e::vec2f:
			j = "vec2f";
			break;
		case material_parameter_type_e::vec4f:
			j = "vec4f";
			break;
		default:
			j = "f32";
			break;
		}
	}

	void from_json(const nlohmann::json& j, material_parameter_type_e& t)
	{
		const string_t s = j.get<string_t>();
		if (s == "u32")
			t = material_parameter_type_e::u32;
		else if (s == "uint2")
			t = material_parameter_type_e::uint2;
		else if (s == "uint4")
			t = material_parameter_type_e::uint4;
		else if (s == "i32")
			t = material_parameter_type_e::i32;
		else if (s == "vec2f")
			t = material_parameter_type_e::vec2f;
		else if (s == "vec4f")
			t = material_parameter_type_e::vec4f;
		else
			t = material_parameter_type_e::f32;
	}

	void to_json(nlohmann::json& j, const material_parameter_t& p)
	{
		j["type"]	= p.type;
		j["values"] = {p.values[0], p.values[1], p.values[2], p.values[3]};
	}

	void from_json(const nlohmann::json& j, material_parameter_t& p)
	{
		p.type					   = j.value<material_parameter_type_e>("type", material_parameter_type_e::f32);
		const vector_t<f32> values = j.value<vector_t<f32>>("values", {});
		for (u8 i = 0; i < 4 && i < values.size(); ++i)
			p.values[i] = values[i];
	}

	void to_json(nlohmann::json& j, const material_json_t& m)
	{
		j["schema"]				= "sfg.schema.material";
		j["pass_flags"]			= m.pass_flags;
		j["shader"]				= m.shader;
		j["textures"]			= m.textures;
		j["parameters"]			= m.parameters;
		j["double_sided"]		= m.double_sided;
		j["use_alpha_cutoff"]	= m.use_alpha_cutoff;
		j["sampler_definition"] = m.sampler_definition;
	}

	void from_json(const nlohmann::json& j, material_json_t& m)
	{
		m.pass_flags		 = j.value<world_pass_flags>("pass_flags", wpf_none);
		m.shader			 = j.value<resource_json_ref_t>("shader", {});
		m.textures			 = j.value<vector_t<resource_json_ref_t>>("textures", {});
		m.parameters		 = j.value<vector_t<material_parameter_t>>("parameters", {});
		m.double_sided		 = j.value<bool>("double_sided", false);
		m.use_alpha_cutoff	 = j.value<bool>("use_alpha_cutoff", false);
		m.sampler_definition = j.value<sampler_desc_t>("sampler_definition", {});
	}
}
