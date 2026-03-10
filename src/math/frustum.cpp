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

#include "frustum.hpp"
#include "aabb.hpp"
#include "matrix4x4.hpp"
#include "matrix3x3.hpp"

namespace SFG
{
	frustum_result frustum::test(const frustum& fr, const aabb& local_box)
	{
		frustum_result test = frustum_result::inside;

		auto performTest = [&](const plane& p) {
			const f32	  pos	 = -p.distance;
			const vector3 normal = p.normal;

			if (vector3::dot(normal, local_box.get_positive(normal)) + pos < 0.0f)
				test = frustum_result ::outside;

			if (vector3::dot(normal, local_box.get_negative(normal)) + pos < 0.0f)
				test = frustum_result::intersects;
		};

		performTest(fr.left);
		if (test == frustum_result::outside)
			return test;

		performTest(fr.right);
		if (test == frustum_result::outside)
			return test;

		performTest(fr.top);
		if (test == frustum_result::outside)
			return test;

		performTest(fr.bottom);
		if (test == frustum_result::outside)
			return test;

		performTest(fr.near);
		if (test == frustum_result::outside)
			return test;

		performTest(fr.far);

		return test;
	}

	frustum_result frustum::test(const frustum& fr, const vector3& position, f32 sphere_radius)
	{
		auto performTest = [&](const plane& p) {
			const f32 distance = vector3::dot(p.normal, position) + p.distance;

			if (distance < -sphere_radius)
				return frustum_result::outside;

			return frustum_result::inside;
		};

		if (performTest(fr.left) == frustum_result::outside)
			return frustum_result::outside;

		if (performTest(fr.right) == frustum_result::outside)
			return frustum_result::outside;

		if (performTest(fr.top) == frustum_result::outside)
			return frustum_result::outside;

		if (performTest(fr.bottom) == frustum_result::outside)
			return frustum_result::outside;

		if (performTest(fr.near) == frustum_result::outside)
			return frustum_result::outside;

		if (performTest(fr.far) == frustum_result::outside)
			return frustum_result::outside;

		return frustum_result::inside;
	}

	frustum_result frustum::test(const frustum& fr, const aabb& local_box, const matrix3x3& linear_model, const vector3& position)
	{
		const vector3 c_local = (local_box.bounds_min + local_box.bounds_max) * 0.5f;
		const vector3 e_local = (local_box.bounds_max - local_box.bounds_min) * 0.5f;

		frustum_result agg = frustum_result::inside;

		auto acc = [&](const plane& pl) {
			const frustum_result r = classify_obb_vs_plane(pl, c_local, e_local, linear_model, position);
			if (r == frustum_result::outside)
				return frustum_result::outside;
			if (r == frustum_result::intersects)
				agg = frustum_result::intersects;
			return agg;
		};

		if (acc(fr.left) == frustum_result::outside)
			return frustum_result::outside;
		if (acc(fr.right) == frustum_result::outside)
			return frustum_result::outside;
		if (acc(fr.top) == frustum_result::outside)
			return frustum_result::outside;
		if (acc(fr.bottom) == frustum_result::outside)
			return frustum_result::outside;
		if (acc(fr.near) == frustum_result::outside)
			return frustum_result::outside;
		if (acc(fr.far) == frustum_result::outside)
			return frustum_result::outside;

		return agg;
	}

	frustum_result frustum::classify_obb_vs_plane(const plane& p, const vector3& c_local, const vector3& e_local, const matrix3x3& linear_model, const vector3& position)
	{
		// world-space center
		const vector3 c_world = linear_model * c_local + position;

		// r = |L^T * n| · e_local
		const vector3 v = linear_model.transposed() * p.normal;
		const f32	  r = vector3::dot(vector3::abs(v), e_local);
		const f32	  s = vector3::dot(p.normal, c_world) + p.distance;

		if (s < -r)
			return frustum_result::outside;
		if (s > r)
			return frustum_result::inside;
		return frustum_result::intersects;
	}

	frustum frustum::extract(const matrix4x4& m)
	{
		frustum fr = {};
		fr.left	   = plane(m[3] + m[0], m[7] + m[4], m[11] + m[8], m[15] + m[12]);
		fr.right   = plane(m[3] - m[0], m[7] - m[4], m[11] - m[8], m[15] - m[12]);
		fr.bottom  = plane(m[3] + m[1], m[7] + m[5], m[11] + m[9], m[15] + m[13]);
		fr.top	   = plane(m[3] - m[1], m[7] - m[5], m[11] - m[9], m[15] - m[13]);
		fr.near	   = plane(m[3] + m[2], m[7] + m[6], m[11] + m[10], m[15] + m[14]);
		fr.far	   = plane(m[3] - m[2], m[7] - m[6], m[11] - m[10], m[15] - m[14]);
		fr.left.normalize();
		fr.right.normalize();
		fr.bottom.normalize();
		fr.near.normalize();
		fr.far.normalize();
		fr.top.normalize();
		return fr;
	}
}