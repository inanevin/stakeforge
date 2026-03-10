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

#include "world_debug_rendering.hpp"
#include "gui/vekt.hpp"
#include "memory/memory.hpp"
#include "io/assert.hpp"
#include "math/color.hpp"
#include "math/math.hpp"
#include "math/vector3.hpp"
#include "math/vector2ui16.hpp"
#include "gfx/common/gfx_constants.hpp"

namespace SFG
{
	namespace
	{
		vector3 normalized_or_up(const vector3& v)
		{
			if (v.is_zero())
				return vector3::up;
			return v.normalized();
		}

		void make_basis(const vector3& direction, vector3& axis0, vector3& axis1, vector3& normal)
		{
			normal				 = normalized_or_up(direction);
			const f32	  dot_up = math::abs(vector3::dot(normal, vector3::up));
			const vector3 ref	 = dot_up > 0.99f ? vector3::right : vector3::up;
			axis0				 = vector3::cross(ref, normal).normalized();
			axis1				 = vector3::cross(normal, axis0).normalized();
		}
	}

	void world_debug_rendering::init()
	{
		for (u32 i = 0; i < 3; ++i)
		{
			snapshot& sh	  = _snapshots[i];
			sh.vertices_gui	  = new vertex_gui[MAX_VERTEX_COUNT_GUI];
			sh.vertices_line  = new vertex_3d_line[MAX_VERTEX_COUNT_LINE];
			sh.vertices_tri	  = new vertex_simple[MAX_VERTEX_COUNT_TRI];
			sh.indices_gui	  = new primitive_index[MAX_INDEX_COUNT_GUI];
			sh.indices_line	  = new primitive_index[MAX_INDEX_COUNT_LINE];
			sh.indices_tri	  = new primitive_index[MAX_INDEX_COUNT_TRI];
			sh.draw_calls_gui = new gui_draw_call[MAX_DRAW_CALLS_GUI];
			sh.draw_data_gui  = new gui_draw_call_data[MAX_DRAW_CALLS_GUI];
		}

		_builder						= new vekt::builder();
		vekt::builder::init_config conf = {
			.widget_count				 = 1024,
			.vertex_buffer_sz			 = 1024 * 1024,
			.index_buffer_sz			 = 1024 * 1024,
			.text_cache_vertex_buffer_sz = 1024 * 1024,
			.text_cache_index_buffer_sz	 = 1024 * 1024,
			.buffer_count				 = 50,
		};
		_builder->init(conf);

		_published.store(UINT32_MAX, std::memory_order_relaxed);
		_snapshot_in_use.store(UINT32_MAX, std::memory_order_relaxed);
		_writer_slot  = 0;
		_current_read = UINT32_MAX;
	}

	void world_debug_rendering::uninit()
	{
		for (u32 i = 0; i < 3; ++i)
		{
			snapshot& sh = _snapshots[i];
			delete[] sh.vertices_gui;
			delete[] sh.vertices_line;
			delete[] sh.vertices_tri;
			delete[] sh.indices_gui;
			delete[] sh.indices_tri;
			delete[] sh.indices_line;
			delete[] sh.draw_calls_gui;
			delete[] sh.draw_data_gui;
			sh = {};
		}

		if (_builder)
		{
			_builder->uninit();
			delete _builder;
			_builder = nullptr;
		}
	}

	void world_debug_rendering::begin_frame(const vector2ui16& res)
	{
		world_debug_rendering::snapshot& w = _snapshots[_writer_slot];
		w.reset();
		_builder->build_begin(vector2(24000, 24000));
	}

