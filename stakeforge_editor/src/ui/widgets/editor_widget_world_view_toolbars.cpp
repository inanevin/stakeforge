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

#include "ui/widgets/editor_widget_world_view_toolbars.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_edit_context.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	static void set_icon_button_parent_relative_height(ui::layout_tree_t& tree, const editor_icon_button_t& button)
	{
		ui::layout_in_t& in = tree.in(button.get_root());
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value.y		= 1.0f;
	}

	void editor_widget_world_view_toolbars_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "world_view_toolbars");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		_left_column = ui.allocate_widget();
		ui.set_widget_debug_name(_left_column, "world_view_toolbar_left_column");
		tree.attach(_root, _left_column);

		ui::layout_in_t& left_column_in = tree.in(_left_column);
		left_column_in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		left_column_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		left_column_in.pos_value		= {0.0f, 0.0f};
		left_column_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		left_column_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		left_column_in.size_value		= {0.5f, 1.0f};
		left_column_in.flow				= ui::flow_e::column;
		left_column_in.child_spacing	= theme.item_spacing;

		_right_column = ui.allocate_widget();
		ui.set_widget_debug_name(_right_column, "world_view_toolbar_right_column");
		tree.attach(_root, _right_column);

		ui::layout_in_t& right_column_in = tree.in(_right_column);
		right_column_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		right_column_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		right_column_in.pos_value		 = {0.5f, 0.0f};
		right_column_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		right_column_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		right_column_in.size_value		 = {0.5f, 1.0f};
		right_column_in.flow			 = ui::flow_e::column;
		right_column_in.child_spacing	 = theme.item_spacing;

		_top_left_row = ui.allocate_widget();
		ui.set_widget_debug_name(_top_left_row, "world_view_toolbar_top_left_row");
		tree.attach(_left_column, _top_left_row);

		ui::layout_in_t& left_in = tree.in(_top_left_row);
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::fixed;
		left_in.size_value		 = {1.0f, theme.item_area_height * 0.75f};
		left_in.flow			 = ui::flow_e::row;
		left_in.child_spacing	 = 0.0f;

		_top_right_row = ui.allocate_widget();
		ui.set_widget_debug_name(_top_right_row, "world_view_toolbar_top_right_row");
		tree.attach(_right_column, _top_right_row);

		ui::layout_in_t& right_in = tree.in(_top_right_row);
		right_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		right_in.size_mode_y	  = ui::axis_mode_e::fixed;
		right_in.size_value		  = {1.0f, theme.item_area_height};
		right_in.flow			  = ui::flow_e::row;
		right_in.child_spacing	  = 0.0f;

		_global_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_global_frame, "world_view_toolbar_global_frame");
		tree.attach(_top_left_row, _global_frame);

		ui::layout_in_t& global_frame_in = tree.in(_global_frame);
		global_frame_in.pos_mode_x		 = ui::pos_mode_e::flow;
		global_frame_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		global_frame_in.pos_value.y		 = 0.5f;
		global_frame_in.anchor_y		 = ui::anchor_e::center;
		global_frame_in.size_mode_x		 = ui::axis_mode_e::sum_children;
		global_frame_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		global_frame_in.size_value.y	 = 1.0f;
		global_frame_in.flow			 = ui::flow_e::row;
		global_frame_in.child_spacing	 = 0.0f;

		editor_misc_widgets_t::add_spacer(ui, _top_left_row, {theme.item_height, theme.item_height});

		_controls_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_controls_frame, "world_view_toolbar_buttons_frame");
		tree.attach(_top_left_row, _controls_frame);

		ui::layout_in_t& buttons_frame_in = tree.in(_controls_frame);
		buttons_frame_in.pos_mode_x		  = ui::pos_mode_e::flow;
		buttons_frame_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		buttons_frame_in.pos_value.y	  = 0.5f;
		buttons_frame_in.anchor_y		  = ui::anchor_e::center;
		buttons_frame_in.size_mode_x	  = ui::axis_mode_e::sum_children;
		buttons_frame_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		buttons_frame_in.size_value.y	  = 1.0f;
		buttons_frame_in.flow			  = ui::flow_e::row;
		buttons_frame_in.child_spacing	  = 0.0f;

		ui::vg_rect_paint_t buttons_frame_rect = {};
		buttons_frame_rect.fill_color_a		   = {theme.color_frame.x, theme.color_frame.y, theme.color_frame.z, 0.9f};
		buttons_frame_rect.fill_color_b		   = {theme.color_frame.x, theme.color_frame.y, theme.color_frame.z, 0.9f};
		buttons_frame_rect.outline_color	   = theme.color_frame;
		buttons_frame_rect.outline_thickness   = theme.outline_thickness;
		paint.set_rect(_global_frame, buttons_frame_rect);
		paint.set_rect(_controls_frame, buttons_frame_rect);

		editor_icon_button_config_t button_config = {};
		button_config.frame_color				  = {};
		button_config.toggled_frame_color		  = theme.color_accent1_dim;
		button_config.outline_color				  = {};
		button_config.toggled_outline_color		  = {};
		button_config.hover_color				  = theme.color_panel_light1;
		button_config.toggled_hover_color		  = {theme.color_accent1_dim.x, theme.color_accent1_dim.y, theme.color_accent1_dim.z, 0.1f};
		button_config.press_color				  = theme.color_frame_light;
		button_config.icon_color				  = theme.color_text0;
		button_config.size						  = theme.item_area_height;
		button_config.icon_size					  = theme.text_big_px_size;
		button_config.rounding					  = 0.0f;
		button_config.icon						  = ICON_SETTINGS_WHEEL;
		button_config.tooltip					  = "Settings";
		button_config.toggle_enabled			  = false;
		button_config.user_data					  = this;
		button_config.on_clicked				  = on_settings_pressed;
		_settings_button.init(ui, _global_frame, button_config);
		set_icon_button_parent_relative_height(tree, _settings_button);

		button_config.toggle_enabled = true;
		button_config.on_clicked	 = on_transform_control_toggled;

		const struct
		{
			const char*						icon;
			const char*						tooltip;
			editor_transform_control_type_e type;
		} transform_button_specs[3] = {
			{ICON_MOVE, "Move", editor_transform_control_type_e::move},
			{ICON_ROTATE, "Rotate", editor_transform_control_type_e::rotate},
			{ICON_SCALE, "Scale", editor_transform_control_type_e::scale},
		};

		for (size_t i = 0; i < 3; ++i)
		{
			_transform_button_data[i]  = {.toolbar = this, .type = transform_button_specs[i].type};
			button_config.icon		   = transform_button_specs[i].icon;
			button_config.toggled_icon = transform_button_specs[i].icon;
			button_config.tooltip	   = transform_button_specs[i].tooltip;
			button_config.toggled	   = transform_button_specs[i].type == editor_transform_control_type_e::move;
			button_config.user_data	   = &_transform_button_data[i];
			_transform_buttons[i].init(ui, _controls_frame, button_config);
			set_icon_button_parent_relative_height(tree, _transform_buttons[i]);
			editor_dividers_t::add_divider_ver(ui, _controls_frame, theme.border_thickness * 0.5f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);
		}

		button_config.icon		   = ICON_L;
		button_config.toggled_icon = ICON_WORLD;
		button_config.tooltip	   = "Locality";
		button_config.toggled	   = false;
		button_config.user_data	   = this;
		button_config.on_clicked   = on_locality_toggled;
		_locality_button.init(ui, _controls_frame, button_config);
		set_icon_button_parent_relative_height(tree, _locality_button);
		editor_dividers_t::add_divider_ver(ui, _controls_frame, theme.border_thickness * 0.5f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		button_config.icon		   = ICON_MAGNET_OFF;
		button_config.toggled_icon = ICON_MAGNET;
		button_config.tooltip	   = "Snapping";
		button_config.toggled	   = false;
		button_config.on_clicked   = on_snapping_toggled;
		_snapping_button.init(ui, _controls_frame, button_config);
		set_icon_button_parent_relative_height(tree, _snapping_button);

		editor_misc_widgets_t::add_spacer(ui, _top_left_row, {theme.item_height, theme.item_height});

		_view_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_view_frame, "world_view_toolbar_view_frame");
		tree.attach(_top_left_row, _view_frame);

		ui::layout_in_t& view_frame_in = tree.in(_view_frame);
		view_frame_in.pos_mode_x	   = ui::pos_mode_e::flow;
		view_frame_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		view_frame_in.pos_value.y	   = 0.5f;
		view_frame_in.anchor_y		   = ui::anchor_e::center;
		view_frame_in.size_mode_x	   = ui::axis_mode_e::sum_children;
		view_frame_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		view_frame_in.size_value.y	   = 1.0f;
		view_frame_in.flow			   = ui::flow_e::row;
		view_frame_in.child_spacing	   = 0.0f;
		paint.set_rect(_view_frame, buttons_frame_rect);

		button_config.icon		   = ICON_FRAME;
		button_config.toggled_icon = ICON_FRAME;
		button_config.tooltip	   = "Grid";
		button_config.toggled	   = false;
		button_config.on_clicked   = on_grid_toggled;
		_grid_button.init(ui, _view_frame, button_config);
		set_icon_button_parent_relative_height(tree, _grid_button);
		editor_dividers_t::add_divider_ver(ui, _view_frame, theme.border_thickness * 0.5f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		button_config.icon		   = ICON_FRAME_DOTTED;
		button_config.toggled_icon = ICON_FRAME_DOTTED;
		button_config.tooltip	   = "Bounding Boxes";
		button_config.on_clicked   = on_bounding_boxes_toggled;
		_bounding_boxes_button.init(ui, _view_frame, button_config);
		set_icon_button_parent_relative_height(tree, _bounding_boxes_button);
		editor_dividers_t::add_divider_ver(ui, _view_frame, theme.border_thickness * 0.5f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		button_config.icon		   = ICON_CUBES;
		button_config.toggled_icon = ICON_CUBES;
		button_config.tooltip	   = "Physics Debug";
		button_config.on_clicked   = on_physics_debug_toggled;
		_physics_debug_button.init(ui, _view_frame, button_config);
		set_icon_button_parent_relative_height(tree, _physics_debug_button);
	}

	void editor_widget_world_view_toolbars_t::uninit()
	{
		if (_settings_popup.is_initialized())
			editor_popup_controller_t::find(*_ui)->close_popup(false);
		_physics_debug_button.uninit();
		_bounding_boxes_button.uninit();
		_grid_button.uninit();
		_snapping_button.uninit();
		_locality_button.uninit();
		for (editor_icon_button_t& button : _transform_buttons)
			button.uninit();
		_settings_button.uninit();
		_ui->deallocate_widget(_root);

		_ui				= nullptr;
		_edit_world		= {};
		_root			= NULL_WIDGET;
		_left_column	= NULL_WIDGET;
		_right_column	= NULL_WIDGET;
		_top_left_row	= NULL_WIDGET;
		_top_right_row	= NULL_WIDGET;
		_global_frame	= NULL_WIDGET;
		_controls_frame = NULL_WIDGET;
		_view_frame		= NULL_WIDGET;
	}

	void editor_widget_world_view_toolbars_t::set_edit_world(editor_world_handle_t world)
	{
		_edit_world = world;
		if (!_edit_world.is_null())
			refresh();
	}

	void editor_widget_world_view_toolbars_t::set_transform_control_type(editor_transform_control_type_e type)
	{
		editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().set_transform_control_type(type);
		refresh();
	}

	void editor_widget_world_view_toolbars_t::toggle_transform_locality()
	{
		editor_world_edit_context_t& context = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context();
		context.set_transform_locality(context.get_transform_locality() == editor_transform_locality_e::world ? editor_transform_locality_e::local : editor_transform_locality_e::world);
		refresh();
	}

	void editor_widget_world_view_toolbars_t::toggle_transform_snapping()
	{
		editor_world_edit_context_t& context = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context();
		context.set_transform_snapping(context.get_transform_snapping() == editor_transform_snapping_e::default_ ? editor_transform_snapping_e::none : editor_transform_snapping_e::default_);
		refresh();
	}

	void editor_widget_world_view_toolbars_t::refresh()
	{
		const editor_world_edit_context_t& context = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context();
		for (size_t i = 0; i < 3; ++i)
			_transform_buttons[i].set_toggled(_transform_button_data[i].type == context.get_transform_control_type());
		_locality_button.set_toggled(context.get_transform_locality() == editor_transform_locality_e::world);
		_snapping_button.set_toggled(context.get_transform_snapping() == editor_transform_snapping_e::default_);
		_grid_button.set_toggled(context.is_grid_enabled());
		_bounding_boxes_button.set_toggled(context.is_bounding_boxes_enabled());
		_physics_debug_button.set_toggled(context.is_physics_debug_enabled());
	}

	void editor_widget_world_view_toolbars_t::on_transform_control_toggled(bool toggled, void* user_data)
	{
		transform_button_data_t& data = *static_cast<transform_button_data_t*>(user_data);
		data.toolbar->set_transform_control_type(data.type);
	}

	void editor_widget_world_view_toolbars_t::on_settings_pressed(bool toggled, void* user_data)
	{
		editor_widget_world_view_toolbars_t& toolbar = *static_cast<editor_widget_world_view_toolbars_t*>(user_data);
		const editor_theme_t&				 theme	 = editor_theme_t::get();
		const ui::layout_out_t&				 out	 = toolbar._ui->get_tree().out(toolbar._settings_button.get_root());
		editor_popup_controller_t::find(*toolbar._ui)
			->request_custom_popup({
				.install   = on_settings_popup_install,
				.uninstall = on_settings_popup_uninstall,
				.user_data = &toolbar,
				.pos	   = {out.pos.x, out.pos.y + out.size.y + theme.item_spacing},
			});
	}

	void editor_widget_world_view_toolbars_t::on_settings_popup_install(ui::ui_context& ui, ui::widget_id_t parent, void* user_data)
	{
		editor_widget_world_view_toolbars_t& toolbar = *static_cast<editor_widget_world_view_toolbars_t*>(user_data);
		editor_world_edit_context_t&		 context = editor_world_controller_t::get().get_editor_world(toolbar._edit_world)->get_edit_context();
		toolbar._settings_popup.init(ui, parent, {.settings = &context.get_world_view_settings()});
	}

	void editor_widget_world_view_toolbars_t::on_settings_popup_uninstall(ui::ui_context& ui, void* user_data)
	{
		editor_widget_world_view_toolbars_t& toolbar = *static_cast<editor_widget_world_view_toolbars_t*>(user_data);
		toolbar._settings_popup.uninit();
	}

	void editor_widget_world_view_toolbars_t::on_locality_toggled(bool toggled, void* user_data)
	{
		editor_widget_world_view_toolbars_t& toolbar = *static_cast<editor_widget_world_view_toolbars_t*>(user_data);
		editor_world_controller_t::get().get_editor_world(toolbar._edit_world)->get_edit_context().set_transform_locality(toggled ? editor_transform_locality_e::world : editor_transform_locality_e::local);
	}

	void editor_widget_world_view_toolbars_t::on_snapping_toggled(bool toggled, void* user_data)
	{
		editor_widget_world_view_toolbars_t& toolbar = *static_cast<editor_widget_world_view_toolbars_t*>(user_data);
		editor_world_controller_t::get().get_editor_world(toolbar._edit_world)->get_edit_context().set_transform_snapping(toggled ? editor_transform_snapping_e::default_ : editor_transform_snapping_e::none);
	}

	void editor_widget_world_view_toolbars_t::on_grid_toggled(bool toggled, void* user_data)
	{
		editor_widget_world_view_toolbars_t& toolbar = *static_cast<editor_widget_world_view_toolbars_t*>(user_data);
		editor_world_controller_t::get().get_editor_world(toolbar._edit_world)->get_edit_context().set_grid_enabled(toggled);
	}

	void editor_widget_world_view_toolbars_t::on_bounding_boxes_toggled(bool toggled, void* user_data)
	{
		editor_widget_world_view_toolbars_t& toolbar = *static_cast<editor_widget_world_view_toolbars_t*>(user_data);
		editor_world_controller_t::get().get_editor_world(toolbar._edit_world)->get_edit_context().set_bounding_boxes_enabled(toggled);
	}

	void editor_widget_world_view_toolbars_t::on_physics_debug_toggled(bool toggled, void* user_data)
	{
		editor_widget_world_view_toolbars_t& toolbar = *static_cast<editor_widget_world_view_toolbars_t*>(user_data);
		editor_world_controller_t::get().get_editor_world(toolbar._edit_world)->get_edit_context().set_physics_debug_enabled(toggled);
	}
}
