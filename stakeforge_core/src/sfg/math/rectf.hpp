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

#pragma once

#include <sfg/common/type_id.hpp>

#include "math_common.hpp"
#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;

	struct vec2f_t;

	struct rectf_t
	{
		f32 x;
		f32 y;
		f32 w;
		f32 h;

		static const rectf_t zero;
		static const rectf_t one;

		static rectf_t from_min_max(const vec2f_t& min, const vec2f_t& max);
		static rectf_t from_min_max(f32 min_x, f32 min_y, f32 max_x, f32 max_y);

		bool	equals(const rectf_t& other, f32 epsilon = MATH_EPS) const;
		bool	is_zero(f32 epsilon = MATH_EPS) const;
		bool	is_point_inside(const vec2f_t& point) const;
		bool	is_point_inside(f32 px, f32 py) const;
		bool	is_overlapping(const rectf_t& other) const;
		bool	is_inside(const rectf_t& other) const;
		bool	contains(const rectf_t& other) const;
		bool	contains(const vec2f_t& point) const;
		bool	contains(f32 px, f32 py) const;
		rectf_t expand(f32 value) const;

		vec2f_t get_min() const;
		vec2f_t get_max() const;
		vec2f_t get_pos() const;
		vec2f_t get_size() const;
		f32		get_left() const;
		f32		get_right() const;
		f32		get_top() const;
		f32		get_bottom() const;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		inline bool operator==(const rectf_t& other) const
		{
			return equals(other);
		}
		inline bool operator!=(const rectf_t& other) const
		{
			return !equals(other);
		}
	};

	SFG_DEFINE_TYPE_ID(rectf_t);

	struct rectf_reflection_t
	{
		rectf_reflection_t();
	};

	inline rectf_reflection_t g_reflect_rectf;
}
