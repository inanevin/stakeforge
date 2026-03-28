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

#include "physics_material_settings.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_JSON_SERIALIZE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	void physical_material_settings::serialize(ostream& stream) const
	{
		stream << restitution;
		stream << friction;
		stream << angular_damp;
		stream << linear_damp;
		stream << mass;
		stream << gravity_multiplier;
	}

	void physical_material_settings::deserialize(istream& stream)
	{
		stream >> restitution;
		stream >> friction;
		stream >> angular_damp;
		stream >> linear_damp;
		stream >> mass;
		stream >> gravity_multiplier;
	}

#ifdef SFG_JSON_SERIALIZE
	void to_json(nlohmann::json& j, const physical_material_settings& s)
	{
		j["restitution"]		= s.restitution;
		j["friction"]			= s.friction;
		j["angular_damp"]		= s.angular_damp;
		j["linear_damp"]		= s.linear_damp;
		j["mass"]				= s.mass;
		j["gravity_multiplier"] = s.gravity_multiplier;
	}

	void from_json(const nlohmann::json& j, physical_material_settings& s)
	{
		s.restitution		 = j.value<f32>("restitution", 0.0f);
		s.friction			 = j.value<f32>("friction", 0.2f);
		s.angular_damp		 = j.value<f32>("angular_damp", 0.05f);
		s.linear_damp		 = j.value<f32>("linear_damp", 0.05f);
		s.mass				 = j.value<f32>("mass", 1.0f);
		s.gravity_multiplier = j.value<f32>("gravity_multiplier", 1.0f);
	}
#endif

}
