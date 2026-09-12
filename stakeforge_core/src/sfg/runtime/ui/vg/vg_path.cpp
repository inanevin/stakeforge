/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.
*/

#include "vg_path.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>

namespace sfg::ui
{
	namespace
	{
		constexpr f32 deg2rad = 0.0174532925f;
	}

	void vg_path_sharp_rect(frame_vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max)
	{
		out_path.resize(4);
		out_path[0] = {min.x, min.y};
		out_path[1] = {max.x, min.y};
		out_path[2] = {max.x, max.y};
		out_path[3] = {min.x, max.y};
	}

	void vg_path_rounded_rect(frame_vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max, f32 rounding, u32 segments)
	{
		const f32 half_min = math::min((max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f);
		f32		  r		   = math::min(rounding, half_min);

		if (r < 0.0f)
			r = 0.0f;

		i32 segs = static_cast<i32>(segments);

		if (segs == 0)
			segs = 10;
		if (segs < 1)
			segs = 1;
		if (segs > 90)
			segs = 90;

		out_path.resize(0);
		out_path.reserve(static_cast<size_t>(segs + 1) * 4);

		const f32 step = 90.0f / static_cast<f32>(segs);

		{
			const vec2f_t c = {min.x + r, min.y + r};

			for (i32 i = 0; i <= segs; ++i)
			{
				const f32	  a		= deg2rad * (270.0f + step * static_cast<f32>(i));
				const vec2f_t local = {math::sin(a) * r, -math::cos(a) * r};
				out_path.push_back({c.x + local.x, c.y + local.y});
			}
		}

		{
			const vec2f_t c = {max.x - r, min.y + r};

			for (i32 i = 0; i <= segs; ++i)
			{
				const f32	  a		= deg2rad * (step * static_cast<f32>(i));
				const vec2f_t local = {math::sin(a) * r, -math::cos(a) * r};
				out_path.push_back({c.x + local.x, c.y + local.y});
			}
		}

		{
			const vec2f_t c = {max.x - r, max.y - r};

			for (i32 i = 0; i <= segs; ++i)
			{
				const f32	  a		= deg2rad * (90.0f + step * static_cast<f32>(i));
				const vec2f_t local = {math::sin(a) * r, -math::cos(a) * r};
				out_path.push_back({c.x + local.x, c.y + local.y});
			}
		}

		{
			const vec2f_t c = {min.x + r, max.y - r};

			for (i32 i = 0; i <= segs; ++i)
			{
				const f32	  a		= deg2rad * (180.0f + step * static_cast<f32>(i));
				const vec2f_t local = {math::sin(a) * r, -math::cos(a) * r};
				out_path.push_back({c.x + local.x, c.y + local.y});
			}
		}
	}

	void vg_path_inset_rect_4(frame_vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max, f32 amount)
	{
		out_path.resize(4);
		out_path[0] = {min.x + amount, min.y + amount};
		out_path[1] = {max.x - amount, min.y + amount};
		out_path[2] = {max.x - amount, max.y - amount};
		out_path[3] = {min.x + amount, max.y - amount};
	}

	void vg_path_expand(frame_vector_t<vec2f_t>& out_path, span_t<const vec2f_t> base_path, f32 expand)
	{
		const size_t n = base_path.size;

		if (n < 2)
		{
			out_path.resize(0);
			return;
		}

		out_path.resize(n);

		const f32 d = -expand;

		for (size_t i = 0; i < n; ++i)
		{
			const vec2f_t& p_curr = base_path[i];
			const vec2f_t& p_prev = base_path[(i + n - 1) % n];
			const vec2f_t& p_next = base_path[(i + 1) % n];

			const vec2f_t t1	= (p_curr - p_prev).normalized();
			const vec2f_t t2	= (p_next - p_curr).normalized();
			const vec2f_t n1	= {-t1.y, t1.x};
			const vec2f_t n2	= {-t2.y, t2.x};
			const vec2f_t miter = (n1 + n2).normalized();

			out_path[i] = {p_curr.x + miter.x * d, p_curr.y + miter.y * d};
		}
	}

	void vg_path_circle(frame_vector_t<vec2f_t>& out_path, const vec2f_t& center, f32 radius, u32 segments)
	{
		if (segments < 3)
			segments = 3;

		out_path.resize(segments);

		const f32 step = 6.2831853f / static_cast<f32>(segments);

		for (u32 i = 0; i < segments; ++i)
		{
			const f32 a = step * static_cast<f32>(i);
			out_path[i] = {center.x + math::cos(a) * radius, center.y + math::sin(a) * radius};
		}
	}

	void vg_path_arc(frame_vector_t<vec2f_t>& out_path, const vec2f_t& center, f32 radius, f32 start, f32 end, u32 segments)
	{
		if (segments < 1)
			segments = 1;

		out_path.resize(segments + 1);

		for (u32 i = 0; i <= segments; ++i)
		{
			const f32 t = static_cast<f32>(i) / static_cast<f32>(segments);
			const f32 a = math::lerp(start, end, t);
			out_path[i] = {center.x + math::cos(a) * radius, center.y + math::sin(a) * radius};
		}
	}

	vec2f_t vg_cubic_bezier_point(const vec2f_t& p0, const vec2f_t& p1, const vec2f_t& p2, const vec2f_t& p3, f32 t)
	{
		const f32 inverse_t			= 1.0f - t;
		const f32 inverse_t_squared = inverse_t * inverse_t;
		const f32 t_squared			= t * t;

		return p0 * (inverse_t_squared * inverse_t) + p1 * (3.0f * inverse_t_squared * t) + p2 * (3.0f * inverse_t * t_squared) + p3 * (t_squared * t);
	}

	void vg_path_cubic_bezier(frame_vector_t<vec2f_t>& out_path, const vec2f_t& p0, const vec2f_t& p1, const vec2f_t& p2, const vec2f_t& p3, u32 segments)
	{
		SFG_ASSERT(segments != 0);

		out_path.resize(segments + 1);

		for (u32 segment_index = 0; segment_index <= segments; ++segment_index)
		{
			const f32 t = static_cast<f32>(segment_index) / static_cast<f32>(segments);

			out_path[segment_index] = vg_cubic_bezier_point(p0, p1, p2, p3, t);
		}
	}
}
