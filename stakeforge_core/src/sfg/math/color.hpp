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
#include "vec4f.hpp"

#include "vendor/nhlohmann/json_fwd.hpp"

namespace sfg
{
	class ostream_t;
	class istream_t;

	class color_t
	{

	public:
		color_t(f32 rv = 1.0f, f32 gv = 1.0f, f32 bv = 1.0f, f32 av = 1.0f) : x(rv), y(gv), z(bv), w(av) {};
		static color_t from255(f32 r, f32 g, f32 b, f32 a);
		color_t		   linear_to_srgb();
		color_t		   srgb_to_linear();

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

	void to_json(nlohmann::json& j, const color_t& c);
	void from_json(const nlohmann::json& j, color_t& c);

} // namespace sfg
