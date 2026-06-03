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

#include "physical_material_json_reflection.hpp"
#include <sfg/data/string.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void to_json(nlohmann::json& j, const physical_material_json_t& m)
	{
		j["schema"]			 = "sfg.schema.physical_material";
		j["restitution"]	 = m.restitution;
		j["friction"]		 = m.friction;
		j["angular_damping"] = m.angular_damping;
		j["linear_damping"]	 = m.linear_damping;
	}

	void from_json(const nlohmann::json& j, physical_material_json_t& m)
	{
		m.restitution	  = j.value<f32>("restitution", 0.0f);
		m.friction		  = j.value<f32>("friction", 0.2f);
		m.angular_damping = j.value<f32>("angular_damping", j.value<f32>("angular_damp", 0.05f));
		m.linear_damping  = j.value<f32>("linear_damping", j.value<f32>("linear_damp", 0.05f));
	}

}
