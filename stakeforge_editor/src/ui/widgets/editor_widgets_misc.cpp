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
#include "ui/widgets/editor_widgets_misc.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		ui::widget_id_t add_window_button_frame(ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& frame_color, const vec4f_t& hover_color, const vec4f_t& press_color)
		{
			ui::layout_tree_t&	   tree			 = ui.get_tree();
			ui::paint_layer_t&	   paint		 = ui.get_paint();
			const ui::layout_in_t& parent_in	 = tree.in_const(parent);
			const bool			   parent_row	 = parent_in.flow == ui::flow_e::row;
			const bool			   parent_column = parent_in.flow == ui::flow_e::column;
			SFG_ASSERT(parent_row || parent_column);

			const ui::widget_id_t id = ui.allocate_widget();
			tree.attach(parent, id);

			ui::layout_in_t& in = tree.in(id);
			in.flags			= ui::wf_visible | ui::wf_input;
			in.size_mode_x		= parent_row ? ui::axis_mode_e::fill : ui::axis_mode_e::parent_relative;
			in.size_mode_y		= parent_row ? ui::axis_mode_e::parent_relative : ui::axis_mode_e::fill;
			in.size_value		= {1.0f, 1.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = frame_color;
			rect.fill_color_b		 = frame_color;
			paint.set_rect(id, rect);
			paint.set_hover_color(id, hover_color);
			paint.set_press_color(id, press_color);

			return id;
		}
	}

	editor_property_row_t editor_misc_widgets_t::make_property_row(ui::ui_context& ui, ui::widget_id_t parent, f32 indentation)
	{
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		editor_property_row_t row = {};

		row.row = ui.allocate_widget();
		ui.set_widget_debug_name(row.row, "property_row");
		tree.attach(parent, row.row);

		ui::layout_in_t& row_in = tree.in(row.row);
		row_in.flags			= ui::wf_visible;
		row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		row_in.size_mode_y		= ui::axis_mode_e::fixed;
		row_in.size_value		= {1.0f, theme.item_area_height};
		row_in.flow				= ui::flow_e::row;

		row.left = ui.allocate_widget();
		ui.set_widget_debug_name(row.left, "property_row_left");
		tree.attach(row.row, row.left);

		ui::layout_in_t& left_in = tree.in(row.left);
		left_in.flags			 = ui::wf_visible;
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {0.4f, 1.0f};
		left_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal + indentation};
		left_in.flow			 = ui::flow_e::row;

		row.divider = ui.allocate_widget();
		ui.set_widget_debug_name(row.divider, "property_row_divider");
		tree.attach(row.row, row.divider);

		ui::layout_in_t& divider_in = tree.in(row.divider);
		divider_in.flags			= ui::wf_visible;
		divider_in.size_mode_x		= ui::axis_mode_e::fixed;
		divider_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		divider_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		divider_in.size_value		= {theme.divider_thickness * 2, 1.0f};
		divider_in.pos_value.y		= 0.5f;
		divider_in.anchor_y			= ui::anchor_e::center;

		ui::vg_rect_paint_t divider_rect = {};
		divider_rect.fill_color_a		 = theme.color_frame;
		divider_rect.fill_color_b		 = theme.color_frame;
		paint.set_rect(row.divider, divider_rect);

		row.right = ui.allocate_widget();
		ui.set_widget_debug_name(row.right, "property_row_right");
		tree.attach(row.row, row.right);

		ui::layout_in_t& right_in = tree.in(row.right);
		right_in.flags			  = ui::wf_visible;
		right_in.size_mode_x	  = ui::axis_mode_e::fill;
		right_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		right_in.size_value		  = {1.0f, 1.0f};
		right_in.child_spacing	  = theme.item_spacing;
		right_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		right_in.flow			  = ui::flow_e::row;

		return row;
	}

	editor_property_row_t editor_misc_widgets_t::make_property_row_with_label(ui::ui_context& ui, ui::widget_id_t parent, const char* label, bool sub_item, bool remove_button, f32 indentation)
	{
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		editor_property_row_t row		 = make_property_row(ui, parent, indentation);
		ui::layout_in_t&	  row_layout = tree.in(row.left);
		row_layout.child_clip_mode		 = ui::clip_mode_e::cpu_rect;

		if (sub_item)
		{
			row_layout.child_spacing = theme.item_spacing * 0.5f;
			editor_icon_widgets_t::add_sub_item_icon(ui, row.left);
		}

		row.label = ui.allocate_widget();
		ui.set_widget_debug_name(row.label, "property_row_label");
		tree.attach(row.left, row.label);
		tree.draw_order(row.label) = tree.draw_order_const(row.left) + 1;

		ui::layout_in_t& label_in = tree.in(row.label);
		label_in.flags			  = ui::wf_visible | ui::wf_input;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(row.label, label != nullptr ? label : "");
		paint.set_text(row.label,
					   ui.widget_text(row.label),
					   ui.widget_text_len(row.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		if (remove_button)
		{
			row.remove_button = editor_icon_widgets_t::add_naked_icon_button(ui, row.right, ICON_MINUS, theme.item_height * 0.75f, theme.color_text1, theme.color_text0, theme.color_accent1, theme.color_text_disabled);

			ui::layout_in_t& remove_in = tree.in(row.remove_button);
			remove_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
			remove_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
			remove_in.pos_value		   = {1.0f, 0.5f};
			remove_in.anchor_x		   = ui::anchor_e::end;
			remove_in.anchor_y		   = ui::anchor_e::center;
		}

		return row;
	}

	editor_vector_property_row_t editor_misc_widgets_t::make_vector_property_row_with_label(ui::ui_context& ui, ui::widget_id_t parent, const char* label, u32 item_count, bool unfolded, bool sub_item, f32 indentation)
	{
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		editor_vector_property_row_t vector_row	   = {};
		vector_row.row							   = make_property_row(ui, parent, indentation);
		tree.in(vector_row.row.left).child_spacing = theme.item_spacing * 0.5f;

		if (sub_item)
			editor_icon_widgets_t::add_sub_item_icon(ui, vector_row.row.left);

		vector_row.dropdown_button = ui.allocate_widget();
		ui.set_widget_debug_name(vector_row.dropdown_button, "property_row_vector_dropdown");
		tree.attach(vector_row.row.left, vector_row.dropdown_button);

		ui::layout_in_t& icon_frame_in = tree.in(vector_row.dropdown_button);
		icon_frame_in.flags			   = ui::wf_visible | ui::wf_input;
		icon_frame_in.size_mode_x	   = ui::axis_mode_e::fixed;
		icon_frame_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		icon_frame_in.size_value	   = {theme.item_height * 0.5f, 1.0f};

		vector_row.dropdown_icon = editor_icon_widgets_t::add_icon(ui, vector_row.dropdown_button, unfolded ? ICON_DD_DOWN : ICON_DD_RIGHT, theme.icon_default_px_size, theme.color_text1);

		vector_row.label	 = ui.allocate_widget();
		vector_row.row.label = vector_row.label;
		ui.set_widget_debug_name(vector_row.label, "property_row_vector_label");
		tree.attach(vector_row.row.left, vector_row.label);

		ui::layout_in_t& label_in = tree.in(vector_row.label);
		label_in.flags			  = ui::wf_visible | ui::wf_input;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(vector_row.label, label != nullptr ? label : "");
		label_in.size_mode_x  = ui::axis_mode_e::fixed;
		label_in.size_value.x = static_cast<f32>(ui.widget_text_len(vector_row.label)) * theme.text_default_px_size * 0.7f;
		paint.set_text(vector_row.label,
					   ui.widget_text(vector_row.label),
					   ui.widget_text_len(vector_row.label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		ui::layout_in_t& right_in = tree.in(vector_row.row.right);
		right_in.child_spacing	  = theme.item_spacing;

		const ui::widget_id_t filler = ui.allocate_widget();
		ui.set_widget_debug_name(filler, "property_row_vector_controls_filler");
		tree.attach(vector_row.row.right, filler);

		ui::layout_in_t& filler_in = tree.in(filler);
		filler_in.flags			   = ui::wf_visible;
		filler_in.size_mode_x	   = ui::axis_mode_e::fill;
		filler_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		filler_in.size_value	   = {1.0f, 1.0f};

		char	  count_text[32] = {};
		const int count_text_len = std::snprintf(count_text, sizeof(count_text), "%u items", item_count);

		vector_row.count_label = ui.allocate_widget();
		ui.set_widget_debug_name(vector_row.count_label, "property_row_vector_count");
		tree.attach(vector_row.row.right, vector_row.count_label);

		ui::layout_in_t& count_label_in = tree.in(vector_row.count_label);
		count_label_in.flags			= ui::wf_visible;
		count_label_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		count_label_in.pos_value.y		= 0.5f;
		count_label_in.anchor_y			= ui::anchor_e::center;
		count_label_in.size_mode_x		= ui::axis_mode_e::fixed;
		count_label_in.size_value.x		= static_cast<f32>(count_text_len) * theme.text_default_px_size * 0.7f;

		ui.set_widget_text(vector_row.count_label, count_text);
		paint.set_text(vector_row.count_label,
					   ui.widget_text(vector_row.count_label),
					   ui.widget_text_len(vector_row.count_label),
					   {.font = theme.font_default, .color = theme.color_text1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		vector_row.reset_button = editor_icon_widgets_t::add_naked_icon_button(ui, vector_row.row.right, ICON_RESET, theme.item_height * 0.75f, theme.color_text1, theme.color_text0, theme.color_accent1, theme.color_text_disabled);
		vector_row.add_button	= editor_icon_widgets_t::add_naked_icon_button(ui, vector_row.row.right, ICON_PLUS, theme.item_height * 0.75f, theme.color_text1, theme.color_text0, theme.color_accent1, theme.color_text_disabled);

		return vector_row;
	}

	ui::widget_id_t editor_misc_widgets_t::add_spacer(ui::ui_context& ui, ui::widget_id_t parent, const vec2f_t& size)
	{
		ui::layout_tree_t& tree = ui.get_tree();

		const ui::widget_id_t id = ui.allocate_widget();
		tree.attach(parent, id);
		ui.set_widget_debug_name(id, "spacer");

		ui::layout_in_t& in = tree.in(id);
		in.flags			= ui::wf_visible;
		in.size_mode_x		= ui::axis_mode_e::fixed;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= size;

		return id;
	}

	editor_window_buttons_t editor_misc_widgets_t::add_window_buttons(
		ui::ui_context& ui, ui::widget_id_t parent, const vec4f_t& frame_color, const vec4f_t& alternative_frame_color, const vec4f_t& hover_color, const vec4f_t& press_color, const vec4f_t& icon_color, f32 icon_point_size)
	{
		editor_window_buttons_t buttons = {};
		buttons.minimize_frame			= add_window_button_frame(ui, parent, frame_color, hover_color, press_color);
		buttons.minimize_icon			= editor_icon_widgets_t::add_icon(ui, buttons.minimize_frame, ICON_WINDOW_MINIMIZE, icon_point_size, icon_color);
		buttons.maximize_frame			= add_window_button_frame(ui, parent, frame_color, hover_color, press_color);
		buttons.maximize_icon			= editor_icon_widgets_t::add_icon(ui, buttons.maximize_frame, ICON_WINDOW_MAXIMIZE, icon_point_size, icon_color);
		buttons.close_frame				= add_window_button_frame(ui, parent, alternative_frame_color, hover_color, press_color);
		buttons.close_icon				= editor_icon_widgets_t::add_icon(ui, buttons.close_frame, ICON_WINDOW_CLOSE, icon_point_size, icon_color);
		return buttons;
	}
}
