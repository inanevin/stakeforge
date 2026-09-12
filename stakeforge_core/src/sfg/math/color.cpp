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

#include "color.hpp"
#include <sfg/reflection/reflection_registry.hpp>
#include "math.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>

namespace sfg
{
	color_t color_t::red	= color_t(1, 0, 0, 1);
	color_t color_t::green	= color_t(0, 1, 0);
	color_t color_t::blue	= color_t(0, 0, 1);
	color_t color_t::cyan	= color_t(0, 1, 1);
	color_t color_t::yellow = color_t(1, 1, 0);
	color_t color_t::black	= color_t(0, 0, 0);
	color_t color_t::white	= color_t(1, 1, 1);
	color_t color_t::purple = color_t(1, 0, 1);
	color_t color_t::maroon = color_t(0.5f, 0, 0);
	color_t color_t::beige	= color_t(0.96f, 0.96f, 0.862f);
	color_t color_t::brown	= color_t(0.647f, 0.164f, 0.164f);
	color_t color_t::gray	= color_t(0.5f, 0.5f, 0.5f);

	color_t color_t::from255(f32 r, f32 g, f32 b, f32 a)
	{
		return color_t(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	}

	void color_t::round()
	{
		x = math::round(x);
		y = math::round(y);
		z = math::round(z);
		w = math::round(w);
	}

	void color_t::serialize(ostream_t& stream) const
	{
		stream << x << y << z << w;
	}

	void color_t::deserialize(istream_t& stream)
	{
		stream >> x >> y >> z >> w;
	}

	vec4f_t color_t::to_vector() const
	{
		return vec4f_t(x, y, z, w);
	}

} // namespace sfg

namespace sfg
{
	color_reflection_t::color_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "color_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(color_t, x), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "y", .offset = offsetof(color_t, y), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "z", .offset = offsetof(color_t, z), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "w", .offset = offsetof(color_t, w), .size = sizeof(f32), .type = reflected_value_type_e::f32},
				},
			.type_id   = type_id_t<color_t>::value,
			.size	   = sizeof(color_t),
			.alignment = alignof(color_t),
		});
	}
}
