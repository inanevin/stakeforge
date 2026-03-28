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
#include "common/size_definitions.hpp"

#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json_fwd.hpp"
#endif

namespace SFG
{

	class istream;
	class ostream;

	struct vec2u16
	{
	public:
		vec2u16(){};
		vec2u16(u32 _x, u32 _y) : x(_x), y(_y){};

		static vec2u16 zero;
		static vec2u16 one;

		void serialize(ostream& out) const;
		void deserialize(istream& in);

		inline bool operator==(const vec2u16& other) const
		{
			return x == other.x && y == other.y;
		}

		inline vec2u16 operator/(u16 val) const
		{
			return vec2u16(x / val, y / val);
		}
		u16 x = 0;
		u16 y = 0;
	};

#ifdef SFG_JSON_SERIALIZE

	void to_json(nlohmann::json& j, const vec2u16& v);
	void from_json(const nlohmann::json& j, vec2u16& v);

#endif

}
