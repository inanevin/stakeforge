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

#pragma once

#include "texture.hpp"
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class ostream_t;

	enum class texture_cook_payload_type_e : u8
	{
		uncompressed,
		ktx2_uastc,
	};

	struct texture_cook_config_t
	{
		texture_cook_payload_type_e payload_type	 = {};
		bool						generate_mipmaps = false;
		bool						is_linear		 = false;
	};

	class texture_cooker
	{
	public:
		static bool cook_from_file(const texture_cook_config_t& cfg, const char* full_path, ostream_t& stream);
	};

	void to_json(nlohmann::json& j, const texture_cook_payload_type_e& e);
	void from_json(const nlohmann::json& j, texture_cook_payload_type_e& e);
	void to_json(nlohmann::json& j, const texture_cook_config_t& c);
	void from_json(const nlohmann::json& j, texture_cook_config_t& c);
}
