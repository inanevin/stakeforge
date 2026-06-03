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

#include "descriptions_reflection.hpp"
#include "shader_description_reflection.hpp"
#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const sampler_filter_e& f)
	{
		switch (f)
		{
		case sampler_filter_e::anisotropic:
			j = "anisotropic";
			break;
		case sampler_filter_e::nearest:
			j = "nearest";
			break;
		case sampler_filter_e::linear:
			j = "linear";
			break;
		}
	}

	void from_json(const nlohmann::json& j, sampler_filter_e& f)
	{
		const string_t& str = j.get<string_t>();

		if (str.compare("anisotropic") == 0)
		{
			f = sampler_filter_e::anisotropic;
			return;
		}

		if (str.compare("nearest") == 0)
		{
			f = sampler_filter_e::nearest;
			return;
		}

		if (str.compare("linear") == 0)
		{
			f = sampler_filter_e::linear;
			return;
		}

		SFG_ASSERT(false);
	}

	void to_json(nlohmann::json& j, const sampler_border_e& b)
	{
		switch (b)
		{
		case sampler_border_e::transparent:
			j = "transparent";
			break;
		case sampler_border_e::white:
			j = "white";
			break;
		}
	}

	void from_json(const nlohmann::json& j, sampler_border_e& b)
	{
		const string_t& str = j.get<string_t>();

		if (str.compare("transparent") == 0)
		{
			b = sampler_border_e::transparent;
			return;
		}

		if (str.compare("white") == 0)
		{
			b = sampler_border_e::white;
			return;
		}

		SFG_ASSERT(false);
	}

	void to_json(nlohmann::json& j, const address_mode& a)
	{
		switch (a)
		{
		case address_mode::repeat:
			j = "repeat";
			break;
		case address_mode::border:
			j = "border";
			break;
		case address_mode::clamp:
			j = "clamp";
			break;
		case address_mode::mirrored_repeat:
			j = "mirrored_repeat";
			break;
		case address_mode::mirrored_clamp:
			j = "mirrored_clamp";
			break;
		}
	}

	void from_json(const nlohmann::json& j, address_mode& a)
	{
		const string_t& str = j.get<string_t>();

		if (str.compare("repeat") == 0)
		{
			a = address_mode::repeat;
			return;
		}

		if (str.compare("border") == 0)
		{
			a = address_mode::border;
			return;
		}

		if (str.compare("clamp") == 0)
		{
			a = address_mode::clamp;
			return;
		}

		if (str.compare("mirrored_repeat") == 0)
		{
			a = address_mode::mirrored_repeat;
			return;
		}

		if (str.compare("mirrored_clamp") == 0)
		{
			a = address_mode::mirrored_clamp;
			return;
		}

		SFG_ASSERT(false);
	}

	void to_json(nlohmann::json& j, const sampler_desc_t& s)
	{
		j["anisotropy"]	 = s.anisotropy;
		j["min_lod"]	 = s.min_lod;
		j["max_lod"]	 = s.max_lod;
		j["lod_bias"]	 = s.lod_bias;
		j["compare"]	 = s.compare;
		j["min_filter"]	 = s.min_filter;
		j["mag_filter"]	 = s.mag_filter;
		j["mip_filter"]	 = s.mip_filter;
		j["address_u"]	 = s.address_u;
		j["address_v"]	 = s.address_v;
		j["address_w"]	 = s.address_w;
		j["border"]		 = s.border;
		j["use_compare"] = s.use_compare;
	}

	void from_json(const nlohmann::json& j, sampler_desc_t& s)
	{
		s.anisotropy  = j.value<u32>("anisotropy", 0);
		s.min_lod	  = j.value<f32>("min_lod", .0f);
		s.max_lod	  = j.value<f32>("max_lod", .0f);
		s.lod_bias	  = j.value<f32>("lod_bias", .0f);
		s.compare	  = j.value<compare_op>("compare", compare_op::less);
		s.min_filter  = j.value<sampler_filter_e>("min_filter", j.value<sampler_filter_e>("min", sampler_filter_e::anisotropic));
		s.mag_filter  = j.value<sampler_filter_e>("mag_filter", j.value<sampler_filter_e>("mag", sampler_filter_e::anisotropic));
		s.mip_filter  = j.value<sampler_filter_e>("mip_filter", j.value<sampler_filter_e>("mip", sampler_filter_e::linear));
		s.address_u	  = j.value<address_mode>("address_u", address_mode::clamp);
		s.address_v	  = j.value<address_mode>("address_v", address_mode::clamp);
		s.address_w	  = j.value<address_mode>("address_w", address_mode::clamp);
		s.border	  = j.value<sampler_border_e>("border", sampler_border_e::transparent);
		s.use_compare = j.value<bool>("use_compare", false);
	}

}
