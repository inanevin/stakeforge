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

#include "vec2u16.hpp"
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/istream.hpp>

namespace sfg
{

	vec2u16_t vec2u16_t::zero = {};
	vec2u16_t vec2u16_t::one  = {.x = 1, .y = 1};

	void vec2u16_t::serialize(ostream_t& out) const
	{
		out << x << y;
	}

	void vec2u16_t::deserialize(istream_t& in)
	{
		in >> x >> y;
	}

}

namespace sfg
{
	vec2u16_reflection_t::vec2u16_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "vec2u16_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(vec2u16_t, x), .size = sizeof(u16), .type = reflected_value_type_e::u16},
					{.name = "y", .offset = offsetof(vec2u16_t, y), .size = sizeof(u16), .type = reflected_value_type_e::u16},
				},
			.type_id   = type_id_t<vec2u16_t>::value,
			.size	   = sizeof(vec2u16_t),
			.alignment = alignof(vec2u16_t),
		});
	}
}
