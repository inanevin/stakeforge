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

#include "rectf.hpp"
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include "math.hpp"
#include "vec2f.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>

namespace sfg
{
	const rectf_t rectf_t::zero = {0.0f, 0.0f, 0.0f, 0.0f};
	const rectf_t rectf_t::one	= {0.0f, 0.0f, 1.0f, 1.0f};

	rectf_t rectf_t::from_min_max(const vec2f_t& min, const vec2f_t& max)
	{
		return {min.x, min.y, max.x - min.x, max.y - min.y};
	}

	rectf_t rectf_t::from_min_max(f32 min_x, f32 min_y, f32 max_x, f32 max_y)
	{
		return {min_x, min_y, max_x - min_x, max_y - min_y};
	}

	bool rectf_t::equals(const rectf_t& other, f32 epsilon) const
	{
		return math::almost_equal(x, other.x, epsilon) && math::almost_equal(y, other.y, epsilon) && math::almost_equal(w, other.w, epsilon) && math::almost_equal(h, other.h, epsilon);
	}

	bool rectf_t::is_zero(f32 epsilon) const
	{
		return math::almost_equal(x, 0.0f, epsilon) && math::almost_equal(y, 0.0f, epsilon) && math::almost_equal(w, 0.0f, epsilon) && math::almost_equal(h, 0.0f, epsilon);
	}

	bool rectf_t::is_point_inside(const vec2f_t& point) const
	{
		return is_point_inside(point.x, point.y);
	}

	bool rectf_t::is_point_inside(f32 px, f32 py) const
	{
		return contains(px, py);
	}

	bool rectf_t::is_overlapping(const rectf_t& other) const
	{
		return x < other.get_right() && get_right() > other.x && y < other.get_bottom() && get_bottom() > other.y;
	}

	bool rectf_t::is_inside(const rectf_t& other) const
	{
		return other.contains(*this);
	}

	bool rectf_t::contains(const rectf_t& other) const
	{
		return other.x >= x && other.get_right() <= get_right() && other.y >= y && other.get_bottom() <= get_bottom();
	}

	bool rectf_t::contains(const vec2f_t& point) const
	{
		return contains(point.x, point.y);
	}

	bool rectf_t::contains(f32 px, f32 py) const
	{
		return px >= x && px <= get_right() && py >= y && py <= get_bottom();
	}

	rectf_t rectf_t::expand(f32 value) const
	{
		return {x - value, y - value, w + value * 2.0f, h + value * 2.0f};
	}

	vec2f_t rectf_t::get_min() const
	{
		return {x, y};
	}

	vec2f_t rectf_t::get_max() const
	{
		return {get_right(), get_bottom()};
	}

	vec2f_t rectf_t::get_pos() const
	{
		return {x, y};
	}

	vec2f_t rectf_t::get_size() const
	{
		return {w, h};
	}

	f32 rectf_t::get_left() const
	{
		return x;
	}

	f32 rectf_t::get_right() const
	{
		return x + w;
	}

	f32 rectf_t::get_top() const
	{
		return y;
	}

	f32 rectf_t::get_bottom() const
	{
		return y + h;
	}

	void rectf_t::serialize(ostream_t& stream) const
	{
		stream << x << y << w << h;
	}

	void rectf_t::deserialize(istream_t& stream)
	{
		stream >> x >> y >> w >> h;
	}

}

namespace sfg
{
	rectf_reflection_t::rectf_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "rectf_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(rectf_t, x), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "y", .offset = offsetof(rectf_t, y), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "w", .offset = offsetof(rectf_t, w), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "h", .offset = offsetof(rectf_t, h), .size = sizeof(f32), .type = reflected_value_type_e::f32},
				},
			.type_id   = type_id_t<rectf_t>::value,
			.size	   = sizeof(rectf_t),
			.alignment = alignof(rectf_t),
		});
	}
}
