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

}
