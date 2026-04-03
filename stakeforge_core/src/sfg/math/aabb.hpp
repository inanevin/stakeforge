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
#include "math/vec3f.hpp"

namespace sfg
{
	struct plane_t;

	class ostream_t;
	class istream_t;

	struct aabb_t
	{
		aabb_t() = default;
		aabb_t(vec3f_t min, vec3f_t max)
		{
			bounds_min		   = min;
			bounds_max		   = max;
			bounds_half_extent = (max - min) / 2.0f;
		}
		~aabb_t() = default;

		vec3f_t bounds_half_extent = vec3f_t::zero;
		vec3f_t bounds_min		   = vec3f_t::zero;
		vec3f_t bounds_max		   = vec3f_t::zero;

		bool	is_inside_plane(const vec3f_t& center, const plane_t& plane_t);
		vec3f_t get_positive(const vec3f_t& normal) const;
		vec3f_t get_negative(const vec3f_t& normal) const;

		void remove(const aabb_t& other);
		void add(const aabb_t& other);
		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);

		inline void update_half_extents()
		{
			bounds_half_extent = (bounds_max - bounds_min) / 2.0f;
		}
	};
}
