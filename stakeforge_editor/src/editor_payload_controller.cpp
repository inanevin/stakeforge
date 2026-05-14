#include "editor_payload_controller.hpp"
#include "editor_surface.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		constexpr vec2i16_t PAYLOAD_CURSOR_OFFSET = {20, 20};
	}

	void editor_payload_controller_t::init(editor_surface_t& surface)
	{
		SFG_ASSERT(_runtime == nullptr);
		SFG_ASSERT(surface.ui);
		SFG_ASSERT(surface.runtime);

		_runtime					= surface.runtime.get();
		_ui							= surface.ui.get();
		ui::ui_context&		  ui	= *surface.ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_frame, "payload_frame");
		tree.attach(ui.get_root(), _frame);

		ui::layout_in_t& frame_in = tree.in(_frame);
		frame_in.flags			  = ui::wf_overlay;
		frame_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		frame_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		frame_in.size_value		  = {1.0f, 1.0f};
		frame_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = {0.0f, 0.0f, 0.0f, 0.0f};
		rect.fill_color_b		 = {0.0f, 0.0f, 0.0f, 0.0f};
		paint.set_rect(_frame, rect);

		_text_widget = ui.allocate_widget();
		ui.set_widget_debug_name(_text_widget, "payload_text");
		tree.attach(_frame, _text_widget);

		ui::layout_in_t& text_in = tree.in(_text_widget);
		text_in.flags			 = ui::wf_overlay;
		text_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		text_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		text_in.pos_value.y		 = 0.5f;
		text_in.anchor_y		 = ui::anchor_e::center;
		text_in.size_mode_x		 = ui::axis_mode_e::max_children;
		text_in.size_mode_y		 = ui::axis_mode_e::max_children;

		ui.set_widget_text(_text_widget, "");
		paint.set_text(_text_widget,
					   ui.widget_text(_text_widget),
					   ui.widget_text_len(_text_widget),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		surface.payload_root = _frame;
		surface.payload_text = _text_widget;
		process::set_window_visible(surface.runtime->window_handle, false);
	}

	void editor_payload_controller_t::uninit()
	{
		if (_runtime == nullptr)
			return;

		if (_ui != nullptr)
		{
			_ui->deallocate_widget(_frame);
		}

		_runtime			 = nullptr;
		_ui					 = nullptr;
		_frame				 = NULL_WIDGET;
		_text_widget		 = NULL_WIDGET;
		_text				 = {};
		_user_ptr			 = nullptr;
		_unhandled_user_data = nullptr;
		_unhandled_fn		 = nullptr;
		_size_value			 = {};
		_type				 = editor_payload_type_e::panel;
		_active				 = false;
		_mouse_was_down		 = false;
		_listeners.clear();
	}

	void editor_payload_controller_t::tick()
	{
		if (!_active)
			return;

		follow_cursor();

		const bool mouse_down = is_any_mouse_down();
		if (mouse_down)
		{
			_mouse_was_down = true;
			return;
		}

		if (_mouse_was_down)
			drop_payload();
	}

	void editor_payload_controller_t::create_payload(const char* text, editor_payload_type_e type, void* user_ptr, vec2u16_t size_value)
	{
		SFG_ASSERT(_runtime != nullptr);
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(!_active);

		_text			= text != nullptr ? text : "";
		_type			= type;
		_user_ptr		= user_ptr;
		_size_value		= size_value;
		_active			= true;
		_mouse_was_down = is_any_mouse_down();

		_ui->set_widget_text(_text_widget, _text.c_str());
		set_visible(true);
		follow_cursor();
	}

	void editor_payload_controller_t::register_listener(editor_payload_listener_fn fn, void* user_data)
	{
		SFG_ASSERT(fn != nullptr);
		_listeners.push_back({fn, user_data});
	}

	void editor_payload_controller_t::set_unhandled_listener(editor_payload_unhandled_fn fn, void* user_data)
	{
		_unhandled_fn		 = fn;
		_unhandled_user_data = user_data;
	}

	bool editor_payload_controller_t::drop_payload()
	{
		SFG_ASSERT(_runtime != nullptr);
		SFG_ASSERT(_active);

		editor_payload_t payload = {};
		payload.text			 = _text.c_str();
		payload.user_ptr		 = _user_ptr;
		payload.type			 = _type;
		payload.pos				 = process::get_cursor_position();
		payload.size_value		 = _size_value;

		bool accepted = false;
		for (const listener_t& listener : _listeners)
		{
			if (listener.fn(payload, listener.user_data))
			{
				accepted = true;
				break;
			}
		}

		if (!accepted && _unhandled_fn != nullptr)
			_unhandled_fn(payload, _unhandled_user_data);

		_active			= false;
		_mouse_was_down = false;
		_user_ptr		= nullptr;
		_size_value		= {};
		_text.clear();
		set_visible(false);
		return accepted;
	}

	bool editor_payload_controller_t::is_any_mouse_down() const
	{
		return process::is_mouse_down(static_cast<u16>(input_code::mouse_0)) || process::is_mouse_down(static_cast<u16>(input_code::mouse_1)) || process::is_mouse_down(static_cast<u16>(input_code::mouse_2));
	}

	void editor_payload_controller_t::set_visible(bool visible)
	{
		SFG_ASSERT(_runtime != nullptr);
		SFG_ASSERT(_ui != nullptr);

		ui::layout_tree_t& tree		= _ui->get_tree();
		tree.in(_frame).flags		= visible ? static_cast<u16>(ui::wf_visible) : static_cast<u16>(ui::wf_overlay);
		tree.in(_text_widget).flags = visible ? static_cast<u16>(ui::wf_visible) : static_cast<u16>(ui::wf_overlay);
		process::set_window_visible(_runtime->window_handle, visible);
	}

	void editor_payload_controller_t::follow_cursor()
	{
		SFG_ASSERT(_runtime != nullptr);

		const vec2i16_t pos = process::get_cursor_position() + PAYLOAD_CURSOR_OFFSET;
		process::set_window_position(_runtime->window_handle, pos);
	}
}
