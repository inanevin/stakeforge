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
#include "ui/editor_modal_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_buttons.hpp"
#include "ui/widgets/editor_widgets_frames.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		constexpr u32 MODAL_FG_DRAW_ORDER = 60000u;
		constexpr u32 MODAL_DRAW_ORDER	  = 60001u;

		editor_modal_controller_t* s_controllers[editor_modal_controller_t::MAX_CONTROLLERS] = {};
		u32						   s_controller_count										 = 0;

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
		SFG_ASSERT(s_controller_count < MAX_CONTROLLERS);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_foreground = ui.allocate_widget();
		ui.set_widget_debug_name(_foreground, "modal_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = MODAL_DRAW_ORDER;

		ui::layout_in_t& foreground_in = tree.in(_foreground);
		foreground_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_value	   = {1.0f, 1.0f};

		const ui::widget_id_t dimmer = ui.allocate_widget();
		ui.set_widget_debug_name(dimmer, "modal_dimmer");
		tree.attach(_foreground, dimmer);
		tree.draw_order(dimmer) = MODAL_FG_DRAW_ORDER;

		ui::layout_in_t& dimmer_in = tree.in(dimmer);
		dimmer_in.flags			   = ui::wf_visible | ui::wf_input;
		dimmer_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		dimmer_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		dimmer_in.size_value	   = {1.0f, 1.0f};

		ui::vg_rect_paint_t dimmer_rect = {};
		dimmer_rect.fill_color_a		= {0.0f, 0.0f, 0.0f, 0.5f};
		dimmer_rect.fill_color_b		= {0.0f, 0.0f, 0.0f, 0.5f};
		paint.set_rect(dimmer, dimmer_rect);

		_window = ui.allocate_widget();
		ui.set_widget_debug_name(_window, "modal_window");
		tree.attach(_foreground, _window);

		ui::layout_in_t& window_in = tree.in(_window);
		window_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		window_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		window_in.pos_value		   = {0.5f, 0.5f};
		window_in.anchor_x		   = ui::anchor_e::center;
		window_in.anchor_y		   = ui::anchor_e::center;
		window_in.size_mode_x	   = ui::axis_mode_e::max_children;
		window_in.size_mode_y	   = ui::axis_mode_e::sum_children;
		window_in.flow			   = ui::flow_e::column;
		window_in.child_spacing	   = theme.margin_vertical * 2.0f;
		window_in.child_margins	   = {theme.margin_vertical * 4.0f, theme.margin_horizontal * 3.0f, theme.margin_vertical * 3.0f, theme.margin_horizontal * 3.0f};

		editor_widgets_frames_t::make_frame_modal(ui, _window);

		_title = ui.allocate_widget();
		ui.set_widget_debug_name(_title, "modal_title");
		tree.attach(_window, _title);
		tree.draw_order(_title) = MODAL_DRAW_ORDER + 3;
		paint.set_text(_title, nullptr, 0, {.font = theme.font_title, .color = theme.color_accent1, .point_size = theme.text_big_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_description = ui.allocate_widget();
		ui.set_widget_debug_name(_description, "modal_description");
		tree.attach(_window, _description);
		paint.set_text(_description, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_container = ui.allocate_widget();
		ui.set_widget_debug_name(_container, "modal_container");
		tree.attach(_window, _container);

		ui::layout_in_t& container_in = tree.in(_container);
		container_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		container_in.pos_mode_y		  = ui::pos_mode_e::flow;
		container_in.pos_value.x	  = 0.0f;
		container_in.size_mode_x	  = ui::axis_mode_e::max_children;
		container_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		container_in.size_value.x	  = 1.0f;
		container_in.flow			  = ui::flow_e::column;
		container_in.child_spacing	  = theme.item_spacing;

		_button_row = ui.allocate_widget();
		ui.set_widget_debug_name(_button_row, "modal_button_row");
		tree.attach(_window, _button_row);

		ui::layout_in_t& button_row_in = tree.in(_button_row);
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
			_button_frames[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_button_frames[i], "modal_button");
			tree.attach(_button_row, _button_frames[i]);

			_button_labels[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_button_labels[i], "modal_button_label");
			tree.attach(_button_frames[i], _button_labels[i]);

			editor_widgets_buttons_t::make_button_modal(ui, _button_frames[i], _button_labels[i]);
			ui.get_input().set_listener(_button_frames[i], button_listener);
		}

		s_controllers[s_controller_count++] = this;
		set_visible(false);
	}

	void editor_modal_controller_t::uninit()
	{
		if (_visible)
			close_modal();
		else
			close_content();

		_ui->deallocate_widget(_foreground);

		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i] == this)
			{
				s_controllers[i]					  = s_controllers[s_controller_count - 1];
				s_controllers[s_controller_count - 1] = nullptr;
				s_controller_count--;
				break;
			}
		}

		_ui				 = nullptr;
		_foreground		 = NULL_WIDGET;
		_window			 = NULL_WIDGET;
		_title			 = NULL_WIDGET;
		_description	 = NULL_WIDGET;
		_container		 = NULL_WIDGET;
		_button_row		 = NULL_WIDGET;
		_button_count	 = 0;
		_buttons_visible = false;
		_visible		 = false;
		for (u32 i = 0; i < MAX_BUTTONS; ++i)
		{
			_button_frames[i] = NULL_WIDGET;
			_button_labels[i] = NULL_WIDGET;
			_buttons[i]		  = {};
		}
	}

	void editor_modal_controller_t::request_modal(const char* title, const char* description, const editor_modal_button_desc_t* buttons, u16 button_count, editor_modal_severity_e severity)
	{
		request_modal(title, description, true, buttons, button_count, nullptr, severity);
	}

	void editor_modal_controller_t::request_modal(const char* title, const char* description, bool show_buttons, const editor_modal_button_desc_t* buttons, u16 button_count, const editor_modal_content_desc_t* content, editor_modal_severity_e severity)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(button_count <= MAX_BUTTONS);
		SFG_ASSERT(buttons != nullptr || button_count == 0);

		close_content();

		_button_count	 = button_count;
		_buttons_visible = show_buttons;
		for (u32 i = 0; i < MAX_BUTTONS; ++i)
			_buttons[i] = i < button_count ? buttons[i] : editor_modal_button_desc_t{};

		_ui->set_widget_text(_title, title);
		_ui->set_widget_text(_description, description);
		_ui->get_paint().def(_title).text.color = get_title_color(severity);

		for (u32 i = 0; i < MAX_BUTTONS; ++i)
		{
			const bool visible = show_buttons && i < button_count;
			if (visible)
				_ui->set_widget_text(_button_labels[i], buttons[i].text);
			else
				_ui->clear_widget_text(_button_labels[i]);
		}

		if (content != nullptr && content->init != nullptr)
		{
			_content						= *content;
			_content_active					= true;
			ui::layout_tree_t& tree			= _ui->get_tree();
			ui::layout_in_t&   window_in	= tree.in(_window);
			window_in.size_mode_x			= _content.frame_width_x > 0.0f ? ui::axis_mode_e::parent_relative : ui::axis_mode_e::max_children;
			window_in.size_value.x			= _content.frame_width_x;
			tree.in(_container).size_mode_x = _content.fill_x ? ui::axis_mode_e::parent_relative : ui::axis_mode_e::max_children;
			_content.init(*_ui, _container, _content.user_data);
		}

		set_visible(true);
	}

	void editor_modal_controller_t::set_body_text(const char* text)
	{
		SFG_ASSERT(_ui != nullptr);
		_ui->set_widget_text(_description, text != nullptr ? text : "");
	}

	void editor_modal_controller_t::close_modal()
	{
		SFG_ASSERT(_ui != nullptr);
		if (!_visible)
			return;
		close_content();
		set_visible(false);
		_button_count	 = 0;
		_buttons_visible = false;
		for (editor_modal_button_desc_t& button : _buttons)
			button = {};
	}

	editor_modal_controller_t* editor_modal_controller_t::find(ui::ui_context& ui)
	{
		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i]->_ui == &ui)
				return s_controllers[i];
		}
		return nullptr;
	}

	bool editor_modal_controller_t::is_visible() const
	{
		return _visible;
	}

	void editor_modal_controller_t::set_visible(bool visible)
	{
		ui::layout_tree_t& tree = _ui->get_tree();

		_visible = visible;
		tree.set_visible(_foreground, visible, false);
		tree.set_visible(_container, visible && _content_active, false);
		tree.set_visible(_button_row, visible && _buttons_visible, false);
		for (u32 i = 0; i < MAX_BUTTONS; ++i)
		{
			const bool button_visible = visible && _buttons_visible && i < _button_count;
			tree.set_visible(_button_frames[i], button_visible, button_visible);
		}
	}

	void editor_modal_controller_t::close_content()
	{
		if (!_content_active)
			return;
		if (_content.uninit != nullptr)
			_content.uninit(_content.user_data);
		_content		= {};
		_content_active = false;
		if (_ui != nullptr && _container != NULL_WIDGET)
		{
			ui::layout_tree_t& tree			= _ui->get_tree();
			tree.in(_window).size_mode_x	= ui::axis_mode_e::max_children;
			tree.in(_window).size_value.x	= 0.0f;
			tree.in(_container).size_mode_x = ui::axis_mode_e::max_children;
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
