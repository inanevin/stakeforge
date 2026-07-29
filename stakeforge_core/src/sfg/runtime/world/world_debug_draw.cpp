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

#include "world_debug_draw.hpp"
#include "ecs.hpp"
#include "ecs_helpers.hpp"
#include "engine_components.hpp"
#include "system_components.hpp"
#include "world.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/texture.hpp>
#include <sfg/runtime/render/world_debug_draw_snapshot.hpp>
#include <sfg/runtime/render/world_draw.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
#define DEBUG_DRAW_MISSING_RESOURCE_TEXT_SIZE_PX		 32.0f
#define DEBUG_DRAW_MISSING_RESOURCE_TEXT_LINE_SPACING_PX 36.0f
#define WORLD_DEBUG_DRAW_FONT							 TO_SIDC("engine/resource_pack/IBMPlexMono-Regular.ttf")

	world_debug_draw_t::world_debug_draw_t()  = default;
	world_debug_draw_t::~world_debug_draw_t() = default;

	void world_debug_draw_t::init(const world_debug_draw_config_t& config)
	{
		SFG_ASSERT((config.line_vertex_max_count == 0) == (config.line_index_max_count == 0));
		SFG_ASSERT((config.triangle_vertex_max_count == 0) == (config.triangle_index_max_count == 0));
		SFG_ASSERT((config.text_vertex_max_count == 0) == (config.text_index_max_count == 0));
		SFG_ASSERT((config.text_vertex_max_count == 0) == (config.text_command_max_count == 0));
		SFG_ASSERT((config.text_vertex_max_count == 0) == (config.text_budget_bytes == 0));

		_config = config;
		_vertices.reserve(config.line_vertex_max_count);
		_indices.reserve(config.line_index_max_count);
		_triangle_vertices.reserve(config.triangle_vertex_max_count);
		_triangle_indices.reserve(config.triangle_index_max_count);
		_text_commands.reserve(config.text_command_max_count);
		_texture_commands.reserve(config.texture_max_count);
		_text_bytes.reserve(config.text_budget_bytes);

		if (config.text_vertex_max_count > 0)
		{
			_text_canvas = make_unique<ui::vg_canvas_t>();
			_text_canvas->init({
				.vertex_pool_budget_bytes		= config.text_vertex_max_count * sizeof(ui::vg_vertex_t),
				.index_pool_budget_bytes		= config.text_index_max_count * sizeof(ui::vg_index_t),
				.buffer_count					= 1,
				.geometry_span_count			= 1,
				.text_cache_vertex_budget_bytes = config.text_vertex_max_count * sizeof(ui::vg_vertex_t),
				.text_cache_index_budget_bytes	= config.text_index_max_count * sizeof(ui::vg_index_t),
				.clip_stack_initial_capacity	= 1,
			});
		}

		begin_frame();
	}

	void world_debug_draw_t::uninit()
	{
		_vertices.resize(0);
		_vertices.shrink_to_fit();
		_indices.resize(0);
		_indices.shrink_to_fit();
		_triangle_vertices.resize(0);
		_triangle_vertices.shrink_to_fit();
		_triangle_indices.resize(0);
		_triangle_indices.shrink_to_fit();
		_text_commands.resize(0);
		_text_commands.shrink_to_fit();
		_texture_commands.resize(0);
		_texture_commands.shrink_to_fit();
		_text_bytes.resize(0);
		_text_bytes.shrink_to_fit();

		if (_text_canvas)
		{
			_text_canvas->uninit();
			_text_canvas.reset();
		}

		_config = {};

		_dropped_line_count		= 0;
		_dropped_triangle_count = 0;
		_dropped_text_count		= 0;
		_dropped_texture_count	= 0;
	}

	void world_debug_draw_t::begin_frame()
	{
		_vertices.resize(0);
		_indices.resize(0);
		_triangle_vertices.resize(0);
		_triangle_indices.resize(0);
		_text_commands.resize(0);
		_texture_commands.resize(0);
		_text_bytes.resize(0);

		_dropped_line_count		= 0;
		_dropped_triangle_count = 0;
		_dropped_text_count		= 0;
		_dropped_texture_count	= 0;
	}

	void world_debug_draw_t::draw_line(const vec3f_t& from, const vec3f_t& to, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		if (from.equals(to) || thickness_px <= 0.0f)
			return;

		if (_vertices.size() + 4 > _config.line_vertex_max_count || _indices.size() + 6 > _config.line_index_max_count)
		{
			++_dropped_line_count;
			return;
		}

		const f32			  signed_thickness_px = depth == debug_draw_depth_e::depth_tested ? thickness_px : -thickness_px;
		const vec4f_t		  line_color		  = color.to_vector();
		const primitive_index base_vertex		  = static_cast<primitive_index>(_vertices.size());

		_vertices.push_back({.color = line_color, .position = from, .other_position = to, .corner = 0.0f, .signed_thickness_px = signed_thickness_px});
		_vertices.push_back({.color = line_color, .position = from, .other_position = to, .corner = 1.0f, .signed_thickness_px = signed_thickness_px});
		_vertices.push_back({.color = line_color, .position = to, .other_position = from, .corner = 2.0f, .signed_thickness_px = signed_thickness_px});
		_vertices.push_back({.color = line_color, .position = to, .other_position = from, .corner = 3.0f, .signed_thickness_px = signed_thickness_px});
		_indices.push_back(base_vertex);
		_indices.push_back(base_vertex + 1);
		_indices.push_back(base_vertex + 2);
		_indices.push_back(base_vertex + 2);
		_indices.push_back(base_vertex + 1);
		_indices.push_back(base_vertex + 3);
	}

	void world_debug_draw_t::draw_arrow(const vec3f_t& from, const vec3f_t& to, const color_t& color, f32 head_length, f32 head_radius, f32 thickness_px, debug_draw_depth_e depth)
	{
		SFG_ASSERT(head_length > 0.0f && head_radius > 0.0f);

		const vec3f_t delta	 = to - from;
		const f32	  length = delta.magnitude();

		if (length <= MATH_EPS)
			return;

		const vec3f_t direction			 = delta / length;
		const f32	  actual_head_length = math::min(head_length, length * 0.5f);
		const f32	  actual_head_radius = math::min(head_radius, length * 0.25f);
		const vec3f_t reference			 = math::abs(vec3f_t::dot(direction, vec3f_t::up)) > 0.99f ? vec3f_t::right : vec3f_t::up;
		const vec3f_t axis0				 = vec3f_t::cross(reference, direction).normalized();
		const vec3f_t axis1				 = vec3f_t::cross(direction, axis0).normalized();
		const vec3f_t head_base			 = to - direction * actual_head_length;

		draw_line(from, head_base, color, thickness_px, depth);
		draw_circle(head_base, actual_head_radius, direction, color, thickness_px, depth, 8);
		draw_line(to, head_base + axis0 * actual_head_radius, color, thickness_px, depth);
		draw_line(to, head_base - axis0 * actual_head_radius, color, thickness_px, depth);
		draw_line(to, head_base + axis1 * actual_head_radius, color, thickness_px, depth);
		draw_line(to, head_base - axis1 * actual_head_radius, color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_triangle(const vec3f_t& p0, const vec3f_t& p1, const vec3f_t& p2, const color_t& color)
	{
		if (_triangle_vertices.size() + 3 > _config.triangle_vertex_max_count || _triangle_indices.size() + 3 > _config.triangle_index_max_count)
		{
			++_dropped_triangle_count;
			return;
		}

		const vec4f_t		  triangle_color = color.to_vector();
		const primitive_index base_vertex	 = static_cast<primitive_index>(_triangle_vertices.size());
		_triangle_vertices.push_back({.position = p0, .color = triangle_color});
		_triangle_vertices.push_back({.position = p1, .color = triangle_color});
		_triangle_vertices.push_back({.position = p2, .color = triangle_color});
		_triangle_indices.push_back(base_vertex);
		_triangle_indices.push_back(base_vertex + 1);
		_triangle_indices.push_back(base_vertex + 2);
	}

	void world_debug_draw_t::draw_polyline(span_t<const vec3f_t> points, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, bool closed)
	{
		if (points.size < 2)
			return;

		for (size_t i = 1; i < points.size; ++i)
			draw_line(points.data[i - 1], points.data[i], color, thickness_px, depth);
		if (closed)
			draw_line(points.data[points.size - 1], points.data[0], color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_aabb(const aabb_t& bounds, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		const vec3f_t corners[8] = {
			{bounds.bounds_min.x, bounds.bounds_min.y, bounds.bounds_min.z},
			{bounds.bounds_max.x, bounds.bounds_min.y, bounds.bounds_min.z},
			{bounds.bounds_max.x, bounds.bounds_min.y, bounds.bounds_max.z},
			{bounds.bounds_min.x, bounds.bounds_min.y, bounds.bounds_max.z},
			{bounds.bounds_min.x, bounds.bounds_max.y, bounds.bounds_min.z},
			{bounds.bounds_max.x, bounds.bounds_max.y, bounds.bounds_min.z},
			{bounds.bounds_max.x, bounds.bounds_max.y, bounds.bounds_max.z},
			{bounds.bounds_min.x, bounds.bounds_max.y, bounds.bounds_max.z},
		};

		const u8 edges[12][2] = {
			{0, 1},
			{1, 2},
			{2, 3},
			{3, 0},
			{4, 5},
			{5, 6},
			{6, 7},
			{7, 4},
			{0, 4},
			{1, 5},
			{2, 6},
			{3, 7},
		};
		for (const auto& edge : edges)
			draw_line(corners[edge[0]], corners[edge[1]], color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_box(const mat4x3_t& transform, const vec3f_t& half_extents, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		const vec3f_t local_corners[8] = {
			{-half_extents.x, -half_extents.y, -half_extents.z},
			{half_extents.x, -half_extents.y, -half_extents.z},
			{half_extents.x, -half_extents.y, half_extents.z},
			{-half_extents.x, -half_extents.y, half_extents.z},
			{-half_extents.x, half_extents.y, -half_extents.z},
			{half_extents.x, half_extents.y, -half_extents.z},
			{half_extents.x, half_extents.y, half_extents.z},
			{-half_extents.x, half_extents.y, half_extents.z},
		};

		vec3f_t corners[8];
		for (u32 i = 0; i < 8; ++i)
			corners[i] = transform * local_corners[i];

		const u8 edges[12][2] = {
			{0, 1},
			{1, 2},
			{2, 3},
			{3, 0},
			{4, 5},
			{5, 6},
			{6, 7},
			{7, 4},
			{0, 4},
			{1, 5},
			{2, 6},
			{3, 7},
		};

		for (const auto& edge : edges)
			draw_line(corners[edge[0]], corners[edge[1]], color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_rectangle(const vec3f_t& center, const vec3f_t& right, const vec3f_t& up, const vec2f_t& size, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		SFG_ASSERT(!right.is_zero() && !up.is_zero() && size.x > 0.0f && size.y > 0.0f);
		const vec3f_t half_right = right.normalized() * size.x * 0.5f;
		const vec3f_t half_up	 = up.normalized() * size.y * 0.5f;
		const vec3f_t corners[4] = {
			center - half_right - half_up,
			center + half_right - half_up,
			center + half_right + half_up,
			center - half_right + half_up,
		};

		draw_polyline({.data = corners, .size = std::size(corners)}, color, thickness_px, depth, true);
	}

	void world_debug_draw_t::draw_arc(const vec3f_t& center, const vec3f_t& normal, const vec3f_t& start_direction, f32 radius, f32 angle_radians, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		if (normal.is_zero() || start_direction.is_zero())
			return;

		SFG_ASSERT(radius > 0.0f && segments > 0);

		if (math::abs(angle_radians) <= MATH_EPS)
			return;

		const vec3f_t arc_normal	  = normal.normalized();
		const vec3f_t projected_start = start_direction - arc_normal * vec3f_t::dot(start_direction, arc_normal);
		if (projected_start.is_zero())
			return;

		SFG_ASSERT(!projected_start.is_zero());

		const vec3f_t axis0	   = projected_start.normalized();
		const vec3f_t axis1	   = vec3f_t::cross(arc_normal, axis0).normalized();
		const f32	  step	   = angle_radians / static_cast<f32>(segments);
		vec3f_t		  previous = center + axis0 * radius;

		for (u32 i = 1; i <= segments; ++i)
		{
			const f32	  angle = step * static_cast<f32>(i);
			const vec3f_t point = center + (axis0 * math::cos(angle) + axis1 * math::sin(angle)) * radius;

			draw_line(previous, point, color, thickness_px, depth);
			previous = point;
		}
	}

	void world_debug_draw_t::draw_circle(const vec3f_t& center, f32 radius, const vec3f_t& normal, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		if (normal.is_zero())
			return;

		SFG_ASSERT(radius > 0.0f && segments >= 3);
		const vec3f_t direction = normal.normalized();
		const vec3f_t reference = math::abs(vec3f_t::dot(direction, vec3f_t::up)) > 0.99f ? vec3f_t::right : vec3f_t::up;
		const vec3f_t axis0		= vec3f_t::cross(reference, direction).normalized();
		const vec3f_t axis1		= vec3f_t::cross(direction, axis0).normalized();
		const f32	  step		= MATH_PI * 2.0f / static_cast<f32>(segments);
		vec3f_t		  previous	= center + axis0 * radius;

		for (u32 i = 1; i <= segments; ++i)
		{
			const f32	  angle = step * static_cast<f32>(i);
			const vec3f_t point = center + (axis0 * math::cos(angle) + axis1 * math::sin(angle)) * radius;
			draw_line(previous, point, color, thickness_px, depth);
			previous = point;
		}
	}

	void world_debug_draw_t::draw_sphere(const vec3f_t& center, f32 radius, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		draw_circle(center, radius, vec3f_t::right, color, thickness_px, depth, segments);
		draw_circle(center, radius, vec3f_t::up, color, thickness_px, depth, segments);
		draw_circle(center, radius, vec3f_t::forward, color, thickness_px, depth, segments);
	}

	void world_debug_draw_t::draw_capsule(const vec3f_t& center, f32 radius, f32 half_height, const vec3f_t& direction, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		if (direction.is_zero())
			return;

		SFG_ASSERT(radius > 0.0f && half_height >= 0.0f && segments >= 4);
		const vec3f_t normal	= direction.normalized();
		const vec3f_t reference = math::abs(vec3f_t::dot(normal, vec3f_t::up)) > 0.99f ? vec3f_t::right : vec3f_t::up;
		const vec3f_t axis0		= vec3f_t::cross(reference, normal).normalized();
		const vec3f_t axis1		= vec3f_t::cross(normal, axis0).normalized();
		const vec3f_t top		= center + normal * half_height;
		const vec3f_t bottom	= center - normal * half_height;

		draw_circle(top, radius, normal, color, thickness_px, depth, segments);
		draw_circle(bottom, radius, normal, color, thickness_px, depth, segments);
		draw_line(top + axis0 * radius, bottom + axis0 * radius, color, thickness_px, depth);
		draw_line(top - axis0 * radius, bottom - axis0 * radius, color, thickness_px, depth);
		draw_line(top + axis1 * radius, bottom + axis1 * radius, color, thickness_px, depth);
		draw_line(top - axis1 * radius, bottom - axis1 * radius, color, thickness_px, depth);

		const u32	  arc_segments = math::max(segments / 2, 2u);
		const f32	  step		   = MATH_PI / static_cast<f32>(arc_segments);
		const vec3f_t axes[2]	   = {axis0, axis1};

		for (const vec3f_t& axis : axes)
		{
			vec3f_t previous_top	= top + axis * radius;
			vec3f_t previous_bottom = bottom + axis * radius;
			for (u32 i = 1; i <= arc_segments; ++i)
			{
				const f32	  angle		   = step * static_cast<f32>(i);
				const vec3f_t lateral	   = axis * math::cos(angle);
				const vec3f_t vertical	   = normal * math::sin(angle);
				const vec3f_t point_top	   = top + (lateral + vertical) * radius;
				const vec3f_t point_bottom = bottom + (lateral - vertical) * radius;
				draw_line(previous_top, point_top, color, thickness_px, depth);
				draw_line(previous_bottom, point_bottom, color, thickness_px, depth);
				previous_top	= point_top;
				previous_bottom = point_bottom;
			}
		}
	}

	void world_debug_draw_t::draw_cylinder(const vec3f_t& center, f32 radius, f32 half_height, const vec3f_t& direction, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		if (direction.is_zero())
			return;

		SFG_ASSERT(radius > 0.0f && half_height > 0.0f && segments >= 3);
		const vec3f_t normal	= direction.normalized();
		const vec3f_t reference = math::abs(vec3f_t::dot(normal, vec3f_t::up)) > 0.99f ? vec3f_t::right : vec3f_t::up;
		const vec3f_t axis0		= vec3f_t::cross(reference, normal).normalized();
		const vec3f_t axis1		= vec3f_t::cross(normal, axis0).normalized();
		const vec3f_t top		= center + normal * half_height;
		const vec3f_t bottom	= center - normal * half_height;

		draw_circle(top, radius, normal, color, thickness_px, depth, segments);
		draw_circle(bottom, radius, normal, color, thickness_px, depth, segments);
		draw_line(top + axis0 * radius, bottom + axis0 * radius, color, thickness_px, depth);
		draw_line(top - axis0 * radius, bottom - axis0 * radius, color, thickness_px, depth);
		draw_line(top + axis1 * radius, bottom + axis1 * radius, color, thickness_px, depth);
		draw_line(top - axis1 * radius, bottom - axis1 * radius, color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_cone(const vec3f_t& origin, const vec3f_t& direction, f32 length, f32 half_angle_radians, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		if (direction.is_zero())
			return;

		SFG_ASSERT(length > 0.0f && half_angle_radians >= 0.0f && half_angle_radians <= MATH_PI && segments >= 3);

		const vec3f_t forward	  = direction.normalized();
		const vec3f_t reference	  = math::abs(vec3f_t::dot(forward, vec3f_t::up)) > 0.99f ? vec3f_t::right : vec3f_t::up;
		const vec3f_t axis0		  = vec3f_t::cross(reference, forward).normalized();
		const vec3f_t axis1		  = vec3f_t::cross(forward, axis0).normalized();
		const f32	  axial		  = math::cos(half_angle_radians) * length;
		const f32	  radial	  = math::sin(half_angle_radians) * length;
		const vec3f_t ring_center = origin + forward * axial;

		if (math::abs(radial) <= MATH_EPS)
		{
			draw_line(origin, ring_center, color, thickness_px, depth);
			return;
		}

		const f32 step	   = MATH_PI * 2.0f / static_cast<f32>(segments);
		vec3f_t	  previous = ring_center + axis0 * radial;

		for (u32 i = 1; i <= segments; ++i)
		{
			const f32	  angle = step * static_cast<f32>(i);
			const vec3f_t point = ring_center + (axis0 * math::cos(angle) + axis1 * math::sin(angle)) * radial;

			draw_line(previous, point, color, thickness_px, depth);
			previous = point;
		}

		draw_line(origin, ring_center + axis0 * radial, color, thickness_px, depth);
		draw_line(origin, ring_center - axis0 * radial, color, thickness_px, depth);
		draw_line(origin, ring_center + axis1 * radial, color, thickness_px, depth);
		draw_line(origin, ring_center - axis1 * radial, color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_cone(const vec3f_t& origin, const vec3f_t& direction, const vec3f_t& up, f32 length, const vec2f_t& half_angles_radians, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		if (direction.is_zero() || up.is_zero())
			return;

		SFG_ASSERT(length > 0.0f && half_angles_radians.x >= 0.0f && half_angles_radians.y >= 0.0f && segments >= 4);

		const vec3f_t forward = direction.normalized();
		const vec3f_t right	  = vec3f_t::cross(up, forward).normalized();
		SFG_ASSERT(!right.is_zero());

		const vec3f_t cone_up	= vec3f_t::cross(forward, right).normalized();
		const f32	  angle_x	= math::min(half_angles_radians.x, MATH_PI * 0.499f);
		const f32	  angle_y	= math::min(half_angles_radians.y, MATH_PI * 0.499f);
		const f32	  tangent_x = math::tan(angle_x);
		const f32	  tangent_y = math::tan(angle_y);

		if (tangent_x <= MATH_EPS && tangent_y <= MATH_EPS)
		{
			draw_line(origin, origin + forward * length, color, thickness_px, depth);
			return;
		}

		const f32 step	   = MATH_PI * 2.0f / static_cast<f32>(segments);
		vec3f_t	  previous = (forward + right * tangent_x).normalized() * length + origin;

		for (u32 i = 1; i <= segments; ++i)
		{
			const f32	  angle = step * static_cast<f32>(i);
			const vec3f_t ray	= (forward + right * (tangent_x * math::cos(angle)) + cone_up * (tangent_y * math::sin(angle))).normalized();
			const vec3f_t point = origin + ray * length;

			draw_line(previous, point, color, thickness_px, depth);
			previous = point;
		}

		draw_line(origin, origin + (forward + right * tangent_x).normalized() * length, color, thickness_px, depth);
		draw_line(origin, origin + (forward - right * tangent_x).normalized() * length, color, thickness_px, depth);
		draw_line(origin, origin + (forward + cone_up * tangent_y).normalized() * length, color, thickness_px, depth);
		draw_line(origin, origin + (forward - cone_up * tangent_y).normalized() * length, color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_frustum(const vec3f_t& origin, const vec3f_t& direction, f32 fov_degrees, f32 aspect_ratio, f32 near_distance, f32 far_distance, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		const vec3f_t forward	= direction.normalized();
		const vec3f_t reference = math::abs(vec3f_t::dot(forward, vec3f_t::up)) > 0.99f ? vec3f_t::right : vec3f_t::up;
		draw_frustum(origin, direction, reference, fov_degrees, aspect_ratio, near_distance, far_distance, color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_frustum(const vec3f_t& origin, const vec3f_t& direction, const vec3f_t& up, f32 fov_degrees, f32 aspect_ratio, f32 near_distance, f32 far_distance, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		if (direction.is_zero() || up.is_zero())
			return;

		SFG_ASSERT(fov_degrees > 0.0f && aspect_ratio > 0.0f && near_distance >= 0.0f && far_distance > near_distance);
		const vec3f_t forward	 = direction.normalized();
		const vec3f_t right		 = vec3f_t::cross(up, forward).normalized();
		const vec3f_t frustum_up = vec3f_t::cross(forward, right).normalized();
		SFG_ASSERT(!right.is_zero());

		const f32	  tangent	  = math::tan(fov_degrees * DEG_2_RAD * 0.5f);
		const f32	  near_height = tangent * near_distance;
		const f32	  near_width  = near_height * aspect_ratio;
		const f32	  far_height  = tangent * far_distance;
		const f32	  far_width	  = far_height * aspect_ratio;
		const vec3f_t near_center = origin + forward * near_distance;
		const vec3f_t far_center  = origin + forward * far_distance;

		const vec3f_t corners[8] = {
			near_center - right * near_width - frustum_up * near_height,
			near_center + right * near_width - frustum_up * near_height,
			near_center + right * near_width + frustum_up * near_height,
			near_center - right * near_width + frustum_up * near_height,
			far_center - right * far_width - frustum_up * far_height,
			far_center + right * far_width - frustum_up * far_height,
			far_center + right * far_width + frustum_up * far_height,
			far_center - right * far_width + frustum_up * far_height,
		};

		const u8 edges[12][2] = {
			{0, 1},
			{1, 2},
			{2, 3},
			{3, 0},
			{4, 5},
			{5, 6},
			{6, 7},
			{7, 4},
			{0, 4},
			{1, 5},
			{2, 6},
			{3, 7},
		};

		for (const auto& edge : edges)
			draw_line(corners[edge[0]], corners[edge[1]], color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_text_2d(const vec2f_t& position, const char* text, const color_t& color, f32 size_px, debug_draw_text_alignment_e alignment, resource_handle_t font)
	{
		SFG_ASSERT(text != nullptr && size_px > 0.0f);
		const u32 text_length = static_cast<u32>(std::strlen(text));
		if (text_length == 0)
			return;

		if (_config.text_vertex_max_count == 0 || _text_commands.size() >= _config.text_command_max_count || _text_bytes.size() + text_length > _config.text_budget_bytes)
		{
			++_dropped_text_count;
			return;
		}

		const u32 text_offset = static_cast<u32>(_text_bytes.size());
		_text_bytes.resize(_text_bytes.size() + text_length);
		SFG_MEMCPY(_text_bytes.data() + text_offset, text, text_length);

		_text_commands.push_back({
			.font		 = font,
			.color		 = color.to_vector(),
			.anchor		 = vec3f_t(position.x, position.y, 0.0f),
			.size_px	 = size_px,
			.text_offset = text_offset,
			.text_length = text_length,
			.alignment	 = alignment,
			.is_screen	 = true,
		});
	}

	void world_debug_draw_t::draw_text_3d(const vec3f_t& position, const char* text, const color_t& color, f32 size_px, debug_draw_depth_e depth, debug_draw_text_alignment_e alignment, const vec2f_t& screen_offset, resource_handle_t font)
	{
		SFG_ASSERT(text != nullptr && size_px > 0.0f);
		const u32 text_length = static_cast<u32>(std::strlen(text));
		if (text_length == 0)
			return;

		if (_config.text_vertex_max_count == 0 || _text_commands.size() >= _config.text_command_max_count || _text_bytes.size() + text_length > _config.text_budget_bytes)
		{
			++_dropped_text_count;
			return;
		}

		const u32 text_offset = static_cast<u32>(_text_bytes.size());
		_text_bytes.resize(_text_bytes.size() + text_length);
		SFG_MEMCPY(_text_bytes.data() + text_offset, text, text_length);

		_text_commands.push_back({
			.font		   = font,
			.color		   = color.to_vector(),
			.anchor		   = position,
			.screen_offset = screen_offset,
			.size_px	   = size_px,
			.text_offset   = text_offset,
			.text_length   = text_length,
			.alignment	   = alignment,
			.depth		   = depth,
		});
	}

	void world_debug_draw_t::draw_texture_3d(const vec3f_t& position, resource_handle_t texture, const vec2f_t& size_px, const color_t& color, entity_id_t entity_id, debug_draw_depth_e depth, const vec2f_t& screen_offset, bool is_linear_sample)
	{
		SFG_ASSERT(texture != NULL_RESOURCE_HANDLE && size_px.x > 0.0f && size_px.y > 0.0f);

		if (_texture_commands.size() >= _config.texture_max_count)
		{
			++_dropped_texture_count;
			return;
		}

		_texture_commands.push_back({
			.color			  = color.to_vector(),
			.position		  = position,
			.texture		  = texture,
			.size_px		  = size_px,
			.screen_offset	  = screen_offset,
			.entity_id		  = entity_id,
			.depth			  = depth,
			.is_linear_sample = is_linear_sample,
		});
	}

	void world_debug_draw_t::debug_draw_missing_resources(const world_t& world)
	{
		const resource_manager_t&	 resource_manager			 = resource_manager_t::get();
		const ecs_component_table_t& alive_table				 = world.get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t& transform_table			 = world.get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_t& disabled_table				 = world.get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& mesh_renderer_table		 = world.get_component_table(type_id_t<component_mesh_renderer_t>::value);
		const ecs_component_table_t& skinned_mesh_renderer_table = world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value);
		const ecs_component_table_t& sprite_renderer_table		 = world.get_component_table(type_id_t<component_sprite_renderer_t>::value);
		const ecs_component_table_t& particle_emitter_table		 = world.get_component_table(type_id_t<component_particle_emitter_t>::value);

		const auto draw_missing_resource_texts = [&](const vec3f_t& position, bool missing_mesh, bool missing_sprite, bool missing_material, bool missing_skeleton) {
			const u32 missing_count = static_cast<u32>(missing_mesh) + static_cast<u32>(missing_sprite) + static_cast<u32>(missing_material) + static_cast<u32>(missing_skeleton);
			f32		  text_y		= -(static_cast<f32>(missing_count) - 1.0f) * DEBUG_DRAW_MISSING_RESOURCE_TEXT_LINE_SPACING_PX * 0.5f;

			if (missing_mesh)
			{
				draw_text_3d(position, "MISSING MESH", color_t::red, DEBUG_DRAW_MISSING_RESOURCE_TEXT_SIZE_PX, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::center, {0.0f, text_y});
				text_y += DEBUG_DRAW_MISSING_RESOURCE_TEXT_LINE_SPACING_PX;
			}

			if (missing_sprite)
			{
				draw_text_3d(position, "MISSING SPRITE", color_t::red, DEBUG_DRAW_MISSING_RESOURCE_TEXT_SIZE_PX, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::center, {0.0f, text_y});
				text_y += DEBUG_DRAW_MISSING_RESOURCE_TEXT_LINE_SPACING_PX;
			}

			if (missing_material)
			{
				draw_text_3d(position, "MISSING MATERIAL", color_t::red, DEBUG_DRAW_MISSING_RESOURCE_TEXT_SIZE_PX, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::center, {0.0f, text_y});
				text_y += DEBUG_DRAW_MISSING_RESOURCE_TEXT_LINE_SPACING_PX;
			}

			if (missing_skeleton)
				draw_text_3d(position, "MISSING SKELETON", color_t::red, DEBUG_DRAW_MISSING_RESOURCE_TEXT_SIZE_PX, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::center, {0.0f, text_y});
		};

		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table.ref(),
				transform_table.ref(),
				mesh_renderer_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform		 = ecs_helpers_t::row_get<component_system_transform_t>(row, 1);
				const component_mesh_renderer_t&	mesh_renderer	 = ecs_helpers_t::row_get<component_mesh_renderer_t>(row, 2);
				const bool							missing_mesh	 = resource_manager.find_entry(mesh_renderer.mesh) == nullptr;
				bool								missing_material = mesh_renderer.materials.empty();

				for (const resource_handle_t material : mesh_renderer.materials)
				{
					if (resource_manager.find_entry(material) != nullptr)
						continue;

					missing_material = true;
					break;
				}

				if (missing_mesh || missing_material)
					draw_missing_resource_texts(transform.abs_pos, missing_mesh, false, missing_material, false);
			}
		}

		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table.ref(),
				transform_table.ref(),
				sprite_renderer_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform		 = ecs_helpers_t::row_get<component_system_transform_t>(row, 1);
				const component_sprite_renderer_t&	sprite_renderer	 = ecs_helpers_t::row_get<component_sprite_renderer_t>(row, 2);
				const bool							missing_sprite	 = resource_manager.find_entry(sprite_renderer.sprite) == nullptr;
				const bool							missing_material = resource_manager.find_entry(sprite_renderer.material) == nullptr;

				if (missing_sprite || missing_material)
					draw_missing_resource_texts(transform.abs_pos, false, missing_sprite, missing_material, false);
			}
		}

		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table.ref(),
				transform_table.ref(),
				particle_emitter_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t& transform		 = ecs_helpers_t::row_get<component_system_transform_t>(row, 1);
				const component_particle_emitter_t& emitter			 = ecs_helpers_t::row_get<component_particle_emitter_t>(row, 2);
				const bool							missing_material = resource_manager.find_entry(emitter.material) == nullptr;

				if (missing_material)
					draw_missing_resource_texts(transform.abs_pos, false, false, true, false);
			}
		}

		{
			const ecs_component_table_ref_t table_refs[] = {
				alive_table.ref(),
				transform_table.ref(),
				skinned_mesh_renderer_table.ref(),
				!disabled_table.ref(),
			};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
			{
				const component_system_transform_t&		 transform			   = ecs_helpers_t::row_get<component_system_transform_t>(row, 1);
				const component_skinned_mesh_renderer_t& skinned_mesh_renderer = ecs_helpers_t::row_get<component_skinned_mesh_renderer_t>(row, 2);
				const bool								 missing_mesh		   = resource_manager.find_entry(skinned_mesh_renderer.mesh) == nullptr;
				const bool								 missing_skeleton	   = resource_manager.find_entry(skinned_mesh_renderer.skeleton) == nullptr;
				bool									 missing_material	   = skinned_mesh_renderer.materials.empty();

				for (const resource_handle_t material : skinned_mesh_renderer.materials)
				{
					if (resource_manager.find_entry(material) != nullptr)
						continue;

					missing_material = true;
					break;
				}

				if (missing_mesh || missing_material || missing_skeleton)
					draw_missing_resource_texts(transform.abs_pos, missing_mesh, false, missing_material, missing_skeleton);
			}
		}
	}

	void world_debug_draw_t::write_snapshot(world_debug_draw_snapshot_t& snapshot)
	{
		snapshot.line_vertices.resize(_vertices.size());
		if (!_vertices.empty())
			SFG_MEMCPY(snapshot.line_vertices.data(), _vertices.data(), _vertices.size() * sizeof(vertex_debug_line_t));

		snapshot.line_indices.resize(_indices.size());
		if (!_indices.empty())
			SFG_MEMCPY(snapshot.line_indices.data(), _indices.data(), _indices.size() * sizeof(primitive_index));

		snapshot.triangle_vertices.resize(_triangle_vertices.size());
		if (!_triangle_vertices.empty())
			SFG_MEMCPY(snapshot.triangle_vertices.data(), _triangle_vertices.data(), _triangle_vertices.size() * sizeof(vertex_debug_triangle_t));

		snapshot.triangle_indices.resize(_triangle_indices.size());
		if (!_triangle_indices.empty())
			SFG_MEMCPY(snapshot.triangle_indices.data(), _triangle_indices.data(), _triangle_indices.size() * sizeof(primitive_index));

		snapshot.text_vertices.resize(0);
		snapshot.text_indices.resize(0);
		snapshot.textures.resize(0);

		resource_manager_t& resource_manager = resource_manager_t::get();

		for (const texture_command_t& command : _texture_commands)
		{
			const resource_entry_t* entry = resource_manager.find_entry(command.texture);

			if (entry == nullptr || entry->type != resource_type_e::texture || entry->state != resource_state_e::ready)
			{
				++_dropped_texture_count;
				continue;
			}

			const texture_internals_t* internals = resource_manager.find_internals<texture_internals_t>(command.texture);
			SFG_ASSERT(internals != nullptr && !internals->texture.is_null());

			u32 flags = world_debug_draw_texture_flag_none;
			flags |= command.depth == debug_draw_depth_e::depth_tested ? world_debug_draw_texture_flag_depth_tested : 0;
			flags |= command.is_linear_sample ? world_debug_draw_texture_flag_linear_sample : 0;

			snapshot.textures.push_back({
				.color		   = command.color,
				.position	   = command.position,
				.texture	   = internals->texture,
				.size_px	   = command.size_px,
				.screen_offset = command.screen_offset,
				.entity_id	   = command.entity_id,
				.flags		   = flags,
			});
		}

		if (_text_commands.empty())
			return;

		_text_canvas->frame_begin({0.0f, 0.0f, static_cast<f32>(_config.text_budget_bytes), static_cast<f32>(_config.text_budget_bytes)});
		const ui::ui_render_state_t state = {};

		for (const text_command_t& command : _text_commands)
		{
			if (snapshot.text_vertices.size() + command.text_length * 4 > _config.text_vertex_max_count || snapshot.text_indices.size() + command.text_length * 6 > _config.text_index_max_count)
			{
				++_dropped_text_count;
				continue;
			}

			const resource_handle_t font_handle = command.font == NULL_RESOURCE_HANDLE ? WORLD_DEBUG_DRAW_FONT : command.font;
			SFG_ASSERT(font_handle != NULL_RESOURCE_HANDLE);

			const font_runtime_t* font = resource_manager.find_runtime<font_runtime_t>(font_handle);
			SFG_ASSERT(font != nullptr);

			const ui::vg_text_paint_t paint = {
				.font		 = font,
				.color		 = command.color,
				.size_px	 = command.size_px,
				.raster_px	 = ui::get_text_raster_px(command.size_px, 1.0f),
				.spacing	 = 0.0f,
				.raster_mode = ui::glyph_raster_mode_e::grayscale,
			};

			const char*			  text		   = _text_bytes.data() + command.text_offset;
			const vec2f_t		  size		   = ui::vg_canvas_t::measure_text(text, command.text_length, paint);
			const u8			  alignment	   = static_cast<u8>(command.alignment);
			const f32			  horizontal   = static_cast<f32>(alignment % 3) * 0.5f;
			const f32			  vertical	   = static_cast<f32>(alignment / 3) * 0.5f;
			const vec2f_t		  offset	   = command.screen_offset - vec2f_t(size.x * horizontal, size.y * vertical);
			ui::vg_draw_buffer_t* draw_buffer  = _text_canvas->get_draw_buffer(0, state, command.text_length * 4);
			const u32			  vertex_start = draw_buffer->vertex_count;
			const u32			  index_start  = draw_buffer->index_count;

			_text_canvas->add_text(text, command.text_length, offset, paint, state, 0, true);

			const u32			  vertex_count = draw_buffer->vertex_count - vertex_start;
			const u32			  index_count  = draw_buffer->index_count - index_start;
			const primitive_index base_vertex  = static_cast<primitive_index>(snapshot.text_vertices.size());
			const f32			  mode		   = command.is_screen ? 0.0f : command.depth == debug_draw_depth_e::depth_tested ? 1.0f : 2.0f;

			snapshot.text_vertices.resize(snapshot.text_vertices.size() + vertex_count);
			for (u32 i = 0; i < vertex_count; ++i)
			{
				const ui::vg_vertex_t& source			= _text_canvas->get_draw_buffer_vertex(*draw_buffer, vertex_start + i);
				snapshot.text_vertices[base_vertex + i] = {
					.color	= source.color,
					.anchor = command.anchor,
					.offset = source.pos,
					.uv		= source.uv,
					.mode	= mode,
				};
			}

			snapshot.text_indices.resize(snapshot.text_indices.size() + index_count);
			const size_t index_output_start = snapshot.text_indices.size() - index_count;
			for (u32 i = 0; i < index_count; ++i)
				snapshot.text_indices[index_output_start + i] = base_vertex + static_cast<primitive_index>(_text_canvas->get_draw_buffer_index(*draw_buffer, index_start + i) - vertex_start);
		}

		_text_canvas->frame_end();
	}
}
