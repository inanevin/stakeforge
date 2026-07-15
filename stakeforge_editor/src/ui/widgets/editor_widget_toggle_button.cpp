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

#include "ui/widgets/editor_widget_toggle_button.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_toggle_button_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_toggle_button_config_t& config)
	{
		_ui			= &ui;
		_config		= config;
		_is_toggled = config.is_toggled;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "toggle_button");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {config.width, theme.item_height};
		root_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_value.y		 = 0.5f;
		root_in.anchor_y		 = ui::anchor_e::center;

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_click;
		ui.get_input().set_listener(_root, listener);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "toggle_button_label");
		tree.attach(_root, _label);

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.5f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;

		refresh();
	}

	void editor_widget_toggle_button_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_config		= {};
		_ui			= nullptr;
		_root		= NULL_WIDGET;
		_label		= NULL_WIDGET;
		_is_toggled = false;
	}

	void editor_widget_toggle_button_t::set_toggled(bool is_toggled)
	{
		_is_toggled = is_toggled;
		refresh();
	}

	void editor_widget_toggle_button_t::refresh()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::paint_layer_t&	  paint = _ui->get_paint();

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = _is_toggled ? _config.toggled_frame_color : _config.frame_color;
		rect.fill_color_b		 = rect.fill_color_a;
		rect.outline_color		 = _is_toggled ? _config.toggled_outline_color : _config.outline_color;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.rounding			 = theme.item_rounding;
		rect.rounding_segs		 = 4;
		paint.set_rect(_root, rect);
		paint.set_hover_color(_root, _is_toggled ? _config.toggled_hover_color : _config.hover_color);
		paint.set_press_color(_root, _config.pressed_color);

		_ui->set_widget_text(_label, _is_toggled ? _config.toggled_text : _config.text);
		paint.set_text(_label,
					   _ui->widget_text(_label),
					   _ui->widget_text_len(_label),
					   {.font = theme.font_default, .color = _is_toggled ? _config.toggled_text_color : _config.text_color, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_widget_toggle_button_t::on_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_toggle_button_t& button = *static_cast<editor_widget_toggle_button_t*>(user_data);
		button._is_toggled					  = !button._is_toggled;
		button.refresh();
		if (button._config.on_toggle != nullptr)
			button._config.on_toggle(button._is_toggled, button._config.user_data);
	}
}
