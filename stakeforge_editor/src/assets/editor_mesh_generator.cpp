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

#include "editor_mesh_generator.hpp"

#include <sfg/data/ostream.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/mesh_cook.hpp>
#include <sfg/runtime/resources/mesh_def.hpp>
#include <sfg/runtime/resources/mesh_util.hpp>
#include <utility>

namespace sfg
{
	namespace
	{
		u16 sanitize_segments(u16 value, u16 min_value)
		{
			return value < min_value ? min_value : value;
		}

		vertex_static_t make_vertex(const vec3f_t& pos, const vec3f_t& normal, const vec2f_t& uv)
		{
			return {
				.pos	= pos,
				.normal = normal,
				.uv		= uv,
			};
		}

		void push_tri(primitive_static_def_t& primitive, u32 a, u32 b, u32 c)
		{
			primitive.indices.push_back(static_cast<primitive_index>(a));
			primitive.indices.push_back(static_cast<primitive_index>(b));
			primitive.indices.push_back(static_cast<primitive_index>(c));
		}

		void push_quad(primitive_static_def_t& primitive, const vec3f_t& normal, const vec3f_t& a, const vec3f_t& b, const vec3f_t& c, const vec3f_t& d)
		{
			const u32 base = static_cast<u32>(primitive.vertices.size());
			primitive.vertices.push_back(make_vertex(a, normal, {0.0f, 1.0f}));
			primitive.vertices.push_back(make_vertex(b, normal, {1.0f, 1.0f}));
			primitive.vertices.push_back(make_vertex(c, normal, {1.0f, 0.0f}));
			primitive.vertices.push_back(make_vertex(d, normal, {0.0f, 0.0f}));
			push_tri(primitive, base, base + 1, base + 2);
			push_tri(primitive, base, base + 2, base + 3);
		}

		void push_cylinder_side(primitive_static_def_t& primitive, f32 radius, f32 min_y, f32 max_y, u16 segments)
		{
			const u32 base = static_cast<u32>(primitive.vertices.size());
			for (u32 y = 0; y < 2; ++y)
			{
				const f32 py = y == 0 ? min_y : max_y;
				const f32 v	 = y == 0 ? 1.0f : 0.0f;
				for (u32 x = 0; x <= segments; ++x)
				{
					const f32	  u		 = static_cast<f32>(x) / static_cast<f32>(segments);
					const f32	  theta	 = u * MATH_TWO_PI;
					const vec3f_t normal = {math::cos(theta), 0.0f, math::sin(theta)};
					primitive.vertices.push_back(make_vertex({normal.x * radius, py, normal.z * radius}, normal, {u, v}));
				}
			}

			const u32 row = static_cast<u32>(segments) + 1;
			for (u32 x = 0; x < segments; ++x)
			{
				const u32 a = base + x;
				const u32 b = base + row + x;
				const u32 c = b + 1;
				const u32 d = a + 1;
				push_tri(primitive, a, b, c);
				push_tri(primitive, a, c, d);
			}
		}

		void push_disc(primitive_static_def_t& primitive, f32 radius, f32 y, const vec3f_t& normal, u16 segments)
		{
			const u32 center = static_cast<u32>(primitive.vertices.size());
			primitive.vertices.push_back(make_vertex({0.0f, y, 0.0f}, normal, {0.5f, 0.5f}));
			const u32 ring = static_cast<u32>(primitive.vertices.size());
			for (u32 x = 0; x <= segments; ++x)
			{
				const f32 u		= static_cast<f32>(x) / static_cast<f32>(segments);
				const f32 theta = u * MATH_TWO_PI;
				const f32 px	= math::cos(theta) * radius;
				const f32 pz	= math::sin(theta) * radius;
				primitive.vertices.push_back(make_vertex({px, y, pz}, normal, {px / (radius * 2.0f) + 0.5f, pz / (radius * 2.0f) + 0.5f}));
			}

			for (u32 x = 0; x < segments; ++x)
			{
				if (normal.y < 0.0f)
					push_tri(primitive, center, ring + x, ring + x + 1);
				else
					push_tri(primitive, center, ring + x + 1, ring + x);
			}
		}