	void world_debug_rendering::end_frame()
	{
		_builder->build_end();

		const auto& dbs		   = _builder->get_draw_buffers();
		u32			vertex_ctr = 0;
		u32			index_ctr  = 0;

		snapshot& s = _snapshots[_writer_slot];

		for (const vekt::draw_buffer& db : dbs)
		{
			SFG_MEMCPY(&s.vertices_gui[s.vtx_count_gui], db.vertex_start, sizeof(vekt::vertex) * db.vertex_count);
			SFG_MEMCPY(&s.indices_gui[s.idx_count_gui], db.index_start, sizeof(vekt::index) * db.index_count);

			gpu_index fidx = NULL_GPU_INDEX;

			if (db.font_id == _font_id_default)
				fidx = _gpu_index_font_default;
			else if (db.font_id == _font_id_icons)
				fidx = _gpu_index_font_icon;

			gui_draw_call_data* dt	= reinterpret_cast<gui_draw_call_data*>(db.user_data);
			const size_t		off = dt - s.draw_data_gui;

			gui_draw_call& dc = s.draw_calls_gui[s.dc_count_gui];
			dc				  = {
							   .start_index	  = s.idx_count_gui,
							   .index_count	  = db.index_count,
							   .base_vertex	  = s.vtx_count_gui,
							   .vertex_size	  = sizeof(vekt::vertex),
							   .font_idx	  = fidx,
							   .draw_data_idx = static_cast<u32>(off),
							   .is_icon		  = fidx == _gpu_index_font_icon,
			   };

			s.vtx_count_gui += db.vertex_count;
			s.idx_count_gui += db.index_count;
			s.dc_count_gui++;
		}

		const u32 next = (_writer_slot + 1) % 3;
		_published.store(_writer_slot, std::memory_order_release);
		u32 cur_reader = _snapshot_in_use.load(std::memory_order_acquire);
		if (cur_reader != UINT32_MAX)
			_snapshot_in_use.store(UINT32_MAX, std::memory_order_release);
		_writer_slot = next;
	}

	void world_debug_rendering::add_indices_line(const primitive_index* data, u32 count)
	{
		snapshot& s		 = _snapshots[_writer_slot];
		const u32 before = s.idx_count_line;
		SFG_ASSERT(s.idx_count_line + count < MAX_INDEX_COUNT_LINE);
		SFG_MEMCPY(&s.indices_line[before], data, sizeof(primitive_index) * count);
		s.idx_count_line += count;
	}

	void world_debug_rendering::add_indices_tri(const primitive_index* data, u32 count)
	{
		snapshot& s		 = _snapshots[_writer_slot];
		const u32 before = s.idx_count_tri;
		SFG_ASSERT(s.idx_count_tri + count < MAX_INDEX_COUNT_TRI);
		SFG_MEMCPY(&s.indices_tri[before], data, sizeof(primitive_index) * count);
		s.idx_count_tri += count;
	}

	void world_debug_rendering::add_indices_gui(const primitive_index* data, u32 count)
	{
		snapshot& s		 = _snapshots[_writer_slot];
		const u32 before = s.idx_count_gui;
		SFG_ASSERT(s.idx_count_gui + count < MAX_INDEX_COUNT_GUI);
		SFG_MEMCPY(&s.indices_gui[before], data, sizeof(primitive_index) * count);
		s.idx_count_gui += count;
	}

	u32 world_debug_rendering::add_vertex_line(const vertex_3d_line* data, u32 count)
	{
		snapshot& s		 = _snapshots[_writer_slot];
		const u32 before = s.vtx_count_line;
		SFG_ASSERT(s.vtx_count_line + count < MAX_VERTEX_COUNT_LINE);

		SFG_MEMCPY(&s.vertices_line[before], data, sizeof(vertex_3d_line) * count);
		s.vtx_count_line += count;
		return before;
	}
	u32 world_debug_rendering::add_vertex_tri(const vertex_simple* data, u32 count)
	{
		snapshot& s		 = _snapshots[_writer_slot];
		const u32 before = s.vtx_count_tri;
		SFG_ASSERT(s.vtx_count_tri + count < MAX_VERTEX_COUNT_TRI);
		SFG_MEMCPY(&s.vertices_tri[before], data, sizeof(vertex_simple) * count);
		s.vtx_count_tri += count;
		return before;
	}
	u32 world_debug_rendering::add_vertex_gui(const vertex_gui* data, u32 count)
	{
		snapshot& s		 = _snapshots[_writer_slot];
		const u32 before = s.vtx_count_gui;
		SFG_ASSERT(s.vtx_count_gui + count < MAX_VERTEX_COUNT_GUI);
		SFG_MEMCPY(&s.vertices_gui[before], data, sizeof(vertex_gui) * count);
		s.vtx_count_gui += count;
		return before;
	}

