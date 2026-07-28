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
#include "ui/widgets/editor_widget_color_wheel.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_color_wheel_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_color_wheel_config_t& config)
	{
		_ui							= &ui;
		_config						= config;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "editor_widget_color_wheel");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = 0;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;

		const ui::widget_id_t top_pane	  = make_pane(_root, "color_wheel_top_pane", ui::flow_e::row, ui::axis_mode_e::parent_relative, ui::axis_mode_e::parent_relative, {1.0f, 0.6f}, 0.0f);
		const ui::widget_id_t bottom_pane = make_pane(_root, "color_wheel_bottom_pane", ui::flow_e::row, ui::axis_mode_e::parent_relative, ui::axis_mode_e::parent_relative, {1.0f, 0.4f}, 0.0f);

		const ui::widget_id_t top_left_pane		= make_pane(top_pane, "color_wheel_top_pane_left", ui::flow_e::none, ui::axis_mode_e::parent_relative, ui::axis_mode_e::parent_relative, {0.6f, 1.0f}, 0.0f);
		const ui::widget_id_t top_right_pane	= make_pane(top_pane, "color_wheel_top_pane_right", ui::flow_e::row, ui::axis_mode_e::fill, ui::axis_mode_e::parent_relative, {1.0f, 1.0f}, theme.item_spacing);
		const ui::widget_id_t bottom_left_pane	= make_pane(bottom_pane, "color_wheel_bottom_pane_left", ui::flow_e::column, ui::axis_mode_e::parent_relative, ui::axis_mode_e::parent_relative, {0.5f, 1.0f}, 0.0f);
		const ui::widget_id_t bottom_right_pane = make_pane(bottom_pane, "color_wheel_bottom_pane_right", ui::flow_e::column, ui::axis_mode_e::fill, ui::axis_mode_e::parent_relative, {1.0f, 1.0f}, 0.0f);

		make_top_left_frame(top_left_pane);
		make_top_right_frame(top_right_pane, 0);
		make_top_right_frame(top_right_pane, 1);
		make_number_row(bottom_left_pane, 0, 0, "R");
		make_number_row(bottom_left_pane, 1, 1, "G");
		make_number_row(bottom_left_pane, 2, 2, "B");
		make_number_row(bottom_left_pane, 3, 3, "A");
		make_number_row(bottom_left_pane, EXPOSURE_ROW, EXPOSURE_FIELD, "EV");
		make_number_row(bottom_right_pane, 4, 4, "H");
		make_number_row(bottom_right_pane, 5, 5, "S");
		make_number_row(bottom_right_pane, 6, 6, "V");
		make_text_row(bottom_right_pane, HEX_ROW, "Hex");

		update_config(config);
	}

	f32 editor_widget_color_wheel_t::calculate_min_height(bool hdr)
	{
		const editor_theme_t& theme		  = editor_theme_t::get();
		const f32			  bottom_pane = theme.item_area_height * (hdr ? 5.0f : 4.0f) + theme.margin_vertical * 2.0f;
		const f32			  top_pane	  = bottom_pane * 1.5f;
		return bottom_pane + top_pane;
	}

	void editor_widget_color_wheel_t::uninit()
	{
		for (editor_input_field_t& input : _inputs)
			input.uninit();
		_ui->deallocate_widget(_root);

		_ui				 = nullptr;
		_root			 = NULL_WIDGET;
		_top_left_frame	 = NULL_WIDGET;
		_top_left_handle = NULL_WIDGET;
		_exposure_row	 = NULL_WIDGET;
		_exposure_label	 = NULL_WIDGET;

		for (ui::widget_id_t& frame : _top_right_frames)
			frame = NULL_WIDGET;

		for (ui::widget_id_t& handle : _top_right_handles)
			handle = NULL_WIDGET;

		_fields.resize(0);
		_config		   = {};
		_display_color = color_t::white;
		_exposure	   = 0.0f;

		for (f32& value : _top_right_values)
			value = 0.0f;

		for (color_t& color : _top_right_base_colors)
			color = color_t::white;

		for (f32& value : _number_values)
			value = 0.0f;

		_hex_value[0] = '\0';
	}

	void editor_widget_color_wheel_t::update_config(const editor_color_wheel_config_t& config)
	{
		_config.edit_begin		= config.edit_begin;
		_config.on_data_changed = config.on_data_changed;
		_config.user_data		= config.user_data;
		_config.hdr				= config.hdr;

		ui::layout_in_t& exposure_row_in = _ui->get_tree().in(_exposure_row);
		exposure_row_in.size_value.y	 = config.hdr ? editor_theme_t::get().item_area_height : 0.0f;
		_ui->get_tree().set_visible(_exposure_row, config.hdr);
		_ui->get_tree().set_visible(_exposure_label, config.hdr);
		_inputs[EXPOSURE_ROW].set_visible(config.hdr);

		update_field_data(config.field);
	}

	void editor_widget_color_wheel_t::update_field_data(editor_color_wheel_field_t field)
	{
		if (field.fields.data != _fields.data())
			_fields.assign(field.fields.data, field.fields.data + field.fields.size);

		field.fields  = {.data = _fields.data(), .size = _fields.size()};
		_config.field = field;

		const color_t& linear_color	  = *field.fields.data[0];
		color_t		   display_linear = linear_color;
		_exposure					  = 0.0f;

		if (_config.hdr)
		{
			const f32 max_channel = math::max(linear_color.x, math::max(linear_color.y, linear_color.z));

			if (max_channel > 1.0f)
			{
				_exposure = std::log2(max_channel);
				display_linear.x /= max_channel;
				display_linear.y /= max_channel;
				display_linear.z /= max_channel;
			}
		}

		_display_color	 = color_utils_t::linear_to_srgb(display_linear);
		_display_color.w = linear_color.w;

		for (u32 i = 0; i < TOP_RIGHT_FRAME_COUNT; ++i)
		{
			_top_right_values[i]	  = 0.0f;
			_top_right_base_colors[i] = _display_color;
		}

		update_displays(true, false);
	}

	void editor_widget_color_wheel_t::refresh_field_data()
	{
		update_field_data(_config.field);
	}

	ui::widget_id_t editor_widget_color_wheel_t::make_pane(ui::widget_id_t parent, const char* debug_name, ui::flow_e flow, ui::axis_mode_e size_mode_x, ui::axis_mode_e size_mode_y, const vec2f_t& size_value, f32 child_spacing)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::widget_id_t pane = _ui->allocate_widget();
		_ui->set_widget_debug_name(pane, debug_name);
		tree.attach(parent, pane);

		ui::layout_in_t& pane_in = tree.in(pane);
		pane_in.pos_mode_x		 = ui::pos_mode_e::flow;
		pane_in.pos_mode_y		 = ui::pos_mode_e::flow;
		pane_in.size_mode_x		 = size_mode_x;
		pane_in.size_mode_y		 = size_mode_y;
		pane_in.size_value		 = size_value;
		pane_in.flow			 = flow;
		pane_in.child_spacing	 = child_spacing;
		pane_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		return pane;
	}

	void editor_widget_color_wheel_t::make_top_left_frame(ui::widget_id_t parent)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_top_left_frame = _ui->allocate_widget();
		_ui->set_widget_debug_name(_top_left_frame, "color_wheel_top_left_frame");
		tree.attach(parent, _top_left_frame);
		tree.draw_order(_top_left_frame) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& in = tree.in(_top_left_frame);
		in.flags			= ui::wf_visible | ui::wf_input;
		in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		in.pos_value		= {0.5f, 0.5f};
		in.anchor_x			= ui::anchor_e::center;
		in.anchor_y			= ui::anchor_e::center;
		in.size_mode_x		= ui::axis_mode_e::copy_other;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= {1.0f, 1.0f};

		ui::vg_rect_paint_t rect	= {};
		rect.fill_color_a			= {0.0f, 0.0f, 0.0f, 1.0f};
		rect.fill_color_b			= {0.0f, 0.0f, 0.0f, 1.0f};
		ui::ui_render_state_t state = {};
		state.pipeline				= "editor/resource_pack/shaders/editor_ui_color_wheel.hlsl"_hs;
		paint.set_rect(_top_left_frame, rect, state);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_press			   = on_top_left_frame_press;
		listener.on_drag			   = on_top_left_frame_drag;
		_ui->get_input().set_listener(_top_left_frame, listener);

		_top_left_handle = _ui->allocate_widget();
		_ui->set_widget_debug_name(_top_left_handle, "color_wheel_top_left_handle");
		tree.attach(_top_left_frame, _top_left_handle);
		tree.draw_order(_top_left_handle) = tree.draw_order_const(_top_left_frame) + 1;

		ui::layout_in_t& handle_in = tree.in(_top_left_handle);
		handle_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		handle_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		handle_in.pos_value		   = {0.5f, 0.5f};
		handle_in.anchor_x		   = ui::anchor_e::center;
		handle_in.anchor_y		   = ui::anchor_e::center;

		_ui->set_widget_text(_top_left_handle, ICON_FILLED_CIRCLE);
		paint.set_text(_top_left_handle,
					   _ui->widget_text(_top_left_handle),
					   _ui->widget_text_len(_top_left_handle),
					   {.font = theme.font_icons, .color = theme.color_accent0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_widget_color_wheel_t::make_top_right_frame(ui::widget_id_t parent, u32 frame)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		const ui::widget_id_t id = _ui->allocate_widget();
		_ui->set_widget_debug_name(id, "color_wheel_top_right_frame");
		tree.attach(parent, id);

		ui::layout_in_t& in = tree.in(id);
		in.flags			= ui::wf_visible | ui::wf_input;
		in.pos_mode_x		= ui::pos_mode_e::flow;
		in.pos_mode_y		= ui::pos_mode_e::flow;
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= {theme.item_area_height, 1.0f};
		tree.draw_order(id) = tree.draw_order_const(parent) + 1;

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_press			   = on_top_right_frame_press;
		listener.on_drag			   = on_top_right_frame_drag;
		_ui->get_input().set_listener(id, listener);

		const ui::widget_id_t handle = _ui->allocate_widget();
		_ui->set_widget_debug_name(handle, "color_wheel_top_right_handle");
		tree.attach(id, handle);
		tree.draw_order(handle) = tree.draw_order_const(id) + 1;

		ui::layout_in_t& handle_in = tree.in(handle);
		handle_in.flags			   = ui::wf_visible;
		handle_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		handle_in.pos_value.y	   = 0.0f;
		handle_in.anchor_y		   = ui::anchor_e::center;
		handle_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		handle_in.size_mode_y	   = ui::axis_mode_e::fixed;
		handle_in.size_value	   = {1.0f, theme.border_thickness};

		ui::vg_rect_paint_t handle_rect = {};
		handle_rect.fill_color_a		= theme.color_outline_light;
		handle_rect.fill_color_b		= theme.color_outline_light;
		_ui->get_paint().set_rect(handle, handle_rect);

		_top_right_frames[frame]  = id;
		_top_right_handles[frame] = handle;
	}

	void editor_widget_color_wheel_t::make_number_row(ui::widget_id_t parent, u32 row, u32 field, const char* label)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		const ui::widget_id_t row_widget = _ui->allocate_widget();
		_ui->set_widget_debug_name(row_widget, "color_wheel_channel_row");
		tree.attach(parent, row_widget);

		if (field == EXPOSURE_FIELD)
			_exposure_row = row_widget;

		ui::layout_in_t& row_in = tree.in(row_widget);
		row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		row_in.size_mode_y		= ui::axis_mode_e::fixed;
		row_in.size_value		= {1.0f, theme.item_area_height};
		row_in.flow				= ui::flow_e::row;
		row_in.child_spacing	= theme.item_spacing;
		row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		const ui::widget_id_t label_widget = _ui->allocate_widget();
		_ui->set_widget_debug_name(label_widget, "color_wheel_channel_label");
		tree.attach(row_widget, label_widget);

		if (field == EXPOSURE_FIELD)
			_exposure_label = label_widget;

		ui::layout_in_t& label_in = tree.in(label_widget);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fixed;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {theme.item_height * 1.5f, theme.item_height};
		_ui->set_widget_text(label_widget, label);
		paint.set_text(label_widget,
					   _ui->widget_text(label_widget),
					   _ui->widget_text_len(label_widget),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		u8*							input_field	 = reinterpret_cast<u8*>(&_number_values[field]);
		editor_input_field_config_t input_config = {};
		input_config.field						 = {
			.fields		= {.data = &input_field, .size = 1},
			.field_size = sizeof(f32),
			.type		= editor_input_field_field_type_e::pod_number,
		};

		const bool is_exposure				  = field == EXPOSURE_FIELD;
		input_config.field.is_slider		  = !is_exposure;
		input_config.increment				  = is_exposure ? 0.1f : 0.01f;
		input_config.min_value				  = is_exposure ? -15.0f : 0.0f;
		input_config.max_value				  = is_exposure ? 15.0f : 1.0f;
		input_config.callbacks.edited		  = field < 4 ? on_rgba_changed : (is_exposure ? on_exposure_changed : on_hsv_changed);
		input_config.callbacks.edit_submitted = on_input_edit_submitted;
		input_config.callbacks.user_data	  = this;
		_inputs[row].init(*_ui, row_widget, input_config);

		ui::layout_in_t& input_in				 = tree.in(_inputs[row].get_root());
		input_in.pos_mode_y						 = ui::pos_mode_e::relative_in_parent;
		input_in.pos_value.y					 = 0.5f;
		input_in.anchor_y						 = ui::anchor_e::center;
		input_in.size_mode_x					 = ui::axis_mode_e::fill;
		input_in.size_mode_y					 = ui::axis_mode_e::fixed;
		input_in.size_value						 = {1.0f, theme.item_height};
		tree.draw_order(_inputs[row].get_root()) = tree.draw_order_const(parent) + 1;
	}

	void editor_widget_color_wheel_t::make_text_row(ui::widget_id_t parent, u32 row, const char* label)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		const ui::widget_id_t row_widget = _ui->allocate_widget();
		_ui->set_widget_debug_name(row_widget, "color_wheel_channel_row");
		tree.attach(parent, row_widget);

		ui::layout_in_t& row_in = tree.in(row_widget);
		row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		row_in.size_mode_y		= ui::axis_mode_e::fixed;
		row_in.size_value		= {1.0f, theme.item_area_height};
		row_in.flow				= ui::flow_e::row;
		row_in.child_spacing	= theme.item_spacing;
		row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		const ui::widget_id_t label_widget = _ui->allocate_widget();
		_ui->set_widget_debug_name(label_widget, "color_wheel_channel_label");
		tree.attach(row_widget, label_widget);

		ui::layout_in_t& label_in = tree.in(label_widget);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fixed;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {theme.item_height * 1.5f, theme.item_height};
		_ui->set_widget_text(label_widget, label);
		paint.set_text(label_widget,
					   _ui->widget_text(label_widget),
					   _ui->widget_text_len(label_widget),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		u8*							input_field	 = reinterpret_cast<u8*>(_hex_value);
		editor_input_field_config_t input_config = {};
		input_config.field						 = {
			.fields		= {.data = &input_field, .size = 1},
			.field_size = HEX_TEXT_CAPACITY,
			.type		= editor_input_field_field_type_e::char_array,
		};
		input_config.callbacks.edited		  = on_hex_changed;
		input_config.callbacks.edit_submitted = on_input_edit_submitted;
		input_config.callbacks.user_data	  = this;
		_inputs[row].init(*_ui, row_widget, input_config);

		ui::layout_in_t& input_in				 = tree.in(_inputs[row].get_root());
		input_in.pos_mode_y						 = ui::pos_mode_e::relative_in_parent;
		input_in.pos_value.y					 = 0.5f;
		input_in.anchor_y						 = ui::anchor_e::center;
		input_in.size_mode_x					 = ui::axis_mode_e::fill;
		input_in.size_mode_y					 = ui::axis_mode_e::fixed;
		input_in.size_value						 = {1.0f, theme.item_height};
		tree.draw_order(_inputs[row].get_root()) = tree.draw_order_const(parent) + 1;
	}

	void editor_widget_color_wheel_t::modify_field()
	{
		if (_config.edit_begin != nullptr)
			_config.edit_begin(_config.user_data);

		color_t linear_color = color_utils_t::srgb_to_linear(_display_color);
		linear_color.w		 = _display_color.w;

		if (_config.hdr)
		{
			const double multiplier = math::pow(2.0, static_cast<double>(_exposure));
			const double max_value	= static_cast<double>(std::numeric_limits<f32>::max());
			linear_color.x			= static_cast<f32>(math::min(static_cast<double>(linear_color.x) * multiplier, max_value));
			linear_color.y			= static_cast<f32>(math::min(static_cast<double>(linear_color.y) * multiplier, max_value));
			linear_color.z			= static_cast<f32>(math::min(static_cast<double>(linear_color.z) * multiplier, max_value));
		}

		for (size_t i = 0; i < _config.field.fields.size; ++i)
			*_config.field.fields.data[i] = linear_color;

		if (_config.on_data_changed != nullptr)
			_config.on_data_changed(_config.user_data);
	}

	void editor_widget_color_wheel_t::update_displays(bool apply_wheel, bool preserve_focused_input)
	{
		_number_values[0] = _display_color.x;
		_number_values[1] = _display_color.y;
		_number_values[2] = _display_color.z;
		_number_values[3] = _display_color.w;

		const color_t hsv			   = color_utils_t::srgb_to_hsv(_display_color);
		_number_values[4]			   = hsv.x / 360.0f;
		_number_values[5]			   = hsv.y;
		_number_values[6]			   = hsv.z;
		_number_values[EXPOSURE_FIELD] = _exposure;
		color_utils_t::to_hex(_display_color, _hex_value, sizeof(_hex_value));

		if (apply_wheel)
		{
			ui::layout_in_t& top_left_handle_in = _ui->get_tree().in(_top_left_handle);
			const f32		 top_left_angle		= _number_values[4] * MATH_TWO_PI;
			top_left_handle_in.pos_value.x		= math::cos(top_left_angle) * _number_values[5] * 0.5f + 0.5f;
			top_left_handle_in.pos_value.y		= -math::sin(top_left_angle) * _number_values[5] * 0.5f + 0.5f;
		}

		const ui::widget_id_t focused_input = _ui->get_input().get_focused();

		for (editor_input_field_t& input : _inputs)
		{
			if (preserve_focused_input && input.get_root() == focused_input)
				continue;

			input.refresh_field_data();
		}

		for (u32 i = 0; i < TOP_RIGHT_FRAME_COUNT; ++i)
			_ui->get_tree().in(_top_right_handles[i]).pos_value.y = _top_right_values[i];

		for (u32 i = 0; i < TOP_RIGHT_FRAME_COUNT; ++i)
		{
			color_t linear_base_color = color_utils_t::srgb_to_linear(_top_right_base_colors[i]);
			linear_base_color.w		  = _top_right_base_colors[i].w;

			ui::vg_rect_paint_t value = {};
			value.fill_color_a		  = linear_base_color.to_vector();
			value.fill_color_b		  = i == 0 ? vec4f_t{1.0f, 1.0f, 1.0f, linear_base_color.w} : vec4f_t{0.0f, 0.0f, 0.0f, linear_base_color.w};
			value.gradient			  = ui::vg_gradient_e::vertical;
			_ui->get_paint().set_rect(_top_right_frames[i], value);
		}
	}

	void editor_widget_color_wheel_t::on_rgba_changed(void* user_data)
	{
		editor_widget_color_wheel_t& wheel = *static_cast<editor_widget_color_wheel_t*>(user_data);
		wheel._display_color			   = {wheel._number_values[0], wheel._number_values[1], wheel._number_values[2], wheel._number_values[3]};

		for (u32 i = 0; i < TOP_RIGHT_FRAME_COUNT; ++i)
		{
			wheel._top_right_values[i]		= 0.0f;
			wheel._top_right_base_colors[i] = wheel._display_color;
		}

		wheel.modify_field();
		wheel.update_displays(true, true);
	}

	void editor_widget_color_wheel_t::on_hsv_changed(void* user_data)
	{
		editor_widget_color_wheel_t& wheel = *static_cast<editor_widget_color_wheel_t*>(user_data);
		const color_t				 color = color_utils_t::hsv_to_srgb({wheel._number_values[4] * 360.0f, wheel._number_values[5], wheel._number_values[6], wheel._display_color.w});
		wheel._display_color			   = color;

		for (u32 i = 0; i < TOP_RIGHT_FRAME_COUNT; ++i)
		{
			wheel._top_right_values[i]		= 0.0f;
			wheel._top_right_base_colors[i] = wheel._display_color;
		}

		wheel.modify_field();
		wheel.update_displays(true, true);
	}

	void editor_widget_color_wheel_t::on_exposure_changed(void* user_data)
	{
		editor_widget_color_wheel_t& wheel = *static_cast<editor_widget_color_wheel_t*>(user_data);
		wheel._exposure					   = math::clamp(wheel._number_values[EXPOSURE_FIELD], -15.0f, 15.0f);
		wheel.modify_field();
		wheel.update_displays(true, true);
	}

	void editor_widget_color_wheel_t::on_hex_changed(void* user_data)
	{
		editor_widget_color_wheel_t& wheel = *static_cast<editor_widget_color_wheel_t*>(user_data);
		if (wheel._hex_value[0] != '#' || wheel._hex_value[1] == '\0' || wheel._hex_value[2] == '\0' || wheel._hex_value[3] == '\0' || wheel._hex_value[4] == '\0' || wheel._hex_value[5] == '\0' || wheel._hex_value[6] == '\0' || wheel._hex_value[7] != '\0')
			return;

		wheel._display_color = color_utils_t::from_hex(wheel._hex_value);

		for (u32 i = 0; i < TOP_RIGHT_FRAME_COUNT; ++i)
		{
			wheel._top_right_values[i]		= 0.0f;
			wheel._top_right_base_colors[i] = wheel._display_color;
		}

		wheel.modify_field();
		wheel.update_displays(true, true);
	}

	void editor_widget_color_wheel_t::on_input_edit_submitted(void* user_data)
	{
		editor_widget_color_wheel_t& wheel = *static_cast<editor_widget_color_wheel_t*>(user_data);
		wheel.update_displays(true, false);
	}

	void editor_widget_color_wheel_t::apply_top_left_wheel(const vec2f_t& pos)
	{
		const ui::layout_out_t& out = _ui->get_tree().out(_top_left_frame);
		const f32				x	= out.size.x > 0.0f ? math::clamp((pos.x - out.pos.x) / out.size.x, 0.0f, 1.0f) * 2.0f - 1.0f : 0.0f;
		const f32				y	= out.size.y > 0.0f ? math::clamp((pos.y - out.pos.y) / out.size.y, 0.0f, 1.0f) * 2.0f - 1.0f : 0.0f;
		const f32				r	= math::sqrt(x * x + y * y);
		const f32				s	= math::min(r, 1.0f);
		const color_t			hsv = color_utils_t::srgb_to_hsv(_display_color);
		f32						h	= hsv.x;
		if (r > MATH_EPS)
		{
			h = std::atan2(-y, x) * RAD_2_DEG;
			if (h < 0.0f)
				h += 360.0f;
		}

		ui::layout_in_t& top_left_handle_in = _ui->get_tree().in(_top_left_handle);
		const f32		 inv_r				= r > MATH_EPS ? 1.0f / r : 0.0f;
		top_left_handle_in.pos_value.x		= x * inv_r * s * 0.5f + 0.5f;
		top_left_handle_in.pos_value.y		= y * inv_r * s * 0.5f + 0.5f;

		_display_color = color_utils_t::hsv_to_srgb({h, s, 1.0f, _display_color.w});

		for (u32 i = 0; i < TOP_RIGHT_FRAME_COUNT; ++i)
		{
			_top_right_values[i]	  = 0.0f;
			_top_right_base_colors[i] = _display_color;
		}

		modify_field();
		update_displays(false, false);
	}

	void editor_widget_color_wheel_t::apply_top_right_slider(ui::widget_id_t id, const vec2f_t& pos)
	{
		u32 index = TOP_RIGHT_FRAME_COUNT;
		for (u32 i = 0; i < TOP_RIGHT_FRAME_COUNT; ++i)
		{
			if (_top_right_frames[i] == id)
			{
				index = i;
				break;
			}
		}

		const ui::layout_out_t& out = _ui->get_tree().out(id);
		const f32				t	= out.size.y > 0.0f ? math::clamp((pos.y - out.pos.y) / out.size.y, 0.0f, 1.0f) : 0.0f;
		_top_right_values[index]	= t;

		_display_color	 = index == 0 ? color_utils_t::lerp(_top_right_base_colors[index], color_t::white, t) : color_utils_t::lerp(_top_right_base_colors[index], color_t::black, t);
		_display_color.w = _top_right_base_colors[index].w;

		const u32 other_index				= index == 0 ? 1 : 0;
		_top_right_values[other_index]		= 0.0f;
		_top_right_base_colors[other_index] = _display_color;

		modify_field();
		update_displays(true, false);
	}

	void editor_widget_color_wheel_t::on_top_left_frame_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_widget_color_wheel_t*>(user_data)->apply_top_left_wheel(pos);
	}

	void editor_widget_color_wheel_t::on_top_left_frame_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) == id)
			static_cast<editor_widget_color_wheel_t*>(user_data)->apply_top_left_wheel(pos);
	}

	void editor_widget_color_wheel_t::on_top_right_frame_press(ui::input_router_t&, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_widget_color_wheel_t*>(user_data)->apply_top_right_slider(id, pos);
	}

	void editor_widget_color_wheel_t::on_top_right_frame_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_widget_color_wheel_t& wheel = *static_cast<editor_widget_color_wheel_t*>(user_data);
		if (router.is_pressed(ui::mouse_button_e::left) == id)
			wheel.apply_top_right_slider(id, pos);
	}

}
