/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#include "vec3u.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>

namespace sfg
{
	vec3u_t vec3u_t::zero = {0, 0, 0};
	vec3u_t vec3u_t::one  = {1, 1, 1};

	vec3u_reflection_t::vec3u_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "vec3u_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(vec3u_t, x), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					{.name = "y", .offset = offsetof(vec3u_t, y), .size = sizeof(u32), .type = reflected_value_type_e::u32},
					{.name = "z", .offset = offsetof(vec3u_t, z), .size = sizeof(u32), .type = reflected_value_type_e::u32},
				},
			.type_id   = type_id_t<vec3u_t>::value,
			.size	   = sizeof(vec3u_t),
			.alignment = alignof(vec3u_t),
		});
	}
}
