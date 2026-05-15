// Copyright (c) 2025 Inan Evin

#include "panels/editor_secondary_base.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_misc.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_secondary_base_t::init(ui::ui_context& ui, window_runtime_t& runtime)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui							= &ui;
		_runtime					= &runtime;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "secondary_base");
		tree.attach(ui.get_root(), _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};

		_title_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_title_frame, "secondary_title_frame");
		tree.attach(_root, _title_frame);

		ui::layout_in_t& title_in = tree.in(_title_frame);
		title_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		title_in.size_mode_y	  = ui::axis_mode_e::fixed;
		title_in.size_value		  = {1.0f, theme.item_height};
		title_in.flow			  = ui::flow_e::row;
		title_in.child_spacing	  = 0.0f;
		title_in.child_margins	  = {0.0f, 0.0f, 0.0f, theme.margin_horizontal};

		ui::vg_rect_paint_t title_rect = {};
		title_rect.fill_color_a		   = theme.color_frame_light;
		title_rect.fill_color_b		   = theme.color_frame_light;
		paint.set_rect(_title_frame, title_rect);

		const ui::widget_id_t title = ui.allocate_widget();
		ui.set_widget_debug_name(title, "secondary_title");
		tree.attach(_title_frame, title);

		ui::layout_in_t& title_text_in = tree.in(title);
		title_text_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		title_text_in.pos_value.y	   = 0.5f;
		title_text_in.anchor_y		   = ui::anchor_e::center;
		title_text_in.size_mode_x	   = ui::axis_mode_e::max_children;
		title_text_in.size_mode_y	   = ui::axis_mode_e::max_children;

		ui.set_widget_text(title, "Stakeforge");
		ui::ui_render_state_t title_state = {};
		title_state.pipeline			  = theme.shader_glitch_lcd;
		paint.set_text(title, ui.widget_text(title), ui.widget_text_len(title), {.font = theme.font_title, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::lcd}, title_state);

		const ui::widget_id_t spacer = ui.allocate_widget();
		ui.set_widget_debug_name(spacer, "secondary_title_spacer");
		tree.attach(_title_frame, spacer);

		ui::layout_in_t& spacer_in = tree.in(spacer);
		spacer_in.size_mode_x	   = ui::axis_mode_e::fill;
		spacer_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		spacer_in.size_value	   = {1.0f, 1.0f};

		_window_buttons = ui.allocate_widget();
		ui.set_widget_debug_name(_window_buttons, "secondary_window_buttons");
		tree.attach(_title_frame, _window_buttons);

		ui::layout_in_t& buttons_in = tree.in(_window_buttons);
		buttons_in.size_mode_x		= ui::axis_mode_e::fixed;
		buttons_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		buttons_in.size_value		= {theme.item_height * 6.0f, 1.0f};
		buttons_in.flow				= ui::flow_e::row;
		buttons_in.child_spacing	= 0.0f;
		buttons_in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

		const editor_window_buttons_t buttons = editor_misc_widgets_t::add_window_buttons(ui, _window_buttons, theme.color_frame_light, theme.color_accent_err, theme.color_light, theme.color_panel, theme.color_text0, theme.icon_default_px_size);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_click			   = on_minimize_window;
		ui.get_input().set_listener(buttons.minimize_frame, listener);
		listener.on_click = on_maximize_window;
		ui.get_input().set_listener(buttons.maximize_frame, listener);
		listener.on_click = on_close_window;
		ui.get_input().set_listener(buttons.close_frame, listener);

		dock_widget_config_t dock_config   = {};
		dock_config.runtime				   = &runtime;
		dock_config.root_drag_out_behavior = dock_widget_root_drag_out_e::close_window;
		_dock_widget.init(ui, _root, dock_config);
		ui::layout_in_t& dock_in = tree.in(_dock_widget.get_root());
		dock_in.size_mode_y		 = ui::axis_mode_e::fill;
	}

	void editor_secondary_base_t::uninit()
	{
		_dock_widget.uninit();

		_ui->deallocate_widget(_root);

		_ui				= nullptr;
		_runtime		= nullptr;
		_root			= NULL_WIDGET;
		_title_frame	= NULL_WIDGET;
		_window_buttons = NULL_WIDGET;
	}

	bool editor_secondary_base_t::is_window_drag_region(const vec2i16_t& pos) const
	{
		const vec2f_t p = {static_cast<f32>(pos.x), static_cast<f32>(pos.y)};

		const vec4f_t buttons = _ui->get_tree().bounds(_window_buttons);
		if (p.x >= buttons.x && p.x <= buttons.x + buttons.z && p.y >= buttons.y && p.y <= buttons.y + buttons.w)
			return false;

		const vec4f_t title = _ui->get_tree().bounds(_title_frame);
		return p.x >= title.x && p.x <= title.x + title.z && p.y >= title.y && p.y <= title.y + title.w;
	}

	void editor_secondary_base_t::on_minimize_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_secondary_base_t& base = *static_cast<editor_secondary_base_t*>(user_data);
		process::minimize_window(base._runtime->window_handle);
	}

	void editor_secondary_base_t::on_maximize_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_secondary_base_t& base = *static_cast<editor_secondary_base_t*>(user_data);
		process::toggle_maximize_window(base._runtime->window_handle);
	}

	void editor_secondary_base_t::on_close_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_secondary_base_t& base = *static_cast<editor_secondary_base_t*>(user_data);
		base._runtime->set_flag(window_runtime_flags_e::close_requested);
	}
}