		void push_annulus(primitive_static_def_t& primitive, f32 inner_radius, f32 outer_radius, f32 y, const vec3f_t& normal, u16 segments)
		{
			const u32 base = static_cast<u32>(primitive.vertices.size());
			for (u32 x = 0; x <= segments; ++x)
			{
				const f32 u		= static_cast<f32>(x) / static_cast<f32>(segments);
				const f32 theta = u * MATH_TWO_PI;
				const f32 cs	= math::cos(theta);
				const f32 sn	= math::sin(theta);
				primitive.vertices.push_back(make_vertex({cs * outer_radius, y, sn * outer_radius}, normal, {u, 0.0f}));
				primitive.vertices.push_back(make_vertex({cs * inner_radius, y, sn * inner_radius}, normal, {u, 1.0f}));
			}

			for (u32 x = 0; x < segments; ++x)
			{
				const u32 a = base + x * 2;
				const u32 b = a + 1;
				const u32 c = a + 3;
				const u32 d = a + 2;
				if (normal.y > 0.0f)
				{
					push_tri(primitive, a, b, c);
					push_tri(primitive, a, c, d);
				}
				else
				{
					push_tri(primitive, a, c, b);
					push_tri(primitive, a, d, c);
				}
			}
		}

		void push_cone_side(primitive_static_def_t& primitive, f32 radius, f32 min_y, f32 max_y, u16 segments)
		{
			const f32 normal_y = radius / (max_y - min_y);
			for (u32 x = 0; x < segments; ++x)
			{
				const f32	  u0		 = static_cast<f32>(x) / static_cast<f32>(segments);
				const f32	  u1		 = static_cast<f32>(x + 1) / static_cast<f32>(segments);
				const f32	  theta0	 = u0 * MATH_TWO_PI;
				const f32	  theta1	 = u1 * MATH_TWO_PI;
				const f32	  mid		 = (theta0 + theta1) * 0.5f;
				const vec3f_t normal0	 = vec3f_t(math::cos(theta0), normal_y, math::sin(theta0)).normalized();
				const vec3f_t normal1	 = vec3f_t(math::cos(theta1), normal_y, math::sin(theta1)).normalized();
				const vec3f_t normal_tip = vec3f_t(math::cos(mid), normal_y, math::sin(mid)).normalized();

				const u32 base = static_cast<u32>(primitive.vertices.size());
				primitive.vertices.push_back(make_vertex({math::cos(theta0) * radius, min_y, math::sin(theta0) * radius}, normal0, {u0, 1.0f}));
				primitive.vertices.push_back(make_vertex({0.0f, max_y, 0.0f}, normal_tip, {(u0 + u1) * 0.5f, 0.0f}));
				primitive.vertices.push_back(make_vertex({math::cos(theta1) * radius, min_y, math::sin(theta1) * radius}, normal1, {u1, 1.0f}));
				push_tri(primitive, base, base + 1, base + 2);
			}
		}

		bool write_mesh_def(mesh_def_t& def, primitive_static_def_t& primitive, ostream_t& out)
		{
			primitive.material_index = 0;
			mesh_util_t::generate_tangents(primitive);
			def.static_primitives.push_back(std::move(primitive));

			resource_header_t header = {};
			return mesh_cooker::cook_from_def(def, header, out, false);
		}
	}