	void world_debug_rendering::draw_line(const vector3& p0, const vector3& p1, const color& col, f32 thickness)
	{
		snapshot& s = _snapshots[_writer_slot];
		if (s.vtx_count_line + 4 >= MAX_VERTEX_COUNT_LINE || s.idx_count_line + 6 >= MAX_INDEX_COUNT_LINE)
			return;

		const vertex_3d_line v[4] = {
			{.pos = p0, .next_pos = p1, .color = col.to_vector(), .direction = thickness},
			{.pos = p0, .next_pos = p1, .color = col.to_vector(), .direction = -thickness},
			{.pos = p1, .next_pos = p0, .color = col.to_vector(), .direction = thickness},
			{.pos = p1, .next_pos = p0, .color = col.to_vector(), .direction = -thickness},

		};

		const u32			  base_vtx = add_vertex_line(v, 4);
		const primitive_index idxs[6]  = {
			 (primitive_index)(base_vtx + 0),
			 (primitive_index)(base_vtx + 1),
			 (primitive_index)(base_vtx + 2),
			 (primitive_index)(base_vtx + 2),
			 (primitive_index)(base_vtx + 3),
			 (primitive_index)(base_vtx + 0),
		 };
		const u32 start_idx = static_cast<u32>(s.idx_count_line);
		add_indices_line(idxs, 6);
	}

	void world_debug_rendering::draw_triangle(const vector3& p0, const vector3& p1, const vector3& p2, const color& col)
	{
		snapshot& s = _snapshots[_writer_slot];
		if (s.vtx_count_tri + 3 >= MAX_VERTEX_COUNT_TRI || s.idx_count_tri + 3 >= MAX_INDEX_COUNT_TRI)
			return;

		const vertex_simple v[3] = {
			{.pos = p0, .color = col.to_vector()},
			{.pos = p1, .color = col.to_vector()},
			{.pos = p2, .color = col.to_vector()},
		};
		const u32			  base_vtx	= add_vertex_tri(v, 3);
		const primitive_index idxs[3]	= {(primitive_index)(base_vtx + 0), (primitive_index)(base_vtx + 1), (primitive_index)(base_vtx + 2)};
		const u32			  start_idx = static_cast<u32>(s.idx_count_tri);
		add_indices_tri(idxs, 3);
	}

	void world_debug_rendering::draw_box(const vector3& center, const vector3& half_extents, const vector3& forward, const color& col, f32 thickness)
	{
		const vector3 dir	 = normalized_or_up(forward);
		const f32	  dot_up = math::abs(vector3::dot(dir, vector3::up));
		const vector3 ref	 = dot_up > 0.99f ? vector3::right : vector3::up;
		const vector3 right	 = vector3::cross(ref, dir).normalized();
		const vector3 up	 = vector3::cross(dir, right).normalized();

		const vector3 x = right * half_extents.x;
		const vector3 y = up * half_extents.y;
		const vector3 z = dir * half_extents.z;

		const vector3 c[8] = {
			center - x - y - z, // 0
			center + x - y - z, // 1
			center + x - y + z, // 2
			center - x - y + z, // 3
			center - x + y - z, // 4
			center + x + y - z, // 5
			center + x + y + z, // 6
			center - x + y + z, // 7
		};

		// bottom rectangle
		draw_line(c[0], c[1], col, thickness);
		draw_line(c[1], c[2], col, thickness);
		draw_line(c[2], c[3], col, thickness);
		draw_line(c[3], c[0], col, thickness);

		// top rectangle
		draw_line(c[4], c[5], col, thickness);
		draw_line(c[5], c[6], col, thickness);
		draw_line(c[6], c[7], col, thickness);
		draw_line(c[7], c[4], col, thickness);

		// vertical edges
		draw_line(c[0], c[4], col, thickness);
		draw_line(c[1], c[5], col, thickness);
		draw_line(c[2], c[6], col, thickness);
		draw_line(c[3], c[7], col, thickness);
	}

