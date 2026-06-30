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

#include "vec4u.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	vec4u_t vec4u_t::zero = {0, 0, 0, 0};
	vec4u_t vec4u_t::one  = {1, 1, 1, 1};

	vec4u_reflection_t::vec4u_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "vec4u_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(vec4u_t, x), .size = sizeof(u32), .type = reflected_value_type_e_v2::u32},
					{.name = "y", .offset = offsetof(vec4u_t, y), .size = sizeof(u32), .type = reflected_value_type_e_v2::u32},
					{.name = "z", .offset = offsetof(vec4u_t, z), .size = sizeof(u32), .type = reflected_value_type_e_v2::u32},
					{.name = "w", .offset = offsetof(vec4u_t, w), .size = sizeof(u32), .type = reflected_value_type_e_v2::u32},
				},
			.type_id   = type_id_t<vec4u_t>::value,
			.size	   = sizeof(vec4u_t),
			.alignment = alignof(vec4u_t),
		});
	}
}
