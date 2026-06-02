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
	void to_json(nlohmann::json& j, const sampler_desc_t& s)
		{
			j["anisotropy"] = s.anisotropy;
			j["min_lod"]	= s.min_lod;
			j["max_lod"]	= s.max_lod;
			j["lod_bias"]	= s.lod_bias;
			j["compare"]	= s.compare;
	
			// --- min filter ---
			if (s.flags.is_set(sampler_flags::saf_min_anisotropic))
				j["min"] = "anisotropic";
			else if (s.flags.is_set(sampler_flags::saf_min_linear))
				j["min"] = "linear";
			else if (s.flags.is_set(sampler_flags::saf_min_nearest))
				j["min"] = "nearest";
	
			// --- mag filter ---
			if (s.flags.is_set(sampler_flags::saf_mag_anisotropic))
				j["mag"] = "anisotropic";
			else if (s.flags.is_set(sampler_flags::saf_mag_linear))
				j["mag"] = "linear";
			else if (s.flags.is_set(sampler_flags::saf_mag_nearest))
				j["mag"] = "nearest";
	
			// --- mip filter ---
			if (s.flags.is_set(sampler_flags::saf_mip_nearest))
				j["mip"] = "nearest";
			else if (s.flags.is_set(sampler_flags::saf_mip_linear))
				j["mip"] = "linear";
	
			auto write_addr = [&](const char* name, address_mode mode) {
				if (mode == address_mode::repeat)
					j[name] = "repeat";
				else if (mode == address_mode::border)
					j[name] = "border";
				else if (mode == address_mode::clamp)
					j[name] = "clamp";
				else if (mode == address_mode::mirrored_repeat)
					j[name] = "mirrored_repeat";
				else if (mode == address_mode::mirrored_clamp)
					j[name] = "mirrored_clamp";
			};
	
			write_addr("address_u", s.address_u);
			write_addr("address_v", s.address_v);
			write_addr("address_w", s.address_w);
	
			// --- border color ---
			if (s.flags.is_set(sampler_flags::saf_border_transparent))
				j["border"] = "transparent";
			else if (s.flags.is_set(sampler_flags::saf_border_white))
				j["border"] = "white";
	
			if (s.flags.is_set(sampler_flags::saf_compare))
				j["use_compare"] = 1;
		}

	void from_json(const nlohmann::json& j, sampler_desc_t& s)
		{
			s.anisotropy		 = j.value<u32>("anisotropy", 0);
			s.min_lod			 = j.value<f32>("min_lod", .0f);
			s.max_lod			 = j.value<f32>("max_lod", .0f);
			s.lod_bias			 = j.value<f32>("lod_bias", .0f);
			s.flags				 = j.value<u16>("flags", 0);
			s.compare			 = j.value<compare_op>("compare", compare_op::less);
			const u8 use_compare = j.value<u8>("use_compare", 0);
			s.flags.set(sampler_flags::saf_compare, use_compare);
	
			const string_t min	  = j.value<string_t>("min", "anisotropic");
			const string_t mag	  = j.value<string_t>("mag", "anisotropic");
			const string_t mip	  = j.value<string_t>("mip", "linear");
			const string_t border = j.value<string_t>("border", "transparent");
	
			if (min.compare("anisotropic") == 0)
				s.flags.set(sampler_flags::saf_min_anisotropic);
			else if (min.compare("linear") == 0)
				s.flags.set(sampler_flags::saf_min_linear);
			else if (min.compare("nearest") == 0)
				s.flags.set(sampler_flags::saf_min_nearest);
	
			if (mag.compare("anisotropic") == 0)
				s.flags.set(sampler_flags::saf_mag_anisotropic);
			else if (mag.compare("linear") == 0)
				s.flags.set(sampler_flags::saf_mag_linear);
			else if (mag.compare("nearest") == 0)
				s.flags.set(sampler_flags::saf_mag_nearest);
	
			if (mip.compare("nearest") == 0)
				s.flags.set(sampler_flags::saf_mip_nearest);
			else if (mip.compare("linear") == 0)
				s.flags.set(sampler_flags::saf_mip_linear);
	
			auto read_addr = [&](const char* name, address_mode& out_mode) {
				const string_t s = j.value<string_t>(name, "clamp");
	
				if (s.compare("repeat") == 0)
					out_mode = address_mode::repeat;
				else if (s.compare("border") == 0)
					out_mode = address_mode::border;
				else if (s.compare("clamp") == 0)
					out_mode = address_mode::clamp;
				else if (s.compare("mirrored_repeat") == 0)
					out_mode = address_mode::mirrored_repeat;
				else if (s.compare("mirrored_clamp") == 0)
					out_mode = address_mode::mirrored_clamp;
			};
	
			read_addr("address_u", s.address_u);
			read_addr("address_v", s.address_v);
			read_addr("address_w", s.address_w);
	
			if (border.compare("transparent") == 0)
				s.flags.set(sampler_flags::saf_border_transparent);
			else if (border.compare("white") == 0)
				s.flags.set(sampler_flags::saf_border_white);
		}

}
