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
#include "math/vector3.hpp"

namespace SFG
{
	struct plane;

	class ostream;
	class istream;

	struct aabb
	{
		aabb() = default;
		aabb(vector3 min, vector3 max)
		{
			bounds_min		   = min;
			bounds_max		   = max;
			bounds_half_extent = (max - min) / 2.0f;
		}
		~aabb() = default;

		vector3 bounds_half_extent = vector3::zero;
		vector3 bounds_min		   = vector3::zero;
		vector3 bounds_max		   = vector3::zero;

		bool	is_inside_plane(const vector3& center, const plane& plane);
		vector3 get_positive(const vector3& normal) const;
		vector3 get_negative(const vector3& normal) const;

		void remove(const aabb& other);
		void add(const aabb& other);
		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

		inline void update_half_extents()
		{
			bounds_half_extent = (bounds_max - bounds_min) / 2.0f;
		}
	};
}