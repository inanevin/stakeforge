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

#include <sfg/common/type_id.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/physics/physics_config.hpp>

namespace sfg
{
	struct physics_collision_layer_definition_t
	{
		string_t name		   = {};
		u64		 collides_with = 0;
		u64		 identifier	   = 0;
		u8		 slot		   = 0;

		bool operator==(const physics_collision_layer_definition_t&) const = default;
	};

	struct physics_project_settings_t
	{
		physics_project_settings_t();

		vector_t<physics_collision_layer_definition_t> collision_layers				   = {};
		u64											   next_collision_layer_identifier = 2;

		void					 normalize(const physics_project_settings_t* previous = nullptr);
		physics_runtime_config_t make_runtime_config(u32 physics_rate, u32 max_sub_steps) const;

		bool operator==(const physics_project_settings_t&) const = default;
	};

	SFG_DEFINE_TYPE_ID(physics_collision_layer_definition_t);
	SFG_DEFINE_TYPE_ID(physics_project_settings_t);

	struct physics_project_settings_reflection_t
	{
		physics_project_settings_reflection_t();
	};

	inline physics_project_settings_reflection_t g_reflect_physics_project_settings;
}