	bool editor_mesh_generator_t::generate_cube(const editor_mesh_generator_cube_params_t& params, ostream_t& out)
	{
		const vec3f_t		   half		 = params.size * 0.5f;
		primitive_static_def_t primitive = {};

		push_quad(primitive, {0.0f, 0.0f, 1.0f}, {-half.x, -half.y, half.z}, {half.x, -half.y, half.z}, {half.x, half.y, half.z}, {-half.x, half.y, half.z});
		push_quad(primitive, {0.0f, 0.0f, -1.0f}, {half.x, -half.y, -half.z}, {-half.x, -half.y, -half.z}, {-half.x, half.y, -half.z}, {half.x, half.y, -half.z});
		push_quad(primitive, {1.0f, 0.0f, 0.0f}, {half.x, -half.y, half.z}, {half.x, -half.y, -half.z}, {half.x, half.y, -half.z}, {half.x, half.y, half.z});
		push_quad(primitive, {-1.0f, 0.0f, 0.0f}, {-half.x, -half.y, -half.z}, {-half.x, -half.y, half.z}, {-half.x, half.y, half.z}, {-half.x, half.y, -half.z});
		push_quad(primitive, {0.0f, 1.0f, 0.0f}, {-half.x, half.y, half.z}, {half.x, half.y, half.z}, {half.x, half.y, -half.z}, {-half.x, half.y, -half.z});
		push_quad(primitive, {0.0f, -1.0f, 0.0f}, {-half.x, -half.y, -half.z}, {half.x, -half.y, -half.z}, {half.x, -half.y, half.z}, {-half.x, -half.y, half.z});

		mesh_def_t def	 = {};
		def.name		 = "cube";
		def.local_bounds = aabb_t(-half, half);
		return write_mesh_def(def, primitive, out);
	}

	bool editor_mesh_generator_t::generate_plane(const editor_mesh_generator_plane_params_t& params, ostream_t& out)
	{
		const vec2f_t		   half		 = params.size * 0.5f;
		primitive_static_def_t primitive = {};

		push_quad(primitive, {0.0f, 1.0f, 0.0f}, {-half.x, 0.0f, half.y}, {half.x, 0.0f, half.y}, {half.x, 0.0f, -half.y}, {-half.x, 0.0f, -half.y});

		mesh_def_t def = {
			.local_bounds = aabb_t({-half.x, 0.0f, -half.y}, {half.x, 0.0f, half.y}),
			.name		  = "plane",
		};

		return write_mesh_def(def, primitive, out);
	}

	bool editor_mesh_generator_t::generate_sphere(const editor_mesh_generator_sphere_params_t& params, ostream_t& out)
	{
		const f32			   radius	 = math::max(params.radius, MATH_EPS);
		const u16			   segments	 = sanitize_segments(params.segments, 3);
		const u16			   rings	 = sanitize_segments(params.rings, 2);
		primitive_static_def_t primitive = {};
		primitive.vertices.reserve(static_cast<size_t>(segments + 1) * static_cast<size_t>(rings + 1));
		primitive.indices.reserve(static_cast<size_t>(segments) * static_cast<size_t>(rings) * 6);

		for (u16 y = 0; y <= rings; ++y)
		{
			const f32 v	  = static_cast<f32>(y) / static_cast<f32>(rings);
			const f32 phi = v * MATH_PI;
			const f32 sy  = math::cos(phi);
			const f32 sr  = math::sin(phi);

			for (u16 x = 0; x <= segments; ++x)
			{
				const f32	  u		 = static_cast<f32>(x) / static_cast<f32>(segments);
				const f32	  theta	 = u * MATH_TWO_PI;
				const vec3f_t normal = {
					math::cos(theta) * sr,
					sy,
					math::sin(theta) * sr,
				};
				primitive.vertices.push_back(make_vertex(normal * radius, normal, {u, v}));
			}
		}

		const u32 row = static_cast<u32>(segments) + 1;
		for (u16 y = 0; y < rings; ++y)
		{
			for (u16 x = 0; x < segments; ++x)
			{
				const u32 a = static_cast<u32>(y) * row + x;
				const u32 b = a + row;
				const u32 c = b + 1;
				const u32 d = a + 1;
				push_tri(primitive, a, b, c);
				push_tri(primitive, a, c, d);
			}
		}

		const vec3f_t half(radius, radius, radius);
		mesh_def_t	  def = {};
		def.name		  = "sphere";
		def.local_bounds  = aabb_t(-half, half);
		return write_mesh_def(def, primitive, out);
	}

