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
#include "ui/widgets/editor_split_border.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
	void editor_split_border_t::init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui		= &ui;
		_config = config;
		_root	= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "editor_split_border");
		ui.get_tree().attach(parent, _root);

		ui::layout_in_t& in = ui.get_tree().in(_root);
		in.flags			= ui::wf_visible | ui::wf_input;

		ui::listener_bundle_t listener = {};
		listener.on_hover_enter		   = on_hover_enter;
		listener.on_hover_exit		   = on_hover_exit;
		listener.on_hover_move		   = on_hover_move;
		listener.on_drag_begin		   = on_drag_begin;
		listener.on_drag			   = on_drag;
		listener.on_drag_end		   = on_drag_end;
		listener.user_data			   = this;
		ui.get_input().set_listener(_root, listener);
		ui.get_paint().set_custom(_root, draw, this);
	}

	void editor_split_border_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui			 = nullptr;
		_root		 = NULL_WIDGET;
		_config		 = {};
		_is_dragging = false;
	}

	void editor_split_border_t::apply_drag(const vec2f_t& pos, const vec2f_t& delta)
	{
		if (_config.on_drag != nullptr)
			_config.on_drag(*this, pos, delta, _config.user_data);
	}

	void editor_split_border_t::set_resize_cursor() const
	{
		const window_cursor_state_e cursor = _config.direction == editor_split_border_direction_e::horizontal ? window_cursor_state_e::resize_hr : window_cursor_state_e::resize_vt;
		process::set_cursor_state(cursor);
	}

	void editor_split_border_t::on_hover_enter(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		editor_split_border_t& border = *static_cast<editor_split_border_t*>(user_data);
		border.set_resize_cursor();
	}

	void editor_split_border_t::on_hover_exit(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		editor_split_border_t& border = *static_cast<editor_split_border_t*>(user_data);
		if (!border._is_dragging)
			process::set_cursor_state(window_cursor_state_e::arrow);
	}

	void editor_split_border_t::on_hover_move(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		editor_split_border_t& border = *static_cast<editor_split_border_t*>(user_data);
		border.set_resize_cursor();
	}

	void editor_split_border_t::on_drag_begin(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_split_border_t& border = *static_cast<editor_split_border_t*>(user_data);
		border._is_dragging			  = true;
		border.set_resize_cursor();
		border.apply_drag(pos, delta);
	}

	void editor_split_border_t::on_drag(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_split_border_t& border = *static_cast<editor_split_border_t*>(user_data);
		border.set_resize_cursor();
		border.apply_drag(pos, delta);
	}

	void editor_split_border_t::on_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_split_border_t& border = *static_cast<editor_split_border_t*>(user_data);
		border.apply_drag(pos, delta);
		border._is_dragging = false;
		if (router.get_hovered() == id)
			border.set_resize_cursor();
		else
			process::set_cursor_state(window_cursor_state_e::arrow);
	}

	void editor_split_border_t::draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const editor_split_border_t& border = *static_cast<editor_split_border_t*>(user_data);
		const editor_theme_t&		 theme	= editor_theme_t::get();
		const ui::layout_out_t&		 out	= border._ui->get_tree().out(id);

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = border._is_dragging || border._ui->get_input().get_hovered() == id ? theme.color_frame_light : theme.color_frame;
		rect.fill_color_b		 = rect.fill_color_a;

		ui::ui_render_state_t state = {};
		state.pipeline				= paint.get_pipelines().default_pipeline;
		canvas.add_rect({out.pos.x, out.pos.y}, {out.pos.x + out.size.x, out.pos.y + out.size.y}, rect, state, border._ui->get_tree().draw_order_const(id));
	}
}