	void world_debug_rendering::draw_oriented_circle(const vector3& center, f32 radius, const vector3& direction, const color& col, f32 thickness, u32 segments)
	{
		if (segments < 3)
			segments = 3;

		vector3 axis0, axis1, normal;
		make_basis(direction, axis0, axis1, normal);

		const f32 two_pi = 2.0f * MATH_PI;

		for (u32 i = 0; i < segments; ++i)
		{
			const f32 t0 = (f32)i / (f32)segments;
			const f32 t1 = (f32)(i + 1) / (f32)segments;

			const f32 a0 = two_pi * t0;
			const f32 a1 = two_pi * t1;

			const f32 c0 = math::cos(a0);
			const f32 s0 = math::sin(a0);
			const f32 c1 = math::cos(a1);
			const f32 s1 = math::sin(a1);

			const vector3 p0 = center + (axis0 * (c0 * radius)) + (axis1 * (s0 * radius));
			const vector3 p1 = center + (axis0 * (c1 * radius)) + (axis1 * (s1 * radius));

			draw_line(p0, p1, col, thickness);
		}
	}

	void world_debug_rendering::draw_sphere(const vector3& center, f32 radius, const color& col, f32 thickness, u32 segments)
	{
		draw_oriented_circle(center, radius, vector3::up, col, thickness, segments);
		draw_oriented_circle(center, radius, vector3::right, col, thickness, segments);
		draw_oriented_circle(center, radius, vector3::forward, col, thickness, segments);
	}

	void world_debug_rendering::draw_oriented_hemisphere(const vector3& center, f32 radius, const vector3& direction, const color& col, f32 thickness, u32 segments)
	{
		if (segments < 4)
			segments = 4;

		vector3 axis0, axis1, normal;
		make_basis(direction, axis0, axis1, normal);

		draw_oriented_circle(center, radius, normal, col, thickness, segments);

		const f32 two_pi = 2.0f * MATH_PI;

		auto draw_meridian = [&](const vector3& lateral_axis) {
			vector3 prev = center + (lateral_axis * radius);
			for (u32 i = 1; i <= segments; ++i)
			{
				const f32 t = (f32)i / (f32)segments;
				const f32 a = MATH_PI * t;

				const f32 ca = math::cos(a);
				const f32 sa = math::sin(a);

				const vector3 p = center + (lateral_axis * (ca * radius)) + (normal * (sa * radius));
				draw_line(prev, p, col, thickness);
				prev = p;
			}
		};

		draw_meridian(axis0);
		draw_meridian(axis1);
		draw_meridian((axis0 + axis1).normalized());
		draw_meridian((axis0 - axis1).normalized());
	}

	void world_debug_rendering::draw_capsule(const vector3& center, f32 radius, f32 half_height, const vector3& direction, const color& col, f32 thickness, u32 segments)
	{
		if (segments < 3)
			segments = 3;

		const vector3 dir		 = normalized_or_up(direction);
		const vector3 top_center = center + dir * half_height;
		const vector3 bot_center = center - dir * half_height;

		vector3 axis0, axis1, normal;
		make_basis(dir, axis0, axis1, normal);

		const f32 two_pi = 2.0f * MATH_PI;
		for (f32 t = 0.0f; t < two_pi; t += MATH_PI * 0.25f)
		{
			const f32 ca = math::cos(t);
			const f32 sa = math::sin(t);

			const vector3 ring_offset = (axis0 * (ca * radius)) + (axis1 * (sa * radius));
			const vector3 p_top		  = top_center + ring_offset;
			const vector3 p_bot		  = bot_center + ring_offset;

			draw_line(p_top, p_bot, col, thickness);
		}

		draw_oriented_hemisphere(top_center, radius, dir, col, thickness, segments);
		draw_oriented_hemisphere(bot_center, radius, -dir, col, thickness, segments);
	}

