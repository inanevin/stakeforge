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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/render/world_draw_common.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;

	enum class material_parameter_type_e : u8
	{
		u32,
		uint2,
		uint4,
		i32,
		f32,
		vec2f,
		vec4f,
	};

	struct material_parameter_t
	{
		f32						  values[4] = {};
		material_parameter_type_e type		= material_parameter_type_e::f32;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	struct material_json_t
	{
		vector_t<sid_t>				   textures			= {};
		vector_t<material_parameter_t> parameters		= {};
		sid_t						   shader			= NULL_SID;
		sid_t						   sampler			= NULL_SID;
		world_pass_flags			   pass_flags		= wpf_none;
		bool						   double_sided		= false;
		bool						   use_alpha_cutoff = false;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	void to_json(nlohmann::json& j, const material_parameter_type_e& t);
	void from_json(const nlohmann::json& j, material_parameter_type_e& t);
	void to_json(nlohmann::json& j, const material_parameter_t& p);
	void from_json(const nlohmann::json& j, material_parameter_t& p);
	void to_json(nlohmann::json& j, const material_json_t& m);
	void from_json(const nlohmann::json& j, material_json_t& m);
}
