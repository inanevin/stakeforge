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

#include "ui_context.hpp"

namespace sfg::ui
{
	void ui_context::init(const ui_config_t& cfg)
	{
		_tree.init(cfg.max_widgets);
		_paint.init(cfg.max_widgets);
		_input.init(cfg.input);
		_canvas.init(cfg.canvas);
		_text_pool.init(cfg.text_pool_capacity);
	}

	void ui_context::uninit()
	{
		_widget_texts.clear();
		_text_pool.uninit();
		_canvas.uninit();
		_input.uninit();
		_paint.uninit();
		_tree.uninit();
	}

	void ui_context::tick(const vec4f_t& screen_rect, f32 dt_seconds)
	{
		_tree.solve(screen_rect);
		_input.tick(_tree, dt_seconds);

		_canvas.frame_begin(screen_rect);
		_paint.paint_all(_tree, _input, _canvas);
		_canvas.frame_end();
	}

	void ui_context::on_mouse_move(const vec2f_t& pos)
	{
		_input.on_mouse_move(pos);
	}

	void ui_context::on_mouse_button(mouse_button_e btn, bool pressed)
	{
		_input.on_mouse_button(btn, pressed);
	}

	void ui_context::on_wheel(f32 delta)
	{
		_input.on_wheel(delta);
	}

	void ui_context::on_key(const key_event_t& ev)
	{
		_input.on_key(ev);
	}

	void ui_context::set_widget_text(widget_id_t id, const char* text, u32 len)
	{
		auto& e = _widget_texts[id];
		if (e.ptr != nullptr)
			_text_pool.deallocate(e.ptr);
		e.ptr = _text_pool.allocate(text, len);
		e.len = len;
	}

	void ui_context::clear_widget_text(widget_id_t id)
	{
		auto it = _widget_texts.find(id);
		if (it == _widget_texts.end())
			return;
		if (it->second.ptr != nullptr)
			_text_pool.deallocate(it->second.ptr);
		_widget_texts.erase(it);
	}

	const char* ui_context::widget_text(widget_id_t id) const
	{
		auto it = _widget_texts.find(id);
		if (it == _widget_texts.end())
			return nullptr;
		return it->second.ptr;
	}

	u32 ui_context::widget_text_len(widget_id_t id) const
	{
		auto it = _widget_texts.find(id);
		if (it == _widget_texts.end())
			return 0;
		return it->second.len;
	}

	widget_id_t ui_context::make_panel(widget_id_t parent)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in	 = _tree.in(id);
		in.size_mode_x	 = axis_mode_e::parent_relative;
		in.size_mode_y	 = axis_mode_e::parent_relative;
		in.size_value	 = {1.0f, 1.0f};
		in.child_margins = {_theme.margin_vertical, _theme.margin_horizontal, _theme.margin_vertical, _theme.margin_horizontal};
		in.child_spacing = _theme.item_spacing;

		vg_rect_paint_t rect   = {};
		rect.fill_color_a	   = _theme.color_panel_bg;
		rect.fill_color_b	   = _theme.color_panel_bg;
		rect.outline_color	   = _theme.color_item_outline;
		rect.outline_thickness = _theme.outline_thickness;
		_paint.set_rect(id, rect);

