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
#include "ui/widgets/editor_widget_window_frame.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_window_frame_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_window_frame_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "window_frame");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {1.0f, theme.item_height};
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, 0.0f, theme.margin_horizontal};

		ui::vg_rect_paint_t root_rect = {};
		root_rect.fill_color_a		  = theme.color_frame_light;
		root_rect.fill_color_b		  = theme.color_frame_light;
		paint.set_rect(_root, root_rect);

		const ui::widget_id_t title = ui.allocate_widget();
		ui.set_widget_debug_name(title, "window_frame_title");
		tree.attach(_root, title);

		ui::layout_in_t& title_text_in = tree.in(title);
		title_text_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		title_text_in.pos_value.y	   = 0.5f;
		title_text_in.anchor_y		   = ui::anchor_e::center;
		title_text_in.size_mode_x	   = ui::axis_mode_e::max_children;
		title_text_in.size_mode_y	   = ui::axis_mode_e::max_children;

		ui.set_widget_text(title, _config.title);
		ui::ui_render_state_t title_state = {};
		title_state.pipeline			  = theme.shader_glitch_lcd;
		paint.set_text(title, ui.widget_text(title), ui.widget_text_len(title), {.font = theme.font_title, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::lcd}, title_state);

		const ui::widget_id_t spacer = ui.allocate_widget();
		ui.set_widget_debug_name(spacer, "window_frame_spacer");
		tree.attach(_root, spacer);

		ui::layout_in_t& spacer_in = tree.in(spacer);
		spacer_in.size_mode_x	   = ui::axis_mode_e::fill;
		spacer_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		spacer_in.size_value	   = {1.0f, 1.0f};

		_window_buttons = ui.allocate_widget();
		ui.set_widget_debug_name(_window_buttons, "window_frame_buttons");
		tree.attach(_root, _window_buttons);

		ui::layout_in_t& buttons_in = tree.in(_window_buttons);
		buttons_in.size_mode_x		= ui::axis_mode_e::fixed;
		buttons_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		buttons_in.size_value		= {theme.item_height * (_config.only_close ? 2.0f : 6.0f), 1.0f};
		buttons_in.flow				= ui::flow_e::row;
		buttons_in.child_spacing	= 0.0f;
		buttons_in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

		const editor_window_buttons_t buttons =
			editor_misc_widgets_t::add_window_buttons(ui, _window_buttons, theme.color_frame_light, theme.color_accent_err, theme.color_light, theme.color_panel, theme.color_text0, theme.icon_default_px_size, {.only_close = _config.only_close});

		_minimize_frame = buttons.minimize_frame;
		_maximize_frame = buttons.maximize_frame;
		_close_frame	= buttons.close_frame;

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		if (!_config.only_close)
		{
			listener.on_click = on_minimize_window;
			ui.get_input().set_listener(_minimize_frame, listener);
			listener.on_click = on_maximize_window;
			ui.get_input().set_listener(_maximize_frame, listener);
		}
		listener.on_click = on_close_window;
		ui.get_input().set_listener(_close_frame, listener);
	}

	void editor_widget_window_frame_t::uninit()
	{
		if (!_config.only_close)
		{
			_ui->get_input().clear_listener(_minimize_frame);
			_ui->get_input().clear_listener(_maximize_frame);
		}
		_ui->get_input().clear_listener(_close_frame);
		_ui->deallocate_widget(_root);

		_config			= {};
		_ui				= nullptr;
		_root			= NULL_WIDGET;
		_window_buttons = NULL_WIDGET;
		_minimize_frame = NULL_WIDGET;
		_maximize_frame = NULL_WIDGET;
		_close_frame	= NULL_WIDGET;
	}

	bool editor_widget_window_frame_t::is_window_drag_region(const vec2i16_t& pos) const
	{
		const vec2f_t p = {static_cast<f32>(pos.x), static_cast<f32>(pos.y)};

		const vec4f_t buttons = _ui->get_tree().bounds(_window_buttons);
		if (p.x >= buttons.x && p.x <= buttons.x + buttons.z && p.y >= buttons.y && p.y <= buttons.y + buttons.w)
			return false;

		const vec4f_t title = _ui->get_tree().bounds(_root);
		return p.x >= title.x && p.x <= title.x + title.z && p.y >= title.y && p.y <= title.y + title.w;
	}

	void editor_widget_window_frame_t::on_minimize_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_window_frame_t& frame = *static_cast<editor_widget_window_frame_t*>(user_data);
		process::minimize_window(frame._config.runtime->window_handle);
	}

	void editor_widget_window_frame_t::on_maximize_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_window_frame_t& frame = *static_cast<editor_widget_window_frame_t*>(user_data);
		process::toggle_maximize_window(frame._config.runtime->window_handle);
	}

	void editor_widget_window_frame_t::on_close_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_window_frame_t& frame = *static_cast<editor_widget_window_frame_t*>(user_data);
		frame._config.runtime->set_flag(window_runtime_flags_e::close_requested);
	}
}
