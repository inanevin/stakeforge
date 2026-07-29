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

#include "triangulation_2d.hpp"

#include "math.hpp"

namespace sfg
{
	bool math::triangulate_2d(span_t<const vec2f_t> points, vector_t<triangle_indices_t>& out_triangles)
	{
		struct edge_t
		{
			u32 first  = UINT32_MAX;
			u32 second = UINT32_MAX;
		};

		out_triangles.resize(0);

		for (u32 point_index = 0; point_index < points.size; ++point_index)
		{
			for (u32 other_index = point_index + 1; other_index < points.size; ++other_index)
			{
				if (points[point_index].equals(points[other_index]))
					return false;
			}
		}

		if (points.size < 3)
			return true;

		vec2f_t bounds_min = points[0];
		vec2f_t bounds_max = points[0];

		for (u32 point_index = 1; point_index < points.size; ++point_index)
		{
			bounds_min = vec2f_t::min(bounds_min, points[point_index]);
			bounds_max = vec2f_t::max(bounds_max, points[point_index]);
		}

		const vec2f_t bounds_size = bounds_max - bounds_min;
		const f32	  bounds_span = math::max(bounds_size.x, bounds_size.y);

		if (bounds_span <= MATH_EPS)
			return true;

		const vec2f_t bounds_center = (bounds_min + bounds_max) * 0.5f;
		const u32	  super_first	= static_cast<u32>(points.size);

		vector_t<vec2f_t> working_points = {};
		working_points.reserve(points.size + 3);

		for (const vec2f_t& point : points)
			working_points.push_back(point);

		working_points.push_back({bounds_center.x - bounds_span * 20.0f, bounds_center.y - bounds_span});
		working_points.push_back({bounds_center.x, bounds_center.y + bounds_span * 20.0f});
		working_points.push_back({bounds_center.x + bounds_span * 20.0f, bounds_center.y - bounds_span});

		vector_t<triangle_indices_t> triangles = {};
		triangles.reserve(points.size * 2 + 1);
		triangles.push_back({.indices = {super_first, super_first + 1, super_first + 2}});

		for (u32 point_index = 0; point_index < points.size; ++point_index)
		{
			vector_t<u8>	 bad_triangles(triangles.size(), 0);
			vector_t<edge_t> boundary_edges = {};
			boundary_edges.reserve(triangles.size() * 3);

			for (u32 triangle_index = 0; triangle_index < triangles.size(); ++triangle_index)
			{
				const triangle_indices_t& triangle	  = triangles[triangle_index];
				const vec2f_t&			  a			  = working_points[triangle.indices[0]];
				const vec2f_t&			  b			  = working_points[triangle.indices[1]];
				const vec2f_t&			  c			  = working_points[triangle.indices[2]];
				const double			  ax		  = static_cast<double>(a.x);
				const double			  ay		  = static_cast<double>(a.y);
				const double			  bx		  = static_cast<double>(b.x);
				const double			  by		  = static_cast<double>(b.y);
				const double			  cx		  = static_cast<double>(c.x);
				const double			  cy		  = static_cast<double>(c.y);
				const double			  denominator = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));

				if (math::abs(denominator) <= static_cast<double>(MATH_EPS))
					continue;

				const double a_length		= ax * ax + ay * ay;
				const double b_length		= bx * bx + by * by;
				const double c_length		= cx * cx + cy * cy;
				const double center_x		= (a_length * (by - cy) + b_length * (cy - ay) + c_length * (ay - by)) / denominator;
				const double center_y		= (a_length * (cx - bx) + b_length * (ax - cx) + c_length * (bx - ax)) / denominator;
				const double radius_x		= center_x - ax;
				const double radius_y		= center_y - ay;
				const double point_x		= center_x - static_cast<double>(points[point_index].x);
				const double point_y		= center_y - static_cast<double>(points[point_index].y);
				const double radius_sq		= radius_x * radius_x + radius_y * radius_y;
				const double point_distance = point_x * point_x + point_y * point_y;

				if (point_distance > radius_sq + static_cast<double>(MATH_EPS))
					continue;

				bad_triangles[triangle_index] = 1;

				const edge_t triangle_edges[] = {
					{.first = triangle.indices[0], .second = triangle.indices[1]},
					{.first = triangle.indices[1], .second = triangle.indices[2]},
					{.first = triangle.indices[2], .second = triangle.indices[0]},
				};

				for (const edge_t& edge : triangle_edges)
				{
					const auto edge_it = std::find_if(
						boundary_edges.begin(), boundary_edges.end(), [&edge](const edge_t& existing) { return (existing.first == edge.first && existing.second == edge.second) || (existing.first == edge.second && existing.second == edge.first); });

					if (edge_it == boundary_edges.end())
						boundary_edges.push_back(edge);
					else
						boundary_edges.erase(edge_it);
				}
			}

