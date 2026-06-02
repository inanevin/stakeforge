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

#include "texture_cook_reflection.hpp"
#include <sfg/math/vec2u16_reflection.hpp>
#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const texture_payload_type_e& e)
		{
			switch (e)
			{
			case texture_payload_type_e::uncompressed:
				j = "uncompressed";
				return;
			case texture_payload_type_e::ktx2_uastc:
				j = "ktx2_uastc";
				return;
			}
	
			j = "uncompressed";
		}

	void to_json(nlohmann::json& j, const texture_cook_config_t& c)
		{
			j["payload_type"]	  = c.payload_type;
			j["generate_mipmaps"] = c.generate_mipmaps;
			j["is_linear"]		  = c.is_linear;
			j["size"]			  = c.size;
		}

	void from_json(const nlohmann::json& j, texture_payload_type_e& e)
		{
			const string_t str = j.get<string_t>();
	
			if (str.compare("uncompressed") == 0)
			{
				e = texture_payload_type_e::uncompressed;
				return;
			}
			if (str.compare("ktx2_uastc") == 0)
			{
				e = texture_payload_type_e::ktx2_uastc;
				return;
			}
	
			e = texture_payload_type_e::uncompressed;
		}

	void from_json(const nlohmann::json& j, texture_cook_config_t& c)
		{
			c.payload_type	   = j.value<texture_payload_type_e>("payload_type", texture_payload_type_e::uncompressed);
			c.generate_mipmaps = j.value<bool>("generate_mipmaps", false);
			c.is_linear		   = j.value<bool>("is_linear", false);
			c.size			   = j.value<vec2u16_t>("size", vec2u16_t::zero);
		}

}
