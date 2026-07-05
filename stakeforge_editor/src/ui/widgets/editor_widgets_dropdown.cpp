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
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_dropdown_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_dropdown_config_t& config)
	{
		SFG_ASSERT(config.items != nullptr);

		_ui		= &ui;
		_config = config;
		_items.reserve(config.item_count);
		for (u16 i = 0; i < config.item_count; ++i)
			_items.push_back(config.items[i]);
		_config.items = _items.data();

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "dropdown");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_focusable;
		root_in.size_mode_x		 = ui::axis_mode_e::sum_children;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {1.0f, theme.item_height};
		root_in.pos_mode_y		 = config.pos_y == editor_dropdown_pos_y_e::center ? ui::pos_mode_e::relative_in_parent : ui::pos_mode_e::flow;
		root_in.pos_value.y		 = config.pos_y == editor_dropdown_pos_y_e::center ? 0.5f : 0.0f;
		root_in.anchor_y		 = config.pos_y == editor_dropdown_pos_y_e::center ? ui::anchor_e::center : ui::anchor_e::start;
		root_in.flow			 = config.width == editor_dropdown_width_e::sum_children ? ui::flow_e::row : ui::flow_e::none;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		if (config.width == editor_dropdown_width_e::parent_relative)
			root_in.size_mode_x = ui::axis_mode_e::parent_relative;
		else if (config.width == editor_dropdown_width_e::fixed)
		{
			root_in.size_mode_x	 = ui::axis_mode_e::fixed;
			root_in.size_value.x = config.fixed_width;
		}

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.rounding			 = theme.item_rounding;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.outline_color		 = theme.color_panel_light;
		paint.set_rect(_root, rect);
		paint.set_hover_color(_root, theme.color_panel);
		paint.set_press_color(_root, theme.color_frame_light);
		paint.set_focus_color(_root, theme.color_accent0);

		ui::listener_bundle_t root_listener = {};
		root_listener.user_data				= this;
		root_listener.on_click				= on_root_click;
		root_listener.on_key				= on_root_key;
		ui.get_input().set_listener(_root, root_listener);

		_title = ui.allocate_widget();
		ui.set_widget_debug_name(_title, "dropdown_title");
		tree.attach(_root, _title);
		tree.draw_order(_title) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& title_in = tree.in(_title);
		title_in.flags			  = ui::wf_visible;
		title_in.size_mode_x	  = ui::axis_mode_e::fixed;
		title_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		title_in.size_value		  = {0.0f, 1.0f};
		if (config.width != editor_dropdown_width_e::sum_children)
		{
			title_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
			title_in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
			title_in.pos_value	= {0.0f, 0.5f};
			title_in.anchor_y	= ui::anchor_e::center;
		}
		else
		{
			title_in.pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
			title_in.pos_value.y = 0.5f;
			title_in.anchor_y	 = ui::anchor_e::center;
		}

		paint.set_text(_title, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_icon_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_icon_frame, "dropdown_icon");
		tree.attach(_root, _icon_frame);
		tree.draw_order(_icon_frame) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& icon_in = tree.in(_icon_frame);
		icon_in.flags			 = ui::wf_visible;
		icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
		icon_in.size_mode_y		 = ui::axis_mode_e::fixed;
		icon_in.size_value		 = {theme.item_height, theme.item_height};
		if (config.width != editor_dropdown_width_e::sum_children)
		{
			icon_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
			icon_in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
			icon_in.pos_value  = {1.0f, 0.5f};
			icon_in.anchor_x   = ui::anchor_e::end;
			icon_in.anchor_y   = ui::anchor_e::center;
		}
		else
		{
			icon_in.pos_mode_y	= ui::pos_mode_e::relative_in_parent;
			icon_in.pos_value.y = 0.5f;
			icon_in.anchor_y	= ui::anchor_e::center;
		}
		editor_icon_widgets_t::add_icon(ui, _icon_frame, ICON_DD_DOWN, theme.icon_default_px_size, theme.color_text1);

		if (config.field.fields.size > 0)
			update_field_data(config.field);
		else
			refresh_title();
	}

	void editor_dropdown_t::uninit()
	{
		close();
		_ui->deallocate_widget(_root);

		_ui			= nullptr;
		_root		= NULL_WIDGET;
		_title		= NULL_WIDGET;
		_icon_frame = NULL_WIDGET;
		_config		= {};
		_items.resize(0);
		_fields.resize(0);
		_selected_value = 0;
		_mixed			= false;
	}

	void editor_dropdown_t::close()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);
		popup->close_popup();
	}

	void editor_dropdown_t::update_field_data(editor_dropdown_field_t field)
	{
		SFG_ASSERT(field.fields.size > 0);
		SFG_ASSERT(field.fields.data != nullptr);
		SFG_ASSERT(field.field_size == sizeof(u8) || field.field_size == sizeof(u16) || field.field_size == sizeof(u32));

		if (field.fields.data != _fields.data())
		{
			_fields.resize(0);
			_fields.reserve(field.fields.size);
			for (size_t i = 0; i < field.fields.size; ++i)
			{
				SFG_ASSERT(field.fields.data[i] != nullptr);
				_fields.push_back(field.fields.data[i]);
			}
		}
		else
		{
			SFG_ASSERT(field.fields.size == _fields.size());
		}

		_config.field.fields	 = {.data = _fields.data(), .size = _fields.size()};
		_config.field.field_size = field.field_size;
		refresh_field_data();
	}

	void editor_dropdown_t::refresh_field_data()
	{
		SFG_ASSERT(is_field_bound());

		_selected_value = read_field_value(_fields[0]);
		_mixed			= false;
		for (size_t i = 1; i < _fields.size(); ++i)
		{
			if (read_field_value(_fields[i]) != _selected_value)
			{
				_mixed = true;
				break;
			}
		}
		refresh_title();
	}

	void editor_dropdown_t::refresh_title()
	{
		SFG_ASSERT(_ui != nullptr);
		_ui->set_widget_text(_title, get_selected_text());
		const editor_theme_t& theme	   = editor_theme_t::get();
		const bool			  mixed	   = _mixed || (!_config.title_from_selection && _config.title != nullptr && std::strcmp(_config.title, "Mixed") == 0);
		ui::layout_in_t&	  title_in = _ui->get_tree().in(_title);
		title_in.size_value.x		   = static_cast<f32>(_ui->widget_text_len(_title)) * theme.text_default_px_size * 0.7f;
		_ui->get_paint().set_text(_title,
								  _ui->widget_text(_title),
								  _ui->widget_text_len(_title),
								  {.font = theme.font_default, .color = mixed ? theme.color_accent_warn : theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_dropdown_t::set_mixed(bool mixed)
	{
		_mixed						 = mixed;
		_config.title				 = mixed ? "Mixed" : nullptr;
		_config.title_from_selection = !mixed;
		refresh_title();
	}

	bool editor_dropdown_t::is_field_bound() const
	{
		return _config.field.fields.size > 0;
	}

	u16 editor_dropdown_t::read_field_value(const u8* field) const
	{
		switch (_config.field.field_size)
		{
		case sizeof(u8):
			return *reinterpret_cast<const u8*>(field);
		case sizeof(u16):
			return *reinterpret_cast<const u16*>(field);
		case sizeof(u32):
			return static_cast<u16>(*reinterpret_cast<const u32*>(field));
		default:
			SFG_ASSERT(false);
			return 0;
		}
	}

	void editor_dropdown_t::write_field_value(u8* field, u16 value) const
	{
		switch (_config.field.field_size)
		{
		case sizeof(u8):
			*reinterpret_cast<u8*>(field) = static_cast<u8>(value);
			break;
		case sizeof(u16):
			*reinterpret_cast<u16*>(field) = value;
			break;
		case sizeof(u32):
			*reinterpret_cast<u32*>(field) = value;
			break;
		default:
			SFG_ASSERT(false);
			break;
		}
	}

	void editor_dropdown_t::modify_field(u16 value)
	{
		SFG_ASSERT(is_field_bound());
		for (u8* field : _fields)
			write_field_value(field, value);
		refresh_field_data();
		if (_config.callbacks.edited != nullptr)
			_config.callbacks.edited(_config.callbacks.user_data);
	}

	u16 editor_dropdown_t::get_selected() const
	{
		if (is_field_bound())
			return _selected_value;
		return _config.selected != nullptr ? _config.selected(_config.user_data) : (_config.item_count > 0 ? _config.items[0].value : 0);
	}

	const char* editor_dropdown_t::get_selected_text() const
	{
		if (_mixed)
			return "Mixed";

		if (!_config.title_from_selection && _config.title != nullptr)
			return _config.title;

		const u16 selected = get_selected();
		for (u32 i = 0; i < _config.item_count; ++i)
		{
			if (_config.items[i].value == selected)
				return _config.items[i].text;
		}
		return _config.item_count > 0 ? _config.items[0].text : "";
	}

	void editor_dropdown_t::open_popup()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		editor_popup_item_desc_t items[editor_popup_controller_t::MAX_ITEMS] = {};
		SFG_ASSERT(_config.item_count <= editor_popup_controller_t::MAX_ITEMS);
		const u16 selected = get_selected();
		for (u32 i = 0; i < _config.item_count; ++i)
		{
			items[i].text	  = _config.items[i].text;
			items[i].id		  = _config.items[i].value;
			items[i].selected = _config.items[i].value == selected;
		}

		const editor_theme_t&	theme	 = editor_theme_t::get();
		const ui::layout_out_t& root_out = _ui->get_tree().out(_root);
		editor_popup_desc_t		desc	 = {};
		desc.items						 = items;
		desc.item_count					 = _config.item_count;
		desc.pos						 = {root_out.pos.x, root_out.pos.y + root_out.size.y + theme.item_spacing};
		desc.width						 = root_out.size.x;
		desc.pressed					 = on_popup_item_pressed;
		desc.user_data					 = this;
		popup->request_popup(desc);
	}

	void editor_dropdown_t::on_root_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_dropdown_t*>(user_data)->open_popup();
	}

	void editor_dropdown_t::on_root_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press || ev.key != static_cast<u16>(input_code::key_return))
			return;

		static_cast<editor_dropdown_t*>(user_data)->open_popup();
	}

	void editor_dropdown_t::on_popup_item_pressed(u16 value, void* user_data)
	{
		editor_dropdown_t& dropdown = *static_cast<editor_dropdown_t*>(user_data);
		if (dropdown.is_field_bound())
		{
			if (dropdown._config.callbacks.edit_begin != nullptr)
				dropdown._config.callbacks.edit_begin(dropdown._config.callbacks.user_data);
			dropdown.modify_field(value);
			if (dropdown._config.callbacks.edit_submitted != nullptr)
				dropdown._config.callbacks.edit_submitted(dropdown._config.callbacks.user_data);
		}
		else if (dropdown._config.pressed != nullptr)
			dropdown._config.pressed(value, dropdown._config.user_data);
		dropdown.refresh_title();
	}
}
