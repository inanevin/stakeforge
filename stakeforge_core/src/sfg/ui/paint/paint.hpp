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

#include "common/size_definitions.hpp"
#include "data/vector.hpp"
#include "data/span.hpp"
#include "math/vec4f.hpp"
#include "ui/ui_common.hpp"
#include "ui/vg/vg_canvas.hpp"

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class layout_tree_t;

	enum class paint_kind_e : u8
	{
		none,
		rect,
		text,
		custom,
	};

	enum paint_state_flag_e : u8
	{
		psf_has_hover = 1 << 0,
		psf_has_press = 1 << 1,
		psf_has_focus = 1 << 2,
	};

	using paint_custom_fn = void (*)(paint_layer_t& paint, widget_id_t id, vg_canvas_t& canvas, void* user_data);

	struct paint_def_t
	{
		paint_kind_e	kind		= paint_kind_e::none;
		u8				state_flags = 0;
		vg_rect_paint_t rect		= {};
		vg_text_paint_t text		= {};
		// paint layer does not own this memory.
		const char* text_data = nullptr;
		u32			text_len  = 0;

		vec4f_t hover_color = {0, 0, 0, 0};
		vec4f_t press_color = {0, 0, 0, 0};
		vec4f_t focus_color = {0, 0, 0, 0};

		paint_custom_fn custom_fn = nullptr;
		void*			custom_ud = nullptr;
	};

	class paint_layer_t
	{
	public:
		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(u32 max_widgets);
		void uninit();
		void paint_all(const layout_tree_t& tree, const input_router_t& input, vg_canvas_t& canvas);

		// -----------------------------------------------------------------------------
		// widget
		// -----------------------------------------------------------------------------

		paint_def_t&	   def(widget_id_t id);
		const paint_def_t& def_const(widget_id_t id) const;
		void			   clear(widget_id_t id);

		// -----------------------------------------------------------------------------
		// draw
		// -----------------------------------------------------------------------------

		void set_rect(widget_id_t id, const vg_rect_paint_t& p);
		void set_text(widget_id_t id, const char* text, u32 len, const vg_text_paint_t& p);
		void set_custom(widget_id_t id, paint_custom_fn fn, void* user_data);
		void set_hover_color(widget_id_t id, const vec4f_t& c);
		void set_press_color(widget_id_t id, const vec4f_t& c);
		void set_focus_color(widget_id_t id, const vec4f_t& c);

	private:
		vector_t<paint_def_t> _defs;
		vector_t<u8>		  _clip_stack;
	};
}
