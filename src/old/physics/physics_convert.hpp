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

#include "physics/common_physics.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "math/color.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Physics/Body/MotionType.h>

namespace SFG
{
	static inline JPH::Vec3 to_jph_vec3(const vector3& v)
	{
		return JPH::Vec3(v.x, v.y, v.z);
	}

	static inline JPH::Quat to_jph_quat(const quat& q)
	{
		return JPH::Quat(q.x, q.y, q.z, q.w);
	}

	static inline JPH::Color to_jph_color(const color& c)
	{
		return JPH::Color(c.x, c.y, c.z, c.w);
	}

	static inline vector3 from_jph_vec3(const JPH::Vec3& v)
	{
		return vector3(v.GetX(), v.GetY(), v.GetZ());
	}

	static inline quat from_jph_quat(const JPH::Quat& q)
	{
		return quat(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
	}

	static inline color from_jph_color(const JPH::Color& c)
	{
		return color(c.r, c.g, c.b, c.a);
	}
}