	bool editor_mesh_generator_t::generate_cylinder(const editor_mesh_generator_cylinder_params_t& params, ostream_t& out)
	{
		const f32			   radius	 = math::max(params.radius, MATH_EPS);
		const f32			   half_y	 = math::max(params.height, MATH_EPS) * 0.5f;
		const u16			   segments	 = sanitize_segments(params.segments, 3);
		primitive_static_def_t primitive = {};
		primitive.vertices.reserve(static_cast<size_t>(segments + 1) * 2 + static_cast<size_t>(segments + 1) * 2 + 2);
		primitive.indices.reserve(static_cast<size_t>(segments) * 12);

		for (u16 y = 0; y < 2; ++y)
		{
			const f32 py = y == 0 ? -half_y : half_y;
			const f32 v	 = y == 0 ? 1.0f : 0.0f;
			for (u16 x = 0; x <= segments; ++x)
			{
				const f32	  u		 = static_cast<f32>(x) / static_cast<f32>(segments);
				const f32	  theta	 = u * MATH_TWO_PI;
				const vec3f_t normal = {math::cos(theta), 0.0f, math::sin(theta)};
				primitive.vertices.push_back(make_vertex({normal.x * radius, py, normal.z * radius}, normal, {u, v}));
			}
		}

		const u32 row = static_cast<u32>(segments) + 1;
		for (u16 x = 0; x < segments; ++x)
		{
			const u32 a = x;
			const u32 b = row + x;
			const u32 c = b + 1;
			const u32 d = a + 1;
			push_tri(primitive, a, b, c);
			push_tri(primitive, a, c, d);
		}

		const u32 bottom_center = static_cast<u32>(primitive.vertices.size());
		primitive.vertices.push_back(make_vertex({0.0f, -half_y, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}));
		const u32 bottom_ring = static_cast<u32>(primitive.vertices.size());
		for (u16 x = 0; x <= segments; ++x)
		{
			const f32 u		= static_cast<f32>(x) / static_cast<f32>(segments);
			const f32 theta = u * MATH_TWO_PI;
			const f32 px	= math::cos(theta) * radius;
			const f32 pz	= math::sin(theta) * radius;
			primitive.vertices.push_back(make_vertex({px, -half_y, pz}, {0.0f, -1.0f, 0.0f}, {px / (radius * 2.0f) + 0.5f, pz / (radius * 2.0f) + 0.5f}));
		}
		for (u16 x = 0; x < segments; ++x)
			push_tri(primitive, bottom_center, bottom_ring + x, bottom_ring + x + 1);

		const u32 top_center = static_cast<u32>(primitive.vertices.size());
		primitive.vertices.push_back(make_vertex({0.0f, half_y, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}));
		const u32 top_ring = static_cast<u32>(primitive.vertices.size());
		for (u16 x = 0; x <= segments; ++x)
		{
			const f32 u		= static_cast<f32>(x) / static_cast<f32>(segments);
			const f32 theta = u * MATH_TWO_PI;
			const f32 px	= math::cos(theta) * radius;
			const f32 pz	= math::sin(theta) * radius;
			primitive.vertices.push_back(make_vertex({px, half_y, pz}, {0.0f, 1.0f, 0.0f}, {px / (radius * 2.0f) + 0.5f, pz / (radius * 2.0f) + 0.5f}));
		}
		for (u16 x = 0; x < segments; ++x)
			push_tri(primitive, top_center, top_ring + x + 1, top_ring + x);

		mesh_def_t def	 = {};
		def.name		 = "cylinder";
		def.local_bounds = aabb_t({-radius, -half_y, -radius}, {radius, half_y, radius});
		return write_mesh_def(def, primitive, out);
	}

