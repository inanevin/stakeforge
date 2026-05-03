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

#include "paint.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/ui/input/input_router.hpp>
#include <sfg/ui/layout/layout_tree.hpp>

namespace sfg::ui
{
	void paint_layer_t::init(u32 max_widgets)
	{
		_defs.resize(max_widgets);
		_clip_stack.reserve(64);
	}

	void paint_layer_t::uninit()
	{
		_defs.resize(0);
		_clip_stack.resize(0);
	}

	paint_def_t& paint_layer_t::def(widget_id_t id)
	{
		SFG_ASSERT(id < _defs.size());
		return _defs[id];
	}

	const paint_def_t& paint_layer_t::def_const(widget_id_t id) const
	{
		SFG_ASSERT(id < _defs.size());
		return _defs[id];
	}

	void paint_layer_t::clear(widget_id_t id)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id] = {};
	}

	void paint_layer_t::set_rect(widget_id_t id, const vg_rect_paint_t& p)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].kind = paint_kind_e::rect;
		_defs[id].rect = p;
	}

	void paint_layer_t::set_text(widget_id_t id, const char* text, u32 len, const vg_text_paint_t& p)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].kind		= paint_kind_e::text;
		_defs[id].text		= p;
		_defs[id].text_data = text;
		_defs[id].text_len	= len;
	}

	void paint_layer_t::set_custom(widget_id_t id, paint_custom_fn fn, void* user_data)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].kind		= paint_kind_e::custom;
		_defs[id].custom_fn = fn;
		_defs[id].custom_ud = user_data;
	}

	void paint_layer_t::set_hover_color(widget_id_t id, const vec4f_t& c)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].hover_color = c;
		_defs[id].state_flags |= psf_has_hover;
	}

	void paint_layer_t::set_press_color(widget_id_t id, const vec4f_t& c)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].press_color = c;
		_defs[id].state_flags |= psf_has_press;
	}

	void paint_layer_t::set_focus_color(widget_id_t id, const vec4f_t& c)
	{
		SFG_ASSERT(id < _defs.size());
		_defs[id].focus_color = c;
		_defs[id].state_flags |= psf_has_focus;
	}

	void paint_layer_t::paint_all(const layout_tree_t& tree, const input_router_t& input, vg_canvas_t& canvas)
	{
		const auto dfs = tree.get_dfs();

		_clip_stack.resize(0);

		const widget_id_t get_hovered = input.get_hovered();
		const widget_id_t get_focused = input.get_focused();
		const widget_id_t pressed_l	  = input.is_pressed(mouse_button_e::left);

		for (size_t i = 0; i < dfs.size; ++i)
		{
			const widget_id_t id = dfs.data[i];
			const u8		  d	 = tree.node(id).depth;

			while (!_clip_stack.empty() && _clip_stack.back() >= d)
			{
				_clip_stack.pop_back();
				canvas.pop_clip();
			}

			const layout_in_t&	in = tree.in_const(id);
			const layout_out_t& o  = tree.out(id);

			if (id == tree.get_root())
			{
				if (in.flags & wf_clip_children)
				{
					canvas.push_clip({o.pos.x, o.pos.y, o.size.x, o.size.y});
					_clip_stack.push_back(d);
				}
				continue;
			}

			if (!(in.flags & wf_visible) || o.clip.z <= 0.0f || o.clip.w <= 0.0f)
				continue;

			const paint_def_t& pd		  = _defs[id];
			const u32		   draw_order = tree.draw_order_const(id);

			vec4f_t override_color = {0, 0, 0, 0};
			bool	has_override   = false;
			if ((pd.state_flags & psf_has_press) && pressed_l == id)
			{
				override_color = pd.press_color;
				has_override   = true;
			}
			else if ((pd.state_flags & psf_has_hover) && get_hovered == id)
			{
				override_color = pd.hover_color;
				has_override   = true;
			}

			if (pd.kind == paint_kind_e::rect)
			{
				vg_rect_paint_t paint = pd.rect;
				if (has_override)
				{
					paint.fill_color_a = override_color;
					paint.fill_color_b = override_color;
					paint.gradient	   = vg_gradient_e::none;
				}
				if ((pd.state_flags & psf_has_focus) && get_focused == id)
				{
					paint.outline_color = pd.focus_color;
					if (paint.outline_thickness <= 0.0f)
						paint.outline_thickness = 1.0f;
				}
				canvas.add_rect({o.pos.x, o.pos.y}, {o.pos.x + o.size.x, o.pos.y + o.size.y}, paint, draw_order, nullptr);
			}
			else if (pd.kind == paint_kind_e::text && pd.text_data != nullptr && pd.text_len > 0)
			{
				vg_text_paint_t paint = pd.text;
				if (has_override)
					paint.color = override_color;
				canvas.add_text(pd.text_data, pd.text_len, {o.pos.x, o.pos.y}, paint, draw_order, nullptr);
			}
			else if (pd.kind == paint_kind_e::custom && pd.custom_fn)
			{
				pd.custom_fn(*this, id, canvas, pd.custom_ud);
			}

			if (in.flags & wf_clip_children)
			{
				canvas.push_clip({o.pos.x, o.pos.y, o.size.x, o.size.y});
				_clip_stack.push_back(d);
			}
		}

		while (!_clip_stack.empty())
		{
			_clip_stack.pop_back();
			canvas.pop_clip();
		}
	}
}