			for (u32 triangle_index = static_cast<u32>(triangles.size()); triangle_index > 0; --triangle_index)
			{
				if (bad_triangles[triangle_index - 1] != 0)
					triangles.erase(triangles.begin() + triangle_index - 1);
			}

			for (const edge_t& edge : boundary_edges)
			{
				const vec2f_t& a		  = working_points[edge.first];
				const vec2f_t& b		  = working_points[edge.second];
				const vec2f_t& point	  = working_points[point_index];
				const f32	   area_twice = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);

				if (math::abs(area_twice) <= MATH_EPS)
					continue;

				if (area_twice > 0.0f)
					triangles.push_back({.indices = {edge.first, edge.second, point_index}});
				else
					triangles.push_back({.indices = {edge.second, edge.first, point_index}});
			}
		}

		out_triangles.reserve(triangles.size());

		for (const triangle_indices_t& triangle : triangles)
		{
			if (triangle.indices[0] >= super_first || triangle.indices[1] >= super_first || triangle.indices[2] >= super_first)
				continue;

			out_triangles.push_back(triangle);
		}

		return true;
	}

	bool math::triangle_barycentric_2d(const vec2f_t& point, const vec2f_t& a, const vec2f_t& b, const vec2f_t& c, vec3f_t& out_weights)
	{
		const f32 denominator = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);

		if (math::abs(denominator) <= MATH_EPS)
			return false;

		out_weights.x = ((b.y - c.y) * (point.x - c.x) + (c.x - b.x) * (point.y - c.y)) / denominator;
		out_weights.y = ((c.y - a.y) * (point.x - c.x) + (a.x - c.x) * (point.y - c.y)) / denominator;
		out_weights.z = 1.0f - out_weights.x - out_weights.y;
		return true;
	}

	vec3f_t math::closest_triangle_barycentric_2d(const vec2f_t& point, const vec2f_t& a, const vec2f_t& b, const vec2f_t& c)
	{
		const vec2f_t vertices[]	= {a, b, c};
		vec3f_t		  best_weights	= vec3f_t::zero;
		f32			  best_distance = MATH_INF_F;

		for (u32 edge_index = 0; edge_index < 3; ++edge_index)
		{
			const u32	  next_index = (edge_index + 1) % 3;
			const vec2f_t edge		 = vertices[next_index] - vertices[edge_index];
			const f32	  length_sq	 = edge.magnitude_sqr();
			const f32	  blend		 = length_sq > MATH_EPS ? math::clamp(vec2f_t::dot(point - vertices[edge_index], edge) / length_sq, 0.0f, 1.0f) : 0.0f;
			const vec2f_t closest	 = vertices[edge_index] + edge * blend;
			const f32	  distance	 = (closest - point).magnitude_sqr();

			if (distance >= best_distance)
				continue;

			f32 edge_weights[3]		 = {};
			edge_weights[edge_index] = 1.0f - blend;
			edge_weights[next_index] = blend;

			best_distance = distance;
			best_weights  = {edge_weights[0], edge_weights[1], edge_weights[2]};
		}

		return best_weights;
	}
}