		return id;
	}

	widget_id_t ui_context::make_row(widget_id_t parent)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in	 = _tree.in(id);
		in.size_mode_x	 = axis_mode_e::parent_relative;
		in.size_mode_y	 = axis_mode_e::sum_children;
		in.size_value	 = {1.0f, 0.0f};
		in.flow			 = flow_e::row;
		in.child_spacing = _theme.item_spacing;
		return id;
	}

	widget_id_t ui_context::make_column(widget_id_t parent)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in	 = _tree.in(id);
		in.size_mode_x	 = axis_mode_e::parent_relative;
		in.size_mode_y	 = axis_mode_e::parent_relative;
		in.size_value	 = {1.0f, 1.0f};
		in.flow			 = flow_e::column;
		in.child_spacing = _theme.item_spacing;
		return id;
	}

	widget_id_t ui_context::make_spacer(widget_id_t parent, f32 size_px)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in = _tree.in(id);
		if (size_px > 0.0f)
		{
			in.size_mode_x = axis_mode_e::fixed;
			in.size_mode_y = axis_mode_e::fixed;
			in.size_value  = {size_px, size_px};
		}
		else
		{
			in.size_mode_x = axis_mode_e::fill;
			in.size_mode_y = axis_mode_e::fill;
		}
		return id;
	}

	widget_id_t ui_context::make_label(widget_id_t parent, const char* text, font_runtime_t* font)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		const u32 len = static_cast<u32>(text ? strlen(text) : 0);
		set_widget_text(id, text, len);

		layout_in_t& in = _tree.in(id);
		in.size_mode_x	= axis_mode_e::sum_children;
		in.size_mode_y	= axis_mode_e::fixed;
		in.size_value	= {0.0f, _theme.item_height};

		const vg_text_paint_t tp = {.font = font, .color = _theme.color_item_fg, .scale = 1.0f, .spacing = 0};
		const vec2f_t		  m	 = vg_canvas_t::measure_text(text, len, tp);
		in.size_mode_x			 = axis_mode_e::fixed;
		in.size_value.x			 = m.x;
		in.size_value.y			 = m.y > 0.0f ? m.y : _theme.item_height;

		_paint.set_text(id, widget_text(id), widget_text_len(id), tp);
		return id;
	}

	widget_id_t ui_context::make_button(widget_id_t parent, const char* text, font_runtime_t* font)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		const u32 len = static_cast<u32>(text ? strlen(text) : 0);
		set_widget_text(id, text, len);

		layout_in_t& in	 = _tree.in(id);
		in.size_mode_x	 = axis_mode_e::sum_children;
		in.size_mode_y	 = axis_mode_e::fixed;
		in.size_value	 = {0.0f, _theme.item_height};
		in.flags		 = wf_visible | wf_focusable;
		in.child_margins = {_theme.margin_vertical, _theme.margin_horizontal * 2, _theme.margin_vertical, _theme.margin_horizontal * 2};
		in.flow			 = flow_e::row;

		vg_rect_paint_t rect   = {};
		rect.fill_color_a	   = _theme.color_item_bg;
		rect.fill_color_b	   = _theme.color_item_bg;
		rect.outline_color	   = _theme.color_item_outline;
		rect.outline_thickness = _theme.outline_thickness;
		_paint.set_rect(id, rect);
		_paint.set_hover_color(id, _theme.color_item_hover);
		_paint.set_press_color(id, _theme.color_item_press);
		_paint.set_focus_color(id, _theme.color_focus);

		const widget_id_t lbl = _tree.allocate();
		_tree.attach(id, lbl);

		const vg_text_paint_t tp = {.font = font, .color = _theme.color_item_fg, .scale = 1.0f, .spacing = 0};
		const vec2f_t		  m	 = vg_canvas_t::measure_text(text, len, tp);

		layout_in_t& lin = _tree.in(lbl);
		lin.size_mode_x	 = axis_mode_e::fixed;
		lin.size_mode_y	 = axis_mode_e::fixed;
		lin.size_value	 = {m.x, m.y > 0.0f ? m.y : _theme.item_height};
		lin.flags		 = wf_visible | wf_no_input;
		_paint.set_text(lbl, widget_text(id), widget_text_len(id), tp);

		return id;
	}

	widget_id_t ui_context::make_divider(widget_id_t parent, bool horizontal)
	{
		const widget_id_t id = _tree.allocate();
		_tree.attach(parent, id);

		layout_in_t& in = _tree.in(id);
		if (horizontal)
		{
			in.size_mode_x = axis_mode_e::parent_relative;
			in.size_mode_y = axis_mode_e::fixed;
			in.size_value  = {1.0f, 1.0f};
		}
		else
		{
			in.size_mode_x = axis_mode_e::fixed;
			in.size_mode_y = axis_mode_e::parent_relative;
			in.size_value  = {1.0f, 1.0f};
		}

		vg_rect_paint_t rect = {};
		rect.fill_color_a	 = _theme.color_divider;
		rect.fill_color_b	 = _theme.color_divider;
		_paint.set_rect(id, rect);
		return id;
	}
}