	void world_debug_rendering::draw_cylinder(const vector3& center, f32 radius, f32 half_height, const vector3& direction, const color& col, f32 thickness, u32 segments)
	{
		if (segments < 3)
			segments = 3;

		const vector3 dir		 = normalized_or_up(direction);
		const vector3 top_center = center + dir * half_height;
		const vector3 bot_center = center - dir * half_height;

		vector3 axis0, axis1, normal;
		make_basis(dir, axis0, axis1, normal);

		draw_oriented_circle(top_center, radius, dir, col, thickness, segments);
		draw_oriented_circle(bot_center, radius, dir, col, thickness, segments);

		const f32 two_pi = 2.0f * MATH_PI;
		for (f32 t = 0.0f; t < two_pi; t += MATH_PI * 0.25f)
		{
			const f32 ca = math::cos(t);
			const f32 sa = math::sin(t);

			const vector3 ring_offset = (axis0 * (ca * radius)) + (axis1 * (sa * radius));
			const vector3 p_top		  = top_center + ring_offset;
			const vector3 p_bot		  = bot_center + ring_offset;

			draw_line(p_top, p_bot, col, thickness);
		}
	}

	void world_debug_rendering::draw_oriented_cone(const vector3& apex, const vector3& direction, f32 length, f32 radius, const color& col, f32 thickness, u32 segments)
	{
		const vector3 dir		  = normalized_or_up(direction);
		const vector3 base_center = apex + dir * length;

		vector3 axis0, axis1, normal;
		make_basis(dir, axis0, axis1, normal);

		draw_oriented_circle(base_center, radius, dir, col, thickness, segments);

		const f32 two_pi = 2.0f * MATH_PI;
		for (u32 i = 0; i < segments; ++i)
		{
			const f32 t = (f32)i / (f32)segments;
			const f32 a = two_pi * t;

			const f32 ca = math::cos(a);
			const f32 sa = math::sin(a);

			const vector3 p = base_center + (axis0 * (ca * radius)) + (axis1 * (sa * radius));
			draw_line(apex, p, col, thickness);
		}

		draw_line(base_center + axis0 * radius, base_center - axis0 * radius, col, thickness);
		draw_line(base_center + axis1 * radius, base_center - axis1 * radius, col, thickness);
	}

	void world_debug_rendering::draw_oriented_plane(const vector3& center, f32 width, f32 height, const vector3& orientation, const color& col, f32 thickness, u32 segments)
	{
		if (segments < 2)
			segments = 2;

		vector3 axis0, axis1, normal;
		make_basis(orientation, axis0, axis1, normal);

		const f32 half_w = width * 0.5f;
		const f32 half_h = height * 0.5f;

		for (u32 i = 0; i < segments; ++i)
		{
			const f32	  t	 = static_cast<f32>(i) / static_cast<f32>(segments - 1);
			const f32	  v	 = math::lerp(-half_h, half_h, t);
			const vector3 p0 = center + (axis1 * v) - (axis0 * half_w);
			const vector3 p1 = center + (axis1 * v) + (axis0 * half_w);
			draw_line(p0, p1, col, thickness);
		}

		for (u32 i = 0; i < segments; ++i)
		{
			const f32	  t	 = static_cast<f32>(i) / static_cast<f32>(segments - 1);
			const f32	  v	 = math::lerp(-half_w, half_w, t);
			const vector3 p0 = center + (axis0 * v) - (axis1 * half_h);
			const vector3 p1 = center + (axis0 * v) + (axis1 * half_h);
			draw_line(p0, p1, col, thickness);
		}
	}

