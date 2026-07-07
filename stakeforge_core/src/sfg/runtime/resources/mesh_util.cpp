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

#include "mesh_util.hpp"

#include <sfg/math/math.hpp>

namespace sfg
{
	namespace
	{
		vec3f_t get_fallback_tangent(const vec3f_t& normal)
		{
			const vec3f_t n = normal.normalized();
			if (n.is_zero())
				return vec3f_t::zero;

			const vec3f_t axis = math::abs(n.z) < 0.999f ? vec3f_t{0.0f, 0.0f, 1.0f} : vec3f_t{0.0f, 1.0f, 0.0f};
			return vec3f_t::cross(axis, n).normalized();
		}

		template <typename vertex_t> bool generate_tangents_impl(vector_t<vertex_t>& vertices, const vector_t<primitive_index>& indices)
		{
			if (indices.size() % 3 != 0)
				return false;

			vector_t<vec3f_t> tangents;
			vector_t<vec3f_t> bitangents;
			tangents.resize(vertices.size());
			bitangents.resize(vertices.size());

			for (size_t i = 0; i < indices.size(); i += 3)
			{
				const primitive_index i0 = indices[i];
				const primitive_index i1 = indices[i + 1];
				const primitive_index i2 = indices[i + 2];
				if (static_cast<size_t>(i0) >= vertices.size() || static_cast<size_t>(i1) >= vertices.size() || static_cast<size_t>(i2) >= vertices.size())
					return false;

				const vertex_t& v0 = vertices[i0];
				const vertex_t& v1 = vertices[i1];
				const vertex_t& v2 = vertices[i2];

				const vec3f_t delta_pos1 = v1.pos - v0.pos;
				const vec3f_t delta_pos2 = v2.pos - v0.pos;
				const vec2f_t delta_uv1	 = v1.uv - v0.uv;
				const vec2f_t delta_uv2	 = v2.uv - v0.uv;
				const f32	  det		 = delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x;
				if (math::abs(det) <= MATH_EPS)
					continue;

				const f32	  inv_det	= 1.0f / det;
				const vec3f_t tangent	= (delta_pos1 * delta_uv2.y - delta_pos2 * delta_uv1.y) * inv_det;
				const vec3f_t bitangent = (delta_pos2 * delta_uv1.x - delta_pos1 * delta_uv2.x) * inv_det;

				tangents[i0] += tangent;
				tangents[i1] += tangent;
				tangents[i2] += tangent;
				bitangents[i0] += bitangent;
				bitangents[i1] += bitangent;
				bitangents[i2] += bitangent;
			}

			for (size_t i = 0; i < vertices.size(); ++i)
			{
				vertex_t&	  vertex  = vertices[i];
				const vec3f_t normal  = vertex.normal.normalized();
				vec3f_t		  tangent = tangents[i] - normal * vec3f_t::dot(normal, tangents[i]);
				if (tangent.magnitude_sqr() <= MATH_EPS * MATH_EPS)
					tangent = get_fallback_tangent(normal);
				else
					tangent.normalize();

				const f32 handedness = vec3f_t::dot(vec3f_t::cross(normal, tangent), bitangents[i]) < 0.0f ? -1.0f : 1.0f;
				vertex.tangent		 = {tangent.x, tangent.y, tangent.z, handedness};
			}

			return true;
		}
	}

	bool mesh_util_t::generate_tangents(primitive_static_def_t& primitive)
	{
		return generate_tangents_impl(primitive.vertices, primitive.indices);
	}

	bool mesh_util_t::generate_tangents(primitive_skinned_def_t& primitive)
	{
		return generate_tangents_impl(primitive.vertices, primitive.indices);
	}
}
