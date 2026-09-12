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
#include "vec4f.hpp"

namespace sfg
{
	class ostream_t;
	class istream_t;

	class color_t
	{

	public:
		color_t(f32 rv = 1.0f, f32 gv = 1.0f, f32 bv = 1.0f, f32 av = 1.0f) : x(rv), y(gv), z(bv), w(av) {};
		static color_t from255(f32 r, f32 g, f32 b, f32 a);

		vec4f_t to_vector() const;
		void	round();
		void	serialize(ostream_t& stream) const;
		void	deserialize(istream_t& stream);

		bool operator!=(const color_t& rhs) const
		{
			return !(x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
		}

		bool operator==(const color_t& rhs) const
		{
			return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);
		}

		color_t operator*(const f32& rhs) const
		{
			return color_t(x * rhs, y * rhs, z * rhs, w * rhs);
		}

		color_t operator+(const color_t& rhs) const
		{
			return color_t(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
		}

		color_t operator/(f32 v) const
		{
			return color_t(x / v, y / v, z / v, w / v);
		}

		color_t operator/=(f32 v) const
		{
			return color_t(x / v, y / v, z / v, w / v);
		}

		color_t operator*=(f32 v) const
		{
			return color_t(x * v, y * v, z * v, w * v);
		}

		f32& operator[](unsigned int i)
		{
			return (&x)[i];
		}

		static color_t red;
		static color_t green;
		static color_t LightBlue;
		static color_t blue;
		static color_t DarkBlue;
		static color_t cyan;
		static color_t yellow;
		static color_t black;
		static color_t white;
		static color_t purple;
		static color_t maroon;
		static color_t beige;
		static color_t brown;
		static color_t gray;

		f32 x, y, z, w = 1.0f;
	};

	SFG_DEFINE_TYPE_ID(color_t);

	struct color_reflection_t
	{
		color_reflection_t();
	};

	inline color_reflection_t g_reflect_color;
} // namespace sfg