	bool editor_mesh_generator_t::generate_capsule(const editor_mesh_generator_capsule_params_t& params, ostream_t& out)
	{
		const f32			   radius			= math::max(params.radius, MATH_EPS);
		const f32			   half_body		= math::max(0.0f, params.height * 0.5f - radius);
		const u16			   segments			= sanitize_segments(params.segments, 3);
		const u16			   hemisphere_rings = sanitize_segments(params.hemisphere_rings, 2);
		const u16			   rings			= static_cast<u16>(hemisphere_rings * 2);
		primitive_static_def_t primitive		= {};
		primitive.vertices.reserve(static_cast<size_t>(segments + 1) * static_cast<size_t>(rings + 1));
		primitive.indices.reserve(static_cast<size_t>(segments) * static_cast<size_t>(rings) * 6);

		for (u16 y = 0; y <= rings; ++y)
		{
			const f32 v		= static_cast<f32>(y) / static_cast<f32>(rings);
			const f32 angle = -MATH_HALF_PI + v * MATH_PI;
			const f32 sy	= math::sin(angle);
			const f32 sr	= math::cos(angle);
			const f32 cy	= sy < 0.0f ? -half_body : half_body;

			for (u16 x = 0; x <= segments; ++x)
			{
				const f32	  u		 = static_cast<f32>(x) / static_cast<f32>(segments);
				const f32	  theta	 = u * MATH_TWO_PI;
				const vec3f_t normal = {
					math::cos(theta) * sr,
					sy,
					math::sin(theta) * sr,
				};
				primitive.vertices.push_back(make_vertex({normal.x * radius, cy + normal.y * radius, normal.z * radius}, normal, {u, 1.0f - v}));
			}
		}

		const u32 row = static_cast<u32>(segments) + 1;
		for (u16 y = 0; y < rings; ++y)
		{
			for (u16 x = 0; x < segments; ++x)
			{
				const u32 a = static_cast<u32>(y) * row + x;
				const u32 b = a + row;
				const u32 c = b + 1;
				const u32 d = a + 1;
				push_tri(primitive, a, b, c);
				push_tri(primitive, a, c, d);
			}
		}

		mesh_def_t def	 = {};
		def.name		 = "capsule";
		def.local_bounds = aabb_t({-radius, -half_body - radius, -radius}, {radius, half_body + radius, radius});
		return write_mesh_def(def, primitive, out);
	}

	bool editor_mesh_generator_t::generate_translation_gizmo(const editor_mesh_generator_translation_gizmo_params_t& params, ostream_t& out)
	{
		const f32			   shaft_radius = math::max(params.shaft_radius, MATH_EPS);
		const f32			   arrow_radius = math::max(params.arrow_radius, shaft_radius);
		const f32			   arrow_length = math::clamp(params.arrow_length, MATH_EPS, 1.0f - MATH_EPS);
		const f32			   shaft_end	= 1.0f - arrow_length;
		const u16			   segments		= sanitize_segments(params.segments, 3);
		primitive_static_def_t primitive	= {};
		primitive.vertices.reserve(static_cast<size_t>(segments) * 7 + 6);
		primitive.indices.reserve(static_cast<size_t>(segments) * 18);

		push_cylinder_side(primitive, shaft_radius, 0.0f, shaft_end, segments);
		push_disc(primitive, shaft_radius, 0.0f, {0.0f, -1.0f, 0.0f}, segments);
		if (arrow_radius > shaft_radius)
			push_annulus(primitive, shaft_radius, arrow_radius, shaft_end, {0.0f, -1.0f, 0.0f}, segments);
		push_cone_side(primitive, arrow_radius, shaft_end, 1.0f, segments);

		mesh_def_t def	 = {};
		def.name		 = "translation_gizmo";
		def.local_bounds = aabb_t({-arrow_radius, 0.0f, -arrow_radius}, {arrow_radius, 1.0f, arrow_radius});
		return write_mesh_def(def, primitive, out);
	}

