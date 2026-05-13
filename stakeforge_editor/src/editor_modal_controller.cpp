// Copyright (c) 2025 Inan Evin

#include "editor_modal_controller.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_buttons.hpp"
#include "widgets/editor_widgets_frames.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		constexpr u32 MODAL_DRAW_ORDER = 60000u;
		constexpr f32 MODAL_WIDTH	   = 480.0f;

		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			ui::layout_in_t& in = tree.in(id);
			in.flags			= visible ? static_cast<u8>(ui::wf_visible | (input ? ui::wf_input : 0)) : static_cast<u8>(ui::wf_overlay);
		}

		vec4f_t get_title_color(editor_modal_severity_e severity)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			switch (severity)
			{
			case editor_modal_severity_e::error:
				return theme.color_accent_err;
			case editor_modal_severity_e::warning:
				return theme.color_accent_warn;
			default:
				return theme.color_accent1;
			}
		}
	}

	void editor_modal_controller_t::handle_button_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		static_cast<editor_modal_controller_t*>(user_data)->on_button_click(id, btn);
	}

	void editor_modal_controller_t::init(ui::ui_context& ui)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_foreground = tree.allocate();
		ui.set_widget_debug_name(_foreground, "modal_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = MODAL_DRAW_ORDER;

		ui::layout_in_t& foreground_in = tree.in(_foreground);
		foreground_in.flags			   = ui::wf_overlay;
		foreground_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_value	   = {1.0f, 1.0f};

		_dimmer = tree.allocate();
		ui.set_widget_debug_name(_dimmer, "modal_dimmer");
		tree.attach(_foreground, _dimmer);
		tree.draw_order(_dimmer) = MODAL_DRAW_ORDER + 1;

		ui::layout_in_t& dimmer_in = tree.in(_dimmer);
		dimmer_in.flags			   = ui::wf_overlay;
		dimmer_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		dimmer_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		dimmer_in.size_value	   = {1.0f, 1.0f};

		ui::vg_rect_paint_t dimmer_rect = {};
		dimmer_rect.fill_color_a		= {0.0f, 0.0f, 0.0f, 0.5f};
		dimmer_rect.fill_color_b		= {0.0f, 0.0f, 0.0f, 0.5f};
		paint.set_rect(_dimmer, dimmer_rect);

		_window = tree.allocate();
		ui.set_widget_debug_name(_window, "modal_window");
		tree.attach(_foreground, _window);
		tree.draw_order(_window) = MODAL_DRAW_ORDER + 2;

		ui::layout_in_t& window_in = tree.in(_window);
		window_in.flags			   = ui::wf_overlay;
		window_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		window_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		window_in.pos_value		   = {0.5f, 0.5f};
		window_in.anchor_x		   = ui::anchor_e::center;
		window_in.anchor_y		   = ui::anchor_e::center;
		window_in.size_mode_x	   = ui::axis_mode_e::fixed;
		window_in.size_mode_y	   = ui::axis_mode_e::sum_children;
		window_in.size_value	   = {MODAL_WIDTH, 0.0f};
		window_in.flow			   = ui::flow_e::column;
		window_in.child_spacing	   = theme.margin_vertical * 2.0f;
		window_in.child_margins	   = {theme.margin_vertical * 4.0f, theme.margin_horizontal * 3.0f, theme.margin_vertical * 3.0f, theme.margin_horizontal * 3.0f};

		editor_widgets_frames_t::make_frame_modal(ui, _window);

		_title = tree.allocate();
		ui.set_widget_debug_name(_title, "modal_title");
		tree.attach(_window, _title);
		tree.draw_order(_title) = MODAL_DRAW_ORDER + 3;
		paint.set_text(_title, nullptr, 0, {.font = theme.font_title, .color = theme.color_accent1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_description = tree.allocate();
		ui.set_widget_debug_name(_description, "modal_description");
		tree.attach(_window, _description);
		tree.draw_order(_description) = MODAL_DRAW_ORDER + 3;
		paint.set_text(_description, nullptr, 0, {.font = theme.font_default, .color = theme.color_fg1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_button_row = tree.allocate();
		ui.set_widget_debug_name(_button_row, "modal_button_row");
		tree.attach(_window, _button_row);
		tree.draw_order(_button_row) = MODAL_DRAW_ORDER + 3;

		ui::layout_in_t& button_row_in = tree.in(_button_row);
		button_row_in.flags			   = ui::wf_overlay;
		button_row_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		button_row_in.size_mode_y	   = ui::axis_mode_e::fixed;
		button_row_in.size_value	   = {1.0f, theme.item_height};
		button_row_in.flow			   = ui::flow_e::row;
		button_row_in.child_spacing	   = theme.margin_horizontal;

		ui::listener_bundle_t button_listener = {};
		button_listener.user_data			  = this;
		button_listener.on_click			  = handle_button_click;

		for (u32 i = 0; i < MAX_BUTTONS; ++i)
		{
			_button_frames[i] = tree.allocate();
			ui.set_widget_debug_name(_button_frames[i], "modal_button");
			tree.attach(_button_row, _button_frames[i]);
			tree.draw_order(_button_frames[i]) = MODAL_DRAW_ORDER + 4;

			ui::layout_in_t& button_in = tree.in(_button_frames[i]);
			button_in.flags			   = ui::wf_overlay;
			button_in.size_mode_x	   = ui::axis_mode_e::fill;
			button_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
			button_in.size_value	   = {1.0f, 1.0f};

			editor_widgets_buttons_t::make_button_modal(ui, _button_frames[i]);
			ui.get_input().set_listener(_button_frames[i], button_listener);

			_button_labels[i] = tree.allocate();
			ui.set_widget_debug_name(_button_labels[i], "modal_button_label");
			tree.attach(_button_frames[i], _button_labels[i]);
			tree.draw_order(_button_labels[i]) = MODAL_DRAW_ORDER + 5;

			ui::layout_in_t& label_in = tree.in(_button_labels[i]);
			label_in.flags			  = ui::wf_overlay;
			label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value		  = {0.5f, 0.5f};
			label_in.anchor_x		  = ui::anchor_e::center;
			label_in.anchor_y		  = ui::anchor_e::center;
			paint.set_text(_button_labels[i], nullptr, 0, {.font = theme.font_default, .color = theme.color_fg3, .point_size = theme.text_small_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		set_visible(false);
	}

	void editor_modal_controller_t::uninit()
	{
		if (_ui == nullptr)
			return;

		if (_visible)
			set_visible(false);

		ui::ui_context&		ui	  = *_ui;
		ui::input_router_t& input = ui.get_input();
		for (u32 i = 0; i < MAX_BUTTONS; ++i)
		{
			input.clear_listener(_button_frames[i]);
			ui.clear_widget_text(_button_labels[i]);
			ui.clear_widget_debug_name(_button_frames[i]);
			ui.clear_widget_debug_name(_button_labels[i]);
		}

		ui.clear_widget_text(_title);
		ui.clear_widget_text(_description);
		ui.clear_widget_debug_name(_foreground);
		ui.clear_widget_debug_name(_dimmer);
		ui.clear_widget_debug_name(_window);
		ui.clear_widget_debug_name(_title);
		ui.clear_widget_debug_name(_description);
		ui.clear_widget_debug_name(_button_row);

		_ui			  = nullptr;
		_foreground	  = NULL_WIDGET;
		_dimmer		  = NULL_WIDGET;
		_window		  = NULL_WIDGET;
		_title		  = NULL_WIDGET;
		_description  = NULL_WIDGET;
		_button_row	  = NULL_WIDGET;
		_button_count = 0;
		_visible	  = false;
		for (u32 i = 0; i < MAX_BUTTONS; ++i)
		{
			_button_frames[i] = NULL_WIDGET;
			_button_labels[i] = NULL_WIDGET;
			_buttons[i]		  = {};
		}
	}

	void editor_modal_controller_t::request_modal(const char* title, const char* description, const editor_modal_button_desc_t* buttons, u16 button_count, editor_modal_severity_e severity)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(button_count <= MAX_BUTTONS);

		_button_count = button_count;
		for (u32 i = 0; i < MAX_BUTTONS; ++i)
			_buttons[i] = i < button_count ? buttons[i] : editor_modal_button_desc_t{};

		_ui->set_widget_text(_title, title);
		_ui->set_widget_text(_description, description);
		_ui->get_paint().def(_title).text.color = get_title_color(severity);

		for (u32 i = 0; i < MAX_BUTTONS; ++i)
		{
			const bool visible = i < button_count;
			set_widget_visible(_ui->get_tree(), _button_frames[i], visible, visible);
			set_widget_visible(_ui->get_tree(), _button_labels[i], visible, false);
			if (visible)
				_ui->set_widget_text(_button_labels[i], buttons[i].text);
			else
				_ui->clear_widget_text(_button_labels[i]);
		}

		set_visible(true);
	}

	void editor_modal_controller_t::close_modal()
	{
		if (_ui == nullptr || !_visible)
			return;
		set_visible(false);
		_button_count = 0;
		for (editor_modal_button_desc_t& button : _buttons)
			button = {};
	}

	void editor_modal_controller_t::set_visible(bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();

		_visible = visible;
		set_widget_visible(tree, _foreground, visible, false);
		set_widget_visible(tree, _dimmer, visible, true);
		set_widget_visible(tree, _window, visible, false);
		set_widget_visible(tree, _title, visible, false);
		set_widget_visible(tree, _description, visible, false);
		set_widget_visible(tree, _button_row, visible, false);
		for (u32 i = 0; i < MAX_BUTTONS; ++i)
		{
			const bool button_visible = visible && i < _button_count;
			set_widget_visible(tree, _button_frames[i], button_visible, button_visible);
			set_widget_visible(tree, _button_labels[i], button_visible, false);
		}
	}

	void editor_modal_controller_t::on_button_click(ui::widget_id_t id, ui::mouse_button_e btn)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		const u32				   index  = find_button_index(id);
		editor_modal_button_desc_t button = _buttons[index];
		close_modal();
		if (button.callback)
			button.callback(button.user_data);
	}

	u32 editor_modal_controller_t::find_button_index(ui::widget_id_t id) const
	{
		for (u32 i = 0; i < _button_count; ++i)
		{
			if (_button_frames[i] == id)
				return i;
		}
		SFG_ASSERT(false);
		return 0;
	}
}
