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
#include "ui/widgets/editor_widgets_draws.hpp"
#include <sfg/runtime/ui/vg/vg_canvas.hpp>
#include <sfg/math/vec2f.hpp>

namespace sfg
{
	void editor_custom_draws_t::add_leaned_convex_rect(ui::vg_canvas_t& canvas, const vec2f_t& pos, const vec2f_t& size, f32 lean, const ui::vg_convex_paint_t& paint, const ui::ui_render_state_t& state, u32 draw_order)
	{
		const f32 skew = size.y * lean;

		const vec2f_t p0	  = {pos.x + skew, pos.y};
		const vec2f_t p1	  = {p0.x + size.x, pos.y};
		const vec2f_t p2	  = {pos.x + size.x, pos.y + size.y};
		const vec2f_t p3	  = {pos.x, pos.y + size.y};
		vec2f_t		  path[4] = {p0, p1, p2, p3};

		canvas.add_convex({path, 4}, paint, state, draw_order);
	}
}
