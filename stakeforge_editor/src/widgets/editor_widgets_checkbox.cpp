// Copyright (c) 2025 Inan Evin

#include "widgets/editor_widgets_checkbox.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_icons.hpp"
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_checkbox_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_checkbox_config_t& config)
	{
		_ui		 = &ui;
		_config	 = config;
		_checked = config.checked;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "checkbox");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_input;
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {theme.item_height, theme.item_height};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.outline_color		 = theme.color_outline_light;
		rect.outline_thickness	 = theme.outline_thickness;
		rect.rounding			 = theme.item_rounding;
		rect.rounding_segs		 = 4;
		paint.set_rect(_root, rect);
		paint.set_hover_color(_root, theme.color_panel);
		paint.set_press_color(_root, theme.color_frame_light);
		paint.set_focus_color(_root, theme.color_accent0);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_press			   = on_press;
		ui.get_input().set_listener(_root, listener);

		_check = ui.allocate_widget();
		ui.set_widget_debug_name(_check, "checkbox_check");
		tree.attach(_root, _check);
		tree.draw_order(_check) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& check_in = tree.in(_check);
		check_in.flags			  = ui::wf_overlay;
		check_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		check_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		check_in.pos_value		  = {0.5f, 0.5f};
		check_in.anchor_x		  = ui::anchor_e::center;
		check_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(_check, ICON_CHECK);
		paint.set_text(
			_check, ui.widget_text(_check), ui.widget_text_len(_check), {.font = theme.font_icons, .color = theme.color_accent0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		refresh();
	}

	void editor_checkbox_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui		 = nullptr;
		_root	 = NULL_WIDGET;
		_check	 = NULL_WIDGET;
		_config	 = {};
		_checked = false;
	}

	void editor_checkbox_t::set_checked(bool checked)
	{
		_checked = checked;
		refresh();
	}

	void editor_checkbox_t::refresh()
	{
		_ui->get_tree().in(_check).flags = _checked ? ui::wf_visible | ui::wf_overlay : 0;
	}

	void editor_checkbox_t::on_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_checkbox_t& checkbox = *static_cast<editor_checkbox_t*>(user_data);
		checkbox.set_checked(!checkbox._checked);
		if (checkbox._config.on_changed != nullptr)
			checkbox._config.on_changed(checkbox._checked, checkbox._config.user_data);
	}
}