	bool editor_mesh_generator_t::generate_scale_gizmo(const editor_mesh_generator_scale_gizmo_params_t& params, ostream_t& out)
	{
		const f32			   shaft_radius = math::max(params.shaft_radius, MATH_EPS);
		const f32			   cube_size	= math::clamp(params.cube_size, MATH_EPS, 1.0f - MATH_EPS);
		const f32			   cube_half	= cube_size * 0.5f;
		const f32			   cube_min_y	= 1.0f - cube_size;
		const u16			   segments		= sanitize_segments(params.segments, 3);
		primitive_static_def_t primitive	= {};
		primitive.vertices.reserve(static_cast<size_t>(segments + 1) * 3 + 25);
		primitive.indices.reserve(static_cast<size_t>(segments) * 9 + 36);

		push_cylinder_side(primitive, shaft_radius, 0.0f, cube_min_y, segments);
		push_disc(primitive, shaft_radius, 0.0f, {0.0f, -1.0f, 0.0f}, segments);

		const vec3f_t min{-cube_half, cube_min_y, -cube_half};
		const vec3f_t max{cube_half, 1.0f, cube_half};
		push_quad(primitive, {0.0f, 0.0f, 1.0f}, {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z});
		push_quad(primitive, {0.0f, 0.0f, -1.0f}, {max.x, min.y, min.z}, {min.x, min.y, min.z}, {min.x, max.y, min.z}, {max.x, max.y, min.z});
		push_quad(primitive, {1.0f, 0.0f, 0.0f}, {max.x, min.y, max.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {max.x, max.y, max.z});
		push_quad(primitive, {-1.0f, 0.0f, 0.0f}, {min.x, min.y, min.z}, {min.x, min.y, max.z}, {min.x, max.y, max.z}, {min.x, max.y, min.z});
		push_quad(primitive, {0.0f, 1.0f, 0.0f}, {min.x, max.y, max.z}, {max.x, max.y, max.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z});
		push_quad(primitive, {0.0f, -1.0f, 0.0f}, {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, min.y, max.z}, {min.x, min.y, max.z});

		const f32  bounds_radius = math::max(shaft_radius, cube_half);
		mesh_def_t def			 = {};
		def.name				 = "scale_gizmo";
		def.local_bounds		 = aabb_t({-bounds_radius, 0.0f, -bounds_radius}, {bounds_radius, 1.0f, bounds_radius});
		return write_mesh_def(def, primitive, out);
	}

	bool editor_mesh_generator_t::generate_rotation_gizmo(const editor_mesh_generator_rotation_gizmo_params_t& params, ostream_t& out)
	{
		const f32			   radius	  = math::max(params.radius, MATH_EPS * 2.0f);
		const f32			   thickness  = math::clamp(params.thickness, MATH_EPS, radius - MATH_EPS);
		const f32			   half_depth = thickness * 0.5f;
		const f32			   inner	  = radius - thickness;
		const u16			   segments	  = sanitize_segments(params.segments, 3);
		primitive_static_def_t primitive  = {};
		primitive.vertices.reserve(static_cast<size_t>(segments + 1) * 8);
		primitive.indices.reserve(static_cast<size_t>(segments) * 24);

		push_annulus(primitive, inner, radius, half_depth, {0.0f, 1.0f, 0.0f}, segments);
		push_annulus(primitive, inner, radius, -half_depth, {0.0f, -1.0f, 0.0f}, segments);
		push_cylinder_side(primitive, radius, -half_depth, half_depth, segments);

		const u32 inner_base = static_cast<u32>(primitive.vertices.size());
		for (u32 y = 0; y < 2; ++y)
		{
			const f32 py = y == 0 ? -half_depth : half_depth;
			const f32 v	 = y == 0 ? 1.0f : 0.0f;
			for (u32 x = 0; x <= segments; ++x)
			{
				const f32 u		= static_cast<f32>(x) / static_cast<f32>(segments);
				const f32 theta = u * MATH_TWO_PI;
				const f32 cs	= math::cos(theta);
				const f32 sn	= math::sin(theta);
				primitive.vertices.push_back(make_vertex({cs * inner, py, sn * inner}, {-cs, 0.0f, -sn}, {u, v}));
			}
		}

		const u32 row = static_cast<u32>(segments) + 1;
		for (u32 x = 0; x < segments; ++x)
		{
			const u32 a = inner_base + x;
			const u32 b = inner_base + row + x;
			const u32 c = b + 1;
			const u32 d = a + 1;
			push_tri(primitive, a, c, b);
			push_tri(primitive, a, d, c);
		}

		mesh_def_t def	 = {};
		def.name		 = "rotation_gizmo";
		def.local_bounds = aabb_t({-radius, -half_depth, -radius}, {radius, half_depth, radius});
		return write_mesh_def(def, primitive, out);
	}
}
