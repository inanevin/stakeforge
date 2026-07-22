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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2f.hpp>

namespace sfg::ui
{
	void	vg_path_sharp_rect(vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max);
	void	vg_path_rounded_rect(vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max, f32 rounding, u32 segments);
	void	vg_path_inset_rect_4(vector_t<vec2f_t>& out_path, const vec2f_t& min, const vec2f_t& max, f32 amount);
	void	vg_path_expand(vector_t<vec2f_t>& out_path, const vector_t<vec2f_t>& base_path, f32 expand);
	void	vg_path_circle(vector_t<vec2f_t>& out_path, const vec2f_t& center, f32 radius, u32 segments);
	void	vg_path_arc(vector_t<vec2f_t>& out_path, const vec2f_t& center, f32 radius, f32 start, f32 end, u32 segments);
	vec2f_t vg_cubic_bezier_point(const vec2f_t& p0, const vec2f_t& p1, const vec2f_t& p2, const vec2f_t& p3, f32 t);
	void	vg_path_cubic_bezier(vector_t<vec2f_t>& out_path, const vec2f_t& p0, const vec2f_t& p1, const vec2f_t& p2, const vec2f_t& p3, u32 segments);
}
