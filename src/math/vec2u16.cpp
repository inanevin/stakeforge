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

#include "vec2u16.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#ifdef SFG_JSON_SERIALIZE
#include "vendor/nhlohmann/json.hpp"
#endif

namespace SFG
{
	vec2u16 vec2u16::zero = vec2u16(0, 0);
	vec2u16 vec2u16::one  = vec2u16(1, 1);

	void vec2u16::serialize(ostream& out) const
	{
		out << x << y;
	}

	void vec2u16::deserialize(istream& in)
	{
		in >> x >> y;
	}

#ifdef SFG_JSON_SERIALIZE

	void to_json(nlohmann::json& j, const vec2u16& v)
	{
		j = nlohmann::json::array({v.x, v.y});
	}

	void from_json(const nlohmann::json& j, vec2u16& v)
	{
		if (!j.is_array() || j.size() < 2)
			throw std::runtime_error("vec2u16 json err");
		v.x = j.at(0).get<u16>();
		v.y = j.at(1).get<u16>();
	}
#endif
}
