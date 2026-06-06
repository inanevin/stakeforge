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

#include "material_def_reflection.hpp"
#include <sfg/runtime/render/world_draw_common_reflection.hpp>
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
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

	void to_json(nlohmann::json& j, const material_parameter_t& p)
	{
		j["type"]	= p.type;
		j["values"] = {p.values[0], p.values[1], p.values[2], p.values[3]};
	}

	void to_json(nlohmann::json& j, const material_def_t& m)
	{
		j["schema"]			  = "sfg.schema.material";
		j["pass_flags"]		  = m.pass_flags;
		j["shader"]			  = m.shader;
		j["sampler"]		  = m.sampler;
		j["textures"]		  = m.textures;
		j["parameters"]		  = m.parameters;
		j["double_sided"]	  = m.double_sided;
		j["use_alpha_cutoff"] = m.use_alpha_cutoff;
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

	void from_json(const nlohmann::json& j, material_def_t& m)
	{
		m.pass_flags	   = j.value<world_pass_flags_e>("pass_flags", wpf_none);
		m.shader		   = j.value<sid_t>("shader", NULL_SID);
		m.sampler		   = j.value<sid_t>("sampler", NULL_SID);
		m.textures		   = j.value<vector_t<sid_t>>("textures", {});
		m.parameters	   = j.value<vector_t<material_parameter_t>>("parameters", {});
		m.double_sided	   = j.value<bool>("double_sided", false);
		m.use_alpha_cutoff = j.value<bool>("use_alpha_cutoff", false);
		SFG_ASSERT(m.textures.empty() || m.sampler != NULL_SID);
	}

	void from_json(const nlohmann::json& j, material_parameter_t& p)
	{
		p.type					   = j.value<material_parameter_type_e>("type", material_parameter_type_e::f32);
		const vector_t<f32> values = j.value<vector_t<f32>>("values", {});
		for (u8 i = 0; i < 4 && i < values.size(); ++i)
			p.values[i] = values[i];
	}

}