	void world_debug_rendering::draw_frustum(const vector3& origin, const vector3& direction, f32 fov_degrees, f32 aspect_ratio, f32 near_distance, f32 far_distance, const color& col, f32 thickness)
	{
		if (far_distance <= near_distance)
			return;
		if (near_distance < 0.0f)
			near_distance = 0.0f;
		if (aspect_ratio <= 0.0f)
			aspect_ratio = 1.0f;

		const vector3 dir = normalized_or_up(direction);
		vector3		  axis0, axis1, normal;
		make_basis(dir, axis0, axis1, normal);

		const f32 fov_rad	   = math::degrees_to_radians(fov_degrees);
		const f32 tan_half_fov = math::tan(0.5f * fov_rad);

		const vector3 near_center = origin + dir * near_distance;
		const vector3 far_center  = origin + dir * far_distance;

		const f32 near_half_h = near_distance * tan_half_fov;
		const f32 near_half_w = near_half_h * aspect_ratio;
		const f32 far_half_h  = far_distance * tan_half_fov;
		const f32 far_half_w  = far_half_h * aspect_ratio;

		const vector3 n0 = near_center + (axis0 * near_half_w) + (axis1 * near_half_h);
		const vector3 n1 = near_center - (axis0 * near_half_w) + (axis1 * near_half_h);
		const vector3 n2 = near_center - (axis0 * near_half_w) - (axis1 * near_half_h);
		const vector3 n3 = near_center + (axis0 * near_half_w) - (axis1 * near_half_h);

		const vector3 f0 = far_center + (axis0 * far_half_w) + (axis1 * far_half_h);
		const vector3 f1 = far_center - (axis0 * far_half_w) + (axis1 * far_half_h);
		const vector3 f2 = far_center - (axis0 * far_half_w) - (axis1 * far_half_h);
		const vector3 f3 = far_center + (axis0 * far_half_w) - (axis1 * far_half_h);

		draw_line(origin, n1, col, thickness);
		draw_line(origin, n2, col, thickness);
		draw_line(origin, n3, col, thickness);
		draw_line(origin, n0, col, thickness);

		draw_line(n0, n1, col, thickness);
		draw_line(n1, n2, col, thickness);
		draw_line(n2, n3, col, thickness);
		draw_line(n3, n0, col, thickness);

		draw_line(f0, f1, col, thickness);
		draw_line(f1, f2, col, thickness);
		draw_line(f2, f3, col, thickness);
		draw_line(f3, f0, col, thickness);

		draw_line(n0, f0, col, thickness);
		draw_line(n1, f1, col, thickness);
		draw_line(n2, f2, col, thickness);
		draw_line(n3, f3, col, thickness);
	}

	void world_debug_rendering::draw_icon(const vector3& pos, const color& col, const char* txt)
	{
		if (_font_icon == nullptr)
			return;
		const vekt::text_props tp = {
			.text		   = txt,
			.font		   = _font_icon,
			.text_capacity = strlen(txt),
			.scale		   = 1.0f,
		};
		const vector2 sz = _builder->get_text_size(tp);

		snapshot&			s  = _snapshots[_writer_slot];
		gui_draw_call_data& dd = s.draw_data_gui[s.draw_data_count_gui];

		_builder->add_text(tp, col.to_vector(), vector2::zero, sz, 0, &dd, true);
		dd.pos_and_scale = vector4(pos.x, pos.y, pos.z, 1.0f);
		dd.size			 = sz;
		s.draw_data_count_gui++;
	}

	void world_debug_rendering::draw_text(const vector3&, const color& col, const char* txt)
	{
		if (_font_default == nullptr)
			return;
	}

	const world_debug_rendering::snapshot* world_debug_rendering::get_read_snapshot() const
	{
		u32 idx = _published.load(std::memory_order_acquire);
		if (idx != UINT32_MAX)
			return &_snapshots[idx];
		return nullptr;
	}

	void world_debug_rendering::set_default_font(vekt::font* f, gpu_index idx)

	{
		_font_default			= f;
		_gpu_index_font_default = idx;
		_font_id_default		= f->_font_id;
	}

	void world_debug_rendering::set_icon_font(vekt::font* f, gpu_index idx)

	{
		_font_icon			 = f;
		_gpu_index_font_icon = idx;
		_font_id_icons		 = f->_font_id;
	}
}
