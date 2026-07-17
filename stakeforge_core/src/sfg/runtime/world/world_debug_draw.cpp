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
#include <sfg/io/assert.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/render/world_debug_draw_snapshot.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
	world_debug_draw_t::world_debug_draw_t()  = default;
	world_debug_draw_t::~world_debug_draw_t() = default;

	void world_debug_draw_t::init(const world_debug_draw_config_t& config)
	{
		SFG_ASSERT((config.line_vertex_reserve == 0) == (config.line_index_reserve == 0));
		SFG_ASSERT((config.text_vertex_max == 0) == (config.text_index_max == 0));
		SFG_ASSERT((config.text_vertex_max == 0) == (config.text_command_reserve == 0));
		SFG_ASSERT((config.text_vertex_max == 0) == (config.text_byte_reserve == 0));
		_config = config;
		_vertices.reserve(config.line_vertex_reserve);
		_indices.reserve(config.line_index_reserve);
		_text_commands.reserve(config.text_command_reserve);
		_text_bytes.reserve(config.text_byte_reserve);

		if (config.text_vertex_max > 0)
		{
			_text_canvas = make_unique<ui::vg_canvas_t>();
			_text_canvas->init({
				.vertex_buffer_bytes	 = config.text_vertex_max * sizeof(ui::vg_vertex_t),
				.index_buffer_bytes		 = config.text_index_max * sizeof(ui::vg_index_t),
				.buffer_count			 = 1,
				.text_cache_vertex_bytes = config.text_vertex_max * sizeof(ui::vg_vertex_t),
				.text_cache_index_bytes	 = config.text_index_max * sizeof(ui::vg_index_t),
				.clip_stack_capacity	 = 1,
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
		_text_commands.resize(0);
		_text_commands.shrink_to_fit();
		_text_bytes.resize(0);
		_text_bytes.shrink_to_fit();
		if (_text_canvas)
		{
			_text_canvas->uninit();
			_text_canvas.reset();
		}
		_config				= {};
		_dropped_line_count = 0;
		_dropped_text_count = 0;
	}

	void world_debug_draw_t::begin_frame()
	{
		_vertices.resize(0);
		_indices.resize(0);
		_text_commands.resize(0);
		_text_bytes.resize(0);
		_dropped_line_count = 0;
		_dropped_text_count = 0;
	}

	void world_debug_draw_t::draw_line(const vec3f_t& from, const vec3f_t& to, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		if (from.equals(to) || thickness_px <= 0.0f)
			return;

		if (_vertices.size() + 4 > _config.line_vertex_reserve || _indices.size() + 6 > _config.line_index_reserve)
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

	void world_debug_draw_t::draw_circle(const vec3f_t& center, f32 radius, const vec3f_t& normal, const color_t& color, f32 thickness_px, debug_draw_depth_e depth, u32 segments)
	{
		SFG_ASSERT(radius > 0.0f && segments >= 3 && !normal.is_zero());
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
		SFG_ASSERT(radius > 0.0f && half_height >= 0.0f && segments >= 4 && !direction.is_zero());
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

	void world_debug_draw_t::draw_frustum(const vec3f_t& origin, const vec3f_t& direction, f32 fov_degrees, f32 aspect_ratio, f32 near_distance, f32 far_distance, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		const vec3f_t forward	= direction.normalized();
		const vec3f_t reference = math::abs(vec3f_t::dot(forward, vec3f_t::up)) > 0.99f ? vec3f_t::right : vec3f_t::up;
		draw_frustum(origin, direction, reference, fov_degrees, aspect_ratio, near_distance, far_distance, color, thickness_px, depth);
	}

	void world_debug_draw_t::draw_frustum(const vec3f_t& origin, const vec3f_t& direction, const vec3f_t& up, f32 fov_degrees, f32 aspect_ratio, f32 near_distance, f32 far_distance, const color_t& color, f32 thickness_px, debug_draw_depth_e depth)
	{
		SFG_ASSERT(!direction.is_zero() && !up.is_zero() && fov_degrees > 0.0f && aspect_ratio > 0.0f && near_distance >= 0.0f && far_distance > near_distance);
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
		const vec3f_t corners[8]  = {
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
		if (_config.text_vertex_max == 0 || _text_commands.size() >= _config.text_command_reserve || _text_bytes.size() + text_length > _config.text_byte_reserve)
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
		if (_config.text_vertex_max == 0 || _text_commands.size() >= _config.text_command_reserve || _text_bytes.size() + text_length > _config.text_byte_reserve)
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

	void world_debug_draw_t::write_snapshot(world_debug_draw_snapshot_t& snapshot)
	{
		snapshot.line_vertices.resize(_vertices.size());
		if (!_vertices.empty())
			SFG_MEMCPY(snapshot.line_vertices.data(), _vertices.data(), _vertices.size() * sizeof(vertex_debug_line_t));

		snapshot.line_indices.resize(_indices.size());
		if (!_indices.empty())
			SFG_MEMCPY(snapshot.line_indices.data(), _indices.data(), _indices.size() * sizeof(primitive_index));

		snapshot.text_vertices.resize(0);
		snapshot.text_indices.resize(0);
		if (_text_commands.empty())
			return;

		_text_canvas->frame_begin({0.0f, 0.0f, static_cast<f32>(_config.text_byte_reserve), static_cast<f32>(_config.text_byte_reserve)});
		const ui::ui_render_state_t state = {};

		for (const text_command_t& command : _text_commands)
		{
			if (snapshot.text_vertices.size() + command.text_length * 4 > _config.text_vertex_max || snapshot.text_indices.size() + command.text_length * 6 > _config.text_index_max)
			{
				++_dropped_text_count;
				continue;
			}

			const resource_handle_t font_handle = command.font == NULL_RESOURCE_HANDLE ? _config.font : command.font;
			SFG_ASSERT(font_handle != NULL_RESOURCE_HANDLE);

			const font_runtime_t* font = resource_manager_t::get().find_runtime<font_runtime_t>(font_handle);
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
			ui::vg_draw_buffer_t* draw_buffer  = _text_canvas->get_draw_buffer(0, state);
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
				const ui::vg_vertex_t& source			= draw_buffer->vertex_start[vertex_start + i];
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
				snapshot.text_indices[index_output_start + i] = base_vertex + static_cast<primitive_index>(draw_buffer->index_start[index_start + i] - vertex_start);
		}

		_text_canvas->frame_end();
	}
}
