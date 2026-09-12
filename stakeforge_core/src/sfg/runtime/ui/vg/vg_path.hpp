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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/span.hpp>
#include <sfg/math/vec2f.hpp>

namespace sfg::ui
{
	void	vg_path_sharp_rect(frame_vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max);
	void	vg_path_rounded_rect(frame_vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max, f32 rounding, u32 segments);
	void	vg_path_inset_rect_4(frame_vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max, f32 amount);
	void	vg_path_expand(frame_vector_t<vec2f_t>& out_path, span_t<const vec2f_t> base_path, f32 expand);
	void	vg_path_circle(frame_vector_t<vec2f_t>& out_path, const vec2f_t& center, f32 radius, u32 segments);
	void	vg_path_arc(frame_vector_t<vec2f_t>& out_path, const vec2f_t& center, f32 radius, f32 start, f32 end, u32 segments);
	vec2f_t vg_cubic_bezier_point(const vec2f_t& p0, const vec2f_t& p1, const vec2f_t& p2, const vec2f_t& p3, f32 t);
	void	vg_path_cubic_bezier(frame_vector_t<vec2f_t>& out_path, const vec2f_t& p0, const vec2f_t& p1, const vec2f_t& p2, const vec2f_t& p3, u32 segments);
}
