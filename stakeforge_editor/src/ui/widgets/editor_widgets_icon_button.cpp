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
#include "ui/widgets/editor_widgets_icon_button.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_icon_button_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_icon_button_config_t& config)
	{
		_ui		  = &ui;
		_config	  = config;
		_toggled  = config.toggled;
		_disabled = false;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "icon_button");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		root_in.size_mode_x		 = ui::axis_mode_e::copy_other;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {config.size, config.size};
		root_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_value.y		 = 0.5f;
		root_in.anchor_y		 = ui::anchor_e::center;

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_click;
		ui.get_input().set_listener(_root, listener);

		_icon = ui.allocate_widget();
		ui.set_widget_debug_name(_icon, "icon_button_icon");
		tree.attach(_root, _icon);
		tree.draw_order(_icon) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& icon_in = tree.in(_icon);
		icon_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		icon_in.pos_value		 = {0.5f, 0.5f};
		icon_in.anchor_x		 = ui::anchor_e::center;
		icon_in.anchor_y		 = ui::anchor_e::center;

		if (config.tooltip != nullptr)
		{
			editor_tooltip_desc_t tooltip = {};
			tooltip.text				  = config.tooltip;
			editor_tooltip_controller_t::find(ui)->set_tooltip(_root, tooltip);
		}

		refresh();
	}

	void editor_icon_button_t::uninit()
	{
		if (_config.tooltip != nullptr)
			editor_tooltip_controller_t::find(*_ui)->clear_tooltip(_root);

		_ui->deallocate_widget(_root);

		_ui		  = nullptr;
		_root	  = NULL_WIDGET;
		_icon	  = NULL_WIDGET;
		_config	  = {};
		_toggled  = false;
		_disabled = false;
	}

	void editor_icon_button_t::set_toggled(bool toggled)
	{
		_toggled = toggled;
		refresh();
	}

	void editor_icon_button_t::set_disabled(bool disabled)
	{
		_disabled = disabled;

		ui::layout_in_t& root_in = _ui->get_tree().in(_root);
		if (_disabled)
			root_in.flags |= ui::wf_disabled;
		else
			root_in.flags &= ~ui::wf_disabled;

		refresh();
	}

	void editor_icon_button_t::refresh()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::paint_layer_t&	  paint = _ui->get_paint();

		ui::vg_rect_paint_t rect	= {};
		const bool			toggled = _config.toggle_enabled && _toggled;
		const vec4f_t		color	= toggled ? _config.toggled_frame_color : _config.frame_color;
		rect.fill_color_a			= color;
		rect.fill_color_b			= color;
		rect.rounding				= _config.rounding;
		rect.outline_color			= toggled ? _config.toggled_outline_color : _config.outline_color;
		rect.outline_thickness		= theme.outline_thickness;

		paint.set_rect(_root, rect);
		paint.set_hover_color(_root, toggled ? _config.toggled_hover_color : _config.hover_color);
		paint.set_press_color(_root, _config.press_color);

		_ui->set_widget_text(_icon, get_icon());
		paint.set_text(_icon, _ui->widget_text(_icon), _ui->widget_text_len(_icon), {.font = theme.font_icons, .color = _config.icon_color, .point_size = _config.icon_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		paint.set_disabled_color(_icon, _config.disabled_color);
		paint.set_state_source(_icon, _root);
	}

	const char* editor_icon_button_t::get_icon() const
	{
		if (_config.toggle_enabled && _toggled && _config.toggled_icon != nullptr)
			return _config.toggled_icon;
		return _config.icon;
	}

	void editor_icon_button_t::on_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_icon_button_t& button = *static_cast<editor_icon_button_t*>(user_data);
		if (button._disabled)
			return;

		if (button._config.toggle_enabled)
		{
			button._toggled = !button._toggled;
			button.refresh();
		}
		if (button._config.on_clicked != nullptr)
			button._config.on_clicked(button._toggled, button._config.user_data);
	}
}
