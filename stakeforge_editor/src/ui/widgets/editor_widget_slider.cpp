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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include "ui/widgets/editor_widget_slider.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_slider_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_slider_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "editor_slider_base");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = config.fixed_width ? ui::axis_mode_e::fixed : ui::axis_mode_e::fill;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {config.fixed_width ? config.width : 1.0f, theme.item_height};
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = theme.item_spacing;
		root_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_value.y		 = 0.5f;
		root_in.anchor_y		 = ui::anchor_e::center;

		_slider = ui.allocate_widget();
		ui.set_widget_debug_name(_slider, "editor_slider_slider");
		tree.attach(_root, _slider);

		ui::layout_in_t& slider_in = tree.in(_slider);
		slider_in.flags			   = ui::wf_visible | ui::wf_input;
		slider_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		slider_in.pos_value.y	   = 0.5f;
		slider_in.anchor_y		   = ui::anchor_e::center;
		slider_in.size_mode_x	   = ui::axis_mode_e::fill;
		slider_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		slider_in.size_value	   = {1.0f, 1.0f};

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_press			   = on_press;
		listener.on_drag			   = on_drag;
		ui.get_input().set_listener(_slider, listener);

		_bg = ui.allocate_widget();
		ui.set_widget_debug_name(_bg, "editor_slider_bg");
		tree.attach(_slider, _bg);

		ui::layout_in_t& bg_in = tree.in(_bg);
		bg_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		bg_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		bg_in.pos_value		   = {0.0f, 0.5f};
		bg_in.anchor_y		   = ui::anchor_e::center;
		bg_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		bg_in.size_mode_y	   = ui::axis_mode_e::fixed;
		bg_in.size_value	   = {1.0f, theme.border_thickness};

		ui::vg_rect_paint_t bg_rect = {};
		bg_rect.fill_color_a		= theme.color_frame;
		bg_rect.fill_color_b		= theme.color_frame;
		paint.set_rect(_bg, bg_rect);

		_icon = ui.allocate_widget();
		ui.set_widget_debug_name(_icon, "editor_slider_icon");
		tree.attach(_slider, _icon);

		ui::layout_in_t& icon_in = tree.in(_icon);
		icon_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		icon_in.pos_value.y		 = 0.5f;
		icon_in.anchor_x		 = ui::anchor_e::center;
		icon_in.anchor_y		 = ui::anchor_e::center;
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::fixed;

		ui.set_widget_text(_icon, ICON_FILLED_CIRCLE);
		paint.set_text(
			_icon, ui.widget_text(_icon), ui.widget_text_len(_icon), {.font = theme.font_icons, .color = theme.color_accent0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		paint.set_hover_color(_icon, theme.color_accent0_light);
		paint.set_press_color(_icon, theme.color_accent0_light);
		paint.set_state_source(_icon, _slider);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "editor_slider_label");
		tree.attach(_root, _label);

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.flags			  = config.display_label ? ui::wf_visible : 0;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fixed;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;

		ui.set_widget_text(_label, "");
		paint.set_text(
			_label, ui.widget_text(_label), ui.widget_text_len(_label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		update_field_data(config.field);
	}

	void editor_slider_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui			   = nullptr;
		_root		   = NULL_WIDGET;
		_slider		   = NULL_WIDGET;
		_bg			   = NULL_WIDGET;
		_icon		   = NULL_WIDGET;
		_label		   = NULL_WIDGET;
		_config		   = {};
		_label_text[0] = '\0';
		_value		   = 0.0f;
		_mixed		   = false;
	}

	void editor_slider_t::set_value(f32 value)
	{
		_value = math::clamp(value, _config.min_value, _config.max_value);
		_mixed = false;
		modify_field();
		refresh();
	}

	void editor_slider_t::update_field_data(editor_slider_field_t field)
	{
		_config.field = field;
		_value		  = math::clamp(*field.fields.data[0], _config.min_value, _config.max_value);
		_mixed		  = false;
		for (size_t i = 1; i < field.fields.size; ++i)
		{
			if (*field.fields.data[i] != *field.fields.data[0])
			{
				_mixed = true;
				break;
			}
		}
		refresh();
	}

	void editor_slider_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	void editor_slider_t::refresh()
	{
		const f32 range						  = _config.max_value - _config.min_value;
		const f32 t							  = range > 0.0f ? math::clamp((_value - _config.min_value) / range, 0.0f, 1.0f) : 0.0f;
		_ui->get_tree().in(_icon).pos_value.x = t;
		refresh_label();
	}

	void editor_slider_t::refresh_label()
	{
		if (!_config.display_label)
			return;

		char text[sizeof(_label_text)] = {};
		if (_mixed)
			snprintf(text, sizeof(text), "Mixed");
		else
		{
			const u32 decimals = math::min(static_cast<u32>(_config.decimal_count), 6u);
			snprintf(text, sizeof(text), "%.*f", static_cast<int>(decimals), _value);
		}

		if (strcmp(_label_text, text) == 0)
			return;

		memcpy(_label_text, text, sizeof(_label_text));
		_ui->set_widget_text(_label, _label_text);

		const editor_theme_t& theme				= editor_theme_t::get();
		_ui->get_paint().def(_label).text.color = _mixed ? theme.color_accent_warn : theme.color_text0;
	}

	void editor_slider_t::set_value_from_pos(const vec2f_t& pos)
	{
		const ui::layout_out_t& out = _ui->get_tree().out(_slider);
		const f32				t	= out.size.x > 0.0f ? math::clamp((pos.x - out.pos.x) / out.size.x, 0.0f, 1.0f) : 0.0f;
		_value						= _config.min_value + (_config.max_value - _config.min_value) * t;
		_mixed						= false;
		modify_field();
		refresh();
	}

	void editor_slider_t::modify_field()
	{
		for (size_t i = 0; i < _config.field.fields.size; ++i)
			*_config.field.fields.data[i] = _value;
	}

	void editor_slider_t::on_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_slider_t*>(user_data)->set_value_from_pos(pos);
	}

	void editor_slider_t::on_drag(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		static_cast<editor_slider_t*>(user_data)->set_value_from_pos(pos);
	}
}
